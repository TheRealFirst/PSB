#pragma once
#include <filesystem>
#include <vector>
#include <webgpu\webgpu.h>

#include "Renderer/Vertex.h"

#include <glm/glm.hpp>

namespace PSB
{
	class AssetManager
	{
	public:
		static bool LoadGeometry(const std::filesystem::path& path, std::vector<float>& pointData, std::vector<uint32_t>& indexData, int dimensions);
		static bool LoadGeometryFromObj(const std::filesystem::path& path, std::vector<VertexAttributes3D>& vertexData);
		static WGPUShaderModule LoadShaderModule(const std::filesystem::path& path, WGPUDevice device);
		static WGPUTexture LoadTexture(const std::filesystem::path& path, WGPUDevice device, WGPUTextureView* pTextureView = nullptr);
	private:

	};
}