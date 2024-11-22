#pragma once

#include "Shader.h"

#include "DX11Relatives.h"
#include <array>

class Buffer;

enum class ShaderType : uint32_t
{
	Vertex,
	Pixel,
	Compute,
	Geometry,
	StreamOutGeometry,
	Hull,
	Domain,

	Count
};

struct CBMapping
{
	ShaderType Type = ShaderType::Count;
	std::string BufferName = "";
	std::string VariableName = "";
	uint32_t Register = 0;
	uint32_t Offset = 0;
	uint32_t Size = 0;
	BYTE** Data = nullptr;
	uint32_t Index = 0;
};

enum class SamplerType
{
	gsamPointWrap,
	gsamPointClamp,
	gsamLinearWrap,
	gsamLinearClamp,
	gsamAnisotropicWrap,
	gsamAnisotropicClamp,
	gsamShadow,

	Count
};

struct DX11ResourceBinding
{
	// 어느 shader에 resource가 binding되는지
	ShaderType Type;
	std::string Name;
	uint32_t Register;
	uint32_t BindCount;
	uint32_t Flags;
	uint32_t ReturnType;
	uint32_t Dimension;
	uint32_t NumSamples;
	_D3D_SHADER_INPUT_TYPE ResourceType;

	// 샘플러일떄만
	SamplerType samplerType = SamplerType::Count;
};

/// WOO : 나중에 animation 추가되면 수정 필요
/// 그리고 똑같은거 여러 개 쓴다고 하면 아직 잘 모르겠음
struct InputLayoutFlags
{
	bool usePosition = false;
	bool useNormal = false;
	bool useUV1 = false;
	bool useUV2 = false;
	bool useTangent = false;
	bool useBiTangent = false;
	bool useColor = false;
	uint32_t positionStride = 0;
	uint32_t normalStride = 0;
	uint32_t uv1Stride = 0;
	uint32_t uv2Stride = 0;
	uint32_t tangentStride = 0;
	uint32_t biTangentStride = 0;
	uint32_t colorStride = 0;
};

class DX11ConstantBuffer;
class Mesh;

class DX11Shader : public Shader
{
	friend class ParticleManager;

public:
	struct CBLayout
	{
		D3D11_SHADER_BUFFER_DESC Desc;
		std::vector<D3D11_SHADER_VARIABLE_DESC> Variables;
		std::vector<D3D11_SHADER_TYPE_DESC> Types;
		uint32_t Size = 0;
		ShaderType Type = ShaderType::Count;
		uint32_t Register = 0;
		uint32_t ParameterIndex = 0;
	};

	DX11Shader(RendererContext* context, std::string_view path);
	DX11Shader(const DX11Shader& other) = default;
	DX11Shader& operator=(const DX11Shader& other) = default;
	virtual ~DX11Shader();

	// Shader을(를) 통해 상속됨
	void Bind(Renderer* context) override;
	void Unbind(Renderer* context) override;

	void SetInt(const std::string& name, int value) override;
	void SetIntArray(const std::string& name, int* value) override;
	void SetFloat(const std::string& name, float value) override;
	void SetFloat2(const std::string& name, const Vector2& value) override;
	void SetFloat3(const std::string& name, const Vector3& value) override;
	void SetFloat4(const std::string& name, const Vector4& value) override;
	void SetMatrix(const std::string& name, const Matrix& value) override;
	void SetStruct(const std::string& name, const void* value) override;
	void SetConstant(const std::string& name, const void* value, uint32_t size) override;
	void MapConstantBuffer(RendererContext* context) override;
	void UnmapConstantBuffer(RendererContext* context) override;
	void MapConstantBuffer(RendererContext* context, std::string bufName) override;
	void UnmapConstantBuffer(RendererContext* context, std::string bufName) override;
	void Recompile(RendererContext* context);
	Buffer* GetConstantBuffer(const std::string& name, int index) override;

	std::vector<DX11ResourceBinding> GetTextureBindings() const { return m_TextureBindings; }
	std::vector<DX11ResourceBinding> GetSamplerBindings() const { return m_SamplerBindings; }
	std::vector<DX11ResourceBinding> GetUAVBindings() const { return m_UAVBindings; }

	void Destroy();

