#include "pch.h"
#include "SkinnedVertexConverter.h"
#include "ChangDXII.h"
#include "DX12Helper.h"
#include "DX12Shader.h"

namespace RootSig
{
	namespace Slot
	{
		enum Enum
		{
			gVertices,
			gBoneWeights,
			gOutputVertices,
			BoneMatrices,
			VertexCount,
			Count,
		};
	}
}

void SkinnedVertexConverter::SetUp(ChangDXII* renderer, DX12Shader* shader)
{
	m_Renderer = renderer;
	m_Shader = shader;

	CreateRootSignatures();

	CreatePipelineState();

	CreateConstantBuffers();

}

void SkinnedVertexConverter::Compute(Mesh& mesh, const std::vector<Matrix>& boneMatrices, const uint32_t& vertexCount)
{
	auto shader = m_Shader;
	auto commandList = m_Renderer->GetContext()->GetCommandList();
	DX12VertexBuffer* vertexBuffer12 = static_cast<DX12VertexBuffer*>(mesh.vertexBuffer.get());
	auto descriptorHeap = m_Renderer->GetShaderVisibleHeap();

	commandList->SetPipelineState(m_PipelineState.get());

	commandList->SetComputeRootSignature(m_RootSignature.get());

	commandList->SetDescriptorHeaps(1, &descriptorHeap);

	// 여기서 본매트릭스를 업데이트 해준다.
	auto boneMatrixPointer = m_BoneMatrices->Allocate();
	memcpy(boneMatrixPointer, boneMatrices.data(), sizeof(Matrix) * boneMatrices.size());

	auto vertexCountPointer = m_VertexCount->Allocate();
	memcpy(vertexCountPointer, &vertexCount, sizeof(uint32_t));


	commandList->SetComputeRootDescriptorTable(RootSig::Slot::gVertices, vertexBuffer12->GetSRVGpuHandle());
	commandList->SetComputeRootShaderResourceView(RootSig::Slot::gBoneWeights, vertexBuffer12->m_BoneWeights.GpuVirtualAddress());
	vertexBuffer12->m_OutputVertexBuffer->ResourceBarrier(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	commandList->SetComputeRootDescriptorTable(RootSig::Slot::gOutputVertices, vertexBuffer12->m_OutputVertexBuffer->GetGpuDescriptorHandleUAV());
	commandList->SetComputeRootConstantBufferView(RootSig::Slot::BoneMatrices, m_BoneMatrices->GetAllocatedInstance().GpuMemoryAddress);
	commandList->SetComputeRootConstantBufferView(RootSig::Slot::VertexCount, m_VertexCount->GetAllocatedInstance().GpuMemoryAddress);

	auto threadGroupCountX = (vertexCount / 64) + (vertexCount % 64 ? 1 : 0);

	commandList->Dispatch(threadGroupCountX, 1, 1);

	vertexBuffer12->m_OutputVertexBuffer->ResourceBarrier(commandList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	// 		m_Context->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);


}

void SkinnedVertexConverter::ResetConstantBuffers()
{
	m_BoneMatrices->Reset();
	m_VertexCount->Reset();
}

void SkinnedVertexConverter::CreateRootSignatures()
{
	auto device = m_Renderer->GetContext()->GetDevice();

	{
		using namespace RootSig;

		CD3DX12_DESCRIPTOR_RANGE ranges[Slot::Count];
		ranges[Slot::gVertices].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		ranges[Slot::gOutputVertices].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);


		CD3DX12_ROOT_PARAMETER rootParameters[Slot::Count];
		rootParameters[Slot::gVertices].InitAsDescriptorTable(1, &ranges[Slot::gVertices]);
		rootParameters[Slot::gBoneWeights].InitAsShaderResourceView(1);
		rootParameters[Slot::gOutputVertices].InitAsDescriptorTable(1, &ranges[Slot::gOutputVertices]);
		rootParameters[Slot::BoneMatrices].InitAsConstantBufferView(0);
		rootParameters[Slot::VertexCount].InitAsConstantBufferView(1);

		CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
		SerializeAndCreateRootSignature(device, globalRootSignatureDesc, &m_RootSignature, L"Global root signature");
	}
}

void SkinnedVertexConverter::CreatePipelineState()
{
	auto device = m_Renderer->GetContext()->GetDevice();

	IDxcBlob* ComputeShaderBlob = m_Shader->m_ShaderBlob[ShaderStage::CS].get();

	D3D12_COMPUTE_PIPELINE_STATE_DESC descComputePSO = {};
	descComputePSO.pRootSignature = m_RootSignature.get();
	descComputePSO.CS = CD3DX12_SHADER_BYTECODE(ComputeShaderBlob->GetBufferPointer(), ComputeShaderBlob->GetBufferSize());

	ThrowIfFailed(device->CreateComputePipelineState(&descComputePSO, IID_PPV_ARGS(&m_PipelineState)));
}

void SkinnedVertexConverter::CreateConstantBuffers()
{
	m_BoneMatrices = DX12ConstantBuffer::Create(m_Renderer, sizeof(Matrix) * 96);
	m_VertexCount = DX12ConstantBuffer::Create(m_Renderer, sizeof(uint32_t));

}
