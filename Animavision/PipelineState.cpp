#include "pch.h"
#include "PipelineState.h"
#include "DX12Context.h"
#include "DX12Helper.h"
#include "DX12Shader.h"
#include "Renderer.h"

#include "DX12Buffer.h"
#include "ChangDXII.h"

void PipelineState::Build(Renderer* renderer)
{
	auto context = renderer->GetContext();
	ID3D12Device* device = static_cast<DX12Context*>(context)->GetDevice();

	IDxcBlob* VertexShaderBlob = m_Shader->m_ShaderBlob[ShaderStage::VS].get();
	IDxcBlob* PixelShaderBlob = m_Shader->m_ShaderBlob[ShaderStage::PS].get();
	IDxcBlob* geometryShaderBlob = m_Shader->m_ShaderBlob[ShaderStage::GS].get();

	if (VertexShaderBlob == nullptr || (PixelShaderBlob == nullptr && geometryShaderBlob == nullptr))
	{
		OutputDebugStringA("Failed to load shader\n");
		return;
	}

	auto rootSignature = m_Shader->GetRootSignature();

	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { m_Shader->m_InputLayout.data(), static_cast<UINT>(m_Shader->m_InputLayout.size()) };
	psoDesc.pRootSignature = rootSignature.get();
	if (geometryShaderBlob != nullptr)
	{
		psoDesc.GS = CD3DX12_SHADER_BYTECODE(geometryShaderBlob->GetBufferPointer(), geometryShaderBlob->GetBufferSize());
	}
	if (VertexShaderBlob != nullptr)
	{
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize());
	}
	if (PixelShaderBlob != nullptr)
	{
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(PixelShaderBlob->GetBufferPointer(), PixelShaderBlob->GetBufferSize());
	}
	// 	psoDesc.DepthStencilState.DepthEnable = false;
	// 	psoDesc.DepthStencilState.StencilEnable = false;

	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = (UINT)m_Shader->m_RTVFormats.size();
	for (size_t i = 0; i < m_Shader->m_RTVFormats.size(); i++)
	{
		psoDesc.RTVFormats[i] = (DXGI_FORMAT)m_Shader->m_RTVFormats[i];
	}
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// 기본 레스터라이저 상태
	D3D12_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.ForcedSampleCount = 0;
	rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	switch (m_RasterizerState)
	{
	case RasterizerState::CULL_NONE:
		rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
		break;
	case RasterizerState::CULL_BACK:
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		break;
	case RasterizerState::CULL_FRONT:
		rasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT;
		break;
	case RasterizerState::WIREFRAME:
		rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
		rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
		break;
	case RasterizerState::SHADOW:
		rasterizerDesc.DepthBias = 100000;
		rasterizerDesc.DepthBiasClamp = 0.0f;
		rasterizerDesc.SlopeScaledDepthBias = 1.0f;
		rasterizerDesc.DepthClipEnable = TRUE;
		rasterizerDesc.MultisampleEnable = FALSE;
		rasterizerDesc.AntialiasedLineEnable = FALSE;
		rasterizerDesc.ForcedSampleCount = 0;
		rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		break;
	case RasterizerState::COUNT:
	default:
		break;
	}
	psoDesc.RasterizerState = rasterizerDesc;

	switch (m_BlendState)
	{
	case BlendState::NO_BLEND:
		break;
	case BlendState::ALPHA_BLEND:
		break;
	case BlendState::ADDITIVE_BLEND:
		break;
	case BlendState::COUNT:
		break;
	default:
		break;
	}
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);


	D3D12_DEPTH_STENCIL_DESC depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	switch (m_DepthStencilState)
	{
	case DepthStencilState::DEPTH_ENABLED_LESS_EQUAL:
		depthStencilDesc.DepthEnable = TRUE;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		depthStencilDesc.StencilEnable = FALSE;
		depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		depthStencilDesc.FrontFace.StencilPassOp = depthStencilDesc.FrontFace.StencilFailOp = depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		depthStencilDesc.BackFace = depthStencilDesc.FrontFace;
		break;

	case DepthStencilState::DEPTH_ENABLED:
		depthStencilDesc.DepthEnable = TRUE;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		depthStencilDesc.StencilEnable = FALSE;
		depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		depthStencilDesc.FrontFace.StencilPassOp = depthStencilDesc.FrontFace.StencilFailOp = depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		depthStencilDesc.BackFace = depthStencilDesc.FrontFace;
		break;
	case DepthStencilState::NO_DEPTH_WRITE:
		depthStencilDesc.DepthEnable = TRUE;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		depthStencilDesc.StencilEnable = FALSE;
		break;
	case DepthStencilState::COUNT:
		psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
		break;

	default:
		break;
	}
	psoDesc.DepthStencilState = depthStencilDesc;


	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PipelineState)));
}

