#include "module.h"

#include "ecsModule/common.h"
//#include "ecsModule/physicsModule/module.h"
//#include "ecsModule/renderModule/module.h"
//#include "ecsModule/timeModule/module.h"
#include "ecsModule/inputModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/windowModule/module.h"
#include "ecsModule/windowModule/components.h"
#include "ext/matrix_clip_space.hpp"
#include "spdlog/spdlog.h"

using namespace se;

CameraModule::CameraModule(flecs::world& world) {
	world.module<CameraModule>();

	//world.import<PhysicsModule>();
	world.import<TransformModule>();
	world.import<WindowModule>();
	world.import<InputModule>();
	//world.import<TimeModule>();
	//world.import<RenderModule>();

	// NOTE: requires RenderModule::CameraCompositionPipeline
	world.component<CameraCompositionGraph>()
		.add(flecs::Singleton);

	world.component<VisibleEntities>()
		.member<std::unordered_set<flecs::entity_t>>("entities");

	world.component<RenderLayers>()
		.member<std::uint32_t>("mask");

	world.component<Camera>()
		.add(flecs::With, world.component<RenderLayers>())
		.add(flecs::With, world.component<VisibleEntities>())
		.add(flecs::With, world.component<Transform>())
		.add(flecs::With, world.component<Aabb>())
//		.add(flecs::With, world.component<Velocity>())
		.add(flecs::Exclusive);

	world.component<Camera>()
		.member<glm::vec2>("viewport")
		.member<glm::vec2>("offset");

	world.component<Visible2d>()
		.member<bool>("value");

	world.component<Aabb>()
		.member<glm::vec2>("min")
		.member<glm::vec2>("max");

	auto main_window_query = world.query_builder<Window>()
		.with<MainWindow>()
		.build();

	world.observer<Camera>()
		.event(flecs::OnAdd)
		.each([main_window_query](Camera& camera) {
			if (!std::holds_alternative<std::monostate>(camera.render_target)) {
				return;
			}

			main_window_query.each([&](flecs::entity entity, Window& window) {
				camera.projection = glm::ortho(0.f, static_cast<float>(window.width), static_cast<float>(window.height), 0.f, -1.f, 1.f);
				camera.viewport = glm::vec2(window.width, window.height);
				camera.render_target = entity;
			});
		});

	world.observer<Camera>()
		.event(flecs::OnSet)
		.each([&world, main_window_query](Camera& camera) {
			if (std::holds_alternative<flecs::entity_t>(camera.render_target)) {
				auto window = world.entity(std::get<flecs::entity_t>(camera.render_target)).get<Window>();

				camera.projection = glm::ortho(0.f, static_cast<float>(window.width), static_cast<float>(window.height), 0.f, -1.f, 1.f);
				camera.viewport = glm::vec2(window.width, window.height);
			}
			else if (std::holds_alternative<std::shared_ptr<Texture>>(camera.render_target)) {
				const auto texture = std::get<std::shared_ptr<Texture>>(camera.render_target);

				camera.viewport = texture->get_size();
			}
			else {
				main_window_query.each([&](flecs::entity entity, Window& window) {
					camera.projection = glm::ortho(0.f, static_cast<float>(window.width), static_cast<float>(window.height), 0.f, -1.f, 1.f);
					camera.viewport = glm::vec2(window.width, window.height);
					camera.render_target = entity;
				});
			}
		});

	world.observer<Camera>()
		.event<WindowResize>()
		.each([](flecs::iter& it, size_t i, Camera& camera) {
			const auto eventData = it.param<WindowResize>();

			if (std::holds_alternative<flecs::entity_t>(camera.render_target) && std::get<flecs::entity_t>(camera.render_target) == eventData->window_entity) {
				camera.projection = glm::ortho(0.f, static_cast<float>(eventData->width), static_cast<float>(eventData->height), 0.f, -1.f, 1.f);
				camera.viewport = glm::vec2(eventData->width, eventData->height);
			}
		});

	world.system<Camera, GlobalTransform, Aabb>()
		.kind(Phases::Update)
		.each([](Camera& camera, GlobalTransform& transform, Aabb& aabb) {
			aabb.min = transform.translation;
			aabb.max = glm::vec2(transform.translation) + camera.viewport;
		});

	world.system<Visible2d>()
		.kind(Phases::PreUpdate)
		.each([](flecs::entity entity, Visible2d& visible) {
			visible.value = false;
		});

	auto visible_query = world.query<Aabb, Visible2d, RenderLayers>();

	world.system<Camera, GlobalTransform, RenderLayers&, VisibleEntities>()
		.kind(Phases::PostUpdate)
		.each([visible_query](flecs::entity entity, Camera& camera, GlobalTransform& transform, RenderLayers& camera_render_layers, VisibleEntities& visible_entities) {
			visible_entities.entities.clear();

			if (camera.skip_visible_check) {
				return;
			}

			Aabb camera_aabb{
				.min = glm::vec2(transform.translation),
				.max = glm::vec2(transform.translation) + camera.viewport,
			};

			visible_query.each([&](flecs::entity entity, Aabb& other_aabb, Visible2d& visible, RenderLayers& render_layers) {
				if (!visible.value) {
					visible.value = camera_aabb.is_intersect(other_aabb);
				}

				if (camera_aabb.is_intersect(other_aabb) && camera_render_layers.intersects(render_layers)) {
					visible_entities.entities.emplace(entity);
				}
			});
		});

	//world.system<Application, Camera>()
	//	.kind(Phases::Update)
	//	.each([](Application& app, Camera& c) {
	//		if (!app.window_resized) {
	//			return;
	//		}

	//		int width;
	//		int height;

	//		SDL_GetWindowSize(app.window, &width, &height);

	//		c.projection = glm::ortho(0.f, static_cast<float>(width), static_cast<float>(height), 0.f, -1.f, 1.f);
	//	});

	//world.system<Window, Camera, Transform>()
	//	.term_at(0).singleton()
	//	.kind(Phases::Update)
	//	.each([](Window& w, Camera& c, Transform& t) {
	//		g_camera.target = Vector2{ t.translation.x, t.translation.y };
	//		g_camera.zoom   = t.scale.x;
	//		g_camera.offset = Vector2{ c.offset.x + w.width * 0.5f, c.offset.y + w.height * 0.5f };
	//	});
}
