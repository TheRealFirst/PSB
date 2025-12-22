#pragma once

#include <glm/glm.hpp>

namespace PSB
{
	struct VertexAttributes3D {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 color;
		glm::vec2 uv;
	};
}