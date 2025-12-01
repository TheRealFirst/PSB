#pragma once
#include <vector>
#include "webgpu\webgpu.h"

namespace PSB
{
	class VertexBuffer
	{
		VertexBuffer() = default;
		VertexBuffer(WGPUDevice device, WGPUQueue queue, std::vector<float> data, uint32_t vertexCount);

		~VertexBuffer();

	private:
		void CreateBuffer(std::vector<float> data);

	private:
		WGPUDevice m_Device = nullptr;
		WGPUQueue m_Queue = nullptr;
		WGPUBuffer m_Buffer = nullptr;
		uint32_t m_VertexCount = 0;
	};
}