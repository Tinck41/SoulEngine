#include "module.h"

#include "ecsModule/common.h"
#include "text_vertex.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/render_module/module.h"
#include "ecsModule/windowModule/module.h"
#include "ecsModule/textModule/module.h"
#include "font.h"
#include "utils/sdl.h"
#include "SDL3_ttf/SDL_ttf.h"

using namespace se;

#define TEXT_MAX_VERTEX_COUNT 4000
#define TEXT_MAX_INDEX_COUNT  6000

TextRenderModule::TextRenderModule(flecs::world& world) {
	world.module<TextRenderModule>();

	world.import<RenderModule>();

	world.system<Text2d, TextComputed, TextFont, TextData>("init text 2d")
		.with<InitText>()
		.kind(Phases::Update)
		.each([&](flecs::entity entity, Text2d& text, TextComputed& computed, TextFont& font, TextData& data) {
			if (!font.handle) {
				return;
			}

			const auto actual_text = [&] {
				if (!computed.computed_text.empty()) {
					return computed.computed_text;
				}

				return static_cast<std::string>(text);
			}();

			const auto calculate_min_size = [&]() {
				size_t word_begin_index = 0;

				for (size_t i = 0; i < actual_text.size(); ++i) {
					if (actual_text[i] != ' ' || actual_text[i] != '\n') {
						continue;
					}

					int width;

					TTF_GetStringSize(font.handle->get_resource(), actual_text.c_str() + word_begin_index, i - word_begin_index, &width, nullptr);

					data.min_width = std::max(data.min_width, static_cast<float>(width));
					word_begin_index = i;
				}

				data.min_width *= font.font_scale;
			};

			if (!data.ttf_data) {
				data.ttf_data = TTF_CreateText(engine, *font.handle, text.c_str(), text.size());
				calculate_min_size();
			}
			else {
				TTF_SetTextString(data.ttf_data, text.c_str(), text.size());
				calculate_min_size();
			}

			TTF_GetTextSize(data.ttf_data, &data.size.x, &data.size.y);

			data.size = glm::vec2(data.size) * font.font_scale;

			entity.remove<InitText>();
		});

	world.observer<Text2d, TextData>()
		.event(flecs::OnRemove)
		.each([](flecs::entity e, Text2d& text, TextData& data){
			TTF_DestroyText(data.ttf_data);
			data.size = { 0, 0 };
		});

	engine = TTF_CreateGPUTextEngine(world.get<RenderDevice>().gpu);
}

TextRenderModule::~TextRenderModule() {
	TTF_DestroyRendererTextEngine(engine);
}

