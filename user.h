#ifndef USER_H
#define USER_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <memory>
#include "rapidxml.hpp"
#include "defines.h"
#include "atomic_unordered_map.h"
#include "post.h"

namespace lightforums {

	class user
	{
	public:
		user();
		user(rapidxml::xml_node<>* from);
		~user();
		rapidxml::xml_node<char>* getNode(rapidxml::xml_document<char>* doc, std::vector<std::shared_ptr<std::string>>& strings);

		std::shared_ptr<std::string> name_;
		std::shared_ptr<std::string> password_;
		std::shared_ptr<std::string> salt_;
		std::shared_ptr<std::string> description_;
		rank rank_;
		std::atomic_uint_fast32_t posts_;
		std::atomic_int rating_[ratingSize];
		atomic_unordered_map<postPath, rating> ratings_;

		Wt::WContainerWidget* makeOverview() const;
		static Wt::WContainerWidget* makeGuestOverview(const std::string& name);
		Wt::WContainerWidget* show(const std::string& viewer);
		void digestPost(std::shared_ptr<post> digested);
		void ratePost(std::shared_ptr<post> rated, rating rate);

		static bool validateUsername(const std::string& name, bool warn = true);

		std::string getTitle() const;
		void setTitle(const std::string& newName) { title_ = std::make_shared<std::string>(newName); }

	private:
		std::shared_ptr<std::string> title_;

		friend class userProxy;
	};

}

#endif // USER_H
