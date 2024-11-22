#include "pch.h"
#include "SimpleLighting.h"
#include "ChangDXII.h"
#include "DX12Shader.h"
#include "Mesh.h"

namespace GlobalRootSignatureParams {
	enum Value {
		OutputViewSlot = 0,
		AccelerationStructureSlot,
		SceneConstantSlot,
		Count
	};
}

namespace LocalRootSignatureParams {
	enum Value {
		CubeConstantSlot = 0,
		VertexBuffersSlot,
		IndexBuffersSlot,
		OffsetsSlot,
		Count
	};
}

void SimpleLighting::SetUp(ChangDXII* renderer, DX12Shader* shader)
{
	m_Renderer = renderer;
	m_RaytracingShader = shader;


	// Create root signatures for the shaders.
	CreateRootSignatures();

	// Create a raytracing pipeline state object which defines the binding of shaders, state and resources to be used during raytracing.
	CreateRaytracingPipelineStateObject();

	CreateConstantBuffers();

	{
		m_cubeCB.albedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	XMFLOAT4 lightPosition;
	XMFLOAT4 lightAmbientColor;
	XMFLOAT4 lightDiffuseColor;
	lightPosition = XMFLOAT4(0.0f, 1.8f, -3.0f, 0.0f);
	lightAmbientColor = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	lightDiffuseColor = XMFLOAT4(0.5f, 0.0f, 0.0f, 1.0f);

	// Apply the initial values to all frames' buffer instances.
	for (auto& sceneCB : m_sceneCB)
	{
		sceneCB.lightPosition = XMLoadFloat4(&lightPosition);
		sceneCB.lightAmbientColor = XMLoadFloat4(&lightAmbientColor);
		sceneCB.lightDiffuseColor = XMLoadFloat4(&lightDiffuseColor);
	}
}

void SimpleLighting::SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, winrt::com_ptr<ID3D12RootSignature>* rootSig)
{
	auto device = m_Renderer->GetContext()->GetDevice();
	winrt::com_ptr<ID3DBlob> blob;
	winrt::com_ptr<ID3DBlob> error;

	ThrowIfFailed(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, blob.put(), error.put()), error ? static_cast<wchar_t*>(error->GetBufferPointer()) : nullptr);
	ThrowIfFailed(device->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&(*rootSig))));
}

void SimpleLighting::CreateRootSignatures()
{
	auto device = m_Renderer->GetContext()->GetDevice();

	// Global Root Signature
	// This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
	{
		CD3DX12_DESCRIPTOR_RANGE ranges[1]; // Perfomance TIP: Order from most frequent to least frequent.
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);  // 1 output texture

		CD3DX12_ROOT_PARAMETER rootParameters[GlobalRootSignatureParams::Count];
		rootParameters[GlobalRootSignatureParams::OutputViewSlot].InitAsDescriptorTable(1, &ranges[0]);
		rootParameters[GlobalRootSignatureParams::AccelerationStructureSlot].InitAsShaderResourceView(0);
		rootParameters[GlobalRootSignatureParams::SceneConstantSlot].InitAsConstantBufferView(0);
		CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
		SerializeAndCreateRaytracingRootSignature(globalRootSignatureDesc, &m_raytracingGlobalRootSignature);
	}

	// Local Root Signature
	// This is a root signature that enables a shader to have unique arguments that come from shader tables.
	{
		CD3DX12_DESCRIPTOR_RANGE ranges[2]; // Perfomance TIP: Order from most frequent to least frequent.
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 1);  // 2 static index and vertex buffers.
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 1);  // 2 static index and vertex buffers.

		CD3DX12_ROOT_PARAMETER rootParameters[LocalRootSignatureParams::Count];
		rootParameters[LocalRootSignatureParams::CubeConstantSlot].InitAsConstants(SizeOfInUint32(m_cubeCB), 1, 1);
		rootParameters[LocalRootSignatureParams::VertexBuffersSlot].InitAsDescriptorTable(1, &ranges[0]);
		rootParameters[LocalRootSignatureParams::IndexBuffersSlot].InitAsDescriptorTable(1, &ranges[1]);
		rootParameters[LocalRootSignatureParams::OffsetsSlot].InitAsConstants(SizeOfInUint32(Offsets), 2, 1);
		CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
		localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
		SerializeAndCreateRaytracingRootSignature(localRootSignatureDesc, &m_raytracingLocalRootSignature);
	}
}

