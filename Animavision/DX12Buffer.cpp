#include "pch.h"
#include "DX12Buffer.h"
#include "DX12Context.h"
#include "DX12Helper.h"
#include "Renderer.h"
#include "Mesh.h"
#include "ChangDXII.h"


struct BasicVertex
{
	Vector3 position;
	Vector3 normal;
	Vector3 tangent;
	Vector2 uv;
	Vector2 uv2;
	Vector3 bitangent;
};

struct DX12SkinnedVertex
{
	Vector3 position;
	Vector3 normal;
	Vector3 tangent;
	Vector2 uv;
	Vector2 uv2;

	uint32_t boneIndices[4]{};
	float boneWeights[4]{};
};

std::shared_ptr<DX12Buffer> DX12Buffer::Create(Renderer* renderer, Mesh* mesh, Usage usage)
{
	switch (usage)
	{
	case Usage::Vertex:
		return DX12VertexBuffer::Create(renderer, mesh);
		break;
	case Usage::Index:
		return DX12IndexBuffer::Create(renderer, mesh);
		break;
	default:
		break;
	}

	return nullptr;

}

void DX12VertexBuffer::Bind(RendererContext* context)
{
	ID3D12GraphicsCommandList* commandList = static_cast<DX12Context*>(context)->GetCommandList();
	commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);

}

std::shared_ptr<DX12Buffer> DX12VertexBuffer::Create(Renderer* renderer, Mesh* mesh)
{
	// 창 : 이거 char로 어떻게 잘 하면 또 될거같은데..ㅜ
// 	std::vector<char> vertexData;
// 	vertexData.resize(mesh->vertices.size() * sizeof(BasicVertex));

	ChangDXII* changdxii = static_cast<ChangDXII*>(renderer);

	if (changdxii->IsRayTracing())
	{
		std::vector<BasicVertex> vertices;
		vertices.reserve(mesh->vertices.size());
		for (int i = 0; i < mesh->vertices.size(); i++)
		{
			BasicVertex vertex;
			vertex.position = mesh->vertices[i];
			vertex.normal = mesh->normals.size() > 0 ? mesh->normals[i] : Vector3::Zero;
			vertex.tangent = mesh->tangents.size() > 0 ? mesh->tangents[i] : Vector3::Zero;
			vertex.uv = mesh->uv.size() > 0 ? mesh->uv[i] : Vector2::Zero;
			vertex.bitangent = mesh->bitangents.size() > 0 ? mesh->bitangents[i] : Vector3::Zero;
			vertices.emplace_back(vertex);
		}

		auto ret = changdxii->CreateVertexBuffer(vertices.data(), static_cast<uint32_t>(vertices.size()), sizeof(BasicVertex));


		if (mesh->boneWeights.size() > 0)
		{
			// 여기서 포문 한번 더 돌리고
			// m_OutputVertexBuffer를 할당해준다.
			auto vertexBuffer = static_cast<DX12VertexBuffer*>(ret.get());
			vertexBuffer->m_BoneWeights.Create(changdxii->GetContext()->GetDevice(), UINT(mesh->boneWeights.size()), /*instance count*/1, L"BoneWeights");

			for (int i = 0; i < mesh->boneWeights.size(); i++)
			{
				vertexBuffer->m_BoneWeights[i] = mesh->boneWeights[i];
			}
			vertexBuffer->m_BoneWeights.CopyStagingToGpu(0);

			// RWStructuredBuffer<BasicVertex> gOutputVertices : register(u0); 
			// 여기에 대응되게 만들어야한다.
			TextureDesc desc(
				"OutputVertices",
				Texture::Type::Buffer,
				sizeof(BasicVertex),
				uint32_t(vertices.size()),
				1,
				Texture::Format::UNKNOWN,
				Texture::Usage::UAV,
				nullptr,
				Texture::UAVType::NONE);

			vertexBuffer->m_OutputVertexBuffer = static_pointer_cast<DX12Texture>(changdxii->CreateEmptyTexture(desc));
		}




		return ret;
	}
	else
	{
		if (mesh->boneWeights.size() > 0)
		{
			std::vector<DX12SkinnedVertex> vertices;
			vertices.reserve(mesh->vertices.size());
			for (int i = 0; i < mesh->vertices.size(); i++)
			{
				DX12SkinnedVertex vertex;
				vertex.position = mesh->vertices[i];
				vertex.normal = mesh->normals.size() > 0 ? mesh->normals[i] : Vector3::Zero;
				vertex.tangent = mesh->tangents.size() > 0 ? mesh->tangents[i] : Vector3::Zero;
				vertex.uv = mesh->uv.size() > 0 ? mesh->uv[i] : Vector2::Zero;

				for (int j = 0; j < 4; j++)
				{
					vertex.boneIndices[j] = mesh->boneWeights[i].boneIndex[j];
					vertex.boneWeights[j] = mesh->boneWeights[i].weight[j];
				}

				vertices.emplace_back(vertex);
			}

			return changdxii->CreateVertexBuffer(vertices.data(), static_cast<uint32_t>(vertices.size()), sizeof(DX12SkinnedVertex));
		}
		else
		{
			std::vector<BasicVertex> vertices;
			vertices.reserve(mesh->vertices.size());
			for (int i = 0; i < mesh->vertices.size(); i++)
			{
				BasicVertex vertex;
				vertex.position = mesh->vertices[i];
				vertex.normal = mesh->normals.size() > 0 ? mesh->normals[i] : Vector3::Zero;
				vertex.tangent = mesh->tangents.size() > 0 ? mesh->tangents[i] : Vector3::Zero;
				vertex.uv = mesh->uv.size() > 0 ? mesh->uv[i] : Vector2::Zero;
				vertex.bitangent = mesh->bitangents.size() > 0 ? mesh->bitangents[i] : Vector3::Zero;
				vertices.emplace_back(vertex);
			}

			return changdxii->CreateVertexBuffer(vertices.data(), static_cast<uint32_t>(vertices.size()), sizeof(BasicVertex));
		}
	}

}


