#include "text_helpers.h"

#include "components.h"
#include "ecsModule/render_module/module.h"
#include "ecsModule/ui_render_module_new/offsets.h"
#include "font.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "ecsModule/ui_module/module.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/textModule/module.h"
#include "ecsModule/render_module/material.h"
#include "ecsModule/text_render_module/text_vertex.h"

using namespace se;

#define TEXT_MAX_VERTEX_COUNT 4000
#define TEXT_MAX_INDEX_COUNT  6000

RenderPhaseExtractor se::create_text_node_extractor(flecs::world& world, flecs::entity_t helper) {
	auto text_query = world.query_builder()
		.with<const Node>()
		.with<const NodeIndex>()
		.with<const Text>()
		.with<const TextFont>()
		.with<const TextColor>()
		.with<const TextOutline>().optional()
		.with<const TextShadow>().optional()
		.with<const TextData>()
		.with<const GlobalTransform>()
		.with<const Aabb>()
		.with<const Material>().optional()
		.with<const ClipContent>().optional()
		.with<ExtractedTextNodes>().src("$helper").inout()
		.with<RenderPhase>().src("$phase_entity").inout()
		.with<Aabb>().src("$camera").inout()
		.build();

	return {
		.callback = [](flecs::iter& it) {
			auto helper = it.get_var("helper");

			while (it.next()) {
				auto node_field = it.field<const Node>(0);
				auto stack_index_field = it.field<const NodeIndex>(1);
				auto text_field = it.field<const Text>(2);
				auto font_field = it.field<const TextFont>(3);
				auto color_field = it.field<const TextColor>(4);
				auto outline_field = it.field<const TextOutline>(5);
				auto shadow_field = it.field<const TextShadow>(6);
				auto data_field = it.field<const TextData>(7);
				auto transform_field = it.field<const GlobalTransform>(8);
				auto aabb_field = it.field<const Aabb>(9);

				auto& extracted_texts = it.field<ExtractedTextNodes>(12)[0];
				auto& render_phase = it.field<RenderPhase>(13)[0];
				auto& camera_aabb = it.field<Aabb>(14)[0];

				for (auto i : it) {
					if (!camera_aabb.is_intersect(aabb_field[i]) || !node_field[i].display) {
						continue;
					}

					const auto entity = it.entity(i);

					const auto& node = node_field[i];
					const auto& stack_index = stack_index_field[i].dfs;
					const auto& text = text_field[i];
					const auto& font = font_field[i];
					const auto& color = color_field[i];
					const auto& data = data_field[i];
					const auto& transform = transform_field[i];

					const auto material_id = it.is_set(10) ? entity : helper;
					const auto scissor = it.is_set(11) ? it.field<const ClipContent>(11)[i] : std::optional<ClipContent>{};
					const auto [outline_width, outline_color] = [&] {
						if (it.is_set(5)) {
							const auto& outline_data = outline_field[i];

							return std::pair{ outline_data.width, outline_data.color };
						}

						return std::pair{ 0.f, TRANSPARENT };
					}();

					auto seq = TTF_GetGPUTextDrawData(data.ttf_data);

					SDL_GPUTexture* last_texture = nullptr;

					while (seq) {
						if (last_texture == seq->atlas_texture) {
							seq = seq->next;

							continue;
						}

						if (it.is_set(6)) {
							render_phase.items.emplace_back(entity, helper, material_id,extracted_texts.size(), stack_index + node_offsets::TEXT_SHADOW, scissor);

							extracted_texts.emplace_back(
								entity,
								material_id,
								seq->atlas_texture,
								transform.translation + glm::vec3(data.alignment_offset, 0.f) + glm::vec3(shadow_field[i].shift, 0.f),
								transform.scale,
								font.font_scale,
								shadow_field[i].color,
								0,
								TRANSPARENT,
								seq,
								scissor
							);
						}

						render_phase.items.emplace_back(entity, helper, material_id,extracted_texts.size(), stack_index + node_offsets::TEXT, scissor);

						extracted_texts.emplace_back(
							entity,
							material_id,
							seq->atlas_texture,
							transform.translation + glm::vec3(data.alignment_offset, 0.f),
							transform.scale,
							font.font_scale,
							color,
							outline_width,
							outline_color,
							seq,
							scissor
						);

						last_texture = seq->atlas_texture;
						seq = seq->next;
					}
				}
			}
		},
		.query = text_query,
		.helper = helper,
	};
}

