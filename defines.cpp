#include "defines.h"

#include <WContainerWidget>
#include <WText>
#include <WInPlaceEdit>
#include <WPushButton>
#include <WComboBox>
#include "translation.h"

void lightforums::formatString(const std::string& str, Wt::WContainerWidget* into) {
	new Wt::WText(Wt::WString(str), into);
}

Wt::WInPlaceEdit* lightforums::makeEditableText(std::shared_ptr<std::string>* target, Wt::WContainerWidget* parent) {
	Wt::WInPlaceEdit* result = new Wt::WInPlaceEdit(*target ? *std::atomic_load(target) : "", parent);
	result->setPlaceholderText(Wt::WString(*tr::get(tr::DONT_KEEP_THIS_EMPTY)));
	result->saveButton()->setText(Wt::WString(*tr::get(tr::SAVE_CHANGES)));
	result->saveButton()->clicked().connect(std::bind([=] () {
		std::atomic_store(target, std::make_shared<std::string>(result->text().toUTF8()));
	}));
	result->cancelButton()->setText(Wt::WString(*tr::get(tr::DISCARD_CHANGES)));
	result->setToolTip(Wt::WString(*tr::get(tr::CLICK_TO_EDIT)));
	return result;
}

Wt::WInPlaceEdit* lightforums::makeEditableNumber(unsigned long int* target, Wt::WContainerWidget* parent) {
	Wt::WInPlaceEdit* result = new Wt::WInPlaceEdit(std::to_string(*target), parent);
	result->setPlaceholderText(Wt::WString(*tr::get(tr::DONT_KEEP_THIS_EMPTY)));
	result->saveButton()->setText(Wt::WString(*tr::get(tr::SAVE_CHANGES)));
	result->saveButton()->clicked().connect(std::bind([=] () {
		*target = std::stoi(result->text().toUTF8());
	}));
	result->cancelButton()->setText(Wt::WString(*tr::get(tr::DISCARD_CHANGES)));
	result->setToolTip(Wt::WString(*tr::get(tr::CLICK_TO_EDIT)));
	return result;
}

Wt::WInPlaceEdit* lightforums::makeEditableNumber(unsigned int* target, Wt::WContainerWidget* parent) {
	Wt::WInPlaceEdit* result = new Wt::WInPlaceEdit(std::to_string(*target), parent);
	result->setPlaceholderText(Wt::WString(*tr::get(tr::DONT_KEEP_THIS_EMPTY)));
	result->saveButton()->setText(Wt::WString(*tr::get(tr::SAVE_CHANGES)));
	result->saveButton()->clicked().connect(std::bind([=] () {
		*target = std::stoi(result->text().toUTF8());
	}));
	result->cancelButton()->setText(Wt::WString(*tr::get(tr::DISCARD_CHANGES)));
	result->setToolTip(Wt::WString(*tr::get(tr::CLICK_TO_EDIT)));
	return result;
}

Wt::WComboBox* lightforums::makeRankEditor(rank* changed, Wt::WContainerWidget* parent) {
	Wt::WComboBox* result = new Wt::WComboBox(parent);
	result->addItem(*tr::get(tr::RANK_USER));
	result->addItem(*tr::get(tr::RANK_VALUED_USER));
	result->addItem(*tr::get(tr::RANK_MODERATOR));
	result->addItem(*tr::get(tr::RANK_ADMIN));
	result->setCurrentIndex(*changed);
	result->changed().connect(std::bind([=] () {
		*changed = (rank)result->currentIndex();
	}));
	return result;
}

std::string lightforums::replaceVar(const std::string& str, char X, int x) {
	std::string result;
	for (unsigned int i = 0; i < str.size(); i++) {
		if (str[i] == X && (i == 0 || str[i - 1] == ' ') && (str[i + 1] == 0 || str[i + 1] == ' ')) {
			result += std::to_string(x);
		} else
			result.push_back(str[i]);
	}
	return result;
}

std::string lightforums::replaceVar(const std::string& str, char X, const std::string& x) {
	std::string result;
	for (unsigned int i = 0; i < str.size(); i++) {
		if (str[i] == X && (i == 0 || str[i - 1] == ' ') && (str[i + 1] == 0 || str[i + 1] == ' ')) {
			result += x;
		} else
			result.push_back(str[i]);
	}
	return result;
}

std::vector<std::string> lightforums::splitString(const std::string& splitted, char delimeter) {
	std::vector<std::string> result;
	if (splitted.empty()) return result;
	result.push_back(std::string());
	const char* str = splitted.c_str();
	if (*str == delimeter) str++;
	while (*str != 0) {
		if (*str != delimeter)
			result.back().push_back(*str);
		else
			result.push_back(std::string());
		str++;
	}
	if (result.back().empty()) result.pop_back();
	return result;
}

static const std::string b64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

class invB64Chars
{
public:
	static invB64Chars& getInstance()
	{
		static invB64Chars instance;
		return instance;
	}
	const char* get() {
		return data;
	}
	invB64Chars(invB64Chars const&) = delete;
	void operator=(invB64Chars const&)  = delete;
	~invB64Chars() {
		delete[] data;
	}
private:
	char* data;
	invB64Chars() {
		data = new char[256];
		memset(data, 0, 256);
		for (unsigned int i = 0; i < b64chars.size(); i++) {
			data[b64chars[i]] = i;
		}
	}
};

std::string lightforums::toBase64(const std::string& from) {
	const char* start = from.c_str();
	std::string result;
	for (unsigned int i = 0; i < from.size(); i += 3) {
		const unsigned char* s = reinterpret_cast<const unsigned char*>(start + i);
		char piece[5] = "====";
		piece[0] = b64chars[s[0] >> 2];
		piece[1] = b64chars[((s[0] & 3) << 4) + (s[1] >> 4)];
		if ((long int)&s[1] - (long int)start < from.size()) {
			piece[2] = b64chars[((s[1] & 15) << 2) + (s[2] >> 6)];
			if ((long int)&s[2] - (long int)start < from.size())
				piece[3] = b64chars[s[2] & 63];
		}
		result.append(piece);
	}
	return result;
}

std::string lightforums::fromBase64(const std::string& from) {
	const char* start = from.c_str();
	std::string result;
	const char* invb64 = invB64Chars::getInstance().get();
	for (unsigned int i = 0; i < from.size(); i += 4) {
		const char* s = start + i;
		result.push_back((invb64[s[0]] << 2) | (invb64[s[1]] >> 4));
		if (s[2] != '=') {
			result.push_back((invb64[s[1]] << 4) | (invb64[s[2]] >> 2));
			if (s[3] != '=') result.push_back((invb64[s[2]] << 6) | invb64[s[3]]);
		}
	}
	return result;
}
