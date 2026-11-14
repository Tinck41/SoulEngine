#include "node_helpers.h"

#include "components.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/render_module/module.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/ui_render_module_new/offsets.h"
#include "ecsModule/windowModule/module.h"
#include "ecsModule/ui_module/module.h"
#include "utils/sdl.h"

using namespace se;

#define NODE_MAX_ 10'000 // ????

Material se::create_node_material_new(flecs::world& world) {
	auto& device = world.get<RenderDevice>();

	auto vert_shader = load_shader(*device.gpu, "assets/shaders/out/ui_2.vert.msl", 1, 0, 1);
	auto frag_shader = load_shader(*device.gpu, "assets/shaders/out/ui_2.frag.msl", 0, 1);

	SDL_GPUColorTargetDescription color_target_description{
		.format = SDL_GetGPUSwapchainTextureFormat(device.gpu, world.get<WindowModule>().main_window),
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

	std::array<SDL_GPUVertexAttribute, 12> vertex_attrs{
		SDL_GPUVertexAttribute{
			.location = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			.offset = offsetof(UiVertexNew, position),
		},
		SDL_GPUVertexAttribute{
			.location = 1,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
			.offset = offsetof(UiVertexNew, rotation),
		},
		SDL_GPUVertexAttribute{
			.location = 2,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = offsetof(UiVertexNew, scale),
		},
		SDL_GPUVertexAttribute{
			.location = 3,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = offsetof(UiVertexNew, padding_1),
		},
		SDL_GPUVertexAttribute{
			.location = 4,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = offsetof(UiVertexNew, tex_pos),
		},
		SDL_GPUVertexAttribute{
			.location = 5,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = offsetof(UiVertexNew, tex_size),
		},
		SDL_GPUVertexAttribute{
			.location = 6,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(UiVertexNew, color),
		},
		SDL_GPUVertexAttribute{
			.location = 7,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(UiVertexNew, border_color),
		},
		SDL_GPUVertexAttribute{
			.location = 8,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
			.offset = offsetof(UiVertexNew, border_width),
		},
		SDL_GPUVertexAttribute{
			.location = 9,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
			.offset = offsetof(UiVertexNew, border_radius),
		},
		SDL_GPUVertexAttribute{
			.location = 10,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = offsetof(UiVertexNew, padding_2),
		},
		SDL_GPUVertexAttribute{
			.location = 11,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_UINT4,
			.offset = offsetof(UiVertexNew, flags),
		},
	};

	SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info{
		.vertex_shader = vert_shader,
		.fragment_shader = frag_shader,
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.target_info = {
			.color_target_descriptions = &color_target_description,
			.num_color_targets = 1,
		}
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
	};
}

Material se::create_node_material(flecs::world& world) {
	auto& device = world.get<RenderDevice>();

	auto vert_shader = load_shader(*device.gpu, "assets/shaders/out/ui.vert.msl", 1);
	auto frag_shader = load_shader(*device.gpu, "assets/shaders/out/ui.frag.msl", 0, 1);

	SDL_GPUColorTargetDescription color_target_description{
		.format = SDL_GetGPUSwapchainTextureFormat(device.gpu, world.get<WindowModule>().main_window),
		.blend_state = {
			.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
			.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
			.color_blend_op = SDL_GPU_BLENDOP_ADD,
			.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
			.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
			.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
			.enable_blend = true,
		}
	};

	std::array<SDL_GPUVertexAttribute, 9> vertex_attrs{
		SDL_GPUVertexAttribute{
			.location = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			.offset = offsetof(UiVertex, position),
		},
		SDL_GPUVertexAttribute{
			.location = 1,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = offsetof(UiVertex, uv),
		},
		SDL_GPUVertexAttribute{
			.location = 2,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(UiVertex, color),
		},
		SDL_GPUVertexAttribute{
			.location = 3,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_UINT4,
			.offset = offsetof(UiVertex, flags),
		},
		SDL_GPUVertexAttribute{
			.location = 4,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = offsetof(UiVertex, size),
		},
		SDL_GPUVertexAttribute{
			.location = 5,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
			.offset = offsetof(UiVertex, border_radius),
		},
		SDL_GPUVertexAttribute{
			.location = 6,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(UiVertex, border_color),
		},
		SDL_GPUVertexAttribute{
			.location = 7,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
			.offset = offsetof(UiVertex, border_width),
		},
		SDL_GPUVertexAttribute{
			.location = 8,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = offsetof(UiVertex, local_pos),
		},
	};

	SDL_GPUVertexBufferDescription vertex_buffer_description{
		.slot = 0,
		.pitch = sizeof(UiVertex),
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
		.target_info = {
			.color_target_descriptions = &color_target_description,
			.num_color_targets = 1,
		}
	};

	SDL_GPUTextureCreateInfo texture_create_info{
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
		.width = 1,
		.height = 1,
		.layer_count_or_depth = 1,
		.num_levels = 1,
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
	};
}

PhaseContext se::create_node_context_new(flecs::world& world) {
	auto& device = world.get<RenderDevice>();

	const auto verticies_byte_szie = NODE_MAX_ * sizeof(UiVertexNew);

	SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = verticies_byte_szie,
	};

	auto transfer_buffer = SDL_CreateGPUTransferBuffer(device.gpu, &transfer_buffer_create_info);

	SDL_GPUBufferCreateInfo vertex_buffer_create_info{
		.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
		.size = verticies_byte_szie,
	};

	auto vertex_buffer = SDL_CreateGPUBuffer(device.gpu, &vertex_buffer_create_info);

	return {
		.storage_buffer = vertex_buffer,
		.transfer_buffer = transfer_buffer,
		.index_element_size = SDL_GPU_INDEXELEMENTSIZE_16BIT,
	};
}

