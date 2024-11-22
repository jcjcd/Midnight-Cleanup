#include "pch.h"
#include "Pathtracer.h"

#include "ChangDXII.h"
#include "DX12Shader.h"
#include "DX12Texture.h"
#include "Mesh.h"

// Shader entry points.
const wchar_t* Pathtracer::c_rayGenShaderNames[] = { L"MyRayGenShader_RadianceRay" };
const wchar_t* Pathtracer::c_closestHitShaderNames[] = { L"MyClosestHitShader_RadianceRay", L"MyClosestHitShader_ShadowRay" };
const wchar_t* Pathtracer::c_missShaderNames[] = { L"MyMissShader_RadianceRay", L"MyMissShader_ShadowRay" };

// Hit groups.
const wchar_t* Pathtracer::c_hitGroupNames[] = { L"MyHitGroup_Triangle_RadianceRay", L"MyHitGroup_Triangle_ShadowRay" };

#define MAX_RAY_RECURSION_DEPTH 5

#ifdef _DEBUG
//#define DEBUG_PRINT
#endif

namespace GlobalRootSignature {
	namespace Slot {
		enum Enum {
			Output = 0,
			GBufferResources,
			AccelerationStructure,
			ConstantBuffer,
			SampleBuffers,
			EnvironmentMap,
			GbufferNormalRGB,
			PrevFrameBottomLevelASIstanceTransforms,
			MotionVector,
			ReprojectedNormalDepth,
			Color,
			AOSurfaceAlbedo,
			Debug1,
			Debug2,
			Count
		};
	}
}

struct Offsets
{
	uint32_t vertexOffset;
	uint32_t indexOffset;
};

namespace LocalRootSignature {
	namespace Slot {
		enum Enum {
			ConstantBuffer = 0,
			IndexBuffer,
			VertexBuffer,
			PreviousFrameVertexBuffer,
			DiffuseTexture,
			NormalTexture,
			MetallicTexture,
			RoughnessTexture,
			OpacityTexture,
			OffsetsBuffer,
			Count
		};
	}
	struct RootArguments {
		PrimitiveMaterialBuffer cb;
		// Bind each resource via a descriptor.
		// This design was picked for simplicity, but one could optimize for shader record size by:
		//    1) Binding multiple descriptors via a range descriptor instead.
		//    2) Storing 4 Byte indices (instead of 8 Byte descriptors) to a global pool resources.
		D3D12_GPU_DESCRIPTOR_HANDLE indexBufferGPUHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE vertexBufferGPUHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE previousFrameVertexBufferGPUHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE diffuseTextureGPUHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE normalTextureGPUHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE metallicTextureGPUHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE roughnessTextureGPUHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE opacityTextureGPUHandle;
		Offsets offsets;
	};
}


Pathtracer::Pathtracer()
{
	for (auto& rayGenShaderTableRecordSizeInBytes : m_rayGenShaderTableRecordSizeInBytes)
	{
		rayGenShaderTableRecordSizeInBytes = UINT_MAX;
	}
}

void Pathtracer::SetUp(ChangDXII* renderer, DX12Shader* shader)
{
	m_Renderer = renderer;
	m_RaytracingShader = shader;

	CreateAuxilaryDeviceResources();

	CreateRootSignatures();

	CreateRaytracingPipelineStateObject();

	CreateConstantBuffers();

}

void Pathtracer::CreateAuxilaryDeviceResources()
{
	auto device = m_Renderer->GetContext()->GetDevice();
	auto FrameCount = m_Renderer->GetContext()->GetBackBufferCount();

	m_calculatePartialDerivativesKernel.Initialize(device, FrameCount);
	m_downsampleGBufferBilateralFilterKernel.Initialize(device, FrameCount);
}

// Create constant buffers.
void Pathtracer::CreateConstantBuffers()
{
	auto device = m_Renderer->GetContext()->GetDevice();
	auto FrameCount = m_Renderer->GetContext()->GetBackBufferCount();

	m_CB.Create(device, FrameCount, L"Pathtracer Constant Buffer");
}


