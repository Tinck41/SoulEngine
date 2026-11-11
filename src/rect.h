#pragma once

#include "SDL3/SDL_rect.h"
#include "glm.hpp"

namespace se {
	template<typename T>
	struct Rect {
		using vec_t = glm::vec<2, T>;

		vec_t min;
		vec_t max;

		static Rect<T> from_sdl_rect(const SDL_Rect& rect) {
			return {
				.min = vec_t(rect.x, rect.y),
				.max = vec_t(rect.x + rect.w, rect.y + rect.h),
			};
		}

		static Rect<T> from_pos_size(vec_t pos, vec_t size) {
			return {
				.min = vec_t(pos.x, pos.y),
				.max = vec_t(pos.x + size.x, pos.y + size.y),
			};
		}

		static Rect<T> from_xy_size(T x, T y, vec_t size) {
			return {
				.min = vec_t(x, y),
				.max = vec_t(x + size.x, y + size.y),
			};
		}

		static Rect<T> from_xy_wh(T x, T y, T w, T h) {
			return {
				.min = vec_t(x, y),
				.max = vec_t(x + w, y + h),
			};
		}

		static Rect<T> clamp(Rect<T> value, Rect<T> restrictor) {
			return {
				.min = glm::clamp(value.min, restrictor.min, restrictor.max),
				.max = glm::clamp(value.max, restrictor.min, restrictor.max),
			};
		}

		SDL_Rect to_sdl_rect() const {
			return SDL_Rect{
				.x = static_cast<int>(min.x),
				.y = static_cast<int>(min.y),
				.w = static_cast<int>(get_width()),
				.h = static_cast<int>(get_height()),
			};
		}

		bool operator !=(const Rect& other) {
			return this != &other;
		}

		bool operator ==(const Rect& other) const {
			if (&other == this) {
				return true;
			}

			return min == other.min && max == other.max;
		}

		operator SDL_Rect() {
			return {
				.x = min.x,
				.y = min.y,
				.w = max.x - min.x,
				.h = max.y - min.y,
			};
		}

		T get_width() const {
			return max.x - min.x;
		}

		T get_height() const {
			return max.y - min.y;
		}

		bool contains(const Rect& other) const {
			return glm::all(glm::greaterThanEqual(min, other.min)) && glm::all(glm::lessThanEqual(max, other.max));
		}

		bool overlap(const Rect& other) const {
			return glm::all(glm::lessThanEqual(min, other.max)) &&
				glm::all(glm::greaterThanEqual(max, other.min));
		}

		bool contains_point(const vec_t& point) const {
			return point.x > min.x && point.x < max.x && point.y > min.y && point.y < max.y;
		}
	};

	using RectF = Rect<float>;
	using RectI = Rect<int>;
}