void DX12IndexBuffer::Bind(RendererContext* context)
{
	ID3D12GraphicsCommandList* commandList = static_cast<DX12Context*>(context)->GetCommandList();
	commandList->IASetIndexBuffer(&m_IndexBufferView);

}

std::shared_ptr<DX12Buffer> DX12IndexBuffer::Create(Renderer* renderer, Mesh* mesh)
{
	ChangDXII* changdxii = static_cast<ChangDXII*>(renderer);

	uint32_t* data = mesh->indices.data();
	uint32_t indexCount = static_cast<uint32_t>(mesh->indices.size());

	return changdxii->CreateIndexBuffer(data, indexCount, sizeof(uint32_t));

}

DX12ConstantBuffer::DX12ConstantBuffer(Renderer* renderer, uint32_t sizePerCB)
{
	if (sizePerCB % 256)
	{
		sizePerCB = (sizePerCB / 256 + 1) * 256;
	}
	m_MaxCount = MAX_CONSTANT_BUFFER_SIZE / sizePerCB;
	auto context = renderer->GetContext();

	auto device = static_cast<DX12Context*>(context)->GetDevice();

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(MAX_CONSTANT_BUFFER_SIZE),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_Buffer)
	));

	// 디스크립터 힙 생성
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = m_MaxCount;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_DescriptorHeap)));

	CD3DX12_RANGE writeRange(0, 0);
	ThrowIfFailed(m_Buffer->Map(0, &writeRange, reinterpret_cast<void**>(&m_MappedData)));

	m_ConstantBufferInstances = new ConstantBufferInstance[m_MaxCount];

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_Buffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = sizePerCB;

	BYTE* data = m_MappedData;
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	m_DescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (uint32_t i = 0; i < m_MaxCount; i++)
	{
		device->CreateConstantBufferView(&cbvDesc, cpuHandle);

		m_ConstantBufferInstances[i].CBVCpuHandle = cpuHandle;
		m_ConstantBufferInstances[i].GpuMemoryAddress = cbvDesc.BufferLocation;
		m_ConstantBufferInstances[i].MappedData = data;

		cpuHandle.Offset(1, m_DescriptorSize);
		cbvDesc.BufferLocation += sizePerCB;
		data += sizePerCB;
	}
}

