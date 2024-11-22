#pragma once
#include "ShaderResource.h"
#include "DX12Helper.h"

#include "Mesh.h"

class RendererContext;
class DX12Texture;

constexpr uint32_t MAX_CONSTANT_BUFFER_SIZE = 65536;



class GpuUploadBuffer
{
public:
	winrt::com_ptr<ID3D12Resource> GetResource() { return m_resource; }
	virtual void Release() { m_resource.detach(); }
	UINT64 Size() { return m_resource.get() ? m_resource->GetDesc().Width : 0; }
protected:
	winrt::com_ptr<ID3D12Resource> m_resource;

	GpuUploadBuffer() {}
	~GpuUploadBuffer()
	{
		if (m_resource.get())
		{
			m_resource->Unmap(0, nullptr);
		}
	}

	void Allocate(ID3D12Device5* device, UINT bufferSize, LPCWSTR resourceName = nullptr)
	{
		auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
		ThrowIfFailed(device->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_resource)));
		if (resourceName)
		{
			m_resource->SetName(resourceName);
		}
	}

	uint8_t* MapCpuWriteOnly()
	{
		uint8_t* mappedData;
		// We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_resource->Map(0, &readRange, reinterpret_cast<void**>(&mappedData)));
		return mappedData;
	}
};

// Helper class to create and update a structured buffer.
// Usage: 
//    StructuredBuffer<...> sb;
//    sb.Create(...);
//    sb[index].var = ... ; 
//    sb.CopyStagingToGPU(...);
//    Set...View(..., sb.GputVirtualAddress());
template <class T>
class StructuredBuffer : public GpuUploadBuffer
{
	T* m_mappedBuffers;
	std::vector<T> m_staging;
	UINT m_numInstances;

public:
	// Performance tip: Align structures on sizeof(float4) boundary.
	// Ref: https://developer.nvidia.com/content/understanding-structured-buffer-performance
	static_assert(sizeof(T) % 16 == 0, "Align structure buffers on 16 byte boundary for performance reasons.");

	StructuredBuffer() : m_mappedBuffers(nullptr), m_numInstances(0) {}

	void Create(ID3D12Device5* device, UINT numElements, UINT numInstances = 1, LPCWSTR resourceName = nullptr)
	{
		m_numInstances = numInstances;
		m_staging.resize(numElements);
		UINT bufferSize = numInstances * numElements * sizeof(T);
		Allocate(device, bufferSize, resourceName);
		m_mappedBuffers = reinterpret_cast<T*>(MapCpuWriteOnly());
	}

	void CopyStagingToGpu(UINT instanceIndex = 0)
	{
		memcpy(m_mappedBuffers + instanceIndex * NumElements(), &m_staging[0], InstanceSize());
	}

	auto begin() { return m_staging.begin(); }
	auto end() { return m_staging.end(); }
	auto begin() const { return m_staging.begin(); }
	auto end() const { return m_staging.end(); }

	// Accessors
	T& operator[](UINT elementIndex) { return m_staging[elementIndex]; }
	const T& operator[](UINT elementIndex) const { return m_staging[elementIndex]; }
	size_t NumElements() const { return m_staging.size(); }
	UINT ElementSize() const { return sizeof(T); }
	UINT NumInstances() const { return m_numInstances; }
	size_t InstanceSize() const { return NumElements() * ElementSize(); }
	D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress(UINT instanceIndex = 0, UINT elementIndex = 0)
	{
		return m_resource->GetGPUVirtualAddress() + instanceIndex * InstanceSize() + elementIndex * ElementSize();
	}
};

// Helper class to create and update a constant buffer with proper constant buffer alignments.
// Usage: 
//    ConstantBuffer<...> cb;
//    cb.Create(...);
//    cb.staging.var = ... ; | cb->var = ... ; 
//    cb.CopyStagingToGPU(...);
//    Set...View(..., cb.GputVirtualAddress());
template <class T>
class ConstantBuffer : public GpuUploadBuffer
{
	uint8_t* m_mappedConstantData;
	UINT m_alignedInstanceSize;
	UINT m_numInstances;

public:
	ConstantBuffer() : m_alignedInstanceSize(0), m_numInstances(0), m_mappedConstantData(nullptr) {}

