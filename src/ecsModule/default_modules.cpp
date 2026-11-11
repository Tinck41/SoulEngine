#include "default_modules.h"

#include "assetModule/module.h"
#include "ecsModule/debug_module/module.h"
#include "ecsModule/render_module/module.h"
#include "transformModule/module.h"
#include "cameraModule/module.h"
//#include "renderModule/module.h"
#include "textModule/module.h"
#include "text_render_module/module.h"
#include "spriteModule/module.h"
#include "sprite_render_module/module.h"
#include "meshModule/module.h"
#include "inputModule/module.h"
#include "ui_module/module.h"
#include "ui_render_module_new/module.h"
#include "windowModule/module.h"
#include "ecsModule/mesh3d_module/module.h"
#include "windowModule/components.h"
#include <iostream>
#include <ostream>

using namespace se;

flecs::opaque<std::string> std_string_support(flecs::world& /*unused*/) {
	flecs::opaque<std::string> ts;

	// Let reflection framework know what kind of type this is
	ts.as_type(flecs::String);

	// Forward std::string value to (JSON/...) serializer
	ts.serialize([](const flecs::serializer *s, const std::string *data) {
		const char *value = data->c_str();
		return s->value(flecs::String, &value);
	});

	// Serialize string into std::string
	ts.assign_string([](std::string *data, const char *value) {
		*data = value;
	});

	return ts;
}

template <typename T> 
flecs::opaque<std::vector<T>, T> std_vector_support(flecs::world& world) {
	flecs::opaque<std::vector<T>, T> ts;

	// Let reflection framework know what kind of type this is
	ts.as_type(world.vector<T>());

	// Forward elements of std::vector value to (JSON/...) serializer
	ts.serialize([](const flecs::serializer *s, const std::vector<T> *data) {
		for (const auto& el : *data) {
			s->value(el);
		}
		return 0;
	});

	// Enable direct access to vector elements
	ts.serialize_element([](const flecs::serializer *s, const std::vector<T> *data, size_t element) -> int {
		if (element >= data->size()) {
			return 1;
		}
		return s->value((*data)[element]);
	});

	// Return vector count
	ts.count([](const std::vector<T> *data) {
		return data->size();
	});

	// Ensure element exists, return
	ts.ensure_element([](std::vector<T> *data, size_t elem) {
		if (data->size() <= elem) {
			data->resize(elem + 1);
		}

		return &data->data()[elem];
	});

	// Resize contents of vector
	ts.resize([](std::vector<T> *data, size_t size) {
		data->resize(size);
	});

	return ts;
}

template <typename T>
flecs::opaque<std::unordered_set<T>, T> std_unordered_set_support(flecs::world& world) {
	flecs::opaque<std::unordered_set<T>, T> ts;

	ts.as_type(world.vector<T>());

	ts.serialize([](const flecs::serializer *s, const std::unordered_set<T> *data) {
		for (const auto& el : *data) {
			s->value(el);
		}
		return 0;
	});

	ts.count([](const std::unordered_set<T> *data) {
		return data->size();
	});

	ts.serialize_element([](const flecs::serializer *s, const std::unordered_set<T> *data, size_t element) -> int {
		if (element >= data->size()) {
			return 1;
		}
		auto it = data->begin();
		std::advance(it, element);
		return s->value(*it);
	});

	ts.ensure_element([](std::unordered_set<T> *data, size_t elem) {
		if (data->size() <= elem) {
			data->insert(T{});
		}
		auto it = data->begin();
		std::advance(it, elem);
		return const_cast<T*>(&*it);
	});

	ts.resize([](std::unordered_set<T> *data, size_t size) {
		while (data->size() < size) {
			data->insert(T{});
		}
		while (data->size() > size) {
			data->erase(data->begin());
		}
	});

	return ts;
}

