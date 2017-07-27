#include "defines.h"

#include <Wt/WContainerWidget>
#include <Wt/WText>
#include <Wt/WInPlaceEdit>
#include <Wt/WPushButton>
#include <Wt/WComboBox>
#include <Wt/WColor>
#include <Wt/WPen>
#include <Wt/Chart/WPieChart>
#include <Wt/Chart/WChartPalette>
#include <Wt/WStandardItemModel>
#include <Wt/WStandardItem>
#include <Wt/WPanel>
#include <Wt/WAnimation>
#include <Wt/WDialog>
#include <Wt/WVBoxLayout>
#include <Wt/WHBoxLayout>
#include "settings.h"
#include "translation.h"

void lightforums::formatString(const std::string& str, Wt::WContainerWidget* into) {
	enum markup : unsigned int {
		PLAIN = 0x0,
		HEADING = 0x1,
		SPOILER = 0x2,
		ITALIC = 0x4,
		BOLD = 0x8,
		CODE = 0x10,
		STRIKETHROUGH = 0x20,
		SUBSCRIPT = 0x40,
		SUPERSCRIPT = 0x80,
		LINK = 0x100,
	};
	struct node {
		markup style;
		std::string text;
		std::string path;
		std::vector<node> deeper;
		node(const std::string& source, markup styleOutside = PLAIN, std::string linkPath = "") : style(styleOutside) {
			if (styleOutside == CODE) {
				text = source;
				return;
			}
			path = linkPath;
			std::pair<char, markup> pairMarkups[] = {{'_', ITALIC}, {'*', BOLD}, {'-', STRIKETHROUGH}, {'^', SUPERSCRIPT}, {'~', SUBSCRIPT}};
			auto getMarkup = [&] (const char character) -> markup {
				for (int i = 0; i < 5; i++) if (character == pairMarkups[i].first) return pairMarkups[i].second;
				return PLAIN; // PLAIN == 0 == false
			};

			std::string soFar;
			auto pushSoFar = [&] () -> void {
				if (soFar.empty()) return;
				deeper.push_back(node(soFar, PLAIN));
				soFar.clear();
			};
			auto increment = [&] (const char*& str) { // special decrement/increment to deal with code
				str++;
				if (*str == '`' && (str == source.c_str() || str[-1] != '//')) {
					do str++; while (*str && (*str != '`' || (str[-1] != '//')));
					str++;
				}
			};
			auto decrement = [&] (const char*& str) {
				if (str > source.c_str()) str--;
				if (*str == '`' && str[-1] != '\\') {
					do str--; while ((*str != '`' || (str != source.c_str() && str[-1] != '//')) && str >= source.c_str());
					str--;
				}
			};

			for (const char* str = source.c_str(); *str; str++) {
				if ((str == source.c_str() || str[-1] == '\n') && *str == '!') {
					pushSoFar();
					const char* strIn = str + 1;
					for ( ; *strIn && (*strIn != '\n' || strIn[-1] == '\\'); increment(strIn)) {
						soFar.push_back(*strIn);
					}
					if (!*strIn) continue;
					deeper.push_back(node(soFar, HEADING, path));
					soFar.clear();
					str = strIn;
				} else if ((str == source.c_str() || str[-1] != '\\') && *str == '{') {
					pushSoFar();
					const char* strIn = str + 1;
					int depth = 0;
					for ( ; *strIn; increment(strIn)) {
						if (*strIn == '{' && strIn[-1] != '\\') depth++;
						if (*strIn == '}' && strIn[-1] != '\\') {
							if (!depth) break;
							else depth--;
						}
						soFar.push_back(*strIn);
					}
					if (!*strIn) continue;
					deeper.push_back(node(soFar, SPOILER, path));
					soFar.clear();
					str = strIn;
				}
				else if ((str == source.c_str() || str[-1] != '\\') && *str == '[') {
					pushSoFar();
					const char* strIn = str + 1;
					int depth = 0;
					for ( ; *strIn; increment(strIn)) {
						if (*strIn == '[' && strIn[-1] != '\\') depth++;
						if (*strIn == '|' && strIn[-1] != '\\') {
							if (!depth) break;
							else depth--;
						}
						soFar.push_back(*strIn);
					}
					if (!*strIn) continue;
					str = strIn + 1;
					std::string foundPath;
					for ( ; *str && *str != ']'; str++) foundPath.push_back(*str);
					deeper.push_back(node(soFar, LINK, foundPath));
					soFar.clear();
				} else {
					if (str == source.c_str() || str[-1] == ' ' || str[-1] == '\t' || str[-1] == '\n') {
						markup got = getMarkup(*str);
						if (got) {
							bool isMarkup = false;
							for (const char* innerIt = str + 1; *innerIt != ' ' && *innerIt != '\t' && *innerIt != '\n'; increment(innerIt)) {
								if (!getMarkup(*innerIt)) {
									isMarkup = true;
									break;
								}
							}
							if (isMarkup) {
								for (const char* innerIt = str; *innerIt; increment(innerIt)) {
									if (*innerIt == *str && (innerIt[1] == ' ' || innerIt[1] == '\t' || innerIt[1] == '\n' || innerIt[1] == '.' || innerIt[1] == ','
															 || innerIt[1] == ';' || innerIt[1] == '?' || innerIt[1] == '!' || innerIt[1] == 0) && innerIt[-1] != '\\') {
										isMarkup = false;
										for (const char* innerInnerIt = innerIt; innerInnerIt > str; decrement(innerInnerIt)) {
											if (*innerIt == ' ' || *innerIt == '\n' || *innerIt == '\t') break;
											else if (!getMarkup(*innerInnerIt)) {
												isMarkup = true;
												break;
											}
										}
										if (isMarkup) {
											std::string added;
											for ( str++ ; str != innerIt; increment(str)) added.push_back(*str);
											pushSoFar();
											deeper.push_back(node(added, got));
											break;
										} else soFar.push_back(*str);
									}
								}
								continue;
							}
						}
					}
					if (*str == '`' && (str == source.c_str() || str[-1] != '\\')) {
						str++;
						std::string added;
						for ( ; *str != '`'; str++) added.push_back(*str);
						pushSoFar();
						deeper.push_back(node(added, CODE));
					} else soFar.push_back(*str);
				}
			}

			if (deeper.empty()) text.swap(soFar);
			else pushSoFar();
		}
		void chew() {
			if (text.empty()) {
				for (unsigned int i = 0; i < deeper.size(); i++) deeper[i].chew();
			} else {
				std::string newText;
				for (const char* str = text.c_str(); *str; str++) {
					if (*str == '\n') {
						newText.append("<br/>");
					} else if (*str == '\\' && str[1] == '\\') {
						newText.push_back('\\');
						str++;
					} else if (*str == ' ' && str[1] == ' ') {
						newText.append("&nbsp;");
						str++;
					} else if (*str == '<') {
						newText.append("&lt;");
					} else if (*str == '>') {
						newText.append("&gt;");
					} else if (*str == '&') {
						newText.append("&#38;");
					} else if (*str == '"') {
						newText.append("&#34;");
					} else if (*str == '\'') {
						newText.append("&#39;");
					} else if (*str == '\\' && (str[1] == '_' || str[1] == '*' || str[1] == '-' || str[1] == '~' || str[1] == '-' || str[1] == '`' || str[1] == '!' || str[1] == '{' || str[1] == '}' || str[1] == ']' || str[1] == '|')) {}
					else newText.push_back(*str);
				}
				text.swap(newText);
			}
		}

		void construct(Wt::WContainerWidget *into, unsigned int flags = 0x0) {
			if (deeper.empty()) {
				std::string prefix;
				std::string suffix;
				if (flags & CODE) {
					new Wt::WText(Wt::WString("<tt>" + text + "</tt>"), into);
					std::cerr << "Code is " << text << std::endl;
				} else {
					if (flags & ITALIC) { prefix += "<em>"; suffix = "</em>" + suffix; }
					if (flags & BOLD) { prefix += "<strong>"; suffix = "</strong>"  + suffix; }
					if (flags & STRIKETHROUGH) { prefix += "<strike>"; suffix = "</strike>" + suffix; }
					if (flags & SUPERSCRIPT) { prefix += "<sup>"; suffix = "</sup>" + suffix; }
					if (flags & SUBSCRIPT) { prefix += "<sub>"; suffix = "</sub>" + suffix; }
					if (flags & HEADING) { prefix += "<h1>"; suffix = "</h1>" + suffix; }
					if (flags & LINK) {
						new Wt::WAnchor(Wt::WLink(path), Wt::WString(prefix + text + suffix), into);
					} else new Wt::WText(Wt::WString(prefix + text + suffix), into);
				}
			} else {
				for (unsigned int i = 0; i < deeper.size(); i++) {
					if (deeper[i].style == PLAIN) deeper[i].construct(into, flags); // Most common, let it be dealt with easily
					else if (deeper[i].style == SPOILER) {
						Wt::WPanel* panel = new Wt::WPanel(into);
						panel->setTitle(Wt::WString(*tr::get(tr::SPOILER_TITLE)));
						panel->setCollapsible(true);
						Wt::WAnimation animation(Wt::WAnimation::SlideInFromTop, Wt::WAnimation::EaseOut, 100);
						panel->setAnimation(animation);
						Wt::WContainerWidget* container = new Wt::WContainerWidget(into);
						deeper[i].construct(container, flags);
						panel->setCentralWidget(container);
						panel->setCollapsed(true);
					} else {
						deeper[i].construct(into, flags ^ deeper[i].style);
					}
				}
			}
		}
	};

	node parsed(str);
	parsed.chew();
	parsed.construct(into);
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

Wt::WComboBox* lightforums::makeEnumEditor(unsigned char* changed, unsigned char elements, unsigned int first, Wt::WContainerWidget* parent) {
	Wt::WComboBox* result = new Wt::WComboBox(parent);
	for (unsigned int i = 0; i < elements; i++) {
		result->addItem(*tr::get((tr::translatable)(first + i)));
	}

	result->setCurrentIndex(*changed);
	result->changed().connect(std::bind([=] () {
		*changed = result->currentIndex();
	}));
	return result;
}

namespace lightforums {

	Wt::WStandardItemModel* makeModel(const std::atomic_int* data, Wt::WContainerWidget* parent) {
		Wt::WStandardItemModel* model = new Wt::WStandardItemModel(parent);
		//model->setItemPrototype(new NumericItem());

		model->insertColumns(model->columnCount(), 2);
		model->setHeaderData(0, Wt::WString(*tr::get(tr::RATING_TITLE)));
		model->setHeaderData(1, Wt::WString(*tr::get(tr::RATING_VALUE)));

		Settings& settings = Settings::get();
		std::vector<rating> available;
		for (unsigned int i = 0; i < (unsigned int)ratingSize; i++) {
			if (settings.canBeRated[i] && data[i].load() != 0) available.push_back((rating)i);
		}
		model->insertRows(model->rowCount(), available.size());
		for (unsigned int i = 0; i < available.size(); i++) {
			model->setData(i, 0, Wt::WString(*tr::get((tr::translatable)(tr::RATE_USEFUL + available[i]))));
			model->setData(i, 1, data[available[i]].load());
		}

		return model;
	}

	class ratingPalette : public Wt::Chart::WChartPalette {
		std::vector<rating> available_;
	public:
		ratingPalette() : Wt::Chart::WChartPalette() {
			Settings& settings = Settings::get();
			std::vector<rating> available;
			for (unsigned int i = 0; i < (unsigned int)ratingSize; i++) {
				if (settings.canBeRated[i]) available.push_back((rating)i);
			}
		}

		virtual Wt::WBrush brush(int index) {
			return Wt::WBrush(getColour(Settings::get().rateColour[available_[index]]));
		}
		virtual Wt::WPen borderPen(int index) {
			return Wt::WPen(Wt::black);
		}
		virtual Wt::WPen strokePen(int index) {
			return Wt::WPen(Wt::black);
		}
		virtual Wt::WColor fontColor(int index) {
			return getColour(Settings::get().rateColour[available_[index]]);
		}
		virtual ~ratingPalette() { }
	};
}

Wt::Chart::WPieChart* lightforums::makeRatingChart(const std::atomic_int* data, Wt::WContainerWidget* parent) {
	Wt::Chart::WPieChart* chart = new Wt::Chart::WPieChart(parent);
	chart->setModel(makeModel(data, parent));
	chart->setLabelsColumn(0);
	chart->setDataColumn(1);
	chart->setPerspectiveEnabled(true, 0.2);
	chart->setShadowEnabled(true);
	chart->resize(100, 100);
//	chart->setDisplayLabels(Wt::Chart::Outside |
//							Wt::Chart::TextLabel |
//							Wt::Chart::TextPercentage);
	//chart->setMargin(10, Wt::Top | Wt::Bottom);
	//chart->setMargin(Wt::WLength::Auto, Wt::Left | Wt::Right);
	return chart;
}

Wt::WText* lightforums::makeRatingOverview(const std::atomic_int* data, Wt::WContainerWidget* parent) {
	int sum = 0;
	int good = 0;
	rating predominant;
	int max = 0;
	for (unsigned int i = 0; i < ratingSize; i++) {
		sum += data[i];
		if (i <= INTERESTING_POST) good += data[i];
		if (data[i] > max) {
			predominant = (rating)i;
			max = data[i];
		}
	}
	if (sum == 0) return new Wt::WText(Wt::WString(*tr::get(tr::NOT_RATED_YET)), parent);
	int goodPercentage = (good * 100) / sum;

	std::string text;
	if (Settings::get().colouriseSmallRating) {
		Wt::WColor colour = getColour(Settings::get().rateColour[predominant]);
		text += "<font color=\"rgb(" + std::to_string(colour.red()) + "," + std::to_string(colour.green()) + "," + std::to_string(colour.blue()) + "\">";
	}
	text += replaceVar(*tr::get(tr::RATED_AS_X), 'X', goodPercentage);
	if (Settings::get().colouriseSmallRating) text += "</font>";
	return new Wt::WText(Wt::WString(text), parent);
}

void lightforums::messageBox(const std::string& title, const std::string& text) {
	Wt::WDialog* dialog = new Wt::WDialog(Wt::WString(title));
	dialog->setModal(false);
	Wt::WVBoxLayout* layout = new Wt::WVBoxLayout(dialog->contents());

	Wt::WText* announcement = new Wt::WText(Wt::WString(text), dialog->contents());
	layout->addWidget(announcement);

	Wt::WContainerWidget* buttonContainer = new Wt::WContainerWidget(dialog->contents());
	layout->addWidget(buttonContainer);
	Wt::WHBoxLayout* buttonLayout = new Wt::WHBoxLayout(buttonContainer);
	buttonLayout->addStretch(1);
	Wt::WPushButton* okButton = new Wt::WPushButton(Wt::WString(*tr::get(tr::GENERIC_OK)), buttonContainer);
	okButton->setDefault(true);
	buttonLayout->addWidget(okButton);
	buttonLayout->addStretch(1);

	okButton->clicked().connect(std::bind([=] () {
		dialog->accept();
	}));

	dialog->finished().connect(std::bind([=] () { delete dialog; }));

	dialog->show();
}

void lightforums::areYouSureBox(const std::string& title, const std::string& text, std::function<void ()> acceptFunc, std::function<void ()> rejectFunc){
	Wt::WDialog* dialog = new Wt::WDialog(Wt::WString(title));
	dialog->setModal(false);
	Wt::WVBoxLayout* layout = new Wt::WVBoxLayout(dialog->contents());

	Wt::WText* announcement = new Wt::WText(Wt::WString(text), dialog->contents());
	layout->addWidget(announcement);

	Wt::WContainerWidget* buttonContainer = new Wt::WContainerWidget(dialog->contents());
	layout->addWidget(buttonContainer);
	Wt::WHBoxLayout* buttonLayout = new Wt::WHBoxLayout(buttonContainer);
	Wt::WPushButton* continueButton = new Wt::WPushButton(Wt::WString(*tr::get(tr::GENERIC_CONTINUE)), buttonContainer);
	continueButton->setDefault(true);
	buttonLayout->addWidget(continueButton);
	buttonLayout->addStretch(1);
	Wt::WPushButton* cancelButton = new Wt::WPushButton(Wt::WString(*tr::get(tr::GENERIC_CANCEL)), buttonContainer);
	cancelButton->setDefault(true);
	buttonLayout->addWidget(cancelButton);

	continueButton->clicked().connect(std::bind([=] () { acceptFunc(); dialog->accept(); }));
	if (rejectFunc)
		cancelButton->clicked().connect(std::bind([=] () { rejectFunc(); dialog->reject(); }));
	else
		cancelButton->clicked().connect(std::bind([=] () { dialog->reject(); }));

	dialog->finished().connect(std::bind([=] () { delete dialog; }));

	dialog->show();
}

std::string lightforums::replaceVar(const std::string& str, char X, int x) {
	std::string result;
	for (unsigned int i = 0; i < str.size(); i++) {
		if (str[i] == X && (i == 0 || str[i - 1] == ' ') && (str[i + 1] == 0 || str[i + 1] == ' ' || str[i + 1] == '%')) {
			result += std::to_string(x);
		} else
			result.push_back(str[i]);
	}
	return result;
}

std::string lightforums::replaceVar(const std::string& str, char X, const std::string& x) {
	std::string result;
	for (unsigned int i = 0; i < str.size(); i++) {
		if (str[i] == X && (i == 0 || str[i - 1] == ' ') && (str[i + 1] == 0 || str[i + 1] == ' ' || str[i + 1] == '%')) {
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

const Wt::WColor& lightforums::getColour(colour col) {
	if (col >= colourSize) throw(std::runtime_error("Weird colour requested"));
	static const Wt::WColor colours[] = {Wt::green, Wt::gray, Wt::blue, Wt::red, Wt::WColor(255, 0, 255), Wt::WColor(255, 128, 0), Wt::black, Wt::cyan, Wt::yellow, Wt::white};
	return colours[col];
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
