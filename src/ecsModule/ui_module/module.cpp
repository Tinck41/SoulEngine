#include "module.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/common.h"
#include "ecsModule/inputModule/module.h"
#include "ecsModule/render_module/module.h"
#include "ecsModule/utils.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/textModule/module.h"
#include "ecsModule/windowModule/module.h"
#include "font.h"
#include "layout.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <vector>

using namespace se;

void update_clipping(flecs::entity entity, const GlobalTransform& transform, const Node& node, std::optional<RectI> parent_clip) {
	if (!node.display) {
		parent_clip = RectI{
			.min = glm::vec2(0),
			.max = glm::vec2(0),
		};

		entity.set<ClipContent>(parent_clip.value());
	}
	else if (node.overflow.first == Node::Overflow::Scroll && node.overflow.second == Node::Overflow::Scroll || node.overflow.first == Node::Overflow::Clip && node.overflow.second == Node::Overflow::Clip) {
		const auto new_clip = RectI{
			.min = glm::floor(glm::vec2(transform.translation)),
			.max = glm::ceil(glm::vec2(transform.translation) + node.size)
		};

		if (parent_clip) {
			parent_clip = RectI::clamp(new_clip, parent_clip.value());
		}
		else {
			parent_clip = new_clip;
		}

		entity.set<ClipContent>(parent_clip.value());
	}
	else if (parent_clip) {
		entity.set<ClipContent>(parent_clip.value());
	}

	entity.children([&parent_clip](flecs::entity child) {
		if (!child.is_alive() || !child.has<GlobalTransform>() || !child.has<Node>()) {
			return;
		}

		const auto name = child.name();
		const auto children_num = utils::get_children_count(child);

		update_clipping(child, child.get<GlobalTransform>(), child.get<Node>(), parent_clip);
	});
}

