#include "pch.h"
#include "Descriptor.h"

SingleDescriptorAllocator::SingleDescriptorAllocator(winrt::com_ptr<ID3D12Device5> device, uint32_t numDescriptorsPerHeap, D3D12_DESCRIPTOR_HEAP_TYPE type /*= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV*/, D3D12_DESCRIPTOR_HEAP_FLAGS flags /*= D3D12_DESCRIPTOR_HEAP_FLAG_NONE*/)
{
	m_Device = device;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDescriptorsPerHeap;
	desc.Type = type;
	desc.Flags = flags;
	desc.NodeMask = 0;

	m_IndexCreator.Resize(numDescriptorsPerHeap);

	device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_DescriptorHeap.put()));
	m_DescriptorSize = device->GetDescriptorHandleIncrementSize(type);
}

SingleDescriptorAllocator::~SingleDescriptorAllocator()
{

}

D3D12_CPU_DESCRIPTOR_HANDLE SingleDescriptorAllocator::Allocate()
{
	uint32_t index = m_IndexCreator.Allocate();

	if (index != -1)
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart(), index, m_DescriptorSize);
	}
	else
	{
		__debugbreak();
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
	}
}

void SingleDescriptorAllocator::Free(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	SIZE_T index = (handle.ptr - m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr) / m_DescriptorSize;
	m_IndexCreator.Free((uint32_t)index);
}

DescriptorPool::DescriptorPool(winrt::com_ptr<ID3D12Device5> device, uint32_t maxDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE type /*= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV*/)
{
	m_Device = device;
	m_MaxDescriptorCount = maxDescriptorCount;

	m_DescriptorSize = device->GetDescriptorHandleIncrementSize(type);
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = maxDescriptorCount;
	desc.Type = type;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_DescriptorHeap.put()));

	m_CPUDescriptorHandle = m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_GPUDescriptorHandle = m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
}

bool DescriptorPool::AllocateDescriptorTable(D3D12_CPU_DESCRIPTOR_HANDLE* OutCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* OutGpuHandle, uint32_t count)
{
	bool bResult = false;
	if (m_AllocatedCount + count > m_MaxDescriptorCount)
	{
		return false;
	}

	*OutCpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_CPUDescriptorHandle, m_AllocatedCount, m_DescriptorSize);
	*OutGpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_GPUDescriptorHandle, m_AllocatedCount, m_DescriptorSize);

	m_AllocatedCount += count;

	return true;
}

void DescriptorPool::Reset()
{
	m_AllocatedCount = 0;
}
