#pragma once

#include "ecsModule/cameraModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "default_uniform.h"
#include "glm.hpp"
#include "flecs.h"
#include "SDL3/SDL.h"
#include "rect.h"

#include <vector>

namespace se {
	class Texture;
	class Aabb;

	struct RenderPhaseExtractor {
		void(*callback)(flecs::iter&);

		flecs::query<> query;
		flecs::entity_t helper;
	};

	struct RenderPhaseSorter {
		void(*callback)(flecs::world&, flecs::entity&, flecs::entity&);
	};

	struct RenderPhaseUploader {
		void(*callback)(flecs::world&, flecs::entity&, flecs::entity&, SDL_GPUDevice*, SDL_GPUCopyPass*);
	};

	struct RenderPhaseRenderer {
		void(*callback)(flecs::world&, flecs::entity&, flecs::entity&, const DefaultUniform&, SDL_GPURenderPass*, SDL_GPUCommandBuffer*);
	};

	struct PhaseContext {
		SDL_GPUBuffer* index_buffer = nullptr;
		SDL_GPUBuffer* vertex_buffer = nullptr;
		SDL_GPUBuffer* vert_storage_buffer = nullptr;
		SDL_GPUBuffer* frag_storage_buffer = nullptr;

		size_t num_indices  = 0;
		size_t num_vertices = 0;
		size_t num_instances = 0;

		SDL_GPUTransferBuffer* transfer_buffer = nullptr;

		SDL_GPUIndexElementSize index_element_size;
	};

	struct RenderItem {
		flecs::entity_t entity;
		flecs::entity_t context_entity;
		flecs::entity_t material_id;

		size_t extracted_index = 0;

		float sort_value;

		std::optional<RectI> scissor;

		std::vector<SDL_GPUTexture*> textures;

		std::uint32_t num_indices = 0;
		std::uint32_t num_instances = 0;
		std::uint32_t first_index = 0;
		std::int32_t vertex_offset = 0;
		std::uint32_t first_instance = 0;

		std::uint32_t batch_size = 0;

		bool operator<(const RenderItem& other) const noexcept {
			if (sort_value == other.sort_value) {
				return entity < other.entity;
			}
			return sort_value < other.sort_value;
		}
	};

	struct RenderPhase {
		std::vector<flecs::entity_t> extractors; // shoud we also collect all materails in one array in exractors?
		std::vector<flecs::entity_t> sorters;
		std::vector<flecs::entity_t> uploaders;
		std::vector<flecs::entity_t> binders;
		std::vector<flecs::entity_t> renderers;

		std::vector<RenderItem> items;

		glm::mat4(*extract_view_callback)(const Camera& camera, const GlobalTransform& transform);
	};

	using RenderPhases = std::vector<flecs::entity_t>;
}
