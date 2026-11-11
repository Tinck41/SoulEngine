#include "module.h"

#include "ecsModule/common.h"
#include "SDL3_ttf/SDL_ttf.h"
//#include "ecsModule/renderModule/module.h"
#include "ecsModule/spriteModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/meshModule/module.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/windowModule/module.h"
#include "ext/matrix_transform.hpp"
#include "utils/sdl.h"
#include "font.h"

using namespace se;

#define TEXT_MAX_VERTEX_COUNT 4000
#define TEXT_MAX_INDEX_COUNT  6000

void draw_text(flecs::entity_t camera, flecs::entity_t entity, const flecs::world& world) {
	//const auto& batches = world.get<CameraTextBatches>();

	//if (!batches.contains(camera) || !batches.at(camera).contains(entity) || batches.at(camera).at(entity).empty()) {
	//	return;
	//}

	//const auto& pipeline = world.get<TextPipeline>();
	//const auto& storage = world.get<TextStorage>();
	//const auto& commands = world.get<RenderCommands>();

	//auto& batch_seq = batches.at(camera).at(entity);

	//SDL_GPUBufferBinding vertex_bindings{
	//	.buffer = storage.vertex_buffer,
	//};
	//SDL_GPUBufferBinding index_bindings{
	//	.buffer = storage.index_buffer,
	//};

	//const auto camera_entity = world.entity(camera);
	//const auto view = glm::translate(glm::mat4(1.f), -camera_entity.get<GlobalTransform>().translation);
	//const auto view_proj = camera_entity.get<Camera>().projection * view;

	//const auto& render_pass = camera_entity.get<RenderPass>().render_pass;

	//SDL_BindGPUGraphicsPipeline(render_pass, pipeline.pipeline);
	//SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_bindings, 1);
	//SDL_BindGPUIndexBuffer(render_pass, &index_bindings, SDL_GPU_INDEXELEMENTSIZE_32BIT);
	//SDL_PushGPUVertexUniformData(commands.cmd_buffer, 0, &view_proj, sizeof(glm::mat4));

	//for (const auto& batch : batch_seq) {
	//	SDL_GPUTextureSamplerBinding bindings{
	//		.texture = batch.texture,
	//		.sampler = pipeline.sampler,
	//	};

	//	SDL_BindGPUFragmentSamplers(render_pass, 0, &bindings, 1);

	//	SDL_DrawGPUIndexedPrimitives(render_pass, batch.num_indices, 1, batch.index_offset, 0, 0);
	//}
}

