#include "module.h"

#include "debug/profiler.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/common.h"
#include "ecsModule/render_module/default_uniform.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/windowModule/components.h"
#include "ext/matrix_transform.hpp"
#include "render_phase.h"
#include "material.h"
#include "spdlog/spdlog.h"
#include "utils/visit.h"

#include <algorithm>
#include <format>

using namespace se;

glm::mat4 extract_view_default(const Camera& camera, const GlobalTransform& transform) {
	const auto view = glm::translate(glm::mat4(1.f), -transform.translation);

	return camera.projection * view;
}

void extract(flecs::iter& it) {
	auto world = it.world();

	const auto& device = world.get<RenderDevice>();
	auto command_buffer = SDL_AcquireGPUCommandBuffer(device.gpu);
	auto copy_pass = SDL_BeginGPUCopyPass(command_buffer);

	while(it.next()) {
		auto camera_field = it.field<Camera>(0);
		auto render_phases_field = it.field<RenderPhases>(1);
		auto traansform_field = it.field<GlobalTransform>(2);

		for (auto i : it) {
			auto& camera = camera_field[i];
			auto& render_phases = render_phases_field[i];
			auto& transform = traansform_field[i];

			const auto aabb = Aabb{
				.min = glm::vec2(transform.translation),
				.max = glm::vec2(transform.translation) + camera.viewport,
			};

			for (const auto& phase : render_phases) {
				auto phase_entity = world.entity(phase);
				const auto& render_phase = phase_entity.get<RenderPhase>();

				for (const auto& extractor : render_phase.extractors) {
					auto extractor_entity = world.entity(extractor);

					if (!extractor_entity.enabled()) {
						continue;
					}

					auto& phase_extractor = extractor_entity.get_mut<RenderPhaseExtractor>();

					phase_extractor.query
						.set_var("phase_entity", phase_entity)
						.set_var("helper", phase_extractor.helper)
						.set_var("camera", it.entity(i))
						.run(phase_extractor.callback);
				}

				// TODO: Aabb test

				for (const auto& sorter : render_phase.sorters) {
					auto sorter_entity = world.entity(sorter);

					if (!sorter_entity.enabled()) {
						continue;
					}

					auto& phase_sorter = sorter_entity.get_mut<RenderPhaseSorter>();

					phase_sorter.callback(world, phase_entity, sorter_entity);
				}

				for (const auto& uploader : render_phase.uploaders) {
					auto uploader_entity = world.entity(uploader);

					if (!uploader_entity.enabled()) {
						continue;
					}

					auto& phase_uploader = uploader_entity.get_mut<RenderPhaseUploader>();

					phase_uploader.callback(world, phase_entity, uploader_entity, device.gpu, copy_pass);
				}
			}
		}
	}

	SDL_EndGPUCopyPass(copy_pass);
	assert(SDL_SubmitGPUCommandBuffer(command_buffer) && SDL_GetError());
}

