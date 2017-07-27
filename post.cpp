#include "post.h"

#include "rapidxml.hpp"
#include <Wt/WContainerWidget>
#include <Wt/WGridLayout>
#include <Wt/WVBoxLayout>
#include <Wt/WHBoxLayout>
#include <Wt/WAnchor>
#include <Wt/WText>
#include <Wt/WPushButton>
#include <Wt/WTextArea>
#include <Wt/WLineEdit>
#include <Wt/WDialog>
#include <Wt/WMessageBox>
#include <Wt/WComboBox>
#include <Wt/Chart/WPieChart>
#include "settings.h"
#include "translation.h"
#include "userlist.h"

lightforums::postPath::postPath(std::shared_ptr<post> from) {
	std::shared_ptr<post> iter = from;
	std::vector<unsigned long int> idRow;
	while (iter->self() != iter->parent_) {
		idRow.push_back(iter->id_);
		iter = iter->parent_;
	}
	idRow.push_back(iter->id_);
	std::vector<unsigned char> result;
	result.reserve(50);
	for (unsigned int i = idRow.size() - 1; i < idRow.size(); i--) {
		bool zero = true;
		for (int j = sizeof(unsigned long int) - 1; j >= 0; j--) {
			unsigned char got = (idRow[i] >> (j << 3)) & 0xff; // Get ith byte
			if (got != 0 && zero) {
				result.push_back(j + 1);
				zero = false;
			}
			if (!zero) {
				result.push_back(got);
			}
			if (got == 0 && j == 0) {
				result.push_back(0);
			}
		}
	}
	size_ = result.size();
	path_ = new unsigned char[size_];
	for (unsigned int i = 0; i < result.size(); i++) {
		path_[i] = result[i];
	}
}

lightforums::post::post(std::shared_ptr<post> parent)
{
	setParent(parent);
	setRatings();
}

lightforums::post::post(std::shared_ptr<post> parent, rapidxml::xml_node<>* node) {
	if (parent != nullptr) parent_ = parent;
	else parent_ = std::shared_ptr<post>(this);
	auto getAttribute = [&] (const char* attr) -> const char* {
		rapidxml::xml_attribute<>* got = node->first_attribute(attr);
		if (!got) return "";
		else return got->value();
	};
	title_ = std::make_shared<std::string>(getAttribute("title"));
	author_ = std::make_shared<std::string>(getAttribute("author"));
	visibility_ = (rank)atoi(getAttribute("visibility"));
	postedAt_ = (time_t)atoi(getAttribute("posted_at"));
	lastActivity_.store(postedAt_);
	sortBy_ = (sortPosts)atoi(getAttribute("sort_by"));
	rapidxml::xml_node<>* textNode = node->first_node("text");
	if (textNode) text_ = std::make_shared<std::string>(textNode->value());
	else text_ = std::make_shared<std::string>("");
	id_ = atoi(getAttribute("id"));
	depth_ = atoi(getAttribute("depth"));
	if (parent_.get() != this) parent->children_.insert(std::make_pair(id_, std::shared_ptr<post>(this)));

	// Update post time of all posts this one replied to
	std::shared_ptr<post> ancestor = parent_;
	do {
		ancestor->lastActivity_ .store(postedAt_);
		ancestor = ancestor->parent_;
	} while (ancestor->parent_ != ancestor);

	// Deal with descendants
	for (rapidxml::xml_node<>* child = node->first_node("post"); child; child = child->next_sibling()) {
		new lightforums::post(self(), child);
	}
	setRatings();
}

lightforums::post::~post()
{
}

void lightforums::post::setRatings() {
	for (unsigned int i = 0; i < (unsigned int)ratingSize; i++) {
		rating_[(rating)i] = 0;
	}
}

std::shared_ptr<lightforums::post> lightforums::post::self() {
	if (parent_.get() == this) return parent_;
	return parent_->children_[id_];
}

