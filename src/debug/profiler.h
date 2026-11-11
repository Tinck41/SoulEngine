#pragma once

#include <string>
#include <chrono>

namespace se::internal {
	struct ProfileData {
		std::string scope_name;
		std::chrono::high_resolution_clock::time_point time_point;
	};

	void push_scope(std::string name);
	void pop_scope();
}

#ifdef DEBUG
#define push_profile_scope(x) se::internal::push_scope(x)
#define pop_profile_scope() se::internal::pop_scope()
#else
#define push_profile_scope(x)
#define pop_profile_scope()
#endif
