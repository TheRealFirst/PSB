#pragma once
#include <memory>
#include "webgpu/webgpu.h"

namespace PSB {

	class IndexBuffer
	{
	public:
		IndexBuffer() = default;
		IndexBuffer(WGPUDevice device, WGPUQueue queue, const void* data, uint64_t sizeBytes);

		~IndexBuffer();

		void Bind(WGPURenderPassEncoder pass, WGPUIndexFormat format = WGPUIndexFormat_Uint32) const;

		IndexBuffer(const IndexBuffer&) = delete;
		IndexBuffer& operator=(const IndexBuffer&) = delete;

		IndexBuffer(IndexBuffer&& other) noexcept {*this = std::move(other);}
		IndexBuffer& operator=(IndexBuffer&& other) noexcept;

	private:
		void CreateBuffer(const void* data);
	private:
		WGPUDevice m_Device = nullptr;
		WGPUQueue m_Queue = nullptr;
		WGPUBuffer m_Buffer = nullptr;
		uint64_t m_Size = 0;
	};
}