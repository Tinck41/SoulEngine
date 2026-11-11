#include "module.h"
#include "components.h"

#include "ecsModule/assetModule/module.h"
#include "ecsModule/common.h"
#include "ecsModule/render_module/module.h"
#include "ecsModule/windowModule/module.h"
#include "spdlog/spdlog.h"
#include "utils/sdl.h"
#include "ext/matrix_clip_space.hpp"
#include "ext/matrix_transform.hpp"
#include "meshoptimizer.h"

using namespace se;

#define MESH3D_MAX_VERTEX_COUNT 100'000
#define MESH3D_MAX_INDEX_COUNT 100'000
#define MESH3D_MAX_INSTANCE_COUNT 100'000

uint64_t group_by_relation(flecs::world_t *world, flecs::table_t *table, flecs::entity_t id, void *) {
	flecs::entity_t target = 0;

	if (ecs_search_relation(world, table, 0, id, flecs::IsA, flecs::Up, &target, nullptr, nullptr) == -1) {
		return 0;
	}

	return target;
}

Mesh3dModule::Mesh3dModule(flecs::world& world) {
	world.module<Mesh3dModule>();

	world.component<Mesh3dDescription::OptimizationFlags>()
		.constant("None",               Mesh3dDescription::OptimizationFlags::None)
		.constant("Indexing",           Mesh3dDescription::OptimizationFlags::Indexing)
		.constant("VertexCache",        Mesh3dDescription::OptimizationFlags::VertexCache)
		.constant("Overdraw",           Mesh3dDescription::OptimizationFlags::Overdraw)
		.constant("VertexFetch",        Mesh3dDescription::OptimizationFlags::VertexFetch)
		.constant("VertexQuantization", Mesh3dDescription::OptimizationFlags::VertexQuantization)
		.constant("IndexFiltering",     Mesh3dDescription::OptimizationFlags::IndexFiltering)
		.constant("ShadowIndexing",     Mesh3dDescription::OptimizationFlags::ShadowIndexing)
		.constant("All",                Mesh3dDescription::OptimizationFlags::All);

	world.component<Mesh3dGpu>()
		.add(flecs::OnInstantiate, flecs::Inherit);
	world.component<Mesh3d>()
		.add(flecs::OnInstantiate, flecs::Inherit)
		.add(flecs::With, world.component<Mesh3dGpu>())
		.add(flecs::With, world.component<Mesh3dInstance>());
	world.component<ObjAsset>()
		.member("path", &ObjAsset::path)
		.member("texture_path", &ObjAsset::texture_path)
		.add(flecs::OnInstantiate, flecs::Inherit);
	world.component<Mesh3dUniform>()
		.member("model", &Mesh3dUniform::model);
	world.component<Mesh3dAllocator>()
		.add(flecs::Singleton);
	world.component<Mesh3dDescription>()
		.member("texture_path", &Mesh3dDescription::texture_path)
		.member("model_path", &Mesh3dDescription::model_path)
		.member("model_format", &Mesh3dDescription::model_format)
		.member("optimization_flags", &Mesh3dDescription::optimization_flags);

	world.observer<ObjAsset, AssetStorage, RenderDevice>()
		.term_at(1).src<AssetStorage>()
		.term_at(2).src<RenderDevice>()
		.event(flecs::OnSet)
		.each([&world](flecs::entity entity, ObjAsset& asset, AssetStorage& storage, RenderDevice& device) {
			auto mesh_e = entity.world().entity(asset.path.c_str())
				.add(flecs::Prefab);

			auto& mesh = mesh_e.ensure<Mesh3d>();

			std::vector<Vertex3d> vertices;
			std::vector<uint32_t> indices;

			mesh.texture = storage.load_texture(*device.gpu, asset.texture_path);

			tinyobj::attrib_t attrib;
			std::vector<tinyobj::shape_t> shapes;
			std::vector<tinyobj::material_t> materials;

			tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, asset.path.c_str());

			for (const auto& shape : shapes) {
				size_t index_offset = 0;

				for (const auto num_face_vertices : shape.mesh.num_face_vertices) {
					for (size_t fv = 0; fv  < num_face_vertices; ++fv) {
						const auto idx = shape.mesh.indices[fv + index_offset];

						Vertex3d vertex;

						vertex.position.x = attrib.vertices[3 * idx.vertex_index + 0];
						vertex.position.y = attrib.vertices[3 * idx.vertex_index + 1];
						vertex.position.z = attrib.vertices[3 * idx.vertex_index + 2];

						if (idx.texcoord_index > 0) {
							vertex.uv.x = attrib.texcoords[2 * idx.texcoord_index + 0];
							vertex.uv.y = 1.f - attrib.texcoords[2 * idx.texcoord_index + 1];
						}

						if (idx.normal_index > 0) {
							vertex.normal.x = attrib.normals[3 * idx.normal_index + 0];
							vertex.normal.y = attrib.normals[3 * idx.normal_index + 1];
							vertex.normal.z = attrib.normals[3 * idx.normal_index + 2];
						}

						indices.push_back(vertices.size());
						vertices.push_back(vertex);
					}

					index_offset += num_face_vertices;
				}
			}

			mesh.vertices = static_cast<Vertex3d*>(malloc(sizeof(vertices[0]) * vertices.size()));
			mesh.indices = static_cast<uint32_t*>(malloc(sizeof(indices[0]) * indices.size()));

			std::memcpy(mesh.vertices, vertices.data(), sizeof(vertices[0]) * vertices.size());
			std::memcpy(mesh.indices, indices.data(), sizeof(indices[0]) * indices.size());

			mesh.vertices_size = vertices.size();
			mesh.indices_size = indices.size();

			mesh_e.set(create_mesh3d_material(world));

			entity.is_a(mesh_e);
		});

	world.observer<Mesh3d, RenderDevice>()
		.term_at(1).src<RenderDevice>()
		.event(flecs::OnSet)
		.each([](flecs::entity entity, Mesh3d& mesh, RenderDevice& device) {
			auto& mesh_gpu = entity.ensure<Mesh3dGpu>();

			if (mesh_gpu.vertex_buffer) {
				SDL_ReleaseGPUBuffer(device.gpu, mesh_gpu.vertex_buffer);

				mesh_gpu.vertex_buffer = nullptr;
			}

			if (mesh_gpu.index_buffer) {
				SDL_ReleaseGPUBuffer(device.gpu, mesh_gpu.index_buffer);

				mesh_gpu.index_buffer = nullptr;
			}

			if (mesh.vertices_size == 0 || mesh.indices_size == 0) {
				return;
			}

			SDL_GPUBufferCreateInfo vertex_buffer_create_info{
				.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
				.size = static_cast<Uint32>(sizeof(Vertex3d) * mesh.vertices_size),
			};

			SDL_GPUBufferCreateInfo index_buffer_create_info{
				.usage = SDL_GPU_BUFFERUSAGE_INDEX,
				.size = static_cast<Uint32>(sizeof(uint32_t) * mesh.indices_size),
			};

			SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = static_cast<Uint32>((sizeof(Vertex3d) * mesh.vertices_size) + (sizeof(uint32_t) * mesh.indices_size)),
			};

			mesh_gpu.vertex_buffer = SDL_CreateGPUBuffer(device.gpu, &vertex_buffer_create_info);
			mesh_gpu.index_buffer  = SDL_CreateGPUBuffer(device.gpu, &index_buffer_create_info);

			auto transfer_buffer = SDL_CreateGPUTransferBuffer(device.gpu, &transfer_buffer_create_info);
			auto copy_cmd_buf = SDL_AcquireGPUCommandBuffer(device.gpu);
			auto copy_pass = SDL_BeginGPUCopyPass(copy_cmd_buf);
			auto transfer_mem = SDL_MapGPUTransferBuffer(device.gpu, transfer_buffer, false);

			auto vertices = static_cast<Vertex3d*>(transfer_mem);
			auto indices = reinterpret_cast<int*>(vertices + mesh.vertices_size);

			std::memcpy(vertices, mesh.vertices, sizeof(Vertex3d) * mesh.vertices_size);
			std::memcpy(indices, mesh.indices, sizeof(uint32_t) * mesh.indices_size);

			SDL_UnmapGPUTransferBuffer(device.gpu, transfer_buffer);

			SDL_GPUTransferBufferLocation vertex_transfer_buffer_location{
				.transfer_buffer = transfer_buffer,
				.offset = 0 ,
			};
			SDL_GPUBufferRegion vertex_buffer_location{
				.buffer = mesh_gpu.vertex_buffer,
				.offset = 0,
				.size = static_cast<Uint32>(sizeof(Vertex3d) * mesh.vertices_size),
			};

			SDL_UploadToGPUBuffer(copy_pass, &vertex_transfer_buffer_location, &vertex_buffer_location, false);

			SDL_GPUTransferBufferLocation index_transfer_buffer_location {
				.transfer_buffer = transfer_buffer,
				.offset = static_cast<Uint32>(sizeof(Vertex3d) * mesh.vertices_size),
			};
			SDL_GPUBufferRegion index_buffer_region {
				.buffer = mesh_gpu.index_buffer,
				.offset = 0,
				.size = static_cast<Uint32>(sizeof(uint32_t) * mesh.indices_size),
			};

			SDL_UploadToGPUBuffer(copy_pass, &index_transfer_buffer_location, &index_buffer_region, false);

			SDL_EndGPUCopyPass(copy_pass);
			assert(SDL_SubmitGPUCommandBuffer(copy_cmd_buf) && SDL_GetError());
		});

	//world.system<Transform, Mesh3d>()
	//	.group_by<Mesh3dGpu>(group_by_relation)
	//	.kind(Phases::Update)
	//	.run([](flecs::iter& it) {
	//		while (it.next()) {
	//			auto group_id = it.group_id();

	//			for (auto i : it) {
	//				auto entity = it.entity(i);

	//				//spdlog::info("entity {}({}) in group_id: {}", entity.name(), entity.id(), group_id);
	//			}
	//		}
	//	});

	world.system<Mesh3dDescription, RenderDevice, AssetStorage, Mesh3dAllocator>("mesh loader")
		.without<Mesh3d>()
		.term_at(1).src<RenderDevice>()
		.kind(Phases::Update)
		.each([&world] (flecs::entity entity, Mesh3dDescription& descriotion, RenderDevice& device, AssetStorage& storage, Mesh3dAllocator& allocator) {
			const auto model_string = descriotion.model_path + std::to_string(static_cast<uint8_t>(descriotion.optimization_flags));

			if (allocator.cache.contains(model_string)) {
				const auto& cached_data = allocator.cache.at(model_string);

				//entity
				//	.set(create_mesh3d_material(world))
				//	.set<Mesh3d>({
				//		.texture = storage.load_texture(*device.gpu, descriotion.texture_path),
				//		.vertex_offset = cached_data.vertex_offset,
				//		.vertex_count = cached_data.vertex_count,
				//		.index_offset = cached_data.index_offset,
				//		.index_count = cached_data.index_count,
				//	});

				return;
			}

			std::vector<Vertex3d> vertices;
			std::vector<std::uint32_t> indices;

			switch (descriotion.model_format) {
			case se::Mesh3dDescription::Format::obj: {
				tinyobj::attrib_t attrib;
				std::vector<tinyobj::shape_t> shapes;
				std::vector<tinyobj::material_t> materials;

				tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, descriotion.model_path.c_str());

				for (const auto& shape : shapes) {
					size_t index_offset = 0;

					for (const auto num_face_vertices : shape.mesh.num_face_vertices) {
						for (size_t fv = 0; fv  < num_face_vertices; ++fv) {
							const auto idx = shape.mesh.indices[fv + index_offset];

							Vertex3d vertex;

							vertex.position.x = attrib.vertices[3 * idx.vertex_index + 0];
							vertex.position.y = attrib.vertices[3 * idx.vertex_index + 1];
							vertex.position.z = attrib.vertices[3 * idx.vertex_index + 2];

							if (idx.texcoord_index > 0) {
								vertex.uv.x = attrib.texcoords[2 * idx.texcoord_index + 0];
								vertex.uv.y = 1.f - attrib.texcoords[2 * idx.texcoord_index + 1];
							}

							indices.push_back(vertices.size());
							vertices.push_back(vertex);
						}

						index_offset += num_face_vertices;
					}
				}
				break;
			}
			default:
				break;
			}

			if (!static_cast<bool>(descriotion.optimization_flags & Mesh3dDescription::OptimizationFlags::None)) {
				const auto optimaze_indexing            = static_cast<bool>(descriotion.optimization_flags & Mesh3dDescription::OptimizationFlags::Indexing);
				const auto optimaze_vetex_cache         = static_cast<bool>(descriotion.optimization_flags & Mesh3dDescription::OptimizationFlags::VertexCache);
				const auto optimaze_overdraw            = static_cast<bool>(descriotion.optimization_flags & Mesh3dDescription::OptimizationFlags::Overdraw);
				const auto optimaze_vertex_fetch        = static_cast<bool>(descriotion.optimization_flags & Mesh3dDescription::OptimizationFlags::VertexFetch);
				const auto optimaze_vertex_quantization = static_cast<bool>(descriotion.optimization_flags & Mesh3dDescription::OptimizationFlags::VertexQuantization);
				const auto optimaze_filtering           = static_cast<bool>(descriotion.optimization_flags & Mesh3dDescription::OptimizationFlags::IndexFiltering);
				const auto optimaze_shadow_indexing     = static_cast<bool>(descriotion.optimization_flags & Mesh3dDescription::OptimizationFlags::ShadowIndexing);


				if (optimaze_indexing) {
					std::vector<std::uint32_t> remap(vertices.size());

					const auto vertex_count = meshopt_generateVertexRemap(remap.data(), indices.data(), indices.size(), vertices.data(), vertices.size(), sizeof(Vertex3d));

					std::vector<Vertex3d> remapped_vertices(vertex_count);

					meshopt_remapIndexBuffer(indices.data(), indices.data(), indices.size(), remap.data());
					meshopt_remapVertexBuffer(remapped_vertices.data(), vertices.data(), vertices.size(), sizeof(Vertex3d), remap.data());

					vertices = std::move(remapped_vertices);
				}

				if (optimaze_vetex_cache) {
					meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), vertices.size());
				}

				if (optimaze_overdraw) {
					meshopt_optimizeOverdraw(indices.data(), indices.data(), indices.size(), &vertices[0].position.x, vertices.size(), sizeof(Vertex3d), 1.05f);
				}

				if (optimaze_vertex_fetch) {
					std::vector<Vertex3d> optimized_vertices(vertices.size());

					meshopt_optimizeVertexFetch(optimized_vertices.data(), indices.data(), indices.size(), vertices.data(), vertices.size(), sizeof(Vertex3d));

					vertices = std::move(optimized_vertices);
				}

				if (optimaze_vertex_quantization) {
					// TODO
				}

				if (optimaze_filtering) {
					indices.resize(meshopt_filterIndexBuffer(indices.data(), indices.data(), indices.size(), &vertices[0].position.x, vertices.size(), sizeof(float) * 3, sizeof(Vertex3d)));
				}

				if (optimaze_shadow_indexing) {
					std::vector<std::uint32_t> shadow_indices(indices.size());

					meshopt_generateShadowIndexBuffer(shadow_indices.data(), indices.data(), indices.size(), &vertices[0].position.x, vertices.size(), sizeof(float) * 3, sizeof(Vertex3d));
					meshopt_optimizeVertexCache(shadow_indices.data(), shadow_indices.data(), indices.size(), vertices.size());
				}
			}

			//const auto [id, pos] = request_mesh3d_allocate(allocator, std::move(vertices), std::move(indices));

			//entity
			//	.set(create_mesh3d_material(world))
			//	.set<Mesh3d>({
			//		.texture = storage.load_texture(*device.gpu, descriotion.texture_path),
			//		.vertex_offset = allocator.vertex_offset,
			//		.vertex_count = vertices.size(),
			//		.index_offset = allocator.index_offset,
			//		.index_count = indices.size(),
			//	});

			allocator.allocate_requests.push_back({
				.vertices = std::move(vertices),
				.indices = std::move(indices),
			});
		});

	world.add<Mesh3dAllocator>();
}

