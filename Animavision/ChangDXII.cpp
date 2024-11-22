#include "pch.h"
#include "ChangDXII.h"
#include "Mesh.h"

#include "ShaderResource.h"

#include "DX12Shader.h"
#include "DX12Buffer.h"
#include "Material.h"
#include "PipelineState.h"

#include "DX12Helper.h"
#include "Pathtracer.h"





ChangDXII::ChangDXII(HWND hwnd, uint32_t width, uint32_t height, bool isRaytracing)
{
	m_Context = std::make_unique<DX12Context>(hwnd, width, height);

	if (isRaytracing && !m_Context->IsRayTracingSupported())
	{
		OutputDebugStringA("Raytracing is not supported on this device\n");
		isRaytracing = false;
	}
	m_IsRaytracing = isRaytracing;

	m_CommandObjectPool = m_Context->GetCommandObjectManager()->m_CommandObjectPool;

	auto commandobject = m_CommandObjectPool->QueryCommandObject(D3D12_COMMAND_LIST_TYPE_DIRECT);

	m_Context->SetRenderCommandObject(commandobject);

	m_ResourceManager = std::make_unique<DX12ResourceManager>(m_Context->m_Device, m_CommandObjectPool, m_IsRaytracing);

	// Library
	m_ShaderLibrary = std::make_unique<ShaderLibrary>();
	m_MeshLibrary = std::make_unique<MeshLibrary>(this);
	m_MaterialLibrary = std::make_unique<MaterialLibrary>();
	m_AnimationLibrary = std::make_unique<AnimationLibrary>();

	m_Context->InitializeShaderResources(m_ResourceManager->m_DescriptorAllocator);

	if (m_IsRaytracing)
	{
		m_AccelerationStructure = std::make_unique<RaytracingAccelerationStructureManager>(m_Context->m_Device.get(), kMaxNumBottomLevelInstances, DX12Context::kFrameCount);
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
		bool allowUpdate = true;
		bool performUpdateOnBuild = true;
		m_AccelerationStructure->InitializeTopLevelAS(m_Context->m_Device.get(), buildFlags, allowUpdate, performUpdateOnBuild, L"Top Level Acceleration Structure");

		m_prevFrameBottomLevelASInstanceTransforms.Create(m_Context->m_Device.get(), kMaxNumBottomLevelInstances, DX12Context::kFrameCount, L"GPU buffer: Bottom Level AS Instance transforms for previous frame");

		//m_SimpleLighting = std::make_shared<SimpleLighting>();
		//auto shader = LoadShader("./Shaders/testRaytracing.hlsl");
		//m_SimpleLighting->SetUp(this, std::static_pointer_cast<DX12Shader>(shader).get());

		m_Pathtracer = std::make_shared<Pathtracer>();
		auto Pathtracershader = LoadShader("./Raytracing/Pathtracer.hlsl");
		m_Pathtracer->SetUp(this, std::static_pointer_cast<DX12Shader>(Pathtracershader).get());
		m_Pathtracer->SetResolution(width, height, width, height);
		m_Pathtracer->SetEnvironmentMap(CreateTextureCube("./Resources/Textures/winterLake.dds"));

		for (uint32_t i = 0; i < 2; ++i)
		{
			std::string name = "DebugOutput" + std::to_string(i);
			auto& texture = CreateEmptyTexture(name, Texture::Type::Texture2D, width, height, 1, Texture::Format::R32G32B32A32_FLOAT, Texture::Usage::RTV | Texture::Usage::UAV);
			m_DebugOutput[i] = std::static_pointer_cast<DX12Texture>(texture);
		}

		m_SkinnedVertexConverter = std::make_unique<SkinnedVertexConverter>();
		auto converterShader = LoadShader("./Shaders/CS_Skinning.hlsl");
		m_SkinnedVertexConverter->SetUp(this, std::static_pointer_cast<DX12Shader>(converterShader).get());

	}


	// test
//	m_graphicsMemory = std::make_unique<GraphicsMemory>(m_Context->m_Device.get());
// 	m_batch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(m_Context->GetDevice());
// 	RenderTargetState rtState(m_Context->m_BackBufferFormat, m_Context->m_DepthStencilFormat);
// 
// 	EffectPipelineStateDescription pd(
// 		&VertexPositionColor::InputLayout,
// 		CommonStates::Opaque,
// 		CommonStates::DepthDefault,
// 		CommonStates::CullNone,
// 		rtState,
// 		D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
// 
// 	m_effect = std::make_unique<DirectX::BasicEffect>(m_Context->GetDevice(), EffectFlags::VertexColor, pd);
}

