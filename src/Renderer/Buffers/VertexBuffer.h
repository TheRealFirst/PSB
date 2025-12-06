#pragma once
#include <memory>
#include "webgpu\webgpu.h"

namespace PSB
{
	class VertexBuffer
	{
	public:
		VertexBuffer() = default;
		VertexBuffer(WGPUDevice device, WGPUQueue queue, const void* data, uint64_t sizeBytes);

		~VertexBuffer();

		void Bind(WGPURenderPassEncoder pass, uint32_t slot = 0) const;

		VertexBuffer(const VertexBuffer&) = delete;
		VertexBuffer& operator=(const VertexBuffer&) = delete;

		VertexBuffer(VertexBuffer&& other) noexcept { *this = std::move(other); }

		VertexBuffer& operator=(VertexBuffer&& other) noexcept;

		uint64_t GetSize() { return m_Size; }
	private:
		void CreateBuffer(const void* data);
	private:
		WGPUDevice m_Device = nullptr;
		WGPUQueue m_Queue = nullptr;
		WGPUBuffer m_Buffer = nullptr;
		uint64_t m_Size = 0;
	};
}