PhaseContext se::create_node_context(flecs::world& world) {
	auto& device = world.get<RenderDevice>();

	std::vector<uint16_t> indices;

	indices.reserve(NODE_MAX_ * 6);

	for (uint32_t i = 0; i < NODE_MAX_; ++i) {
		uint16_t base = i * 4;

		indices.push_back(base + 0);
		indices.push_back(base + 1);
		indices.push_back(base + 2);
		indices.push_back(base + 2);
		indices.push_back(base + 3);
		indices.push_back(base + 0);
	}

	SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = NODE_MAX_ * 4 * sizeof(UiVertex),
	};

	auto transfer_buffer = SDL_CreateGPUTransferBuffer(device.gpu, &transfer_buffer_create_info);

	const auto verticies_byte_szie = NODE_MAX_ * 4 * sizeof(UiVertex);
	const auto indecies_byte_szie = indices.size() * sizeof(indices[0]);

	SDL_GPUBufferCreateInfo vertex_buffer_create_info{
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = verticies_byte_szie,
	};
	SDL_GPUBufferCreateInfo index_buffer_create_info{
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
		.size = static_cast<Uint32>(indecies_byte_szie),
	};

	auto vertex_buffer = SDL_CreateGPUBuffer(device.gpu, &vertex_buffer_create_info);
	auto index_buffer = SDL_CreateGPUBuffer(device.gpu, &index_buffer_create_info);

	{
		SDL_GPUTransferBufferCreateInfo indecies_transfer_buffer_create_info{
			.usage = SDL_GPUTransferBufferUsage::SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = static_cast<uint32_t>(indecies_byte_szie)
		};

		auto transfer_buf = SDL_CreateGPUTransferBuffer(device.gpu, &indecies_transfer_buffer_create_info);

		auto transfer_mem = SDL_MapGPUTransferBuffer(device.gpu ,transfer_buf, false);

		std::memcpy(transfer_mem, indices.data(), indecies_byte_szie);

		SDL_UnmapGPUTransferBuffer(device.gpu, transfer_buf);

		auto copy_cmd_buf = SDL_AcquireGPUCommandBuffer(device.gpu);

		auto copy_pass = SDL_BeginGPUCopyPass(copy_cmd_buf);

		SDL_GPUTransferBufferLocation index_buffer_location{ .transfer_buffer = transfer_buf };

		SDL_GPUBufferRegion index_buffer_region{ .buffer = index_buffer, .size = static_cast<uint32_t>(indecies_byte_szie) };

		SDL_UploadToGPUBuffer(copy_pass, &index_buffer_location, &index_buffer_region, false);

		SDL_EndGPUCopyPass(copy_pass);

		assert(SDL_SubmitGPUCommandBuffer(copy_cmd_buf) && SDL_GetError());

		SDL_ReleaseGPUTransferBuffer(device.gpu, transfer_buf);
	}

	return {
		.index_buffer = index_buffer,
		.vertex_buffer = vertex_buffer,
		.transfer_buffer = transfer_buffer,
		.index_element_size = SDL_GPU_INDEXELEMENTSIZE_16BIT,
	};
}