void Pathtracer::CreateRootSignatures()
{
	auto device = m_Renderer->GetContext()->GetDevice();

	// Global Root Signature
	// This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
	{
		using namespace GlobalRootSignature;

		CD3DX12_DESCRIPTOR_RANGE ranges[Slot::Count]; // Perfomance TIP: Order from most frequent to least frequent.
		ranges[Slot::Output].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);  // 1 output textures
		ranges[Slot::GBufferResources].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 3, 7);  // 3 output GBuffer textures
		ranges[Slot::GbufferNormalRGB].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 14);  // 1 output normal texture
		ranges[Slot::MotionVector].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 17);  // 1 output texture space motion vector.
		ranges[Slot::ReprojectedNormalDepth].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 18);  // 1 output texture reprojected hit position
		ranges[Slot::Color].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 19);  // 1 output texture shaded color
		ranges[Slot::AOSurfaceAlbedo].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 20);  // 1 output texture AO diffuse
		ranges[Slot::Debug1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 21);
		ranges[Slot::Debug2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 22);

		ranges[Slot::EnvironmentMap].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 12);  // 1 input environment map texture

		CD3DX12_ROOT_PARAMETER rootParameters[Slot::Count];
		rootParameters[Slot::Output].InitAsDescriptorTable(1, &ranges[Slot::Output]);
		rootParameters[Slot::GBufferResources].InitAsDescriptorTable(1, &ranges[Slot::GBufferResources]);
		rootParameters[Slot::EnvironmentMap].InitAsDescriptorTable(1, &ranges[Slot::EnvironmentMap]);
		rootParameters[Slot::GbufferNormalRGB].InitAsDescriptorTable(1, &ranges[Slot::GbufferNormalRGB]);
		rootParameters[Slot::MotionVector].InitAsDescriptorTable(1, &ranges[Slot::MotionVector]);
		rootParameters[Slot::ReprojectedNormalDepth].InitAsDescriptorTable(1, &ranges[Slot::ReprojectedNormalDepth]);
		rootParameters[Slot::Color].InitAsDescriptorTable(1, &ranges[Slot::Color]);
		rootParameters[Slot::AOSurfaceAlbedo].InitAsDescriptorTable(1, &ranges[Slot::AOSurfaceAlbedo]);
		rootParameters[Slot::Debug1].InitAsDescriptorTable(1, &ranges[Slot::Debug1]);
		rootParameters[Slot::Debug2].InitAsDescriptorTable(1, &ranges[Slot::Debug2]);

		rootParameters[Slot::AccelerationStructure].InitAsShaderResourceView(0);
		rootParameters[Slot::ConstantBuffer].InitAsConstantBufferView(0);
		rootParameters[Slot::SampleBuffers].InitAsShaderResourceView(4);
		rootParameters[Slot::PrevFrameBottomLevelASIstanceTransforms].InitAsShaderResourceView(15);

		CD3DX12_STATIC_SAMPLER_DESC staticSamplers[] =
		{
			// LinearWrapSampler
			CD3DX12_STATIC_SAMPLER_DESC(0, SAMPLER_FILTER),
		};

		CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters, ARRAYSIZE(staticSamplers), staticSamplers);
		SerializeAndCreateRootSignature(device, globalRootSignatureDesc, &m_raytracingGlobalRootSignature, L"Global root signature");
	}

	// Local Root Signature
	// This is a root signature that enables a shader to have unique arguments that come from shader tables.
	{
		// Triangle geometry
		{
			using namespace LocalRootSignature;

			CD3DX12_DESCRIPTOR_RANGE ranges[Slot::Count]; // Perfomance TIP: Order from most frequent to least frequent.
			ranges[Slot::IndexBuffer].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1);  // 1 buffer - index buffer.
			ranges[Slot::VertexBuffer].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 1);  // 1 buffer - current frame vertex buffer.
			ranges[Slot::PreviousFrameVertexBuffer].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 1);  // 1 buffer - previous frame vertex buffer.
			ranges[Slot::DiffuseTexture].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 1);  // 1 diffuse texture
			ranges[Slot::NormalTexture].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4, 1);  // 1 normal texture
			ranges[Slot::MetallicTexture].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5, 1);  // 1 metallic texture
			ranges[Slot::RoughnessTexture].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6, 1);  // 1 roughness texture
			ranges[Slot::OpacityTexture].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 7, 1);  // 1 opacity texture

			CD3DX12_ROOT_PARAMETER rootParameters[Slot::Count];
			rootParameters[Slot::ConstantBuffer].InitAsConstants(SizeOfInUint32(PrimitiveMaterialBuffer), 0, 1);
			rootParameters[Slot::IndexBuffer].InitAsDescriptorTable(1, &ranges[Slot::IndexBuffer]);
			rootParameters[Slot::VertexBuffer].InitAsDescriptorTable(1, &ranges[Slot::VertexBuffer]);
			rootParameters[Slot::PreviousFrameVertexBuffer].InitAsDescriptorTable(1, &ranges[Slot::PreviousFrameVertexBuffer]);
			rootParameters[Slot::DiffuseTexture].InitAsDescriptorTable(1, &ranges[Slot::DiffuseTexture]);
			rootParameters[Slot::NormalTexture].InitAsDescriptorTable(1, &ranges[Slot::NormalTexture]);
			rootParameters[Slot::MetallicTexture].InitAsDescriptorTable(1, &ranges[Slot::MetallicTexture]);
			rootParameters[Slot::RoughnessTexture].InitAsDescriptorTable(1, &ranges[Slot::RoughnessTexture]);
			rootParameters[Slot::OpacityTexture].InitAsDescriptorTable(1, &ranges[Slot::OpacityTexture]);
			rootParameters[Slot::OffsetsBuffer].InitAsConstants(SizeOfInUint32(Offsets), 1, 1);

			CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
			localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
			SerializeAndCreateRootSignature(device, localRootSignatureDesc, &m_raytracingLocalRootSignature, L"Local root signature");
		}
	}
}


// DXIL library
// This contains the shaders and their entrypoints for the state object.
// Since shaders are not considered a subobject, they need to be passed in via DXIL library subobjects.
void Pathtracer::CreateDxilLibrarySubobject(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline)
{
	auto lib = raytracingPipeline->CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	D3D12_SHADER_BYTECODE libdxil = CD3DX12_SHADER_BYTECODE(
		m_RaytracingShader->m_ShaderBlob[ShaderStage::RayTracing]->GetBufferPointer(),
		m_RaytracingShader->m_ShaderBlob[ShaderStage::RayTracing]->GetBufferSize());
	lib->SetDXILLibrary(&libdxil);
	// Use default shader exports for a DXIL library/collection subobject ~ surface all shaders.
}

// Hit groups
// A hit group specifies closest hit, any hit and intersection shaders 
// to be executed when a ray intersects the geometry.
void Pathtracer::CreateHitGroupSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline)
{
	// Triangle geometry hit groups
	{
		for (UINT rayType = 0; rayType < PathtracerRayType::Count; rayType++)
		{
			auto hitGroup = raytracingPipeline->CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();

			if (c_closestHitShaderNames[rayType])
			{

				hitGroup->SetClosestHitShaderImport(c_closestHitShaderNames[rayType]);
			}
			hitGroup->SetHitGroupExport(c_hitGroupNames[rayType]);
			hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
		}
	}
}


// Local root signature and shader association
// This is a root signature that enables a shader to have unique arguments that come from shader tables.
void Pathtracer::CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline)
{
	// Ray gen and miss shaders in this sample are not using a local root signature and thus one is not associated with them.

	// Hit groups
	// Triangle geometry
	{
		auto localRootSignature = raytracingPipeline->CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
		localRootSignature->SetRootSignature(m_raytracingLocalRootSignature.get());
		// Shader association
		auto rootSignatureAssociation = raytracingPipeline->CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
		rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
		rootSignatureAssociation->AddExports(c_hitGroupNames);
	}
}

