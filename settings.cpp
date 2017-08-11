#include "settings.h"

#include "rapidxml.hpp"
#include <cstring>
#include <iostream>
#include <Wt/WCheckBox>
#include <Wt/WContainerWidget>
#include <Wt/WText>
#include <Wt/WSelectionBox>
#include <Wt/WInPlaceEdit>
#include <Wt/WGridLayout>
#include <Wt/WLink>
#include <Wt/WAnchor>

#pragma GCC diagnostic ignored "-Wunused-parameter"

lightforums::Settings::Settings() :
	fileOrder(0)
{

}

void lightforums::Settings::setup(rapidxml::xml_node<>* from) {
	std::function<void(bool&, bool, const char*, tr::translatable)> doOnBool = [&] (bool& target, bool preset, const char* field, tr::translatable description) {
		if (!from) {
			target = preset;
			return;
		}
		rapidxml::xml_node<>* node = from->first_node(field);
		if (!node) target = preset;
		else target = !strcmp("1", node->value());
	};
	std::function<void(unsigned int&, unsigned int, const char*, tr::translatable)> doOnUint = [&] (unsigned int& target, unsigned int preset, const char* field, tr::translatable description) {
		if (!from) {
			target = preset;
			return;
		}
		rapidxml::xml_node<>* node = from->first_node(field);
		if (!node) target = preset;
		else target = atoi(node->value());
	};
	std::function<void(unsigned long int&, unsigned long int, const char*, tr::translatable)> doOnULint = [&] (unsigned long int& target, unsigned long int preset, const char* field, tr::translatable description) {
		if (!from) {
			target = preset;
			return;
		}
		rapidxml::xml_node<>* node = from->first_node(field);
		if (!node) target = preset;
		else target = atoi(node->value());
	};
	std::function<void(std::shared_ptr<std::string>&, const char*, const char*, tr::translatable)> doOnString = [&] (std::shared_ptr<std::string>& target, const char* preset, const char* field, tr::translatable description) {
		if (!from) {
			target = std::make_shared<std::string>(preset);
			return;
		}
		rapidxml::xml_node<>* node = from->first_node(field);
		if (!node) target = std::make_shared<std::string>(preset);
		else target = std::make_shared<std::string>(node->value());
	};
	std::function<void(unsigned char*, unsigned char, const char*, tr::translatable, unsigned char, tr::translatable)> doOnEnum = [&] (unsigned char* target, unsigned char preset, const char* field, tr::translatable description, unsigned char elements, tr::translatable first) {
		if (!from) {
			*target = preset;
			return;
		}
		rapidxml::xml_node<>* node = from->first_node(field);
		if (!node) *target = preset;
		else {
			*target = atoi(node->value());
			if (*target >= elements) *target = preset;
		}
	};
	goThroughAll(doOnBool, doOnUint, doOnULint, doOnString, doOnEnum);
	if (from && from->first_node("file_order")) fileOrder.store(atoi(from->first_node("file_order")->value()));
}

rapidxml::xml_node<>* lightforums::Settings::save(rapidxml::xml_document<>* doc, std::vector<std::shared_ptr<std::string>>& strings) {
	rapidxml::xml_node<>* made = doc->allocate_node(rapidxml::node_element, "settings");
	auto appendNode = [&] (const char* field, const std::string& added) -> void {
		std::shared_ptr<std::string> contents  = std::make_shared<std::string>(added);
		std::shared_ptr<std::string> name  = std::make_shared<std::string>(field);
		strings.push_back(contents);
		strings.push_back(name);
		made->append_node(doc->allocate_node(rapidxml::node_element, name->c_str(), contents->c_str()));
	};
	std::function<void(bool&, bool, const char*, tr::translatable)> doOnBool = [&] (bool& target, bool preset, const char* field, tr::translatable description) {
		appendNode(field, std::to_string(target));
	};
	std::function<void(unsigned int&, unsigned int, const char*, tr::translatable)> doOnUint = [&] (unsigned int& target, unsigned int preset, const char* field, tr::translatable description) {
		appendNode(field, std::to_string(target));
	};
	std::function<void(unsigned long int&, unsigned long int, const char*, tr::translatable)> doOnULint = [&] (unsigned long int& target, unsigned long int preset, const char* field, tr::translatable description) {
		appendNode(field, std::to_string(target));
	};
	std::function<void(std::shared_ptr<std::string>&, const char*, const char*, tr::translatable)> doOnString = [&] (std::shared_ptr<std::string>& target, const char* preset, const char* field, tr::translatable description) {
		appendNode(field, *target);
	};
	std::function<void(unsigned char*, unsigned char, const char*, tr::translatable, unsigned char, tr::translatable)> doOnEnum = [&] (unsigned char* target, unsigned char preset, const char* field, tr::translatable description, unsigned char elements, tr::translatable first) {
		appendNode(field, std::to_string((int)*target));
	};
	goThroughAll(doOnBool, doOnUint, doOnULint, doOnString, doOnEnum);

	appendNode("file_order", std::to_string(fileOrder));
	return made;
}