RenderPhaseExtractor se::create_image_node_extractor(flecs::world& world, flecs::entity_t helper) {
	auto image_query = world.query_builder()
		.with<Node>()
		.with<NodeIndex>()
		.with<Image>()
		.with<GlobalTransform>()
		.with<Aabb>()
		.with<Material>().optional()
		.with<ClipContent>().optional()
		.with<ExtractedNodes>().src("$helper").inout()
		.with<RenderPhase>().src("$phase_entity").inout()
		.with<Aabb>().src("$camera").inout()
		.build();

	return RenderPhaseExtractor{
		.callback = [](flecs::iter& it) {
			auto world = it.world();
			auto helper = it.get_var("helper");

			while(it.next()) {
				auto node_field = it.field<Node>(0);
				auto stack_index_field = it.field<NodeIndex>(1);
				auto image_field = it.field<Image>(2);
				auto transform_field = it.field<GlobalTransform>(3);
				auto aabb_field = it.field<Aabb>(4);

				auto& extracted_nodes = it.field<ExtractedNodes>(7)[0];
				auto& render_phase = it.field<RenderPhase>(8)[0];
				auto& camera_aabb = it.field<Aabb>(9)[0];

				for (auto i : it) {
					if (!camera_aabb.is_intersect(aabb_field[i])) {
						continue;
					}

					const auto entity = it.entity(i);

					const auto& node = node_field[i];
					const auto& stack_index = stack_index_field[i].dfs;
					const auto& image = image_field[i];
					const auto& transform = transform_field[i];

					const auto material_id = it.is_set(5) ? entity : helper;

					render_phase.items.emplace_back(entity, helper, material_id, extracted_nodes.size(), stack_index + node_offsets::IMAGE, it.is_set(6) ? it.field<ClipContent>(6)[i] : std::optional<RectI>{});

					const auto [uv, size] = [&] {
						if (image.texture_atlas) {
							const auto& atlas = image.texture_atlas.value();
							const auto& rect = atlas.rects[atlas.current_index];

							return std::pair{ glm::vec2{ rect.x, rect.y }, glm::vec2{ rect.w, rect.h }};
						}

						return std::pair{ glm::vec2{ 0.f, 0.f }, image.texture->get_size() };
					}();

					extracted_nodes.emplace_back(
						entity,
						material_id,
						&image.texture->get_gpu_texture(),
						image.texture->get_size(),
						transform.translation,
						transform.rotation,
						transform.scale,
						image.color,
						size,
						uv
					);

				}
			}
		},
		.query = image_query,
		.helper = helper,
	};
}

