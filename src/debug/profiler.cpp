#include "profiler.h"
#include "spdlog/spdlog.h"

#include <vector>

using namespace se::internal;

std::vector<ProfileData> profile_stack;

void se::internal::push_scope(std::string name) {
	profile_stack.emplace_back(std::move(name), std::chrono::high_resolution_clock::now());
}

void se::internal::pop_scope() {
	const auto& last_profile = profile_stack.back();
	spdlog::info("{}: {}", last_profile.scope_name, std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - last_profile.time_point).count());
	profile_stack.pop_back();
}
