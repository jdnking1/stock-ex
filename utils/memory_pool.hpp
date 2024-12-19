#pragma once

#include <vector>

#include <limits>

#include "utils.hpp"

namespace kse::utils {

	template<typename T>
	concept default_constructible = std::is_default_constructible_v<T>;

	template<default_constructible T>
	class memory_pool {
	public:
		explicit memory_pool(size_t size): memory_blocks_(size), free_block_count_(size) {
			for (size_t i = 0; i < size - 1; ++i) {
				memory_blocks_[i].next_free_block_ = i + 1;
			}

			memory_blocks_[size - 1].next_free_block_ = std::numeric_limits<size_t>::max();
			ASSERT(reinterpret_cast<const memory_block*>(&(memory_blocks_[0].data_)) == &(memory_blocks_[0]), "T object should be first member of ObjectBlock.");
		}

		memory_pool(const memory_pool&) = delete;

		memory_pool(const memory_pool&&) = delete;

		memory_pool& operator=(const memory_pool&) = delete;

		memory_pool& operator=(const memory_pool&&) = delete;

		template<typename... Args>
		T* alloc(Args&&... args) noexcept {
			DEBUG_ASSERT(free_block_count_ > 0, "No free memory blocks.");
			auto* memory_block = &memory_blocks_[free_block_index_];
			DEBUG_ASSERT(memory_block->is_free_, "Memory block is not free.");
			memory_block->is_free_ = false;
			T* result = &memory_blocks_[free_block_index_].data_;
			result = new (result) T(std::forward<Args>(args)...);
			free_block_index_ = memory_blocks_[free_block_index_].next_free_block_;
			--free_block_count_;
			return result;
		}

		void free(T* ptr) noexcept {
			size_t block_index = reinterpret_cast<const memory_block*>(ptr) - &memory_blocks_[0];
			DEBUG_ASSERT(block_index < memory_blocks_.size() , "Invalid memory block index.");
			auto* memory_block = &memory_blocks_[block_index];
			DEBUG_ASSERT(!memory_block->is_free_, "Memory block is already free.");
			memory_block->is_free_ = true;
			memory_block->next_free_block_ = free_block_index_;
			free_block_index_ = block_index;
			++free_block_count_;
		}

		size_t capacity() const {
			return memory_blocks_.size();
		}

		size_t available() const {
			return free_block_count_;
		}
 
	private:
		struct memory_block {
			T data_;
			size_t next_free_block_ = 0;
			bool is_free_ = true;
		};

		std::vector<memory_block> memory_blocks_;
		size_t free_block_index_ = 0;
		size_t free_block_count_ = 0;
	};
}