#pragma once

#include "DX12Buffer.h"
#include "RaytracingHlslCompat.h"

class ChangDXII;
class DX12Shader;

class SimpleLighting
{
	struct SceneConstantBuffer
	{
		XMMATRIX projectionToWorld;
		XMVECTOR cameraPosition;
		XMVECTOR lightPosition;
		XMVECTOR lightAmbientColor;
		XMVECTOR lightDiffuseColor;
	};

	struct CubeConstantBuffer
	{
		XMFLOAT4 albedo;
	};

	struct Offsets
	{
		uint32_t vertexOffset;
		uint32_t indexOffset;
	};

public:
	// We'll allocate space for several of these and they will need to be padded for alignment.
	static_assert(sizeof(SceneConstantBuffer) < D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, "Checking the size here.");




	SimpleLighting() {}

	void SetUp(ChangDXII* renderer, DX12Shader* shader);

	void SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, winrt::com_ptr<ID3D12RootSignature>* rootSig);
	void CreateRootSignatures();
	void CreateConstantBuffers();
	void CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline);
	void CreateRaytracingPipelineStateObject();
	void BuildShaderTables();
	void DispatchRays(UINT width = 0, UINT height = 0);
	void CreateRaytracingOutputResource();

	void AddMeshToScene(std::shared_ptr<Mesh> mesh, bool isRebuild);

	void SetOutputTexture(std::shared_ptr<Texture> texture) { m_OutputTexture = texture; }

	void SetCamera(const XMMATRIX& projectionToWorld, const XMVECTOR& cameraPosition);



private:
	inline static const wchar_t* c_hitGroupName = L"MyHitGroup";
	inline static const wchar_t* c_raygenShaderName = L"MyRaygenShader";
	inline static const wchar_t* c_closestHitShaderName = L"MyClosestHitShader";
	inline static const wchar_t* c_missShaderName = L"MyMissShader";
	winrt::com_ptr<ID3D12Resource> m_missShaderTable;
	winrt::com_ptr<ID3D12Resource> m_hitGroupShaderTable;
	winrt::com_ptr<ID3D12Resource> m_rayGenShaderTable;

	// Raytracing pipeline objects
	UINT m_rayGenShaderTableRecordSizeInBytes;
	UINT m_hitGroupShaderTableRecordSizeInBytes = UINT_MAX;
	UINT m_missShaderTableRecordSizeInBytes = UINT_MAX;

	union AlignedSceneConstantBuffer
	{
		SceneConstantBuffer constants;
		uint8_t alignmentPadding[D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT];
	};
	AlignedSceneConstantBuffer* m_mappedConstantData;
	winrt::com_ptr<ID3D12Resource> m_perFrameConstants;

	ChangDXII* m_Renderer;
	DX12Shader* m_RaytracingShader;

	winrt::com_ptr<ID3D12StateObject> m_dxrStateObject;

	SceneConstantBuffer m_sceneCB[2];
	CubeConstantBuffer m_cubeCB;

	std::shared_ptr<Texture> m_OutputTexture;

	// Root signatures
	winrt::com_ptr<ID3D12RootSignature> m_raytracingGlobalRootSignature;
	winrt::com_ptr<ID3D12RootSignature> m_raytracingLocalRootSignature;

	std::vector<std::shared_ptr<Mesh>> m_Meshes;
};