	void Create(ID3D12Device5* device, UINT numInstances = 1, LPCWSTR resourceName = nullptr)
	{
		m_numInstances = numInstances;
		m_alignedInstanceSize = Align(sizeof(T), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		UINT bufferSize = numInstances * m_alignedInstanceSize;
		Allocate(device, bufferSize, resourceName);
		m_mappedConstantData = MapCpuWriteOnly();
	}

	void CopyStagingToGpu(UINT instanceIndex = 0)
	{
		memcpy(m_mappedConstantData + instanceIndex * m_alignedInstanceSize, &staging, sizeof(T));
	}

	// Accessors
	// Align staging object on 16B boundary for faster mempcy to the memory returned by Map()
	alignas(16) T staging;
	T* operator->() { return &staging; }
	UINT NumInstances() { return m_numInstances; }
	D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress(UINT instanceIndex = 0)
	{
		return m_resource->GetGPUVirtualAddress() + instanceIndex * m_alignedInstanceSize;
	}
};

class DX12Buffer : public Buffer
{
public:
	void Bind(RendererContext* context) override = 0;

	uint32_t GetCount() const { return m_Count; }
	uint32_t GetSize() const { return m_Size; }
	uint32_t GetStride() const { return m_Stride; }

	static std::shared_ptr<DX12Buffer> Create(Renderer* renderer, Mesh* mesh, Usage usage);

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCpuHandle() { return m_SRVCpuHandle; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGpuHandle() { return m_SRVGpuHandle; }

	winrt::com_ptr<ID3D12Resource> GetResource() { return m_Buffer; }

protected:
	uint32_t m_Count = 0;
	uint32_t m_Size = 0;
	uint32_t m_Stride = 0;

	winrt::com_ptr<ID3D12Resource> m_Buffer;

	D3D12_CPU_DESCRIPTOR_HANDLE m_SRVCpuHandle = {};
	D3D12_GPU_DESCRIPTOR_HANDLE m_SRVGpuHandle = {};

	friend class DX12ResourceManager;
	friend class ChangDXII;
};

class DX12VertexBuffer : public DX12Buffer
{
public:
	//DX12VertexBuffer(RendererContext* context, const void* data, uint32_t size, uint32_t stride);
	DX12VertexBuffer() = default;

	void Bind(RendererContext* context) override;

	static std::shared_ptr<DX12Buffer> Create(Renderer* renderer, Mesh* mesh);

	DX12Texture* GetOutputVertexBuffer() { return m_OutputVertexBuffer.get(); }

private:
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView = {};

public:
	// dxr
	StructuredBuffer<BoneWeight> m_BoneWeights;
	std::shared_ptr<DX12Texture> m_OutputVertexBuffer;

	friend class DX12ResourceManager;
	friend class ChangDXII;
};

class DX12IndexBuffer : public DX12Buffer
{
public:
	DX12IndexBuffer() = default;

	void Bind(RendererContext* context) override;

	static std::shared_ptr<DX12Buffer> Create(Renderer* renderer, Mesh* mesh);

private:
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView = {};

	friend class DX12ResourceManager;
	friend class ChangDXII;
};

class DX12ConstantBuffer : public DX12Buffer
{
public:
	struct ConstantBufferInstance
	{
		D3D12_CPU_DESCRIPTOR_HANDLE CBVCpuHandle;
		D3D12_GPU_VIRTUAL_ADDRESS GpuMemoryAddress;
		BYTE* MappedData;
	};

	DX12ConstantBuffer(Renderer* renderer, uint32_t sizePerCB);
	~DX12ConstantBuffer();

	void Bind(RendererContext* context) override;
	BYTE* Allocate();
	void Reset() { m_AllocatedCount = 0; }

	// this function must call after Allocate
	BYTE* GetAllocatedData() { return m_CurrentInstance->MappedData; }
	ConstantBufferInstance& GetAllocatedInstance() { return *m_CurrentInstance; }

