#pragma once

#include "SDL3/SDL_mouse.h"
#include "color.h"
#include "ecsModule/inputModule/module.h"
#include "glm.hpp"
#include "flecs.h"
#include "rect.h"
#include "texture.h"

#include <functional>
#include <optional>
#include <span>
#include <stack>
#include <string>
#include <variant>
#include <vector>

namespace se {
	class MouseState;
}

namespace se::editor {
	class Immediate {
	public:
		using HashId = std::uint64_t;

		using WindowFlags = int;
		using WidgetFlags = int;
		using ColorVars   = int;
		using StyleVars   = int;
		using DataTypes   = int;

		enum ColorVars_ {
			ColorVars_Bg              = 0,
			ColorVars_Fg              = 1,
			ColorVars_Text            = 2,
			ColorVars_Hot             = 3,
			ColorVars_Active          = 4,
			ColorVars_Inactive        = 5,
			ColorVars_Focus           = 6,
			ColorVars_Disabled        = 7,
			ColorVars_Border          = 8,
			ColorVars_WindowBg        = 9,
			ColorVars_WindowBorder    = 10,
			ColorVars_WindowHeaderBg  = 11,
			ColorVars_GroupBg         = 12,
			ColorVars_GroupHeaderBg   = 13,
			ColorVars_GroupContentBg  = 14,
			ColorVars_FieldBg         = 15,
			ColorVars_DockspaceBg     = 16,
			ColorVars_ComponentNameBg = 17,
			ColorVars_Size,
		};

		enum StyleVars_ {
			StyleVars_BorderWidth      = 0,
			StyleVars_FocusBorderWidth = 1,
			StyleVars_BorderRadius     = 2,
			StyleVars_Padding          = 3,
			StyleVars_WindowPadding    = 4,
			StyleVars_ContentPadding   = 5,
			StyleVars_Absolute         = 6,
			StyleVars_HeaderHeight     = 7,
			StyleVars_Size,
		};

		template<StyleVars V> struct StyleVarType;

		template<> struct StyleVarType<StyleVars_BorderWidth>      { using type = float; };
		template<> struct StyleVarType<StyleVars_FocusBorderWidth> { using type = float; };
		template<> struct StyleVarType<StyleVars_BorderRadius>     { using type = float; };
		template<> struct StyleVarType<StyleVars_Padding>          { using type = glm::vec4; };
		template<> struct StyleVarType<StyleVars_WindowPadding>    { using type = glm::vec4; };
		template<> struct StyleVarType<StyleVars_Absolute>         { using type = bool; };
		template<> struct StyleVarType<StyleVars_HeaderHeight>     { using type = float; };
		template<> struct StyleVarType<StyleVars_ContentPadding>   { using type = glm::vec4; };

		using StyleVarTypeVariant = std::variant<float, glm::vec4, bool>;

		template<typename V, typename T>
		struct StyleStackVarType {
			V var_index;
			T previous_value;
		};

		using ColorStackVar = StyleStackVarType<ColorVars, Color>;
		using StyleStackVar = StyleStackVarType<StyleVars, StyleVarTypeVariant>;

		struct Style {
			std::vector<Color> colors;
			std::vector<StyleVarTypeVariant> vars;
		};

		enum DataTypes_ {
			DataTypes_Float,
			DataTypes_Double,
		};

		struct DataTypeInfo {
			double (*load)(const void*);
			void   (*store)(void*, double);
		};

		static constexpr std::array<DataTypeInfo, 2> data_type_info = {{
			// DataTypes_Float
			{
				.load  = [](const void* p) -> double { return *static_cast<const float*>(p); },
				.store = [](void* p, double v)       { *static_cast<float*>(p) = static_cast<float>(v); },
			},
			// DataTypes_Double
			{
				.load  = [](const void* p) -> double { return *static_cast<const double*>(p); },
				.store = [](void* p, double v)       { *static_cast<double*>(p) = v; },
			},
		}};

