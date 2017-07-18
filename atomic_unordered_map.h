#ifndef ATOMIC_UNORDERED_MAP
#define ATOMIC_UNORDERED_MAP

#include <memory>
#include <utility>
#include <functional>
#include <atomic>
#include <chrono>
#include <thread>
#include <climits>
#include <iostream>

template <typename K, typename V>
class atomic_unordered_map {
	// A hashtable that can be wildly accessed from various threads. Insertion and deletion are consistent,
	// iteration may go through outdated data. Iterators hold their contents in smart pointers, so copies
	// of data will be kept until the last iterator is destroyed.
	// Enlargment of the table is not lockfree, because it will run only a couple of times during the run. FIXME: Can be done easily

	template <typename T>
	struct selfdestructing_array {
		T* contents_;
		unsigned long int size_;
		selfdestructing_array(unsigned long int size) :
			contents_(new T[size]),
			size_(size) {}
		~selfdestructing_array() { delete[] contents_; }
	};

	typedef std::shared_ptr<selfdestructing_array<std::pair<size_t, std::shared_ptr<std::pair<const K, V>>>>> map_type;
	map_type map_;
	std::atomic_uint_fast64_t occupancy_;

	std::atomic_uint accessed_; // Twice the number of accessors, odd numbers mean that it's time to rehash

