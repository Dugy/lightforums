#ifndef SETTINGS_H
#define SETTINGS_H

#include "defines.h"
#include <vector>
#include <memory>
#include "translation.h"

namespace lightforums {

	struct Settings
	{
		static inline Settings& get() {
			static Settings holder;
			return holder;
		}

		void setup(rapidxml::xml_node<char>* from = nullptr);
		rapidxml::xml_node<char>* save(rapidxml::xml_document<char>* doc, std::vector<std::shared_ptr<std::string>>& strings);
		Wt::WContainerWidget* edit(const std::string& viewer);

		bool guestPosting;
		unsigned int viewDepth;
		rank canEditOwn;
		rank canEditOther;
		rank canDeleteOwn;
		bool canRegister;

	private:
		Settings();

		void goThroughAll(std::function<void(bool&, bool, const char*, tr::translatable)> doOnBool, std::function<void(unsigned int&, unsigned int, const char*, tr::translatable)> doOnUint,
						  std::function<void(unsigned long int&, unsigned long int, const char*, tr::translatable)> doOnULint, std::function<void(std::shared_ptr<std::string>&, const char*, const char*, tr::translatable)> doOnString,
						  std::function<void(rank&, rank, const char*, tr::translatable)> doOnRank) {
			doOnBool(guestPosting, true, "guest_posting", tr::SET_GUEST_POSTING);
			doOnUint(viewDepth, 1, "view_depth", tr::SET_VIEW_DEPTH);
			doOnRank(canEditOwn, USER, "can_edit_own", tr::SET_CAN_EDIT_OWN);
			doOnRank(canDeleteOwn, USER, "can_delete_own", tr::SET_CAN_DELETE_OWN);
			doOnRank(canEditOther, MODERATOR, "can_edit_other", tr::SET_CAN_EDIT_OTHER);
			doOnBool(canRegister, true, "allow_register", tr::SET_ALLOW_REGISTER);
		}

		Settings(const Settings&) = delete;
		void operator=(const Settings&) = delete;
	};
}

#endif // SETTINGS_H
