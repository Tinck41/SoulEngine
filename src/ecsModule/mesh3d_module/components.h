#pragma once

#include "texture.h"
#include "tiny_obj_loader.h"
#include "arena.h"

#include <memory>
#include <unordered_map>

namespace se {
	struct Vertex3d {
		glm::vec3 position;
		glm::vec4 color{ 1.f, 1.f, 1.f, 1.f};
		glm::vec2 uv;
		glm::vec3 normal;
	};

	struct Mesh3d {
		std::shared_ptr<Texture> diffuse_map;
		std::shared_ptr<Texture> specular_map;

		Color color;

		Vertex3d* vertices = nullptr;
		uint32_t* indices = nullptr;

		size_t vertices_size = 0;
		size_t indices_size = 0;
	};

	struct Mesh3dGpuMeta {
		uint64_t vertex_offset;
		uint64_t index_offset;
	};

	struct Mesh3dGpu {
		SDL_GPUBuffer* vertex_buffer = nullptr;
		SDL_GPUBuffer* index_buffer = nullptr;
	};
	
	// static meshed:
	//  - common buffers(vertex/index/storage)
	//  - different material
	//  - mesh meta info?
	//  dynamic mesh:
	//  - per mesh buffers

	struct Mesh3dUniform {
		glm::mat4 model;
	};

	struct Mesh3dDescription {
		enum class OptimizationFlags : uint8_t {
			None               = 0,
			Indexing           = 1 << 0,
			VertexCache        = 1 << 1,
			Overdraw           = 1 << 2,
			VertexFetch        = 1 << 3,
			VertexQuantization = 1 << 4,
			IndexFiltering     = 1 << 5,
			ShadowIndexing     = 1 << 6,

			All = Indexing | VertexCache | Overdraw | VertexFetch | VertexQuantization | IndexFiltering | ShadowIndexing,
		};

		friend constexpr OptimizationFlags operator|(OptimizationFlags lhs, OptimizationFlags rhs) {
			using base_t = std::underlying_type_t<OptimizationFlags>;

			return static_cast<OptimizationFlags>(
				static_cast<base_t>(lhs) | static_cast<base_t>(rhs)
			);
		}
		friend constexpr OptimizationFlags operator&(OptimizationFlags lhs, OptimizationFlags rhs) {
			using base_t = std::underlying_type_t<OptimizationFlags>;

			return static_cast<OptimizationFlags>(
				static_cast<base_t>(lhs) & static_cast<base_t>(rhs)
			);
		}

		enum class Format : std::uint8_t {
			obj,
			fbx,
			gltf,
		};

		std::string texture_path;
		std::string model_path;

		Format model_format;

		OptimizationFlags optimization_flags = OptimizationFlags::All;
	};

	struct ObjAsset {
		std::string path;
		std::string diffuse_map_path;
		std::string specular_map_path;
	};

	struct Mesh3dCachedData {
		uint64_t vertex_offset = 0;
		uint64_t vertex_count = 0;
		uint64_t index_offset  = 0;
		uint64_t index_count  = 0;
	};

	struct Mesh3dAllocator {
		struct AllocateRequest {
			std::vector<Vertex3d> vertices;
			std::vector<uint32_t> indices;
		};

		std::vector<AllocateRequest> allocate_requests;

		uint64_t vertex_offset = 0;
		uint64_t index_offset  = 0;

		std::unordered_map<std::string, Mesh3dCachedData> cache; 
	};

	struct Mesh3dLibrary {
		//std::unordered_map<uint64_t, 
	};
}
