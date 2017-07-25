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

		enum howToDisplay {
			SHOW_ALL,
			SHOW_SMALL,
			SHOW_NOT,
			howToDisplaySize
		};

		bool guestPosting;
		unsigned int viewDepth;
		rank canEditOwn;
		rank canEditOther;
		rank canDeleteOwn;
		bool canRegister;
		bool canBeRated[ratingSize];
		colour rateColour[ratingSize];
		howToDisplay postShowUserRating;
		howToDisplay postShowRating;
		bool profileShowUserRating;
		bool colouriseSmallRating;

	private:
		Settings();

		void goThroughAll(std::function<void(bool&, bool, const char*, tr::translatable)> doOnBool, std::function<void(unsigned int&, unsigned int, const char*, tr::translatable)> doOnUint,
						  std::function<void(unsigned long int&, unsigned long int, const char*, tr::translatable)> doOnULint, std::function<void(std::shared_ptr<std::string>&, const char*, const char*, tr::translatable)> doOnString,
						  std::function<void(unsigned char*, unsigned char, const char*, tr::translatable, unsigned char, tr::translatable)> doOnEnum) {
			doOnBool(guestPosting, true, "guest_posting", tr::SET_GUEST_POSTING);
			doOnUint(viewDepth, 1, "view_depth", tr::SET_VIEW_DEPTH);
			doOnEnum((unsigned char*)&canEditOwn, USER, "can_edit_own", tr::SET_CAN_EDIT_OWN, rankSize, tr::RANK_USER);
			doOnEnum((unsigned char*)&canDeleteOwn, USER, "can_delete_own", tr::SET_CAN_DELETE_OWN, rankSize, tr::RANK_USER);
			doOnEnum((unsigned char*)&canEditOther, MODERATOR, "can_edit_other", tr::SET_CAN_EDIT_OTHER, rankSize, tr::RANK_USER);
			doOnBool(canRegister, true, "allow_register", tr::SET_ALLOW_REGISTER);
			for (unsigned int i = 0; i < (unsigned int)ratingSize; i++) {
				std::string canRateName("cr" + std::to_string(i));
				std::string rateColourName("rc" + std::to_string(i));
				doOnBool(canBeRated[i], true, canRateName.c_str(), (tr::translatable)(tr::RATE_USEFUL + i));
				doOnEnum((unsigned char*)&(rateColour[i]), i, rateColourName.c_str(), (tr::translatable)(tr::RATE_USEFUL + i), colourSize, tr::COLOUR_GREEN);
			}
			doOnEnum((unsigned char*)&postShowRating, SHOW_ALL, "post_show_rating", tr::SET_POST_SHOW_RATING, howToDisplaySize, tr::RATING_SHOW_ALL);
			doOnEnum((unsigned char*)&postShowUserRating, SHOW_SMALL, "post_show_user_rating", tr::SET_POST_SHOW_USER_RATING, howToDisplaySize, tr::RATING_SHOW_ALL);
			doOnBool(profileShowUserRating, true, "profile_show_user_rating", tr::SET_PROFILE_SHOW_USER_RATING);
			doOnBool(colouriseSmallRating, true, "colourise_small_rating", tr::SET_COLOURISE_SMALL_RATING);

		}

		Settings(const Settings&) = delete;
		void operator=(const Settings&) = delete;
	};
}

#endif // SETTINGS_H
