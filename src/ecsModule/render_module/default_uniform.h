#pragma once

#include "glm.hpp"

namespace se {
	struct DefaultUniform {
		glm::mat4 view_proj;

		glm::vec4 light_position;
		glm::vec4 light_color;

		glm::vec3 view_position;
		float     _pad0;

		glm::ivec2 viewport;

		float time;
		float delta_time;
	};
}
