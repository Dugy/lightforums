#include "mainwindow.h"

#include <Wt/WBorderLayout>
#include <Wt/WScrollArea>
#include <Wt/WText>
#include <Wt/WHBoxLayout>
#include <Wt/WVBoxLayout>
#include <Wt/WPushButton>
#include <Wt/WLineEdit>
#include <Wt/WDialog>
#include <Wt/WMessageBox>
#include <Wt/Auth/HashFunction>
#include <Wt/WEnvironment>
#include <Wt/WBootstrapTheme>
#include "user.h"
#include "post.h"
#include "translation.h"
#include "userlist.h"
#include "settings.h"

std::shared_ptr<lightforums::post> root::getRootPost() {
	if (rootPost_) return rootPost_;

	// Create a dummy one
	lightforums::post* made = new lightforums::post();
	made->title_ = std::make_shared<std::string>("Welcome to the forums");
	made->text_ = std::make_shared<std::string>("Create threads or subforums within this. To manage something, log in as 'Administrator_President' with password 'freecandy' (and change your password as soon as possible).");
	// This leads to password $2y$05$WiDKPizNSRb0TkrTahbmLObR0vo1STjeI4xqD2rCRqbc57.LR.SJ2 if salt is bALFMOQ7vVkUr7h5MpzI0AQU9Tc=
	made->author_ = std::make_shared<std::string>("Administrator_President");
	made->visibility_ = lightforums::USER;
	made->depth_ = 1;
	return made->self();
}

mainWindow::mainWindow(const Wt::WEnvironment& env) :
	Wt::WApplication(env)
{
	try {
		std::string cookie = env.getCookie("login");
		auto found = root::get().cookies_.find(cookie);
		if (found != root::get().cookies_.end()) {
			currentUser_ = found->second;
		}
	} catch (std::runtime_error e) {}

//	useStyleSheet("css/style.css");
	setCssTheme("default");
//	setTheme(new Wt::WBootstrapTheme());

	rebuild();
}

mainWindow::~mainWindow() {

}

void mainWindow::rebuild() {
	root()->clear();

	Wt::WBorderLayout* layout = new Wt::WBorderLayout(root());

	scrollArea_ = new Wt::WScrollArea(root());
	layout->addWidget(scrollArea_, Wt::WBorderLayout::Center);

	Wt::WContainerWidget* byContainer = new Wt::WContainerWidget(root());
	byContainer->setStyleClass("lightforums-topbar");
	Wt::WText* by = new Wt::WText(Wt::WString("Software: lightforums (2017)"), byContainer);
	layout->addWidget(byContainer, Wt::WBorderLayout::South);

	authContainer_ = new Wt::WContainerWidget(root());
	layout->addWidget(authContainer_, Wt::WBorderLayout::North);
	authContainer_->setStyleClass("lightforums-bottombar");
	makeAuthBlock();



	internalPathChanged().connect(std::bind([=] () {
		 handlePathChange();
	}));
	handlePathChange();
}

