#include "pch.h"
#include "CommandObject.h"
#include "DX12Helper.h"
#include <utility>

std::shared_ptr<CommandObject> CommandObjectPool::QueryCommandObject(D3D12_COMMAND_LIST_TYPE Type)
{
	assert(Type == D3D12_COMMAND_LIST_TYPE_DIRECT && "other is Not implemented yet");

	std::shared_ptr<CommandObject> ret = nullptr;
	if (m_AvailableCommandObjects.empty())
	{
		ret = std::make_shared<CommandObject>(Type);
		m_Owner->CreateNewCommandList(Type, &ret->m_CommandList, &ret->m_CurrentAllocator);
		ret->m_Owner = m_Owner;
	}
	else
	{
		ret = m_AvailableCommandObjects.front();
		m_AvailableCommandObjects.pop();
		ret->Reset();
	}
	assert(ret != nullptr);

	assert(ret->m_Type == Type);

	return ret;
}

void CommandObjectPool::ReleaseCommandObject(std::shared_ptr<CommandObject> commandObject)
{
	assert(commandObject != nullptr);
	m_AvailableCommandObjects.push(commandObject);
}

CommandObjectManager::CommandObjectManager(winrt::com_ptr<ID3D12Device5> device) : 
	m_Device(device),
	m_CommandQueue(device, D3D12_COMMAND_LIST_TYPE_DIRECT)
{
	m_CommandObjectPool = std::make_shared<CommandObjectPool>(this);
}

void CommandObjectManager::CreateNewCommandList(D3D12_COMMAND_LIST_TYPE Type, winrt::com_ptr<ID3D12GraphicsCommandList>* List, winrt::com_ptr<ID3D12CommandAllocator>* Allocator)
{
	assert(Type == D3D12_COMMAND_LIST_TYPE_DIRECT && "other is Not implemented yet");

	*Allocator = m_CommandQueue.QueryCommandAllocator();

	ThrowIfFailed(m_Device->CreateCommandList(0, Type, (*Allocator).get(), nullptr, IID_PPV_ARGS(List)));
	(*List)->SetName(L"CommandList");

}

void CommandObjectManager::WaitForFence(uint64_t FenceValue)
{

}

void CommandObject::Reset()
{
	assert(m_CommandList != nullptr && m_CurrentAllocator == nullptr);
	m_CurrentAllocator = m_Owner->GetCommandQueue().QueryCommandAllocator();
	ThrowIfFailed(m_CommandList->Reset(m_CurrentAllocator.get(), nullptr));



}

uint64_t CommandObject::Flush(bool WaitForCompletion /*= false*/)
{
	assert(m_CurrentAllocator != nullptr);

	CommandQueue& commandQueue = m_Owner->GetCommandQueue();

	uint64_t FenceValue = commandQueue.ExecuteCommandList(m_CommandList.get());

	m_CommandList->Reset(m_CurrentAllocator.get(), nullptr);


	if (WaitForCompletion)
	{
		m_Owner->GetCommandQueue().WaitForFence(FenceValue);
	}

	return FenceValue;
}

uint64_t CommandObject::Finish(bool WaitForCompletion /*= false*/)
{
	CommandQueue& commandQueue = m_Owner->GetCommandQueue();

	uint64_t FenceValue = commandQueue.ExecuteCommandList(m_CommandList.get());
	commandQueue.ReleaseCommandAllocator(FenceValue, m_CurrentAllocator);
	m_CurrentAllocator = nullptr;


	if (WaitForCompletion)
	{
		commandQueue.WaitForFence(FenceValue);
	}

	m_Owner->GetCommandObjectPool()->ReleaseCommandObject(shared_from_this());

	return FenceValue;
}

CommandQueue::CommandQueue(winrt::com_ptr<ID3D12Device5> device, D3D12_COMMAND_LIST_TYPE Type) :
	m_Type(Type),
	m_AllocatorPool(device, Type),
	m_CommandQueue(nullptr),
	m_Fence(nullptr),
	m_NextFenceValue((uint64_t)Type << 56 | 1),
	m_LastCompletedFenceValue((uint64_t)Type << 56)
{
	assert(device != nullptr);
	assert(m_CommandQueue == nullptr);
	assert(m_AllocatorPool.size() == 0);

	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = m_Type;
	desc.NodeMask = 1;
	device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_CommandQueue));
	m_CommandQueue->SetName(L"CommandListManager::m_CommandQueue");

	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
	m_Fence->SetName(L"CommandListManager::m_pFence");
	m_Fence->Signal((uint64_t)m_Type << 56);

	m_FenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
	assert(m_FenceEventHandle != NULL);

	assert(m_CommandQueue != nullptr);

}

