#pragma once
#include "Shader.h"
#include "ShaderResource.h"

class DX12Shader;
class RendererContext;
class DX12ConstantBuffer;


enum ShaderStage : unsigned
{
	VS = 0,
	PS,
	GS,
	CS,
	HS,
	DS,
	RayTracing,

	COUNT
};

enum class RaytracingShaderType
{
	RayGeneration,
	Miss,
	ClosestHit,
	AnyHit,
	Intersection,
	Callable,
};

struct ConstantBufferMapping
{
	std::string Name;
	uint32_t Register;
	uint32_t Space;
	uint32_t Offset;
	uint32_t Size;
	DX12ConstantBuffer* OriginalBuffer;
};

struct BufferBinding
{
	std::string Name;
	uint32_t Register;
	uint32_t Space;
	ShaderStage Stage;
	uint32_t ParameterIndex;
};

struct TextureBinding
{
	std::string Name;
	uint32_t Register;
	uint32_t Space;
	ShaderStage Stage;
	uint32_t ParameterIndex;
	Texture::Type TextureDimension;
};

struct SamplerBinding
{
	std::string Name;
	uint32_t Register;
	uint32_t Space;
	ShaderStage Stage;
	uint32_t ParameterIndex;
};

struct UAVBinding
{
	std::string Name;
	uint32_t Register;
	uint32_t Space;
	ShaderStage Stage;
	uint32_t ParameterIndex;
	Texture::Type TextureDimension;
};

struct AccelationStructureBinding
{
	std::string Name;
	uint32_t Register;
	uint32_t Space;
	ShaderStage Stage;
	uint32_t ParameterIndex;
	Texture::Type TextureDimension;
};

// template <class Archive>
// void Save(Archive& ar, const winrt::com_ptr<IDxcBlob>& blob)
// {
// 	// 		 blob을 byte배열화시킨다.
// 	std::vector<uint8_t> data(blob->GetBufferSize());
// 	memcpy(data.data(), blob->GetBufferPointer(), blob->GetBufferSize());
// 	ar(data);
// }
// 
// template <class Archive>
// void Load(Archive& ar, winrt::com_ptr<IDxcBlob>& blob)
// {
// 	std::vector<uint8_t> data;
// 	ar(data);
// 
// 	blob = nullptr;
// 	blob = winrt::make<IDxcBlob>(data.data(), data.size());
// }

class DX12Shader : public Shader
{
public:
	enum RootType
	{
		ROOT_GLOBAL,
		ROOT_LOCAL,
		ROOT_COUNT
	};

	struct ConstantBufferLayout
	{
		std::string Name;
		D3D12_SHADER_BUFFER_DESC Desc;
		std::vector<D3D12_SHADER_VARIABLE_DESC> Variables;
		std::vector<D3D12_SHADER_TYPE_DESC> Types;
		uint32_t Size;
		ShaderStage Stage;
		uint32_t Register;
		uint32_t ParameterIndex;
		uint32_t SizeAligned256;
	};

	DX12Shader(Renderer* renderer, const std::string& filePath);

	void Bind(Renderer* renderer) override;
	void BindContext(RendererContext* context) override;

	void Unbind() { DeallocateConstantBuffer(); }
	void ResetAllocateStates();

	void SetInt(const std::string& name, int value) override { SetConstantMapping(name, &value); }
	void SetIntArray(const std::string& name, int* value) override { SetConstantMapping(name, value); }
	void SetFloat(const std::string& name, float value) override { SetConstantMapping(name, &value); }
	void SetFloat2(const std::string& name, const Vector2& value) override { SetConstantMapping(name, &value); }
	void SetFloat3(const std::string& name, const Vector3& value) override { SetConstantMapping(name, &value); }
	void SetFloat4(const std::string& name, const Vector4& value) override { SetConstantMapping(name, &value); }
	void SetMatrix(const std::string& name, const Matrix& value) override { SetConstantMapping(name, &value); }
	void SetStruct(const std::string& name, const void* value) override { SetConstantMapping(name, value); }
	void SetConstant(const std::string& name, const void* value, uint32_t size) override;

	winrt::com_ptr<ID3D12RootSignature> GetRootSignature(RootType type = ROOT_GLOBAL) { return m_RootSignature[type]; }


private:
	bool loadFromHLSL(const std::string& filePath);
 	bool loadFromBinary(const std::string& filePath);
 	void saveToBinary(const std::string& filePath);



