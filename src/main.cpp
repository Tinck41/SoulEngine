#include "core/application.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/default_modules.h"
#include "ecsModule/editor_module/module.h"
#include "ecsModule/inputModule/module.h"
#include "ecsModule/textModule/module.h"
#include "ecsModule/ui_render_module_new/components.h"
#include "ecsModule/ui_render_module_new/module.h"
#include "ecsModule/windowModule/module.h"
#include "ecsModule/spriteModule/module.h"
#include "ecsModule/assetModule/module.h"
#include "ecsModule/common.h"
#include "ecsModule/utils.h"
#include "ecsModule/ui_module/module.h"
#include "ecsModule/ui_render_module_new/text_helpers.h"
#include "ecsModule/ui_render_module_new/node_helpers.h"
#include "ecsModule/windowModule/components.h"
#include "spdlog/spdlog.h"
#include "texture_atlas.h"
#include <algorithm>
#include "ecsModule/render_module/module.h"
#include "ecsModule/sprite_render_module/module.h"
#include "ecsModule/text_render_module/module.h"
#include "utils/sdl.h"
#include "game_module.h"
#include "tiny_obj_loader.h"
#include "ecsModule/mesh3d_module/module.h"
#include "arena.h"

namespace se {
	struct CameraController {
		float speed = 0;
	};
}

void default_init(flecs::world& world) {
	using namespace se;

	world.component<se::CameraController>()
		.member("speed", &se::CameraController::speed);

	auto device = world.get_ref<RenderDevice>();
	const auto window = world.get<WindowModule>().main_window;

	glm::ivec2 window_size;

	SDL_GetWindowSize(window, &window_size.x, &window_size.y);

	auto depth_texture_create_info = SDL_GPUTextureCreateInfo{
		.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
		.width = static_cast<Uint32>(window_size.x),
		.height = static_cast<Uint32>(window_size.y),
		.layer_count_or_depth = 1,
		.num_levels = 1,
	};

	auto dept_texture = SDL_CreateGPUTexture(device->gpu, &depth_texture_create_info);

	auto camera = world.entity("camera")
		.add<CameraController>()
		.set<se::Transform>({ .translation = { 0.f, 0.f, 4.f } })
		.set<se::RenderLayers>({ RenderLayers::layer(2) | RenderLayers::layer(1) })
		.set<se::Camera>({
			.depth_texture = dept_texture,
		});

	{
		auto transparent_3d = world.entity("transparent_3d");
		auto transparent_2d = world.entity("transparent_2d");
		auto transparent_ui = world.entity("transparent_ui");

		{
			auto mesh3d_phase = world.entity("mesh3d_phase").child_of(transparent_3d);

			mesh3d_phase.set(create_mesh3d_material(world));
			mesh3d_phase.set(create_mesh3d_context(world));
			mesh3d_phase.set(create_mesh3d_extractor(world, mesh3d_phase));
			mesh3d_phase.set(create_mesh3d_uploader());

			mesh3d_phase.add<ExtractedMeshes3d>();
			mesh3d_phase.add<UploadedMeshes3d>();

			transparent_3d.set(create_default_sorter());
			transparent_3d.set(create_default_renderer());

			transparent_3d.set(RenderPhase{
				.extractors = { mesh3d_phase },
				.sorters = { transparent_3d },
				.uploaders = { mesh3d_phase },
				.renderers = { transparent_3d },
				.extract_view_callback = &extract_3d_view,
			});
		}

		{
			auto sprite_phase = world.entity("sprite_phase").child_of(transparent_2d);
			auto text_phase = world.entity("text_phase").child_of(transparent_2d);

			sprite_phase.set(create_sprite_material(world));
			sprite_phase.set(create_sprite_context(world));

			text_phase.set(create_text_material(world));
			text_phase.set(create_text_context(world));

			sprite_phase.set(create_sprite_extractor(world, sprite_phase));
			sprite_phase.set(create_sprite_uploader());

			text_phase.set(create_text_extractor(world, text_phase));
			text_phase.set(create_text_uploader());

			sprite_phase.add<ExtractedSprites>();
			text_phase.add<ExtractedText2ds>();

			transparent_2d.set(create_default_sorter());
			transparent_2d.set(create_default_renderer());

			transparent_2d.set(RenderPhase{
				.extractors = { sprite_phase, text_phase },
				.sorters = { transparent_2d },
				.uploaders = { sprite_phase, text_phase },
				.renderers = { transparent_2d },
			});
		}

		{
			auto node_helper = world.entity("node_helper").child_of(transparent_ui);
			auto text_node_helper = world.entity("text_node_helper").child_of(transparent_ui);
			auto image_extractor = world.entity("image_extractor").child_of(node_helper);
			auto color_extractor = world.entity("color_extractor").child_of(node_helper);

			node_helper.add<ExtractedNodes>();
			text_node_helper.add<ExtractedTextNodes>();

			node_helper.set(create_node_context_new(world));
			text_node_helper.set(create_text_context(world));

			transparent_ui.set(create_default_sorter());
			transparent_ui.set(create_default_renderer());

			node_helper.set(create_node_material_new(world));
			text_node_helper.set(create_text_material(world));

			image_extractor.set(create_image_node_extractor(world, node_helper));
			color_extractor.set(create_color_node_extractor(world, node_helper));
			text_node_helper.set(create_text_node_extractor(world, text_node_helper));

			node_helper.set(create_node_uploader_new());
			text_node_helper.set(create_text_node_uploader());

			transparent_ui.set(RenderPhase{
				.extractors = { image_extractor, color_extractor, text_node_helper },
				.sorters = { transparent_ui },
				.uploaders = { node_helper, text_node_helper },
				.renderers = { transparent_ui },
				.extract_view_callback = &extract_ui_view,
			});
		}

		camera.set(RenderPhases{ transparent_3d, transparent_2d, transparent_ui });
	}
}

void rotate(flecs::world& world, se::Transform& transform, se::Mesh3d& mesh, se::Mesh3dUniform& uniform) {
	transform.rotation += glm::vec3(0.f, 30.f, 0.f) * world.delta_time();
	uniform.model = transform.matrix;
}

void camera_controll(flecs::world& world, se::Input& input, se::Transform& transform, se::CameraController& controll) {
	if (input.keys[se::Key::A].remain) {
		transform.translation.x -= controll.speed * world.delta_time();
	}
	else if (input.keys[se::Key::D].remain) {
		transform.translation.x += controll.speed * world.delta_time();
	}
	else if (input.keys[se::Key::W].remain) {
		transform.translation.z -= controll.speed * world.delta_time();
	}
	else if (input.keys[se::Key::S].remain) {
		transform.translation.z += controll.speed * world.delta_time();
	}
}

int main() {
	se::Application::create()
		.add_module<se::DefaultModules>()
		.add_module<se::EditorModule>()
		.add_system(se::Phases::OnStart, &default_init)
		.add_system(se::Phases::Update, &rotate)
		.add_system(se::Phases::Update, &camera_controll)
		.add_module<se::GameModule>()
		.add_script("assets/scripts/sandbox.flecs")
		.run();
}