// Create a raytracing pipeline state object (RTPSO).
// An RTPSO represents a full set of shaders reachable by a DispatchRays() call,
// with all configuration options resolved, such as local root signatures and other state.
void Pathtracer::CreateRaytracingPipelineStateObject()
{
	auto device = m_Renderer->GetContext()->GetDevice();
	// Pathracing state object.
	{
		CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

		// DXIL library
		CreateDxilLibrarySubobject(&raytracingPipeline);

		// Hit groups
		CreateHitGroupSubobjects(&raytracingPipeline);

		// Shader config
		// Defines the maximum sizes in bytes for the ray rayPayload and attribute structure.
		auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
		UINT payloadSize = static_cast<UINT>(max(sizeof(ShadowRayPayload), sizeof(PathtracerRayPayload)));

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
		UINT maxRecursionDepth = MAX_RAY_RECURSION_DEPTH;
		pipelineConfig->Config(maxRecursionDepth);

		PrintStateObjectDesc(raytracingPipeline);

		// Create the state object.
		ThrowIfFailed(device->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(&m_dxrStateObject)), L"Couldn't create DirectX Raytracing state object.\n");
	}
}

void Pathtracer::SetCamera(const Matrix& view, const Matrix& proj, const Vector3& cameraPosition, const Vector3& forward, const Vector3& up, float ZMin, float ZMax)
{
	Matrix worldWithCameraEyeAtOrigin;
	worldWithCameraEyeAtOrigin = XMMatrixLookAtLH(XMVectorSet(0, 0, 0, 1), XMVectorSetW(forward, 1), up);
	Matrix viewProj = worldWithCameraEyeAtOrigin * proj;
	m_CB->Znear = ZMin;
	m_CB->Zfar = ZMax;
	m_CB->projectionToWorldWithCameraAtOrigin = viewProj.Invert();
	XMStoreFloat3(&m_CB->cameraPosition, cameraPosition);

	m_PrevCameraViewProj = m_CameraViewProj;
	m_PrevCameraPosition = m_CameraPosition;
	m_PrevWorldWithCameraEyeAtOrigin = m_WorldWithCameraEyeAtOrigin;

	m_CameraViewProj = view * proj;
	m_WorldWithCameraEyeAtOrigin = m_CB->projectionToWorldWithCameraAtOrigin;
	m_CameraPosition = cameraPosition;
}

void Pathtracer::UpdateConstantBuffer()
{
	XMStoreFloat3(&m_CB->lightPosition, m_LightPosition);
	m_CB->lightColor = m_LightColor;

	// 외부호출로 변경
	//SetCamera(scene.Camera());

	m_CB->maxRadianceRayRecursionDepth = m_maxRadianceRayRecursionDepth;

	if (m_compositionType == PBRShading)
		m_CB->maxShadowRayRecursionDepth = m_maxShadowRayRecursionDepth;
	else
		// Casting shadow rays at multiple TraceRay recursion depths is expensive. Skip if it the result is not getting rendered at composition.
		m_CB->maxShadowRayRecursionDepth = 0;

	m_CB->useNormalMaps = m_UseNormalMap;
	m_CB->defaultAmbientIntensity = m_DefaultAmbientIntensity;
	m_CB->useBaseAlbedoFromMaterial = m_UseAlbedoFromMaterial;

	m_CB->prevFrameViewProj = m_PrevCameraViewProj;
	XMStoreFloat3(&m_CB->prevFrameCameraPosition, m_PrevCameraPosition);

	m_CB->prevFrameProjToViewCameraAtOrigin = m_PrevWorldWithCameraEyeAtOrigin;
}

void Pathtracer::DispatchRays(ID3D12Resource* rayGenShaderTable, UINT width, UINT height)
{
	ID3D12GraphicsCommandList4* commandList;
	m_Renderer->GetContext()->GetCommandList()->QueryInterface(IID_PPV_ARGS(&commandList));

	auto frameIndex = m_Renderer->GetContext()->GetCurrentFrameIndex();

	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	if (m_hitGroupShaderTable)
	{
		dispatchDesc.HitGroupTable.StartAddress = m_hitGroupShaderTable->GetGPUVirtualAddress();
		dispatchDesc.HitGroupTable.SizeInBytes = m_hitGroupShaderTable->GetDesc().Width;
		dispatchDesc.HitGroupTable.StrideInBytes = m_hitGroupShaderTableStrideInBytes;
	}
	dispatchDesc.MissShaderTable.StartAddress = m_missShaderTable->GetGPUVirtualAddress();
	dispatchDesc.MissShaderTable.SizeInBytes = m_missShaderTable->GetDesc().Width;
	dispatchDesc.MissShaderTable.StrideInBytes = m_missShaderTableStrideInBytes;
	dispatchDesc.RayGenerationShaderRecord.StartAddress = rayGenShaderTable->GetGPUVirtualAddress();
	dispatchDesc.RayGenerationShaderRecord.SizeInBytes = rayGenShaderTable->GetDesc().Width;
	dispatchDesc.Width = width != 0 ? width : m_raytracingWidth;
	dispatchDesc.Height = height != 0 ? height : m_raytracingHeight;
	dispatchDesc.Depth = 1;
	commandList->SetPipelineState1(m_dxrStateObject.get());

	commandList->DispatchRays(&dispatchDesc);
}

void Pathtracer::CreateResolutionDependentResources()
{
	CreateTextureResources();
}

void Pathtracer::SetResolution(UINT GBufferWidth, UINT GBufferHeight, UINT RTAOWidth, UINT RTAOHeight)
{
	m_raytracingWidth = GBufferWidth;
	m_raytracingHeight = GBufferHeight;
	m_quarterResWidth = RTAOWidth;
	m_quarterResHeight = RTAOHeight;

	CreateResolutionDependentResources();
}


