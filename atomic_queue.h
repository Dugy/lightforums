#ifndef ATOMIC_QUEUE
#define ATOMIC_QUEUE

#include <memory>
#include <utility>
#include <functional>
#include <atomic>
#include <chrono>
#include <thread>
#include <climits>

template<typename V>
class atomic_queue {
	// A queue that can be simultaneously accessed from many threads...
	template <typename T>
	struct selfdestructing_array {
		T* contents_;
		unsigned long int size_;
		selfdestructing_array(unsigned long int size) :
			contents_(new T[size]),
			size_(size) {}
		~selfdestructing_array() { delete[] contents_; }
	};

	typedef std::shared_ptr<selfdestructing_array<V>> holder_type;
	holder_type data_;
	std::atomic_uint_fast64_t start_; // Here's where entries whose reading was finished start
	std::atomic_uint_fast64_t unread_; // Here's where unread entries start
	std::atomic_uint_fast64_t written_; // Here's where entries that are being written start
	std::atomic_uint_fast64_t end_; // Here's where the next entry will be written

	std::atomic_uint accessed_; // Twice the number of accessors, odd numbers mean that it's time to rehash

	struct read_lock {
		atomic_queue& parent_;
		read_lock(atomic_queue* parent):
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
	void resize(unsigned long int new_size) {
		// This one will be executed only a few times, the lock is thus not performance-degradating
		// Will only enlarge the array, never shrink it
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
		if (std::atomic_load(&data_)->size_ >= new_size) {
			accessed_--;
			return; // Already enlarged
		}
		// Others clearing our way
		while (accessed_ != 3) // It's locked by this thread already, so 3 = 2 + 1
			std::this_thread::sleep_for(std::chrono::microseconds(1));

		// Way's clear, resize
		unsigned long int start = start_;
		unsigned long int length = size();
		unsigned long int end;
		unsigned long int old_size = data_->size_;
		holder_type data(new selfdestructing_array<V>(new_size));
		for (end = 0; end < length; end++) {
			data->contents_[end] = data_->contents_[(start + end + old_size) % old_size];
		}
		start_ = 0;
		unread_ = 0;
		written_ = end;
		end_ = end;
		data_ = data;
		accessed_ = 2;
	}


public:
	atomic_queue(unsigned long int size = 10) :
		data_(new selfdestructing_array<V>(size)),
		start_(0),
		unread_(0),
		written_(0),
		end_(0),
		accessed_(0)
	{ }

	unsigned long int size() {
		unsigned long int start = unread_;
		unsigned long int end = written_;
		if (start <= end) return end - start;
		else return std::atomic_load(&data_)->size_ - (start - end);
	}

	void push(const V& val) {
		read_lock lock(this);
		unsigned long int pos, updated;
		unsigned long int data_size = data_->size_;
		do {
			pos = end_;
			updated = (pos + 1) % data_size;
		} while (!std::atomic_compare_exchange_weak_explicit(&end_, &pos, updated,
										std::memory_order_release, std::memory_order_relaxed));
		if (pos == ((start_ + data_size) - 2) % data_size) resize(std::atomic_load(&data_)->size_ * 2);
		std::atomic_load(&data_)->contents_[pos] = val;
		std::cerr << "Writing at " << pos << ": " << val << std::endl;
		// Move the iterator showing already written parts only if those before were already written
		while (!std::atomic_compare_exchange_weak_explicit(&written_, &pos, updated,
										std::memory_order_release, std::memory_order_relaxed)) {};
	}

	V pop() {
		read_lock lock(this);
		unsigned long int pos, updated;
		do {
			pos = unread_;
			updated = (pos + 1) % data_->size_;
		} while (!std::atomic_compare_exchange_weak_explicit(&unread_, &pos, updated,
										std::memory_order_release, std::memory_order_relaxed));
		V result = std::atomic_load(&data_)->contents_[pos];
		std::cerr << "Reading from " << pos << ": " << result << std::endl;
		// Move the iterator showing already read parts only if those before were already read
		while (!std::atomic_compare_exchange_weak_explicit(&start_, &pos, updated,
										std::memory_order_release, std::memory_order_relaxed)) {};
		return result;
	}

};

#endif // ATOMIC_QUEUE