DX12ConstantBuffer::~DX12ConstantBuffer()
{
	if (m_Buffer != nullptr)
	{
		m_Buffer->Unmap(0, nullptr);
	}

	m_MappedData = nullptr;

	delete[] m_ConstantBufferInstances;
	m_ConstantBufferInstances = nullptr;
}

void DX12ConstantBuffer::Bind(RendererContext* context)
{
	// 	auto dx12Context = static_cast<DX12Context*>(context);
	// 	auto cmdList = dx12Context->GetCommandList();
	// 
	// 	auto instance = GetAllocatedInstance();
	// 	dx12Context->GetDevice()->CopyDescriptorsSimple(1, instance.CBVCpuHandle, 
	// 
	// 	cmdList->SetGraphicsRootConstantBufferView(m_ParameterIndex, m_Buffer->GetGPUVirtualAddress());
}

BYTE* DX12ConstantBuffer::Allocate()
{
	if (m_AllocatedCount >= m_MaxCount)
		return nullptr;

	auto data = m_MappedData + m_Size * m_AllocatedCount;
	m_CurrentInstance = &m_ConstantBufferInstances[m_AllocatedCount];
	m_AllocatedCount++;

	return data;
}

std::shared_ptr<DX12ConstantBuffer> DX12ConstantBuffer::Create(Renderer* renderer, uint32_t size)
{
	return std::make_shared<DX12ConstantBuffer>(renderer, size);
}

void DX12AccelerationStructure::Bind(RendererContext* context)
{

}


RaytracingAccelerationStructureManager::RaytracingAccelerationStructureManager(ID3D12Device5* device, UINT numBottomLevelInstances, UINT frameCount)
{
	m_bottomLevelASInstanceDescs.Create(device, numBottomLevelInstances, frameCount, L"Bottom-Level Acceleration Structure Instance descs.");
}

void RaytracingAccelerationStructureManager::AddBottomLevelAS(ID3D12Device5* device, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags, Mesh& mesh, bool allowUpdate /*= false*/, bool performUpdateOnBuild /*= false*/)
{
	std::wstring name(mesh.name.begin(), mesh.name.end());
	assert(m_vBottomLevelAS.find(name) == m_vBottomLevelAS.end() &&
		L"A bottom level acceleration structure with that name already exists.");

	auto& bottomLevelAS = m_vBottomLevelAS[name];

	bottomLevelAS.Initialize(device, buildFlags, mesh, allowUpdate);

	m_ASmemoryFootprint += bottomLevelAS.RequiredResultDataSizeInBytes();
	m_scratchResourceSize = max(bottomLevelAS.RequiredScratchSize(), m_scratchResourceSize);

	m_vBottomLevelAS[bottomLevelAS.GetName()] = bottomLevelAS;
}