	ID3D11VertexShader* GetVertexShader() const { return static_cast<ID3D11VertexShader*>(m_Shaders[static_cast<uint32_t>(ShaderType::Vertex)].Get()); }
	ID3D11PixelShader* GetPixelShader() const { return static_cast<ID3D11PixelShader*>(m_Shaders[static_cast<uint32_t>(ShaderType::Pixel)].Get()); }
	ID3D11ComputeShader* GetComputeShader() const { return static_cast<ID3D11ComputeShader*>(m_Shaders[static_cast<uint32_t>(ShaderType::Compute)].Get()); }
	ID3D11GeometryShader* GetGeometryShader() const { return static_cast<ID3D11GeometryShader*>(m_Shaders[static_cast<uint32_t>(ShaderType::Geometry)].Get()); }
	ID3D11GeometryShader* GetStreamOutGeometryShader() const { return static_cast<ID3D11GeometryShader*>(m_Shaders[static_cast<uint32_t>(ShaderType::StreamOutGeometry)].Get()); }
	ID3D11HullShader* GetHullShader() const { return static_cast<ID3D11HullShader*>(m_Shaders[static_cast<uint32_t>(ShaderType::Hull)].Get()); }
	ID3D11DomainShader* GetDomainShader() const { return static_cast<ID3D11DomainShader*>(m_Shaders[static_cast<uint32_t>(ShaderType::Domain)].Get()); }
	void setConstantMapping(const std::string& name, const void* value);

private:
	ID3DBlob* compileShaderFromFile(std::string_view path, std::string_view name, std::string_view version);
	bool saveShaderBlobToFile(ID3DBlob* blob, std::string_view path);
	bool loadShaderBlobFromFile(ID3DBlob** blob, std::string_view path);
	void reflectShaderVariables();

	void createInputLayout(RendererContext* context);
	void createConstantBuffer(RendererContext* context);
	void createVertexShader(RendererContext* context, std::string_view path);
	void createPixelShader(RendererContext* context, std::string_view path);
	void createComputeShader(RendererContext* context, std::string_view path);
	void createGeometryShader(RendererContext* context, std::string_view path);
	void createStreamOutGeometryShader(RendererContext* context, std::string_view path);
	void createHullShader(RendererContext* context, std::string_view path);
	void createDomainShader(RendererContext* context, std::string_view path);


	ID3DBlob* getVertexBlob() const { return m_ShaderBlobs[static_cast<uint32_t>(ShaderType::Vertex)].Get(); }
	ID3DBlob* getPixelBlob() const { return m_ShaderBlobs[static_cast<uint32_t>(ShaderType::Pixel)].Get(); }
	ID3DBlob* getComputeBlob() const { return m_ShaderBlobs[static_cast<uint32_t>(ShaderType::Compute)].Get(); }
	ID3DBlob* getGeometryBlob() const { return m_ShaderBlobs[static_cast<uint32_t>(ShaderType::Geometry)].Get(); }
	ID3DBlob* getStreamOutGeometryBlob() const { return m_ShaderBlobs[static_cast<uint32_t>(ShaderType::StreamOutGeometry)].Get(); }
	ID3DBlob* getHullBlob() const { return m_ShaderBlobs[static_cast<uint32_t>(ShaderType::Hull)].Get(); }
	ID3DBlob* getDomainBlob() const { return m_ShaderBlobs[static_cast<uint32_t>(ShaderType::Domain)].Get(); }

public:
	/// ID3D11DeviceChild -> DX11 shader class의 부모 class
	std::array<Microsoft::WRL::ComPtr<ID3D11DeviceChild>, static_cast<uint32_t>(ShaderType::Count)> m_Shaders = {};
	std::array<Microsoft::WRL::ComPtr<ID3DBlob>, static_cast<uint32_t>(ShaderType::Count)> m_ShaderBlobs = {};
	std::array<Microsoft::WRL::ComPtr<ID3D11ShaderReflection>, static_cast<uint32_t>(ShaderType::Count)> m_ShaderReflections = {};

	std::vector<D3D11_INPUT_ELEMENT_DESC> m_InputLayouts = {};
	winrt::com_ptr<ID3D11InputLayout> m_InputLayout = nullptr;
	InputLayoutFlags m_InputLayoutFlags = {};
	uint32_t m_Stride = 0;

	std::vector<std::shared_ptr<DX11ConstantBuffer>> m_ConstantBuffers = {};
	std::vector<CBLayout> m_CBLayouts = {};
	robin_hood::unordered_map<std::string, std::vector<DX11ConstantBuffer*>> m_ConstantBufferMap = {};
	robin_hood::unordered_map<std::string, std::vector<CBMapping>> m_CBMapping = {};

	std::vector<DX11ResourceBinding> m_TextureBindings = {};
	robin_hood::unordered_map<std::string, uint32_t> m_TextureRegisterMap = {};

	std::vector<DX11ResourceBinding> m_SamplerBindings = {};
	robin_hood::unordered_map<std::string, uint32_t> m_SamplerRegisterMap = {};

	std::vector<DX11ResourceBinding> m_UAVBindings = {};
	robin_hood::unordered_map<std::string, uint32_t> m_UAVRegisterMap = {};

	// 기본 변수
	std::string m_Path = "";
};

class ShaderIncludeHandler : public ID3DInclude
{
public:
	STDMETHOD(Open)(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) override;
	STDMETHOD(Close)(LPCVOID pData) override;
};