void lightforums::post::setParent(std::shared_ptr<post> parent) {
	if (!parent) {
		id_ = 0;
		parent_ = std::shared_ptr<post>(this);
	} else {
		unsigned int freeId;
		do {
			freeId = 0;
			for (auto it = parent->children_.begin(); it != parent->children_.end(); ++it) {
				if (it->first >= freeId) freeId = it->first + 1;
			}
		} while (!parent->children_.insert(std::make_pair(freeId, parent_)));
		parent_ = parent;
		id_ = freeId;
	}
}

rapidxml::xml_node<>* lightforums::post::getNode(rapidxml::xml_document<>* doc, std::vector<std::shared_ptr<std::string>>& strings) {
	auto saveString = [&] (const char* key, std::shared_ptr<std::string> saved) -> rapidxml::xml_attribute<>* {
		strings.push_back(saved);
		return doc->allocate_attribute(key, saved->c_str());
	};
	auto saveNumber = [&] (const char* key, auto saved) -> rapidxml::xml_attribute<>* {
		std::shared_ptr<std::string> made = std::make_shared<std::string>(std::to_string(saved));
		strings.push_back(made);
		return doc->allocate_attribute(key, made->c_str());
	};
	rapidxml::xml_node<>* made = doc->allocate_node(rapidxml::node_element, "post");
	made->append_attribute(saveString("title", title_));
	made->append_attribute(saveString("author", author_));
	made->append_attribute(saveNumber("id", id_));
	made->append_attribute(saveNumber("visibility", visibility_.load()));
	made->append_attribute(saveNumber("depth", depth_.load()));
	made->append_attribute(saveNumber("posted_at", postedAt_.load()));
	made->append_attribute(saveNumber("sort_by", sortBy_));
	doc->allocate_node(rapidxml::node_element, "text", text_->c_str());
	std::shared_ptr<std::string> text(text_); // We don't want it to change while being saved
	made->append_node(doc->allocate_node(rapidxml::node_element, "text", text->c_str()));
	for (auto it = children_.begin(); it != children_.end(); it++) {
		made->append_node(it->second->getNode(doc, strings));
	}
	return made;
}