RenderPhaseExtractor se::create_color_node_extractor(flecs::world& world, flecs::entity_t helper) {
	auto color_query = world.query_builder()
		.with<Node>()
		.with<NodeIndex>()
		.with<BorderColor>().optional()
		.with<BackgroundColor>()
		.with<GlobalTransform>()
		.with<Aabb>()
		.with<Material>().optional()
		.with<ClipContent>().optional()
		.with<ExtractedNodes>().src("$helper").inout()
		.with<RenderPhase>().src("$phase_entity").inout()
		.with<Aabb>().src("$camera").inout()
		.build();

	return RenderPhaseExtractor{
		.callback = [](flecs::iter& it) {
			auto world = it.world();
			auto helper = it.get_var("helper");

			while(it.next()) {
				auto node_field = it.field<Node>(0);
				auto index_field = it.field<NodeIndex>(1);
				auto color_field = it.field<BackgroundColor>(3);
				auto transform_field = it.field<GlobalTransform>(4);
				auto aabb_field = it.field<Aabb>(5);

				auto& extracted_nodes = it.field<ExtractedNodes>(8)[0];
				auto& render_phase = it.field<RenderPhase>(9)[0];
				auto& camera_aabb = it.field<Aabb>(10)[0];

				for (auto i : it) {
					if (!camera_aabb.is_intersect(aabb_field[i])) {
						continue;
					}

					const auto entity = it.entity(i);
					const auto& node = node_field[i];
					const auto& index = index_field[i];
					const auto& color = color_field[i];
					const auto& transform = transform_field[i];

					if (node.size.x == 0.f || node.size.y == 0.f) {
						continue;
					}

					const auto material_id = it.is_set(6) ? entity : helper;

					render_phase.items.emplace_back(entity, helper, material_id, extracted_nodes.size(), index.dfs + node_offsets::BACKGROUND_COLOR, it.is_set(7) ? it.field<ClipContent>(7)[i] : std::optional<RectI>{});

					extracted_nodes.push_back(ExtractedNode{
						.entity = entity,
						.material_id = material_id,
						.texture = nullptr,
						.texture_size = glm::vec2{},
						.translation = transform.translation,
						.rotation = transform.rotation,
						.scale = transform.scale,
						.color = color,
						.size = node.size,
						.uv = glm::vec2{ 0.f, 0.f },
						.border_color = it.is_set(2) ? it.field<BorderColor>(2)[i] : glm::vec4{},
						.border_radius = node.border_radius,
						.border_width = node.border_width,
						.dfs_index = index.dfs,
						.bfs_index = index.bfs,
						.scissor = it.is_set(7) ? it.field<ClipContent>(7)[i] : std::optional<RectI>{},
					});

				}
			}
		},
		.query = color_query,
		.helper = helper,
	};
}

RenderPhaseUploader se::create_node_uploader_new() {
	return RenderPhaseUploader{
		.callback = [](flecs::world& world, flecs::entity& render_phase, flecs::entity& uploader, SDL_GPUDevice* gpu, SDL_GPUCopyPass* copy_pass) {
			auto& extracted_nodes = uploader.get_mut<ExtractedNodes>();
			auto& context = uploader.get_mut<PhaseContext>();
			auto& phase_items = render_phase.get_mut<RenderPhase>().items;

			auto instances = static_cast<UiVertexNew*>(SDL_MapGPUTransferBuffer(gpu ,context.transfer_buffer, true));

			uint32_t current_instance = 0;

			size_t current_batch_index = 0;

			for (size_t i = 0; i < phase_items.size(); ++i) {
				const auto& data = phase_items[i];

				if (data.context_entity != uploader || data.extracted_index >= extracted_nodes.size() || extracted_nodes[data.extracted_index].entity != data.entity) {
					current_batch_index = i + 1;

					continue;
				}

				const auto& ui_node = extracted_nodes[data.extracted_index];

				if (phase_items[current_batch_index].num_instances == 0 || phase_items[current_batch_index].textures[0] != ui_node.texture || phase_items[current_batch_index].scissor != ui_node.scissor) {
					current_batch_index = i;

					phase_items[current_batch_index].textures.emplace_back(ui_node.texture);
					phase_items[current_batch_index].num_instances = 1;
					phase_items[current_batch_index].vertex_offset = current_instance * 6;
				}

				std::uint32_t flags = 0;

				if (ui_node.texture) {
					flags |= static_cast<std::uint32_t>(ShaderFlags::Textured);
				}

				instances[current_instance] = UiVertexNew{
					.position = glm::vec3(ui_node.translation, 0.f),
					.rotation = ui_node.rotation.x,
					.scale = ui_node.size, 
					.tex_pos = glm::vec2(0.f),
					.tex_size = glm::vec2(1.f),
					.color = ui_node.color,
					.border_color = ui_node.border_color,
					.border_width = ui_node.border_width,
					.border_radius = ui_node.border_radius,
					.flags = flags,
				};

				++current_instance;
				++phase_items[current_batch_index].batch_size;

				phase_items[current_batch_index].num_indices += 6;
			}

			extracted_nodes.clear();

			SDL_UnmapGPUTransferBuffer(gpu, context.transfer_buffer);

			if (current_instance == 0) {
				return;
			}

			const auto buffer_location = SDL_GPUTransferBufferLocation{
				.transfer_buffer = context.transfer_buffer,
				.offset = 0,
			};

			const auto buffer_region = SDL_GPUBufferRegion{
				.buffer = context.vert_storage_buffer,
				.offset = 0,
				.size = static_cast<Uint32>(current_instance * sizeof(UiVertexNew))
			};

			SDL_UploadToGPUBuffer(copy_pass,  &buffer_location, &buffer_region, true);
		},
	};
}

