#pragma once

#include "RendererDLL.h"
#include <memory>
#include <string>
#include <robin_hood.h>
#include <map>
#include <tuple>
#include "ShaderResource.h"

#include <directxtk/SimpleMath.h>

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector2;

class RendererContext;
class Mesh;
class Shader;
class Material;
class Texture;
struct AnimationClip;
class AnimatorController;
class Buffer;
struct Font;

enum class PrimitiveTopology
{
	UNDEFINED = 0,
	POINTLIST = 1,
	LINELIST = 2,
	LINESTRIP = 3,
	TRIANGLELIST = 4,
	TRIANGLESTRIP = 5,
	COUNT,
};

enum class API
{
	NONE,
	DirectX11,
	DirectX12
};

enum class RasterizerState
{
	CULL_NONE,
	CULL_BACK,
	CULL_FRONT,
	WIREFRAME,
	SHADOW,
	SHADOW_CULL_NONE,
	PSHADOW,
	PSHADOW_CULL_NONE,
	COUNT,
};

enum class BlendState
{
	NO_BLEND,
	ALPHA_BLEND,
	ADDITIVE_BLEND,
	SUBTRACTIVE_BLEND,
	MODULATE_BLEND,
	TRANSPARENT_BLEND,
	COUNT,
};

// 창 : 이거 이넘클래스 이름 좀 더 수정하기
enum class DepthStencilState
{
	DEPTH_ENABLED,
	DEPTH_ENABLED_LESS_EQUAL,
	DEPTH_DISABLED,
	NO_DEPTH_WRITE,
	COUNT,
};

struct TextureDesc
{
	std::string name;
	Texture::Type type;
	uint32_t width;
	uint32_t height;
	uint32_t mipLevels;
	Texture::Format format;
	Texture::Format srvFormat = Texture::Format::UNKNOWN;
	Texture::Format uavFormat = Texture::Format::UNKNOWN;
	Texture::Format rtvFormat = Texture::Format::UNKNOWN;
	Texture::Format dsvFormat = Texture::Format::UNKNOWN;
	Texture::Usage usage = Texture::Usage::NONE;
	float* clearColor = nullptr;
	Texture::UAVType uavType = Texture::UAVType::NONE;
	uint32_t arraySize = 1;

	TextureDesc() = default;

	TextureDesc(
		std::string name,
		Texture::Type type,
		uint32_t width,
		uint32_t height,
		uint32_t mipLevels,
		Texture::Format format,
		Texture::Usage usage = Texture::Usage::NONE,
		float* clearColor = nullptr,
		Texture::UAVType uavType = Texture::UAVType::NONE,
		uint32_t arraySize = 1) :
		name(name),
		type(type),
		width(width),
		height(height),
		mipLevels(mipLevels),
		format(format),
		srvFormat(format),
		uavFormat(format),
		rtvFormat(format),
		dsvFormat(format),
		usage(usage),
		clearColor(clearColor),
		uavType(uavType),
		arraySize(arraySize) {}
};

enum class TextAlign
{
	LeftTop = 0,
	LeftCenter,
	LeftBottom,
	CenterTop,
	CenterCenter,
	CenterBottom,
	RightTop,
	RightCenter,
	RightBottom
};

enum class TextBoxAlign
{
	LeftTop = 0,
	CenterCenter,
};

class ANIMAVISION_DLL Renderer
{
public:
	enum class API
	{
		NONE,
		DirectX11,
		DirectX12
	};

public:
	virtual ~Renderer() = default;

	virtual void Resize(uint32_t width, uint32_t height) = 0;
	virtual void SetFullScreen(bool fullScreen) {}
	virtual void SetVSync(bool vsync) {}
	virtual void Clear(const float* RGBA) = 0;
	virtual void ClearTexture(Texture* texture, const float* clearColor) {}

	virtual void SetRenderTargets(uint32_t numRenderTargets, Texture* renderTargets[], Texture* depthStencil = nullptr, bool useDefaultDSV = true) = 0;
	virtual void SetViewport(uint32_t width, uint32_t height) = 0;
	virtual void ApplyRenderState(BlendState blendstate, RasterizerState rasterizerstate, DepthStencilState depthstencilstate) = 0;