	static std::shared_ptr<DX12ConstantBuffer> Create(Renderer* renderer, uint32_t size);

public:
	uint32_t m_Register = 0;
	BYTE* m_MappedData = nullptr;
	bool m_IsDirty = false;

	uint32_t m_Size = 0;
	uint32_t m_MaxCount = 0;
	uint32_t m_AllocatedCount = 0;

	ConstantBufferInstance* m_ConstantBufferInstances = nullptr;
	ConstantBufferInstance* m_CurrentInstance = nullptr;

	winrt::com_ptr<ID3D12DescriptorHeap> m_DescriptorHeap;
	uint32_t m_DescriptorSize = 0;

	uint32_t m_ParameterIndex = 0;

	friend class DX12ResourceManager;
	friend class ChangDXII;
};

class DX12AccelerationStructure : public DX12Buffer
{
public:
	DX12AccelerationStructure() = default;

	void Bind(RendererContext* context) override;

	static std::shared_ptr<DX12Buffer> Create(Renderer* renderer, Mesh* mesh);

	UINT64 RequiredScratchSize() { return (std::max)(m_prebuildInfo.ScratchDataSizeInBytes, m_prebuildInfo.UpdateScratchDataSizeInBytes); }
	UINT64 RequiredResultDataSizeInBytes() { return m_prebuildInfo.ResultDataMaxSizeInBytes; }

	const std::wstring& GetName() { return m_name; }


	void SetDirty(bool isDirty) { m_isDirty = isDirty; }
	bool IsDirty() { return m_isDirty; }
	UINT64 ResourceSize() { return m_Buffer->GetDesc().Width; }

protected:

	winrt::com_ptr<ID3D12Resource> m_ScratchBuffer;

	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_geometryDescs;
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS m_buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO m_prebuildInfo = {};

	std::wstring m_name;

	bool m_isBuilt = false; // whether the AS has been built at least once.
	bool m_isDirty = true; // whether the AS has been modified and needs to be rebuilt.
	bool m_updateOnBuild = false;
	bool m_allowUpdate = false;

	uint32_t m_ResultDataMaxSizeInBytes = 0;

	friend class DX12ResourceManager;
	friend class ChangDXII;
};

struct BottomLevelAccelerationStructureInstanceDesc : public D3D12_RAYTRACING_INSTANCE_DESC
{
	void SetTransform(const DirectX::XMMATRIX& transform);
	void GetTransform(DirectX::XMMATRIX* transform);
};
static_assert(sizeof(BottomLevelAccelerationStructureInstanceDesc) == sizeof(D3D12_RAYTRACING_INSTANCE_DESC), L"This is a wrapper used in place of the desc. It has to have the same size");


class DX12BottomLevelAS : public DX12AccelerationStructure
{
public:
	DX12BottomLevelAS() {};
	~DX12BottomLevelAS() {}

	void Initialize(ID3D12Device5* device, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags, Mesh& bottomLevelASGeometry, bool allowUpdate = false, bool bUpdateOnBuild = false);
	void Build(ID3D12GraphicsCommandList4* commandList, ID3D12Resource* scratch, ID3D12DescriptorHeap* descriptorHeap, D3D12_GPU_VIRTUAL_ADDRESS baseGeometryTransformGPUAddress = 0);

	UINT GetInstanceContributionToHitGroupIndex() { return m_instanceContributionToHitGroupIndex; }
	void SetInstanceContributionToHitGroupIndex(UINT index) { m_instanceContributionToHitGroupIndex = index; }

	void UpdateGeometryDescsTransform(D3D12_GPU_VIRTUAL_ADDRESS baseGeometryTransformGPUAddress);

	void BuildSkinnedGeometryDescs(Mesh& mesh);

private:
	void BuildGeometryDescs(Mesh& mesh);
	void ComputePrebuildInfo(ID3D12Device5* device);

private:
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_geometryDescs;
	UINT currentID = 0;
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_cacheGeometryDescs[3];
	Matrix m_transform;
	UINT m_instanceContributionToHitGroupIndex = 0;

};

class DX12TopLevelAS : public DX12AccelerationStructure
{
public:
	DX12TopLevelAS() {}
	~DX12TopLevelAS() {}