RenderPhaseUploader se::create_node_uploader() {
	return RenderPhaseUploader{
		.callback = [](flecs::world& world, flecs::entity& render_phase, flecs::entity& uploader, SDL_GPUDevice* gpu, SDL_GPUCopyPass* copy_pass) {
			auto& extracted_nodes = uploader.get_mut<ExtractedNodes>();
			auto& context = uploader.get_mut<PhaseContext>();
			auto& phase_items = render_phase.get_mut<RenderPhase>().items;

			auto vertices = static_cast<UiVertex*>(SDL_MapGPUTransferBuffer(gpu ,context.transfer_buffer, false));

			uint32_t current_vertex = 0;

			size_t current_batch_index = 0;

			for (size_t i = 0; i < phase_items.size(); ++i) {
				const auto& data = phase_items[i];

				if (data.context_entity != uploader || data.extracted_index >= extracted_nodes.size() || extracted_nodes[data.extracted_index].entity != data.entity) {
					current_batch_index = i + 1;

					continue;
				}

				const auto& ui_node = extracted_nodes[data.extracted_index];

				if ((phase_items[current_batch_index].textures.empty() && ui_node.texture) || (phase_items[current_batch_index].textures[0] && !ui_node.texture) || phase_items[current_batch_index].textures[0] != ui_node.texture) {
					current_batch_index = i;

					phase_items[current_batch_index].first_index = current_vertex / 4 * 6;
					phase_items[current_batch_index].textures.emplace_back(ui_node.texture);
				}

				const auto pos = ui_node.translation;
				const auto color = ui_node.color;
				const auto size = ui_node.size;

				std::uint32_t flags = 0;

				if (ui_node.texture) {
					flags |= static_cast<std::uint32_t>(ShaderFlags::Textured);
				}

				const auto center = pos + size * 0.5f;
				const auto local_pos  = pos - center;

				const glm::vec2 corners[4] = {
					{ 0.f,    size.y },
					{ size.x, size.y },
					{ size.x, 0.f    },
					{ 0.f,    0.f    },
				};

				const glm::vec2 uvs[4] = {
					{ 0.f, 1.f },
					{ 1.f, 1.f },
					{ 1.f, 0.f },
					{ 0.f, 0.f },
				};

				for (int i = 0; i < 4; i++) {
					const auto local = corners[i] - size * 0.5f;
					vertices[current_vertex + i] = {
						{ pos.x + corners[i].x, pos.y + corners[i].y, 0.f },
						uvs[i],
						color,
						flags,
						ui_node.size,
						ui_node.border_radius,
						ui_node.border_color,
						ui_node.border_width,
						local,
					};
				}

				current_vertex += 4;

				++phase_items[current_batch_index].batch_size;
				++phase_items[current_batch_index].num_instances;
				phase_items[current_batch_index].num_indices = 6 * phase_items[current_batch_index].batch_size;
			}

			extracted_nodes.clear();

			SDL_UnmapGPUTransferBuffer(gpu, context.transfer_buffer);

			if (current_vertex == 0) {
				return;
			}

			const auto buffer_location = SDL_GPUTransferBufferLocation{
				.transfer_buffer = context.transfer_buffer,
			};

			const auto buffer_region = SDL_GPUBufferRegion{
				.buffer = context.vertex_buffer,
				.size = static_cast<Uint32>(current_vertex * sizeof(UiVertex))
			};

			SDL_UploadToGPUBuffer(copy_pass,  &buffer_location, &buffer_region, false);
		},
	};
}