UINT RaytracingAccelerationStructureManager::AddBottomLevelASInstance(const std::wstring& bottomLevelASname, UINT instanceContributionToHitGroupIndex /*= UINT_MAX*/, const float* transform /*= XMMatrixIdentity()*/, BYTE instanceMask /*= 1*/)
{
	assert(m_numBottomLevelASInstances < m_bottomLevelASInstanceDescs.NumElements() && L"Not enough instance desc buffer size.");
	UINT instanceIndex = m_numBottomLevelASInstances++;
	auto& bottomLevelAS = m_vBottomLevelAS[bottomLevelASname];

	auto& instanceDesc = m_bottomLevelASInstanceDescs[instanceIndex];
	instanceDesc.InstanceMask = instanceMask;
	instanceDesc.InstanceContributionToHitGroupIndex = instanceContributionToHitGroupIndex != UINT_MAX ? instanceContributionToHitGroupIndex : bottomLevelAS.GetInstanceContributionToHitGroupIndex();
	instanceDesc.AccelerationStructure = bottomLevelAS.GetResource()->GetGPUVirtualAddress();
	XMStoreFloat3x4(reinterpret_cast<DirectX::XMFLOAT3X4*>(instanceDesc.Transform), reinterpret_cast<const DirectX::FXMMATRIX&>(*transform));

	return instanceIndex;
}

UINT RaytracingAccelerationStructureManager::AddSkinnedBLASInstance(uint32_t id, UINT instanceContributionToHitGroupIndex /*= UINT_MAX*/, const float* transform /*= XMMatrixIdentity()*/, BYTE instanceMask /*= 1*/)
{
	assert(m_numBottomLevelASInstances < m_bottomLevelASInstanceDescs.NumElements() && L"Not enough instance desc buffer size.");
	UINT instanceIndex = m_numBottomLevelASInstances++;
	auto& bottomLevelAS = m_vSkinnedBottomLevelAS[id];

	auto& instanceDesc = m_bottomLevelASInstanceDescs[instanceIndex];
	instanceDesc.InstanceMask = instanceMask;
	instanceDesc.InstanceContributionToHitGroupIndex = instanceContributionToHitGroupIndex;
	instanceDesc.AccelerationStructure = bottomLevelAS.GetResource()->GetGPUVirtualAddress();
	XMStoreFloat3x4(reinterpret_cast<DirectX::XMFLOAT3X4*>(instanceDesc.Transform), reinterpret_cast<const DirectX::FXMMATRIX&>(*transform));

	return instanceIndex;
}

void RaytracingAccelerationStructureManager::InitializeTopLevelAS(ID3D12Device5* device, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags, bool allowUpdate /*= false*/, bool performUpdateOnBuild /*= false*/, const wchar_t* resourceName /*= nullptr*/)
{
	m_accelerationStructureScratch = nullptr;

	m_topLevelAS.Initialize(device, GetNumberOfBottomLevelASInstances(), buildFlags, allowUpdate, performUpdateOnBuild, resourceName);

	m_ASmemoryFootprint += m_topLevelAS.RequiredResultDataSizeInBytes();
	m_scratchResourceSize = max(m_topLevelAS.RequiredScratchSize(), m_scratchResourceSize);


	auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_scratchResourceSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(m_accelerationStructureScratch.put())));
	m_accelerationStructureScratch->SetName(L"Acceleration structure scratch resource");


}

