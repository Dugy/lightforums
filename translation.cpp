#include "translation.h"
#include <WContainerWidget>
#include <WVBoxLayout>
#include <WInPlaceEdit>
#include <WPushButton>
#include <rapidxml/rapidxml.hpp>

#pragma GCC diagnostic ignored "-Wwrite-strings"

lightforums::tr::tr()
{
	original_[SET_GUEST_POSTING] = "Allow guests to post";
	original_[SET_VIEW_DEPTH] = "Default view depth";
	original_[SET_CAN_EDIT_OWN] = "Minimal rank to edit one's own posts";
	original_[SET_CAN_DELETE_OWN] = "Minimal rank to delete one's own posts";
	original_[SET_CAN_EDIT_OTHER] = "Minimal rank to edit others' posts";
	original_[SET_ALLOW_REGISTER] = "Allow new users to register";
	original_[SHOW_POSTS] = "Posts: X";
	original_[SHOW_GUEST] = "Guest";
	original_[SHOW_REPLIES] = "Show X replies";
	original_[HIDE_REPLIES] = "Hide X replies";
	original_[GO_TO_PARENT] = "See parent post";
	original_[WRITE_AUTHOR_NAME] = "Write your name here";
	original_[WRITE_POST_TITLE] = "Write the title of the post here";
	original_[REPLY_TITLE] = "Re: X";
	original_[POST_A_REPLY] = "Post reply";
	original_[CANCEL_REPLYING] = "Cancel";
	original_[WRITE_A_REPLY] = "Write a reply";
	original_[EDIT_POST] = "Edit post";
	original_[CANCEL_EDITING] = "Cancel editing";
	original_[DELETE_POST] = "Delete post";
	original_[DO_DELETE_POST] = "Are you sure to delete the post?";
	original_[GUEST_NAME] = "X (guest)";
	original_[WRITE_FIRST_REPLY] = "No replies yet, be the first to reply";
	original_[USER_NAME] = "Name:";
	original_[USER_TITLE] = "Title:";
	original_[USER_POSTS] = "Posts:";
	original_[USER_DESCRIPTION] = "Description";
	original_[DO_LOG_IN] = "Log in";
	original_[DO_REGISTER] = "Register";
	original_[DO_LOG_OUT] = "Log out";
	original_[LOGGED_IN_AS] = "Logged in as X";
	original_[WRITE_USERNAME] = "username";
	original_[WRITE_PASSWORD] = "password";
	original_[CANCEL_LOG_IN] = "Cancel log-in";
	original_[LOGIN_ERROR] = "Login error";
	original_[NO_SUCH_USER] = "No user has such a name";
	original_[WRONG_PASSWORD] = "Password for that username is not correct";
	original_[REWRITE_PASSWORD] = "rewrite password";
	original_[USERNAME_UNAVAILABLE] = "This username is not available";
	original_[USERNAME_ILLEGAL_CHARACTER] = "Usernames can contain only characters a-z, A-Z, 0-9 and underscores _";
	original_[REGISTER_ERROR] = "Register error";
	original_[PASSWORDS_DONT_MATCH] = "The two copies of the password don't match";
	original_[CANCEL_ACCOUNT_CREATION] = "Don't create account";
	original_[BACK_TO_MAIN] = "Back to main page";
	original_[MY_ACCOUNT] = "My account";
	original_[CHANGE_SETTINGS] = "Change settings";
	original_[CHANGE_TRANSLATION] = "Change translation";
	original_[CLICK_TO_EDIT] = "Click to edit";
	original_[BLANK_FOR_DEFAULT] = "Leave blank for default";
	original_[SAVE_CHANGES] = "Save";
	original_[DISCARD_CHANGES] = "Discard";
	original_[DONT_KEEP_THIS_EMPTY] = "Don't keep this empty";
	original_[USER_RANK] = "Rank: ";
	original_[RANK_USER] = "User";
	original_[RANK_VALUED_USER] = "Valued user";
	original_[RANK_MODERATOR] = "Moderator";
	original_[RANK_ADMIN] = "Administrator";
}

void  lightforums::tr::init(rapidxml::xml_node<char>* source) {
	for (rapidxml::xml_node<>* node = source->first_node("tr"); node; node = node->next_sibling("tr")) {
		const char* key = node->first_attribute("orig") ? node->first_attribute("orig")->value() : nullptr;
		const char* value = node->first_attribute("new") ? node->first_attribute("new")->value() : nullptr;
		if (!key || !value) continue;
		for (int i = 0; i < (int)translatableMax; i++) {
			if (!strcmp(original_[i], key)) {
				translations_[i] = std::make_shared<std::string>(value);
				break;
			}
		}
	}
}

rapidxml::xml_node<char>*  lightforums::tr::save(rapidxml::xml_document<char>* doc, std::vector<std::shared_ptr<std::string>>& strings) {
	rapidxml::xml_node<>* made = doc->allocate_node(rapidxml::node_element, "translation");
	for (int i = 0; i < (int)translatableMax; i++) if (translations_[i]) {
		std::shared_ptr<std::string> contents  = translations_[i];
		strings.push_back(contents);
		rapidxml::xml_node<>* node = doc->allocate_node(rapidxml::node_element, "tr");
		node->append_attribute(doc->allocate_attribute("orig", original_[i]));
		node->append_attribute(doc->allocate_attribute("new", contents->c_str()));
		made->append_node(node);
	}
	return made;
}

Wt::WContainerWidget* lightforums::tr::edit(const std::string& viewer) {
	Wt::WContainerWidget* result = new Wt::WContainerWidget();
	Wt::WVBoxLayout* layout = new Wt::WVBoxLayout(result);

	for (int i = 0; i < translatableMax; i++) {
		Wt::WInPlaceEdit* editor = new Wt::WInPlaceEdit(result);
		layout->addWidget(editor);
		editor->setPlaceholderText(Wt::WString(original_[i]));
		if (translations_[i]) editor->setText(Wt::WString(*translations_[i]));
		else editor->setText(Wt::WString(""));
		editor->saveButton()->setText(Wt::WString(*tr::get(tr::SAVE_CHANGES)));
		editor->saveButton()->clicked().connect(std::bind([=] () {
			std::shared_ptr<std::string> obtainedText = std::make_shared<std::string>(editor->text().toUTF8());
			if (obtainedText->empty())
				obtainedText = nullptr;
			std::atomic_store(&translations_[i], obtainedText);
		}));
		editor->cancelButton()->setText(Wt::WString(*tr::get(tr::DISCARD_CHANGES)));
		editor->setToolTip(Wt::WString(original_[i]));
	}

	return result;
}
