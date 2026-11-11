#include "module.h"

#include "ecsModule/render_module/default_uniform.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/windowModule/module.h"
#include "ecsModule/spriteModule/module.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/render_module/module.h"
#include "utils/sdl.h"

using namespace se;

#define SPRITE_MAX_INSTANCE_COUNT 4000

SpriteRenderModule::SpriteRenderModule(flecs::world& world) {
	world.module<SpriteRenderModule>();

	world.component<ExtractedSprites>();
	world.component<CirlceUniform>()
		.member(&CirlceUniform::radius, "radius")
		.member(&CirlceUniform::softness, "softness")
		.member(&CirlceUniform::center, "center");
}

Material se::create_sprite_material(flecs::world& world) {
	auto& device = world.get<RenderDevice>();

	auto vert_shader = load_shader(*device.gpu, "assets/shaders/out/sprite_batch.vert.msl", 1);
	auto frag_shader = load_shader(*device.gpu, "assets/shaders/out/sprite_batch.frag.msl", 0, 1);

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

	std::array<SDL_GPUVertexAttribute, 6> vertex_attrs{
		SDL_GPUVertexAttribute{
			.location = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(SpriteInstance, translation),
		},
		SDL_GPUVertexAttribute{
			.location = 1,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(SpriteInstance, rotation),
		},
		SDL_GPUVertexAttribute{
			.location = 2,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(SpriteInstance, scale),
		},
		SDL_GPUVertexAttribute{
			.location = 3,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(SpriteInstance, color),
		},
		SDL_GPUVertexAttribute{
			.location = 4,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = offsetof(SpriteInstance, uv),
		},
		SDL_GPUVertexAttribute{
			.location = 5,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = offsetof(SpriteInstance, size),
		},
	};

	SDL_GPUVertexBufferDescription vertex_buffer_description{
		.slot = 0,
		.pitch = sizeof(SpriteInstance),
		.input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
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

PhaseContext se::create_sprite_context(flecs::world& world) {
	auto& device = world.get<RenderDevice>();

	std::array<std::uint16_t, 6> indices{
		0, 1, 2,
		2, 1, 3
	};

	const auto instances_byte_szie = SPRITE_MAX_INSTANCE_COUNT * sizeof(SpriteInstance);
	const auto indices_byte_szie = indices.size() * sizeof(indices[0]);

	SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = instances_byte_szie,
	};

	auto transfer_buffer = SDL_CreateGPUTransferBuffer(device.gpu, &transfer_buffer_create_info);

	SDL_GPUBufferCreateInfo instance_buffer_create_info{
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = instances_byte_szie,
	};
	SDL_GPUBufferCreateInfo index_buffer_create_info{
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
		.size = indices_byte_szie,
	};

	auto vertex_buffer = SDL_CreateGPUBuffer(device.gpu, &instance_buffer_create_info);
	auto index_buffer = SDL_CreateGPUBuffer(device.gpu, &index_buffer_create_info);

	{
		auto transfer_mem = SDL_MapGPUTransferBuffer(device.gpu, transfer_buffer, false);

		std::memcpy(transfer_mem, indices.data(), indices_byte_szie);

		SDL_UnmapGPUTransferBuffer(device.gpu, transfer_buffer);

		auto copy_cmd_buf = SDL_AcquireGPUCommandBuffer(device.gpu);

		auto copy_pass = SDL_BeginGPUCopyPass(copy_cmd_buf);

		SDL_GPUTransferBufferLocation index_buffer_location{
			.transfer_buffer = transfer_buffer
		};

		SDL_GPUBufferRegion index_buffer_region{
			.buffer = index_buffer,
			.size = static_cast<uint32_t>(indices_byte_szie)
		};

		SDL_UploadToGPUBuffer(copy_pass, &index_buffer_location, &index_buffer_region, false);

		SDL_EndGPUCopyPass(copy_pass);

		assert(SDL_SubmitGPUCommandBuffer(copy_cmd_buf) && SDL_GetError());
	}

	return {
		.index_buffer = index_buffer,
		.vertex_buffer = vertex_buffer,
		.transfer_buffer = transfer_buffer,
		.index_element_size = SDL_GPU_INDEXELEMENTSIZE_16BIT,
	};
}

RenderPhaseExtractor se::create_sprite_extractor(flecs::world& world, flecs::entity_t helper) {
	auto sprite_query = world.query_builder()
		.with<Sprite>()
		.with<GlobalTransform>()
		.with<Aabb>()
		.with<Material>().optional()
		.with<ExtractedSprites>().src("$helper").inout()
		.with<RenderPhase>().src("$phase_entity").inout()
		.with<Aabb>().src("$camera").inout()
		.build();

	return RenderPhaseExtractor{
		.callback = [](flecs::iter& it) {
			auto helper = it.get_var("helper");

			while(it.next()) {
				auto sprite_field = it.field<Sprite>(0);
				auto transform_field = it.field<GlobalTransform>(1);
				auto aabb_field = it.field<Aabb>(2);

				auto& extracted_sprites = it.field<ExtractedSprites>(4)[0];
				auto& render_phase = it.field<RenderPhase>(5)[0];
				auto& camera_aabb = it.field<Aabb>(6)[0];

				for (auto i : it) {
					if (!camera_aabb.is_intersect(aabb_field[i])) {
						continue;
					}

					const auto entity = it.entity(i);

					const auto& sprite = sprite_field[i];
					const auto& transform = transform_field[i];

					const auto material_id = it.is_set(3) ? entity : helper;
					const auto [uv, size] = [&] {
						if (sprite.texture_atlas) {
							const auto& atlas = sprite.texture_atlas.value();
							const auto& rect = atlas.rects[atlas.current_index];

							return std::pair{ glm::vec2{ rect.x, rect.y }, glm::vec2{ rect.w, rect.h }};
						}

						return std::pair{ glm::vec2{ 0.f, 0.f }, sprite.texture->get_size() };
					}();

					render_phase.items.emplace_back(entity, helper, material_id, extracted_sprites.size(), transform.translation.z);

					extracted_sprites.emplace_back(
						entity,
						material_id,
						&sprite.texture->get_gpu_texture(),
						sprite.texture->get_size(),
						transform.translation,
						transform.rotation,
						transform.scale,
						sprite.color,
						size,
						uv
					);
				}
			}
		},
		.query = sprite_query,
		.helper = helper,
	};
}

RenderPhaseUploader se::create_sprite_uploader() {
	return RenderPhaseUploader{
		.callback = [](flecs::world& world, flecs::entity& render_phase, flecs::entity& uploader, SDL_GPUDevice* gpu, SDL_GPUCopyPass* copy_pass) {
			auto& extracted_sprites = uploader.get_mut<ExtractedSprites>();
			auto& context = uploader.get_mut<PhaseContext>();
			auto& phase_items = render_phase.get_mut<RenderPhase>().items;

			auto instances = static_cast<SpriteInstance*>(SDL_MapGPUTransferBuffer(gpu, context.transfer_buffer, false));
			std::uint32_t current_instance = 0;

			size_t batch_index = 0;

			for (size_t i = 0; i < phase_items.size(); ++i) {
				auto& render_data = phase_items[i];

				if (render_data.context_entity != uploader || render_data.extracted_index >= extracted_sprites.size() || extracted_sprites[render_data.extracted_index].entity != render_data.entity) {
					batch_index = i + 1;

					continue;
				}

				const auto& sprite = extracted_sprites[render_data.extracted_index];

				render_data.num_indices = 6;

				if (!phase_items[batch_index].texture || phase_items[batch_index].texture != sprite.texture || phase_items[batch_index].material_id != sprite.material_id) {
					batch_index = i;
					phase_items[batch_index].texture = sprite.texture;
					phase_items[batch_index].first_instance = current_instance;
				}

				instances[current_instance].translation = glm::vec4(sprite.translation, 0.f, 0.f);
				instances[current_instance].rotation    = glm::vec4(sprite.rotation, 0.f, 0.f);
				instances[current_instance].scale       = glm::vec4(sprite.scale * sprite.size, 0.f, 0.f);
				instances[current_instance].color       = sprite.color;
				instances[current_instance].uv          = sprite.uv / sprite.texture_size;
				instances[current_instance].size        = sprite.size / sprite.texture_size;

				++current_instance;
				++phase_items[batch_index].num_instances;
				++phase_items[batch_index].batch_size;
			}

			extracted_sprites.clear();

			SDL_UnmapGPUTransferBuffer(gpu, context.transfer_buffer);

			if (current_instance == 0) {
				return;
			}

			const auto buffer_location = SDL_GPUTransferBufferLocation{
				.transfer_buffer = context.transfer_buffer,
			};

			const auto buffer_region = SDL_GPUBufferRegion{
				.buffer = context.vertex_buffer,
				.size = static_cast<Uint32>(current_instance * sizeof(SpriteInstance))
			};

			SDL_UploadToGPUBuffer(copy_pass,  &buffer_location, &buffer_region, false);
		},
	};
}
