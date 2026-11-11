#pragma once

#include "SDL3/SDL_pixels.h"
#include "glm.hpp"

#include <cstdint>
#include <string>

namespace se {
	struct Color {
		static Color from_uint(std::uint8_t r, std::uint8_t g, std::uint8_t b);
		static Color from_uint(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a);

		static Color from_float(float color[3]);
		static Color from_float(float r, float g, float b);
		static Color from_float(float r, float g, float b, float a);

		static Color from_hex(std::string hex);

		operator SDL_FColor () const;
		operator SDL_Color () const;

		operator glm::vec4 () const;

		std::uint8_t r = 255;
		std::uint8_t g = 255;
		std::uint8_t b = 255;
		std::uint8_t a = 255;
	};

#define TRANSPARENT Color::from_uint(0, 0, 0, 0)
#define BLACK       Color::from_uint(0, 0, 0)
#define WHITE       Color::from_uint(255, 255, 255)
#define RED         Color::from_uint(255, 0, 0)
#define GREEN       Color::from_uint(0, 255, 0)
#define BLUE        Color::from_uint(0, 0, 255)
}