UiModule::UiModule(flecs::world& world) {
	world.module<UiModule>();

	world.import<TransformModule>();
	world.import<InputModule>();
	world.import<TextModule>();
	world.import<CameraModule>();

	world.component<InsertBefore>()
		.add(flecs::Relationship);

	world.component<LayoutComposer>()
		.add(flecs::Singleton);

	world.component<UiTreeChanged>()
		.add(flecs::Singleton);

	world.component<CustomNodeIndex>()
		.member<size_t>("value");

	world.component<BackgroundColor>()
		.member<unsigned char>("r")
		.member<unsigned char>("g")
		.member<unsigned char>("b")
		.member<unsigned char>("a")
		.is_a<Color>();

	world.component<BorderColor>()
		.member<unsigned char>("r")
		.member<unsigned char>("g")
		.member<unsigned char>("b")
		.member<unsigned char>("a")
		.is_a<Color>();

	world.component<Grow>()
		.member("min", &Grow::min)
		.member("max", &Grow::max);

	world.component<Fit>()
		.member("min", &Fit::min)
		.member("max", &Fit::max);

	world.component<NodeIndex>()
		.member("dfs", &NodeIndex::dfs)
		.member("bfs", &NodeIndex::bfs);

	world.component<Fixed>()
		.member("value", &Fixed::value);

	world.component<SizeStrategy::Strategy>();

	world.component<SizeStrategy>()
		.member("x", &SizeStrategy::x)
		.member("y", &SizeStrategy::y);

	world.component<GrowDirection>()
		.constant("Horizontal", GrowDirection::Horizontal)
		.constant("Vertical", GrowDirection::Vertical)
		.add(flecs::DontFragment)
		.add(flecs::Exclusive);

	world.component<Node>()
		.member("self_alignment", &Node::self_alignment)
		.member("size", &Node::size)
		.member("child_alignment", &Node::child_alignment)
		.member("child_gap", &Node::child_gap)
		.member("margin", &Node::margin)
		.member("padding", &Node::padding)
		.member("display", &Node::display)
		.member("absolute", &Node::absolute)
		.member("border_radius", &Node::border_radius)
		.member("border_width", &Node::border_width)
		.add(flecs::With, world.component<SizeStrategy>())
		.add(flecs::With, world.component<NodeIndex>())
		.add(flecs::With, world.component<RenderLayers>())
		.add(flecs::With, world.component<Aabb>())
		.add(flecs::With, world.component<Visible2d>())
		.add(flecs::With, world.component<Transform>())
		.add(flecs::With, world.component<GrowDirection>())
		.add(flecs::With, flecs::OrderedChildren);

	world.component<Image>()
		.add(flecs::With, world.component<Node>());

	world.component<Text>()
		.opaque([](flecs::world&) {
			flecs::opaque<std::string> ts;

			ts.as_type(flecs::String);

			ts.serialize([](const flecs::serializer *s, const std::string *data) {
				const char *value = data->c_str();
				return s->value(flecs::String, &value);
			});

			ts.assign_string([](std::string *data, const char *value) {
				*data = value;
			});

			return ts;
		})
		.add(flecs::With, world.component<TextData>())
		.add(flecs::With, world.component<TextFont>())
		.add(flecs::With, world.component<TextColor>())
		.add(flecs::With, world.component<Node>());

	world.component<Interaction>()
		.constant("None", Interaction::None)
		.constant("Hovered", Interaction::Hovered)
		.constant("Clicked", Interaction::Clicked)
		.add(flecs::Exclusive)
		.add(flecs::Sparse); //TODO: think about it

	world.component<FocusStrategy>()
		.constant("Block", FocusStrategy::Block)
		.constant("Pass", FocusStrategy::Pass)
		.add(flecs::Exclusive)
		.add(flecs::Sparse); //TODO: think about it

	world.component<Button>()
		.add(flecs::With, world.component<Interaction>())
		.add(flecs::With, world.component<FocusStrategy>())
		.add(flecs::With, world.component<Node>());

	world.component<Composite>()
		.add(flecs::With, world.component<Image>());

	world.observer<InsertBefore>()
		.term_at(0).second(flecs::Wildcard)
		.event(flecs::OnAdd)
		.each([](flecs::iter& it, size_t i, InsertBefore) {
			const auto entity = it.entity(i);
			const auto target = it.pair(0).second();
			const auto parent = target.parent();

			utils::insert_child_before(parent, entity, target);
		});

	world.observer<CustomNodeIndex>()
		.event(flecs::OnSet)
		.with(flecs::ChildOf, flecs::Wildcard)
		.with<Node>()
		.each([&world](flecs::entity child, CustomNodeIndex& customIndex) {
			auto parent = child.parent();
			auto children = utils::get_children(parent);

			std::erase(children, child);
			children.insert(children.begin() + customIndex.value, child);
			parent.set_child_order(children.data(), children.size());

			child.remove<CustomNodeIndex>(); // TODO: is it right?
			world.add<UiTreeChanged>();
		});

	world.observer()
		.with(flecs::Disabled)
		.with<Node>().filter()
		.event(flecs::OnAdd)
		.event(flecs::OnRemove)
		.each([&world](flecs::iter& it, size_t i) {
			it.entity(i).children([&it](flecs::entity child) {
				it.event() == flecs::OnAdd ? child.disable() : child.enable();
			});
			world.add<UiTreeChanged>();
		});

	world.observer<Node>()
		.event(flecs::OnAdd)
		.event(flecs::OnRemove)
		.with(flecs::ChildOf, flecs::Wildcard)
		.each([&world](flecs::entity child, Node& node) {
			world.add<UiTreeChanged>();
		});

	world.observer<RenderLayers, Node>()
		.term_at(1).filter()
		.event(flecs::OnAdd)
		.each([&world](RenderLayers& render_layers, Node& node) {
			render_layers.mask = RenderLayers::UI;
		});

	world.system<NodeIndex>()
		.with<UiTreeChanged>()
		.without<NodeIndex>().parent()
		//.without(flecs::ChildOf, flecs::Wildcard) // TODO: SceneRoot in future
		.kind(Phases::Update)
		.run([&world](flecs::iter& it) {
			size_t dfs_index = 0;
			size_t bfs_index = 0;

			std::vector<flecs::entity> dfs_roots;
			std::vector<flecs::entity> bfs_roots;

			while (it.next()) {
				const auto index_field = it.field<NodeIndex>(0);

				for (auto i : it) {
					const auto entity = it.entity(i);

					if (!index_field[i].external_dfs_source) {
						dfs_roots.push_back(it.entity(i));
					}
					if (!index_field[i].external_bfs_source) {
						bfs_roots.push_back(it.entity(i));
					}

				}
			}
			utils::dfs(dfs_roots, [&dfs_index](flecs::entity child) {
				if (!child.enabled()) {
					return;
				}

				if (!child.has<NodeIndex>()) {
					spdlog::error("child {} doesn't have NodeIndex!", child.name());
					return;
				}

				child.get_mut<NodeIndex>().dfs = dfs_index++;
			});

			utils::bfs(bfs_roots, [&bfs_index](flecs::entity child) {
				if (!child.enabled()) {
					return;
				}

				child.get_mut<NodeIndex>().bfs = bfs_index++;
			});

			world.remove<UiTreeChanged>();
		});

	auto interactions_query = world.query_builder<const Node, const GlobalTransform, const NodeIndex, const FocusStrategy, Interaction>()
		.with<const ClipContent>().optional()
		.build();

	world.system<Input>("focus interaction new")
		.kind(Phases::Update)
		.each([interactions_query](Input& input) {
			std::vector<TempInteraction> interactions;

			interactions_query.run([&input, &interactions](flecs::iter& it) {
				while(it.next()) {
					const auto node_field = it.field<const Node>(0);
					const auto transform_field = it.field<const GlobalTransform>(1);
					const auto stack_index_field = it.field<const NodeIndex>(2);

					for (auto i : it) {
						const auto& node = node_field[i];
						const auto& transform = transform_field[i];
						const auto& stack_index = stack_index_field[i];
						const auto& focus_strategy = it.field_at<const FocusStrategy>(3, i);
						auto& interaction = it.field_at<Interaction>(4, i);

						interaction = Interaction::None;

						const auto& mouse_pos = input.mouse.position;
						const auto& pos = transform.translation;
						const auto& size = node.size;

						if (mouse_pos.x >= pos.x && mouse_pos.x <= (pos.x + size.x) && mouse_pos.y >= pos.y && mouse_pos.y <= (pos.y + size.y)) {
							if (it.is_set(5)) {
								auto &clip_content = it.field_at<const ClipContent>(5, i);

								if (clip_content.get_width() == 0 && clip_content.get_height() == 0) {
									continue;
								}
							}

							interactions.emplace_back(it.entity(i), stack_index.dfs, focus_strategy);
						}
					}
				}
			});

			if (interactions.empty()) {
				return;
			}

			std::ranges::sort(interactions, std::greater{}, &TempInteraction::stack_index);

			for (const auto& interaction : interactions) {
				interaction.entity.get_mut<Interaction>() = input.mouse.left.pressed ? Interaction::Clicked : Interaction::Hovered;

				if (interaction.focus_strategy == FocusStrategy::Block) {
					break;
				}
			}
		});

	world.system<LayoutComposer>()
		.kind(Phases::Update)
		.each([](LayoutComposer& composer) {
			composer.clear();
		});

	world.system<const NodeIndex*, const Node, const SizeStrategy, const GrowDirection, const NodeIndex, const Transform>("layout collect")
		.term_at(0).parent()
		.kind(Phases::PostUpdate)
		.run([&world](flecs::iter& it) {
			auto& composer = world.get_mut<LayoutComposer>();

			while (it.next()) {
				const auto node_field = it.field<const Node>(1);
				const auto size_field = it.field<const SizeStrategy>(2);
				const auto index_field = it.field<const NodeIndex>(4);
				const auto transform_field = it.field<const Transform>(5);

				for (auto i : it) {
					const auto entity = it.entity(i);

					const auto& node = node_field[i];
					const auto& size_strategy = size_field[i];
					const auto& grow_direction = it.field_at<const GrowDirection>(3, i);
					const auto& index = index_field[i];
					const auto& transform = transform_field[i];

					// TODO: shoud we realy skip those items
					if (!node.display) {
						continue;
					}

					const auto size = [&] {
						glm::vec2 size = {};

						if (std::holds_alternative<Fixed>(size_strategy.x)) {
							size.x = std::get<Fixed>(size_strategy.x).value;
						}

						if (std::holds_alternative<Fixed>(size_strategy.y)) {
							size.y = std::get<Fixed>(size_strategy.y).value;
						}

						return size;
					}();

					const auto min_size = [&] {
						std::pair<std::optional<float>, std::optional<float>> size;

						if (std::holds_alternative<Fit>(size_strategy.x)) {
							size.first = std::get<Fit>(size_strategy.x).min;
						}
						else if (std::holds_alternative<Grow>(size_strategy.x)) {
							size.first = std::get<Grow>(size_strategy.x).min;
						}

						if (std::holds_alternative<Fit>(size_strategy.y)) {
							size.second = std::get<Fit>(size_strategy.y).min;
						}
						else if (std::holds_alternative<Grow>(size_strategy.y)) {
							size.second = std::get<Grow>(size_strategy.y).min;
						}

						return size;
					}();

					const auto max_size = [&] {
						std::pair<std::optional<float>, std::optional<float>> size;

						if (std::holds_alternative<Fit>(size_strategy.x)) {
							size.first = std::get<Fit>(size_strategy.x).max;
						}
						else if (std::holds_alternative<Grow>(size_strategy.x)) {
							size.first = std::get<Grow>(size_strategy.x).max;
						}

						if (std::holds_alternative<Fit>(size_strategy.y)) {
							size.second = std::get<Fit>(size_strategy.y).max;
						}
						else if (std::holds_alternative<Grow>(size_strategy.y)) {
							size.second = std::get<Grow>(size_strategy.y).max;
						}

						return size;
					}();

					auto layout_node = LayoutNode{
						.parent_bfs_index = (it.is_set(0) ? it.field_at<const NodeIndex>(0, 0).bfs : index.bfs) + 1,
						.bfs_index = index.bfs + 1,
						.horizontal = grow_direction == GrowDirection::Horizontal,
						.display = node.display,
						.absolute = node.absolute,
						.size = size,
						.child_alignment = node.child_alignment,
						.pos = transform.translation,
						.child_gap = node.child_gap,
						.padding = node.padding,
						.fixed = { std::holds_alternative<Fixed>(size_strategy.x), std::holds_alternative<Fixed>(size_strategy.y) },
						.grow = { std::holds_alternative<Grow>(size_strategy.x), std::holds_alternative<Grow>(size_strategy.y) },
						.fit = { std::holds_alternative<Fit>(size_strategy.x), std::holds_alternative<Fit>(size_strategy.y) },
						.self_alignment = node.self_alignment,
						.min_size = min_size,
						.max_size = max_size
					};

					if (node.overflow.first == Node::Overflow::Scroll) {
						layout_node.scroll.first = node.scroll_offset.first.value_or(0.f);
					}

					if (node.overflow.second == Node::Overflow::Scroll) {
						layout_node.scroll.second = node.scroll_offset.second.value_or(0.f);
					}

					composer.push_node(std::move(layout_node));
				}
			}
		});

	world.system<NodeIndex, Text, TextFont, TextData, LayoutComposer>()
		.with<Node>()
		.kind(Phases::PostUpdate)
		.each([](NodeIndex& index, Text& text, TextFont& text_font, TextData& text_data, LayoutComposer& composer) {
			composer.set_text(index.bfs + 1, {
				.text = text,
				.font = text_font.handle->get_resource(),
				.font_scale = text_font.font_scale,
				.text_size = text_data.size,
				.alignment = text_font.alignment,
			});

			composer[index.bfs + 1].size = text_data.size;
		});

	world.system<LayoutComposer>()
		.kind(Phases::PostUpdate)
		.each([](LayoutComposer& composer) {
			composer.build();
		});

	world.system<Node, NodeIndex, Transform, const LayoutComposer>("apply layout")
		.kind(Phases::PostUpdate)
		.each([](flecs::entity entity, Node& node, NodeIndex& index, Transform& transform, const LayoutComposer& composer) {
			const auto& layout = composer[index.bfs + 1];

			if (layout.text_data) {
				entity.set<TextComputed>({
					.computed_text = layout.text_data.value().text,
					.alignment_offset = layout.text_data.value().alignment_offset,
				});
			}

			transform.translation = glm::vec3(layout.pos, transform.translation.z);
			node.size = layout.size;
		});

	world.system<GlobalTransform, Image, Aabb>()
		.kind(Phases::PostUpdate)
		.each([](GlobalTransform& transform, Image& image, Aabb& aabb) {
			aabb.min = transform.translation;
			aabb.max = glm::vec2(transform.translation) + image.texture->get_size();
		});

	world.system<GlobalTransform, Node, Aabb>()
		.with<BackgroundColor>()
		.without<Image>()
		.without<Text>()
		.kind(Phases::PostUpdate)
		.each([](GlobalTransform& transform, Node& node, Aabb& aabb) {
			aabb.min = transform.translation;
			aabb.max = glm::vec2(transform.translation) + node.size;
		});

	world.system<GlobalTransform, TextData, Aabb>()
		.kind(Phases::PostUpdate)
		.each([](GlobalTransform& transform, TextData& text_data, Aabb& aabb) {
			aabb.min = transform.translation;
			aabb.max = glm::vec2(transform.translation) + glm::vec2(text_data.size);
		});

	world.system<Node, GlobalTransform>()
		.without<Node>().parent()
		.kind(Phases::PostUpdate)
		.each([](flecs::entity entity, Node& node, GlobalTransform& transform) {
			update_clipping(entity, transform, node, std::nullopt);
		});

	world.system<Node, Input>()
		.without<Node>().parent()
		.kind(Phases::PostUpdate)
		.each([](flecs::entity entity, Node& node, Input& input) {

			if (input.keys[Key::F1].released) {
				spdlog::info("              [Tree]           ");
				utils::dfs(entity, 0,  [&](flecs::entity e, const size_t depth) {
					const auto global_transform = e.get<GlobalTransform>();
					const auto local_transform = e.get<Transform>();

					spdlog::info("{}[{}]: global [{}:{}] local [{}:{}]", std::string(depth * 2, ' '), e.name(), global_transform.translation.x, global_transform.translation.y, local_transform.translation.x, local_transform.translation.y);
				});
			}
		});

	world.add<LayoutComposer>();
}
