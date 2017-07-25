#include "user.h"

#include <WContainerWidget>
#include <WAnchor>
#include <WText>
#include <WVBoxLayout>
#include <WHBoxLayout>
#include <WGroupBox>
#include <WInPlaceEdit>
#include <WComboBox>
#include <WLineEdit>
#include <WPushButton>
#include <WMessageBox>
#include <Wt/Chart/WPieChart>
#include <atomic>
#include <rapidxml/rapidxml.hpp>
#include "translation.h"
#include "userlist.h"
#include "settings.h"

lightforums::user::user() :
	posts_(0)
{
	for (int i = 0; i < (int)ratingSize; i++) rating_[i] = 0;
}

lightforums::user::user(rapidxml::xml_node<>* from) :
	user()
{
	auto getAttribute = [] (rapidxml::xml_node<>* source, const char* name, const char* ifNot) -> const char* {
		rapidxml::xml_attribute<>* got = source->first_attribute(name);
		if (got) return got->value();
		else return ifNot;
	};
	name_ = std::make_shared<std::string>(getAttribute(from, "name", "The Nameless One (error)"));
	password_ = std::make_shared<std::string>(getAttribute(from, "password", "2y$05$KCrvQ0v0KA7iMSH2MUrFReCr7aVFsennHEmeNzztKDa0MVfx5FOSW"));
	salt_ = std::make_shared<std::string>(getAttribute(from, "salt", "0KqKlv0/d9Bx9kGNu0PgL46B588="));
	const char* title = getAttribute(from, "title", nullptr);
	if (!title) title_ = nullptr;
	else title_ = std::make_shared<std::string>(title);
	rapidxml::xml_node<>* descr = from->first_node("description");
	description_ = std::make_shared<std::string>(descr ? descr->value() : "");
	rank_ = (rank)atoi(getAttribute(from, "rank", "0"));

	rapidxml::xml_node<>* ratingNode = from->first_node("ratings");
	if (ratingNode) for (ratingNode = ratingNode->first_node(); ratingNode; ratingNode = ratingNode->next_sibling()) {
		rating rate = (rating)atoi(ratingNode->name());
		if (rate > ratingSize) continue;
		postPath path(ratingNode->value());
		ratings_.insert(std::make_pair(path, rate));
	}
}

lightforums::user::~user() {

}

rapidxml::xml_node<char>* lightforums::user::getNode(rapidxml::xml_document<char>* doc, std::vector<std::shared_ptr<std::string>>& strings) {
	auto saveString = [&] (const char* key, std::shared_ptr<std::string> saved) -> rapidxml::xml_attribute<>* {
		strings.push_back(saved);
		return doc->allocate_attribute(key, saved->c_str());
	};
	auto saveNumber = [&] (const char* key, auto saved) -> rapidxml::xml_attribute<>* {
		std::shared_ptr<std::string> made = std::make_shared<std::string>(std::to_string(saved));
		strings.push_back(made);
		return doc->allocate_attribute(key, made->c_str());
	};
	rapidxml::xml_node<>* made = doc->allocate_node(rapidxml::node_element, "user");
	if (title_) made->append_attribute(saveString("title", title_));
	made->append_attribute(saveString("name", name_));
	made->append_attribute(saveString("password", password_));
	made->append_attribute(saveString("salt", salt_));
	strings.push_back(description_);
	made->append_node(doc->allocate_node(rapidxml::node_element, "description", description_->c_str()));
	made->append_attribute(saveNumber("rank", (int)rank_));
	rapidxml::xml_node<>* ratings = doc->allocate_node(rapidxml::node_element, "ratings");
	for (auto it = ratings_.begin(); it != ratings_.end(); it++) {
		std::shared_ptr<std::string> rate = std::make_shared<std::string>(std::to_string(it->second));
		std::shared_ptr<std::string> rated = std::make_shared<std::string>(it->first.getString());
		strings.push_back(rate); strings.push_back(rated);
		ratings->append_node(doc->allocate_node(rapidxml::node_element, rate->c_str(), rated->c_str()));
	}
	made->append_node(ratings);
	return made;
}

std::string lightforums::user::getTitle() {
	return title_ ? *std::atomic_load(&title_) : "";
}

