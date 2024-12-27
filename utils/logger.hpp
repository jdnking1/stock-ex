#pragma once

#include <atomic>
#include <cstdint>

#include <cstring>
#include <string>
#include <fstream>
#include <thread>
#include "utils.hpp"
#include "lock_free_queue.hpp"


namespace kse::utils {

	using namespace std::literals::chrono_literals;

	constexpr size_t LOG_QUEUE_SIZE = 1024;

	enum class log_type : int8_t {
		CHAR = 0,
		INTEGER = 1,
		LONG_INTEGER = 2,
		LONG_LONG_INTEGER = 3,
		UNSIGNED_INTEGER = 4,
		UNSIGNED_LONG_INTEGER = 5,
		UNSIGNED_LONG_LONG_INTEGER = 6,
		FLOAT = 7,
		DOUBLE = 8,
		STRING = 9
	};

	struct log_entry {
		log_type type_;
		union {
			char c;
			int i;
			long l;
			long long ll;
			unsigned u;
			unsigned long ul;
			unsigned long long ull;
			float f;
			double d;
			char s[256];
		} data_;
	};

	class logger {
	public:
		explicit logger(const std::string& file_name)
			: file_name_(file_name), log_queue_(LOG_QUEUE_SIZE) {
			file_.open(file_name);
			ASSERT(file_.is_open(), "Could not open log file:" + file_name);
			worker_ = create_thread(-1, [this]() { flush_queue(); });
			ASSERT(worker_.joinable(), "Failed to start logger thread.");
		}

		~logger() {
			std::string time_str;
			std::cerr << (&time_str) << " Flushing and closing logger for " << file_name_ << std::endl;

			while (log_queue_.size()) {
				std::this_thread::sleep_for(1s);
			}
			running_ = false;

			file_.close();
			std::cerr << get_curren_time_str(&time_str) << " logger for " << file_name_ << " exiting." << std::endl;
		}

		logger() = delete;

		logger(const logger&) = delete;

		logger(const logger&&) = delete;

		logger& operator=(const logger&) = delete;

		logger& operator=(const logger&&) = delete;

		void flush_queue() noexcept {
			while (running_) {
				for (auto next = log_queue_.get_next_read_element(); log_queue_.size() && next; next = log_queue_.get_next_read_element()) {
					switch (next->type_) {
					case log_type::CHAR:
						file_ << next->data_.c;
						break;
					case log_type::INTEGER:
						file_ << next->data_.i;
						break;
					case log_type::LONG_INTEGER:
						file_ << next->data_.l;
						break;
					case log_type::LONG_LONG_INTEGER:
						file_ << next->data_.ll;
						break;
					case log_type::UNSIGNED_INTEGER:
						file_ << next->data_.u;
						break;
					case log_type::UNSIGNED_LONG_INTEGER:
						file_ << next->data_.ul;
						break;
					case log_type::UNSIGNED_LONG_LONG_INTEGER:
						file_ << next->data_.ull;
						break;
					case log_type::FLOAT:
						file_ << next->data_.f;
						break;
					case log_type::DOUBLE:
						file_ << next->data_.d;
						break;
					case log_type::STRING:
						file_ << next->data_.s;
						break;
					}
					log_queue_.next_read_index();
				}
				file_.flush();
				std::this_thread::sleep_for(10ms);
			}
		}

		auto push_value(const log_entry& log_entry) noexcept {
			*(log_queue_.get_next_write_element()) = log_entry;
			log_queue_.next_write_index();
		}

		auto push_value(const char value) noexcept {
			push_value(log_entry{ log_type::CHAR, {.c = value} });
		}

		auto push_value(const int value) noexcept {
			push_value(log_entry{ log_type::INTEGER, {.i = value} });
		}

		auto push_value(const long value) noexcept {
			push_value(log_entry{ log_type::LONG_INTEGER, {.l = value} });
		}

		auto push_value(const long long value) noexcept {
			push_value(log_entry{ log_type::LONG_LONG_INTEGER, {.ll = value} });
		}

		auto push_value(const unsigned value) noexcept {
			push_value(log_entry{ log_type::UNSIGNED_INTEGER, {.u = value} });
		}

		auto push_value(const unsigned long value) noexcept {
			push_value(log_entry{ log_type::UNSIGNED_LONG_INTEGER, {.ul = value} });
		}

		auto push_value(const unsigned long long value) noexcept {
			push_value(log_entry{ log_type::UNSIGNED_LONG_LONG_INTEGER, {.ull = value} });
		}

		auto push_value(const float value) noexcept {
			push_value(log_entry{ log_type::FLOAT, {.f = value} });
		}

		auto push_value(const double value) noexcept {
			push_value(log_entry{ log_type::DOUBLE, {.d = value} });
		}

		auto push_value(const char* value) noexcept {
			log_entry l{ log_type::STRING, {.s = {}} };

#ifdef _WIN32
			errno_t err = strncpy_s(l.data_.s, sizeof(l.data_.s), value, _TRUNCATE);

			if (err == 0) {
				push_value(l);
			}
#else
			if (value) {
				std::strncpy(l.data_.s, value, sizeof(l.data_.s) - 1);
				l.data_.s[sizeof(l.data_.s) - 1] = '\0';
				push_value(l);
			}
#endif
			
		}


		auto push_value(const std::string& value) noexcept {
			push_value(value.c_str());
		}

		template<typename T, typename... A>
		auto log(const char* s, const T& value, A... args) noexcept {
			while (*s) {
				if (*s == '%') {
					if (*(s + 1) == '%') [[unlikely]] {
						++s;
					}
					else {
						push_value(value);
						log(s + 1, args...);
						return;
					}
				}
				push_value(*s++);
			}
			FATAL("extra arguments provided to log()");
		}

		auto log(const char* s) noexcept {
			while (*s) {
				if (*s == '%') {
					if (*(s + 1) == '%') [[unlikely]] {
						++s;
					}
					else {
						FATAL("missing arguments to log()");
					}
				}
				push_value(*s++);
			}
		}

	private:
		std::string file_name_;
		std::ofstream file_;

		lock_free_queue<log_entry> log_queue_;
		std::atomic<bool> running_ = true;

		std::jthread worker_;
	};
}