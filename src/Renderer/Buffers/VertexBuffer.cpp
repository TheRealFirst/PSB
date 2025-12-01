#include "VertexBuffer.h""

PSB::VertexBuffer::VertexBuffer(WGPUDevice device, WGPUQueue queue, std::vector<float> data, uint32_t vertexCount) : m_Queue(queue), m_Device(device), m_VertexCount(vertexCount)
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

void PSB::VertexBuffer::CreateBuffer(std::vector<float> data)
{
	
}