Material se::create_text_material(flecs::world& world) {
	const auto& device = world.get<RenderDevice>();

	auto vert_shader = load_shader(*device.gpu, "assets/shaders/out/text.vert.msl", 1);
	auto frag_shader = load_shader(*device.gpu, "assets/shaders/out/text_sdf.frag.msl", 0, 1);

	SDL_GPUColorTargetDescription color_target_description = {
		.format = SDL_GetGPUSwapchainTextureFormat(device.gpu, world.get<WindowModule>().main_window),
		.blend_state = {
			.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
			.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
			.color_blend_op = SDL_GPU_BLENDOP_ADD,
			.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
			.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_DST_ALPHA,
			.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
			.color_write_mask = 0xF,
			.enable_blend = true,
		}
	};

	std::array<SDL_GPUVertexAttribute, 5> vertex_attributes = {
		SDL_GPUVertexAttribute{
			.location = 0,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			.offset = offsetof(TextVertex, position),
		},
		SDL_GPUVertexAttribute{
			.location = 1,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(TextVertex, color),
		},
		SDL_GPUVertexAttribute{
			.location = 2,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = offsetof(TextVertex, uv),
		},
		SDL_GPUVertexAttribute{
			.location = 3,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
			.offset = offsetof(TextVertex, outline_width),
		},
		SDL_GPUVertexAttribute{
			.location = 4,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(TextVertex, outline_color),
		}
	};

	SDL_GPUVertexBufferDescription vertex_buffer_description = {
		.slot = 0,
		.pitch = sizeof(TextVertex),
		.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
		.instance_step_rate = 0,
	};

	SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info = {
		.vertex_shader = vert_shader,
		.fragment_shader = frag_shader,
		.vertex_input_state = {
			.vertex_buffer_descriptions = &vertex_buffer_description,
			.num_vertex_buffers = 1,
			.vertex_attributes = vertex_attributes.data(),
			.num_vertex_attributes = vertex_attributes.size(),
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.target_info = {
			.color_target_descriptions = &color_target_description,
			.num_color_targets = 1,
			.depth_stencil_format =
				SDL_GPU_TEXTUREFORMAT_INVALID, /* Need to set this to avoid
													missing initializer for
													field error */
			.has_depth_stencil_target = false,
		},
	};

	SDL_GPUSamplerCreateInfo sampler_info = {
		.min_filter = SDL_GPU_FILTER_LINEAR,
		.mag_filter = SDL_GPU_FILTER_LINEAR,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
	};

	auto sampler = SDL_CreateGPUSampler(device.gpu, &sampler_info);
	auto pipeline = SDL_CreateGPUGraphicsPipeline(device.gpu, &pipeline_create_info);

	SDL_ReleaseGPUShader(device.gpu, vert_shader);
	SDL_ReleaseGPUShader(device.gpu, frag_shader);

	return {
		.pipeline = pipeline,
		.sampler = sampler,
		.vertex_uniforms = { world.id<DefaultUniform>() },
	};
}

PhaseContext se::create_text_context(flecs::world& world) {
	const auto& device = world.get<RenderDevice>();

	SDL_GPUBufferCreateInfo vertex_buffer_create_info{
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = sizeof(TextVertex) * TEXT_MAX_VERTEX_COUNT,
	};

	SDL_GPUBufferCreateInfo index_buffer_create_info{
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
		.size = sizeof(int) * TEXT_MAX_INDEX_COUNT,
	};

	SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = (sizeof(TextVertex) * TEXT_MAX_VERTEX_COUNT) + (sizeof(int) * TEXT_MAX_INDEX_COUNT)
	};

	auto vertex_buffer = SDL_CreateGPUBuffer(device.gpu, &vertex_buffer_create_info);
	auto index_buffer = SDL_CreateGPUBuffer(device.gpu, &index_buffer_create_info);
	auto transfer_buffer = SDL_CreateGPUTransferBuffer(device.gpu, &transfer_buffer_create_info);

	return {
		.index_buffer = index_buffer,
		.vertex_buffer = vertex_buffer,
		.transfer_buffer = transfer_buffer,
		.index_element_size = SDL_GPU_INDEXELEMENTSIZE_32BIT,
	};
}

RenderPhaseExtractor se::create_text_extractor(flecs::world& world, flecs::entity_t helper) {
	auto text_query = world.query_builder()
		.with<const Text2d>()
		.with<const TextFont>()
		.with<const TextColor>()
		.with<const TextOutline>().optional()
		.with<const TextShadow>().optional()
		.with<const TextData>()
		.with<const GlobalTransform>()
		.with<const Aabb>()
		.with<const Material>().optional()
		.with<ExtractedText2ds>().src("$helper").inout()
		.with<RenderPhase>().src("$phase_entity").inout()
		.with<Aabb>().src("$camera").inout()
		.build();

	return {
		.callback = [](flecs::iter& it) {
			auto helper = it.get_var("helper");

			while (it.next()) {
				auto text_field = it.field<const Text2d>(0);
				auto font_field = it.field<const TextFont>(1);
				auto color_field = it.field<const TextColor>(2);
				auto data_field = it.field<const TextData>(5);
				auto transform_field = it.field<const GlobalTransform>(6);
				auto aabb_field = it.field<const Aabb>(7);

				auto& extracted_texts = it.field<ExtractedText2ds>(9)[0];
				auto& render_phase = it.field<RenderPhase>(10)[0];
				auto& camera_aabb = it.field<Aabb>(11)[0];

				for (auto i : it) {
					if (!camera_aabb.is_intersect(aabb_field[i])) {
						continue;
					}

					const auto entity = it.entity(i);

					const auto& text = text_field[i];
					const auto& font = font_field[i];
					const auto& color = color_field[i];
					const auto& data = data_field[i];
					const auto& transform = transform_field[i];

					const auto has_shadow = it.is_set(4);
					const auto material_id = it.is_set(8) ? entity : helper;
					const auto [outline_width, outline_color] = [&] {
						if (it.is_set(3)) {
							const auto& outline_data = it.field<const TextOutline>(3)[i];

							return std::pair{ outline_data.width, outline_data.color };
						}

						return std::pair{ 0.f, TRANSPARENT };
					}();

					auto seq = TTF_GetGPUTextDrawData(data.ttf_data);

					SDL_GPUTexture* last_texture = nullptr;

					while (seq) {
						if (last_texture == seq->atlas_texture) {
							seq = seq->next;

							continue;
						}

						render_phase.items.emplace_back(entity, helper, material_id, extracted_texts.size(), transform.translation.z);

						extracted_texts.emplace_back(
							entity,
							material_id,
							seq->atlas_texture,
							transform.translation,
							transform.scale,
							font.font_scale,
							color,
							outline_width,
							outline_color,
							seq
						);

						last_texture = seq->atlas_texture;
						seq = seq->next;
					}
				}
			}
		},
		.query = text_query,
		.helper = helper,
	};
}