void render(flecs::iter& it) {
	push_profile_scope("render");
	auto world = it.world();

	const auto& device = world.get<RenderDevice>();
	auto& stats = world.get_mut<RenderStats>();
	push_profile_scope("acquire gpu command buffer");
	auto command_buffer = SDL_AcquireGPUCommandBuffer(device.gpu);
	pop_profile_scope();

	stats.draw_calls = 0;

	push_profile_scope("acquire swapchain_texture");
	world.each([&command_buffer](Window& window) {
		const auto flags = SDL_GetWindowFlags(window.handle);

		//if ((flags & SDL_WINDOW_HIDDEN) || (flags & SDL_WINDOW_MINIMIZED) || (flags & SDL_WINDOW_OCCLUDED)) {
		//	window.swapchain_texture = nullptr;
		//	return;
		//}

		push_profile_scope("SDL_AcquireGPUSwapchainTexture()");
		SDL_AcquireGPUSwapchainTexture(command_buffer, window.handle, &window.swapchain_texture, nullptr, nullptr);
		pop_profile_scope();

		if (!window.swapchain_texture) {
			return;
		}

		auto color_target = SDL_GPUColorTargetInfo{
			.texture = window.swapchain_texture,
			.clear_color = BLACK,
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.store_op = SDL_GPU_STOREOP_STORE
		};

		push_profile_scope("clear pass");
		auto clear_pass =  SDL_BeginGPURenderPass(command_buffer, &color_target, 1, nullptr);
		SDL_EndGPURenderPass(clear_pass);
		pop_profile_scope();
	});
	pop_profile_scope();

	push_profile_scope("render loop");

	while(it.next()) {
		auto camera_field = it.field<Camera>(0);
		auto render_phases_field = it.field<RenderPhases>(1);
		auto traansform_field = it.field<GlobalTransform>(2);

		for (auto i : it) {
			auto camera_entity = it.entity(i);
			auto& camera = camera_field[i];
			auto& render_phases = render_phases_field[i];
			auto& transform = traansform_field[i];

			auto render_texture = visit(camera.render_target, visitors{
				[&world](flecs::entity_t window_entity) {
					return world.entity(window_entity).get<Window>().swapchain_texture;
				},
				[](std::shared_ptr<Texture>& texture) {
					return &texture->get_gpu_texture();
				},
				[](auto&&) -> SDL_GPUTexture* {
					throw std::runtime_error("invalid render target");
				}
			});

			if (!render_texture) {
				for (const auto& phase : render_phases) {
					world.entity(phase).get_mut<RenderPhase>().items.clear();
				}

				continue;
			}

			auto color_target = SDL_GPUColorTargetInfo{
				.texture = render_texture,
				.clear_color = camera.clear_color,
				.load_op = camera.load_op,
				.store_op = SDL_GPU_STOREOP_STORE
			};
			auto depth_stencil_target = SDL_GPUDepthStencilTargetInfo{
				.texture = camera.depth_texture,
				.clear_depth = 1.f,
				.load_op = SDL_GPU_LOADOP_CLEAR,
				.store_op = SDL_GPU_STOREOP_DONT_CARE,
			};
			auto render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target, 1, camera.depth_texture ? &depth_stencil_target : nullptr);

			SDL_PushGPUDebugGroup(command_buffer, std::format("camera: {}", camera_entity.name().size() != 0 ? std::string(camera_entity.name()) : std::to_string(camera_entity.id())).c_str());

			auto default_uniform = DefaultUniform{
				.view_position = transform.translation,
				.delta_time = it.delta_time(),
			};

			for (const auto& phase : render_phases) {
				auto phase_entity = world.entity(phase);
				auto& render_phase = phase_entity.get_mut<RenderPhase>();

				push_profile_scope(phase_entity.name().c_str());

				default_uniform.view_proj = render_phase.extract_view_callback
					? render_phase.extract_view_callback(camera, transform)
					: extract_view_default(camera, transform);
				default_uniform.viewport = camera.viewport;

				SDL_PushGPUDebugGroup(command_buffer, std::format("phase: {}", phase_entity.name().c_str()).c_str());

				for (const auto& renderer : render_phase.renderers) {
					auto renderer_entity = world.entity(renderer);

					if (!renderer_entity.enabled()) {
						continue;
					}

					auto& phase_renderer = renderer_entity.get_mut<RenderPhaseRenderer>();

					phase_renderer.callback(world, phase_entity, renderer_entity, default_uniform, render_pass, command_buffer);
				}

				SDL_PopGPUDebugGroup(command_buffer);

				render_phase.items.clear();

				pop_profile_scope();
			}

			SDL_EndGPURenderPass(render_pass);
			SDL_PopGPUDebugGroup(command_buffer);
		}
	}

	pop_profile_scope();

	push_profile_scope("submit gpu commands");
	assert(SDL_SubmitGPUCommandBuffer(command_buffer) && SDL_GetError());
	pop_profile_scope();
	pop_profile_scope();
}

RenderModule::RenderModule(flecs::world& world) {
	world.module<RenderModule>();

	world.import<CameraModule>();

	world.component<DefaultUniform>();
	world.component<PhaseContext>();
	world.component<Material>()
		.add(flecs::OnInstantiate, flecs::Inherit);
	world.component<RenderPhase>();
	world.component<RenderPhaseExtractor>();
	world.component<RenderPhaseSorter>();
	world.component<RenderPhaseUploader>();
	world.component<RenderPhaseRenderer>();
	world.component<RenderPhases>();
	world.component<RenderDevice>();
	world.component<RenderStats>()
		.add(flecs::Singleton);

	world.component<ClipContent>()
		.is_a<RectI>()
		.member(&ClipContent::min, "min")
		.member(&ClipContent::max, "max");

	world.observer<RenderDevice>()
		.event(flecs::OnSet)
		.each([&world](RenderDevice& render_device) {
			world.each([&render_device](Window& window) {
				assert(SDL_ClaimWindowForGPUDevice(render_device.gpu, window.handle) && SDL_GetError());

				SDL_GPUPresentMode presentMode = SDL_GPU_PRESENTMODE_VSYNC;
				// TODO
				//if (SDL_WindowSupportsGPUPresentMode(render_device.gpu, window.handle, SDL_GPU_PRESENTMODE_IMMEDIATE)) {
				//	presentMode = SDL_GPU_PRESENTMODE_IMMEDIATE;
				//}
				//else if (SDL_WindowSupportsGPUPresentMode(render_device.gpu, window.handle, SDL_GPU_PRESENTMODE_MAILBOX)) {
				//	presentMode = SDL_GPU_PRESENTMODE_MAILBOX;
				//}

				SDL_SetGPUSwapchainParameters(render_device.gpu, window.handle, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, presentMode);
			});
		});

	world.system<Camera, RenderPhases, GlobalTransform>("extract")
		.kind(Phases::Render)
		.run(extract);

	world.system<Camera, RenderPhases, GlobalTransform>("render")
		.kind(Phases::Render)
		.run(render);

	auto gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL, true, nullptr);

	assert(gpu && SDL_GetError());

	world.set<RenderDevice>({ gpu });
	world.set<WhiteTexture>({ std::make_shared<Texture>(gpu, glm::uvec2{1, 1}) });
	world.add<RenderStats>();
}