void Pathtracer::CreateTextureResources()
{
	auto device = m_Renderer->GetContext()->GetDevice();
	auto backbufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	DXGI_FORMAT hitPositionFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	DXGI_FORMAT debugFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

	// Full-res GBuffer resources.
	{
		// Preallocate subsequent descriptor indices for both SRV and UAV groups.
// 		m_GBufferResources[0].uavDescriptorHeapIndex = m_cbvSrvUavHeap->AllocateDescriptorIndices(GBufferResource::Count);
// 		m_GBufferResources[0].srvDescriptorHeapIndex = m_cbvSrvUavHeap->AllocateDescriptorIndices(GBufferResource::Count);
// 		for (UINT i = 0; i < GBufferResource::Count; i++)
// 		{
// 			m_GBufferResources[i].uavDescriptorHeapIndex = m_GBufferResources[0].uavDescriptorHeapIndex + i;
// 			m_GBufferResources[i].srvDescriptorHeapIndex = m_GBufferResources[0].srvDescriptorHeapIndex + i;
// 		}
		float clearColor[] = { 0.1f, 0.2f, 0.4f, 1.0f };
		m_GBufferResources[GBufferResource::HitPosition] = static_pointer_cast<DX12Texture>(m_Renderer->CreateEmptyTexture("GBuffer HitPosition", Texture::Type::Texture2D, m_raytracingWidth, m_raytracingHeight, 1, (Texture::Format)hitPositionFormat, Texture::Usage::RTV | Texture::Usage::UAV, clearColor));
		m_GBufferResources[GBufferResource::SurfaceNormalDepth] = static_pointer_cast<DX12Texture>(m_Renderer->CreateEmptyTexture("GBuffer Normal Depth", Texture::Type::Texture2D, m_raytracingWidth, m_raytracingHeight, 1, (Texture::Format)COMPACT_NORMAL_DEPTH_DXGI_FORMAT, Texture::Usage::RTV | Texture::Usage::UAV, clearColor));
		m_GBufferResources[GBufferResource::Depth] = static_pointer_cast<DX12Texture>(m_Renderer->CreateEmptyTexture("GBuffer Distance", Texture::Type::Texture2D, m_raytracingWidth, m_raytracingHeight, 1, (Texture::Format)DXGI_FORMAT_R16_FLOAT, Texture::Usage::RTV | Texture::Usage::UAV, clearColor));
		m_GBufferResources[GBufferResource::PartialDepthDerivatives] = static_pointer_cast<DX12Texture>(m_Renderer->CreateEmptyTexture("GBuffer Partial Depth Derivatives", Texture::Type::Texture2D, m_raytracingWidth, m_raytracingHeight, 1, Texture::Format::R16G16_FLOAT, Texture::Usage::RTV | Texture::Usage::UAV, clearColor));
		m_GBufferResources[GBufferResource::MotionVector] = static_pointer_cast<DX12Texture>(m_Renderer->CreateEmptyTexture("GBuffer Texture Space Motion Vector", Texture::Type::Texture2D, m_raytracingWidth, m_raytracingHeight, 1, Texture::Format::R16G16_FLOAT, Texture::Usage::RTV | Texture::Usage::UAV, clearColor));
		m_GBufferResources[GBufferResource::ReprojectedNormalDepth] = static_pointer_cast<DX12Texture>(m_Renderer->CreateEmptyTexture("GBuffer Reprojected Hit Position", Texture::Type::Texture2D, m_raytracingWidth, m_raytracingHeight, 1, (Texture::Format)COMPACT_NORMAL_DEPTH_DXGI_FORMAT, Texture::Usage::RTV | Texture::Usage::UAV, clearColor));
		m_GBufferResources[GBufferResource::Color] = static_pointer_cast<DX12Texture>(m_Renderer->CreateEmptyTexture("GBuffer Color", Texture::Type::Texture2D, m_raytracingWidth, m_raytracingHeight, 1, (Texture::Format)backbufferFormat, Texture::Usage::RTV | Texture::Usage::UAV, clearColor));
		m_GBufferResources[GBufferResource::AOSurfaceAlbedo] = static_pointer_cast<DX12Texture>(m_Renderer->CreateEmptyTexture("GBuffer AO Surface Albedo", Texture::Type::Texture2D, m_raytracingWidth, m_raytracingHeight, 1, (Texture::Format)DXGI_FORMAT_R11G11B10_FLOAT, Texture::Usage::RTV | Texture::Usage::UAV, clearColor));
	}

	// Low-res GBuffer resources.
	{
		// Preallocate subsequent descriptor indices for both SRV and UAV groups.
// 		m_GBufferQuarterResResources[0].uavDescriptorHeapIndex = m_cbvSrvUavHeap->AllocateDescriptorIndices(GBufferResource::Count);
// 		m_GBufferQuarterResResources[0].srvDescriptorHeapIndex = m_cbvSrvUavHeap->AllocateDescriptorIndices(GBufferResource::Count);
// 		for (UINT i = 0; i < GBufferResource::Count; i++)
// 		{
// 			m_GBufferQuarterResResources[i].uavDescriptorHeapIndex = m_GBufferQuarterResResources[0].uavDescriptorHeapIndex + i;
// 			m_GBufferQuarterResResources[i].srvDescriptorHeapIndex = m_GBufferQuarterResResources[0].srvDescriptorHeapIndex + i;
// 		}
		float clearColor[] = { 0.1f, 0.2f, 0.4f, 1.0f };
		m_GBufferQuarterResResources[GBufferResource::HitPosition] = static_pointer_cast<DX12Texture>(m_Renderer->CreateEmptyTexture("GBuffer LowRes HitPosition", Texture::Type::Texture2D, m_quarterResWidth, m_quarterResHeight, 1, (Texture::Format)hitPositionFormat, Texture::Usage::RTV | Texture::Usage::UAV, clearColor));
		m_GBufferQuarterResResources[GBufferResource::SurfaceNormalDepth] = static_pointer_cast<DX12Texture>(m_Renderer->CreateEmptyTexture("GBuffer LowRes Normal", Texture::Type::Texture2D, m_quarterResWidth, m_quarterResHeight, 1, (Texture::Format)COMPACT_NORMAL_DEPTH_DXGI_FORMAT, Texture::Usage::RTV | Texture::Usage::UAV, clearColor));
		m_GBufferQuarterResResources[GBufferResource::Depth] = static_pointer_cast<DX12Texture>(m_Renderer->CreateEmptyTexture("GBuffer LowRes Distance", Texture::Type::Texture2D, m_quarterResWidth, m_quarterResHeight, 1, (Texture::Format)DXGI_FORMAT_R16_FLOAT, Texture::Usage::RTV | Texture::Usage::UAV, clearColor));
		m_GBufferQuarterResResources[GBufferResource::PartialDepthDerivatives] = static_pointer_cast<DX12Texture>(m_Renderer->CreateEmptyTexture("GBuffer LowRes Partial Depth Derivatives", Texture::Type::Texture2D, m_quarterResWidth, m_quarterResHeight, 1, Texture::Format::R16G16_FLOAT, Texture::Usage::RTV | Texture::Usage::UAV, clearColor));
		m_GBufferQuarterResResources[GBufferResource::MotionVector] = static_pointer_cast<DX12Texture>(m_Renderer->CreateEmptyTexture("GBuffer LowRes Texture Space Motion Vector", Texture::Type::Texture2D, m_quarterResWidth, m_quarterResHeight, 1, Texture::Format::R16G16_FLOAT, Texture::Usage::RTV | Texture::Usage::UAV, clearColor));

		m_GBufferQuarterResResources[GBufferResource::ReprojectedNormalDepth] = static_pointer_cast<DX12Texture>(m_Renderer->CreateEmptyTexture("GBuffer LowRes Reprojected Normal Depth", Texture::Type::Texture2D, m_quarterResWidth, m_quarterResHeight, 1, (Texture::Format)COMPACT_NORMAL_DEPTH_DXGI_FORMAT, Texture::Usage::RTV | Texture::Usage::UAV, clearColor));
		m_GBufferQuarterResResources[GBufferResource::AOSurfaceAlbedo] = static_pointer_cast<DX12Texture>(m_Renderer->CreateEmptyTexture("GBuffer LowRes AO Surface Albedo", Texture::Type::Texture2D, m_quarterResWidth, m_quarterResHeight, 1, (Texture::Format)DXGI_FORMAT_R11G11B10_FLOAT, Texture::Usage::RTV | Texture::Usage::UAV, clearColor));
	}

}