	virtual void Submit(Mesh& mesh, Material& material, PrimitiveTopology primitiveMode = PrimitiveTopology::TRIANGLELIST, uint32_t instances = 1) = 0;
	virtual void Submit(Mesh& mesh, Material& material, uint32_t subMeshIndex, PrimitiveTopology primitiveMode = PrimitiveTopology::TRIANGLELIST, uint32_t instances = 1) = 0;
	virtual void DispatchCompute(Material& material, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) {}
	virtual void DispatchRays(Material& material, uint32_t width, uint32_t height, uint32_t depth) {}
	virtual void CopyStructureCount(Buffer* dest, uint32_t distAlignedByteOffset, Texture* src) {}
	virtual void SubmitInstancedIndirect(Material& material, Texture* argsBuffer, uint32_t allignedByteOffsetForArgs, PrimitiveTopology primitiveMode = PrimitiveTopology::TRIANGLELIST) {}

	virtual bool IsRayTracing() const { return false; }

	virtual void BeginRender() = 0;
	virtual void EndRender() = 0;

	virtual std::shared_ptr<Texture> GetTexture(const char* path) = 0;
	virtual std::shared_ptr<Texture> CreateTexture(const char* path) = 0;
	virtual std::shared_ptr<Texture> CreateTexture2DArray(const char* path) { return nullptr; }
	virtual std::shared_ptr<Texture> CreateTextureCube(const char* path) { return nullptr; }
	virtual std::shared_ptr<Texture> CreateLightmapTexture(const char* path) { return nullptr; }
	virtual void SaveLightmapTextureArray(std::vector<std::shared_ptr<Texture>> lightmaps, std::string path) {}

	virtual std::shared_ptr<Texture> CreateEmptyTexture(std::string name, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Format format, Texture::Usage usage = Texture::Usage::NONE, float* clearColor = nullptr, Texture::UAVType uavType = Texture::UAVType::NONE, uint32_t arraySize = 1) = 0;
	virtual std::shared_ptr<Texture> CreateEmptyTexture(const TextureDesc& desc) = 0;

	virtual void ReleaseTexture(const std::string& name) = 0;

	virtual std::shared_ptr<Shader> LoadShader(const char* srcPath) = 0;
	virtual void LoadShadersFromDrive(const std::string& path) = 0;
	virtual std::map<std::string, std::shared_ptr<Shader>>* GetShaders() { return nullptr; }

	virtual RendererContext* GetContext() = 0;
	virtual void* GetShaderResourceView() = 0;

	virtual uint32_t GetWidth() const { return 0; }
	virtual uint32_t GetHeight() const { return 0; }
	virtual bool IsFullScreen() const { return false; }
	virtual bool IsVSync() const { return false; }

	virtual void AddMesh(const std::shared_ptr<Mesh>& mesh) = 0;
	virtual void LoadMeshesFromDrive(const std::string& path) = 0;
	virtual std::shared_ptr<Mesh> GetMesh(const std::string& name) = 0;
	virtual std::map<std::string, std::shared_ptr<Mesh>>* GetMeshes() = 0;

	virtual void AddMaterial(const std::shared_ptr<Material>& material) = 0;
	virtual void LoadMaterialsFromDrive(const std::string& path) = 0;
	virtual std::shared_ptr<Material> GetMaterial(const std::string& name) = 0;
	virtual void SaveMaterial(const std::string& name) = 0;
	virtual std::map<std::string, std::shared_ptr<Material>>* GetMaterials() = 0;

	virtual void LoadAnimationClipsFromDrive(const std::string& path) = 0;
	virtual std::shared_ptr<AnimationClip> GetAnimationClip(const std::string& name) = 0;
	virtual std::map<std::string, std::shared_ptr<AnimationClip>>* GetAnimationClips() = 0;

	// Particle
	virtual std::shared_ptr<Texture> CreateRandomTexture() { return nullptr; }
	virtual std::shared_ptr<Texture> CreateIndirectDrawTexture() { return nullptr; }
	virtual float GetRandomSeed(float min, float max) { return 0.0f; }

	using ParticleBuffers = std::tuple<std::shared_ptr<Texture>, std::shared_ptr<Texture>, std::shared_ptr<Texture>>;
	virtual ParticleBuffers CreateParticleBufferTextures(uint32_t maxCount, uint32_t entity) { return ParticleBuffers(); }
	virtual void LoadParticleTexturesFromDrive(const std::string& path) {}
	virtual std::shared_ptr<Texture> GetParticleTexture(std::string name) { return nullptr; }
	virtual std::map<std::string, std::shared_ptr<Texture>>* GetParticleTextures() { return nullptr; }