Wt::WContainerWidget* lightforums::user::makeOverview() const {
	Wt::WContainerWidget* result = new Wt::WContainerWidget();
	std::string& username = *std::atomic_load(&name_);
	Wt::WAnchor* nameWidget = new Wt::WAnchor(Wt::WLink(Wt::WLink::InternalPath, "/" USER_PATH_PREFIX "/" + username), Wt::WString(username), result);
	Wt::WText* titleWidget = new Wt::WText(Wt::WString(*std::atomic_load(&title_)), result);
	Wt::WText* postsWidget = new Wt::WText(Wt::WString(replaceVar(tr::get(tr::SHOW_POSTS), 'X', posts_)), result);
	Wt::WVBoxLayout* layout = new Wt::WVBoxLayout(result);
	layout->addSpacing(Wt::WLength::Auto);
	layout->addWidget(nameWidget);
	layout->addWidget(titleWidget);
	layout->addWidget(postsWidget);
	if (Settings::get().postShowUserRating == Settings::SHOW_ALL) {
		Wt::Chart::WPieChart* chart = makeRatingChart(rating_, result);
		layout->addWidget(chart);
	} else if (Settings::get().postShowUserRating == Settings::SHOW_SMALL) {
		Wt::WText* ratingText = makeRatingOverview(rating_, result);
		layout->addWidget(ratingText);
	}
	layout->addStretch(1);
	return result;
}

Wt::WContainerWidget* lightforums::user::makeGuestOverview(const std::string& name) {
	Wt::WContainerWidget* result = new Wt::WContainerWidget();
	Wt::WText* nameWidget = new Wt::WText(Wt::WString(name), result);
	std::shared_ptr<const std::string> translated = tr::get(tr::SHOW_GUEST);
	Wt::WText* guestWidget = new Wt::WText(Wt::WString(*translated), result);
	Wt::WVBoxLayout* layout = new Wt::WVBoxLayout(result);
	layout->addSpacing(Wt::WLength::Auto);
	layout->addWidget(nameWidget);
	layout->addWidget(guestWidget);
	layout->addStretch(1);
	return result;
}

Wt::WContainerWidget* lightforums::user::show(const std::string& viewer) {
	std::shared_ptr<const user> viewing = userList::get().getUser(viewer);
	Wt::WContainerWidget* result = new Wt::WContainerWidget();
	Wt::WGroupBox* groupBox = new Wt::WGroupBox(Wt::WString(std::atomic_load(&name_)->c_str()), result);
	Wt::WVBoxLayout* outerBox = new Wt::WVBoxLayout(groupBox);

	Wt::WContainerWidget* gridContainer = new Wt::WContainerWidget(groupBox);
	outerBox->addWidget(gridContainer);
	Wt::WGridLayout* grid = new Wt::WGridLayout(gridContainer);
	grid->addWidget(new Wt::WText(Wt::WString(*tr::get(tr::USER_NAME)), gridContainer), 0, 0);

	if (viewing && viewing->rank_ != ADMIN)
		grid->addWidget(new Wt::WText(Wt::WString(*std::atomic_load(&name_)), gridContainer), 0, 1);
	else {
		Wt::WInPlaceEdit* nameEditor = new Wt::WInPlaceEdit(*std::atomic_load(&name_), gridContainer);
		nameEditor->setPlaceholderText(Wt::WString(*tr::get(tr::DONT_KEEP_THIS_EMPTY)));
		nameEditor->saveButton()->setText(Wt::WString(*tr::get(tr::SAVE_CHANGES)));
		nameEditor->saveButton()->clicked().connect(std::bind([=] () {
			const std::string& username = nameEditor->text().toUTF8();
			if (validateUsername(username)) {
				if (!userList::get().getUser(username)) {
					userList::get().renameUser(userList::get().getUser(*name_), username);
				} else {
					 Wt::WMessageBox::show(*tr::get(tr::LOGIN_ERROR), *tr::get(tr::USERNAME_UNAVAILABLE), Wt::Ok);
				}
			}
		}));
		nameEditor->cancelButton()->setText(Wt::WString(*tr::get(tr::DISCARD_CHANGES)));
		nameEditor->setToolTip(Wt::WString(*tr::get(tr::CLICK_TO_EDIT)));
		grid->addWidget(nameEditor, 0, 1);
	}

	grid->addWidget(new Wt::WText(Wt::WString(*tr::get(tr::USER_TITLE)), gridContainer), 1, 0);
	if (viewing && viewing->rank_ == ADMIN) {
		Wt::WInPlaceEdit* rankEdit = makeEditableText(&title_, gridContainer);
		rankEdit->setToolTip(Wt::WString(*tr::get(tr::CLICK_TO_EDIT) + " " + *tr::get(tr::BLANK_FOR_DEFAULT)));
		grid->addWidget(rankEdit, 1, 1);
	} else
		grid->addWidget(new Wt::WText(Wt::WString(getTitle()), gridContainer), 1, 1);
	grid->addWidget(new Wt::WText(Wt::WString(*tr::get(tr::USER_POSTS)), gridContainer), 2, 0);
	grid->addWidget(new Wt::WText(Wt::WString(std::to_string(posts_)), gridContainer), 2, 1);
	grid->addWidget(new Wt::WText(Wt::WString(*tr::get(tr::USER_RANK)), gridContainer), 3, 0);
	if (viewing && viewing->rank_ == ADMIN)
		grid->addWidget(makeEnumEditor((unsigned char*)&rank_, rankSize, tr::RANK_USER, gridContainer), 3, 1);
	else
		grid->addWidget(new Wt::WText(Wt::WString(*tr::get((tr::translatable)(tr::RANK_USER + rank_))), gridContainer), 3, 1);
	grid->setColumnStretch(1, 1);

	Wt::WGroupBox* descrFrame = new Wt::WGroupBox(Wt::WString(*tr::get(tr::USER_DESCRIPTION)), groupBox);
	outerBox->addWidget(descrFrame);
	Wt::WHBoxLayout* descrLayout = new Wt::WHBoxLayout(descrFrame);
	if (viewing && (viewing->rank_ == ADMIN || viewing.get() == this)) {
		Wt::WInPlaceEdit* descrEdit = makeEditableText(&description_, descrFrame);
		descrLayout->addWidget(descrEdit, 1);
		//descrEdit->lineEdit()->setMinimumSize(Wt::WLength(descrFrame->width().Centimeter, Wt::WLength::Centimeter), descrFrame->height()); // How to make it larger when editing? CSS?
	} else {
		descrLayout->addWidget(new Wt::WText(Wt::WString(*std::atomic_load(&description_)), descrFrame), 1);
	}

	Wt::Chart::WPieChart* ratingChart = makeRatingChart(rating_, result);
	ratingChart->setDisplayLabels(Wt::Chart::Outside | Wt::Chart::TextLabel |	Wt::Chart::TextPercentage);
	ratingChart->resize(300, 300);
	outerBox->addWidget(ratingChart);

	return result;
}