void Pathtracer::DownsampleGBuffer()
{
	// 	auto commandList = m_deviceResources->GetCommandList();
	// 	auto resourceStateTracker = m_deviceResources->GetGpuResourceStateTracker();
	// 
	// 	// Transition all output resources to UAV state.
	// 	{
	// 		resourceStateTracker->TransitionResource(&m_GBufferQuarterResResources[GBufferResource::HitPosition], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	// 		resourceStateTracker->TransitionResource(&m_GBufferQuarterResResources[GBufferResource::PartialDepthDerivatives], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	// 		resourceStateTracker->TransitionResource(&m_GBufferQuarterResResources[GBufferResource::MotionVector], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	// 		resourceStateTracker->TransitionResource(&m_GBufferQuarterResResources[GBufferResource::ReprojectedNormalDepth], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	// 		resourceStateTracker->TransitionResource(&m_GBufferQuarterResResources[GBufferResource::Depth], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	// 		resourceStateTracker->TransitionResource(&m_GBufferQuarterResResources[GBufferResource::SurfaceNormalDepth], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	// 		resourceStateTracker->TransitionResource(&m_GBufferQuarterResResources[GBufferResource::AOSurfaceAlbedo], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	// 	}
	// 
	// 	resourceStateTracker->FlushResourceBarriers();
	// 	m_downsampleGBufferBilateralFilterKernel.Run(
	// 		commandList,
	// 		m_raytracingWidth,
	// 		m_raytracingHeight,
	// 		m_cbvSrvUavHeap->GetHeap(),
	// 		m_GBufferResources[GBufferResource::SurfaceNormalDepth].gpuDescriptorReadAccess,
	// 		m_GBufferResources[GBufferResource::HitPosition].gpuDescriptorReadAccess,
	// 		m_GBufferResources[GBufferResource::PartialDepthDerivatives].gpuDescriptorReadAccess,
	// 		m_GBufferResources[GBufferResource::MotionVector].gpuDescriptorReadAccess,
	// 		m_GBufferResources[GBufferResource::ReprojectedNormalDepth].gpuDescriptorReadAccess,
	// 		m_GBufferResources[GBufferResource::Depth].gpuDescriptorReadAccess,
	// 		m_GBufferResources[GBufferResource::AOSurfaceAlbedo].gpuDescriptorReadAccess,
	// 		m_GBufferQuarterResResources[GBufferResource::SurfaceNormalDepth].gpuDescriptorWriteAccess,
	// 		m_GBufferQuarterResResources[GBufferResource::HitPosition].gpuDescriptorWriteAccess,
	// 		m_GBufferQuarterResResources[GBufferResource::PartialDepthDerivatives].gpuDescriptorWriteAccess,
	// 		m_GBufferQuarterResResources[GBufferResource::MotionVector].gpuDescriptorWriteAccess,
	// 		m_GBufferQuarterResResources[GBufferResource::ReprojectedNormalDepth].gpuDescriptorWriteAccess,
	// 		m_GBufferQuarterResResources[GBufferResource::Depth].gpuDescriptorWriteAccess,
	// 		m_GBufferQuarterResResources[GBufferResource::AOSurfaceAlbedo].gpuDescriptorWriteAccess);
	// 
	// 	// Transition GBuffer resources to shader resource state.
	// 	{
	// 		resourceStateTracker->TransitionResource(&m_GBufferQuarterResResources[GBufferResource::HitPosition], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	// 		resourceStateTracker->TransitionResource(&m_GBufferQuarterResResources[GBufferResource::SurfaceNormalDepth], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	// 		resourceStateTracker->TransitionResource(&m_GBufferQuarterResResources[GBufferResource::PartialDepthDerivatives], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	// 		resourceStateTracker->TransitionResource(&m_GBufferQuarterResResources[GBufferResource::MotionVector], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	// 		resourceStateTracker->TransitionResource(&m_GBufferQuarterResResources[GBufferResource::ReprojectedNormalDepth], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	// 		resourceStateTracker->TransitionResource(&m_GBufferQuarterResResources[GBufferResource::Depth], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	// 		resourceStateTracker->TransitionResource(&m_GBufferQuarterResResources[GBufferResource::AOSurfaceAlbedo], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	// 	}
}