void mainWindow::handlePathChange()
{
	std::string path = internalPath();
	unsigned int start = 0;
	while (path[start] == '/' || path[start] == '?' || path[start] == '_' || path[start] == '=') start++;
	path = path.substr(start);
	std::cerr << "Path is " << path << std::endl;

	Wt::WContainerWidget* content = nullptr;
	if (!path.empty()) {
		if (path.find(POST_PATH_PREFIX) == 0) {
			std::string steps = path.substr(strlen(POST_PATH_PREFIX));
			std::cerr << "Post location is " << steps << std::endl;
			std::shared_ptr<lightforums::post> found = getPost(steps);
			if (found) {
				content = found->build(currentUser_, 1, found != root::get().getRootPost());
				setTitle(Wt::WString(*std::atomic_load(&found->title_)));
			}
		} else if (path.find(USER_PATH_PREFIX) == 0) {
			std::string name = path.substr(strlen(USER_PATH_PREFIX) + 1);
			std::cerr << "User name is " << name << std::endl;
			std::shared_ptr<lightforums::user> got = lightforums::userList::get().getUser(name);
			if (got) {
				content = got->show(currentUser_);
				setTitle(Wt::WString(*std::atomic_load(&got->name_)));
			}
		} else if (path.find(SETTINGS_PATH) == 0) {
			std::shared_ptr<lightforums::user> editor = lightforums::userList::get().getUser(currentUser_);
			if (editor->rank_ == lightforums::ADMIN) {
				content = lightforums::Settings::get().edit(currentUser_);
				setTitle(Wt::WString(*lightforums::tr::get(lightforums::tr::CHANGE_SETTINGS)));
			}
		} else if (path.find(TRANSLATION_PATH) == 0) {
			std::shared_ptr<lightforums::user> editor = lightforums::userList::get().getUser(currentUser_);
			if (editor->rank_ == lightforums::ADMIN) {
				content = lightforums::tr::getInstance().edit(currentUser_);
				setTitle(Wt::WString(*lightforums::tr::get(lightforums::tr::CHANGE_TRANSLATION)));
			}
		}
	}
	if (!content) {
		content = root::get().getRootPost()->build(currentUser_, 1);
		setTitle(Wt::WString(*std::atomic_load(&root::get().getRootPost()->title_)));
	}
	root()->addWidget(content);
	scrollArea_->setWidget(content);
}

std::shared_ptr<lightforums::post> mainWindow::getPost(lightforums::postPath path) {
	lightforums::postPath::iterator it = path.getIterator();
	std::shared_ptr<lightforums::post> cur = root::get().getRootPost();
	it.getNext(); // Ignore first, it will always be the same
	while (it.hasNext()) {
		int got = it.getNext();
		auto found = cur->children_.find(got);
		if (found != cur->children_.end())
			cur = found->second;
	}
	return cur;
}

