#pragma once
#include "Renderer.h"
#include "RendererContext.h"

#include "DX12Buffer.h"
// TEST TODO : 테스트용 삭제할것
#include "DX12ShaderCompiler.h"
// TODO 이거 대대적인 수정이 필요함
// 1. Shader를 외부에서 로드할 수 있도록 해야함
// 2. Shader를 외부에서 로드하면서, Shader에 대한 정보를 리플렉션할수 있도록 해야함
// 3. 루트 시그니처가 여기에 있어야하는지 고찰

class Renderer;
class DX12Shader;


// Shader record = {{Shader ID}, {RootArguments}}
class ShaderRecord
{
public:
	ShaderRecord(void* pShaderIdentifier, UINT shaderIdentifierSize) :
		shaderIdentifier(pShaderIdentifier, shaderIdentifierSize)
	{
	}

	ShaderRecord(void* pShaderIdentifier, UINT shaderIdentifierSize, void* pLocalRootArguments, UINT localRootArgumentsSize) :
		shaderIdentifier(pShaderIdentifier, shaderIdentifierSize),
		localRootArguments(pLocalRootArguments, localRootArgumentsSize)
	{
	}

	void CopyTo(void* dest) const
	{
		uint8_t* byteDest = static_cast<uint8_t*>(dest);
		memcpy(byteDest, shaderIdentifier.ptr, shaderIdentifier.size);
		if (localRootArguments.ptr)
		{
			memcpy(byteDest + shaderIdentifier.size, localRootArguments.ptr, localRootArguments.size);
		}
	}

	struct PointerWithSize {
		void* ptr;
		UINT size;

		PointerWithSize() : ptr(nullptr), size(0) {}
		PointerWithSize(void* _ptr, UINT _size) : ptr(_ptr), size(_size) {};
	};
	PointerWithSize shaderIdentifier;
	PointerWithSize localRootArguments;
};




// Shader table = {{ ShaderRecord 1}, {ShaderRecord 2}, ...}
class ShaderTable : public GpuUploadBuffer
{
	uint8_t* m_mappedShaderRecords;
	UINT m_shaderRecordSize;

	// Debug support
	std::wstring m_name;
	std::vector<ShaderRecord> m_shaderRecords;

	ShaderTable() {}
public:
	ShaderTable(ID3D12Device5* device, UINT numShaderRecords, UINT shaderRecordSize, LPCWSTR resourceName = nullptr)
		: m_name(resourceName)
	{
		m_shaderRecordSize = Align(shaderRecordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
		m_shaderRecords.reserve(numShaderRecords);
		UINT bufferSize = numShaderRecords * m_shaderRecordSize;
		Allocate(device, bufferSize, resourceName);
		m_mappedShaderRecords = MapCpuWriteOnly();
	}

	void push_back(const ShaderRecord& shaderRecord)
	{
		assert(m_shaderRecords.size() < m_shaderRecords.capacity());
		m_shaderRecords.push_back(shaderRecord);
		shaderRecord.CopyTo(m_mappedShaderRecords);
		m_mappedShaderRecords += m_shaderRecordSize;
	}

	UINT GetShaderRecordSize() { return m_shaderRecordSize; }
	UINT GeNumShaderRecords() { return static_cast<UINT>(m_shaderRecords.size()); }

	// Pretty-print the shader records.
	void DebugPrint(robin_hood::unordered_map<void*, std::wstring> shaderIdToStringMap)
	{
		std::wstringstream wstr;
		wstr << L"| --------------------------------------------------------------------\n";
		wstr << L"|Shader table - " << m_name.c_str() << L": "
			<< m_shaderRecordSize << L" | "
			<< m_shaderRecords.size() * m_shaderRecordSize << L" bytes\n";

		for (UINT i = 0; i < m_shaderRecords.size(); i++)
		{
			wstr << L"| [" << i << L"]: ";
			wstr << shaderIdToStringMap[m_shaderRecords[i].shaderIdentifier.ptr] << L", ";
			wstr << m_shaderRecords[i].shaderIdentifier.size << L" + " << m_shaderRecords[i].localRootArguments.size << L" bytes \n";
		}
		wstr << L"| --------------------------------------------------------------------\n";
		wstr << L"\n";

		OutputDebugStringW(wstr.str().c_str());
	}
};


class PipelineState
{
public:
	struct PipelineStateBuilder
	{
		PipelineStateBuilder(Renderer* renderer) : 
			m_Renderer(renderer),
			m_Shader(nullptr),
			m_PrimitiveTopology(PrimitiveTopology::TRIANGLELIST),
			m_RasterizerState(RasterizerState::CULL_BACK),
			m_BlendState(BlendState::NO_BLEND) {}