void RaytracingAccelerationStructureManager::Build(ID3D12GraphicsCommandList4* commandList, ID3D12DescriptorHeap* descriptorHeap, UINT frameIndex, bool bForceBuild /*= false*/)
{
	m_bottomLevelASInstanceDescs.CopyStagingToGpu(frameIndex);

	// Build all bottom-level AS.
	{
		for (auto& bottomLevelASpair : m_vBottomLevelAS)
		{
			auto& bottomLevelAS = bottomLevelASpair.second;
			if (bForceBuild || bottomLevelAS.IsDirty())
			{

				D3D12_GPU_VIRTUAL_ADDRESS baseGeometryTransformGpuAddress = 0;
				bottomLevelAS.Build(commandList, m_accelerationStructureScratch.get(), descriptorHeap, baseGeometryTransformGpuAddress);

				// 단일 스크래치 리소스가 재사용되므로 각 호출 사이에 배리어를 둡니다.
				// 성능 팁: 각 BLAS 빌드마다 별도의 스크래치 메모리를 사용하여 GPU 드라이버가 빌드 호출을 중첩할 수 있도록 합니다.
				commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(bottomLevelAS.GetResource().get()));
			}
		}

		for (auto& skinnedBLASpair : m_vSkinnedBottomLevelAS)
		{
			auto& bottomLevelAS = skinnedBLASpair.second;
			if (bForceBuild || bottomLevelAS.IsDirty())
			{
				D3D12_GPU_VIRTUAL_ADDRESS baseGeometryTransformGpuAddress = 0;
				bottomLevelAS.Build(commandList, m_accelerationStructureScratch.get(), descriptorHeap, baseGeometryTransformGpuAddress);

				commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(bottomLevelAS.GetResource().get()));
			}
		}
	}

	// Build the top-level AS.
	{
		bool performUpdate = false; // Always rebuild top-level Acceleration Structure.
		D3D12_GPU_VIRTUAL_ADDRESS instanceDescs = m_bottomLevelASInstanceDescs.GpuVirtualAddress(frameIndex);
		m_topLevelAS.Build(commandList, GetNumberOfBottomLevelASInstances(), instanceDescs, m_accelerationStructureScratch.get(), descriptorHeap, performUpdate);

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(m_topLevelAS.GetResource().get()));
	}
}

void RaytracingAccelerationStructureManager::AddSkinnedBottomLevelAS(ID3D12Device5* device, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags, uint32_t id, Mesh& mesh, bool allowUpdate /*= false*/)
{
	assert(m_vSkinnedBottomLevelAS.find(id) == m_vSkinnedBottomLevelAS.end() &&
		L"A bottom level acceleration structure with that name already exists.");

	auto& bottomLevelAS = m_vSkinnedBottomLevelAS[id];

	bottomLevelAS.Initialize(device, buildFlags, mesh, allowUpdate);

	m_ASmemoryFootprint += bottomLevelAS.RequiredResultDataSizeInBytes();
	m_scratchResourceSize = max(bottomLevelAS.RequiredScratchSize(), m_scratchResourceSize);

	//m_vSkinnedBottomLevelAS[id] = bottomLevelAS;

}

void RaytracingAccelerationStructureManager::ModifyBLASGeometry(uint32_t id, Mesh& mesh)
{
	auto& bottomLevelAS = m_vSkinnedBottomLevelAS[id];

	bottomLevelAS.BuildSkinnedGeometryDescs(mesh);



}

UINT RaytracingAccelerationStructureManager::GetMaxInstanceContributionToHitGroupIndex()
{
	UINT maxInstanceContributionToHitGroupIndex = 0;
	for (UINT i = 0; i < m_numBottomLevelASInstances; i++)
	{
		auto& instanceDesc = m_bottomLevelASInstanceDescs[i];
		maxInstanceContributionToHitGroupIndex = (std::max)(maxInstanceContributionToHitGroupIndex, instanceDesc.InstanceContributionToHitGroupIndex);
	}
	return maxInstanceContributionToHitGroupIndex;
}

void DX12BottomLevelAS::Initialize(ID3D12Device5* device, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags, Mesh& mesh, bool allowUpdate /*= false*/, bool bUpdateOnBuild /*= false*/)
{
	m_allowUpdate = allowUpdate;
	m_updateOnBuild = bUpdateOnBuild;

	m_buildFlags = buildFlags;
	m_name = std::wstring(mesh.name.begin(), mesh.name.end());

	if (allowUpdate)
	{
		m_buildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	}

	BuildGeometryDescs(mesh);
	ComputePrebuildInfo(device);

	auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nullptr,
		IID_PPV_ARGS(m_Buffer.put())));
	if (m_name.empty())
	{
		m_Buffer->SetName(m_name.c_str());
	}


	m_isDirty = true;
	m_isBuilt = false;
}

