#pragma once

#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>


#include <time.h>
#include <stdio.h>


namespace kse::utils {

	inline auto ASSERT(bool condition, std::string_view message) noexcept {
		if (!condition) [[unlikely]] {
			std::cerr << message << std::endl;
			std::exit(EXIT_FAILURE);
		}
	}

	inline auto DEBUG_ASSERT(bool condition [[maybe_unused]], std::string_view message[[maybe_unused]] ) noexcept {
	#ifndef NDEBUG
		ASSERT(condition, message);
	#endif
	}

	inline auto FATAL(std::string_view message) noexcept {
		std::cerr << message << std::endl;
		std::exit(EXIT_FAILURE);
	}

	#ifdef _WIN32

	#include <windows.h>

	inline auto pin_thread(size_t core) noexcept {
		HANDLE threadHandle = GetCurrentThread();

		DWORD_PTR affinityMask = static_cast<DWORD_PTR>(1) << core;

		return SetThreadAffinityMask(threadHandle, affinityMask);
	}

	#elif defined(__linux__)

	inline auto pin_thread(size_t core) noexcept {
		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(core, &cpuset);
		return (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0);
	}
	#else
	#error "Unsupported platform"
	#endif
	/**
	 * Creates a new thread and pins it to a specific core if specified.
	 * The program exits if pinning the thread to the specified core fails.
	 *
	 * @param core The core to pin the thread to. If -1, no pinning is performed.
	 * @param func The function to execute in the new thread.
	 * @param args The arguments to pass to the function.
	 *
	 * @return A std::thread object representing the newly created thread.
	 *
	 */
	template<typename T, typename... A>
	inline auto create_thread(int core, T&& func, A&&... args) noexcept {
		return std::jthread{ 
			[core, func = std::forward<T>(func)](A&&... forwardedArgs) mutable {
				if (core >= 0 && !pin_thread(core)) {
					FATAL("Failed to pin thread to core " + std::to_string(core));
				}
				func(std::forward<A>(forwardedArgs)...);
			},
			std::forward<A>(args)...
		};
	}

	using nananoseconds_t = int64_t;

	constexpr nananoseconds_t NANOS_PER_MICROS = 1000;
	constexpr nananoseconds_t MICROS_PER_MILLIS = 1000;
	constexpr nananoseconds_t MILLIS_PER_SECS = 1000;
	constexpr nananoseconds_t NANOS_PER_MILLIS = NANOS_PER_MICROS * MICROS_PER_MILLIS;
	constexpr nananoseconds_t NANOS_PER_SECS = NANOS_PER_MILLIS * MILLIS_PER_SECS;

	inline auto get_current_timestamp() noexcept {
		return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}

	inline auto& get_curren_time_str(std::string* time_str) {
		const auto clock = std::chrono::system_clock::now();
		const auto time = std::chrono::system_clock::to_time_t(clock);

		char nanos_str[26];

		struct tm tm_time = {};
		errno_t err = ctime_s(nanos_str, sizeof(nanos_str), &time);

		if (err != 0) [[unlikely]] {
			time_str->assign("Error formatting time using ctime_s.");
		}
		else {
			sprintf_s(nanos_str, sizeof(nanos_str), "%.8s.%09lld", nanos_str + 11,
				std::chrono::duration_cast<std::chrono::nanoseconds>(clock.time_since_epoch()).count() % NANOS_PER_SECS);

			time_str->assign(nanos_str);
		}

		return *time_str;
	}

}