void ChangDXII::Resize(uint32_t width, uint32_t height)
{
	m_Context->Resize(width, height);
}

void ChangDXII::Clear(const float* RGBA)
{
	m_Context->Clear(RGBA);
}

void ChangDXII::SetRenderTargets(uint32_t numRenderTargets, Texture* renderTargets[], Texture* depthStencil /*= nullptr*/, bool useDefaultDSV /*= true*/)
{
	if (numRenderTargets == 0)
	{
		m_Context->SetRenderTargets(0, nullptr, depthStencil, useDefaultDSV);
	}
	else if (renderTargets == nullptr)
	{
		m_Context->SetRenderTargets(1, nullptr, depthStencil, useDefaultDSV);
		return;
	}
	else
	{
		m_Context->SetRenderTargets(numRenderTargets, renderTargets, depthStencil, useDefaultDSV);
	}
}

void ChangDXII::ApplyRenderState(BlendState blendstate, RasterizerState rasterizerstate, DepthStencilState depthstencilstate)
{
	m_BlendState = blendstate;
	m_RasterizerState = rasterizerstate;
	m_DepthStencilState = depthstencilstate;
}

void ChangDXII::SetViewport(uint32_t width, uint32_t height)
{
	m_Context->SetViewport(width, height);
}

void ChangDXII::Submit(Mesh& mesh, Material& material, PrimitiveTopology primitiveMode, uint32_t instances)
{
	// 창 : 임시 이것도 나중에 고칠것
	auto it = m_PipelineStates.find(material.m_Shader->ID);

	if (it == m_PipelineStates.end())
	{
		__debugbreak();
	}
	else
	{
		auto pipelineStateArray = it->second.m_PipelineStates;
		m_PipelineState = pipelineStateArray
			[static_cast<size_t>(primitiveMode)]
			[static_cast<size_t>(m_RasterizerState)]
			[static_cast<size_t>(m_BlendState)]
			[static_cast<size_t>(m_DepthStencilState)]
			.get();

		if (m_PipelineState == nullptr)
		{
			pipelineStateArray
				[static_cast<size_t>(primitiveMode)]
				[static_cast<size_t>(m_RasterizerState)]
				[static_cast<size_t>(m_BlendState)]
				[static_cast<size_t>(m_DepthStencilState)]
				=
				PipelineState::PipelineStateBuilder(this)
				.SetShader(std::static_pointer_cast<DX12Shader>(material.m_Shader))
				.SetPrimitiveTopology(primitiveMode)
				.SetRasterizerState(m_RasterizerState)
				.SetBlendState(m_BlendState)
				.SetDepthStencilState(m_DepthStencilState)
				.Build();

			m_PipelineState = pipelineStateArray
				[static_cast<size_t>(primitiveMode)]
				[static_cast<size_t>(m_RasterizerState)]
				[static_cast<size_t>(m_BlendState)]
				[static_cast<size_t>(m_DepthStencilState)]
				.get();
		}
	}

	m_PipelineState->Bind(GetContext());
	auto shader = m_PipelineState->GetShader();
	auto shader12 = std::static_pointer_cast<DX12Shader>(shader);


	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuTable{};
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuTable{};

	auto commandList = m_Context->GetCommandList();


	shader->Bind(this);

	auto descriptorHeap = m_ResourceManager->m_DescriptorAllocator->GetDescriptorHeap().get();
	commandList->SetDescriptorHeaps(1, &descriptorHeap);


	for (auto& constantBuffer : shader12->m_ConstantBuffers)
	{
		auto slot = constantBuffer->m_ParameterIndex;
		auto&& allocatedInstance = constantBuffer->GetAllocatedInstance();
		m_Context->GetCommandList()->SetGraphicsRootConstantBufferView(slot, allocatedInstance.GpuMemoryAddress);
	}

	for (auto& texture : material.m_Textures)
	{
		auto slot = shader12->m_TextureIndexMap[DX12Shader::ROOT_GLOBAL][texture.first];
		auto texture12 = std::static_pointer_cast<DX12Texture>(texture.second.Texture);

		if (texture12 == nullptr)
			continue;

		texture12->ResourceBarrier(m_Context->GetCommandList(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		commandList->SetGraphicsRootDescriptorTable(slot, texture12->GetGpuDescriptorHandle());
	}

	m_Context->SetPrimitiveTopology((D3D12_PRIMITIVE_TOPOLOGY)primitiveMode);

	mesh.vertexBuffer->Bind(m_Context.get());
	mesh.indexBuffer->Bind(m_Context.get());

	for (uint32_t i = 0; i < mesh.subMeshDescriptors.size(); ++i)
	{
		const SubMeshDescriptor& subMesh = mesh.subMeshDescriptors[i];
		m_Context->DrawIndexed(subMesh.indexCount, instances, subMesh.indexOffset, subMesh.vertexOffset);
	}

	shader12->Unbind();

}

void ChangDXII::Submit(Mesh& mesh, Material& material, uint32_t subMeshIndex, PrimitiveTopology primitiveMode /*= PrimitiveTopology::TRIANGLELIST*/, uint32_t instances /*= 1*/)
{
	if (subMeshIndex >= mesh.subMeshDescriptors.size())
	{
		OutputDebugStringA("Invalid submesh index\n");
		return;
	}

	// 창 : 임시 이것도 나중에 고칠것
	auto it = m_PipelineStates.find(material.m_Shader->ID);

	if (it == m_PipelineStates.end())
	{
		__debugbreak();
	}
	else
	{
		auto pipelineStateArray = it->second.m_PipelineStates;
		m_PipelineState = pipelineStateArray
			[static_cast<size_t>(primitiveMode)]
			[static_cast<size_t>(m_RasterizerState)]
			[static_cast<size_t>(m_BlendState)]
			[static_cast<size_t>(m_DepthStencilState)]
			.get();

		if (m_PipelineState == nullptr)
		{
			pipelineStateArray
				[static_cast<size_t>(primitiveMode)]
				[static_cast<size_t>(m_RasterizerState)]
				[static_cast<size_t>(m_BlendState)]
				[static_cast<size_t>(m_DepthStencilState)]
				=
				PipelineState::PipelineStateBuilder(this)
				.SetShader(std::static_pointer_cast<DX12Shader>(material.m_Shader))
				.SetPrimitiveTopology(primitiveMode)
				.SetRasterizerState(m_RasterizerState)
				.SetBlendState(m_BlendState)
				.SetDepthStencilState(m_DepthStencilState)
				.Build();

			m_PipelineState = pipelineStateArray
				[static_cast<size_t>(primitiveMode)]
				[static_cast<size_t>(m_RasterizerState)]
				[static_cast<size_t>(m_BlendState)]
				[static_cast<size_t>(m_DepthStencilState)]
				.get();
		}
	}

	m_PipelineState->Bind(GetContext());
	auto shader = m_PipelineState->GetShader();
	auto shader12 = std::static_pointer_cast<DX12Shader>(shader);


	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuTable{};
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuTable{};

	auto commandList = m_Context->GetCommandList();

	//auto descriptorHeap = m_DescriptorPool->GetDescriptorHeap().get();
	auto descriptorHeap = m_ResourceManager->m_DescriptorAllocator->GetDescriptorHeap().get();
	commandList->SetDescriptorHeaps(1, &descriptorHeap);

	shader->Bind(this);

	for (auto& constantBuffer : shader12->m_ConstantBuffers)
	{
		auto slot = constantBuffer->m_ParameterIndex;
		auto&& allocatedInstance = constantBuffer->GetAllocatedInstance();

		m_Context->GetCommandList()->SetGraphicsRootConstantBufferView(slot, allocatedInstance.GpuMemoryAddress);

	}

	for (auto& texture : material.m_Textures)
	{
		auto slot = shader12->m_TextureIndexMap[DX12Shader::ROOT_GLOBAL][texture.first];
		auto texture12 = std::static_pointer_cast<DX12Texture>(texture.second.Texture);

		if (texture12 == nullptr)
			continue;

		texture12->ResourceBarrier(m_Context->GetCommandList(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		commandList->SetGraphicsRootDescriptorTable(slot, texture12->GetGpuDescriptorHandle());
	}

	const SubMeshDescriptor& subMesh = mesh.subMeshDescriptors[subMeshIndex];

	m_Context->SetPrimitiveTopology((D3D12_PRIMITIVE_TOPOLOGY)primitiveMode);
	mesh.vertexBuffer->Bind(m_Context.get());
	mesh.indexBuffer->Bind(m_Context.get());
	m_Context->DrawIndexed(subMesh.indexCount, instances, subMesh.indexOffset, subMesh.vertexOffset);

	shader12->Unbind();
}

void ChangDXII::DispatchCompute(Material& material, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
{
	auto it = m_ComputePipelineStates.find(material.m_Shader->ID);

	if (it == m_ComputePipelineStates.end())
	{
		__debugbreak();
	}
	else
	{

		auto pipelineState = it->second;
		pipelineState.Bind(this);
		auto shader = pipelineState.GetComputeShader();
		auto shader12 = std::static_pointer_cast<DX12Shader>(shader);

		auto commandList = m_Context->GetCommandList();

		auto descriptorHeap = m_ResourceManager->m_DescriptorAllocator->GetDescriptorHeap().get();
		commandList->SetDescriptorHeaps(1, &descriptorHeap);

		shader->Bind(this);

		for (auto& constantBuffer : shader12->m_ConstantBuffers)
		{
			auto slot = constantBuffer->m_ParameterIndex;
			auto&& allocatedInstance = constantBuffer->GetAllocatedInstance();
			commandList->SetComputeRootConstantBufferView(slot, allocatedInstance.GpuMemoryAddress);
		}

		for (auto& texture : material.m_Textures)
		{
			auto slot = shader12->m_TextureIndexMap[DX12Shader::ROOT_GLOBAL][texture.first];
			auto texture12 = std::static_pointer_cast<DX12Texture>(texture.second.Texture);

			if (texture12 == nullptr)
				continue;

			texture12->ResourceBarrier(m_Context->GetCommandList(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
			commandList->SetComputeRootDescriptorTable(slot, texture12->GetGpuDescriptorHandle());
		}

		for (auto& uavs : material.m_UAVTextures)
		{
			auto slot = shader12->m_UAVIndexMap[DX12Shader::ROOT_GLOBAL][uavs.first];
			auto texture12 = std::static_pointer_cast<DX12Texture>(uavs.second);

			if (texture12 == nullptr)
				continue;

			texture12->ResourceBarrier(m_Context->GetCommandList(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			commandList->SetComputeRootDescriptorTable(slot, texture12->GetGpuDescriptorHandleUAV());
		}

		m_Context->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);

		shader12->Unbind();
	}
}

void ChangDXII::DispatchRays(Material& material, uint32_t width, uint32_t height, uint32_t depth)
{
	auto it = m_RayTracingPipelineStates.find(material.m_Shader->ID);

	if (it == m_RayTracingPipelineStates.end())
	{
		__debugbreak();
	}
	else
	{
		auto& pipelineState = it->second;
		pipelineState.Bind(this);
		auto shader = pipelineState.GetRayTracingShader();
		auto shader12 = std::static_pointer_cast<DX12Shader>(shader);

		ID3D12GraphicsCommandList4* commandList;
		m_Context->GetCommandList()->QueryInterface(IID_PPV_ARGS(&commandList));

		shader->Bind(this);

		auto descriptorHeap = m_ResourceManager->m_DescriptorAllocator->GetDescriptorHeap().get();
		commandList->SetDescriptorHeaps(1, &descriptorHeap);

		for (auto& constantBuffer : shader12->m_ConstantBuffers)
		{
			auto slot = constantBuffer->m_ParameterIndex;
			auto&& allocatedInstance = constantBuffer->GetAllocatedInstance();
			commandList->SetComputeRootConstantBufferView(slot, allocatedInstance.GpuMemoryAddress);
		}

		// 창 : 임시
		for (auto& as : shader12->m_AccelationStructureBindings[DX12Shader::ROOT_GLOBAL])
		{
			auto slot = as.ParameterIndex;
			commandList->SetComputeRootShaderResourceView(slot, m_AccelerationStructure->GetTopLevelASResource()->GetGPUVirtualAddress());
		}

		for (auto& texture : material.m_Textures)
		{
			auto slot = shader12->m_TextureIndexMap[DX12Shader::ROOT_GLOBAL][texture.first];
			auto texture12 = std::static_pointer_cast<DX12Texture>(texture.second.Texture);

			if (texture12 == nullptr)
				continue;

			texture12->ResourceBarrier(m_Context->GetCommandList(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
			commandList->SetComputeRootDescriptorTable(slot, texture12->GetGpuDescriptorHandle());
		}

		for (auto& uavs : material.m_UAVTextures)
		{
			auto slot = shader12->m_UAVIndexMap[DX12Shader::ROOT_GLOBAL][uavs.first];
			auto texture12 = std::static_pointer_cast<DX12Texture>(uavs.second);

			if (texture12 == nullptr)
				continue;

			texture12->ResourceBarrier(m_Context->GetCommandList(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			commandList->SetComputeRootDescriptorTable(slot, texture12->GetGpuDescriptorHandleUAV());
		}

		D3D12_DISPATCH_RAYS_DESC desc = {};

		// Ray Generation program
		desc.RayGenerationShaderRecord.StartAddress = pipelineState.m_ShaderBindingTables[static_cast<size_t>(RayTracingState::TableType::Raygen)]->GetGPUVirtualAddress();
		desc.RayGenerationShaderRecord.SizeInBytes = pipelineState.m_ShaderTableStrideInBytes[static_cast<size_t>(RayTracingState::TableType::Raygen)];
		// Miss program
		desc.MissShaderTable.StartAddress = pipelineState.m_ShaderBindingTables[static_cast<size_t>(RayTracingState::TableType::Miss)]->GetGPUVirtualAddress();
		desc.MissShaderTable.SizeInBytes = pipelineState.m_ShaderTableStrideInBytes[static_cast<size_t>(RayTracingState::TableType::Miss)];
		desc.MissShaderTable.StrideInBytes = pipelineState.m_ShaderTableStrideInBytes[static_cast<size_t>(RayTracingState::TableType::Miss)];
		// Hit program
		desc.HitGroupTable.StartAddress = pipelineState.m_ShaderBindingTables[static_cast<size_t>(RayTracingState::TableType::HitGroup)]->GetGPUVirtualAddress();
		desc.HitGroupTable.SizeInBytes = pipelineState.m_ShaderTableStrideInBytes[static_cast<size_t>(RayTracingState::TableType::HitGroup)];
		desc.HitGroupTable.StrideInBytes = pipelineState.m_ShaderTableStrideInBytes[static_cast<size_t>(RayTracingState::TableType::HitGroup)];

		desc.Width = width;
		desc.Height = height;
		desc.Depth = depth;

		m_Context->DispatchRays(&desc);

		shader12->Unbind();
	}
}

std::shared_ptr<Texture> ChangDXII::GetColorRt()
{
	return m_Pathtracer->GetColorRt();
}

void ChangDXII::SetCamera(const Matrix& view, const Matrix& proj, const Vector3& cameraPosition, const Vector3& forward, const Vector3& up, float ZMin, float ZMax)
{

	// 	m_View = view;
	// 	m_Proj = proj;

		//m_SimpleLighting->SetCamera(projectionToWorld, cameraPosition);

	Matrix viewProj = view * proj;
	Matrix projectionToWorld = XMMatrixInverse(nullptr, viewProj);

	m_Pathtracer->SetCamera(view, proj, cameraPosition, forward, up, ZMin, ZMax);

}

void ChangDXII::DrawLine(const Vector3& from, const Vector3& to, const Vector4& color)
{
	//m_batch->DrawTriangle(VertexPositionColor(Vector3(from.x, from.y, from.z), color), VertexPositionColor(Vector3(to.x, to.y, to.z), color), VertexPositionColor(Vector3(to.x, to.y, to.z), color));
	//m_batch->DrawLine(VertexPositionColor(Vector3(from.x, from.y, from.z), color), VertexPositionColor(Vector3(to.x, to.y, to.z), color));

}

void ChangDXII::AddMeshToScene(std::shared_ptr<Mesh> mesh, const std::vector<std::shared_ptr<Material>>& materials, uint32_t* pOutHitDistribution, bool isRebuild /*= true*/)
{
	//m_SimpleLighting->AddMeshToScene(m_MeshLibrary->GetMesh(name), isRebuild);

	m_Pathtracer->AddMeshToScene(mesh, materials, pOutHitDistribution, isRebuild);
}

void ChangDXII::AddBottomLevelAS(Mesh& mesh, bool allowUpdate /*= false*/)
{
	m_AccelerationStructure->AddBottomLevelAS(
		this->GetContext()->GetDevice(),
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE,
		mesh,
		allowUpdate);
}

void ChangDXII::AddSkinnedBottomLevelAS(uint32_t id, Mesh& mesh, bool allowUpdate /*= false*/)
{
	m_AccelerationStructure->AddSkinnedBottomLevelAS(
		this->GetContext()->GetDevice(),
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE,
		id,
		mesh,
		allowUpdate);
}


void ChangDXII::ComputeSkinnedVertices(Mesh& mesh, const std::vector<Matrix>& boneMatrices, const uint32_t& vertexCount)
{
	m_SkinnedVertexConverter->Compute(mesh, boneMatrices, vertexCount);
}

void ChangDXII::ModifyBLASGeometry(uint32_t id, Mesh& mesh)
{
	m_AccelerationStructure->ModifyBLASGeometry(id, mesh);
}

void ChangDXII::ResetSkinnedBLAS()
{
	m_AccelerationStructure->ResetSkinnedBLAS();
}

UINT ChangDXII::AddBottomLevelASInstance(const std::string& meshName, uint32_t hitDistribution, float* transfrom)
{
	return m_AccelerationStructure->AddBottomLevelASInstance(
		std::wstring(meshName.begin(), meshName.end()),
		hitDistribution,
		transfrom
	);
}

UINT ChangDXII::AddSkinnedBLASInstance(uint32_t id, uint32_t hitDistribution, float* transform)
{
	return m_AccelerationStructure->AddSkinnedBLASInstance(id, hitDistribution, transform);

}

void ChangDXII::UpdateAccelerationStructures()
{
	auto commandList = m_Context->GetCommandList();
	auto frameIndex = m_Context->m_FrameIndex;

	ID3D12GraphicsCommandList4* commandList4;
	commandList->QueryInterface(IID_PPV_ARGS(&commandList4));

	m_AccelerationStructure->Build(commandList4, m_ResourceManager->m_DescriptorAllocator->GetDescriptorHeap().get(), frameIndex);

	// Copy previous frame Bottom Level AS instance transforms to GPU. 
	m_prevFrameBottomLevelASInstanceTransforms.CopyStagingToGpu(frameIndex);

	// Update the CPU staging copy with the current frame transforms.
	const auto& bottomLevelASInstanceDescs = m_AccelerationStructure->GetBottomLevelASInstancesBuffer();
	for (UINT i = 0; i < bottomLevelASInstanceDescs.NumElements(); i++)
	{
		m_prevFrameBottomLevelASInstanceTransforms[i] = *reinterpret_cast<const DirectX::XMFLOAT3X4*>(bottomLevelASInstanceDescs[i].Transform);
	}
}

void ChangDXII::InitializeTopLevelAS()
{
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	bool allowUpdate = false;
	bool performUpdateOnBuild = true;
	m_AccelerationStructure->InitializeTopLevelAS(m_Context->m_Device.get(), buildFlags, allowUpdate, performUpdateOnBuild, L"Top Level Acceleration Structure");
}

void ChangDXII::BeginRender()
{
	m_Context->BeginRender();

	// ========
// 	m_effect->SetWorld(Matrix::Identity);
// 	m_effect->SetView(m_View);
// 	m_effect->SetProjection(m_Proj);
// 
// 	m_effect->Apply(m_Context->GetCommandList());
// 
// 	m_batch->Begin(m_Context->GetCommandList());


}

void ChangDXII::EndRender()
{
	//	m_batch->End();

	m_Context->EndRender();

	m_Context->Present();

	//	m_graphicsMemory->Commit(m_Context->GetCommandObjectManager()->GetCommandQueue().GetCommandQueue());


	m_ShaderLibrary->ResetAllocateState4DX12();


	if (IsRayTracing())
	{
		m_SkinnedVertexConverter->ResetConstantBuffers();
	}
}

std::shared_ptr<DX12Buffer> ChangDXII::CreateVertexBuffer(const void* data, uint32_t count, uint32_t stride)
{
	return m_ResourceManager->CreateVertexBuffer(data, count, stride);
}

std::shared_ptr<DX12Buffer> ChangDXII::CreateIndexBuffer(const void* data, uint32_t count, uint32_t stride)
{
	return m_ResourceManager->CreateIndexBuffer(data, count, stride);
}

void* ChangDXII::GetShaderResourceView()
{
	SIZE_T renderTarget = m_Context->GetRenderTarget();

	return (void*)renderTarget;
}

void ChangDXII::LoadMeshesFromDrive(const std::string& path)
{
	std::filesystem::path directory = path;

	if (!std::filesystem::exists(directory))
	{
		return;
	}

	if (std::filesystem::is_directory(directory))
	{
		m_MeshLibrary->LoadMeshesFromDirectory(path);
	}
	else
	{
		m_MeshLibrary->LoadMeshesFromFile(path);
	}
}

void ChangDXII::LoadMaterialsFromDrive(const std::string& path)
{
	std::filesystem::path directory = path;

	if (!std::filesystem::exists(directory))
	{
		return;
	}

	if (std::filesystem::is_directory(directory))
	{
		m_MaterialLibrary->LoadFromDirectory(path);
	}
	else
	{
		m_MaterialLibrary->LoadFromFile(path);
	}

	for (auto& material : m_MaterialLibrary->GetMaterials())
	{
		if (material.second->m_ShaderString.empty())
		{
			OutputDebugStringA("Material does not have a shader\n");
			continue;
		}
		material.second->m_Shader = LoadShader(material.second->m_ShaderString.c_str());

		if (material.second->m_Shader == nullptr)
		{
			OutputDebugStringA("Shader can't find.\n");
			assert(false);
			continue;
		}

		for (auto& texture : material.second->m_Textures)
		{
			// 창 : 이부분 큐브맵텍스쳐에 대해서도 지원해야한다..아마..
			if (texture.second.Path.empty())
			{
				OutputDebugStringA("Texture does not have a path\n");
				switch (texture.second.Type)
				{
				case Texture::Type::Texture2D:
					texture.second.Path = DEFAULT_TEXTURE2D_PATH;
					texture.second.Texture = m_ResourceManager->CreateTexture(DEFAULT_TEXTURE2D_PATH);
					break;
				case Texture::Type::TextureCube:
					texture.second.Path = DEFAULT_TEXTURECUBE_PATH;
					texture.second.Texture = m_ResourceManager->CreateTextureCube(DEFAULT_TEXTURECUBE_PATH);
					break;
				default:
					break;
				}
			}
			else
			{
				std::filesystem::path texturePath = texture.second.Path;
				if (texturePath.extension().string().empty())
				{

				}
				else
				{
					texture.second.Path = texture.second.Path.c_str();
					texture.second.Texture = Texture::Create(this, texture.second.Path.c_str(), texture.second.Type);
				}
			}

			if (texture.second.Texture == nullptr)
			{
				OutputDebugStringA("Texture does not have a path\n");
				switch (texture.second.Type)
				{
				case Texture::Type::Texture2D:
					texture.second.Path = DEFAULT_TEXTURE2D_PATH;
					texture.second.Texture = m_ResourceManager->CreateTexture(DEFAULT_TEXTURE2D_PATH);
					break;
				case Texture::Type::TextureCube:
					texture.second.Path = DEFAULT_TEXTURECUBE_PATH;
					texture.second.Texture = m_ResourceManager->CreateTextureCube(DEFAULT_TEXTURECUBE_PATH);
					break;
				default:
					break;
				}
			}
		}
	}

}

void ChangDXII::SaveMaterial(const std::string& name)
{
	auto material = m_MaterialLibrary->GetMaterial(name);

	for (auto& texture : material->m_Textures)
	{
		// 창 : 이부분 큐브맵텍스쳐에 대해서도 지원해야한다..아마..
		if (texture.second.Path.empty())
		{
			OutputDebugStringA("Texture does not have a path\n");
			switch (texture.second.Type)
			{
			case Texture::Type::Texture2D:
				texture.second.Path = DEFAULT_TEXTURE2D_PATH;
				texture.second.Texture = m_ResourceManager->CreateTexture(DEFAULT_TEXTURE2D_PATH);
				break;
			case Texture::Type::TextureCube:
				texture.second.Path = DEFAULT_TEXTURECUBE_PATH;
				texture.second.Texture = m_ResourceManager->CreateTextureCube(DEFAULT_TEXTURECUBE_PATH);
				break;
			default:
				break;
			}
		}
		else
		{
			texture.second.Path = texture.second.Path.c_str();
			texture.second.Texture = Texture::Create(this, texture.second.Path.c_str(), texture.second.Type);
		}

		if (texture.second.Texture == nullptr)
		{
			OutputDebugStringA("Texture does not have a path\n");
			switch (texture.second.Type)
			{
			case Texture::Type::Texture2D:
				texture.second.Path = DEFAULT_TEXTURE2D_PATH;
				texture.second.Texture = m_ResourceManager->CreateTexture(DEFAULT_TEXTURE2D_PATH);
				break;
			case Texture::Type::TextureCube:
				texture.second.Path = DEFAULT_TEXTURECUBE_PATH;
				texture.second.Texture = m_ResourceManager->CreateTextureCube(DEFAULT_TEXTURECUBE_PATH);
				break;
			default:
				break;
			}
		}

		m_MaterialLibrary->SaveMaterial(name);
	}

}

void ChangDXII::LoadAnimationClipsFromDrive(const std::string& path)
{
	std::filesystem::path directory = path;

	if (!std::filesystem::exists(directory))
		return;

	if (std::filesystem::is_directory(directory))
		m_AnimationLibrary->LoadAnimationClipsFromDirectory(path);
	else
		m_AnimationLibrary->LoadAnimationClipsFromFile(path);
}

void* ChangDXII::AllocateShaderVisibleDescriptor(void* OutCpuHandle, void* OutGpuHandle)
{
	auto& allocator = m_ResourceManager->m_DescriptorAllocator;
	auto cpu = allocator->Allocate();
	auto gpu = allocator->GetGPUHandleFromCPUHandle(cpu);

	auto heap = allocator->GetDescriptorHeap();

	*reinterpret_cast<D3D12_CPU_DESCRIPTOR_HANDLE*>(OutCpuHandle) = cpu;
	*reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(OutGpuHandle) = gpu;

	return heap.get();
}

void ChangDXII::DoRayTracing()
{
	//m_SimpleLighting->DispatchRays(GetWidth(), GetHeight());

	m_Pathtracer->Run();
}

void ChangDXII::FlushRaytracingData()
{
	m_Pathtracer->FlushRaytracingData();
}

void ChangDXII::ResetAS()
{
	m_AccelerationStructure->ResetBottomLevelASInstances();
}

std::shared_ptr<Texture> ChangDXII::GetTexture(const char* path)
{
	return m_ResourceManager->GetTexture(path);
}

std::shared_ptr<Texture> ChangDXII::CreateTexture(const char* path)
{
	return m_ResourceManager->CreateTexture(path);
}


std::shared_ptr<Texture> ChangDXII::CreateTextureCube(const char* path)
{
	return m_ResourceManager->CreateTextureCube(path);
}

std::shared_ptr<Texture> ChangDXII::CreateEmptyTexture(
	std::string name, Texture::Type type, uint32_t width,
	uint32_t height, uint32_t mipLevels, Texture::Format format,
	Texture::Usage usage /*= Texture::Usage::NONE*/, float* clearColor /*= nullptr*/, Texture::UAVType uavType /*= Texture::UAVType::NONE*/
	, uint32_t arraySize /*= 1*/)
{
	return m_ResourceManager->CreateEmptyTexture(name, type, width, height, mipLevels, format, usage, clearColor, arraySize);
}

std::shared_ptr<Texture> ChangDXII::CreateEmptyTexture(const TextureDesc& desc)
{
	return m_ResourceManager->CreateEmptyTexture(desc);
}

void ChangDXII::ReleaseTexture(const std::string& name)
{
	m_ResourceManager->ReleaseTexture(name);
}

std::shared_ptr<Shader> ChangDXII::LoadShader(const char* srcPath)
{
	auto shader = m_ShaderLibrary->LoadShader(this, srcPath);
	auto shader12 = std::static_pointer_cast<DX12Shader>(shader);
	if (shader)
	{
		if (shader12->m_ShaderBlob[ShaderStage::CS])
		{
			m_ComputePipelineStates.emplace(shader->ID, ComputePipelineState(this, shader12));
		}
		else if (shader12->m_ShaderBlob[ShaderStage::RayTracing])
		{
			// 레이트레이싱의 리플렉션은 포기한다.
			//m_RayTracingPipelineStates.emplace(shader->ID, RayTracingState(this, shader12));
		}
		else
		{
			m_PipelineStates.emplace(shader->ID, PipelineStateArray());
		}
		return shader;
	}
	else
	{
		OutputDebugStringA("Failed to load shader\n");
		return nullptr;
	}
}

void ChangDXII::LoadShadersFromDrive(const std::string& path)
{
	std::filesystem::path shaderPath = path;
	if (!std::filesystem::exists(shaderPath))
	{
		OutputDebugStringA("Shader path does not exist\n");
		return;
	}

	for (const auto& entry : std::filesystem::recursive_directory_iterator(shaderPath))
	{
		if (entry.is_regular_file())
		{
			auto path = entry.path();
			auto extension = path.extension();
			if (extension == ".hlsl")
			{
				auto pathStirng = path.string();
				std::replace(pathStirng.begin(), pathStirng.end(), '\\', '/');
				LoadShader(pathStirng.c_str());
			}
		}
	}
}
