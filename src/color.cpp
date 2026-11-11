#include "color.h"

#include <limits>

using namespace se;

Color Color::from_uint(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
	return from_uint(r, g, b, 255);
}

Color Color::from_uint(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
	return { r, g, b, a };
}

Color Color::from_float(float color[3]) {
	return from_float(color[0], color[1], color[2], 1.f);
}

Color Color::from_float(float r, float g, float b) {
	return from_float(r, g, b, 1.f);
}

Color Color::from_float(float r, float g, float b, float a) {
	constexpr auto max = std::numeric_limits<std::uint8_t>::max();

	Color color;

	color.r = r * max;
	color.g = g * max;
	color.b = b * max;
	color.a = a * max;

	return color;
}

Color Color::from_hex(std::string hex) {
	if (hex[0] == '#') {
		hex.erase(0, 1);
	}

	if (hex.substr(0, 2) == "0x") {
		hex.erase(0, 2);
	}

	const auto value = std::stoul(hex, nullptr, 16);

	return {
		static_cast<std::uint8_t>((value >> 16) & 0xFF),
		static_cast<std::uint8_t>((value >> 8) & 0xFF),
		static_cast<std::uint8_t>(value & 0xFF)
	};
}

Color::operator SDL_FColor() const {
	return {
		static_cast<float>(r) / 255.f,
		static_cast<float>(g) / 255.f,
		static_cast<float>(b) / 255.f,
		static_cast<float>(a) / 255.f,
	};
}

Color::operator SDL_Color() const {
	return { r, g, b, a };
}

Color::operator glm::vec4() const {
	return {
		static_cast<float>(r) / 255.f,
		static_cast<float>(g) / 255.f,
		static_cast<float>(b) / 255.f,
		static_cast<float>(a) / 255.f,
	};
}