se::Material se::create_mesh3d_material(flecs::world& world) {
	const auto window = world.get<WindowModule>().main_window;
	auto& device = world.get<RenderDevice>();

//	auto vert_shader = load_shader(*device.gpu, "assets/shaders/out/mesh.vert.msl", 2);
//	auto frag_shader = load_shader(*device.gpu, "assets/shaders/out/mesh.frag.msl", 0, 1);
	auto vert_shader = load_shader(*device.gpu, "assets/shaders/out/mesh3d_pull.vert.msl", 1, 0, 1);
	auto frag_shader = load_shader(*device.gpu, "assets/shaders/out/mesh3d_pull.frag.msl", 0, 1);

	SDL_GPUColorTargetDescription color_target_description{
		.format = SDL_GetGPUSwapchainTextureFormat(device.gpu, window),
		.blend_state = {
			.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
			.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
			.color_blend_op = SDL_GPU_BLENDOP_ADD,
			.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
			.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
			.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
			.enable_blend = true,
		}
	};

	std::array<SDL_GPUVertexAttribute, 4> vertex_attrs{
		SDL_GPUVertexAttribute{
			.location = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			.offset = offsetof(Vertex3d, position),
		},
		SDL_GPUVertexAttribute{
			.location = 1,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(Vertex3d, color),
		},
		SDL_GPUVertexAttribute{
			.location = 2,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = offsetof(Vertex3d, uv),
		},
		SDL_GPUVertexAttribute{
			.location = 3,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			.offset = offsetof(Vertex3d, normal),
		},
	};

	SDL_GPUVertexBufferDescription vertex_buffer_description{
		.slot = 0,
		.pitch = sizeof(Vertex3d),
		.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
	};

	SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info{
		.vertex_shader = vert_shader,
		.fragment_shader = frag_shader,
		.vertex_input_state = {
			.vertex_buffer_descriptions = &vertex_buffer_description,
			.num_vertex_buffers = 1,
			.vertex_attributes = vertex_attrs.data(),
			.num_vertex_attributes = vertex_attrs.size(),
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.rasterizer_state = {
			//.fill_mode = SDL_GPU_FILLMODE_LINE,
			.cull_mode = SDL_GPU_CULLMODE_BACK,
		},
		.depth_stencil_state = {
			.compare_op = SDL_GPU_COMPAREOP_LESS,
			.enable_depth_test = true,
			.enable_depth_write = true,
		},
		.target_info = {
			.color_target_descriptions = &color_target_description,
			.num_color_targets = 1,
			.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
			.has_depth_stencil_target = true,
		},
	};

	SDL_GPUSamplerCreateInfo sampler_create_info{};

	auto sampler = SDL_CreateGPUSampler(device.gpu, &sampler_create_info);
	auto pipeline = SDL_CreateGPUGraphicsPipeline(device.gpu, &pipeline_create_info);

	SDL_ReleaseGPUShader(device.gpu, vert_shader);
	SDL_ReleaseGPUShader(device.gpu, frag_shader);

	return {
		.pipeline = pipeline,
		.sampler = sampler,
		.vertex_uniforms = { world.id<DefaultUniform>() },
		.fragment_uniforms = { world.id<DefaultUniform>() },
	};
}

se::PhaseContext se::create_mesh3d_context(flecs::world& world) {
	const auto& device = world.get<RenderDevice>();

	SDL_GPUBufferCreateInfo vertex_buffer_create_info{
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = sizeof(Vertex3d) * MESH3D_MAX_VERTEX_COUNT,
	};

	SDL_GPUBufferCreateInfo index_buffer_create_info{
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
		.size = sizeof(int) * MESH3D_MAX_INDEX_COUNT,
	};

	SDL_GPUBufferCreateInfo storage_buffer_create_info{
		.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
		.size = static_cast<Uint32>(sizeof(Mesh3dInstance) * MESH3D_MAX_INSTANCE_COUNT),
	};

	SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = static_cast<Uint32>((sizeof(Vertex3d) * MESH3D_MAX_VERTEX_COUNT) + (sizeof(int) * MESH3D_MAX_INDEX_COUNT) + (sizeof(Mesh3dInstance) * MESH3D_MAX_INSTANCE_COUNT))
	};

	auto vertex_buffer = SDL_CreateGPUBuffer(device.gpu, &vertex_buffer_create_info);
	auto index_buffer = SDL_CreateGPUBuffer(device.gpu, &index_buffer_create_info);
	auto storage_buffer = SDL_CreateGPUBuffer(device.gpu, &storage_buffer_create_info);
	auto transfer_buffer = SDL_CreateGPUTransferBuffer(device.gpu, &transfer_buffer_create_info);

	return {
		.index_buffer = index_buffer,
		.vertex_buffer = vertex_buffer,
		.storage_buffer = storage_buffer,
		.transfer_buffer = transfer_buffer,
		.index_element_size = SDL_GPU_INDEXELEMENTSIZE_32BIT,
	};

}