void lightforums::user::digestPost(std::shared_ptr<post> digested) {
	std::shared_ptr<user> author = userList::get().getUser(*digested->author_);
	auto found = ratings_.find(digested);
	if (found != ratings_.end()) {
		digested->rating_[found->second]++;
		if (author) {
			author->rating_[found->second]++;
			if (author.get() == this) {
				posts_++;
			}
		}
	}
	if (author.get() == this) {
		posts_++;
	}
	for (auto it = digested->children_.begin(); it != digested->children_.end(); it++) {
		digestPost(it->second);
	}
}

bool lightforums::user::validateUsername(const std::string& name, bool warn) {
	bool fine = true;
	for (unsigned int i = 0; i < name.size(); i++) {
		if ((name[i] >= 'A' && name[i] <= 'Z') || (name[i] >= 'a' && name[i] <= 'z') || (name[i] >= '0' && name[i] <= '9') || name[i] == '_') continue;
		fine = false;
		if (warn) Wt::WMessageBox::show(*tr::get(tr::LOGIN_ERROR), *tr::get(tr::USERNAME_ILLEGAL_CHARACTER), Wt::Ok);
		break;
	}
	return fine;
}

void lightforums::user::ratePost(std::shared_ptr<post> rated, rating rate) {
	postPath path(rated);
	std::shared_ptr<std::string> writer = rated->author_;
	std::shared_ptr<user> author = writer ? userList::get().getUser(*writer) : nullptr;
	auto found = ratings_.find(path);
	if (rate < ratingSize) {
		if (found == ratings_.end()) {
			if (author) author->rating_[rate]++;
			rated->rating_[rate]++;
			ratings_.insert(path, rate);
		}
		else {
			if (author) author->rating_[found->second]--;
			rated->rating_[found->second]--;
			found->second = rate;
			if (author) author->rating_[rate]++;
			rated->rating_[rate]++;
		}
	} else {
		if (found != ratings_.end()) {
			if (author) author->rating_[found->second]--;
			rated->rating_[found->second]--;
		}
		ratings_.erase(path);
	}
}