RenderPhaseSorter se::create_default_sorter() {
	return {
		.callback = [](flecs::world& world, flecs::entity& render_phase, flecs::entity& sorter) {
			auto& phase_items = render_phase.get_mut<RenderPhase>().items;

			std::ranges::sort(phase_items, [](const RenderItem& lhs, const RenderItem& rhs) {
				if (lhs.sort_value == rhs.sort_value) {
					return lhs.entity < rhs.entity;
				}

				return lhs.sort_value < rhs.sort_value;
			});
		}
	};
}

RenderPhaseRenderer se::create_default_renderer() {
	return {
		.callback = [](flecs::world& world, flecs::entity& render_phase, flecs::entity& renderer, const DefaultUniform& default_uniform, SDL_GPURenderPass* render_pass, SDL_GPUCommandBuffer* cmd_buffer) {
			auto* white_texture = &world.get<WhiteTexture>().texture->get_gpu_texture();
			auto& phase_items = renderer.get_mut<RenderPhase>().items;
			const auto default_uniform_id = world.id<DefaultUniform>();

			auto& stats = world.get_mut<RenderStats>();

			std::optional<RectI> last_scissor = std::nullopt;

			flecs::entity_t last_context = 0;
			flecs::entity_t last_material = 0;

			const PhaseContext* context = nullptr;
			const Material* material = nullptr;

			for (size_t i = 0; i < phase_items.size();) {
				bool indexed = false;

				const auto& item = phase_items[i];

				if (item.num_indices == 0) {
					auto entity = world.entity(item.entity);
					spdlog::warn("[default_renderer]: item [{}:{}] has 0 indices", entity.id(), entity.name());
					i += item.batch_size > 0 ? item.batch_size : 1;
					continue;
				}

				if (item.scissor != last_scissor) {
					last_scissor = item.scissor;

					if (last_scissor) {
						auto temp = RectI::clamp(last_scissor.value(), RectI::from_xy_size(0, 0, default_uniform.viewport)).to_sdl_rect();

						SDL_SetGPUScissor(render_pass, &temp);
					}
					else {
						auto temp = SDL_Rect{
							.x = 0,
							.y = 0,
							.w = default_uniform.viewport.x,
							.h = default_uniform.viewport.y,
						};

						SDL_SetGPUScissor(render_pass, &temp);
					}
				}

				if (item.context_entity != last_context) {
					context = &world.entity(item.context_entity).get<PhaseContext>();
					last_context = item.context_entity;

					if (context->vertex_buffer) {
						SDL_GPUBufferBinding vertex_buffer_binding{
							.buffer = context->vertex_buffer,
						};
						SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_buffer_binding, 1);
					}

					if (context->index_buffer) {
						SDL_GPUBufferBinding index_buffer_binding{
							.buffer = context->index_buffer,
						};
						SDL_BindGPUIndexBuffer(render_pass, &index_buffer_binding, context->index_element_size);

						indexed = true;
					}

					if (context->storage_buffer) {
						SDL_BindGPUVertexStorageBuffers(render_pass, 0, &context->storage_buffer, 1);
					}
				}

				if (item.material_id != last_material) {
					material = &world.entity(item.material_id).get<Material>();

					SDL_GPUTextureSamplerBinding texture_sampler_binding{
						.texture = item.texture ? item.texture : white_texture,
						.sampler = material->sampler,
					};

					SDL_BindGPUGraphicsPipeline(render_pass, material->pipeline);
					SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_binding, 1);

					for (size_t i = 0; i < material->vertex_uniforms.size(); ++i) {
						const auto& uniform_id = material->vertex_uniforms[i];

						if (uniform_id == default_uniform_id) {
							SDL_PushGPUVertexUniformData(cmd_buffer, i, &default_uniform, sizeof(default_uniform));
						}
						else {
							const auto& entity = world.entity(item.entity);

							SDL_PushGPUVertexUniformData(cmd_buffer, i, entity.get(uniform_id), world.type_info(uniform_id)->size);
						}
					}

					for (size_t i = 0; i < material->fragment_uniforms.size(); ++i) {
						const auto& uniform_id = material->fragment_uniforms[i];

						if (uniform_id == default_uniform_id) {
							SDL_PushGPUFragmentUniformData(cmd_buffer, i, &default_uniform, sizeof(default_uniform));
						}
						else {
							const auto& entity = world.entity(item.entity);

							SDL_PushGPUFragmentUniformData(cmd_buffer, i, entity.get(uniform_id), world.type_info(uniform_id)->size);
						}
					}
				}

				if (indexed) {
					SDL_DrawGPUIndexedPrimitives(render_pass, item.num_indices, item.num_instances, item.first_index, item.vertex_offset, item.first_instance);
				}
				else {
					SDL_DrawGPUPrimitives(render_pass, item.num_indices, item.num_instances, item.vertex_offset, item.first_instance);
				}

				++stats.draw_calls;

				i += item.batch_size > 0 ? item.batch_size : 1;
			}
		}
	};
}