Wt::WContainerWidget* lightforums::post::build(const std::string& viewer, int depth, bool showParentLink) {
	std::shared_ptr<const user> viewing = userList::get().getUser(viewer);
	if ((!viewing && visibility_ > USER) || (viewing && viewing->rank_ < visibility_)) return nullptr;

	std::shared_ptr<std::string> authorName = std::atomic_load(&author_);
	std::shared_ptr<user> author = userList::get().getUser(*authorName);
	Wt::WContainerWidget* result = new Wt::WContainerWidget();
	Wt::WGridLayout* layout = new Wt::WGridLayout(result);

	if (!authorName->empty()) {
		result->setStyleClass("lightforums-postframe");
		Wt::WContainerWidget* authorWidget = author ? author->makeOverview() : user::makeGuestOverview(*authorName);
		result->addWidget(authorWidget);
		layout->addWidget(authorWidget, 0, 0);
	}

	Wt::WContainerWidget* textArea = new Wt::WContainerWidget(result);
	layout->addWidget(textArea, 0, 1);
	Wt::WVBoxLayout* textLayout = new Wt::WVBoxLayout(textArea);

	Wt::WContainerWidget* titleContainer = new Wt::WContainerWidget(textArea);
	titleContainer->setStyleClass("lightforums-titlebar");
	Wt::WHBoxLayout* titleLayout = new Wt::WHBoxLayout(titleContainer);
	textLayout->addWidget(titleContainer);
	Wt::WAnchor* titleWidget = new Wt::WAnchor(Wt::WLink(Wt::WLink::InternalPath, "/" POST_PATH_PREFIX "/" + postPath(self()).getString()), Wt::WString(*std::atomic_load(&title_)), textArea);
	titleLayout->addWidget(titleWidget);
	titleLayout->addStretch(1);

	Wt::WContainerWidget* nextToTextArea;
	Wt::WHBoxLayout* nextToTextLayout;
	bool showChart = (Settings::get().postShowRating == Settings::SHOW_ALL);
	if (showChart) {
		nextToTextArea = new Wt::WContainerWidget(textArea);
		nextToTextLayout = new Wt::WHBoxLayout(nextToTextArea);
		textLayout->addWidget(nextToTextArea, 1);
	}
	Wt::WContainerWidget* text = new Wt::WContainerWidget(showChart ? nextToTextArea : textArea);

	std::shared_ptr<post> ptrToSelf = self(); // To prevent the post from being distroyed at inappropriate time

	if (viewing && ((author == viewing && viewing->rank_ >= Settings::get().canEditOwn) || (viewing->rank_ > Settings::get().canEditOther && (!author || viewing->rank_ > author->rank_)) || viewing->rank_ == ADMIN)) {
		Wt::WPushButton* editButton = new Wt::WPushButton(Wt::WString(*tr::get(tr::EDIT_POST)), titleContainer);
		titleLayout->addWidget(editButton);
		editButton->clicked().connect(std::bind([=] () {

			Wt::WDialog* dialog = new Wt::WDialog(Wt::WString(*tr::get(tr::EDIT_POST)));
			dialog->setModal(false);
			Wt::WVBoxLayout* layout = new Wt::WVBoxLayout(dialog->contents());

			Wt::WLineEdit* nameEdit = nullptr;
			if (viewing->rank_ == ADMIN) {
				nameEdit = new Wt::WLineEdit(dialog->contents());
				nameEdit->setPlaceholderText(Wt::WString(*tr::get(tr::USER_NAME)));
				nameEdit->setText(Wt::WString(*std::atomic_load(&ptrToSelf->author_)));
				layout->addWidget(nameEdit);
			}

			Wt::WLineEdit* titleEdit = new Wt::WLineEdit(dialog->contents());
			titleEdit->setPlaceholderText(Wt::WString(*tr::get(tr::WRITE_POST_TITLE)));
			titleEdit->setText(Wt::WString(*std::atomic_load(&ptrToSelf->title_)));
			layout->addWidget(titleEdit);
			Wt::WTextArea* textArea = new Wt::WTextArea(dialog->contents());
			textArea->setText(Wt::WString(*std::atomic_load(&ptrToSelf->text_)));
			textArea->setColumns(80);
			//textArea->setRows(5);
			layout->addWidget(textArea);

			Wt::WComboBox* sortCombo = nullptr;
			if (viewing->rank_ == ADMIN) {
				sortCombo = makeEnumEditor((unsigned char*)&ptrToSelf->sortBy_, sortPostsSize, tr::REPLIES_SORT_SOMEHOW, dialog->contents());
				layout->addWidget(sortCombo);
			}

			Wt::WContainerWidget* newButtonContainer = new Wt::WContainerWidget(dialog->contents());
			layout->addWidget(newButtonContainer);
			Wt::WHBoxLayout* buttonLayout = new Wt::WHBoxLayout(newButtonContainer);
			Wt::WPushButton* editButton = new Wt::WPushButton(Wt::WString(*tr::get(tr::EDIT_POST)), newButtonContainer);
			editButton->setDefault(true);
			Wt::WPushButton* dontEditButton = new Wt::WPushButton(Wt::WString(*tr::get(tr::CANCEL_EDITING)), newButtonContainer);
			buttonLayout->addWidget(editButton);
			buttonLayout->addWidget(dontEditButton);
			buttonLayout->addStretch(1);

			editButton->clicked().connect(std::bind([=] () {
				if (nameEdit) {
					const std::string& newAuthorName = nameEdit->text().toUTF8();
					//if (!user::validateUsername(newAuthorName)) return;
					if (author) {
						author->posts_--;
						for (unsigned int i = 0; i < (int)ratingSize; i++) {
							author->rating_[i] -= rating_[i];
						}
						std::atomic_store(&ptrToSelf->author_, std::make_shared<std::string>(newAuthorName));
					} else std::atomic_store(&ptrToSelf->author_, std::make_shared<std::string>(newAuthorName.empty() ?
													newAuthorName : replaceVar(*tr::get(tr::GUEST_NAME), 'X', newAuthorName)));
					std::shared_ptr<user> newAuthor = userList::get().getUser(newAuthorName);
					if (newAuthor) {
						newAuthor->posts_++;
						for (unsigned int i = 0; i < (int)ratingSize; i++) {
							author->rating_[i] += rating_[i];
						}
					}
				}
				if (sortCombo) {
					ptrToSelf->sortBy_ = (sortPosts)sortCombo->currentIndex();
				}
				std::shared_ptr<std::string> newText = std::make_shared<std::string>(textArea->text().toUTF8());
				std::atomic_store(&ptrToSelf->title_, std::make_shared<std::string>(titleEdit->text().toUTF8()));
				std::atomic_store(&ptrToSelf->text_, newText);
				titleWidget->setText(Wt::WString(titleEdit->text()));
				text->clear();
				formatString(*newText, text);
				dialog->accept();
			}));

			dontEditButton->clicked().connect(std::bind([=] () {
				dialog->reject();
			}));

			dialog->finished().connect(std::bind([=] () { delete dialog; }));
			dialog->show();
		}));
	}
	if (viewing && ((author == viewing && viewing->rank_ >= Settings::get().canDeleteOwn) || (viewing->rank_ > Settings::get().canEditOther && (!author || viewing->rank_ > author->rank_)) || viewing->rank_ == ADMIN)) {
		Wt::WPushButton* deleteButton = new Wt::WPushButton(Wt::WString(*tr::get(tr::DELETE_POST)), titleContainer);
		titleLayout->addWidget(deleteButton);
		deleteButton->clicked().connect(std::bind([=] () {

			areYouSureBox(*tr::get(tr::DELETE_POST), *tr::get(tr::DO_DELETE_POST), [=] () -> void {
				ptrToSelf->parent_->children_.erase(ptrToSelf->id_);
				if (author) author->posts_--;
				delete result;
			});
		}));
	}

	formatString(*std::atomic_load(&text_), text);
	if (showChart) nextToTextLayout->addWidget(text, 1);
	else textLayout->addWidget(text, 1);

	if (showChart) {
		Wt::Chart::WPieChart* chart = makeRatingChart(rating_, nextToTextArea);
		nextToTextLayout->addWidget(chart, 0);
	} else if (Settings::get().postShowRating == Settings::SHOW_SMALL) {
		Wt::WText* ratingText = makeRatingOverview(rating_, titleContainer);
		titleLayout->addWidget(ratingText);
	}

	std::shared_ptr<post> copy = self();
	Wt::WContainerWidget* replyArea = new Wt::WContainerWidget(textArea);
	replyArea->setStyleClass("lightforums-replyarea");
	std::cerr << "depth " << depth << std::endl;
	if (depth == 0) {
		hideChildren(viewer, replyArea, copy);
	} else {
		showChildren(viewer, replyArea, copy, depth);
	}
	layout->addWidget(replyArea, 1, 1);
	layout->setColumnStretch(1, 1);


	if (showParentLink) {
		Wt::WContainerWidget* outer = new Wt::WContainerWidget();
		new Wt::WAnchor(Wt::WLink(Wt::WLink::InternalPath, "/" POST_PATH_PREFIX "/" + postPath(parent_).getString()), Wt::WString(*lightforums::tr::get(lightforums::tr::GO_TO_PARENT)), outer);
		outer->addWidget(result);
		result = outer;
	}

	return result;
}