	// DebugRender
	virtual void SetDebugViewProj(const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj) {}
	virtual void DrawLine(const DirectX::SimpleMath::Vector3& from, const DirectX::SimpleMath::Vector3& to, const DirectX::SimpleMath::Vector4& color) {}
	virtual void DrawBoundingBox(const DirectX::BoundingBox& box, const DirectX::SimpleMath::Vector4& color) {}
	virtual void DrawOrientedBoundingBox(const DirectX::BoundingOrientedBox& box, const DirectX::SimpleMath::Vector4& color) {}
	virtual void DrawBoundingSphere(const DirectX::BoundingSphere& sphere, const DirectX::SimpleMath::Vector4& color) {}

	virtual void BeginDebugRender() {}
	virtual void EndDebugRender() {}

	// DX11
	virtual void* GetDevice() { return nullptr; }
	virtual void* GetDeviceContext() { return nullptr; }

	// DX12
	virtual void* AllocateShaderVisibleDescriptor(void* OutCpuHandle, void* OutGpuHandle) { return nullptr; }
	virtual void* GetRenderCommandList() { return nullptr; }

	// dxr
	virtual void AddMeshToScene(std::shared_ptr<Mesh> mesh, const std::vector<std::shared_ptr<Material>>& materials, uint32_t* pOutHitDistribution, bool isRebuild = true) { }
	virtual void AddBottomLevelAS(Mesh& mesh, bool allowUpdate = false) {}
	virtual	void AddSkinnedBottomLevelAS(uint32_t id, Mesh& mesh, bool allowUpdate = false) {}
	virtual UINT AddBottomLevelASInstance(const std::string& meshName, uint32_t hitDistribution, float* transfrom) { return 0; }
	virtual UINT AddSkinnedBLASInstance(uint32_t id, uint32_t hitDistribution, float* transform) { return 0; }
	virtual void UpdateAccelerationStructures() {}
	virtual void InitializeTopLevelAS() {}
	virtual void ComputeSkinnedVertices(Mesh& mesh, const std::vector<Matrix>& boneMatrices, const uint32_t& vertexCount) {}
	virtual void ModifyBLASGeometry(uint32_t id, Mesh& mesh) {}
	virtual void ResetSkinnedBLAS() {}

	virtual std::shared_ptr<Texture> GetColorRt() { return nullptr; }

	// raytracing
	virtual void SetCamera(
		const DirectX::SimpleMath::Matrix& view,
		const DirectX::SimpleMath::Matrix& proj,
		const DirectX::SimpleMath::Vector3& cameraPosition,
		const DirectX::SimpleMath::Vector3& forward,
		const DirectX::SimpleMath::Vector3& up, float ZMin, float ZMax) {}
	virtual void DoRayTracing() {}
	virtual void FlushRaytracingData() {}
	virtual void ResetAS() {}

	// ui
	virtual void LoadUITexturesFromDrive(const std::string& path) {}
	virtual std::shared_ptr<Texture> GetUITexture(const std::string& name) { return nullptr; }
	virtual std::shared_ptr<Texture> CreateTexture(const char* path, bool isUI) { return nullptr; }
	virtual robin_hood::unordered_map<std::string, std::shared_ptr<Texture>>* GetUITextures() { return nullptr; }

	// DWrite
	virtual void SubmitText(Texture* rt, Font* font, const std::string& text, Vector2 posH, Vector2 size, Vector2 scale, DirectX::SimpleMath::Color color, float fontSize, TextAlign textAlign = TextAlign::LeftTop, TextBoxAlign boxAlign = TextBoxAlign::CenterCenter, bool useUnderline = false, bool useStrikeThrough = false) {}
	virtual void SubmitComboBox(Texture* rt, Font* font, const std::vector<std::string>& text, uint32_t curIndex, bool isOpen, Vector2 posH, float leftPadding, Vector2 size, Vector2 scale, DirectX::SimpleMath::Color color, float fontSize, TextAlign textAlign = TextAlign::LeftTop, TextBoxAlign boxAlign = TextBoxAlign::CenterCenter, bool useUnderline = false) {}
	virtual void LoadFontsFromDrive(const std::string& path) {}
	virtual std::shared_ptr<Font> GetFont(const std::string& name) { return nullptr; }
	virtual robin_hood::unordered_map<std::string, std::shared_ptr<Font>>* GetFonts() { return nullptr; }

	static std::unique_ptr<Renderer> Create(HWND hwnd, uint32_t width, uint32_t height, API api, bool isRaytracing = false);

	inline static API s_Api = API::NONE;
};