		enum WindowFlags_ {
			WindowFlags_None        = 0,
			WindowFlags_NoTitlebar  = 1 << 0,
			WindowFlags_NoMove      = 1 << 1,
			WindowFlags_NoClose     = 1 << 2,
			WindowFlags_NoCollasee  = 1 << 3,
			WindowFlags_NoResize    = 1 << 4,
			WindowFlags_NoDocking   = 1 << 5,
			WindowFlags_FitViewport = 1 << 6,
			WindowFlags_FixedFocus  = 1 << 7,
			WindowFlags_NoScrollbar   = 1 << 8,
			WindowFlags_Size,
		};

		enum Direction {
			Direction_None  = 0,
			Direction_Left  = 1,
			Direction_Up    = 2,
			Direction_Right = 3,
			Direction_Down  = 4,
			Direction_Size,
		};

		enum Axis : std::uint8_t {
			Axis_None,
			Axis_Horizontal,
			Axis_Vertical
		};

		enum DockSide : std::uint8_t {
			DockSide_TopLeft = 0,
			DockSide_BotRight = 1,
			DockSide_Center = 2,
			DockSide_None,
		};

		enum WidgetFlags_ {
			WidgetFlags_None      = 0 << 0,
			WidgetFlags_Clickable = 1 << 0,
			WidgetFlags_Dummy     = 2 << 0,
		};

		enum DragAndDropState {
			DragAndDropState_None,
			DragAndDropState_Dragging,
			DragAndDropState_Dropping,
			DragAndDropState_Size,
		};

		struct TreeNode {
			HashId widget_id;

			bool collapsed = true;

			std::vector<TreeNode> children;
		};

		struct GroupData {
			bool collapsed = false;
		};

		struct Window {
			HashId                id;
			HashId                overlay_id;

			bool                  has_focus         = false;
			bool                  collapsed         = false;
			bool                  disabled          = false;
			bool                  main_window       = false;

			size_t                focus_order       = 0;

			std::string           name;
			std::optional<HashId> dock_node_id;
			std::optional<HashId> dock_node_host_id;
			std::optional<HashId> active_group_id;
			std::optional<glm::vec2> constant_size;

			glm::vec2            size;
			glm::vec2            pos;

			std::unordered_map<HashId, TreeNode> tree_nodes;
			std::unordered_map<HashId, GroupData> groups;
			std::vector<TreeNode> tree_nodes_stack;

			std::vector<HashId> id_stack;

			WindowFlags          flags;
		};

		struct DockNode {
			std::vector<HashId>   windows;

			HashId                next_active_window = 0;
			HashId                active_window      = 0;
			HashId                host_window        = 0;

			HashId                overlay_widget_id  = 0;

			HashId                central_node_id    = 0;
			HashId                parent_id          = 0;
			HashId                id                 = 0;

			std::array<HashId, 2> children;

			glm::vec2             size;
			glm::vec2             position;

			float                 aspect_ratio     = 1.f;

			bool                  dockspace        = false;
			bool                  central_node     = false;
			bool                  root             = false;
			bool                  skip_next_anims  = false;

			Axis             split_axis;
			DockSide              dock_side        = DockSide_None;

			bool is_leaf() { return children[0] == 0; };
		};

		struct AnimNode {
			std::uint64_t last_active_frame  = 0;
			std::uint64_t first_active_frame = 0;

			float         initial;
			float         current;
			float         target;
			float         rate;
		};

		struct Widget {
			std::string   name;

			flecs::entity entity             = flecs::entity::null();

			Widget*       first              = nullptr;
			Widget*       last               = nullptr;
			Widget*       next               = nullptr;
			Widget*       prev               = nullptr;
			Widget*       parent             = nullptr;

			RectF         last_rect;
			RectF         last_visible_rect;
			std::optional<RectF>         last_clip_rect;

			HashId        id                 = 0;

			std::uint64_t last_active_frame  = 0;
			std::uint64_t first_active_frame = 0;

			WidgetFlags   flags;

			bool operator ==(const Widget& other) {
				if (&other == this) {
					return true;
				}

				return other.id == id;
			}
		};

		struct WidgetGroup {
			std::vector<HashId> widgets;

