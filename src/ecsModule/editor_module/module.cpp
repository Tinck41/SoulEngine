#include "module.h"

#include "ecsModule/assetModule/module.h"
#include "ecsModule/common.h"
#include "ecsModule/editor_module/immediate.h"
#include "ecsModule/utils.h"
#include "ecsModule/windowModule/module.h"
#include "spdlog/spdlog.h"
#include "ecsModule/render_module/render_phase.h"
#include "ecsModule/render_module/module.h"

using namespace se;

void draw_children(flecs::entity entity, editor::Immediate& im) {
	entity.children([&](flecs::entity child) {
		if (im.tree_node(child.name().c_str())) {
			draw_children(child, im);

			im.tree_pop();
		}
	});
}

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

	auto query = world.query<RenderPhase>();

	world.system<editor::Immediate>()
		.kind(Phases::Update)
		.immediate()
		.each([&] (flecs::iter& it, size_t, editor::Immediate& im) {
			static auto img = it.world().get_mut<AssetStorage>().load_texture(*it.world().get_mut<RenderDevice>().gpu, "assets/colormap.png");
			static bool draw_test = false;
			static float test_value = 0.f;
			auto main_window = it.world().get<WindowModule>().main_window;
			glm::ivec2 size;
			SDL_GetWindowSize(main_window, &size.x, &size.y);
			im.push_style<editor::Immediate::StyleVars_Padding>(glm::vec4(0.f));
			im.begin(
				"dockspace",
				editor::Immediate::WindowFlags_NoResize    |
				editor::Immediate::WindowFlags_NoMove      |
				editor::Immediate::WindowFlags_NoTitlebar  |
				editor::Immediate::WindowFlags_NoClose     |
				editor::Immediate::WindowFlags_NoCollasee  | 
				editor::Immediate::WindowFlags_FitViewport | 
				editor::Immediate::WindowFlags_NoDocking   |
				editor::Immediate::WindowFlags_NoScrollbar |
				editor::Immediate::WindowFlags_FixedFocus, 
				{}, size
			);
			im.pop_style();
			im.dockspace("123dockspace_dockspace", glm::vec2(0, 0));
			im.end();
			im.begin("test");

			if (im.button("test_button")) {
				draw_test = !draw_test;
			}

			if (draw_test) {
				world.query<RenderPhase>().run([&](flecs::iter& it) {
					while (it.next()) {
						for (auto i : it) {
							const auto entity = it.entity(i);

							if (im.tree_node(entity.name().c_str())) {
								draw_children(entity, im);

								im.tree_pop();
							}
						}
					}
				});
			}

			im.end();

			im.begin("test_dock");

			//im.dockspace("dockspace_test_dock", glm::vec2(0, 0));
			im.text("text1", "just to check");

			im.end();
			im.begin("test_3");
			im.end();
			im.begin("test_5", editor::Immediate::WindowFlags_NoResize, glm::vec2(100.f), glm::vec2(200.f));
			im.test_widget();
			if (im.begin_group("img_group")) {
				im.image("image", img);
				im.end_group();
			}

			im.end();
		});

	world.system<editor::Immediate>()
		.kind(Phases::Update)
		.immediate()
		.each([] (flecs::iter& it, size_t, editor::Immediate& im) {
			im.end_frame();
		});

	world.add<editor::Immediate>();
}
