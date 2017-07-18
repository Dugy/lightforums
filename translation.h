#ifndef TRANSLATION_H
#define TRANSLATION_H

#include <string>
#include <vector>
#include <memory>
#include "defines.h"

namespace lightforums {
	class tr {
	public:
		enum translatable {
			SET_GUEST_POSTING,
			SET_VIEW_DEPTH,
			SET_CAN_EDIT_OWN,
			SET_CAN_DELETE_OWN,
			SET_CAN_EDIT_OTHER,
			SET_ALLOW_REGISTER,
			SHOW_POSTS,
			SHOW_GUEST,
			SHOW_REPLIES,
			HIDE_REPLIES,
			GO_TO_PARENT,
			WRITE_AUTHOR_NAME,
			WRITE_POST_TITLE,
			REPLY_TITLE,
			POST_A_REPLY,
			CANCEL_REPLYING,
			WRITE_A_REPLY,
			GUEST_NAME,
			WRITE_FIRST_REPLY,
			EDIT_POST,
			CANCEL_EDITING,
			DELETE_POST,
			DO_DELETE_POST,
			USER_NAME,
			USER_TITLE,
			USER_POSTS,
			USER_DESCRIPTION,
			DO_LOG_IN,
			DO_REGISTER,
			DO_LOG_OUT,
			LOGGED_IN_AS,
			WRITE_USERNAME,
			WRITE_PASSWORD,
			CANCEL_LOG_IN,
			LOGIN_ERROR,
			NO_SUCH_USER,
			WRONG_PASSWORD,
			REWRITE_PASSWORD,
			USERNAME_UNAVAILABLE,
			USERNAME_ILLEGAL_CHARACTER,
			REGISTER_ERROR,
			PASSWORDS_DONT_MATCH,
			CANCEL_ACCOUNT_CREATION,
			BACK_TO_MAIN,
			MY_ACCOUNT,
			CHANGE_SETTINGS,
			CHANGE_TRANSLATION,
			CLICK_TO_EDIT,
			BLANK_FOR_DEFAULT,
			SAVE_CHANGES,
			DISCARD_CHANGES,
			DONT_KEEP_THIS_EMPTY,
			USER_RANK,
			RANK_USER,
			RANK_VALUED_USER,
			RANK_MODERATOR,
			RANK_ADMIN,
			translatableMax,
		};

		static std::shared_ptr<const std::string> get(translatable what) {
			const std::shared_ptr<const std::string>& translated = getInstance().translations_[what];
			if (translated) return translated;
			return std::make_shared<std::string>(getInstance().original_[what]);
		}

		void init(rapidxml::xml_node<char>* source);
		rapidxml::xml_node<char>* save(rapidxml::xml_document<char>* doc, std::vector<std::shared_ptr<std::string>>& strings);
		Wt::WContainerWidget* edit(const std::string& viewer);

		static inline tr& getInstance() {
			static tr holder;
			return holder;
		}

	private:

		tr();

		std::shared_ptr<std::string> translations_[translatableMax];
		char* original_[translatableMax];

		tr(const tr&) = delete;
		void operator=(const tr&) = delete;
	};

}

#endif // TRANSLATION_H