			size_t sort_value;
		};

		using DockTarget = std::variant<Window*, DockNode*>;

		struct DockPayload {
			Axis                 split_axis     = Axis_None;
			DockSide                  dock_side      = DockSide_None;

			std::optional<DockTarget> docking_target;
			std::optional<DockTarget> docked_target;
		};

		struct DockContext {
			std::unordered_map<HashId, DockNode> node_id_to_node;

			std::vector<HashId>                  roots;

			HashId                               next_id = 1;
			DockPayload                          payload;

			float                                node_gap = 3.f;
		};

		struct DragAndDropContext {
			DragAndDropState state        = DragAndDropState_None;

			glm::vec2        start_pos    = {};
			glm::vec2        current_pos  = {};
		};

		struct Context {
			Window*                              current_window;

			std::vector<Window*>                 windows_focus_order;
			std::vector<Widget*>                 roots;

			std::vector<HashId>                  last_render_queue;

			std::unordered_map<HashId, Widget>   id_to_widget;
			std::unordered_map<HashId, AnimNode> id_to_anim;
			std::unordered_map<HashId, Window>   id_to_window;

			std::stack<Widget*>                  parent_stack;

			Widget*                              hot_widget             = nullptr;                                           // The hovered widget
			Widget*                              active_widget          = nullptr;                                           // The clicked widget
			Widget*                              general_overlay_widget = nullptr;

			flecs::entity                        root;

			flecs::query<>                       update_query;
			flecs::query<>                       check_query;
			flecs::world*                        world;

			MouseState                           mouse_input;

			DockContext                          dock_ctx;
			DragAndDropContext                   drag_drop_ctx;

			Style                                style;
			std::vector<ColorStackVar>           color_stack;
			std::vector<StyleStackVar>           style_stack;

			SDL_SystemCursor                     cursor_type          = SDL_SYSTEM_CURSOR_DEFAULT;
			SDL_SystemCursor                     last_cursor_type     = SDL_SYSTEM_CURSOR_DEFAULT;
			SDL_Cursor*                          cursor               = nullptr;

			glm::ivec2                           viewport_size;

			size_t                               frame_count          = 0;
			size_t                               widgets_num          = 0;

			float                                anim_pop_rate        = 20.f;
		};

		void                            init(flecs::world& world);

		Color                           get_color(ColorVars var);
		void                            push_color(ColorVars var, Color value);
		void                            pop_color();

		template<StyleVars V>
		const auto& get_style() {
			return std::get<typename StyleVarType<V>::type>(ctx.style.vars[V]);
		}

		template<StyleVars V>
		void push_style(typename StyleVarType<V>::type value) {
			ctx.style_stack.emplace_back(V, ctx.style.vars[V]);
			ctx.style.vars[V] = value;
		}

		void                            pop_style();

		void                            begin(std::string name, WindowFlags flags = {}, std::optional<glm::vec2> pos = std::nullopt, std::optional<glm::vec2> size = std::nullopt);
		void                            end();

		bool                            begin_group(std::string name);
		void                            end_group();

		void                            scrollbar(std::string name, float visible_value, float actual_value, Axis axis, DockSide side, float* scroll_value);

		void                            begin_scrollable(std::string name);
		void                            end_scrollable();

		void                            dockspace(const std::string& name, glm::vec2 size);

		bool                            button(std::string name);
		bool                            drag_float(std::string name, float* value, float speed, float min, float max, std::string format = "{:.3f}");
		bool                            drag_float2(std::string name, float value[2], float speed, float min, float max, std::string format = "{:.3f}");
		bool                            drag_float3(std::string name, float value[3], float speed, float min, float max, std::string format = "{:.3f}");
		bool                            drag_scalar(std::string name, DataTypes type, void* value, float speed, const void* min, const void* max, std::string format = "{:.3f}");
		bool                            drag_scalar_n(std::string name, DataTypes type, void* value, size_t components, float speed, const void* min, const void* max, std::string format = "{:.3f}");
		bool                            color_edit(std::string name, float value[3]);

