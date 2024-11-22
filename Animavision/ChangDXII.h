#pragma once
#include "Renderer.h"
#include "DX12Context.h"
#include "DX12ResourceManager.h"
#include "Shader.h"
#include "Descriptor.h"
#include "Material.h"
#include "MeshLibrary.h"
#include "AnimationLibrary.h"
#include "SkinnedVertexConverter.h"

// test
#include "GraphicsMemory.h"
#include "PrimitiveBatch.h"
#include "VertexTypes.h"
#include "Effects.h"
#include "CommonStates.h"

class Pathtracer;
class SimpleLighting;

class ChangDXII : public Renderer
{
public:
	ChangDXII(HWND hwnd, uint32_t width, uint32_t height, bool isRaytracing);

	void Resize(uint32_t width, uint32_t height) override;
	void Clear(const float* RGBA) override;
	void SetRenderTargets(uint32_t numRenderTargets, Texture* renderTargets[], Texture* depthStencil = nullptr, bool useDefaultDSV = true) override;
	void ApplyRenderState(BlendState blendstate, RasterizerState rasterizerstate, DepthStencilState depthstencilstate) override;
	void SetViewport(uint32_t width, uint32_t height) override;

	void Submit(Mesh& mesh, Material& material, PrimitiveTopology primitiveMode, uint32_t instances) override;
	void Submit(Mesh& mesh, Material& material, uint32_t subMeshIndex, PrimitiveTopology primitiveMode = PrimitiveTopology::TRIANGLELIST, uint32_t instances = 1) override;
	void DispatchCompute(Material& material, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);
	void DispatchRays(Material& material, uint32_t width, uint32_t height, uint32_t depth) override;

	bool IsRayTracing() const override { return m_IsRaytracing; }

	// dxr
	void AddMeshToScene(std::shared_ptr<Mesh> mesh, const std::vector<std::shared_ptr<Material>>& materials, uint32_t* pOutHitDistribution, bool isRebuild = true);
	void AddBottomLevelAS(Mesh& mesh, bool allowUpdate = false) override;
	void AddSkinnedBottomLevelAS(uint32_t id, Mesh& mesh, bool allowUpdate = false) override;
	UINT AddBottomLevelASInstance(const std::string& meshName, uint32_t hitDistribution, float* transfrom) override;
	UINT AddSkinnedBLASInstance(uint32_t id, uint32_t hitDistribution, float* transform) override;
	void UpdateAccelerationStructures() override;
	void InitializeTopLevelAS() override;
	void ComputeSkinnedVertices(Mesh& mesh, const std::vector<Matrix>& boneMatrices, const uint32_t& vertexCount) override;
	void ModifyBLASGeometry(uint32_t id, Mesh& mesh) override;
	void ResetSkinnedBLAS() override;


	void BeginRender() override;
	void EndRender() override;

	void* GetDevice() override { return static_cast<void*>(m_Context->m_Device.get()); }
	DX12Context* GetContext() override { return m_Context.get(); }
	void* GetRenderCommandList() override { return static_cast<void*>(m_Context->GetCommandList()); }

	uint32_t GetWidth() const override { return m_Context->m_ScreenWidth; }
	uint32_t GetHeight() const override { return m_Context->m_ScreenHeight; }

	std::shared_ptr<Texture> GetTexture(const char* path) override;
	std::shared_ptr<Texture> CreateTexture(const char* path) override;
	std::shared_ptr<Texture> CreateTextureCube(const char* path);
	std::shared_ptr<Texture> CreateEmptyTexture(std::string name, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Format format, Texture::Usage usage = Texture::Usage::NONE, float* clearColor = nullptr, Texture::UAVType uavType = Texture::UAVType::NONE, uint32_t arraySize = 1) override;
	std::shared_ptr<Texture> CreateEmptyTexture(const TextureDesc& desc) override;

	void ReleaseTexture(const std::string& name) override;


	std::shared_ptr<Shader> LoadShader(const char* srcPath) override;
	void LoadShadersFromDrive(const std::string& path) override;
	std::map<std::string, std::shared_ptr<Shader>>* GetShaders() override { return &m_ShaderLibrary->GetShaders(); }

	std::shared_ptr<DX12Buffer> CreateVertexBuffer(const void* data, uint32_t count, uint32_t stride);
	std::shared_ptr<DX12Buffer> CreateIndexBuffer(const void* data, uint32_t count, uint32_t stride);

	void AddMesh(const std::shared_ptr<Mesh>& mesh) override { m_MeshLibrary->AddMesh(mesh); }
	void LoadMeshesFromDrive(const std::string& path) override;
	std::shared_ptr<Mesh> GetMesh(const std::string& name) override { return m_MeshLibrary->GetMesh(name); }
	std::map<std::string, std::shared_ptr<Mesh>>* GetMeshes() override { return &m_MeshLibrary->GetMeshes(); }