Wt::WContainerWidget* lightforums::Settings::edit(const std::string& viewer) {
	Wt::WContainerWidget* result = new Wt::WContainerWidget();
	Wt::WGridLayout* grid = new Wt::WGridLayout(result);
	int line = 0;

	std::function<void(bool&, bool, const char*, tr::translatable)> doOnBool = [&] (bool& target, bool preset, const char* field, tr::translatable description) {
		grid->addWidget(new Wt::WText(Wt::WString(*tr::get(description)), result), line, 0);
		Wt::WCheckBox* checkBox = new Wt::WCheckBox(result);
		checkBox->setChecked(target);
		grid->addWidget(checkBox, line, 1);
		checkBox->checked().connect(std::bind([&] () {
			target = true;
		}));
		checkBox->unChecked().connect(std::bind([&] () {
			target = false;
		}));
		line++;
	};
	std::function<void(unsigned int&, unsigned int, const char*, tr::translatable)> doOnUint = [&] (unsigned int& target, unsigned int preset, const char* field, tr::translatable description) {
		grid->addWidget(new Wt::WText(Wt::WString(*tr::get(description)), result), line, 0);
		grid->addWidget(makeEditableNumber(&target, result), line, 1);
		line++;
	};
	std::function<void(unsigned long int&, unsigned long int, const char*, tr::translatable)> doOnULint = [&] (unsigned long int& target, unsigned long int preset, const char* field, tr::translatable description) {
		grid->addWidget(new Wt::WText(Wt::WString(*tr::get(description)), result), line, 0);
		grid->addWidget(makeEditableNumber(&target, result), line, 1);
		line++;
	};
	std::function<void(std::shared_ptr<std::string>&, const char*, const char*, tr::translatable)> doOnString = [&] (std::shared_ptr<std::string>& target, const char* preset, const char* field, tr::translatable description) {
		grid->addWidget(new Wt::WText(Wt::WString(*tr::get(description)), result), line, 0);
		grid->addWidget(makeEditableText(&target, result), line, 1);
		line++;
	};
	std::function<void(unsigned char*, unsigned char, const char*, tr::translatable, unsigned char, tr::translatable)> doOnEnum = [&] (unsigned char* target, unsigned char preset, const char* field, tr::translatable description, unsigned char elements, tr::translatable first) {
		grid->addWidget(new Wt::WText(Wt::WString(*tr::get(description)), result), line, 0);
		grid->addWidget(makeEnumEditor(target, elements, first, result), line, 1);
		line++;
	};
	goThroughAll(doOnBool, doOnUint, doOnULint, doOnString, doOnEnum);

	Wt::WAnchor* showTranslation = new Wt::WAnchor(Wt::WLink(Wt::WLink::InternalPath, "/" TRANSLATION_PATH), Wt::WString(*lightforums::tr::get(lightforums::tr::CHANGE_TRANSLATION)), result);
	grid->addWidget(showTranslation, line, 0);

	grid->setColumnStretch(1, 1);
	return result;
}