std::string lightforums::post::getLink() {
	std::string address = "/" POST_PATH_PREFIX;
	std::shared_ptr<post> iter = self();
	std::vector<unsigned int> idRow;
	while (iter->self() != iter->parent_) {
		idRow.push_back(iter->id_);
		iter = iter->parent_;
	}
	idRow.push_back(iter->id_);
	for (unsigned int i = idRow.size() - 1; i < idRow.size(); i--)
		address.append("/" + std::to_string(idRow[i]));
	return address;
}

void lightforums::post::showChildren(std::string viewer, Wt::WContainerWidget* container, std::shared_ptr<post> from, unsigned int depth) {
	container->clear();
	Wt::WVBoxLayout* layoutV = new Wt::WVBoxLayout(container);
	Wt::WContainerWidget* buttonsContainer = new Wt::WContainerWidget(container);
	layoutV->addWidget(buttonsContainer);
	Wt::WHBoxLayout* layoutH = new Wt::WHBoxLayout(buttonsContainer);
	Wt::WComboBox* ratingCombo = makeRatingCombo(viewer, buttonsContainer, from);
	if (ratingCombo) layoutH->addWidget(ratingCombo);

	if (from->children_.size() == 0) {
		std::cerr << "No children shown\n";
		Wt::WPushButton* replyButton = addReplyButton(tr::WRITE_FIRST_REPLY, viewer, container, container, from);
		layoutH->addWidget(replyButton);
		layoutH->addStretch(1);
		return;
	}

	Wt::WPushButton* hideRepliesButton = new Wt::WPushButton(Wt::WString(replaceVar(*tr::get(tr::HIDE_REPLIES), 'X', from->children_.size())), buttonsContainer);
	layoutH->addWidget(hideRepliesButton);
	Wt::WPushButton* replyButton = addReplyButton(tr::WRITE_A_REPLY, viewer, container, buttonsContainer, from);
	layoutH->addWidget(replyButton);
	layoutH->addStretch(1);
	
	hideRepliesButton->clicked().connect(std::bind([=] () {
		hideChildren(viewer, container, from);
	}));
	auto addChild = [&] (std::shared_ptr<post> child) {
		Wt::WContainerWidget* replyArea = child->build(viewer, depth - 1);
		if (!replyArea) return;
		container->addWidget(replyArea);
		layoutV->addWidget(replyArea);
	};

	if (from->sortBy_ == SORT_BY_ACTIVITY || from->sortBy_ == SORT_BY_POST_TIME) {
		std::vector<std::shared_ptr<post>> posts;
		for (auto it = from->children_.begin(); it != from->children_.end(); it++) posts.push_back(it->second);
		if (from->sortBy_ == SORT_BY_ACTIVITY) {
			std::sort(posts.begin(), posts.end(), [] (const std::shared_ptr<post>& a, const std::shared_ptr<post>& b) -> bool
			{ return a->lastActivity_ > b->lastActivity_; });
		} else {
			std::sort(posts.begin(), posts.end(), [] (const std::shared_ptr<post>& a, const std::shared_ptr<post>& b) -> bool
			{ return a->postedAt_ > b->postedAt_; });
		}
		for (unsigned int i = 0; i < posts.size(); i++) {
			addChild(posts[i]);
		}
	} else if (from->sortBy_ == SORT_SOMEHOW) {
		for (auto it = from->children_.begin(); it != from->children_.end(); it++) {
			addChild(it->second);
		}
	}
}

