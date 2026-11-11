#include "module.h"

#include "ecsModule/common.h"
#include "ecsModule/editor_module/immediate.h"
#include "ecsModule/windowModule/module.h"
#include "spdlog/spdlog.h"

using namespace se;

EditorModule::EditorModule(flecs::world& world) {
	world.module<EditorModule>();

	world.component<editor::Immediate>()
		.member(&editor::Immediate::clear_on_frame_end, "clear")
		.add(flecs::Singleton);

	world.system<editor::Immediate>()
		.kind(Phases::OnStart)
		.immediate()
		.each([&] (editor::Immediate& im) {
			im.init(world);
		});

	world.system<editor::Immediate>()
		.kind(Phases::PreUpdate)
		.immediate()
		.each([&] (editor::Immediate& im) {
			im.begin_frame();
		});

	world.system<editor::Immediate>()
		.kind(Phases::Update)
		.immediate()
		.each([] (flecs::iter& it, size_t, editor::Immediate& im) {
			auto main_window = it.world().get<WindowModule>().main_window;
			glm::ivec2 size;
			SDL_GetWindowSize(main_window, &size.x, &size.y);
			it.world().defer_suspend();
			im.begin(
				"dockspace",
				editor::Immediate::WindowFlags_NoResize    |
				editor::Immediate::WindowFlags_NoMove      |
				editor::Immediate::WindowFlags_NoTitlebar  |
				editor::Immediate::WindowFlags_NoClose     |
				editor::Immediate::WindowFlags_NoCollasee  | 
				editor::Immediate::WindowFlags_FitViewport | 
				editor::Immediate::WindowFlags_NoDocking   |
				editor::Immediate::WindowFlags_FixedFocus, 
				{}, size
			);
			im.dockspace("dockspace_dockspace", glm::vec2(0, 0));
			im.end();
			im.begin("test");

			if (im.button("test_button")) {
				spdlog::info("pressed");
			}

			im.end();

			im.begin("test_dock");

			//im.dockspace("dockspace_test_dock", glm::vec2(0, 0));
			im.text("just to check");

			im.end();
			im.begin("test_3");
			im.end();
			im.begin("test_5");
			im.end();
			it.world().defer_resume();
		});

	world.system<editor::Immediate>()
		.kind(Phases::Update)
		.immediate()
		.each([] (flecs::iter& it, size_t, editor::Immediate& im) {
			it.world().defer_suspend();
			im.end_frame();
			it.world().defer_resume();
		});

	world.add<editor::Immediate>();
}