		PipelineStateBuilder& SetShader(std::shared_ptr<DX12Shader> shader)
		{
			m_Shader = shader;
			return *this;
		}

		PipelineStateBuilder& SetPrimitiveTopology(PrimitiveTopology topology)
		{
			m_PrimitiveTopology = topology;
			return *this;
		}

		PipelineStateBuilder& SetRasterizerState(RasterizerState state)
		{
			m_RasterizerState = state;
			return *this;
		}

		PipelineStateBuilder& SetBlendState(BlendState state)
		{
			m_BlendState = state;
			return *this;
		}

		PipelineStateBuilder& SetDepthStencilState(DepthStencilState state)
		{
			m_DepthStencilState = state;
			return *this;
		}

		std::unique_ptr<PipelineState> Build()
		{
			auto pipelineState = std::make_unique<PipelineState>(m_Shader, m_PrimitiveTopology, m_RasterizerState, m_BlendState, m_DepthStencilState);
			pipelineState->Build(m_Renderer);

			return pipelineState;
		}

		Renderer* m_Renderer;
		std::shared_ptr<DX12Shader> m_Shader;
		PrimitiveTopology m_PrimitiveTopology;
		RasterizerState m_RasterizerState;
		BlendState m_BlendState;
		DepthStencilState m_DepthStencilState;
	};

	PipelineState(std::shared_ptr<DX12Shader> shader, PrimitiveTopology topology, RasterizerState rasterizerState, BlendState blendState, DepthStencilState depthStencilState)
		: m_Shader(shader), m_PrimitiveTopology(topology), m_RasterizerState(rasterizerState), m_BlendState(blendState), m_DepthStencilState(depthStencilState) {}
	
	void Build(Renderer* renderer);

	void Bind(RendererContext* context);

	ID3D12PipelineState* GetPipelineState() { return m_PipelineState.get(); }

	std::shared_ptr<DX12Shader> GetShader() { return m_Shader; }

private:
	std::shared_ptr<DX12Shader> m_Shader;

	winrt::com_ptr<ID3D12PipelineState> m_PipelineState;

	PrimitiveTopology m_PrimitiveTopology;
	RasterizerState m_RasterizerState;
	BlendState m_BlendState;
	DepthStencilState m_DepthStencilState;

};

class PipelineStateArray
{
public:
	std::unique_ptr<PipelineState> m_PipelineStates
		[static_cast<size_t>(PrimitiveTopology::COUNT)]
		[static_cast<size_t>(RasterizerState::COUNT)]
		[static_cast<size_t>(BlendState::COUNT)]
		[static_cast<size_t>(DepthStencilState::COUNT)];
};

class ComputePipelineState
{
public:
	ComputePipelineState(Renderer* renderer, std::shared_ptr<DX12Shader> shader);

	void Bind(Renderer* renderer);

	std::shared_ptr<DX12Shader> GetComputeShader() { return m_ComputeShader; }
	ID3D12PipelineState* GetComputePipelineState() { return m_ComputePipelineState.get(); }

private:
	std::shared_ptr<DX12Shader> m_ComputeShader;

	winrt::com_ptr<ID3D12PipelineState> m_ComputePipelineState;
};


class RayTracingState
{
public:
	enum class TableType
	{
		Raygen,
		Miss,
		HitGroup,
		Count
	};

public:
	inline static const std::wstring HITGROUP_NAME = L"HitGroup";

	struct __declspec(align(256)) ShaderParameter
	{
		char ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
		uint64_t RootParameters[1]; // 일단은 지역 매개변수가 없으므로 사용하지 않는다.
	};

	RayTracingState(Renderer* renderer, std::shared_ptr<DX12Shader> shader);

	void Bind(Renderer* renderer);

	std::shared_ptr<DX12Shader> GetRayTracingShader() { return m_RayTracingShader; }
	ID3D12StateObject* GetStateObject() { return m_StateObject.get(); }
	ID3D12StateObjectProperties* GetStateObjectProperties() { return m_StateObjectProperties.get(); }

private:
	void CreateShaderBindingTables(Renderer* renderer);

private:
	winrt::com_ptr<ID3D12StateObject> m_StateObject;
	winrt::com_ptr<ID3D12StateObjectProperties> m_StateObjectProperties;

	std::shared_ptr<DX12Shader> m_RayTracingShader;

	winrt::com_ptr<ID3D12Resource> m_ShaderBindingTables[static_cast<size_t>(TableType::Count)];
	UINT m_ShaderTableStrideInBytes[static_cast<size_t>(TableType::Count)];

	std::vector<std::wstring> m_HitGroupNames;

	friend class ChangDXII;
	
};