void SimpleLighting::CreateConstantBuffers()
{
	auto device = m_Renderer->GetContext()->GetDevice();
	auto frameCount = m_Renderer->GetContext()->GetCurrentFrameIndex();

	// Create the constant buffer memory and map the CPU and GPU addresses
	const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	// Allocate one constant buffer per frame, since it gets updated every frame.
	size_t cbSize = 2 * sizeof(AlignedSceneConstantBuffer);
	const D3D12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);

	ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&constantBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_perFrameConstants)));

	// Map the constant buffer and cache its heap pointers.
	// We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
	CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
	ThrowIfFailed(m_perFrameConstants->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedConstantData)));
}

void SimpleLighting::CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline)
{
	// Ray gen and miss shaders in this sample are not using a local root signature and thus one is not associated with them.

	// Local root signature to be used in a hit group.
	auto localRootSignature = raytracingPipeline->CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
	localRootSignature->SetRootSignature(m_raytracingLocalRootSignature.get());
	// Define explicit shader association for the local root signature. 
	{
		auto rootSignatureAssociation = raytracingPipeline->CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
		rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
		rootSignatureAssociation->AddExport(c_hitGroupName);
	}
}

void SimpleLighting::CreateRaytracingPipelineStateObject()
{
	// Create 7 subobjects that combine into a RTPSO:
	// Subobjects need to be associated with DXIL exports (i.e. shaders) either by way of default or explicit associations.
	// Default association applies to every exported shader entrypoint that doesn't have any of the same type of subobject associated with it.
	// This simple sample utilizes default shader association except for local root signature subobject
	// which has an explicit association specified purely for demonstration purposes.
	// 1 - DXIL library
	// 1 - Triangle hit group
	// 1 - Shader config
	// 2 - Local root signature and association
	// 1 - Global root signature
	// 1 - Pipeline config
	CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };


	// DXIL library
	// This contains the shaders and their entrypoints for the state object.
	// Since shaders are not considered a subobject, they need to be passed in via DXIL library subobjects.
	auto lib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	D3D12_SHADER_BYTECODE libdxil = CD3DX12_SHADER_BYTECODE(
		m_RaytracingShader->m_ShaderBlob[ShaderStage::RayTracing]->GetBufferPointer(),
		m_RaytracingShader->m_ShaderBlob[ShaderStage::RayTracing]->GetBufferSize());

	lib->SetDXILLibrary(&libdxil);
	// Define which shader exports to surface from the library.
	// If no shader exports are defined for a DXIL library subobject, all shaders will be surfaced.
	// In this sample, this could be ommited for convenience since the sample uses all shaders in the library. 
	{
		lib->DefineExport(c_raygenShaderName);
		lib->DefineExport(c_closestHitShaderName);
		lib->DefineExport(c_missShaderName);
	}

	// Triangle hit group
	// A hit group specifies closest hit, any hit and intersection shaders to be executed when a ray intersects the geometry's triangle/AABB.
	// In this sample, we only use triangle geometry with a closest hit shader, so others are not set.
	auto hitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
	hitGroup->SetClosestHitShaderImport(c_closestHitShaderName);
	hitGroup->SetHitGroupExport(c_hitGroupName);
	hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

	// Shader config
	// Defines the maximum sizes in bytes for the ray payload and attribute structure.
	auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	UINT payloadSize = sizeof(XMFLOAT4);    // float4 pixelColor
	UINT attributeSize = sizeof(XMFLOAT2);  // float2 barycentrics
	shaderConfig->Config(payloadSize, attributeSize);

	// Local root signature and shader association
	// This is a root signature that enables a shader to have unique arguments that come from shader tables.
	CreateLocalRootSignatureSubobjects(&raytracingPipeline);

	// Global root signature
	// This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
	auto globalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	globalRootSignature->SetRootSignature(m_raytracingGlobalRootSignature.get());

	// Pipeline config
	// Defines the maximum TraceRay() recursion depth.
	auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
	// PERFOMANCE TIP: Set max recursion depth as low as needed 
	// as drivers may apply optimization strategies for low recursion depths.
	UINT maxRecursionDepth = 1; // ~ primary rays only. 
	pipelineConfig->Config(maxRecursionDepth);

