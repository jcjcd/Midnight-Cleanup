#pragma once

class ChangDXII;
class DX12Shader;
class DX12ConstantBuffer;


#include "Mesh.h"

class SkinnedVertexConverter
{
public:
	SkinnedVertexConverter() = default;

	void SetUp(ChangDXII* renderer, DX12Shader* shader);

	void Compute(Mesh& mesh, const std::vector<Matrix>& boneMatrices, const uint32_t& vertexCount);
	
	void ResetConstantBuffers();

private:
	void CreateRootSignatures();

	void CreatePipelineState();

	void CreateConstantBuffers();

private:	
	ChangDXII* m_Renderer = nullptr;
	DX12Shader* m_Shader = nullptr;

	winrt::com_ptr<ID3D12RootSignature> m_RootSignature;
	winrt::com_ptr<ID3D12PipelineState> m_PipelineState;

	std::shared_ptr<DX12ConstantBuffer> m_BoneMatrices;
	std::shared_ptr<DX12ConstantBuffer> m_VertexCount;

	// 여기다가 컨테이너를 만들어서 자료를 모을수도 있긴하다.
};

