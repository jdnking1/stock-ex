#pragma once

#include <cstdlib>
#include <iostream>

#include <string>
#include <string_view>
#include <thread>


namespace kse::utils {

	inline auto ASSERT(bool condition, std::string_view message) noexcept {
		if (!condition) [[unlikely]] {
			std::cerr << message << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	inline auto DEBUG_ASSERT(bool condition, std::string_view message) noexcept {
	#ifndef NDEBUG
		ASSERT(condition, message);
	#endif
	}

	inline auto FATAL(std::string_view message) noexcept {
		std::cerr << message << std::endl;
		exit(EXIT_FAILURE);
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
		return std::thread{ 
			[core, func = std::forward<T>(func)](A&&... forwardedArgs) mutable {
				if (core >= 0 && !pin_thread(core)) {
					FATAL("Failed to pin thread to core " + std::to_string(core));
				}
				func(std::forward<A>(forwardedArgs)...);
			},
			std::forward<A>(args)...
		};
	}

}