	void Initialize(ID3D12Device5* device, UINT numBottomLevelASInstanceDescs, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags, bool allowUpdate = false, bool bUpdateOnBuild = false, const wchar_t* resourceName = nullptr);
	void Build(ID3D12GraphicsCommandList4* commandList, UINT numInstanceDescs, D3D12_GPU_VIRTUAL_ADDRESS InstanceDescs, ID3D12Resource* scratch, ID3D12DescriptorHeap* descriptorHeap, bool bUpdate = false);

private:
	void ComputePrebuildInfo(ID3D12Device5* device, UINT numBottomLevelASInstanceDescs);
};


class RaytracingAccelerationStructureManager
{
public:
	RaytracingAccelerationStructureManager(ID3D12Device5* device, UINT numBottomLevelInstances, UINT frameCount);
	~RaytracingAccelerationStructureManager() {}

	void AddBottomLevelAS(ID3D12Device5* device, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags, Mesh& mesh, bool allowUpdate = false, bool performUpdateOnBuild = false);
	UINT AddBottomLevelASInstance(const std::wstring& bottomLevelASname, UINT instanceContributionToHitGroupIndex = UINT_MAX, const float* transform = &Matrix::Identity.m[0][0], BYTE instanceMask = 1);
	UINT AddSkinnedBLASInstance(uint32_t id, UINT instanceContributionToHitGroupIndex = UINT_MAX, const float* transform = &Matrix::Identity.m[0][0], BYTE instanceMask = 1);
	void InitializeTopLevelAS(ID3D12Device5* device, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags, bool allowUpdate = false, bool performUpdateOnBuild = false, const wchar_t* resourceName = nullptr);
	void Build(ID3D12GraphicsCommandList4* commandList, ID3D12DescriptorHeap* descriptorHeap, UINT frameIndex, bool bForceBuild = false);
	BottomLevelAccelerationStructureInstanceDesc& GetBottomLevelASInstance(UINT bottomLevelASinstanceIndex) { return m_bottomLevelASInstanceDescs[bottomLevelASinstanceIndex]; }
	const StructuredBuffer<BottomLevelAccelerationStructureInstanceDesc>& GetBottomLevelASInstancesBuffer() { return m_bottomLevelASInstanceDescs; }

	// skinned
	void AddSkinnedBottomLevelAS(ID3D12Device5* device, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags, uint32_t id, Mesh& mesh, bool allowUpdate /*= false*/);
	void ModifyBLASGeometry(uint32_t id, Mesh& mesh);

	DX12BottomLevelAS& GetBottomLevelAS(const std::wstring& name) { return m_vBottomLevelAS[name]; }
	ID3D12Resource* GetTopLevelASResource() { return m_topLevelAS.GetResource().get(); }
	UINT64 GetASMemoryFootprint() { return m_ASmemoryFootprint; }
	UINT GetNumberOfBottomLevelASInstances() { return static_cast<UINT>(m_bottomLevelASInstanceDescs.NumElements()); }
	UINT GetMaxInstanceContributionToHitGroupIndex();

	void ResetBottomLevelASInstances() {
		auto it = m_bottomLevelASInstanceDescs.begin();
		for (uint32_t i = 0; i < m_numBottomLevelASInstances; i++, it++)
		{
			(*it).AccelerationStructure = 0;
		}
		m_numBottomLevelASInstances = 0;
	}

	void ResetSkinnedBLAS() {
		m_vSkinnedBottomLevelAS.clear();
	}

private:
	DX12TopLevelAS m_topLevelAS;
	std::map<std::wstring, DX12BottomLevelAS> m_vBottomLevelAS;
	StructuredBuffer<BottomLevelAccelerationStructureInstanceDesc> m_bottomLevelASInstanceDescs;
	UINT m_numBottomLevelASInstances = 0;
	winrt::com_ptr<ID3D12Resource>	m_accelerationStructureScratch;
	UINT64 m_scratchResourceSize = 0;
	UINT64 m_ASmemoryFootprint = 0;

	// id , blas
	robin_hood::unordered_map<uint32_t, DX12BottomLevelAS> m_vSkinnedBottomLevelAS;
};