template <typename T>
flecs::opaque<std::optional<T>, T> std_optional_support(flecs::world &world) {
	return flecs::opaque<std::optional<T>, T>()
		.as_type(world.vector<T>())
		.serialize( [](const flecs::serializer *s, const std::optional<T> *data) {
			if (*data) {
				s->value(**data);
			}
			return 0;
		})
	.count([](const std::optional<T> *data) -> size_t {
		return *data ? 1 : 0;
	})
	.resize([](std::optional<T> *data, size_t size) {
		switch (size) {
		case 0:
			*data = std::nullopt;
			break;
		case 1:
			if(!data->has_value()) {
				*data = T();
			}
			break;
		default:
			assert(false);
		}
	})
	.ensure_element( [](std::optional<T> *data, size_t) {
		if(!data->has_value()) {
			*data = T();
		}
		return &data->value();
	});
}

template <typename T>
flecs::opaque<std::pair<T, T>, T> std_pair_support(flecs::world& world) {
    return flecs::opaque<std::pair<T, T>, T>()
        .as_type(world.component<std::pair<T, T>>()
			.member(&std::pair<T, T>::first, "first")
			.member(&std::pair<T, T>::second, "second")
		)
        .serialize([](const flecs::serializer *s, const std::pair<T, T> *data) {
			s->member("first");
            s->value(data->first);
			s->member("second");
            s->value(data->second);
            return 0;
        })
        .ensure_element([](std::pair<T, T> *data, size_t id) {
            return (id == 0) ? &data->first : &data->second;
        });
}

DefaultModules::DefaultModules(flecs::world& world) {
	world.component<std::string>()
		.opaque(std_string_support);

	world.component<std::vector<flecs::entity>>()
		.opaque(std_vector_support<flecs::entity>);

	world.component<std::vector<flecs::entity_t>>()
		.opaque(std_vector_support<flecs::entity_t>);

	world.component<std::unordered_set<flecs::entity_t>>()
		.opaque(std_unordered_set_support<flecs::entity_t>);

	world.component<std::optional<glm::vec2>>()
		.opaque(std_optional_support<glm::vec2>);

	world.component<glm::ivec2>()
		.member<int>("x")
		.member<int>("y");

	world.component<glm::vec2>()
		.member<float>("x")
		.member<float>("y");

	world.component<glm::vec3>()
		.member<float>("x")
		.member<float>("y")
		.member<float>("z");

	world.component<glm::vec4>()
		.member<float>("x")
		.member<float>("y")
		.member<float>("z")
		.member<float>("w");

	world.component<SDL_Color>()
		.member<unsigned char>("r")
		.member<unsigned char>("g")
		.member<unsigned char>("b")
		.member<unsigned char>("a");

	world.component<SDL_FRect>()
		.member<float>("x")
		.member<float>("y")
		.member<float>("w")
		.member<float>("h");

	world.component<std::vector<SDL_FRect>>()
		.opaque(std_vector_support<SDL_FRect>);

	world.component<TextureAtlas>()
		.member(&TextureAtlas::rects, "rect")
		.member(&TextureAtlas::current_index, "current_index");

	world.component<std::optional<TextureAtlas>>()
		.opaque(std_optional_support<TextureAtlas>);

	world.component<Color>()
		.member<std::uint8_t>("r")
		.member<std::uint8_t>("g")
		.member<std::uint8_t>("b")
		.member<std::uint8_t>("a");

	world.component<std::optional<float>>()
		.opaque(std_optional_support<float>);

	world.component<std::optional<double>>()
		.opaque(std_optional_support<double>);

	world.component<std::optional<int>>()
		.opaque(std_optional_support<int>);

	world.component<std::pair<float, float>>()
		.member(&std::pair<float, float>::first, "first")
		.member(&std::pair<float, float>::second, "second");

	world.component<std::pair<std::optional<float>, std::optional<float>>>()
		.member(&std::pair<std::optional<float>, std::optional<float>>::first, "first")
		.member(&std::pair<std::optional<float>, std::optional<float>>::second, "second");


	//world.module<DefaultModules>();

	world.import<flecs::stats>();

	world.import<AssetModule>();
	world.import<TransformModule>();
	world.import<WindowModule>();
	world.import<CameraModule>();
	world.import<RenderModule>();
	world.import<SpriteModule>();
	world.import<SpriteRenderModule>();
	world.import<TextModule>();
	world.import<TextRenderModule>();
	//world.import<MeshModule>();
	world.import<InputModule>();
	world.import<UiModule>();
	world.import<UiRenderModule>();
	world.import<Mesh3dModule>();
	//world.import<DebugModule>();
}