	void AddMaterial(const std::shared_ptr<Material>& material) override { m_MaterialLibrary->AddMaterial(material); }
	void LoadMaterialsFromDrive(const std::string& path) override;
	std::shared_ptr<Material> GetMaterial(const std::string& name) override { return m_MaterialLibrary->GetMaterial(name); }
	void SaveMaterial(const std::string& name) override;
	std::map<std::string, std::shared_ptr<Material>>* GetMaterials() override { return &m_MaterialLibrary->GetMaterials(); }

	void LoadAnimationClipsFromDrive(const std::string& path) override;
	std::shared_ptr<AnimationClip> GetAnimationClip(const std::string& name) override { return m_AnimationLibrary->GetAnimationClip(name); }
	std::map<std::string, std::shared_ptr<AnimationClip>>* GetAnimationClips() override { return &m_AnimationLibrary->GetAnimationClips(); }

	void* GetShaderResourceView() override;
	void* AllocateShaderVisibleDescriptor(void* OutCpuHandle, void* OutGpuHandle) override;


	RaytracingAccelerationStructureManager* GetAccelerationStructure() { return m_AccelerationStructure.get(); }

	ID3D12Resource* GetTopLevelAS() { return m_AccelerationStructure->GetTopLevelASResource(); }

	ID3D12DescriptorHeap* GetShaderVisibleHeap() { return m_ResourceManager->m_DescriptorAllocator->GetDescriptorHeap().get(); }

	void DoRayTracing() override;
	void FlushRaytracingData() override;
	void ResetAS() override;

	StructuredBuffer<DirectX::XMFLOAT3X4>& GetPrevFrameBottomLevelASInstanceTransforms() { return m_prevFrameBottomLevelASInstanceTransforms; }

	std::shared_ptr<DX12Texture>* GetDebugOutput() { return m_DebugOutput; }

	std::shared_ptr<Texture> GetColorRt() override;

	void SetCamera(const Matrix& view, const Matrix& proj, const Vector3& cameraPosition, const Vector3& forward, const Vector3& up, float ZMin, float ZMax) override;


	// Debug Render
	void DrawLine(const Vector3& from, const Vector3& to, const Vector4& color) override;

private:
	// DX12
	std::unique_ptr<DX12Context> m_Context;
	std::shared_ptr<CommandObjectPool> m_CommandObjectPool;

	// RenderState
	RasterizerState m_RasterizerState = RasterizerState::CULL_BACK;
	BlendState m_BlendState = BlendState::NO_BLEND;
	DepthStencilState m_DepthStencilState = DepthStencilState::DEPTH_ENABLED_LESS_EQUAL;

	// Descriptor Heap
	std::unique_ptr<DX12ResourceManager> m_ResourceManager;

	// Library
	std::unique_ptr<ShaderLibrary> m_ShaderLibrary;
	std::unique_ptr<MeshLibrary> m_MeshLibrary;
	std::unique_ptr<MaterialLibrary> m_MaterialLibrary;
	std::unique_ptr<AnimationLibrary> m_AnimationLibrary = nullptr;

	robin_hood::unordered_map<uint32_t, PipelineStateArray> m_PipelineStates;
	robin_hood::unordered_map<uint32_t, ComputePipelineState> m_ComputePipelineStates;
	robin_hood::unordered_map<uint32_t, RayTracingState> m_RayTracingPipelineStates;
	PipelineState* m_PipelineState = nullptr;

	// DXR
	bool m_IsRaytracing = false;

	constexpr static UINT kMaxNumBottomLevelInstances = 1000;
	// Bottom-Level AS Instance transforms used for previous frame. Used for Temporal Reprojection.
	StructuredBuffer<DirectX::XMFLOAT3X4> m_prevFrameBottomLevelASInstanceTransforms;
	std::unique_ptr<RaytracingAccelerationStructureManager> m_AccelerationStructure;
	uint32_t m_HitIndex;

	std::unique_ptr<SkinnedVertexConverter> m_SkinnedVertexConverter;


	// test 
	//std::shared_ptr<SimpleLighting> m_SimpleLighting;
	std::shared_ptr<Pathtracer> m_Pathtracer;
	std::shared_ptr<DX12Texture> m_DebugOutput[2];


	// test
// 	std::unique_ptr<DirectX::GraphicsMemory> m_graphicsMemory;
// 	std::unique_ptr<DirectX::BasicEffect> m_effect;
// 	std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_batch;
// 
// 	Matrix m_View;
// 	Matrix m_Proj;



};