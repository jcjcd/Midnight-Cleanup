#pragma once

#include "DX12Buffer.h"
#include "RaytracingHlslCompat.h"
#include "GpuKernels.h"

class ChangDXII;
class DX12Shader;
class Material;

namespace RayGenShaderType {
	enum Enum {
		Pathtracer = 0,
		Count
	};
}


namespace GBufferResource {
	enum Enum {
		HitPosition = 0,	// 3D position of hit.
		SurfaceNormalDepth,	// Encoded normal and linear depth.
		Depth,          // Linear depth of the hit.
		PartialDepthDerivatives,
		MotionVector,
		ReprojectedNormalDepth,
		Color,
		AOSurfaceAlbedo,
		Count
	};
}




class Pathtracer
{
public:
	Pathtracer();

	void SetUp(ChangDXII* renderer, DX12Shader* shader);

	uint32_t Width() { return m_raytracingWidth; }
	uint32_t Height() { return m_raytracingHeight; }

	void DispatchRays(ID3D12Resource* rayGenShaderTable, UINT width = 0, UINT height = 0);
	void CreateResolutionDependentResources();
	void SetResolution(UINT GBufferWidth, UINT GBufferHeight, UINT RTAOWidth, UINT RTAOHeight);
	void CreateTextureResources();
	void DownsampleGBuffer();

	void SetCamera(const Matrix& view, const Matrix& proj, const Vector3& cameraPosition, const Vector3& forward, const Vector3& up, float ZMin, float ZMax);
	void UpdateConstantBuffer();
	void Run();

	void BuildShaderTables();

	void AddMeshToScene(std::shared_ptr<Mesh> mesh, const std::vector<std::shared_ptr<Material>>& material, uint32_t* pOutHitDistribution, bool isRebuild);

	void SetEnvironmentMap(std::shared_ptr<Texture> texture) { m_EnvironmentMap = static_pointer_cast<DX12Texture>(texture); }

	std::shared_ptr<DX12Texture> GetColorRt() { return m_GBufferResources[GBufferResource::Color]; }

	void FlushRaytracingData() { m_MeshMaterialHitDistributions.clear(); }

private:
	void CreateAuxilaryDeviceResources();
	void CreateConstantBuffers();
	void CreateRootSignatures();
	void CreateDxilLibrarySubobject(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline);
	void CreateHitGroupSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline);
	void CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline);
	void CreateRaytracingPipelineStateObject();

private:
	ChangDXII* m_Renderer;
	DX12Shader* m_RaytracingShader;

	uint32_t m_raytracingWidth = 0;
	uint32_t m_raytracingHeight = 0;
	uint32_t m_quarterResWidth = 0;
	uint32_t m_quarterResHeight = 0;

	// DirectX Raytracing (DXR) attributes
	winrt::com_ptr<ID3D12StateObject> m_dxrStateObject;

	// Shader tables
	winrt::com_ptr<ID3D12Resource> m_rayGenShaderTables[RayGenShaderType::Count];
	UINT m_rayGenShaderTableRecordSizeInBytes[RayGenShaderType::Count];
	winrt::com_ptr<ID3D12Resource> m_hitGroupShaderTable;
	UINT m_hitGroupShaderTableStrideInBytes = UINT_MAX;
	winrt::com_ptr<ID3D12Resource> m_missShaderTable;
	UINT m_missShaderTableStrideInBytes = UINT_MAX;

	// Root signatures
	winrt::com_ptr<ID3D12RootSignature> m_raytracingGlobalRootSignature;
	winrt::com_ptr<ID3D12RootSignature> m_raytracingLocalRootSignature;

	// Raytracing resources.
	ConstantBuffer<PathtracerConstantBuffer> m_CB;
	std::shared_ptr<DX12Texture> m_GBufferResources[GBufferResource::Count];
	std::shared_ptr<DX12Texture> m_GBufferQuarterResResources[GBufferResource::Count];

	std::shared_ptr<DX12Texture> m_EnvironmentMap;

	D3D12_GPU_DESCRIPTOR_HANDLE m_nullVertexBufferGPUhandle;


	// Raytracing args
	// 창 : 이거 변경 가능하게 만들어야함
	// 그리고 이게 렌더러에 있는게 맞는지 생각해볼것. 위치
	Vector3 m_LightPosition = Vector3(-1000.0f, 100.0f, 0.0f);
	Vector3 m_LightColor = Vector3(1.0f, 1.0f, 1.0f);

	uint32_t m_maxRadianceRayRecursionDepth = 5;
	uint32_t m_maxShadowRayRecursionDepth = 3;
	CompositionType m_compositionType = CompositionType::PBRShading;
	bool m_UseNormalMap = true;
	float m_DefaultAmbientIntensity = 0.4f; // 0.f ~ 1.f
	bool m_UseAlbedoFromMaterial = true;

	Matrix m_CameraViewProj = Matrix::Identity;
	Matrix m_WorldWithCameraEyeAtOrigin = Matrix::Identity;
	Vector3 m_CameraPosition = Vector3(0.0f, 0.0f, 0.0f);

	Matrix m_PrevWorldWithCameraEyeAtOrigin = Matrix::Identity;
	Matrix m_PrevCameraViewProj = Matrix::Identity;
	Vector3 m_PrevCameraPosition = Vector3(0.0f, 0.0f, 0.0f);


	bool m_isRecreateRaytracingResourcesRequested = false;



	// Raytracing shaders.
	static const wchar_t* c_rayGenShaderNames[RayGenShaderType::Count];
	static const wchar_t* c_closestHitShaderNames[PathtracerRayType::Count];
	static const wchar_t* c_missShaderNames[PathtracerRayType::Count];
	static const wchar_t* c_hitGroupNames[PathtracerRayType::Count];


	GpuKernels::CalculatePartialDerivatives  m_calculatePartialDerivativesKernel;
	GpuKernels::DownsampleGBufferDataBilateralFilter m_downsampleGBufferBilateralFilterKernel;

	using MeshMaterialHitDistribution = std::tuple<std::shared_ptr<Mesh>, std::vector<std::shared_ptr<Material>>, uint32_t*>; // Mesh, Material, (out)HitDistribution

	std::vector<MeshMaterialHitDistribution> m_MeshMaterialHitDistributions;
};