		void                            image(std::string name, std::shared_ptr<Texture> texture);

		bool                            tree_node(std::string name);
		void                            tree_pop();

		void node_tree_widget(Widget* root);
		void test_widget();

		void                            text(std::string name, std::string text);

		void                            begin_frame();
		void                            end_frame();

		float                           animation_value(std::string name, float initial, float target, float rate, bool skip = false);
		glm::vec2                       animation_value(std::string name, glm::vec2 initial, glm::vec2 target, float rate, bool skip = false);
		float                           animation_precent(std::string name, float initial, float target, float rate);

		HashId                          hash(std::string_view name);

		void                            push_id(HashId id);
		void                            pop_id();

		bool                            can_draw() const;

		bool clear_on_frame_end = true;
	private:

		// General
		Widget*                         get_widget_by_id(HashId id);
		Widget*                         create_widget(Widget* parent, std::string_view name, WidgetFlags flags = WidgetFlags_None);
		Widget*                         create_widget(std::string_view name, WidgetFlags flags = WidgetFlags_None);

		void                            insert_widget_in_tree(Widget* widget, Widget* parent);

		void                            children(Widget* parent, const std::function<void(Widget*)>& callback);

		void                            dfs(Widget* root, const std::function<void(Widget*)>& callback);
		void                            bfs(std::span<Widget*> roots, const std::function<void(Widget*)>& callback);

		std::vector<HashId>             compose_render_queue();

		size_t                          calculate_dfs_indices(const std::vector<HashId>& render_queue);
		size_t                          calculate_bfs_indices(const std::vector<HashId>& render_queue);

		void                            clear_inactive_widgets();
		void                            clear_inactive_animations();

		void                            process_drag_drop();

		void                            push_parent(HashId widget_id);
		void                            push_parent(Widget* widget);
		void                            pop_parent(size_t num = 1);

		Widget*                         get_parent();

		// Interaction
		bool                            is_widget_hovered(const Widget& widget);

		bool                            drag_interaction(Widget* widget);
		bool                            drag_offset_interaction(Widget* widget, glm::vec2 offset);
		bool                            button_interaction(Widget* widget);
		bool                            hover_interaction(Widget* widget);

		bool                            is_new(const Widget& widget);

		void                            titlebar(Window& window);
		void                            tabsbar(Window& window);

		// Docking
		void                            dock_inner_options(const DockTarget& docking_target);
		void                            dock_outer_options(HashId dockspace_id);
		void                            dock_preview(const DockTarget& docking_target, const DockTarget& docked_target, Axis split_axis, DockSide dock_side);

		DockNode*                       get_dock_node(HashId node_id);
		DockNode*                       get_dock_root_node(HashId node_id);
		DockNode*                       create_dock_root(glm::vec2 position, glm::vec2 size);
		DockNode*                       create_dock_root(HashId id, glm::vec2 position, glm::vec2 size);
		DockNode*                       create_dock_child(DockNode* parent, DockSide side);

		std::vector<HashId>             get_all_node_windows(DockNode* root);

		void                            delete_dock_node(HashId node_id);
		void                            dock_node_update_ratio(HashId node_id, float new_ratio);
		void                            dock_node_skip_anims(HashId node_id);

		std::pair<DockNode*, DockNode*> split_node(HashId node_id, Axis split_axis);

		void                            apply_dock(const DockTarget& docking_target, const DockTarget& docked_target, Axis split_axis, DockSide dock_side);
		void                            undock_window(Window& window);

		void                            update_dock_nodes();
		void                            update_dock_children(DockNode* parent, bool animate);

		void                            bfs(HashId root, const std::function<void(DockNode*)>& callback);

		void                            focus_window(Window& window);
		void                            grip_window_borders(Window& window);
		void                            grip_dock_borders(DockNode& node);

		RectF                           calculate_visible_rect(const Widget* widget);

		int                             extract_decimals_from_fmt(const std::string& fmt, int fallback = 3) const;

		Context ctx;
		Context last_ctx;
	};

	struct ImmediateId {
		Immediate::HashId value;
	};
}
