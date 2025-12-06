#pragma once
#include <filesystem>
#include <vector>
#include <webgpu\webgpu.h>

namespace PSB
{
	class AssetManager
	{
	public:
		static bool LoadGeometry(const std::filesystem::path& path, std::vector<float>& pointData, std::vector<uint32_t>& indexData);
		static WGPUShaderModule LoadShaderModule(const std::filesystem::path& path, WGPUDevice device);
	private:

	};
}