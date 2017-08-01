#include <iostream>
#include <fstream>
#include "mainwindow.h"
#include "rapidxml.hpp"
#include "rapidxml_print.hpp"
#include <thread>
#include <chrono>
#include "settings.h"
#include "post.h"
#include "userlist.h"

volatile bool exiting = false;
volatile bool readyToExit = false;

#ifdef __linux__

#include <ucontext.h>

#ifdef __x86_64__
#define REG_EIP REG_RIP
#endif

#include <unistd.h>
#include <sys/types.h>
#include <execinfo.h>
#include <signal.h>
void segfaultHandler(int sig, siginfo_t* info, void* secret) {
	void* trace[30];
	char** messages = (char**)NULL;
	int i, trace_size = 0;
	ucontext_t *uc = (ucontext_t*)secret;

	/* Do something useful with siginfo_t */
	std::string outputFile = "backtrace_" + std::to_string(time(NULL));
	std::cerr << "Crash report writing to " << outputFile << std::endl;

	auto ptrToString = [] (const void* arg) -> std::string {
		std::stringstream stream;
		stream << arg;
		return stream.str();
	};
	if (sig == SIGSEGV)
		system(std::string("echo \"Got signal " + std::to_string(sig) + ", faulty address is " + ptrToString(info->si_addr) + ", from " + ptrToString((void*)uc->uc_mcontext.gregs[REG_EIP]) + "\" >> " + outputFile).c_str());
	else
		system(std::string("echo \"Got signal " + std::to_string(sig) + "\" >> " + outputFile).c_str());

	trace_size = backtrace(trace, 30);
	/* overwrite sigaction with caller's address */
	trace[1] = (void *) uc->uc_mcontext.gregs[REG_EIP];

	messages = backtrace_symbols(trace, trace_size);
	/* skip first stack frame (points here) */
	system(std::string("echo \"Execution path\" >> " + outputFile).c_str());
	for (i = 1; i < trace_size; i++) {
		system(std::string("echo \"[bt] " + std::string(messages[i]) + "\" >> " + outputFile).c_str());
	}

	system(std::string("echo \"[bt]More detailed:\" >> " + outputFile).c_str());

	for (i = 1; i < trace_size; i++) {
		size_t p = 0;
		while (messages[i][p] != '(' && messages[i][p] != ' ' && messages[i][p] != 0) p++;
		system(std::string("addr2line " + ptrToString(trace[i]) + " -e " + std::string(messages[i]).substr(0, p) + " >> " + outputFile).c_str());
	}
	kill(getpid(), SIGTERM);
	exit(1); // For the case if kill fails somehow
}

#endif

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
		} else lightforums::userList::get().setupUserList(nullptr);
	} lightforums::userList::get().setupUserList(nullptr);
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

#ifdef __linux__
	system(std::string("mv " + fileName + ".old " + fileName + ".very_old").c_str());
	system(std::string("mv " + fileName + " " + fileName + ".old").c_str());
#elif __APPLE__
	system(std::string("mv " + fileName + ".old " + fileName + ".very_old").c_str());
	system(std::string("mv " + fileName + " " + fileName + ".old").c_str());
#elif _WIN32
	system(std::string("move " + fileName + ".old " + fileName + ".very_old").c_str());
	system(std::string("move " + fileName + " " + fileName + ".old").c_str());
#endif
	std::cerr << "Saved as " << fileName << std::endl;

	std::ofstream out(fileName);
	out << doc;
	out.close();
	doc.clear();
}

void saveOccasionally() {
	unsigned int waited = 0;
	unsigned int tillBackup = 0;
	while (!exiting) {
		if (waited >= lightforums::Settings::get().savingFrequency) {
			saveStructures("saved_data.xml");
			waited = 0;
			if (tillBackup >= lightforums::Settings::get().backupFrequency) {
				saveStructures("backup_data.xml");
				tillBackup = 0;
			} else tillBackup++;
		} waited++;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	saveStructures("saved_data.xml");
	readyToExit = true;
	std::cerr << "Ready to exit" << std::endl;
}

Wt::WApplication* createApplication(const Wt::WEnvironment& env)
{
	return new mainWindow(env);
}

int main(int argc, char** argv)
{
#ifdef __linux__
	struct sigaction sa;

	sa.sa_sigaction = segfaultHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_SIGINFO;

	sigaction(SIGSEGV, &sa, NULL);
#endif
	setupStructures("saved_data.xml");
	std::thread backupThread(saveOccasionally);
	int result = Wt::WRun(argc, argv, &createApplication);
	exiting = true;
	while (!readyToExit) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	backupThread.join();
	return result;
}
