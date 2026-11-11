#pragma once

#include "SDL3/SDL.h"
#include "flecs.h"
#include "texture.h"
#include "color.h"
#include "texture_atlas.h"

#include <memory>
#include <set>
#include <variant>
#include <vector>

namespace se {
	struct UiTreeChanged {};

	struct CustomNodeIndex {
		size_t value;
	};

	struct Node {
		enum class Overflow : std::uint8_t {
			Visible,
			Clip,
			Scroll,
		};

		struct Fixed {
			float value;
		};

		struct Fit {
			std::optional<float> min;
			std::optional<float> max;
		};

		struct Grow {
			std::optional<float> min;
			std::optional<float> max;
		};

		using SizePolicy = std::variant<Fixed, Fit, Grow>;

		std::pair<SizePolicy, SizePolicy> sizing_policy = { Fit{}, Fit{} };
		std::pair<Overflow, Overflow> overflow = { Overflow::Visible, Overflow::Visible };
		std::pair<std::optional<float>, std::optional<float>> self_alignment;
		std::pair<std::optional<float>, std::optional<float>> scroll_offset;

		glm::vec2 pos;
		glm::vec2 size;
		glm::vec2 child_alignment;
		glm::vec2 child_gap;
		glm::vec2 offsets;
		glm::vec4 margin; // l, r, t, b
		glm::vec4 padding; // l, r, t, b

		bool display = true;
		bool absolute = false;

		float border_radius = 0.f;
		float border_width = 0.f;
	};

	enum class GrowDirection : std::uint8_t {
		Horizontal,
		Vertical,
	};

	enum class Overflow : std::uint8_t {
		Visible,
		Clip,
		Scroll,
	};

	struct Vertical {};
	struct Horizontal {};

	struct Fixed {
		float value;
	};

	struct Fit {
		std::optional<float> min;
		std::optional<float> max;
	};

	struct Grow {
		std::optional<float> min;
		std::optional<float> max;
	};

	struct SizeStrategy {
		using Strategy = std::variant<Fixed, Fit, Grow>;

		Strategy x{ Fit{} };
		Strategy y{ Fit{} };
	};

	struct NodeIndex {
		size_t dfs = 0;
		size_t bfs = 0;

		bool external_dfs_source = false;
		bool external_bfs_source = false;
	};

	enum class Type {
		Retained,
		Immediate,
	};

	enum class Interaction {
		None,
		Hovered,
		Clicked,
	};

	enum class FocusStrategy {
		Block,
		Pass,
	};

	struct Button {
		bool hovered = false;
	};

	struct Image {
		std::shared_ptr<Texture> texture;
		std::optional<TextureAtlas> texture_atlas;
		Color color;
	};

	struct Text : public std::string {
		using std::string::string;

		Text(const std::string& string) : std::string(string) {}
		Text(const flecs::string& string) : std::string(string.c_str()) {}
		Text(flecs::string string) : std::string(string.c_str()) {}
		Text(const char* string) : std::string(string) {}
	};

	struct Composite {
		enum class Kind : std::uint8_t {
			Three
		};

		static Composite create_3_v() {
			return {{
				0b00000'010,
				0b00000'010,
				0b00000'010
			}};
		}

		static Composite create_3_h() {
			return {{
				0b00000'000,
				0b00000'111,
				0b00000'000
			}};
		}

		static Composite create_9() {
			return {{
				0b00000'111,
				0b00000'111,
				0b00000'111
			}};
		}

		std::array<std::uint8_t, 3> structure;
	};

	struct BackgroundColor : public Color {
		using Color::Color;

		BackgroundColor(const Color& c) : Color(c) {}
	};

	struct BorderColor : public Color {
		using Color::Color;

		BorderColor(const Color& c) : Color(c) {}
	};

	struct TempInteraction {
		flecs::entity entity;
		float stack_index;
		FocusStrategy focus_strategy;
	};

	struct InsertBefore {};

	struct UiModule {
		UiModule(flecs::world& world);
	};
}
