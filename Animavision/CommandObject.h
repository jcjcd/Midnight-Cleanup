#pragma once

#include <queue>

class CommandObjectManager;
class CommandQueue;
class CommandObject;


class CommandAllocatorPool
{
public:
	CommandAllocatorPool(winrt::com_ptr<ID3D12Device5> device, D3D12_COMMAND_LIST_TYPE Type) : m_Device(device), m_Type(Type) {}

	winrt::com_ptr<ID3D12CommandAllocator> QueryCommandAllocator(uint64_t CompletedFenceValue);
	void ReleaseCommandAllocator(uint64_t FenceValueForReset, winrt::com_ptr<ID3D12CommandAllocator> allocator);

	size_t size() { return m_AllocatorPool.size(); }

private:
	const D3D12_COMMAND_LIST_TYPE m_Type;

	winrt::com_ptr<ID3D12Device5> m_Device = nullptr;
	std::vector<winrt::com_ptr<ID3D12CommandAllocator>> m_AllocatorPool;
	std::queue<std::pair<uint64_t, winrt::com_ptr<ID3D12CommandAllocator>> > m_AvailableCommandAllocators;


};

class CommandQueue
{
public:
	CommandQueue(winrt::com_ptr<ID3D12Device5> device, D3D12_COMMAND_LIST_TYPE Type);
	~CommandQueue() = default;


	uint64_t ExecuteCommandList(ID3D12CommandList* commandList);
	winrt::com_ptr<ID3D12CommandAllocator> QueryCommandAllocator();
	void ReleaseCommandAllocator(uint64_t FenceValueForReset, winrt::com_ptr<ID3D12CommandAllocator> allocator);


	uint64_t IncrementFence(void);
	bool IsFenceComplete(uint64_t FenceValue);
	void StallForFence(uint64_t FenceValue);
	void StallForProducer(CommandQueue& Producer);
	void WaitForFence(uint64_t FenceValue);
	void WaitForIdle(void) { WaitForFence(IncrementFence()); }

	uint64_t GetNextFenceValue() { return m_NextFenceValue; }

	ID3D12CommandQueue* GetCommandQueue() { return m_CommandQueue.get(); }

private:	
	const D3D12_COMMAND_LIST_TYPE m_Type;
	winrt::com_ptr<ID3D12CommandQueue> m_CommandQueue;

	CommandAllocatorPool m_AllocatorPool;

	winrt::com_ptr<ID3D12Fence> m_Fence;
	uint64_t m_NextFenceValue = 0;
	uint64_t m_LastCompletedFenceValue = 0;
	HANDLE m_FenceEventHandle;
};

class CommandObject : public std::enable_shared_from_this<CommandObject>
{
public:
	CommandObject(D3D12_COMMAND_LIST_TYPE Type) : m_Type(Type) {}
	~CommandObject() = default;

	void Reset();

	ID3D12GraphicsCommandList* GetCommandList() { return m_CommandList.get(); }
	ID3D12CommandAllocator* GetCurrentAllocator() { return m_CurrentAllocator.get(); }

	// 커맨드 오브젝트를 비운다 그리고 릴리즈 하지 않는다.
	uint64_t Flush(bool WaitForCompletion = false);

	// 커맨드 오브젝트를 비우고 릴리즈 한다.
	uint64_t Finish(bool WaitForCompletion = false);

private:
	const D3D12_COMMAND_LIST_TYPE m_Type;
	CommandObjectManager* m_Owner;

	winrt::com_ptr<ID3D12GraphicsCommandList> m_CommandList = nullptr;
	winrt::com_ptr<ID3D12CommandAllocator> m_CurrentAllocator = nullptr;

	friend class CommandObjectManager;
	friend class CommandObjectPool;
};


class CommandObjectPool
{
public:
	CommandObjectPool(CommandObjectManager* Owner) : m_Owner(Owner) {}
	~CommandObjectPool() = default;


	std::shared_ptr<CommandObject> QueryCommandObject(D3D12_COMMAND_LIST_TYPE Type);

	void ReleaseCommandObject(std::shared_ptr<CommandObject> commandObject);

private:
	std::vector<std::shared_ptr<CommandObject>> m_CommandObjects;
	std::queue<std::shared_ptr<CommandObject>> m_AvailableCommandObjects;

	CommandObjectManager* m_Owner;

};

class CommandObjectManager
{
public:
	CommandObjectManager(winrt::com_ptr<ID3D12Device5> device);
	~CommandObjectManager() = default;

	void CreateNewCommandList(D3D12_COMMAND_LIST_TYPE Type, winrt::com_ptr<ID3D12GraphicsCommandList>* List, winrt::com_ptr<ID3D12CommandAllocator>* Allocator);

	CommandQueue& GetCommandQueue() { return m_CommandQueue; }

	CommandObjectPool* GetCommandObjectPool() { return m_CommandObjectPool.get(); }

	void WaitForFence(uint64_t FenceValue);

	void IdleGPU(void) 
	{
		m_CommandQueue.WaitForIdle();
	}

private:
	winrt::com_ptr<ID3D12Device5> m_Device = nullptr;

	std::shared_ptr<CommandObjectPool> m_CommandObjectPool;

	CommandQueue m_CommandQueue;

	friend class ChangDXII;
};
