#ifndef USERLIST_H
#define USERLIST_H

#include <string>
#include <vector>
#include <memory>
#include "atomic_unordered_map.h"
#include "user.h"
#include "defines.h"

namespace lightforums {

	class post;

	class userList
	{
	public:
		static inline userList& get() {
			static userList holder;
			return holder;
		}

		void setupUserList(rapidxml::xml_node<>* from);
		rapidxml::xml_node<>* save(rapidxml::xml_document<char>* doc, std::vector<std::shared_ptr<std::string>>& strings);
		void digestPost(std::shared_ptr<post> digested);
		bool renameUser(std::shared_ptr<user> who, const std::string& newName);
		bool addUser(std::shared_ptr<user> added);

		std::shared_ptr<user> getUser(const std::string& name) {
			atomic_unordered_map<std::string, std::shared_ptr<user>>::iterator found = users_.find(name);
			if (found != users_.end()) return found->second;
			return nullptr;
		}

	private:

		userList();

		atomic_unordered_map<std::string, std::shared_ptr<user>> users_;

		userList(const userList&) = delete;
		void operator=(const userList&) = delete;
	};

}

#endif // USERLIST_H