	const robin_hood::unordered_map <std::string, const CD3DX12_STATIC_SAMPLER_DESC> GetStaticSamplers();

	void SetConstantMapping(const std::string& name, const void* value);

	void ReflectShaderVariables();
	void CreateRootSignature(Renderer* renderer);

private:
	void CreateRootSignatureHelper(Renderer* renderer, RootType type, const std::wstring& name);

	template <typename T>
	void VerifyAndPush(T& value, std::vector<T>& vec)
	{
		bool isFound = false;

		for (auto& element : vec)
		{
			if (value.Name == element.Name)
			{
				isFound = true;
				if (value.Register != element.Register)
				{
					OutputDebugStringA("Buffer register mismatch\n");
					__debugbreak();
					break;
				}
			}
		}
		if (!isFound)
			vec.push_back(value);
	}

	void DetermineRaytracingShaderType(const std::string& name, RaytracingShaderType& type);

	void CreateConstantBuffer(Renderer* renderer);
	void ReflectResourceBindings(ID3D12ShaderReflection* reflection, ShaderStage stage);

	// 오브젝트 개별로 셰이더에서 할당해준다.
	void AllocateConstantBuffer();
	void DeallocateConstantBuffer() { m_IsAllocated = false; }

	// 레이트레이싱
	void ReflectLibraryVariables();

public:
	winrt::com_ptr<IDxcBlob> m_ShaderBlob[ShaderStage::COUNT] = { nullptr };
	winrt::com_ptr<ID3D12ShaderReflection> m_Reflection[ShaderStage::COUNT] = { nullptr };
	winrt::com_ptr<ID3D12LibraryReflection> m_LibraryReflection = nullptr;

	// 동적생성
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;

	std::vector<ConstantBufferLayout> m_ConstantBufferLayouts[ROOT_COUNT];

	std::vector<std::shared_ptr<DX12ConstantBuffer>> m_ConstantBuffers;
	robin_hood::unordered_map<std::string, DX12ConstantBuffer*> m_ConstantBufferMap;
	robin_hood::unordered_map<std::string, ConstantBufferMapping> m_ConstantBufferMappings;
	bool m_IsAllocated = false;

	std::vector<TextureBinding> m_TextureBindings[ROOT_COUNT];
	robin_hood::unordered_map<std::string, uint32_t> m_TextureIndexMap[ROOT_COUNT];

	robin_hood::unordered_map<std::string, SamplerBinding> m_SamplerBindings[ROOT_COUNT];
	
	std::vector<UAVBinding> m_UAVBindings[ROOT_COUNT];
	robin_hood::unordered_map<std::string, uint32_t> m_UAVIndexMap[ROOT_COUNT];

	std::vector<AccelationStructureBinding> m_AccelationStructureBindings[ROOT_COUNT];
	robin_hood::unordered_map<std::string, uint32_t> m_AccelationStructureIndexMap[ROOT_COUNT];

	std::vector<BufferBinding> m_BufferBindings[ROOT_COUNT];
	robin_hood::unordered_map<std::string, uint32_t> m_BufferIndexMap[ROOT_COUNT];

	/// 루트 시그니처는 셰이더에 종속적이므로 셰이더 클래스에 둔다.
	winrt::com_ptr<ID3D12RootSignature> m_RootSignature[ROOT_COUNT];

	Renderer* m_Renderer = nullptr;

	std::vector<Texture::Format> m_RTVFormats;

	// 레이트레이싱
	std::unordered_multimap<RaytracingShaderType, std::wstring> m_DxrFunctions;


// 	template <class Archive>
// 	void save(Archive& ar) const
// 	{
// 		//ar(shader.m_ShaderBlob);
// 		for (int i = 0; i < ShaderStage::COUNT; ++i)
// 		{
// 			::Save(ar, m_ShaderBlob[i]);
// 		}
// 	}
// 
// 	template <class Archive>
// 	void load(Archive& ar)
// 	{
// 		//ar(shader.m_ShaderBlob);
// 		for (int i = 0; i < ShaderStage::COUNT; ++i)
// 		{
// 			::Load(ar, m_ShaderBlob[i]);
// 		}
// 
// 	}
};