RenderPhaseUploader se::create_text_uploader() {
	return {
		.callback = [](flecs::world& world, flecs::entity& render_phase, flecs::entity& uploader, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass) {
			auto& extracted_texts = uploader.get_mut<ExtractedText2ds>();
			auto& context = uploader.get_mut<PhaseContext>();
			auto& phase_items = render_phase.get_mut<RenderPhase>().items;

			auto transfer_buffer = SDL_MapGPUTransferBuffer(device, context.transfer_buffer, false);

			auto vertices = static_cast<TextVertex*>(transfer_buffer);
			auto indices = reinterpret_cast<int*>(vertices + TEXT_MAX_VERTEX_COUNT);

			auto vertex_count = 0;
			auto index_count = 0;

			size_t batch_index = 0;

			for (size_t i = 0; i < phase_items.size(); ++i) {
				auto& data = phase_items[i];

				if (data.context_entity != uploader || data.extracted_index >= extracted_texts.size() || extracted_texts[data.extracted_index].entity != data.entity) {
					batch_index = i + 1;

					continue;
				}

				const auto& text_data = extracted_texts[data.extracted_index];

				auto seq = text_data.seq;

				if (!seq) {
					batch_index = i + 1;

					continue;
				}

				if (phase_items[batch_index].textures.empty() || phase_items[batch_index].textures[0] != seq->atlas_texture) {
					batch_index = i;

					data.textures.emplace_back(seq->atlas_texture);
					data.num_instances = 1;
					data.first_index = index_count;
				}

				while (seq) {
					if (seq->atlas_texture != phase_items[batch_index].textures[0]) {
						batch_index = i + 1;

						break;
					}

					for (int j = 0; j < seq->num_vertices; j++) {
						const auto pos = seq->xy[j];
						const auto uv = seq->uv[j];

						vertices[vertex_count + j] = TextVertex{
							.position{ text_data.translation + glm::vec2{ pos.x, -pos.y } * text_data.scale * text_data.font_scale, 0.f },
							.color = text_data.color,
							.uv{ uv.x, uv.y },
							.outline_width = text_data.outline_width,
							.outline_color = text_data.outline_color,
						};
					}

					for (int j = 0; j < seq->num_indices; j++) {
						indices[index_count + j] = seq->indices[j] + vertex_count;
					}

					vertex_count += seq->num_vertices;
					index_count += seq->num_indices;

					phase_items[batch_index].num_indices += seq->num_indices;

					seq = seq->next;
				}

				++phase_items[batch_index].batch_size;
			}

			extracted_texts.clear();

			SDL_UnmapGPUTransferBuffer(device, context.transfer_buffer);

			if (vertex_count == 0 || index_count == 0) {
				return;
			}

			SDL_GPUTransferBufferLocation vertex_transfer_buffer_location{
				.transfer_buffer = context.transfer_buffer,
				.offset = 0 
			};
			SDL_GPUBufferRegion vertex_buffer_location{
				.buffer = context.vertex_buffer,
				.offset = 0,
				.size = static_cast<Uint32>(sizeof(TextVertex) * vertex_count)
			};

			SDL_UploadToGPUBuffer(copy_pass, &vertex_transfer_buffer_location, &vertex_buffer_location, false);

			SDL_GPUTransferBufferLocation index_transfer_buffer_location {
				.transfer_buffer = context.transfer_buffer,
				.offset = static_cast<Uint32>(sizeof(TextVertex) * TEXT_MAX_VERTEX_COUNT)
			};
			SDL_GPUBufferRegion index_buffer_region {
				.buffer = context.index_buffer,
				.offset = 0,
				.size = static_cast<Uint32>(sizeof(int) * index_count)
			};

			SDL_UploadToGPUBuffer(copy_pass, &index_transfer_buffer_location, &index_buffer_region, false);
		}
	};
}
