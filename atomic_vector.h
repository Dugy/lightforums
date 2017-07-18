#ifndef ATOMIC_VECTOR
#define ATOMIC_VECTOR

#include <memory>
#include <utility>
#include <functional>
#include <atomic>
#include <chrono>
#include <thread>
#include <climits>

template<typename V>
class atomic_vector {
	// A vector that can be simultaneously accessed from many threads. Operations push_back,
	// pop_back, read [] and write are quite fast and practical, insert and erase modify
	// the indexes and can unavoidably break iteration or lead to reading of uninitialised data.
	// Insertion and erasure can be terribly slow if too much manipulation is done.
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
	std::atomic_uint_fast64_t occupancy_;

public:
	atomic_vector(unsigned long int size = 10) :
		data_(new selfdestructing_array<V>(size)),
		occupancy_(0)
	{ }

	void push_back(const V& inserted) {
		// Get a spot
		unsigned long int place = occupancy_++;
		// Increase data size if necessary, update pointer only after copying and testing
		// that it was not done earlier
		holder_type data = std::atomic_load(&data_);
		holder_type new_data;
		unsigned long int occupancy;
		if (occupancy_ >= data->size_) {
			do {
				data = std::atomic_load(&data_);
				occupancy = occupancy_;
				new_data = std::make_shared<selfdestructing_array<V>>(occupancy * 2);
				for (unsigned long int i = 0; i < (occupancy = occupancy_); i++)
					new_data->contents_[i] = data->contents_[i];
			} while (occupancy != occupancy_ || !std::atomic_compare_exchange_weak_explicit(&data_,
							&data, new_data, std::memory_order_release, std::memory_order_relaxed));
		}
		// Write it there, redo if a reallocation happened
		holder_type old;
		do {
			data->contents_[place] = inserted;
			old = data;
		} while (old != (data = std::atomic_load(&data_)));
	}

	void pop_back() {
		occupancy_--;
	}

	unsigned long int size() { return occupancy_; }

	V operator[] (unsigned long int position) {
		return std::atomic_load(&data_)->contents_[position];
	}

	void write(unsigned long int position, const V& inserted) {
		holder_type data = std::atomic_load(&data_);
		holder_type old;
		do {
			data->contents_[position] = inserted;
			old = data;
		} while (old != (data = std::atomic_load(&data_)));
	}

	void insert(unsigned long int position, const V& inserted) {
		occupancy_++;
		holder_type data;
		holder_type new_data;
		unsigned long int occupancy;
		do {
			holder_type data = std::atomic_load(&data_);
			occupancy = occupancy_;
			unsigned long int needed = (occupancy < data->size_ * 1.8)
									   ? data->size_: std::max(data->size_ * 2, occupancy);
			new_data = std::make_shared<selfdestructing_array<V>>(needed);
			for (unsigned long int i = 0; i < position; i++)
				new_data->contents_[i] = data->contents_[i];
			new_data->contents_[position] = inserted;
			for (unsigned long int i = position; i < (occupancy = occupancy_); i++)
				new_data->contents_[i + 1] = data->contents_[i];
		} while (occupancy != occupancy_ || !std::atomic_compare_exchange_weak_explicit(&data_,
						&data, new_data, std::memory_order_release, std::memory_order_relaxed));
	}

	void erase(unsigned long int position) {
		occupancy_--;
		holder_type data;
		holder_type new_data;
		unsigned long int occupancy;
		do {
			data = std::atomic_load(&data_);
			occupancy = occupancy_;
			unsigned long int needed = (occupancy < data->size_ * 1.8)
									   ? data->size_: std::max(data->size_ * 2, occupancy);
			// Resize even if we're close to needing it
			new_data = std::make_shared<selfdestructing_array<V>>(needed);
			for (unsigned long int i = 0; i < position; i++)
				new_data->contents_[i] = data->contents_[i];
			for (unsigned long int i = position; i < (occupancy = occupancy_); i++)
				new_data->contents_[i] = data->contents_[i + 1];
		} while (occupancy != occupancy_ || !std::atomic_compare_exchange_weak_explicit(&data_,
						&data, new_data, std::memory_order_release, std::memory_order_relaxed));
	}
};

#endif // ATOMIC_VECTOR

