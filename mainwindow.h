#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <Wt/WApplication>
#include <Wt/WContainerWidget>
#include "post.h"

class root {
public:
	static inline root& get() {
		static root holder;
		return holder;
	}

	std::shared_ptr<lightforums::post> getRootPost();
	void setRootPost(std::shared_ptr<lightforums::post> set) { rootPost_ = set; }
	atomic_unordered_map<std::string, std::string> cookies_;

private:

	std::shared_ptr<lightforums::post> rootPost_;
	root() :
		rootPost_(nullptr) {}

	root(const root&) = delete;
	void operator=(const root&) = delete;
};

class mainWindow : public Wt::WApplication
{
public:
	mainWindow(const Wt::WEnvironment& env);
	~mainWindow();

private:
	void handlePathChange();
	std::shared_ptr<lightforums::post> getPost(lightforums::postPath path);
	Wt::WScrollArea* scrollArea_;
	Wt::WContainerWidget* authContainer_;
	std::string currentUser_;

	void rebuild();
	void makeAuthBlock();

};

#endif // MAINWINDOW_H