uint64_t CommandQueue::ExecuteCommandList(ID3D12CommandList* commandList)
{
	((ID3D12GraphicsCommandList*)commandList)->Close();
	m_CommandQueue->ExecuteCommandLists(1, &commandList);

	m_CommandQueue->Signal(m_Fence.get(), m_NextFenceValue);

	return m_NextFenceValue++;
}

winrt::com_ptr<ID3D12CommandAllocator> CommandQueue::QueryCommandAllocator()
{
	uint64_t CompletedFence = m_Fence->GetCompletedValue();

	return m_AllocatorPool.QueryCommandAllocator(CompletedFence);
}

void CommandQueue::ReleaseCommandAllocator(uint64_t FenceValueForReset, winrt::com_ptr<ID3D12CommandAllocator> allocator)
{
	m_AllocatorPool.ReleaseCommandAllocator(FenceValueForReset, allocator);
}

uint64_t CommandQueue::IncrementFence(void)
{
	m_CommandQueue->Signal(m_Fence.get(), m_NextFenceValue);
	return m_NextFenceValue++;
}

bool CommandQueue::IsFenceComplete(uint64_t FenceValue)
{
	if (FenceValue > m_LastCompletedFenceValue)
		m_LastCompletedFenceValue = (std::max)(m_LastCompletedFenceValue, m_Fence->GetCompletedValue());

	return FenceValue <= m_LastCompletedFenceValue;
}

void CommandQueue::StallForFence(uint64_t FenceValue)
{
	// not implemented yet
}

void CommandQueue::StallForProducer(CommandQueue& Producer)
{
	// not implemented yet
}

void CommandQueue::WaitForFence(uint64_t FenceValue)
{
	if (IsFenceComplete(FenceValue))
		return;

	// TODO:  Think about how this might affect a multi-threaded situation.  Suppose thread A
	// wants to wait for fence 100, then thread B comes along and wants to wait for 99.  If
	// the fence can only have one event set on completion, then thread B has to wait for 
	// 100 before it knows 99 is ready.  Maybe insert sequential events?
	{
		m_Fence->SetEventOnCompletion(FenceValue, m_FenceEventHandle);
		WaitForSingleObject(m_FenceEventHandle, INFINITE);
		m_LastCompletedFenceValue = FenceValue;
	}
}

winrt::com_ptr<ID3D12CommandAllocator> CommandAllocatorPool::QueryCommandAllocator(uint64_t CompletedFenceValue)
{
	winrt::com_ptr<ID3D12CommandAllocator> pAllocator = nullptr;

	if (!m_AvailableCommandAllocators.empty())
	{
		std::pair<uint64_t, winrt::com_ptr<ID3D12CommandAllocator>>& AllocatorPair = m_AvailableCommandAllocators.front();

		if (AllocatorPair.first <= CompletedFenceValue)
		{
			pAllocator = AllocatorPair.second;
			ThrowIfFailed(pAllocator->Reset());
			m_AvailableCommandAllocators.pop();
		}
	}

	// If no allocator's were ready to be reused, create a new one
	if (pAllocator == nullptr)
	{
		ThrowIfFailed(m_Device->CreateCommandAllocator(m_Type, IID_PPV_ARGS(pAllocator.put())));
		wchar_t AllocatorName[32];
		swprintf(AllocatorName, 32, L"CommandAllocator %zu", m_AllocatorPool.size());
		pAllocator->SetName(AllocatorName);
		m_AllocatorPool.push_back(pAllocator);
	}

	return pAllocator;
}

void CommandAllocatorPool::ReleaseCommandAllocator(uint64_t FenceValueForReset, winrt::com_ptr<ID3D12CommandAllocator> allocator)
{
	m_AvailableCommandAllocators.push(std::make_pair(FenceValueForReset, allocator));
}
