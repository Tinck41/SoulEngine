#pragma once

#include "ecsModule/mesh3d_module/components.h"
#include "ecsModule/render_module/light_uniform.h"
#include "ecsModule/render_module/render_phase.h"
#include "ecsModule/render_module/material.h"

#include "flecs.h"
#include "glm.hpp"

namespace se {
	struct ExtractedMesh3d {
		flecs::entity_t entity;
		flecs::entity_t material_id;

		uint64_t group_id;

		Vertex3d* vertices = nullptr;
		uint32_t* indices = nullptr;

		size_t vertices_size = 0;
		size_t indices_size = 0;

		SDL_GPUTexture* diffuse_map = nullptr;
		SDL_GPUTexture* specular_map = nullptr;

		glm::mat4 transform;
		Color color;

		PhongMaterial material;
	};

	struct Mesh3dInstance {
		glm::mat4 model;
		uint32_t flags = 0;
		uint32_t padding[3];
	};

	using ExtractedMeshes3d = std::vector<ExtractedMesh3d>;
	using UploadedMeshes3d = std::unordered_set<uint64_t>;

	struct Mesh3dModule {
		Mesh3dModule(flecs::world& world);
	};

	enum class Mesh3dShaderFlags : uint32_t {
		None = 0,
		Textured = 1 << 0,
	};

	se::Material create_mesh3d_material(flecs::world& world);
	se::PhaseContext create_mesh3d_context(flecs::world& world);
	se::RenderPhaseExtractor create_mesh3d_extractor(flecs::world& world, flecs::entity_t helper);
	se::RenderPhaseUploader create_mesh3d_uploader();

	glm::mat4 extract_3d_view(const Camera &camera, const GlobalTransform &transfrom);

	//std::pair<uint64_t, uint64_t> request_mesh3d_allocate(Mesh3dAllocator& allocator, std::vector<Vertex3d> vertices, std::vector<uint32_t> indices);
}
