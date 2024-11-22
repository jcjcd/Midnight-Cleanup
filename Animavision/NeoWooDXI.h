#pragma once

#include "Renderer.h"
#include "NeoDX11Context.h"
#include "DX11Shader.h"
#include "MeshLibrary.h"
#include "Material.h"
#include "AnimationLibrary.h"
#include "DX11ResourceManager.h"

#include "DebugDraw.h"
#include <./directxtk/CommonStates.h>

#include "DX11State.h"
#include "VideoTexture.h"


struct Font;

class NeoWooDXI : public Renderer
{
public:
	NeoWooDXI(HWND hwnd, uint32_t width, uint32_t height);
	virtual ~NeoWooDXI();

	// Renderer을(를) 통해 상속됨
	// context
	virtual RendererContext* GetContext() override;
	virtual void Resize(uint32_t width, uint32_t height) override;
	virtual void SetFullScreen(bool fullscreen) override;
	virtual void SetVSync(bool vsync) override;
	virtual void Submit(Mesh& mesh, Material& material, PrimitiveTopology primitiveMode = PrimitiveTopology::TRIANGLELIST, uint32_t instances = 1) override;
	virtual void Submit(Mesh& mesh, Material& material, uint32_t subMeshIndex, PrimitiveTopology primitiveMode = PrimitiveTopology::TRIANGLELIST, uint32_t instances = 1) override;
	virtual void DispatchCompute(Material& material, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) override;
	virtual void CopyStructureCount(Buffer* dest, uint32_t distAlignedByteOffset, Texture* src) override;
	virtual void SubmitInstancedIndirect(Material& material, Texture* argsBuffer, uint32_t allignedByteOffsetForArgs, PrimitiveTopology primitiveMode = PrimitiveTopology::TRIANGLELIST) override;
	virtual void BeginRender() override;
	virtual void EndRender() override;
	virtual void Clear(const float* RGBA) override;
	virtual void ClearTexture(Texture* texture, const float* clearColor) override;
	virtual std::shared_ptr<Texture> CreateEmptyTexture(std::string name, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Format format, Texture::Usage usage = Texture::Usage::NONE, float* clearColor = nullptr, Texture::UAVType uavType = Texture::UAVType::NONE, uint32_t arraySize = 1) override;
	virtual std::shared_ptr<Texture> CreateEmptyTexture(const TextureDesc& desc);
	virtual void SetRenderTargets(uint32_t numRenderTargets, Texture* renderTargets[], Texture* depthStencil = nullptr, bool useDefaultDSV = true) override;
	virtual void ApplyRenderState(BlendState blendstate, RasterizerState rasterizerstate, DepthStencilState depthstencilstate) override;
	virtual void SetViewport(uint32_t width, uint32_t height) override;
	virtual uint32_t GetWidth() const override { return m_Context->GetWidth(); }
	virtual uint32_t GetHeight() const override { return m_Context->GetHeight(); }
	virtual bool IsFullScreen() const override { return m_Context->IsFullScreen(); }
	virtual bool IsVSync() const override { return m_Context->IsVSync(); }
	// For ImGui
	virtual void* GetShaderResourceView() override;
	virtual void* GetDevice() override;
	virtual void* GetDeviceContext() override;

	virtual std::shared_ptr<Texture> GetTexture(const char* path) override;
	virtual std::shared_ptr<Texture> CreateTexture(const char* path) override;
	virtual std::shared_ptr<Texture> CreateTexture2DArray(const char* path) override;
	virtual std::shared_ptr<Texture> CreateTextureCube(const char* path) override;
	virtual std::shared_ptr<Shader> LoadShader(const char* srcPath) override;
	virtual void ReleaseTexture(const std::string& name) override;

	virtual std::shared_ptr<Texture> CreateLightmapTexture(const char* path) override;
	virtual void SaveLightmapTextureArray(std::vector<std::shared_ptr<Texture>> lightmaps, std::string path) override;

	virtual void LoadMaterialsFromDrive(const std::string& path) override;
	virtual void SaveMaterial(const std::string& name) override;
	virtual void AddMaterial(const std::shared_ptr<Material>& material) override { m_MaterialLibrary->AddMaterial(material); }
	virtual std::shared_ptr<Material> GetMaterial(const std::string& name) override { return m_MaterialLibrary->GetMaterial(name); }
	virtual std::map<std::string, std::shared_ptr<Material>>* GetMaterials() override { return &m_MaterialLibrary->GetMaterials(); }

	virtual void LoadShadersFromDrive(const std::string& path) override;
	virtual std::map<std::string, std::shared_ptr<Shader>>* GetShaders() override { return &m_ShaderLibrary->GetShaders(); }

	virtual void LoadMeshesFromDrive(const std::string& path) override;
	virtual std::shared_ptr<Mesh> GetMesh(const std::string& name) override { return m_MeshLibrary->GetMesh(name); }
	virtual std::map<std::string, std::shared_ptr<Mesh>>* GetMeshes() override { return &m_MeshLibrary->GetMeshes(); }
	virtual void AddMesh(const std::shared_ptr<Mesh>& mesh) override { m_MeshLibrary->AddMesh(mesh); }