	size_t hash (const K& key) {
		size_t hashed = std::hash<K>{}(key);
		hashed = (hashed ^ (hashed >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
		hashed = (hashed ^ (hashed >> 27)) * UINT64_C(0x94d049bb133111eb);
		hashed = hashed ^ (hashed >> 31);
		// Need two special values for empty elements and deleted elements
		if (hashed == 0) hashed = 2;
		else if (hashed == 1) hashed = 3;
		return hashed;
	}

	struct read_lock {
		atomic_unordered_map& parent_;
		read_lock(atomic_unordered_map* parent):
		parent_(*parent) {
			uint was, want;
			do {
				was = parent_.accessed_;
				while (was % 2) {
					std::this_thread::sleep_for(std::chrono::microseconds(1));
					was = parent_.accessed_;
				}
				want = was + 2;
			} while (!std::atomic_compare_exchange_weak_explicit(&parent_.accessed_, &was, want,
						std::memory_order_release, std::memory_order_relaxed));
		}
		~read_lock() {
			uint was, want;
			do {
				was = parent_.accessed_;
				want = was - 2;
			} while (!std::atomic_compare_exchange_weak_explicit(&parent_.accessed_, &was, want,
						std::memory_order_release, std::memory_order_relaxed));
		}
	};
	void rehash(unsigned long int new_size) {
		// This one will be executed only a few times, the lock is thus not performance-degradating
		// Will only enlarge the array
		uint was, want;
		do {
			was = accessed_;
			if (was % 2) {
				// Being done elsewhere, wait till done
				do {
					std::this_thread::sleep_for(std::chrono::microseconds(1));
					was = accessed_;
				} while (was % 2);
				return;
			}
			want = was + 1;
		} while (!std::atomic_compare_exchange_weak_explicit(&accessed_, &was, want,
															 std::memory_order_release, std::memory_order_relaxed));
		if (std::atomic_load(&map_)->size_ >= new_size) {
			accessed_--;
			return; // Already enlarged
		}
		// Others clearing our way
		while (accessed_ != 1)
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		// Way's clear, rehash
		map_type map(
					new selfdestructing_array<std::pair<size_t, std::shared_ptr<std::pair<const K, V>>>>(new_size));
		for (unsigned long int i = 0; i < new_size; i++) {
			map->contents_[i].first = 0;
			map->contents_[i].second = nullptr;
		}

		for (unsigned int i = 0; i < map_->size_; i++) {
			if (map_->contents_[i].first <= 1) continue;
			unsigned long int pos = map_->contents_[i].first % map->size_;
			while (map->contents_[pos].first) pos = (pos + 1) % map->size_;
			map->contents_[pos] =  map_->contents_[i];
		}
		map_ = map;
		accessed_ = 0;
	}

public:
	atomic_unordered_map(unsigned long int size = 10) :
		map_(new selfdestructing_array<std::pair<size_t, std::shared_ptr<std::pair<const K, V>>>>(size)),
		occupancy_(0),
		accessed_(0)
	{
		for (unsigned long int i = 0; i < size; i++) {
			map_->contents_[i].first = 0;
			map_->contents_[i].second = nullptr;
		}
	}

	class iterator {
		// Keeps a version of the array, it will not get lost if the array is changed
		map_type map_;
		// Keeps a copy of an element, otherwise it would not safely return a reference
		mutable std::shared_ptr<std::pair<const K, V>> contents_;
		// Position is to iterate map->contents_[pos].first
		mutable unsigned long int position_;
		// Parent class only constructor
		iterator(map_type map,
				 std::shared_ptr<std::pair<const K, V>> contents,
				 unsigned long int position) :
			map_(map),
			contents_(contents),
			position_(position) {}
	public:
		iterator(const iterator& other) :
			map_(other.map_),
			contents_(other.contents_),
			position_(other.position_){ }
		iterator& operator= (const iterator& other) {
			map_ = other.map_;
			position_ = other.position_;
			contents_ = other.contents_;
			return *this;
		}

		iterator& operator++ () {
			position_++;
			while (position_ < map_->size_ && (map_->contents_[position_].first <= 1 ||
				   !(contents_= std::atomic_load(&map_->contents_[position_].second)))) {
				position_++;
			}
			if (position_ == map_->size_) contents_ = nullptr;
			return *this;
		}
		iterator& operator-- () {
			if (position_ > 0) {
				position_--;
				while (position_ > 0 && (map_->contents_[position_].first <= 1 ||
					   !(contents_= std::atomic_load(&map_->contents_[position_].second)))) {
					position_--;
				}
			}
			if (position_ == 0 && (map_->contents_[position_].first <= 1 ||
					!(contents_ = std::atomic_load(&map_->contents_[position_].second)))) {
				operator++();
			}
			return *this;
		}
		iterator& operator++ (int unused) { return operator++(); }
		iterator& operator-- (int unused) { return operator--(); }
		bool operator==(const iterator& other) const {
			if (position_ >= map_->size_ && other.position_ >= other.map_->size_)
				return true;
			return (position_ == other.position_);
		}
		bool operator!=(const iterator& other) const { return !(operator==(other)); }
		std::pair<const K, V>& operator*() {
			return *contents_;
		}
		std::pair<const K, V>* operator->() {
			return contents_.get();
		}
		friend class atomic_unordered_map;
	};

	V operator[] (const K& key) {
		// Can't return a reference
		return find(key)->second;
	}

	bool insert(const K& key, const V& value) {
		unsigned long int occupancy = occupancy_;
		unsigned long int map_size = std::atomic_load(&map_)->size_;
		if (occupancy > 0.666 * map_size) rehash(map_size * 2);
		size_t hashed = hash(key);
		std::shared_ptr<std::pair<const K, V>> got, want;
		want = std::make_shared<std::pair<const K, V>>(std::make_pair(key, value));
		read_lock locked(this); // Hold until destructor
		unsigned long int pos = hashed % map_->size_;
		do {
			while (hashed != map_->contents_[pos].first && map_->contents_[pos].first != 0
				   && map_->contents_[pos].first != 1)
				pos = (pos + 1) % map_->size_;
			got = std::atomic_load(&map_->contents_[pos].second);
			if (map_->contents_[pos].first == hashed && got && got->first == key)
				return false; // Already there
			if (got != nullptr) {
				pos = (pos + 1) % map_->size_;
				continue;
			}
		} while (!std::atomic_compare_exchange_weak_explicit(&map_->contents_[pos].second, &got, want,
												std::memory_order_release, std::memory_order_relaxed));
		occupancy_++;
		map_->contents_[pos].first = hashed;
		return true; // Wasn't there
	}
	bool insert(const std::pair<const K, V>& inserted) {
		return insert(inserted.first, inserted.second);
	}
	iterator find(const K& sought) {
		size_t hashed = hash(sought);
		map_type map = std::atomic_load(&map_);
		std::shared_ptr<std::pair<const K, V>> got;
		unsigned long int pos = hashed % map->size_;
		do {
			while (map_->contents_[pos].first != hashed) {
				pos = (pos + 1) % map->size_;
				if (map_->contents_[pos].first == 0) return end();
			}
			got = std::atomic_load(&map->contents_[pos].second);
			if (got->first == sought) break;
			pos = (pos + 1) % map->size_;
		} while (true);
		return iterator(map, got, pos);
	}
	void erase(const K& erased) {
		std::shared_ptr<std::pair<const K, V>> want = nullptr;
		std::shared_ptr<std::pair<const K, V>> got;
		size_t hashed = hash(erased);
		read_lock locked(this);
		unsigned long int pos = hashed % map_->size_;
		do {
			got = std::atomic_load(&map_->contents_[pos].second);
			unsigned long int attempts = 0;
			while (hashed != map_->contents_[pos].first || (got =
											std::atomic_load(&map_->contents_[pos].second))->first != erased) {
				pos = (pos + 1) % map_->size_;
				if (map_->contents_[pos].first == 0) return;
				if (attempts++ > map_->size_) return; // All deleted
			}
			map_->contents_[pos].first = 1;
			got = std::atomic_load(&map_->contents_[pos].second);
			if (got->first != erased) {
				pos = (pos + 1) % map_->size_;
				continue;
			}
		} while (0);
		if (std::atomic_compare_exchange_weak_explicit(&map_->contents_[pos].second, &got, want,
												std::memory_order_release, std::memory_order_relaxed)) {
			occupancy_--;
		}
	}
	iterator erase(iterator& erased) {
		erase(erased->first);
		if (erased.map_->contents_[erased.position_].first <= 1)
			erased--;
		else {
			std::shared_ptr<std::pair<const K, V>> want = nullptr;
			read_lock locked(this);
			erased.map_->contents_[erased.position_].first = 1;
			if (std::atomic_compare_exchange_weak_explicit(&erased.map_->contents_[erased.position_].second,
										&erased.contents_, want, std::memory_order_release, std::memory_order_relaxed)) {
				erased--;
				if (map_ == erased.map_) occupancy_--;
			}
		}
		return erased;
	}
	iterator begin() {
		map_type map = std::atomic_load(&map_);
		unsigned long int pos = 0;
		std::shared_ptr<std::pair<const K, V>> got;
		while (pos < map->size_ && (map->contents_[pos].first <= 1
			   || !(got = std::atomic_load(&map->contents_[pos].second)))) {
			pos++;
		}
		return iterator(map, got, pos);
	}
	iterator end() {
		map_type map = std::atomic_load(&map_);
		return iterator(map, nullptr, ULONG_MAX); // Beyond the range iterators are equal
	}
	unsigned long int capacity() {
		return std::atomic_load(&map_)->size_;
	}
	unsigned long int size() {
		return occupancy_;
	}
};

#endif // ATOMIC_UNORDERED_MAP

