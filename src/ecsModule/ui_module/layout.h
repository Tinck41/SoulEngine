#pragma once

#include "glm.hpp"
#include "SDL3_ttf/SDL_ttf.h"

#include <functional>
#include <vector>
#include <optional>

namespace se {
	struct LayoutNode {
		struct TextData {
			std::string text;
			TTF_Font* font;
			float font_scale;
			glm::vec2 text_size;
			glm::vec2 alignment;
			glm::vec2 alignment_offset;
		};

		size_t parent_bfs_index = 0;
		size_t first_child = 0;
		size_t children_num = 0;
		size_t non_absolute_children_num = 0;
		size_t bfs_index = 0;

		bool horizontal;
		bool display;
		bool absolute;

		glm::vec2 size;
		glm::vec2 child_alignment;
		glm::vec2 pos;
		glm::vec2 child_gap;
		glm::vec4 padding;

		glm::vec2 offsets;
		glm::vec2 remaining_size;

		std::pair<bool, bool> fixed;
		std::pair<bool, bool> grow;
		std::pair<bool, bool> fit;

		std::pair<std::optional<float>, std::optional<float>> clip;
		std::pair<std::optional<float>, std::optional<float>> scroll;

		std::pair<std::optional<float>, std::optional<float>> self_alignment;
		std::pair<std::optional<float>, std::optional<float>> min_size;
		std::pair<std::optional<float>, std::optional<float>> max_size;

		std::optional<TextData> text_data;
	};

	class LayoutComposer {
	public:
		LayoutComposer();

		void push_node(LayoutNode node);
		void set_text(size_t bfs_index, const LayoutNode::TextData& text_data);
		void build();
		void clear();

		const LayoutNode& operator[](size_t index) const { return nodes[index]; }
		LayoutNode& operator[](size_t index) { return nodes[index]; }
	private:
		void calculate_fit_width();
		void calculate_fit_height();
		void calculate_grow_shrink_width();
		void calculate_grow_shrink_height();
		void wrap_text();
		void calculate_positions();

		void bfs(std::vector<LayoutNode>& nodes, std::function<void(LayoutNode&)> callback);
		void reverse_bfs(std::vector<LayoutNode>& nodes, std::function<void(LayoutNode&)> callback);

		bool has_children(size_t parent);

		std::vector<LayoutNode> nodes{ LayoutNode{} };

		size_t nodes_num = 1;
	};
}