void Pathtracer::Run()
{
	auto device = m_Renderer->GetContext()->GetDevice();
	ID3D12GraphicsCommandList4* commandList;
	m_Renderer->GetContext()->GetCommandList()->QueryInterface(IID_PPV_ARGS(&commandList));
	//auto resourceStateTracker = m_deviceResources->GetGpuResourceStateTracker();
	auto frameIndex = m_Renderer->GetContext()->GetCurrentFrameIndex();

	// TODO: this should be called before any rendering in a frame
	if (m_isRecreateRaytracingResourcesRequested)
	{
		m_isRecreateRaytracingResourcesRequested = false;
		//m_Renderer->

		CreateResolutionDependentResources();
		CreateAuxilaryDeviceResources();
	}

	UpdateConstantBuffer();

	//auto& MaterialBuffer = scene.MaterialBuffer();
	auto& EnvironmentMap = m_EnvironmentMap;
	auto& PrevFrameBottomLevelASInstanceTransforms = m_Renderer->GetPrevFrameBottomLevelASInstanceTransforms();

	auto heap = m_Renderer->GetShaderVisibleHeap();
	commandList->SetDescriptorHeaps(1, &heap);
	commandList->SetComputeRootSignature(m_raytracingGlobalRootSignature.get());

	// Copy dynamic buffers to GPU.
	{
		m_CB.CopyStagingToGpu(frameIndex);
	}

	// Transition all output resources to UAV state.
	{
		m_GBufferResources[GBufferResource::HitPosition]->ResourceBarrier(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		m_GBufferResources[GBufferResource::SurfaceNormalDepth]->ResourceBarrier(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		m_GBufferResources[GBufferResource::Depth]->ResourceBarrier(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		m_GBufferResources[GBufferResource::PartialDepthDerivatives]->ResourceBarrier(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		m_GBufferResources[GBufferResource::MotionVector]->ResourceBarrier(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		m_GBufferResources[GBufferResource::ReprojectedNormalDepth]->ResourceBarrier(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		m_GBufferResources[GBufferResource::Color]->ResourceBarrier(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		m_GBufferResources[GBufferResource::AOSurfaceAlbedo]->ResourceBarrier(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}


	// Bind inputs.
	commandList->SetComputeRootShaderResourceView(GlobalRootSignature::Slot::AccelerationStructure, m_Renderer->GetAccelerationStructure()->GetTopLevelASResource()->GetGPUVirtualAddress());
	commandList->SetComputeRootConstantBufferView(GlobalRootSignature::Slot::ConstantBuffer, m_CB.GpuVirtualAddress(frameIndex));
	//commandList->SetComputeRootShaderResourceView(GlobalRootSignature::Slot::MaterialBuffer, scene.MaterialBuffer().GpuVirtualAddress());
	commandList->SetComputeRootDescriptorTable(GlobalRootSignature::Slot::EnvironmentMap, EnvironmentMap->GetGpuDescriptorHandle());
	commandList->SetComputeRootShaderResourceView(GlobalRootSignature::Slot::PrevFrameBottomLevelASIstanceTransforms, PrevFrameBottomLevelASInstanceTransforms.GpuVirtualAddress(frameIndex));


	// Bind output RTs.
	commandList->SetComputeRootDescriptorTable(GlobalRootSignature::Slot::GBufferResources, m_GBufferResources[0]->GetGpuDescriptorHandleUAV());
	commandList->SetComputeRootDescriptorTable(GlobalRootSignature::Slot::MotionVector, m_GBufferResources[GBufferResource::MotionVector]->GetGpuDescriptorHandleUAV());
	commandList->SetComputeRootDescriptorTable(GlobalRootSignature::Slot::ReprojectedNormalDepth, m_GBufferResources[GBufferResource::ReprojectedNormalDepth]->GetGpuDescriptorHandleUAV());
	commandList->SetComputeRootDescriptorTable(GlobalRootSignature::Slot::Color, m_GBufferResources[GBufferResource::Color]->GetGpuDescriptorHandleUAV());
	commandList->SetComputeRootDescriptorTable(GlobalRootSignature::Slot::AOSurfaceAlbedo, m_GBufferResources[GBufferResource::AOSurfaceAlbedo]->GetGpuDescriptorHandleUAV());

	auto debugResources = m_Renderer->GetDebugOutput();
	commandList->SetComputeRootDescriptorTable(GlobalRootSignature::Slot::Debug1, debugResources[0]->GetGpuDescriptorHandleUAV());
	commandList->SetComputeRootDescriptorTable(GlobalRootSignature::Slot::Debug2, debugResources[1]->GetGpuDescriptorHandleUAV());

	// Dispatch Rays.
	DispatchRays(m_rayGenShaderTables[RayGenShaderType::Pathtracer].get());

	// Transition GBuffer resources to shader resource state.
	{
		m_GBufferResources[GBufferResource::HitPosition]->ResourceBarrier(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		m_GBufferResources[GBufferResource::SurfaceNormalDepth]->ResourceBarrier(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		m_GBufferResources[GBufferResource::Depth]->ResourceBarrier(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		m_GBufferResources[GBufferResource::MotionVector]->ResourceBarrier(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		m_GBufferResources[GBufferResource::ReprojectedNormalDepth]->ResourceBarrier(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		m_GBufferResources[GBufferResource::Color]->ResourceBarrier(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		m_GBufferResources[GBufferResource::AOSurfaceAlbedo]->ResourceBarrier(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}

	// Calculate partial derivatives.
	{
		//resourceStateTracker->FlushResourceBarriers();
		m_calculatePartialDerivativesKernel.Run(
			commandList,
			m_Renderer->GetShaderVisibleHeap(),
			m_raytracingWidth,
			m_raytracingHeight,
			m_GBufferResources[GBufferResource::Depth]->GetGpuDescriptorHandle(),
			m_GBufferResources[GBufferResource::PartialDepthDerivatives]->GetGpuDescriptorHandleUAV());

		m_GBufferResources[GBufferResource::PartialDepthDerivatives]->ResourceBarrier(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}
	// 	if (RTAO_Args::QuarterResAO)
	// 	{
	// 		DownsampleGBuffer();
	// 	}
}

void Pathtracer::BuildShaderTables()
{
	auto device = m_Renderer->GetContext()->GetDevice();

	void* rayGenShaderIDs[RayGenShaderType::Count];
	void* missShaderIDs[PathtracerRayType::Count];
	void* hitGroupShaderIDs_TriangleGeometry[PathtracerRayType::Count];

	// A shader name look-up table for shader table debug print out.
	robin_hood::unordered_map<void*, std::wstring> shaderIdToStringMap;

	auto GetShaderIDs = [&](auto* stateObjectProperties)
		{
			for (UINT i = 0; i < RayGenShaderType::Count; i++)
			{
				rayGenShaderIDs[i] = stateObjectProperties->GetShaderIdentifier(c_rayGenShaderNames[i]);
				shaderIdToStringMap[rayGenShaderIDs[i]] = c_rayGenShaderNames[i];
			}

			for (UINT i = 0; i < PathtracerRayType::Count; i++)
			{
				missShaderIDs[i] = stateObjectProperties->GetShaderIdentifier(c_missShaderNames[i]);
				shaderIdToStringMap[missShaderIDs[i]] = c_missShaderNames[i];
			}

			for (UINT i = 0; i < PathtracerRayType::Count; i++)
			{
				hitGroupShaderIDs_TriangleGeometry[i] = stateObjectProperties->GetShaderIdentifier(c_hitGroupNames[i]);
				shaderIdToStringMap[hitGroupShaderIDs_TriangleGeometry[i]] = c_hitGroupNames[i];
			}
		};

	// Get shader identifiers.
	UINT shaderIDSize;
	winrt::com_ptr<ID3D12StateObjectProperties> stateObjectProperties;
	//ThrowIfFailed(m_dxrStateObject.as(&stateObjectProperties));
	stateObjectProperties = m_dxrStateObject.as<ID3D12StateObjectProperties>();
	GetShaderIDs(stateObjectProperties.get());
	shaderIDSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	// RayGen shader tables.
	{
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIDSize;

		for (UINT i = 0; i < RayGenShaderType::Count; i++)
		{
			ShaderTable rayGenShaderTable(device, numShaderRecords, shaderRecordSize, L"RayGenShaderTable");
			rayGenShaderTable.push_back(ShaderRecord(rayGenShaderIDs[i], shaderIDSize, nullptr, 0));

#ifdef DEBUG_PRINT
			rayGenShaderTable.DebugPrint(shaderIdToStringMap);
#endif

			m_rayGenShaderTables[i] = rayGenShaderTable.GetResource();
		}
	}

	// Miss shader table.
	{
		UINT numShaderRecords = PathtracerRayType::Count;
		UINT shaderRecordSize = shaderIDSize; // No root arguments

		ShaderTable missShaderTable(device, numShaderRecords, shaderRecordSize, L"MissShaderTable");
		for (UINT i = 0; i < PathtracerRayType::Count; i++)
		{
			missShaderTable.push_back(ShaderRecord(missShaderIDs[i], shaderIDSize, nullptr, 0));
		}

#ifdef DEBUG_PRINT
		missShaderTable.DebugPrint(shaderIdToStringMap);
#endif

		m_missShaderTableStrideInBytes = missShaderTable.GetShaderRecordSize();
		m_missShaderTable = missShaderTable.GetResource();
	}

	// Hit group shader table.
	// 창 : 일단 grass patch에 대해서는 이걸 하지 않는다.
	{
		//auto& bottomLevelASGeometries = m_Renderer->GetAccelerationStructure()->GetB
		//auto& grassPatchVB = scene.GrassPatchVB();
		auto& accelerationStructure = *m_Renderer->GetAccelerationStructure();

		UINT numShaderRecords = 0;
		for (auto& meshMaterialHitDistribution : m_MeshMaterialHitDistributions)
		{
			auto mesh = std::get<0>(meshMaterialHitDistribution);
			numShaderRecords += mesh->subMeshDescriptors.size() * PathtracerRayType::Count;
		}

		if (numShaderRecords == 0)
		{
			m_hitGroupShaderTableStrideInBytes = 0;
			m_hitGroupShaderTable = nullptr;
			return;
		}

		UINT shaderRecordSize = shaderIDSize + sizeof(LocalRootSignature::RootArguments);
		ShaderTable hitGroupShaderTable(device, numShaderRecords, shaderRecordSize, L"HitGroupShaderTable");

		// Triangle geometry hit groups.
		for (auto& meshMaterialHitDistribution : m_MeshMaterialHitDistributions)
		{
			auto& mesh = std::get<0>(meshMaterialHitDistribution);
			auto& name = mesh->name;

			UINT shaderRecordOffset = hitGroupShaderTable.GeNumShaderRecords();
			std::wstring wName = std::wstring(name.begin(), name.end());
			accelerationStructure.GetBottomLevelAS(wName).SetInstanceContributionToHitGroupIndex(shaderRecordOffset);
			*std::get<2>(meshMaterialHitDistribution) = shaderRecordOffset;

			{
				for (int i = 0; i < mesh->subMeshDescriptors.size(); i++)
				{
					if (std::get<1>(meshMaterialHitDistribution).size() <= i || std::get<1>(meshMaterialHitDistribution)[i] == nullptr)
					{
						continue;
					}
					LocalRootSignature::RootArguments rootArgs;
					//rootArgs.cb.materialID = mesh->subMeshDescriptors[i].materialID;
					//rootArgs.cb.isVertexAnimated = mesh->subMeshDescriptors[i].isVertexAnimated;
					rootArgs.cb.isVertexAnimated = 0;
					rootArgs.cb.hasDiffuseTexture = 1;
					rootArgs.cb.hasNormalTexture = 1;
					rootArgs.cb.type = MaterialType::Default;
					auto vertexBuffer = mesh->vertexBuffer;
					auto indexBuffer = mesh->indexBuffer;
					DX12Buffer* vertexBufferDX12 = static_cast<DX12Buffer*>(vertexBuffer.get());
					DX12Buffer* indexBufferDX12 = static_cast<DX12Buffer*>(indexBuffer.get());
					memcpy(&rootArgs.indexBufferGPUHandle, &indexBufferDX12->GetSRVGpuHandle(), sizeof(indexBufferDX12->GetSRVGpuHandle()));
					memcpy(&rootArgs.vertexBufferGPUHandle, &vertexBufferDX12->GetSRVGpuHandle(), sizeof(vertexBufferDX12->GetSRVGpuHandle()));
					memcpy(&rootArgs.previousFrameVertexBufferGPUHandle, &m_nullVertexBufferGPUhandle, sizeof(m_nullVertexBufferGPUhandle));

					auto& albedo = static_pointer_cast<DX12Texture>(
						m_Renderer->CreateTexture(std::get<1>(meshMaterialHitDistribution)[i]->GetTextures().find("gAlbedo")->second.Path.c_str()));
					rootArgs.diffuseTextureGPUHandle = albedo->GetGpuDescriptorHandle();

					auto& normal = static_pointer_cast<DX12Texture>(
						m_Renderer->CreateTexture(std::get<1>(meshMaterialHitDistribution)[i]->GetTextures().find("gNormal")->second.Path.c_str()));
					rootArgs.normalTextureGPUHandle = normal->GetGpuDescriptorHandle();

					auto& metallic = static_pointer_cast<DX12Texture>(
						m_Renderer->CreateTexture(std::get<1>(meshMaterialHitDistribution)[i]->GetTextures().find("gMetallic")->second.Path.c_str()));
					rootArgs.metallicTextureGPUHandle = metallic->GetGpuDescriptorHandle();

					auto& roughness = static_pointer_cast<DX12Texture>(
						m_Renderer->CreateTexture(std::get<1>(meshMaterialHitDistribution)[i]->GetTextures().find("gRoughness")->second.Path.c_str()));
					rootArgs.roughnessTextureGPUHandle = roughness->GetGpuDescriptorHandle();

					auto& ao = static_pointer_cast<DX12Texture>(
						m_Renderer->CreateTexture(std::get<1>(meshMaterialHitDistribution)[i]->GetTextures().find("gAO")->second.Path.c_str()));
					rootArgs.opacityTextureGPUHandle = ao->GetGpuDescriptorHandle();

					rootArgs.offsets.vertexOffset = mesh->subMeshDescriptors[i].vertexOffset;
					rootArgs.offsets.indexOffset = mesh->subMeshDescriptors[i].indexOffset;

					for (auto& hitGroupShaderID : hitGroupShaderIDs_TriangleGeometry)
					{
						hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderID, shaderIDSize, &rootArgs, sizeof(rootArgs)));
					}
				}
			}
		}

#ifdef DEBUG_PRINT
		hitGroupShaderTable.DebugPrint(shaderIdToStringMap);
#endif

		m_hitGroupShaderTableStrideInBytes = hitGroupShaderTable.GetShaderRecordSize();
		m_hitGroupShaderTable = hitGroupShaderTable.GetResource();
	}
}

void Pathtracer::AddMeshToScene(std::shared_ptr<Mesh> mesh, const std::vector<std::shared_ptr<Material>>& material, uint32_t* pOutHitDistribution, bool isRebuild)
{
	if (mesh != nullptr && material.size() > 0)
	{
		m_MeshMaterialHitDistributions.emplace_back(mesh, std::vector<std::shared_ptr<Material>>{}, pOutHitDistribution);
		for (auto& mat : material)
		{
			if (mat != nullptr)
			{
				if (mat->m_ShaderString.find("PBR") != std::string::npos || mat->m_ShaderString.find("Geometry") != std::string::npos)
				{
					std::get<1>(m_MeshMaterialHitDistributions.back()).push_back(mat);
					continue;
				}
			}
			std::filesystem::path defaultMaterial = "Resources/Materials/rtpbr.material";
			// 절대경로를 가지고온다.
			std::filesystem::path materialPath = std::filesystem::absolute(defaultMaterial);
			std::get<1>(m_MeshMaterialHitDistributions.back()).push_back(m_Renderer->GetMaterial(materialPath.string()));
		}
	}

	if (isRebuild)
	{
		BuildShaderTables();
	}
}