#if _DEBUG
	PrintStateObjectDesc(raytracingPipeline);
#endif 

	auto dxrDevice = m_Renderer->GetContext()->GetDevice();
	// Create the state object.
	ThrowIfFailed(dxrDevice->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(&m_dxrStateObject)), L"Couldn't create DirectX Raytracing state object.\n");
}

void SimpleLighting::BuildShaderTables()
{
	auto device = m_Renderer->GetContext()->GetDevice();

	void* rayGenShaderIdentifier;
	void* missShaderIdentifier;
	void* hitGroupShaderIdentifier;

	auto GetShaderIdentifiers = [&](auto* stateObjectProperties)
		{
			rayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(c_raygenShaderName);
			missShaderIdentifier = stateObjectProperties->GetShaderIdentifier(c_missShaderName);
			hitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(c_hitGroupName);
		};

	// Get shader identifiers.
	UINT shaderIdentifierSize;
	{
		winrt::com_ptr<ID3D12StateObjectProperties> stateObjectProperties;
		m_dxrStateObject.as(stateObjectProperties);
		GetShaderIdentifiers(stateObjectProperties.get());
		shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	}

	// Ray gen shader table
	{
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize;
		ShaderTable rayGenShaderTable(device, numShaderRecords, shaderRecordSize, L"RayGenShaderTable");
		rayGenShaderTable.push_back(ShaderRecord(rayGenShaderIdentifier, shaderIdentifierSize));
		m_rayGenShaderTableRecordSizeInBytes = rayGenShaderTable.GetShaderRecordSize();
		m_rayGenShaderTable = rayGenShaderTable.GetResource();
	}

	// Miss shader table
	{
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize;
		ShaderTable missShaderTable(device, numShaderRecords, shaderRecordSize, L"MissShaderTable");
		missShaderTable.push_back(ShaderRecord(missShaderIdentifier, shaderIdentifierSize));
		m_missShaderTableRecordSizeInBytes = missShaderTable.GetShaderRecordSize();
		m_missShaderTable = missShaderTable.GetResource();
	}

	// Hit group shader table
	{
		struct RootArguments {

			CubeConstantBuffer cb;
			D3D12_GPU_DESCRIPTOR_HANDLE vertexBufferGPUHandle;
			D3D12_GPU_DESCRIPTOR_HANDLE indexBufferGPUHandle;
			Offsets offsets;
		};

		UINT numShaderRecords = 0;
		for (auto& mesh : m_Meshes)
		{
			numShaderRecords += mesh->subMeshDescriptors.size();
		}
		UINT shaderRecordSize = shaderIdentifierSize + sizeof(RootArguments);
		ShaderTable hitGroupShaderTable(device, numShaderRecords, shaderRecordSize, L"HitGroupShaderTable");
		for (UINT i = 0; i < m_Meshes.size() ; i++)
		{
			for (auto& geometryInstances : m_Meshes[i]->subMeshDescriptors)
			{
				RootArguments rootArguments;
				Offsets offsets = { geometryInstances.vertexOffset, geometryInstances.indexOffset };
				rootArguments.cb = m_cubeCB;
				auto mesh = m_Meshes[i];
				auto vertexBuffer = mesh->vertexBuffer;
				auto indexBuffer = mesh->indexBuffer;
				DX12Buffer* vertexBufferDX12 = static_cast<DX12Buffer*>(vertexBuffer.get());
				DX12Buffer* indexBufferDX12 = static_cast<DX12Buffer*>(indexBuffer.get());
				memcpy(&rootArguments.indexBufferGPUHandle, &indexBufferDX12->GetSRVGpuHandle(), sizeof(indexBufferDX12->GetSRVGpuHandle()));
				memcpy(&rootArguments.vertexBufferGPUHandle, &vertexBufferDX12->GetSRVGpuHandle(), sizeof(vertexBufferDX12->GetSRVGpuHandle()));
				rootArguments.offsets = offsets;

				hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderIdentifier, shaderIdentifierSize, &rootArguments, sizeof(rootArguments)));
			}
		}
		m_hitGroupShaderTableRecordSizeInBytes = hitGroupShaderTable.GetShaderRecordSize();
		m_hitGroupShaderTable = hitGroupShaderTable.GetResource();

	}
}

