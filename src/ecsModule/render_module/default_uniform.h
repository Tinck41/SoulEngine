#pragma once

#include "glm.hpp"

namespace se {
	struct DefaultUniform {
		glm::mat4 view_proj;

		glm::vec3 view_position;

		glm::ivec2 viewport;

		float time;
		float delta_time;
	};
}