se::RenderPhaseExtractor se::create_mesh3d_extractor(flecs::world& world, flecs::entity_t helper) {
	world.readonly_end();

	auto mesh_query = world.query_builder()
		.with<const Mesh3d>()
		.with<const Mesh3dGpu>()
		.with<const GlobalTransform>()
		.with<const Material>()
		.group_by<Mesh3dGpu>(group_by_relation)
		.with<ExtractedMeshes3d>().src("$helper").inout()
		.with<RenderPhase>().src("$phase_entity").inout()
		.with<Aabb>().src("$camera").inout()
		.build();

	world.readonly_begin();

	return {
		.callback = [](flecs::iter& it) {
			auto helper = it.get_var("helper");

			while (it.next()) {
				auto group_id = it.group_id();

				auto mesh_field = it.field<const Mesh3d>(0);
				auto mesh_gpu_field = it.field<const Mesh3dGpu>(1);
				auto transform_field = it.field<const GlobalTransform>(2);
				auto material_field = it.field<const Material>(3);

				auto& extracted_meshes = it.field<ExtractedMeshes3d>(4)[0];
				auto& render_phase = it.field<RenderPhase>(5)[0];
				//auto& camera_aabb = it.field<Aabb>(11)[0];

				for (auto i : it) {
					//if (!camera_aabb.is_intersect(aabb_field[i])) {
					//	continue;
					//}

					const auto entity = it.entity(i);

					const auto& mesh = mesh_field[0];
					const auto& mesh_gpu = mesh_gpu_field[0];
					const auto& transform = transform_field[i];
					const auto& material = material_field[0];

					render_phase.items.push_back({
						.entity = entity,
						.context_entity = helper,
						.material_id = entity,
						.extracted_index = extracted_meshes.size(),
						.sort_value = static_cast<float>(group_id),
						.num_indices = static_cast<uint32_t>(mesh.indices_size),
					});

					extracted_meshes.push_back({
						.entity = entity.id(),
						.material_id = entity.id(),
						.group_id = group_id,
						.vertices = mesh.vertices,
						.indices = mesh.indices,
						.vertices_size = mesh.vertices_size,
						.indices_size = mesh.indices_size,
						.texture = mesh.texture ? &mesh.texture->get_gpu_texture() : nullptr,
						.transform = transform.matrix,
						.color = mesh.color
					});
				}
			}
		},
		.query = mesh_query,
		.helper = helper,
	};
}