void PipelineState::Bind(RendererContext* context)
{
	if (m_PipelineState == nullptr)
	{
		OutputDebugStringA("Pipeline state is not built\n");
		return;
	}
	ID3D12GraphicsCommandList* commandList = static_cast<DX12Context*>(context)->GetCommandList();
	commandList->SetPipelineState(m_PipelineState.get());
}


ComputePipelineState::ComputePipelineState(Renderer* renderer, std::shared_ptr<DX12Shader> shader) :
	m_ComputeShader(shader)
{
	auto context = renderer->GetContext();
	ID3D12Device* device = static_cast<DX12Context*>(context)->GetDevice();

	IDxcBlob* ComputeShaderBlob = m_ComputeShader->m_ShaderBlob[ShaderStage::CS].get();

	if (ComputeShaderBlob == nullptr)
	{
		OutputDebugStringA("Failed to load shader\n");
		return;
	}

	auto rootSignature = m_ComputeShader->GetRootSignature();

	// Describe and create the compute pipeline state object (PSO).
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = rootSignature.get();
	psoDesc.CS = CD3DX12_SHADER_BYTECODE(ComputeShaderBlob->GetBufferPointer(), ComputeShaderBlob->GetBufferSize());

	ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_ComputePipelineState)));
}


void ComputePipelineState::Bind(Renderer* renderer)
{
	ChangDXII* changDXII = static_cast<ChangDXII*>(renderer);
	if (m_ComputePipelineState == nullptr)
	{
		OutputDebugStringA("Pipeline state is not built\n");
		return;
	}
	ID3D12GraphicsCommandList* commandList = changDXII->GetContext()->GetCommandList();

	commandList->SetPipelineState(m_ComputePipelineState.get());
}

