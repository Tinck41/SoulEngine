#pragma once

#include "ecsModule/render_module/material.h"
#include "ecsModule/render_module/render_phase.h"

#include "flecs.h"

namespace se {
	struct UiVertex {
		glm::vec3 position;
		glm::vec2 uv;
		glm::vec4 color{ 1.f, 1.f, 1.f, 1.f};

		std::uint32_t flags;

		glm::vec2 size;
		float border_radius;
		glm::vec4 border_color{ 1.f, 1.f, 1.f, 1.f};
		float border_width;
		glm::vec2 local_pos;
	};

	struct UiVertexNew {
		glm::vec3 position;
		float rotation;
		glm::vec2 scale;
		glm::vec2 padding_1;

		glm::vec2 tex_pos;
		glm::vec2 tex_size;
		glm::vec4 color{ 1.f, 1.f, 1.f, 1.f};

		glm::vec4 border_color{ 1.f, 1.f, 1.f, 1.f};
		float border_width;
		float border_radius;
		glm::vec2 padding_2;

		std::uint32_t flags;
		glm::vec3 padding_3;
	};

	enum class ShaderFlags : std::uint8_t {
		None = 0,
		Textured = 1 << 0,
	};

	Material create_node_material_new(flecs::world& world);
	Material create_node_material(flecs::world& world);
	PhaseContext create_node_context_new(flecs::world& world);
	PhaseContext create_node_context(flecs::world& world);
	RenderPhaseExtractor create_image_node_extractor(flecs::world& world, flecs::entity_t helper);
	RenderPhaseExtractor create_color_node_extractor(flecs::world& world, flecs::entity_t helper);
	RenderPhaseUploader create_node_uploader_new();
	RenderPhaseUploader create_node_uploader();
}
