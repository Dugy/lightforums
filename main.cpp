#include <iostream>
#include <fstream>
#include "mainwindow.h"
#include <rapidxml/rapidxml.hpp>
#include <rapidxml/rapidxml_print.hpp>
#include "settings.h"
#include "post.h"
#include "userlist.h"

void setupStructures(const std::string& fileName) {
	std::ifstream in(fileName);
	if (in.is_open()) {
		std::cerr << "Could open file\n";
		std::string source((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		rapidxml::xml_document<> docSource;
		docSource.parse<0>((char*)source.c_str());
		rapidxml::xml_node<>* parent = docSource.first_node();
		if (parent) {
			std::cerr << "All right, reading\n";
			rapidxml::xml_node<>* settingsNode = parent->first_node("settings");
			lightforums::Settings::get().setup(settingsNode);
			rapidxml::xml_node<>* usersNode = parent->first_node("users");
			lightforums::userList::get().setupUserList(usersNode);
			rapidxml::xml_node<>* postsNode = parent->first_node("posts");
			if (!postsNode) return;
			rapidxml::xml_node<>* postNode = postsNode->first_node("post");
			if (!postNode) return;
			lightforums::post* firstPost = new lightforums::post(nullptr, postNode);
			root::get().setRootPost(firstPost->self());
			lightforums::userList::get().digestPost(root::get().getRootPost());
			rapidxml::xml_node<>* translationsNode = parent->first_node("translation");
			if (translationsNode) lightforums::tr::getInstance().init(translationsNode);
			rapidxml::xml_node<>* cookiesNode = parent->first_node("cookies");
			if (cookiesNode) for (rapidxml::xml_node<>* cookie = cookiesNode->first_node("cookie"); cookie; cookie = cookie->next_sibling("cookie")) {
				rapidxml::xml_attribute<>* token = cookie->first_attribute("token");
				rapidxml::xml_attribute<>* user = cookie->first_attribute("user");
				if (!token || !user) continue;
				root::get().cookies_.insert(token->value(), user->value());
			}
			return;
		}
	}
	lightforums::Settings::get().setup();
}

void saveStructures(const std::string& fileName) {
	rapidxml::xml_document<> doc;
	rapidxml::xml_node<>* decl = doc.allocate_node(rapidxml::node_declaration);
	decl->append_attribute(doc.allocate_attribute("version", "1.0"));
	decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
	doc.append_node(decl);

	rapidxml::xml_node<>* root = doc.allocate_node(rapidxml::node_element, "forums");
	doc.append_node(root);
	std::vector<std::shared_ptr<std::string>> strings;
	root->append_node(lightforums::Settings::get().save(&doc, strings));

	rapidxml::xml_node<>* usersNode = lightforums::userList::get().save(&doc, strings);
	root->append_node(usersNode);

	rapidxml::xml_node<>* postsNode = doc.allocate_node(rapidxml::node_element, "posts");
	root->append_node(postsNode);
	rapidxml::xml_node<>* postsSaved = root::get().getRootPost()->getNode(&doc, strings);
	postsNode->append_node(postsSaved);

	rapidxml::xml_node<>* translationsSaved = lightforums::tr::getInstance().save(&doc, strings);
	root->append_node(translationsSaved);

	rapidxml::xml_node<>* cookiesSaved = doc.allocate_node(rapidxml::node_element, "cookies");
	for (auto it = root::get().cookies_.begin(); it != root::get().cookies_.end(); it++) {
		rapidxml::xml_node<>* cookie = doc.allocate_node(rapidxml::node_element, "cookie");
		cookie->append_attribute(doc.allocate_attribute("token", it->first.c_str()));
		cookie->append_attribute(doc.allocate_attribute("user", it->second.c_str()));
		cookiesSaved->append_node(cookie);
	}
	root->append_node(cookiesSaved);

	std::ofstream out(fileName);
	out << doc;
	out.close();
	doc.clear();
}

Wt::WApplication* createApplication(const Wt::WEnvironment& env)
{
	return new mainWindow(env);
}

int main(int argc, char** argv)
{
	setupStructures("saved_data.xml");
	int result = Wt::WRun(argc, argv, &createApplication);
	system("mv saved_data_old.xml saved_data_very_old.xml");
	system("mv saved_data.xml saved_data_old.xml");
	saveStructures("saved_data.xml");
	return result;
}