RayTracingState::RayTracingState(Renderer* renderer, std::shared_ptr<DX12Shader> shader) :
	m_RayTracingShader(shader)
{
	auto device = static_cast<DX12Context*>(renderer->GetContext())->GetDevice();

	// create Raytracing pipeline state
	{
		// CreateDxilLibrarySubobject
		CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

		auto lib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
		D3D12_SHADER_BYTECODE libDxil = CD3DX12_SHADER_BYTECODE(
			m_RayTracingShader->m_ShaderBlob[ShaderStage::RayTracing]->GetBufferPointer(),
			m_RayTracingShader->m_ShaderBlob[ShaderStage::RayTracing]->GetBufferSize());
		lib->SetDXILLibrary(&libDxil);

		// Hit group
		std::wstring hitGroupExportName = HITGROUP_NAME + std::to_wstring(0);
		m_HitGroupNames.push_back(hitGroupExportName);

		auto hitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
		hitGroup->SetClosestHitShaderImport(m_RayTracingShader->m_DxrFunctions.find(RaytracingShaderType::ClosestHit)->second.c_str());
		hitGroup->SetHitGroupExport(hitGroupExportName.c_str());
		hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

		// Shader config
		auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
		UINT payloadSize = 4 * sizeof(float); // float4 color
		UINT attributeSize = 2 * sizeof(float); // float2 barycentrics
		shaderConfig->Config(payloadSize, attributeSize);

		if (m_RayTracingShader->m_RootSignature[DX12Shader::ROOT_LOCAL] != nullptr)
		{
			auto localRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
			localRootSignature->SetRootSignature(m_RayTracingShader->m_RootSignature[DX12Shader::ROOT_LOCAL].get());

			// Define the root signature association
			auto rootSignatureAssociation = raytracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
			rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
			rootSignatureAssociation->AddExport(hitGroupExportName.c_str());
		}

		// Global root signature
		auto globalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
		globalRootSignature->SetRootSignature(m_RayTracingShader->m_RootSignature[DX12Shader::ROOT_GLOBAL].get());

		// Pipeline config
				// Pipeline config
		// Defines the maximum TraceRay() recursion depth.
		auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
		// PERFOMANCE TIP: Set max recursion depth as low as needed
		// as drivers may apply optimization strategies for low recursion depths.
		UINT maxRecursionDepth = 1;
		pipelineConfig->Config(maxRecursionDepth);

#if _DEBUG
		PrintStateObjectDesc(raytracingPipeline);
#endif

		ThrowIfFailed(device->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(&m_StateObject)));
		ThrowIfFailed(m_StateObject->QueryInterface(IID_PPV_ARGS(&m_StateObjectProperties)));
	}

	CreateShaderBindingTables(renderer);
}

void RayTracingState::Bind(Renderer* renderer)
{
	ChangDXII* changDXII = static_cast<ChangDXII*>(renderer);
	if (m_StateObject == nullptr)
	{
		OutputDebugStringA("Pipeline state is not built\n");
		return;
	}
	ID3D12GraphicsCommandList4* commandList;
	changDXII->GetContext()->GetCommandList()->QueryInterface(IID_PPV_ARGS(&commandList));

	commandList->SetPipelineState1(m_StateObject.get());

}

void RayTracingState::CreateShaderBindingTables(Renderer* renderer)
{
	ShaderParameter parameter{};

	auto device = static_cast<DX12Context*>(renderer->GetContext())->GetDevice();

	auto& dxrFunctions = m_RayTracingShader->m_DxrFunctions;

	std::wstring wview = dxrFunctions.find(RaytracingShaderType::RayGeneration)->second;

	// RayGen shader tables.
	{
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;


		ShaderTable rayGenShaderTable(device, numShaderRecords, shaderRecordSize, L"RayGenShaderTable");
		rayGenShaderTable.push_back(ShaderRecord(m_StateObjectProperties->GetShaderIdentifier(wview.c_str()), shaderRecordSize));
		//rayGenShaderTable.DebugPrint(shaderIdToStringMap);
		m_ShaderBindingTables[static_cast<size_t>(TableType::Raygen)] = rayGenShaderTable.GetResource();

	}

	wview = dxrFunctions.find(RaytracingShaderType::Miss)->second;

	// Miss shader tables.
	{
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

		ShaderTable missShaderTable(device, numShaderRecords, shaderRecordSize, L"MissShaderTable");
		missShaderTable.push_back(ShaderRecord(m_StateObjectProperties->GetShaderIdentifier(wview.c_str()), shaderRecordSize));
		//missShaderTable.DebugPrint(shaderIdToStringMap);
		m_ShaderTableStrideInBytes[static_cast<size_t>(TableType::Miss)] = missShaderTable.GetShaderRecordSize();
		m_ShaderBindingTables[static_cast<size_t>(TableType::Miss)] = missShaderTable.GetResource();
	}


	// 여기까지 히트그룹이 아닌 레이제네이션, 미스에 대한 sbt 생성

	// 밑에부터는 히트그룹에 대한 sbt 생성

	wview = m_HitGroupNames[0];



}
