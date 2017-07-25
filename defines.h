#ifndef DEFINES
#define DEFINES

#include <string>
#include <memory>
#include <vector>
#include <random>
#include <atomic>

#define ALL_PATH_PREFIX "?_="
#define POST_PATH_PREFIX "post"
#define USER_PATH_PREFIX "user"
#define SETTINGS_PATH "settings"
#define TRANSLATION_PATH "translation"

#define RANDOM_TEXT_LENGTH 5

namespace rapidxml {
	template<class Ch> class xml_node;
	template<class Ch> class xml_document;
}

namespace Wt {
	class WContainerWidget;
	class WPushButton;
	class WVBoxLayout;
	class WInPlaceEdit;
	class WComboBox;
	class WColor;
	class WText;
	namespace Chart {
		class WPieChart;
	}
}

namespace lightforums {

	enum rank : unsigned char {
		USER, // Standard rights
		VALUED_USER, // Slightly elevated rights
		MODERATOR, // All rights but change global settings
		ADMIN, // All rights
		rankSize
	};

	enum rating : unsigned char {
		// Upvote
		USEFUL_POST, // Useful information
		FUNNY_POST, // Not useful, but humorous and worth it
		INTERESTING_POST, // Not useful, but worth the read
		// Downvote
		BAD_ADVICE_POST, // Alternative facts
		ANNOYING_POST, // Bad humour attempts, nitpicking, utterly useless
		OFFTOPIC_POST, // Unrelated to the discussion
		NECROMANTIC_POST, // Commented long after the post was forgotten without a good reason
		ratingSize
	};

	enum colour : unsigned char {
		GREEN,
		GREY,
		BLUE,
		RED,
		PURPLE,
		ORANGE,
		BLACK,
		CYAN,
		YELLOW,
		WHITE,
		colourSize
	};

	void formatString(const std::string& str, Wt::WContainerWidget* into);
	std::string replaceVar(const std::string& str, char X, int x);
	inline std::string replaceVar(std::shared_ptr<const std::string> str, char X, int x) { return replaceVar(*str, X, x); }
	std::string replaceVar(const std::string& str, char X, const std::string& x);
	inline std::string replaceVar(std::shared_ptr<const std::string> str, char X, const std::string& x) { return replaceVar(*str, X, x); }
	std::vector<std::string> splitString(const std::string& splitted, char delimeter);
	Wt::WInPlaceEdit* makeEditableText(std::shared_ptr<std::string>* target, Wt::WContainerWidget* parent); // Pointer to the shared_ptr that will be changed by editing
	Wt::WInPlaceEdit* makeEditableNumber(unsigned long int* target, Wt::WContainerWidget* parent);
	Wt::WInPlaceEdit* makeEditableNumber(unsigned int* target, Wt::WContainerWidget* parent);
	Wt::WComboBox* makeEnumEditor(unsigned char* changed, unsigned char elements, unsigned int first, Wt::WContainerWidget* parent);
	Wt::Chart::WPieChart* makeRatingChart(const std::atomic_int* data, Wt::WContainerWidget* parent);
	Wt::WText* makeRatingOverview(const std::atomic_int* data, Wt::WContainerWidget* parent);
	const Wt::WColor& getColour(colour col);

	std::string toBase64(const std::string& from);
	std::string fromBase64(const std::string& from);

	inline uint32_t safeRandom() {
		static std::mt19937 mtRand(time(0)); // Better than rand(), possibly not perfectly safe, but login is done by BCrypt
		return mtRand();
	}

	inline std::string safeRandomString() {
		uint32_t orig[RANDOM_TEXT_LENGTH];
		for (int i = 0; i < RANDOM_TEXT_LENGTH; i++) orig[i] = safeRandom();
		unsigned char* asChar = (unsigned char*)((void*)orig);
		std::string done;
		for (unsigned int i = 0; i < RANDOM_TEXT_LENGTH * sizeof(uint32_t); i++) done.push_back(asChar[i]);
		const std::string& result = toBase64(done);
		return result;
	}

#define ROT32(x, y) ((x << y) | (x >> (32 - y)))
	static uint32_t murmur(const void* key, uint32_t len, uint32_t seed) {
		uint32_t hash = seed;

		const int nblocks = len / 4;
		const uint32_t* blocks = (const uint32_t*)key;

		auto murmu = [] (uint32_t var) -> uint32_t {
			var *= 0xcc9e2d51;
			var = ROT32(var, 15);
			var *= 0x1b873593;
			return var;
		};

		for (int i = 0; i < nblocks; i++) {
			hash ^= murmu(blocks[i]);
			hash = ROT32(hash, 13) * 5 + 0xe6546b64;
		}

		const uint8_t* tail = (const uint8_t*)((const char*)key + nblocks * 4);
		uint32_t k = 0;

		switch (len & 3) {
			case 3:
				k ^= tail[2] << 16;
			case 2:
				k ^= tail[1] << 8;
			case 1:
				k ^= tail[0];
				hash ^= murmu(k);
		}

		hash ^= len;
		hash ^= (hash >> 16);
		hash *= 0x85ebca6b;
		hash ^= (hash >> 13);
		hash *= 0xc2b2ae35;
		hash ^= (hash >> 16);

		return hash;
	}

}


#endif // DEFINES
