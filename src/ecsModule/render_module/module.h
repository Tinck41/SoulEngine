#pragma once

#include "ecsModule/render_module/render_phase.h"
#include "flecs.h"
#include "SDL3/SDL.h"
#include "rect.h"

#include <memory>

namespace se {
	struct RenderStats {
		int draw_calls = 0;
	};

	struct RenderDevice {
		SDL_GPUDevice* gpu;
	};

	struct WhiteTexture {
		std::shared_ptr<Texture> texture;
	};

	struct RenderModule {
		RenderModule(flecs::world& world);
	};

	struct ClipContent : public RectI {
		ClipContent(int x, int y, int w, int h) : RectI{ .min = glm::vec2(x, y), .max = glm::vec2(x + w, y + h) } {}
		ClipContent(RectI rect) : RectI(rect) {}

		operator RectF() const {
			return {
				.min = min,
				.max = max,
			};
		}

		ClipContent() = default;
	};

	RenderPhaseSorter create_default_sorter();
	RenderPhaseRenderer create_default_renderer();
}
