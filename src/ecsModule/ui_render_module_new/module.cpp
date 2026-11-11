#include "module.h"

#include "components.h"
#include "ecsModule/common.h"
#include "ecsModule/text_render_module/module.h"
#include "ecsModule/ui_module/module.h"
#include "ecsModule/textModule/module.h"
#include "font.h"

using namespace se;

UiRenderModule::UiRenderModule(flecs::world& world) {
	world.module<UiRenderModule>();

	world.import<UiModule>();
	world.import<TextRenderModule>();

	text_engine = world.get<TextRenderModule>().engine;

	world.system<Node, SizeStrategy, Text, TextComputed, TextData, TextFont>("init text node")
		.with<InitText>()
		.kind(Phases::Update)
		.each([&](flecs::entity entity, Node& node, SizeStrategy& size_strategy, Text& text, TextComputed& computed, TextData& data, TextFont& font){
			if (!font.handle) {
				return;
			}

			const auto actual_text = [&] {
				if (!computed.computed_text.empty()) {
					return computed.computed_text;
				}

				return static_cast<std::string>(text);
			}();

			const auto calculate_min_size = [&] {
				size_t word_begin_index = 0;

				for (size_t i = 0; i < actual_text.size(); ++i) {
					if (actual_text[i] != ' ' && actual_text[i] != '\n'&& i != actual_text.size() - 1) {
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
				data.ttf_data = TTF_CreateText(text_engine, *font.handle, actual_text.c_str(), actual_text.size());

				calculate_min_size();
			}
			else {
				TTF_SetTextString(data.ttf_data, actual_text.c_str(), actual_text.size());

				calculate_min_size();
			}

			TTF_GetStringSize(font.handle->get_resource(), text.c_str(), text.length(), &data.size.x, &data.size.y);

			data.size = glm::vec2(data.size) * font.font_scale;
			data.alignment_offset = computed.alignment_offset;
			size_strategy.x = Grow{ .min = data.min_width };
			size_strategy.y = Grow{ .min = data.size.y };

			entity.remove<InitText>();
		});

	world.observer<Text, TextData>()
		.event(flecs::OnRemove)
		.each([](flecs::entity e, Text& text, TextData& data){
			TTF_DestroyText(data.ttf_data);
			data.size = { 0, 0 };
		});

	world.component<ExtractedNodes>();
	world.component<ExtractedTextNodes>();
}

glm::mat4 se::extract_ui_view(const Camera& camera, const GlobalTransform& transfrom) {
	return camera.projection;
}
