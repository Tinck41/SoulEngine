#include "module.h"

#include "ecsModule/common.h"
#include "ecsModule/assetModule/module.h"
#include "ecsModule/textModule/module.h"
#include "ecsModule/ui_module/module.h"
#include "ecsModule/render_module/module.h"

using namespace se;

DebugModule::DebugModule(flecs::world& world) {
	world.module<DebugModule>();

	world.import<TextModule>();
	world.import<UiModule>();
	world.import<AssetModule>();

	world.component<FpsLabel>();
	world.component<DrawCallsLabel>();
	world.component<FpsUpdateTimer>()
		.member(&FpsUpdateTimer::update_rate, "update_rate");

	world.system<AssetStorage>("spawn fps label")
		.kind(Phases::OnStart)
		.each([&world](AssetStorage& storage) {
			world.entity("fps label")
				.add<FpsLabel>()
				.add<Text>()
				.set<FpsUpdateTimer>({ 0.f, 1.f })
				.set<TextColor>(Color::from_hex("00ff00"))
				.set<TextFont>({ storage.load_font("assets/FreeSans.ttf"), 32, 32 });

			world.entity("draw calls label")
				.set<Transform>({ .translation = {0.f, 30.f, 0.f } })
				.add<DrawCallsLabel>()
				.add<Text>()
				.set<TextColor>(Color::from_hex("00ff00"))
				.set<TextFont>({ storage.load_font("assets/FreeSans.ttf"), 32, 32 });
		});

	world.system<Text, FpsUpdateTimer>()
		.with<FpsLabel>()
		.kind(Phases::Update)
		.each([&world](flecs::entity entity, Text& text, FpsUpdateTimer& timer) {
			timer.elapsed_time += world.delta_time();

			if (timer.elapsed_time >= timer.update_rate) {
				const auto fps_value = static_cast<int>(1.f / world.delta_time());

				entity.set<Text>("FPS: " + std::to_string(fps_value));

				timer.elapsed_time = 0.f;
			}
		});

	world.system<Text, RenderStats>()
		.with<DrawCallsLabel>()
		.kind(Phases::Update)
		.each([&world](flecs::entity entity, Text& text, RenderStats& stats) {
			entity.set<Text>("draw calls: " + std::to_string(stats.draw_calls));
		});
}
