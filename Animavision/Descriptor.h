#pragma once

#include "IndexCreator.h"

class SingleDescriptorAllocator
{
public:
	SingleDescriptorAllocator(winrt::com_ptr<ID3D12Device5>, uint32_t numDescriptorsPerHeap,
		D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	~SingleDescriptorAllocator();

	D3D12_CPU_DESCRIPTOR_HANDLE Allocate();
	void Free(D3D12_CPU_DESCRIPTOR_HANDLE handle);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandleFromCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle)
	{
		return CD3DX12_GPU_DESCRIPTOR_HANDLE(
			m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart(), 
			INT((handle.ptr - m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr) / m_DescriptorSize), 
			m_DescriptorSize);
	}

	uint32_t GetDescriptorSize() { return m_DescriptorSize; }

	winrt::com_ptr<ID3D12DescriptorHeap> GetDescriptorHeap() { return m_DescriptorHeap; }

private:
	winrt::com_ptr<ID3D12Device5> m_Device;
	winrt::com_ptr<ID3D12DescriptorHeap> m_DescriptorHeap;
	uint32_t m_DescriptorSize = 0;
	IndexCreator m_IndexCreator;
};

class DescriptorPool
{
public:
	DescriptorPool(winrt::com_ptr<ID3D12Device5> device, uint32_t maxDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	winrt::com_ptr<ID3D12DescriptorHeap> GetDescriptorHeap() { return m_DescriptorHeap; }
	uint32_t GetDescriptorSize() { return m_DescriptorSize; }
	bool AllocateDescriptorTable(D3D12_CPU_DESCRIPTOR_HANDLE* OutCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* OutGpuHandle, uint32_t count);
	void Reset(); 

private:
	winrt::com_ptr<ID3D12Device5> m_Device;
	winrt::com_ptr<ID3D12DescriptorHeap> m_DescriptorHeap;

	uint32_t m_DescriptorSize = 0;
	uint32_t m_MaxDescriptorCount = 0;
	uint32_t m_AllocatedCount = 0;
	
	D3D12_CPU_DESCRIPTOR_HANDLE m_CPUDescriptorHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_GPUDescriptorHandle;

};