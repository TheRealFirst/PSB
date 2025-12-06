#include "IndexBuffer.h"

namespace PSB
{
	IndexBuffer::IndexBuffer(WGPUDevice device, WGPUQueue queue, const void* data, uint64_t sizeBytes) : m_Device(device), m_Queue(queue), m_Buffer(nullptr), m_Size(sizeBytes)
	{
		CreateBuffer(data);
	}

	IndexBuffer::~IndexBuffer()
	{
		if (m_Buffer) {
			wgpuBufferRelease(m_Buffer);
			m_Buffer = nullptr;
		}
	}

	void IndexBuffer::Bind(WGPURenderPassEncoder pass, WGPUIndexFormat format) const
	{
		if (!m_Buffer) return;

		wgpuRenderPassEncoderSetIndexBuffer(pass, m_Buffer, format, 0, m_Size);
	}

	IndexBuffer& IndexBuffer::operator=(IndexBuffer&& other) noexcept
	{
		if (this != &other) {
			// Release our current buffer
			if (m_Buffer) {
				wgpuBufferRelease(m_Buffer);
			}

			m_Device = other.m_Device;
			m_Queue = other.m_Queue;
			m_Buffer = other.m_Buffer;
			m_Size = other.m_Size;

			// Null out the moved-from object
			other.m_Device = nullptr;
			other.m_Queue = nullptr;
			other.m_Buffer = nullptr;
			other.m_Size = 0;
		}
		return *this;
	}
	

	void IndexBuffer::CreateBuffer(const void* data)
	{
		if (!m_Device || !m_Queue || !data || m_Size == 0)
			return;

		WGPUBufferDescriptor bufferDesc{};
		bufferDesc.nextInChain = nullptr;
		m_Size = (m_Size + 3) & ~3;
		bufferDesc.size = m_Size;
		// bufferDesc.size = (bufferDesc.size + 3) & ~3;
		bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
		bufferDesc.mappedAtCreation = false;
		m_Buffer = wgpuDeviceCreateBuffer(m_Device, &bufferDesc);

		wgpuQueueWriteBuffer(m_Queue, m_Buffer, 0, data, bufferDesc.size);
	}
}