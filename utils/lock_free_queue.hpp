#pragma once

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "utils.hpp"

namespace kse::utils {
	template<typename T>
	class lock_free_queue {
	public:
		explicit lock_free_queue(size_t size) : data_{size, T()} {}

		lock_free_queue(const lock_free_queue&) = delete;

		lock_free_queue(const lock_free_queue&&) = delete;

		lock_free_queue& operator=(const lock_free_queue&) = delete;

		lock_free_queue& operator=(const lock_free_queue&&) = delete;

		auto get_next_write_element() noexcept {
			return &data_[next_write_index_];
		}

		auto get_next_read_element() noexcept {
			return size() ? &data_[next_read_index_] : nullptr;
		}

		auto next_write_index() noexcept {
			next_write_index_ = (next_write_index_ + 1) % data_.size();
			num_elements_++;
		}

		auto next_read_index() noexcept {
			if (size() != 0) {
				next_read_index_ = (next_read_index_ + 1) % data_.size();
				num_elements_--;
			}
		}

		auto size() const noexcept {
			return num_elements_.load();
		}
	private:
		std::vector<T> data_;
		size_t next_write_index_ = 0;
		size_t next_read_index_ = 0;
		std::atomic<size_t> num_elements_ = 0;
	};
}
