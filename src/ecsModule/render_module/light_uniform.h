#pragma once

#include "glm.hpp"

namespace se {
	struct PhongMaterial {
		glm::vec4 ambient;
		glm::vec4 diffuse;
		glm::vec4 specular;

		float shininess;
	};

	struct PhongLight {
		glm::vec4 position;

		glm::vec4 ambient;
		glm::vec4 diffuse;
		glm::vec4 specular;
	};
}