void DX12BottomLevelAS::Build(
	ID3D12GraphicsCommandList4* commandList,
	ID3D12Resource* scratch,
	ID3D12DescriptorHeap* descriptorHeap,
	D3D12_GPU_VIRTUAL_ADDRESS baseGeometryTransformGPUAddress /*= 0*/)
{
	assert(m_prebuildInfo.ScratchDataSizeInBytes <= scratch->GetDesc().Width && L"Insufficient scratch buffer size provided!");

	if (baseGeometryTransformGPUAddress > 0)
	{
		UpdateGeometryDescsTransform(baseGeometryTransformGPUAddress);
	}

	currentID = (currentID + 1) % DX12Context::kFrameCount;
	m_cacheGeometryDescs[currentID].clear();
	m_cacheGeometryDescs[currentID].resize(m_geometryDescs.size());
	copy(m_geometryDescs.begin(), m_geometryDescs.end(), m_cacheGeometryDescs[currentID].begin());

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& bottomLevelInputs = bottomLevelBuildDesc.Inputs;
	{
		bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		bottomLevelInputs.Flags = m_buildFlags;
		if (m_isBuilt && m_allowUpdate && m_updateOnBuild)
		{
			bottomLevelInputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
			bottomLevelBuildDesc.SourceAccelerationStructureData = m_Buffer->GetGPUVirtualAddress();
		}
		bottomLevelInputs.NumDescs = static_cast<UINT>(m_cacheGeometryDescs[currentID].size());
		bottomLevelInputs.pGeometryDescs = m_cacheGeometryDescs[currentID].data();

		bottomLevelBuildDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
		bottomLevelBuildDesc.DestAccelerationStructureData = m_Buffer->GetGPUVirtualAddress();
	}

	commandList->SetDescriptorHeaps(1, &descriptorHeap);
	commandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);

	m_isDirty = false;
	m_isBuilt = true;
}

void DX12BottomLevelAS::UpdateGeometryDescsTransform(D3D12_GPU_VIRTUAL_ADDRESS baseGeometryTransformGPUAddress)
{
	struct alignas(16) AlignedGeometryTransform3x4
	{
		float transform3x4[12];
	};

	for (UINT i = 0; i < m_geometryDescs.size(); i++)
	{
		auto& geometryDesc = m_geometryDescs[i];
		geometryDesc.Triangles.Transform3x4 = baseGeometryTransformGPUAddress + i * sizeof(AlignedGeometryTransform3x4);
	}
}

void DX12BottomLevelAS::BuildSkinnedGeometryDescs(Mesh& mesh)
{
	DX12IndexBuffer* indexBuffer = static_cast<DX12IndexBuffer*>(mesh.indexBuffer.get());
	DX12VertexBuffer* vertexBuffer = static_cast<DX12VertexBuffer*>(mesh.vertexBuffer.get());

	for (uint32_t i = 0; i < m_geometryDescs.size(); i++)
	{
		auto& subMesh = mesh.subMeshDescriptors[i];
		m_geometryDescs[i].Triangles.VertexBuffer.StartAddress = vertexBuffer->GetOutputVertexBuffer()->GetTexture()->GetGPUVirtualAddress() + subMesh.vertexOffset * vertexBuffer->GetStride();
	}

	m_isDirty = true;
}