TextModule::TextModule(flecs::world& world) {
	world.module<TextModule>();

	//world.import<RenderModule>();
	world.import<CameraModule>();
	//world.import<SpriteRenderModule>();
	world.import<TransformModule>();

	world.component<TextStorage>().add(flecs::Singleton);
	world.component<TextPipeline>().add(flecs::Singleton);
	world.component<CameraCollectedTextItems>().add(flecs::Singleton);
	world.component<CameraTextBatches>().add(flecs::Singleton);

	world.component<TextFont>()
		.member(&TextFont::size, "size")
		.member(&TextFont::alignment, "alignment");
	world.component<TextData>()
		.member<glm::ivec2>("size");

	world.component<TextColor>()
		.is_a<Color>();

	world.component<TextOutline>()
		.member(&TextOutline::width, "width")
		.member(&TextOutline::color, "color");

	world.component<TextShadow>()
		.member(&TextShadow::shift, "shift")
		.member(&TextShadow::color, "color");

	world.component<InitText>();

	world.component<TextComputed>()
		.member(&TextComputed::computed_text, "computed_text");

	world.component<Text2d>()
		.is_a<std::string>()
		.add(flecs::With, world.component<InitText>())
		.add(flecs::With, world.component<TextComputed>())
		.add(flecs::With, world.component<RenderLayers>())
		.add(flecs::With, world.component<Aabb>())
		.add(flecs::With, world.component<Visible2d>())
		.add(flecs::With, world.component<TextData>())
		.add(flecs::With, world.component<TextFont>())
		.add(flecs::With, world.component<TextColor>())
		.add(flecs::With, world.component<Transform>());

	world.observer<TextFont>()
		.event(flecs::OnSet)
		.each([](flecs::entity entity, TextFont& font) {
			if (font.handle) {
				font.font_scale = font.size / font.handle->get_size();

				TTF_GetFontDPI(font.handle->get_resource(), &font.font_dpi.x, &font.font_dpi.y);

				entity.add<InitText>();
			}
		});

	world.observer<TextComputed>()
		.event(flecs::OnSet)
		.each([](flecs::entity entity, TextComputed&) {
			entity.add<InitText>();
		});

	//world.system<RenderDevice, TextPipeline>()
	//	.kind(Phases::OnStart)
	//	.each([&world](RenderDevice& device, TextPipeline& pipeline) {
	//		constexpr bool use_sdf = true;

	//		auto vert_shader = load_shader(*device.gpu, "assets/shaders/out/text.vert.msl", 1);
	//		auto frag_shader = load_shader(*device.gpu, use_sdf ? "assets/shaders/out/text_sdf.frag.msl" : "assets/shaders/out/text.frag.msl", 0, 1);

	//		SDL_GPUColorTargetDescription color_target_description = {
	//			.format = SDL_GetGPUSwapchainTextureFormat(device.gpu, world.get<WindowModule>().main_window),
	//			.blend_state = {
	//				.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
	//				.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
	//				.color_blend_op = SDL_GPU_BLENDOP_ADD,
	//				.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
	//				.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_DST_ALPHA,
	//				.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
	//				.color_write_mask = 0xF,
	//				.enable_blend = true,
	//			}
	//		};

	//		std::array<SDL_GPUVertexAttribute, 3> vertex_attributes = {
	//			SDL_GPUVertexAttribute{
	//				.location = 0,
	//				.buffer_slot = 0,
	//				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
	//				.offset = offsetof(Vertex, position),
	//			},
	//			SDL_GPUVertexAttribute{
	//				.location = 1,
	//				.buffer_slot = 0,
	//				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
	//				.offset = offsetof(Vertex, color),
	//			},
	//			SDL_GPUVertexAttribute{
	//				.location = 2,
	//				.buffer_slot = 0,
	//				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
	//				.offset = offsetof(Vertex, uv),
	//			}
	//		};

	//		SDL_GPUVertexBufferDescription vertex_buffer_description = {
	//			.slot = 0,
	//			.pitch = sizeof(Vertex),
	//			.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
	//			.instance_step_rate = 0,
	//		};

	//		SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info = {
	//			.vertex_shader = vert_shader,
	//			.fragment_shader = frag_shader,
	//			.vertex_input_state = {
	//				.vertex_buffer_descriptions = &vertex_buffer_description,
	//				.num_vertex_buffers = 1,
	//				.vertex_attributes = vertex_attributes.data(),
	//				.num_vertex_attributes = vertex_attributes.size(),
	//			},
	//			.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
	//			.target_info = {
	//				.color_target_descriptions = &color_target_description,
	//				.num_color_targets = 1,
	//				.depth_stencil_format =
	//					SDL_GPU_TEXTUREFORMAT_INVALID, /* Need to set this to avoid
	//														missing initializer for
	//														field error */
	//				.has_depth_stencil_target = false,
	//			},
	//		};

	//		SDL_GPUSamplerCreateInfo sampler_info = {
	//			.min_filter = SDL_GPU_FILTER_LINEAR,
	//			.mag_filter = SDL_GPU_FILTER_LINEAR,
	//			.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
	//			.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
	//			.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
	//			.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
	//		};

	//		pipeline.sampler = SDL_CreateGPUSampler(device.gpu, &sampler_info);
	//		pipeline.pipeline = SDL_CreateGPUGraphicsPipeline(device.gpu, &pipeline_create_info);

	//		SDL_ReleaseGPUShader(device.gpu, vert_shader);
	//		SDL_ReleaseGPUShader(device.gpu, frag_shader);

	//		pipeline.engine = TTF_CreateGPUTextEngine(device.gpu);
	//	});

	//world.system<RenderDevice, TextStorage>()
	//	.kind(Phases::OnStart)
	//	.each([](RenderDevice& device, TextStorage& storage) {
	//		SDL_GPUBufferCreateInfo vertex_buffer_create_info{
	//			.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
	//			.size = sizeof(Vertex) * TEXT_MAX_VERTEX_COUNT,
	//		};

	//		SDL_GPUBufferCreateInfo index_buffer_create_info{
	//			.usage = SDL_GPU_BUFFERUSAGE_INDEX,
	//			.size = sizeof(int) * TEXT_MAX_INDEX_COUNT,
	//		};

	//		SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
	//			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
	//			.size = (sizeof(Vertex) * TEXT_MAX_VERTEX_COUNT) + (sizeof(int) * TEXT_MAX_INDEX_COUNT)
	//		};

	//		storage.vertex_buffer = SDL_CreateGPUBuffer(device.gpu, &vertex_buffer_create_info);
	//		storage.index_buffer = SDL_CreateGPUBuffer(device.gpu, &index_buffer_create_info);
	//		storage.transfer_buffer = SDL_CreateGPUTransferBuffer(device.gpu, &transfer_buffer_create_info);
	//	});

	world.system<TextData, GlobalTransform, Aabb>()
		.with<Text2d>()
		.kind(Phases::Update)
		.each([](TextData& data, GlobalTransform& transform, Aabb& aabb) {
			aabb.min = transform.translation;
			aabb.max = glm::vec2(transform.translation) + glm::vec2(data.size);
		});

	auto transparent_2d = world.component<Transparent2d>();
	auto text_query = world.query<Text2d, TextFont, TextData, TextColor, GlobalTransform, Visible2d>();

	//world.system<VisibleEntities, CameraRenderPhaseItems, CameraCollectedTextItems>()
	//	.with<Camera>()
	//	.kind(Phases::CollectRenderData)
	//	.each([transparent_2d, text_query](flecs::entity camera_entity, VisibleEntities& visible_entities, CameraRenderPhaseItems& render_items, CameraCollectedTextItems& text_items) {
	//		text_query.each([&](flecs::entity entity, Text2d& text, TextFont& font, TextData& data, TextColor& color, GlobalTransform& transform, Visible2d& visible) {
	//			if (!visible.value) {
	//				return;
	//			}

	//			if (!visible_entities.entities.contains(entity)) {
	//				return;
	//			}

	//			render_items[camera_entity][transparent_2d].emplace_back(entity, &draw_text, transform.translation.z);

	//			text_items[camera_entity].lookup[entity] = text_items[camera_entity].items.size();
	//			text_items[camera_entity].items.emplace_back(entity, nullptr, transform, color, data.ttf_data, font.size / font.handle->get_size());
	//	});
	//});

	//world.system<CameraCollectedTextItems, CameraRenderPhaseItems, TextStorage, CameraTextBatches, RenderDevice, CopyCommands>()
	//	.kind(Phases::PrepareRenderData)
	//	.each([transparent_2d](CameraCollectedTextItems& camera_text_items, CameraRenderPhaseItems& camera_render_items, TextStorage& storage, CameraTextBatches& camera_batches, RenderDevice& device, CopyCommands& commands) {
	//		camera_batches.clear();

	//		auto transfer_buffer = SDL_MapGPUTransferBuffer(device.gpu, storage.transfer_buffer, false);

	//		auto vertices = static_cast<Vertex*>(transfer_buffer);
	//		auto indices = reinterpret_cast<int*>(vertices + TEXT_MAX_VERTEX_COUNT);

	//		auto vertex_count = 0;
	//		auto index_count = 0;

	//		for (auto& [camera, camera_phase_items] : camera_render_items) {
	//			auto& phase_items = camera_phase_items[transparent_2d];
	//			auto& text_items = camera_text_items[camera];
	//			auto& batches = camera_batches[camera];

	//			flecs::entity_t current_batch_entity = flecs::entity::null();
	//			size_t current_batch_index = 0;

	//			for (size_t i = 0; i < phase_items.size(); ++i) {
	//				const auto& data = phase_items[i];

	//				if (!text_items.lookup.contains(data.entity)) {
	//					current_batch_entity = flecs::entity::null();

	//					continue;
	//				}

	//				auto& text_data = text_items.items[text_items.lookup[data.entity]];

	//				auto seq = TTF_GetGPUTextDrawData(text_data.ttf_data);

	//				if (!seq) {
	//					current_batch_entity = flecs::entity::null();

	//					continue;
	//				}

	//				if (!current_batch_entity || batches.at(current_batch_entity).back().texture != seq->atlas_texture) {
	//					current_batch_entity = text_data.entity;
	//					current_batch_index = i;

	//					batches[current_batch_entity].emplace_back(
	//						static_cast<size_t>(index_count),
	//						0,
	//						seq->atlas_texture
	//					);
	//				}

	//				auto& current_batch = batches.at(current_batch_entity).back();

	//				while (seq) {
	//					if (seq->atlas_texture != current_batch.texture) {
	//						current_batch_entity = text_data.entity;
	//						current_batch_index = i;

	//						batches[current_batch_entity].emplace_back(
	//							static_cast<size_t>(index_count),
	//							0,
	//							seq->atlas_texture
	//						);

	//						current_batch = batches.at(current_batch_entity).back();
	//					}

	//					for (int j = 0; j < seq->num_vertices; j++) {
	//						const auto pos = seq->xy[j];
	//						const auto uv = seq->uv[j];

	//						vertices[vertex_count + j] = Vertex{
	//							.position{ text_data.transform.translation + glm::vec3{ pos.x, -pos.y, 0.f } * text_data.scale },
	//							.color = text_data.color,
	//							.uv{ uv.x, uv.y },
	//						};
	//					}

	//					for (int j = 0; j < seq->num_indices; j++) {
	//						indices[index_count + j] = seq->indices[j] + vertex_count;
	//					}

	//					vertex_count += seq->num_vertices;
	//					index_count += seq->num_indices;

	//					current_batch.num_indices += seq->num_indices;

	//					seq = seq->next;
	//				}

	//				++phase_items[current_batch_index].batch_size;
	//			}

	//			text_items.items.clear();
	//			text_items.lookup.clear();
	//		}

	//		if (vertex_count == 0 || index_count == 0) {
	//			return;
	//		}

	//		SDL_UnmapGPUTransferBuffer(device.gpu, storage.transfer_buffer);

	//		auto copy_pass = SDL_BeginGPUCopyPass(commands.buffer);

	//		SDL_GPUTransferBufferLocation vertex_transfer_buffer_location{
	//			.transfer_buffer = storage.transfer_buffer,
	//			.offset = 0 
	//		};
	//		SDL_GPUBufferRegion vertex_buffer_location{
	//			.buffer = storage.vertex_buffer,
	//			.offset = 0,
	//			.size = static_cast<Uint32>(sizeof(Vertex) * vertex_count)
	//		};

	//		SDL_UploadToGPUBuffer(copy_pass, &vertex_transfer_buffer_location, &vertex_buffer_location, false);

	//		SDL_GPUTransferBufferLocation index_transfer_buffer_location {
	//			.transfer_buffer = storage.transfer_buffer,
	//			.offset = static_cast<Uint32>(sizeof(Vertex) * TEXT_MAX_VERTEX_COUNT)
	//		};
	//		SDL_GPUBufferRegion index_buffer_region {
	//			.buffer = storage.index_buffer,
	//			.offset = 0,
	//			.size = static_cast<Uint32>(sizeof(int) * index_count)
	//		};

	//		SDL_UploadToGPUBuffer(copy_pass, &index_transfer_buffer_location, &index_buffer_region, false);

	//		SDL_EndGPUCopyPass(copy_pass);
	//	});

	TTF_Init();

	world.add<TextStorage>();
	world.add<TextPipeline>();
	world.add<CameraTextBatches>();
	world.add<CameraCollectedTextItems>();
}