void lightforums::post::hideChildren(std::string viewer, Wt::WContainerWidget* container, std::shared_ptr<post> from) {
	container->clear();
	makeRatingCombo(viewer, container, from);
	std::cerr << "Hide children called\n";
	if (from->children_.size() > 0) {
		Wt::WPushButton* showRepliesButton = new Wt::WPushButton(Wt::WString(replaceVar(*tr::get(tr::SHOW_REPLIES), 'X', from->children_.size())), container);
		showRepliesButton->clicked().connect(std::bind([=] () {
			showChildren(viewer, container, from, 1);
		}));
	} else {
		std::cerr << "No children, adding a reply button\n";
		addReplyButton(tr::WRITE_FIRST_REPLY, viewer, container, container, from);
	}
}

Wt::WPushButton* lightforums::post::addReplyButton(tr::translatable title, std::string viewer, Wt::WContainerWidget* container, Wt::WContainerWidget* buttonContainer, std::shared_ptr<post> from) {
	Wt::WPushButton* replyButton = new Wt::WPushButton(Wt::WString(replaceVar(*tr::get(title), 'X', from->children_.size())), buttonContainer);
	std::cerr << "Adding reply button\n";
	std::shared_ptr<user> poster = userList::get().getUser(viewer);
	replyButton->clicked().connect(std::bind([=] () {

		Wt::WDialog* dialog = new Wt::WDialog(*tr::get(tr::WRITE_A_REPLY));
		dialog->setModal(false);
		Wt::WVBoxLayout* layout = new Wt::WVBoxLayout(dialog->contents());

		Wt::WLineEdit* nameEdit;
		if (!poster) {
			nameEdit = new Wt::WLineEdit(dialog->contents());
			nameEdit->setPlaceholderText(Wt::WString(*tr::get(tr::WRITE_AUTHOR_NAME)));
			layout->addWidget(nameEdit);
		}
		Wt::WLineEdit* titleEdit = new Wt::WLineEdit(dialog->contents());
		titleEdit->setPlaceholderText(Wt::WString(*tr::get(tr::WRITE_POST_TITLE)));
		titleEdit->setText(Wt::WString(replaceVar(*tr::get(tr::REPLY_TITLE), 'X', *std::atomic_load(&from->title_))));
		layout->addWidget(titleEdit);
		Wt::WTextArea* textArea = new Wt::WTextArea(dialog->contents());
		textArea->setColumns(80);
		textArea->setRows(5);
		layout->addWidget(textArea);
		Wt::WContainerWidget* newButtonContainer = new Wt::WContainerWidget(dialog->contents());
		layout->addWidget(newButtonContainer);
		Wt::WHBoxLayout* buttonLayout = new Wt::WHBoxLayout(newButtonContainer);
		Wt::WPushButton* postButton = new Wt::WPushButton(*tr::get(tr::POST_A_REPLY), newButtonContainer);
		postButton->setDefault(true);
		Wt::WPushButton* dontPostButton = new Wt::WPushButton(*tr::get(tr::CANCEL_REPLYING), newButtonContainer);
		buttonLayout->addWidget(postButton);
		buttonLayout->addWidget(dontPostButton);
		buttonLayout->addStretch(1);

		postButton->clicked().connect(std::bind([=] () {
			post* reply = new post();
			reply->title_ = std::make_shared<std::string>(titleEdit->text().toUTF8());
			if (poster) {
				reply->author_ = std::make_shared<std::string>(viewer);
				poster->posts_++;
			} else {
				std::string nameGiven = nameEdit->text().toUTF8();
				if (!user::validateUsername(nameGiven)) return;
				reply->author_ = std::make_shared<std::string>(replaceVar(*tr::get(tr::GUEST_NAME), 'X', nameGiven));
			}
			reply->text_ = std::make_shared<std::string>(textArea->text().toUTF8());
			reply->visibility_ = USER;
			reply->depth_ = Settings::get().viewDepth;
			reply->sortBy_ = Settings::get().sortBy;
			reply->postedAt_ = time(nullptr);
			reply->lastActivity_.store(reply->postedAt_);
			std::shared_ptr<post> ancestor = from;
			do {
				ancestor->lastActivity_.store(reply->postedAt_);
				ancestor = ancestor->parent_;
			} while (ancestor->parent_ != ancestor);
			reply->setParent(from);
			showChildren(viewer, container, from, 1);
			dialog->accept();
		}));

		dontPostButton->clicked().connect(std::bind([=] () {
			dialog->reject();
		}));

		dialog->finished().connect(std::bind([=] () { delete dialog; }));
		dialog->show();
	}));
	return replyButton;
}

Wt::WComboBox* lightforums::post::makeRatingCombo(std::string viewer, Wt::WContainerWidget* container, std::shared_ptr<post> from) {
	std::shared_ptr<user> viewing = userList::get().getUser(viewer);
	std::cerr << "Viewer " << viewer << std::endl;
	if (!viewing) return nullptr;
	Wt::WComboBox* made = new Wt::WComboBox(container);
	std::vector<rating> available;
	auto ratingFound = viewing->ratings_.find(from);
	for (unsigned int i = 0; i < (unsigned int)ratingSize; i++) if (Settings::get().canBeRated[i]) {
		made->addItem(*tr::get((tr::translatable)(tr::RATE_USEFUL + i)));
		available.push_back((rating)i);
		if (ratingFound != viewing->ratings_.end() && ratingFound->second == i) made->setCurrentIndex(available.size() - 1);
	}
	made->addItem(*tr::get(tr::NOT_RATED_YET));
	if (ratingFound == viewing->ratings_.end()) made->setCurrentIndex(available.size());
	available.push_back(ratingSize);
	made->changed().connect(std::bind([=] () {
		viewing->ratePost(from, available[made->currentIndex()]);
	}));
	return made;
}
