#include "module.h"

#include "SDL3_image/SDL_image.h"
#include "ecsModule/common.h"
#include "spdlog/spdlog.h"

using namespace se;

#define FONT_BASE_SIZE 64

void AssetStorage::update() {
	auto it = textures.begin();

	while (it != textures.end()) {
		if (it->second.use_count() == 0) {
			it = textures.erase(it);
		} else {
			++it;
		}
	}
}

std::shared_ptr<Texture> AssetStorage::load_texture(SDL_GPUDevice& gpu, const std::string& path) {
	if (path.empty()) {
		return nullptr;
	}

	if (!textures.contains(path)) {
		auto copy_cmd_buf = SDL_AcquireGPUCommandBuffer(&gpu);
		auto copy_pass = SDL_BeginGPUCopyPass(copy_cmd_buf);

		glm::ivec2 size;

		auto texture = IMG_LoadGPUTexture(&gpu, copy_pass, path.c_str(), &size.x, &size.y);

		SDL_EndGPUCopyPass(copy_pass);
		assert(SDL_SubmitGPUCommandBuffer(copy_cmd_buf) && SDL_GetError());

		textures[path] = std::make_shared<Texture>(&gpu, texture, size);
	}

	return textures.at(path);
}

std::shared_ptr<Font> AssetStorage::load_font(const std::string& path) {
	return load_font(path, FONT_BASE_SIZE);
}

std::shared_ptr<Font> AssetStorage::load_font(const std::string& path, float size) {
	if (!fonts.contains(path)) {
		TTF_Font* resource = TTF_OpenFont(path.c_str(), size);

		if (!resource) {
			spdlog::error(SDL_GetError());

			return nullptr;
		}

		if (!TTF_SetFontSDF(resource, true)) { // TODO: world->get<AppConfig>().use_sdf;
			spdlog::info(SDL_GetError());
		}

		fonts[path] = std::make_shared<Font>(resource, path);
	}

	return fonts.at(path);
}

AssetModule::AssetModule(flecs::world& world) {
	world.module<AssetModule>();

	world.component<AssetStorage>().add(flecs::Singleton) ;

	world.system<AssetStorage>()
		.kind(Phases::Update)
		.each([](AssetStorage& storage) {
			storage.update();
		});

	world.add<AssetStorage>();
}