RenderPhaseUploader se::create_text_node_uploader() {
	return {
		.callback = [](flecs::world& world, flecs::entity& render_phase, flecs::entity& uploader, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass) {
			auto& extracted_texts = uploader.get_mut<ExtractedTextNodes>();
			auto& context = uploader.get_mut<PhaseContext>();
			auto& phase_items = render_phase.get_mut<RenderPhase>().items;

			auto transfer_buffer = SDL_MapGPUTransferBuffer(device, context.transfer_buffer, false);

			auto vertices = static_cast<TextVertex*>(transfer_buffer);
			auto indices = reinterpret_cast<int*>(vertices + TEXT_MAX_VERTEX_COUNT);

			auto vertex_count = 0;
			auto index_count = 0;

			size_t batch_index = 0;

			for (size_t i = 0; i < phase_items.size(); ++i) {
				auto& data = phase_items[i];

				if (data.context_entity != uploader || data.extracted_index >= extracted_texts.size() || extracted_texts[data.extracted_index].entity != data.entity) {
					batch_index = i + 1;

					continue;
				}

				const auto& text_data = extracted_texts[data.extracted_index];

				auto seq = text_data.seq;

				if (!seq) {
					batch_index = i + 1;

					continue;
				}

				if (phase_items[batch_index].textures.empty() || phase_items[batch_index].textures[0] != seq->atlas_texture || phase_items[batch_index].scissor != text_data.scissor) {
					batch_index = i;

					data.textures.emplace_back(seq->atlas_texture);
					data.num_instances = 1;
					data.first_index = index_count;
				}

				while (seq) {
					if (seq->atlas_texture != phase_items[batch_index].textures[0]) {
						batch_index = i + 1;

						break;
					}

					for (int j = 0; j < seq->num_vertices; j++) {
						const auto pos = seq->xy[j];
						const auto uv = seq->uv[j];

						vertices[vertex_count + j] = TextVertex{
							.position{ text_data.translation + glm::vec2{ pos.x, -pos.y } * text_data.scale * text_data.font_scale, 0.f },
							.color = text_data.color,
							.uv{ uv.x, uv.y },
							.outline_width = text_data.outline_width,
							.outline_color = text_data.outline_color,
						};
					}

					for (int j = 0; j < seq->num_indices; j++) {
						indices[index_count + j] = seq->indices[j] + vertex_count;
					}

					vertex_count += seq->num_vertices;
					index_count += seq->num_indices;

					phase_items[batch_index].num_indices += seq->num_indices;

					seq = seq->next;
				}

				++phase_items[batch_index].batch_size;
			}

			extracted_texts.clear();

			SDL_UnmapGPUTransferBuffer(device, context.transfer_buffer);

			if (vertex_count == 0 || index_count == 0) {
				return;
			}

			SDL_GPUTransferBufferLocation vertex_transfer_buffer_location{
				.transfer_buffer = context.transfer_buffer,
				.offset = 0 
			};
			SDL_GPUBufferRegion vertex_buffer_location{
				.buffer = context.vertex_buffer,
				.offset = 0,
				.size = static_cast<Uint32>(sizeof(TextVertex) * vertex_count)
			};

			SDL_UploadToGPUBuffer(copy_pass, &vertex_transfer_buffer_location, &vertex_buffer_location, false);

			SDL_GPUTransferBufferLocation index_transfer_buffer_location {
				.transfer_buffer = context.transfer_buffer,
				.offset = static_cast<Uint32>(sizeof(TextVertex) * TEXT_MAX_VERTEX_COUNT)
			};
			SDL_GPUBufferRegion index_buffer_region {
				.buffer = context.index_buffer,
				.offset = 0,
				.size = static_cast<Uint32>(sizeof(int) * index_count)
			};

			SDL_UploadToGPUBuffer(copy_pass, &index_transfer_buffer_location, &index_buffer_region, false);
		}
	};
}
