#ifndef POST_H
#define POST_H

#include <string>
#include <atomic>
#include <vector>
#include <memory>
#include "defines.h"
#include "atomic_unordered_map.h"
#include "translation.h"

namespace lightforums {

	class post;

	class postPath {
		unsigned int size_;
		unsigned char* path_;
	public:
		postPath(const char* str) {
			std::vector<unsigned char> result;
			result.reserve(50);
			unsigned long int number = 0;
			auto append = [&] () {
				bool zero = true;
				for (int i = sizeof(unsigned long int) - 1; i >= 0; i--) {
					unsigned char got = (number >> (i << 3)) & 0xff; // Get ith byte
					if (got != 0 && zero) {
						result.push_back(i + 1);
						zero = false;
					}
					if (!zero) {
						result.push_back(got);
					}
					if (got == 0 && i == 0) {
						result.push_back(0);
					}
				}
			};
			while (*str == '/') str++;
			for ( ; str; str++) {
				if (*str >= '0' && *str <= '9') {
					number = number * 10 + *str - '0';
				} else if (*str == '/') {
					append();
					number = 0;
				} else break; // Wrong string
			}
			if (*(str - 1) != '/') append(); // Maybe something was not finished by slash
			size_ = result.size();
			path_ = new unsigned char[size_];
			for (unsigned int i = 0; i < result.size(); i++) {
				path_[i] = result[i];
			}
		}
		postPath(const std::string& asString) : postPath(asString.c_str()) { }
		postPath(std::shared_ptr<post> from);
		postPath(const postPath& other) : size_(other.size_), path_(new unsigned char[other.size_]) {
			for (unsigned int i = 0; i < size_; i++) path_[i] = other.path_[i];
		}
		void operator=(const postPath& other) {
			size_ = other.size_;
			path_ = new unsigned char[size_];
			for (unsigned int i = 0; i < size_; i++) path_[i] = other.path_[i];
		}

		~postPath() {
			delete[] path_;
		}

		class iterator {
			unsigned int i;
			const postPath* path;
			iterator(const postPath* pathSet) : i(0), path(pathSet) {}
		public:
			bool hasNext() { return i < path->size_; }
			unsigned int getNext() {
				int partSize = path->path_[i];
				i++;
				unsigned long int num = 0;
				for (unsigned int origI = i; i < origI + partSize; i++) {
					num = num + (path->path_[i] << ((partSize + origI - i - 1) << 3));
				}
				return num;
			}
			friend class postPath;
		};

		iterator getIterator() const {
			return iterator(this);
		}

		std::string getString() const {
			std::string result;
			iterator it = getIterator();
			while (it.hasNext()) {
				result += std::to_string(it.getNext()) + "/";
			}
			return result;
		}

		bool operator== (const postPath& other) const {
			if (size_ != other.size_) return false;
			for (unsigned int i = 0; i < size_; i++)
				if (path_[i] != other.path_[i]) return false;
			return true;
		}
		bool operator!= (const postPath& other) const { return !operator ==(other); }

		friend class std::hash<postPath>;
	};

	class post
	{
	public:
		post(std::shared_ptr<post> parent = nullptr);
		post(std::shared_ptr<post> parent, rapidxml::xml_node<char>* node);
		~post();
		rapidxml::xml_node<char>* getNode(rapidxml::xml_document<char>* doc, std::vector<std::shared_ptr<std::string>>& strings);
		Wt::WContainerWidget* build(const std::string& viewer, int depth, bool showParentLink = false);
		void setParent(std::shared_ptr<post> parent = nullptr);

		std::shared_ptr<std::string> title_;
		std::shared_ptr<std::string> text_;
		std::shared_ptr<std::string> author_;
		std::atomic<rank> visibility_;
		std::atomic_uint depth_;
		std::atomic_int rating_[ratingSize];
		sortPosts sortBy_;
		std::atomic<time_t> postedAt_;
		std::atomic<time_t> lastActivity_;

		atomic_unordered_map<unsigned int, std::shared_ptr<post>> children_;

		unsigned int getId() { return id_; }
		std::shared_ptr<post> self();
		std::string getLink();
		std::shared_ptr<post> getParent() { return parent_; }

	private:

		static void showChildren(std::string viewer, Wt::WContainerWidget* container, std::shared_ptr<post> from, unsigned int depth);
		static void hideChildren(std::string viewer, Wt::WContainerWidget* container, std::shared_ptr<post> from);
		static void addReplyMenu(std::string viewer, Wt::WContainerWidget* container, std::shared_ptr<post> from);
		static Wt::WPushButton* addReplyButton(tr::translatable title, std::string viewer, Wt::WContainerWidget* container, Wt::WContainerWidget* buttonContainer, std::shared_ptr<post> from);
		static Wt::WComboBox* makeRatingCombo(std::string viewer, Wt::WContainerWidget* container, std::shared_ptr<post> from);

		std::shared_ptr<post> parent_;
		unsigned long int id_;
		void setRatings();

		friend class postPath;
	};

}

namespace std {
	template <>
	struct hash<lightforums::postPath> {
	public:
		size_t operator()(lightforums::postPath x) const throw() {
			size_t h = lightforums::murmur(x.path_, x.size_, 1020959);
			return h;
		}
	};
}

#endif // POST_H
