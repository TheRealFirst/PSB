#include "VertexBuffer.h""

PSB::VertexBuffer::VertexBuffer(WGPUDevice device, WGPUQueue queue, const void* data, uint64_t sizeBytes) : m_Queue(queue), m_Device(device), m_Size(sizeBytes)
{
	CreateBuffer(data);
}

PSB::VertexBuffer::~VertexBuffer()
{
	if (m_Buffer)
	{
		wgpuBufferRelease(m_Buffer);
		m_Buffer = nullptr;
	}
}

void PSB::VertexBuffer::Bind(WGPURenderPassEncoder pass, uint32_t slot) const
{
	wgpuRenderPassEncoderSetVertexBuffer(pass, slot, m_Buffer, 0, m_Size);
}

PSB::VertexBuffer& PSB::VertexBuffer::operator=(VertexBuffer&& other) noexcept
{
	if (this != &other) {
		if (m_Buffer) wgpuBufferRelease(m_Buffer);
		m_Buffer = other.m_Buffer;
		m_Device = other.m_Device;
		m_Queue = other.m_Queue;
		m_Size = other.m_Size;

		other.m_Buffer = nullptr;
		other.m_Device = nullptr;
		other.m_Queue = nullptr;
		other.m_Size = 0;
	}
	return *this;
}

void PSB::VertexBuffer::CreateBuffer(const void* data)
{
	WGPUBufferDescriptor bufferDesc{};
	bufferDesc.nextInChain = nullptr;
	bufferDesc.size = m_Size;
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
	bufferDesc.mappedAtCreation = false;
	m_Buffer = wgpuDeviceCreateBuffer(m_Device, &bufferDesc);

	wgpuQueueWriteBuffer(m_Queue, m_Buffer, 0, data, bufferDesc.size);
}
