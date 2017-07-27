#include "userlist.h"
#include <iostream>
#include "rapidxml.hpp"

lightforums::userList::userList()
{
}

void lightforums::userList::setupUserList(rapidxml::xml_node<>* from) {
	if (from) for (rapidxml::xml_node<>* node = from->first_node("user"); node; node = node->next_sibling("user")) {
		std::shared_ptr<user> made = std::make_shared<user>(node);
		users_.insert(*std::atomic_load(&made->name_), made);
	}
	std::cerr << "Users size " << users_.size() << std::endl;
	if (users_.size() == 0) {
		std::shared_ptr<user> dummy = std::make_shared<user>();
		dummy->name_ = std::make_shared<std::string>("Administrator_President");
		dummy->password_ = std::make_shared<std::string>("$2y$05$KCrvQ0v0KA7iMSH2MUrFReCr7aVFsennHEmeNzztKDa0MVfx5FOSW");
		dummy->salt_ = std::make_shared<std::string>("0KqKlv0/d9Bx9kGNu0PgL46B588=");
		dummy->setTitle("Dictator");
		dummy->description_ = std::make_shared<std::string>("This is the highest post, second only to the Eternal President of Administrators for Life, Ruler of Earth and Surrounding Planets and All Life on Them.");
		dummy->rank_ = ADMIN;
		users_.insert(*dummy->name_, dummy);
	}
}

rapidxml::xml_node<>* lightforums::userList::save(rapidxml::xml_document<>* doc, std::vector<std::shared_ptr<std::string>>& strings) {
	rapidxml::xml_node<>* result = doc->allocate_node(rapidxml::node_element, "users");
	for (auto it = users_.begin(); it != users_.end(); it++) {
		rapidxml::xml_node<>* node = it->second->getNode(doc, strings);
		result->append_node(node);
	}
	return result;
}

void lightforums::userList::digestPost(std::shared_ptr<post> digested) {
	for (auto it = users_.begin(); it != users_.end(); it++) {
		it->second->digestPost(digested);
	}
}

bool lightforums::userList::renameUser(std::shared_ptr<user> who, const std::string& newName) {
	const std::string& oldName = *who->name_;
	auto found = users_.find(newName);
	if (found != users_.end()) return false; // Exists
	users_.insert(newName, who);
	users_.erase(oldName);
	std::shared_ptr<std::string> renamed = std::make_shared<std::string>(newName);
	std::atomic_store(&who->name_, renamed);
	return true;
}

bool lightforums::userList::addUser(std::shared_ptr<user> added) {
	return users_.insert(*added->name_, added);
}

//bool lightforums::userList::deleteUser(const std::string& name) {
//	users_.erase(name);
//	return true;
//}
