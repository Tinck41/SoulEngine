#pragma once

#include "flecs.h"

#include <functional>
#include <string>

namespace se {
	class Application {
	public:
		static Application create();

		template<typename T>
		Application& add_module() {
			world.import<T>();

			return *this;
		};

		template<typename T, typename... Args>
		Application& add_system(void(*callback)(Args&...)) {
			world.system<Args...>()
				.template kind<T>()
				.each(callback);

			return *this;
		}

		template<typename T, typename... Args>
		Application& add_system(void(*callback)(flecs::entity, Args&...)) {
			world.system<Args...>()
				.template kind<T>()
				.each(callback);

			return *this;
		}

		template<typename T, typename... Args>
		Application& add_system(void(*callback)(flecs::iter&, size_t, Args&...)) {
			world.system<Args...>()
				.template kind<T>()
				.each(callback);

			return *this;
		}

		template<typename T, typename... Args>
		Application& add_system(void(*callback)(flecs::iter&)) {
			world.system<Args...>()
				.template kind<T>()
				.run(callback);

			return *this;
		}

		template<typename T, typename... Args>
		Application& add_system(void(*callback)(flecs::world&, Args&...)) {
			world.system<Args...>()
				.template kind<T>()
				.each([callback, this](Args&... args) {
					callback(world, args...);
				});

			return *this;
		}

		template<typename T, typename... Args>
		Application& add_system(T phase, void(*callback)(flecs::iter& it, size_t i, Args&...)) {
			world.system<Args...>()
				.kind(phase)
				.each([callback, this](flecs::iter& it, size_t i, Args&... args) {
					callback(it, i, args...);
				});

			return *this;
		}

		Application& build_system(void(*callback)(flecs::world&)) {
			callback(world);

			return *this;
		}

		Application& add_script(const std::string& path) {
			world.script().filename(path.c_str()).run();

			return *this;
		}

		void run();
	private:
		Application() = default;

		bool init_sdl();

		void init_phases();

		flecs::world world;
	};
}