void SimpleLighting::DispatchRays(UINT width /*= 0*/, UINT height /*= 0*/)
{
	auto frameIndex = m_Renderer->GetContext()->GetCurrentFrameIndex();
	ID3D12GraphicsCommandList4* commandList;
	m_Renderer->GetContext()->GetCommandList()->QueryInterface(IID_PPV_ARGS(&commandList));

	auto DispatchRays = [&](auto* commandList, auto* stateObject, auto* dispatchDesc)
		{
			// Since each shader table has only one shader record, the stride is same as the size.
			dispatchDesc->HitGroupTable.StartAddress = m_hitGroupShaderTable->GetGPUVirtualAddress();
			dispatchDesc->HitGroupTable.SizeInBytes = m_hitGroupShaderTable->GetDesc().Width;
			dispatchDesc->HitGroupTable.StrideInBytes = m_hitGroupShaderTableRecordSizeInBytes;
			dispatchDesc->MissShaderTable.StartAddress = m_missShaderTable->GetGPUVirtualAddress();
			dispatchDesc->MissShaderTable.SizeInBytes = m_missShaderTable->GetDesc().Width;
			dispatchDesc->MissShaderTable.StrideInBytes = m_missShaderTableRecordSizeInBytes;
			dispatchDesc->RayGenerationShaderRecord.StartAddress = m_rayGenShaderTable->GetGPUVirtualAddress();
			dispatchDesc->RayGenerationShaderRecord.SizeInBytes = m_rayGenShaderTable->GetDesc().Width;
			dispatchDesc->Width = width;
			dispatchDesc->Height = height;
			dispatchDesc->Depth = 1;
			commandList->SetPipelineState1(stateObject);
			commandList->DispatchRays(dispatchDesc);
		};

	auto SetCommonPipelineState = [&](auto* descriptorSetCommandList)
		{
			auto descriptorHeap = m_Renderer->GetShaderVisibleHeap();
			descriptorSetCommandList->SetDescriptorHeaps(1, &descriptorHeap);
			// Set index and successive vertex buffer decriptor tables
			DX12Texture* texture = static_cast<DX12Texture*>(m_OutputTexture.get());
			texture->ResourceBarrier(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			commandList->SetComputeRootDescriptorTable(GlobalRootSignatureParams::OutputViewSlot, texture->GetGpuDescriptorHandleUAV());
		};


	commandList->SetComputeRootSignature(m_raytracingGlobalRootSignature.get());

	// Copy the updated scene constant buffer to GPU.
	memcpy(&m_mappedConstantData[frameIndex].constants, &m_sceneCB[frameIndex], sizeof(m_sceneCB[frameIndex]));
	auto cbGpuAddress = m_perFrameConstants->GetGPUVirtualAddress() + frameIndex * sizeof(m_mappedConstantData[0]);
	commandList->SetComputeRootConstantBufferView(GlobalRootSignatureParams::SceneConstantSlot, cbGpuAddress);

	// Bind the heaps, acceleration structure and dispatch rays.
	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	SetCommonPipelineState(commandList);
	commandList->SetComputeRootShaderResourceView(GlobalRootSignatureParams::AccelerationStructureSlot, m_Renderer->GetTopLevelAS()->GetGPUVirtualAddress());
	DispatchRays(commandList, m_dxrStateObject.get(), &dispatchDesc);
}

void SimpleLighting::CreateRaytracingOutputResource()
{
	m_OutputTexture = m_Renderer->CreateEmptyTexture("Output", Texture::Type::Texture2D,
		m_Renderer->GetWidth(), m_Renderer->GetHeight(),
		1, Texture::Format::R8G8B8A8_UNORM, Texture::Usage::UAV);
}

void SimpleLighting::AddMeshToScene(std::shared_ptr<Mesh> mesh, bool isRebuild)
{
	if (mesh != nullptr)
		m_Meshes.push_back(mesh);

	if (isRebuild)
		BuildShaderTables();
}

void SimpleLighting::SetCamera(const XMMATRIX& projectionToWorld, const XMVECTOR& cameraPosition)
{
	auto frameIndex = m_Renderer->GetContext()->GetCurrentFrameIndex();
	m_sceneCB[frameIndex].projectionToWorld = projectionToWorld;
	m_sceneCB[frameIndex].cameraPosition = cameraPosition;
}