void DX12BottomLevelAS::BuildGeometryDescs(Mesh& mesh)
{
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDescTemplate = {};
	geometryDescTemplate.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDescTemplate.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
	geometryDescTemplate.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

	m_geometryDescs.reserve(mesh.subMeshCount);

	DX12IndexBuffer* indexBuffer = static_cast<DX12IndexBuffer*>(mesh.indexBuffer.get());
	DX12VertexBuffer* vertexBuffer = static_cast<DX12VertexBuffer*>(mesh.vertexBuffer.get());

	for (auto& subMesh : mesh.subMeshDescriptors)
	{
		auto& geometryDesc = geometryDescTemplate;
		geometryDesc.Triangles.IndexBuffer = indexBuffer->GetResource()->GetGPUVirtualAddress() + subMesh.indexOffset * sizeof(uint32_t);
		geometryDesc.Triangles.IndexCount = subMesh.indexCount;
		geometryDesc.Triangles.VertexBuffer.StartAddress = vertexBuffer->GetResource()->GetGPUVirtualAddress() + subMesh.vertexOffset * vertexBuffer->GetStride();
		geometryDesc.Triangles.VertexBuffer.StrideInBytes = vertexBuffer->GetStride();
		geometryDesc.Triangles.VertexCount = subMesh.vertexCount;
		geometryDesc.Triangles.Transform3x4 = 0;

		// 창 : 여기서 반투명 이런거 처리해야할듯하다.
		//geometryDescTemplate.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
		geometryDescTemplate.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

		m_geometryDescs.push_back(geometryDesc);
	}
}

void DX12BottomLevelAS::ComputePrebuildInfo(ID3D12Device5* device)
{
	// Get the size requirements for the scratch and AS buffers.
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& bottomLevelInputs = bottomLevelBuildDesc.Inputs;
	bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	bottomLevelInputs.Flags = m_buildFlags;
	bottomLevelInputs.NumDescs = static_cast<UINT>(m_geometryDescs.size());
	bottomLevelInputs.pGeometryDescs = m_geometryDescs.data();

	device->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &m_prebuildInfo);
	assert(m_prebuildInfo.ResultDataMaxSizeInBytes > 0);
}

void DX12TopLevelAS::Initialize(
	ID3D12Device5* device,
	UINT numBottomLevelASInstanceDescs,
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags,
	bool allowUpdate /*= false*/,
	bool bUpdateOnBuild /*= false*/,
	const wchar_t* resourceName /*= nullptr*/)
{
	m_allowUpdate = allowUpdate;
	m_updateOnBuild = bUpdateOnBuild;
	m_buildFlags = buildFlags;
	m_name = resourceName;

	if (allowUpdate)
	{
		m_buildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	}

	ComputePrebuildInfo(device, numBottomLevelASInstanceDescs);
	auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nullptr,
		IID_PPV_ARGS(m_Buffer.put())));
	if (m_name.empty())
	{
		m_Buffer->SetName(m_name.c_str());
	}

	m_isDirty = true;
	m_isBuilt = false;
}

void DX12TopLevelAS::Build(
	ID3D12GraphicsCommandList4* commandList,
	UINT numInstanceDescs,
	D3D12_GPU_VIRTUAL_ADDRESS InstanceDescs,
	ID3D12Resource* scratch,
	ID3D12DescriptorHeap* descriptorHeap,
	bool bUpdate /*= false*/)
{
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = topLevelBuildDesc.Inputs;
	{
		topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		topLevelInputs.Flags = m_buildFlags;
		if (m_isBuilt && m_allowUpdate && m_updateOnBuild)
		{
			topLevelInputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
			topLevelBuildDesc.SourceAccelerationStructureData = m_Buffer->GetGPUVirtualAddress();
		}
		topLevelInputs.NumDescs = numInstanceDescs;

		topLevelBuildDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
		topLevelBuildDesc.DestAccelerationStructureData = m_Buffer->GetGPUVirtualAddress();
	}
	topLevelInputs.InstanceDescs = InstanceDescs;

	commandList->SetDescriptorHeaps(1, &descriptorHeap);
	commandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

	m_isDirty = false;
	m_isBuilt = true;
}

void DX12TopLevelAS::ComputePrebuildInfo(ID3D12Device5* device, UINT numBottomLevelASInstanceDescs)
{
	// Get the size requirements for the scratch and AS buffers.
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = topLevelBuildDesc.Inputs;
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.Flags = m_buildFlags;
	topLevelInputs.NumDescs = numBottomLevelASInstanceDescs;

	device->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &m_prebuildInfo);
	assert(m_prebuildInfo.ResultDataMaxSizeInBytes > 0);
}
