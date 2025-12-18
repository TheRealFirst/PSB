#pragma once
#include <filesystem>
#include <vector>
#include <webgpu\webgpu.h>

#include <glm/glm.hpp>

namespace PSB
{
	struct VertexAttributes {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 color;
	};

	class AssetManager
	{
	public:
		static bool LoadGeometry(const std::filesystem::path& path, std::vector<float>& pointData, std::vector<uint32_t>& indexData, int dimensions);
		static bool LoadGeometryFromObj(const std::filesystem::path& path, std::vector<VertexAttributes>& vertexData);
		static WGPUShaderModule LoadShaderModule(const std::filesystem::path& path, WGPUDevice device);
	private:

	};
}