se::RenderPhaseUploader se::create_mesh3d_uploader() {
	return {
		.callback = [](flecs::world& world, flecs::entity& render_phase, flecs::entity& uploader, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass) {
			auto& uploaded_meshes = uploader.get_mut<UploadedMeshes3d>();
			auto& extracted_meshes = uploader.get_mut<ExtractedMeshes3d>();
			auto& context = uploader.get_mut<PhaseContext>();
			auto& phase_items = render_phase.get_mut<RenderPhase>().items;

			auto transfer_buffer = SDL_MapGPUTransferBuffer(device, context.transfer_buffer, false);

			auto vertices = static_cast<Vertex3d*>(transfer_buffer);
			auto indices = reinterpret_cast<int*>(vertices + MESH3D_MAX_VERTEX_COUNT);
			auto instances = reinterpret_cast<Mesh3dInstance*>(indices + MESH3D_MAX_INDEX_COUNT);

			uint32_t vertex_count = 0;
			uint32_t index_count  = 0;
			uint32_t instances_index = 0;

			uint32_t upload_vertex_count = 0;
			uint32_t upload_index_count  = 0;

			size_t current_batch_index = 0;

			for (size_t i = 0; i < phase_items.size(); ++i) {
				auto& item = phase_items[i];

				if (item.extracted_index >= extracted_meshes.size() || extracted_meshes[item.extracted_index].entity != item.entity) {
					current_batch_index = i + 1;
					continue;
				}

				const auto& mesh = extracted_meshes[item.extracted_index];

				if (i == current_batch_index || phase_items[current_batch_index].texture != mesh.texture || phase_items[current_batch_index].sort_value != static_cast<float>(mesh.group_id)) {
					current_batch_index = i;

					phase_items[current_batch_index].texture = mesh.texture;
					phase_items[current_batch_index].first_index = index_count;
					phase_items[current_batch_index].num_indices = mesh.indices_size;
					phase_items[current_batch_index].vertex_offset = index_count;
					// NOTE: first_instance supports only on Metal and Vulkan
					phase_items[current_batch_index].first_instance = instances_index;

					vertex_count += mesh.vertices_size;
					index_count += mesh.indices_size;
				}

				if (!uploaded_meshes.contains(mesh.group_id)) {
					std::memcpy(vertices + upload_vertex_count, mesh.vertices, sizeof(mesh.vertices[0]) * mesh.vertices_size);
					std::memcpy(indices + upload_index_count, mesh.indices, sizeof(mesh.indices[0]) * mesh.indices_size);

					//for (size_t i = 0; i < mesh.indices_size; ++i) {
					//	indices[upload_index_count + i] = context.num_indices + upload_index_count + mesh.indices[i];
					//}

					upload_vertex_count += mesh.vertices_size;
					upload_index_count += mesh.indices_size;

					uploaded_meshes.emplace(mesh.group_id);
				}

				phase_items[current_batch_index].batch_size += 1;
				phase_items[current_batch_index].num_instances += 1;

				instances[instances_index].flags |= static_cast<uint32_t>((bool)mesh.texture ? Mesh3dShaderFlags::Textured : Mesh3dShaderFlags::None);
				instances[instances_index].model = mesh.transform;

				++instances_index;
			}

			extracted_meshes.clear();

			SDL_UnmapGPUTransferBuffer(device, context.transfer_buffer);

			if (instances_index == 0) {
				return;
			}

			SDL_GPUTransferBufferLocation instances_transfer_buffer_location {
				.transfer_buffer = context.transfer_buffer,
				.offset = static_cast<Uint32>(sizeof(Vertex3d) * MESH3D_MAX_VERTEX_COUNT + sizeof(int) * MESH3D_MAX_INDEX_COUNT)
			};
			SDL_GPUBufferRegion instances_buffer_region {
				.buffer = context.storage_buffer,
				.offset = 0,
				.size = static_cast<Uint32>(sizeof(Mesh3dInstance) * instances_index)
			};

			SDL_UploadToGPUBuffer(copy_pass, &instances_transfer_buffer_location, &instances_buffer_region, false);

			if (upload_vertex_count == 0 || upload_index_count == 0) {
				return;
			}

			SDL_GPUTransferBufferLocation vertex_transfer_buffer_location{
				.transfer_buffer = context.transfer_buffer,
				.offset = 0 
			};
			SDL_GPUBufferRegion vertex_buffer_location{
				.buffer = context.vertex_buffer,
				.offset = static_cast<Uint32>(context.num_vertices),
				.size = static_cast<Uint32>(sizeof(Vertex3d) * upload_vertex_count)
			};

			SDL_UploadToGPUBuffer(copy_pass, &vertex_transfer_buffer_location, &vertex_buffer_location, false);

			SDL_GPUTransferBufferLocation index_transfer_buffer_location {
				.transfer_buffer = context.transfer_buffer,
				.offset = static_cast<Uint32>(sizeof(Vertex3d) * MESH3D_MAX_VERTEX_COUNT)
			};
			SDL_GPUBufferRegion index_buffer_region {
				.buffer = context.index_buffer,
				.offset = static_cast<Uint32>(context.num_indices),
				.size = static_cast<Uint32>(sizeof(int) * upload_index_count)
			};

			SDL_UploadToGPUBuffer(copy_pass, &index_transfer_buffer_location, &index_buffer_region, false);

			context.num_vertices += upload_vertex_count;
			context.num_indices += upload_index_count;
			//context.num_instances = instances_index;
		}
	};
}

glm::mat4 se::extract_3d_view(const Camera& camera, const GlobalTransform& transform) {
	auto proj = glm::perspective(glm::radians(60.f), camera.viewport.x / camera.viewport.y, 1.f, 1000.f);
	auto view = glm::translate(glm::mat4(1.f), -transform.translation);
	return proj * view;
}

//std::pair<uint64_t, uint64_t> se::request_mesh3d_allocate(Mesh3dAllocator& allocator, std::vector<Vertex3d> vertices, std::vector<uint32_t> indices) {
//	auto id  = allocator.allocate_requests.size();
//	auto pos = allocator.last_allocate_pos;
//
//	allocator.last_allocate_pos += vertices.size();
//	allocator.allocate_requests.emplace_back(std::move(vertices), std::move(indices));
//
//	return { id, pos };
//}