	virtual void LoadAnimationClipsFromDrive(const std::string& path);
	virtual std::shared_ptr<AnimationClip> GetAnimationClip(const std::string& name) { return m_AnimationLibrary->GetAnimationClip(name); }
	virtual std::map<std::string, std::shared_ptr<AnimationClip>>* GetAnimationClips() { return &m_AnimationLibrary->GetAnimationClips(); }

	// UI
	virtual void LoadUITexturesFromDrive(const std::string& path) override;
	virtual std::shared_ptr<Texture> GetUITexture(const std::string& name) override;
	virtual robin_hood::unordered_map<std::string, std::shared_ptr<Texture>>* GetUITextures() override { return &m_UITextures; }

	// DebugDraw
	virtual void SetDebugViewProj(const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj) override;
	virtual void BeginDebugRender() override;
	virtual void EndDebugRender() override;

	virtual void DrawBoundingBox(const DirectX::BoundingBox& box, const DirectX::SimpleMath::Vector4& color) override;
	virtual void DrawOrientedBoundingBox(const DirectX::BoundingOrientedBox& box, const DirectX::SimpleMath::Vector4& color) override;
	virtual void DrawBoundingSphere(const DirectX::BoundingSphere& sphere, const DirectX::SimpleMath::Vector4& color) override;
	// Particle
	virtual std::shared_ptr<Texture> CreateRandomTexture() override;
	virtual std::shared_ptr<Texture> CreateIndirectDrawTexture() override;
	virtual void LoadParticleTexturesFromDrive(const std::string& path) override;
	virtual std::shared_ptr<Texture> GetParticleTexture(std::string name) override;
	virtual std::map<std::string, std::shared_ptr<Texture>>* GetParticleTextures() override { return &m_ParticleTextures; }
	virtual ParticleBuffers CreateParticleBufferTextures(uint32_t maxCount, uint32_t entity) override;
	virtual float GetRandomSeed(float min, float max) override;

	// DWrite
	virtual void SubmitText(Texture* rt, Font* font, const std::string& text, Vector2 posH, Vector2 size, Vector2 scale, DirectX::SimpleMath::Color color, float fontSize, TextAlign textAlign = TextAlign::LeftTop, TextBoxAlign boxAlign = TextBoxAlign::CenterCenter, bool useUnderline = false, bool useStrikeThrough = false) override;
	virtual void SubmitComboBox(Texture* rt, Font* font, const std::vector<std::string>& text, uint32_t curIndex, bool isOpen,Vector2 posH, float leftPadding, Vector2 size, Vector2 scale, DirectX::SimpleMath::Color color, float fontSize, TextAlign textAlign = TextAlign::LeftTop, TextBoxAlign boxAlign = TextBoxAlign::CenterCenter, bool useUnderline = false) override;
	virtual void LoadFontsFromDrive(const std::string& path) override;
	virtual std::shared_ptr<Font> GetFont(const std::string& name) override;
	virtual robin_hood::unordered_map<std::string, std::shared_ptr<Font>>* GetFonts() override { return &m_Fonts; }

private:
	void bindShaderResources(Material& material);
	void unbindShaderResources(Material& material);

	// UI
	void loadUITexturesFromDirectory(const std::string& path);
	void loadUITexturesFromFile(const std::string& path);

	// Particle
	void loadParticleTexturesFromDirectory(const std::string& path);
	void loadParticleTexturesFromFile(const std::string& path);

	// Font
	void loadFontsFromDirectory(const std::string& path);
	void loadFontsFromFile(const std::string& path);

private:
	// DirectX11
	API m_API = API::DirectX11;
	std::unique_ptr<NeoDX11Context> m_Context = nullptr;
	std::unique_ptr<DX11ResourceManager> m_ResourceManager = nullptr;

	// Library
	std::unique_ptr<ShaderLibrary> m_ShaderLibrary = nullptr;
	std::unique_ptr<MeshLibrary> m_MeshLibrary = nullptr;
	std::unique_ptr<MaterialLibrary> m_MaterialLibrary = nullptr;
	std::unique_ptr<AnimationLibrary> m_AnimationLibrary = nullptr;

	// RenderState
	RasterizerState m_RasterizerState = RasterizerState::CULL_BACK;
	BlendState m_BlendState = BlendState::NO_BLEND;
	DepthStencilState m_DepthStencilState = DepthStencilState::DEPTH_ENABLED;

	// PipelineState
	robin_hood::unordered_map<uint32_t, DX11PipelineState> m_PipelineStates;

	// UI
	robin_hood::unordered_map<std::string, std::shared_ptr<Texture>> m_UITextures;

	// Particle
	std::map<std::string, std::shared_ptr<Texture>> m_ParticleTextures;

	// DWrite
	robin_hood::unordered_map<std::string, std::shared_ptr<Font>> m_Fonts;

	// Win
	HWND m_hWnd = nullptr;

	// DebugDraw
	using VertexType = DirectX::DX11::VertexPositionColor;
	std::unique_ptr<DirectX::DX11::CommonStates> m_states;
	std::unique_ptr<DirectX::BasicEffect> m_effect;
	std::unique_ptr<DirectX::DX11::PrimitiveBatch<VertexType>> m_batch;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
};