void mainWindow::makeAuthBlock() {
	authContainer_->clear();
	Wt::WHBoxLayout* authLayout = new Wt::WHBoxLayout(authContainer_);
	Wt::WAnchor* backToMain = new Wt::WAnchor(Wt::WLink(Wt::WLink::InternalPath, "/"), Wt::WString(*lightforums::tr::get(lightforums::tr::BACK_TO_MAIN)), authContainer_);
	authLayout->addWidget(backToMain);
	if (currentUser_.empty()) {
		authLayout->addStretch(1);
		Wt::WPushButton* loginButton = new Wt::WPushButton(Wt::WString(*lightforums::tr::get(lightforums::tr::DO_LOG_IN)), authContainer_);
		authLayout->addWidget(loginButton);
		loginButton->clicked().connect(std::bind([=] () {
			Wt::WDialog* dialog = new Wt::WDialog(*lightforums::tr::get(lightforums::tr::DO_LOG_IN));
			dialog->setModal(false);
			Wt::WVBoxLayout* layout = new Wt::WVBoxLayout(dialog->contents());

			Wt::WLineEdit* nameEdit = new Wt::WLineEdit(dialog->contents());
			nameEdit->setPlaceholderText(Wt::WString(*lightforums::tr::get(lightforums::tr::WRITE_USERNAME)));
			layout->addWidget(nameEdit);
			Wt::WLineEdit* passwordEdit = new Wt::WLineEdit(dialog->contents());
			passwordEdit->setEchoMode(Wt::WLineEdit::Password);
			passwordEdit->setPlaceholderText(Wt::WString(*lightforums::tr::get(lightforums::tr::WRITE_PASSWORD)));
			layout->addWidget(passwordEdit);

			Wt::WContainerWidget* buttonContainer = new Wt::WContainerWidget(dialog->contents());
			layout->addWidget(buttonContainer);
			Wt::WHBoxLayout* buttonLayout = new Wt::WHBoxLayout(buttonContainer);
			Wt::WPushButton* loginButton = new Wt::WPushButton(*lightforums::tr::get(lightforums::tr::DO_LOG_IN), buttonContainer);
			loginButton->setDefault(true);
			buttonLayout->addStretch(1);
			buttonLayout->addWidget(loginButton);

			loginButton->clicked().connect(std::bind([=] () {
				std::shared_ptr<lightforums::user> found = lightforums::userList::get().getUser(nameEdit->text().toUTF8());
				if (!found) lightforums::messageBox(*lightforums::tr::get(lightforums::tr::LOGIN_ERROR), *lightforums::tr::get(lightforums::tr::NO_SUCH_USER));
				else {
//					std::string salt = lightforums::safeRandomString();
					static Wt::Auth::BCryptHashFunction cryptHasher(5);
//					found->salt_ = std::make_shared<std::string>(salt); // Enable this to make login attempt change the password to the written one
//					found->password_ = std::make_shared<std::string>(cryptHasher.compute(passwordEdit->text().toUTF8(), salt));
					if (!cryptHasher.verify(passwordEdit->text().toUTF8(), *found->salt_, *found->password_))
						lightforums::messageBox(*lightforums::tr::get(lightforums::tr::LOGIN_ERROR), *lightforums::tr::get(lightforums::tr::WRONG_PASSWORD));
					else {
						currentUser_ = *found->name_;
						std::string cookie = lightforums::safeRandomString();
						setCookie("login", cookie, INT_MAX); // Cookie forever, it's very, very small
						root::get().cookies_.insert(cookie, currentUser_);
						dialog->accept();
						rebuild();
					}
				}
			}));
			Wt::WPushButton* dontLoginButton = new Wt::WPushButton(*lightforums::tr::get(lightforums::tr::CANCEL_LOG_IN), buttonContainer);
			buttonLayout->addWidget(dontLoginButton);

			dontLoginButton->clicked().connect(std::bind([=] () {
				dialog->reject();
			}));

			dialog->finished().connect(std::bind([=] () { delete dialog; }));
			dialog->show();

		}));
	} else {
		Wt::WAnchor* profile = new Wt::WAnchor(Wt::WLink(Wt::WLink::InternalPath, "/" USER_PATH_PREFIX "/" + currentUser_), Wt::WString(*lightforums::tr::get(lightforums::tr::MY_ACCOUNT)), authContainer_);
		authLayout->addWidget(profile);
		std::shared_ptr<lightforums::user> viewer = lightforums::userList::get().getUser(currentUser_);
		if (viewer && viewer->rank_ == lightforums::ADMIN) {
			Wt::WAnchor* showSettings = new Wt::WAnchor(Wt::WLink(Wt::WLink::InternalPath, "/" SETTINGS_PATH), Wt::WString(*lightforums::tr::get(lightforums::tr::CHANGE_SETTINGS)), authContainer_);
			authLayout->addWidget(showSettings);
		}

		authLayout->addStretch(1);
		authLayout->addWidget(new Wt::WText(Wt::WString(lightforums::replaceVar(*lightforums::tr::get(lightforums::tr::LOGGED_IN_AS), 'X', currentUser_))));
		Wt::WPushButton* logoutButton = new Wt::WPushButton(Wt::WString(*lightforums::tr::get(lightforums::tr::DO_LOG_OUT)), authContainer_);
		authLayout->addWidget(logoutButton);
		logoutButton->clicked().connect(std::bind([=] () {
			root::get().cookies_.erase(environment().getCookie("login"));
			setCookie("login", "", 0);
			currentUser_.clear();
			rebuild();
		}));
	}

	if ((currentUser_.empty() && lightforums::Settings::get().canRegister) || lightforums::userList::get().getUser(currentUser_)->rank_ == lightforums::ADMIN) {
		Wt::WPushButton* registerButton = new Wt::WPushButton(Wt::WString(*lightforums::tr::get(lightforums::tr::DO_REGISTER)), authContainer_);
		authLayout->addWidget(registerButton);
		registerButton->clicked().connect(std::bind([=] () {

			Wt::WDialog* dialog = new Wt::WDialog(*lightforums::tr::get(lightforums::tr::DO_LOG_IN));
			dialog->setModal(false);
			Wt::WVBoxLayout* layout = new Wt::WVBoxLayout(dialog->contents());

			Wt::WLineEdit* nameEdit = new Wt::WLineEdit(dialog->contents());
			nameEdit->setPlaceholderText(Wt::WString(*lightforums::tr::get(lightforums::tr::WRITE_USERNAME)));
			layout->addWidget(nameEdit);
			Wt::WLineEdit* passwordEdit[2];
			for (int i = 0; i <= 1; i++) {
				passwordEdit[i] = new Wt::WLineEdit(dialog->contents());
				passwordEdit[i]->setEchoMode(Wt::WLineEdit::Password);
				passwordEdit[i]->setPlaceholderText(Wt::WString(*lightforums::tr::get((i == 0) ? lightforums::tr::WRITE_PASSWORD : lightforums::tr::REWRITE_PASSWORD)));
				layout->addWidget(passwordEdit[i]);
			}

			Wt::WContainerWidget* buttonContainer = new Wt::WContainerWidget(dialog->contents());
			layout->addWidget(buttonContainer);
			Wt::WHBoxLayout* buttonLayout = new Wt::WHBoxLayout(buttonContainer);
			Wt::WPushButton* registerButton = new Wt::WPushButton(*lightforums::tr::get(lightforums::tr::DO_REGISTER), buttonContainer);
			registerButton->setDefault(true);
			buttonLayout->addStretch(1);
			buttonLayout->addWidget(registerButton);

			registerButton->clicked().connect(std::bind([=] () {
				std::string username = nameEdit->text().toUTF8();
				std::shared_ptr<lightforums::user> found = lightforums::userList::get().getUser(username);
				if (found) lightforums::messageBox(*lightforums::tr::get(lightforums::tr::REGISTER_ERROR), *lightforums::tr::get(lightforums::tr::USERNAME_UNAVAILABLE));
				else if (passwordEdit[0]->text() != passwordEdit[1]->text()) lightforums::messageBox(*lightforums::tr::get(lightforums::tr::REGISTER_ERROR), *lightforums::tr::get(lightforums::tr::PASSWORDS_DONT_MATCH));
				else {
					if (lightforums::user::validateUsername(username)) {
						std::shared_ptr<lightforums::user> made = std::make_shared<lightforums::user>();
						made->name_ = std::make_shared<std::string>(username);
						std::string salt = lightforums::safeRandomString();
						static Wt::Auth::BCryptHashFunction cryptHasher(5);
						made->salt_ = std::make_shared<std::string>(salt);
						made->password_ = std::make_shared<std::string>(cryptHasher.compute(passwordEdit[0]->text().toUTF8(), salt));
						made->rank_ = lightforums::USER;
						if (lightforums::userList::get().addUser(made)) {
							currentUser_ = username;
							std::string cookie = lightforums::safeRandomString();
							setCookie("login", cookie, INT_MAX); // Cookie forever, it's very, very small
							root::get().cookies_.insert(cookie, currentUser_);
							dialog->accept();
							rebuild();
						} else {
							lightforums::messageBox(*lightforums::tr::get(lightforums::tr::REGISTER_ERROR), *lightforums::tr::get(lightforums::tr::USERNAME_UNAVAILABLE));
						}
					}
				}
			}));

			Wt::WPushButton* dontRegisterButton = new Wt::WPushButton(*lightforums::tr::get(lightforums::tr::CANCEL_ACCOUNT_CREATION), buttonContainer);
			buttonLayout->addWidget(dontRegisterButton);

			dontRegisterButton->clicked().connect(std::bind([=] () {
				dialog->reject();
			}));

			dialog->finished().connect(std::bind([=] () { delete dialog; }));
			dialog->show();
		}));
	}
}
