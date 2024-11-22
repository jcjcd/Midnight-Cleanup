#include "pch.h"
#include "DX12Shader.h"
#include "DX12Buffer.h"
#include "DX12ShaderCompiler.h"
#include "DX12Context.h"
#include "Renderer.h"
#include "Utility.h"
#include "DX12Helper.h"
#include <set>
#include "ChangDXII.h"

#include <fstream>

DX12Shader::DX12Shader(Renderer* renderer, const std::string& filePath) :
	m_Renderer{ renderer }
{
	ID = s_ShaderCount++;

	Name = filePath;

	// 자체 포맷으로 저장되었는지 확인 후 읽기
	std::filesystem::path shaderPath(filePath);

	shaderPath.replace_extension(".mcs12");

	bool check = false;
	
	if (std::filesystem::exists(shaderPath))
	{
		check = loadFromBinary(shaderPath.string());
	}
	else
	{
		check = loadFromHLSL(filePath);
	}

	if (!check)
		return;

	// 레이트레이싱의 쉐이더는 일단 리플렉션 하지 않는다.
	if (!m_ShaderBlob[ShaderStage::RayTracing])
	{
		ReflectShaderVariables();

		CreateRootSignature(renderer);
		CreateConstantBuffer(renderer);
	}

	IsValid = true;
}

bool DX12Shader::loadFromBinary(const std::string& filePath)
{
// 	std::ifstream file(filePath, std::ios::binary);
// 	cereal::BinaryInputArchive archive(file);
// 
// 	archive(*this);
// 
// 	return true;
	return true;
}

bool DX12Shader::loadFromHLSL(const std::string& filePath)
{
	bool isProcessed = false;
	// 창 : 다른 셰이더에 대해서도 더 만들자..
	auto shaderCompiler = DX12ShaderCompiler::GetInstance();
	auto vsResult = shaderCompiler->CompileFromFile(Utility::UTF8ToWideString(filePath.data()), L"vs_6_5", L"VSMain");
	auto psResult = shaderCompiler->CompileFromFile(Utility::UTF8ToWideString(filePath.data()), L"ps_6_5", L"PSMain");
	auto gsResult = shaderCompiler->CompileFromFile(Utility::UTF8ToWideString(filePath.data()), L"gs_6_5", L"GSMain");

	if (vsResult && (psResult || gsResult))
	{
		auto vsReflectionData = shaderCompiler->Reflect(vsResult.get());

		m_ShaderBlob[ShaderStage::VS] = shaderCompiler->GetBlob(vsResult.get());
		m_Reflection[ShaderStage::VS] = vsReflectionData;

		if (psResult)
		{
			auto psReflectionData = shaderCompiler->Reflect(psResult.get());
			m_ShaderBlob[ShaderStage::PS] = shaderCompiler->GetBlob(psResult.get());
			m_Reflection[ShaderStage::PS] = psReflectionData;
		}
		if (gsResult)
		{
			auto gsReflectionData = shaderCompiler->Reflect(gsResult.get());
			m_ShaderBlob[ShaderStage::GS] = shaderCompiler->GetBlob(gsResult.get());
			m_Reflection[ShaderStage::GS] = gsReflectionData;
		}

		isProcessed = true;
	}

	if (!isProcessed)
	{
		auto csResult = shaderCompiler->CompileFromFile(Utility::UTF8ToWideString(filePath.data()), L"cs_6_5", L"CSMain");

		if (csResult)
		{
			auto csReflectionData = shaderCompiler->Reflect(csResult.get());
			m_ShaderBlob[ShaderStage::CS] = shaderCompiler->GetBlob(csResult.get());
			m_Reflection[ShaderStage::CS] = csReflectionData;

			isProcessed = true;
		}
	}

	if (!isProcessed)
	{
		auto raygen = shaderCompiler->CompileFromFile(Utility::UTF8ToWideString(filePath.data()), L"lib_6_6", L"");

		if (raygen)
		{
			auto raygenReflectionData = shaderCompiler->ReflectLibrary(raygen.get());
			m_ShaderBlob[ShaderStage::RayTracing] = shaderCompiler->GetBlob(raygen.get());
			m_LibraryReflection = raygenReflectionData;

			ReflectLibraryVariables();

			if (m_DxrFunctions.size() > 0)
				isProcessed = true;
		}
	}



	if (!isProcessed)
	{
		OutputDebugStringA("Failed to compile shader\n");
		return false;
	}

	return true;
}

void DX12Shader::saveToBinary(const std::string& filePath)
{
// 	std::ofstream file(filePath, std::ios::binary);
// 	cereal::BinaryOutputArchive archive(file);
// 
// 	archive(*this);


}

void DX12Shader::Bind(Renderer* renderer)
{
	ChangDXII* changDXII = static_cast<ChangDXII*>(renderer);



	if (m_ShaderBlob[ShaderStage::CS] || m_ShaderBlob[ShaderStage::RayTracing])
	{
		ID3D12GraphicsCommandList* commandList = changDXII->GetContext()->GetCommandList();
		commandList->SetComputeRootSignature(m_RootSignature[ROOT_GLOBAL].get());
	}
	else
	{
		DX12Context* context = static_cast<DX12Context*>(renderer->GetContext());
		ID3D12GraphicsCommandList* commandList = context->GetCommandList();
		commandList->SetGraphicsRootSignature(m_RootSignature[ROOT_GLOBAL].get());
	}

	AllocateConstantBuffer();
}


void DX12Shader::BindContext(RendererContext* context)
{
	OutputDebugStringA("Deprecated\n");
	__debugbreak();
}

void DX12Shader::ResetAllocateStates()
{
	for (auto& buffer : m_ConstantBuffers)
		buffer->Reset();
}

void DX12Shader::SetConstant(const std::string& name, const void* value, uint32_t size)
{
	AllocateConstantBuffer();

	auto it = m_ConstantBufferMap.find(name.data());
	if (it != m_ConstantBufferMap.end())
	{
		auto cb = it->second;
		memcpy(cb->GetAllocatedData(), value, size);
	}
}


void DX12Shader::SetConstantMapping(const std::string& name, const void* value)
{
	AllocateConstantBuffer();
	auto it = m_ConstantBufferMappings.find(name.data());
	if (it != m_ConstantBufferMappings.end())
	{
		auto& mapping = it->second;
		auto data = mapping.OriginalBuffer->GetAllocatedData() + mapping.Offset;
		memcpy(data, value, mapping.Size);
		mapping.OriginalBuffer->m_IsDirty = true;
	}
}

void DX12Shader::ReflectShaderVariables()
{
	if (m_Reflection[ShaderStage::VS])
	{
		D3D12_SHADER_DESC shaderDesc;
		m_Reflection[ShaderStage::VS]->GetDesc(&shaderDesc);

		for (UINT i = 0; i < shaderDesc.InputParameters; i++)
		{
			D3D12_SIGNATURE_PARAMETER_DESC inputParameterDesc;
			m_Reflection[ShaderStage::VS]->GetInputParameterDesc(i, &inputParameterDesc);

			D3D12_INPUT_ELEMENT_DESC inputElementDesc = {};
			inputElementDesc.SemanticName = inputParameterDesc.SemanticName;
			inputElementDesc.SemanticIndex = inputParameterDesc.SemanticIndex;
			inputElementDesc.InputSlot = inputParameterDesc.Stream;
			inputElementDesc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
			inputElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;

			if (inputParameterDesc.Mask == 1)
			{
				if (inputParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) inputElementDesc.Format = DXGI_FORMAT_R32_UINT;
				else if (inputParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) inputElementDesc.Format = DXGI_FORMAT_R32_SINT;
				else if (inputParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) inputElementDesc.Format = DXGI_FORMAT_R32_FLOAT;
			}
			else if (inputParameterDesc.Mask <= 3)
			{
				if (inputParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) inputElementDesc.Format = DXGI_FORMAT_R32G32_UINT;
				else if (inputParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) inputElementDesc.Format = DXGI_FORMAT_R32G32_SINT;
				else if (inputParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) inputElementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
			}
			else if (inputParameterDesc.Mask <= 7)
			{
				if (inputParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) inputElementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
				else if (inputParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) inputElementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
				else if (inputParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) inputElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
			}
			else if (inputParameterDesc.Mask <= 15)
			{
				if (inputParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) inputElementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
				else if (inputParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) inputElementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
				else if (inputParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) inputElementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			}

			m_InputLayout.push_back(inputElementDesc);
		}
	}

	if (m_Reflection[ShaderStage::PS])
	{
		D3D12_SHADER_DESC shaderDesc;
		m_Reflection[ShaderStage::PS]->GetDesc(&shaderDesc);

		for (uint32_t i = 0; i < shaderDesc.OutputParameters; i++)
		{
			D3D12_SIGNATURE_PARAMETER_DESC outputParameterDesc;
			m_Reflection[ShaderStage::PS]->GetOutputParameterDesc(i, &outputParameterDesc);

			switch (outputParameterDesc.ComponentType)
			{
			case D3D_REGISTER_COMPONENT_UINT32:
				if (outputParameterDesc.Mask == 1)
					m_RTVFormats.push_back(Texture::Format::R32_UINT);
				else if (outputParameterDesc.Mask <= 3)
					m_RTVFormats.push_back(Texture::Format::R16G16_UINT);
				else if (outputParameterDesc.Mask <= 7)
					m_RTVFormats.push_back(Texture::Format::R8G8B8A8_UINT);
				else if (outputParameterDesc.Mask <= 15)
					m_RTVFormats.push_back(Texture::Format::R8G8B8A8_UINT);
				break;

			case D3D_REGISTER_COMPONENT_SINT32:
				if (outputParameterDesc.Mask == 1)
					m_RTVFormats.push_back(Texture::Format::R32_SINT);
				else if (outputParameterDesc.Mask <= 3)
					m_RTVFormats.push_back(Texture::Format::R16G16_SINT);
				else if (outputParameterDesc.Mask <= 7)
					m_RTVFormats.push_back(Texture::Format::R8G8B8A8_SINT);
				else if (outputParameterDesc.Mask <= 15)
					m_RTVFormats.push_back(Texture::Format::R8G8B8A8_SINT);
				break;

			case D3D_REGISTER_COMPONENT_FLOAT32:
				if (outputParameterDesc.Mask == 1)
					m_RTVFormats.push_back(Texture::Format::R32_FLOAT);
				else if (outputParameterDesc.Mask <= 3)
					m_RTVFormats.push_back(Texture::Format::R32G32_FLOAT);
				else if (outputParameterDesc.Mask <= 7)
					m_RTVFormats.push_back(Texture::Format::R32G32B32_FLOAT);
				else if (outputParameterDesc.Mask <= 15)
				{
					if (outputParameterDesc.Register == 0)
						m_RTVFormats.push_back(Texture::Format::R8G8B8A8_UNORM);
					else
						m_RTVFormats.push_back(Texture::Format::R32G32B32A32_FLOAT);
				}

				break;
			}
		}
	}


	// constant buffer reflection
	//std::set<std::string> alreadyInserted;

	for (ShaderStage stage = ShaderStage::VS; stage < ShaderStage::COUNT; stage = (ShaderStage)((int)stage + 1))
	{
		uint32_t bufSlot = 0;
		if (m_Reflection[stage])
		{
			// constant buffer reflection
			auto reflection = m_Reflection[stage].get();

			D3D12_SHADER_DESC shaderDesc;
			reflection->GetDesc(&shaderDesc);

			ReflectResourceBindings(m_Reflection[stage].get(), stage);

			for (uint32_t i = 0; i < shaderDesc.ConstantBuffers; ++i)
			{
				ConstantBufferLayout cbLayout;
				ID3D12ShaderReflectionConstantBuffer* constantBuffer = reflection->GetConstantBufferByIndex(i);
				constantBuffer->GetDesc(&cbLayout.Desc);

				cbLayout.Size = 0;

				if (cbLayout.Desc.Type == D3D_CT_RESOURCE_BIND_INFO)
					continue;

				for (uint32_t j = 0; j < cbLayout.Desc.Variables; ++j)
				{
					// 변수와 타입 정보를 가져온다.
					ID3D12ShaderReflectionVariable* var = constantBuffer->GetVariableByIndex(j);
					D3D12_SHADER_VARIABLE_DESC varDesc;
					var->GetDesc(&varDesc);
					cbLayout.Variables.push_back(varDesc);

					ID3D12ShaderReflectionType* varType = var->GetType();
					D3D12_SHADER_TYPE_DESC varTypeDesc;
					varType->GetDesc(&varTypeDesc);
					cbLayout.Types.push_back(varTypeDesc);

					// 사이즈 누적
					cbLayout.Size += varDesc.Size;
				}
				cbLayout.Name = cbLayout.Desc.Name;
				cbLayout.Stage = stage;

				BufferBinding bufferBinding;

				bool isFound = false;

				for (auto& bindings : m_BufferBindings)
				{
					for (auto& binding : bindings)
					{
						if (binding.Name == cbLayout.Name)
						{
							bufferBinding = binding;
							isFound = true;
							break;
						}
					}
					if (isFound)
						break;
				}
				assert(isFound);

				cbLayout.Register = bufferBinding.Register;
				//cbLayout.Register = bufSlot;
				cbLayout.SizeAligned256 = Math::AlignUp(cbLayout.Size, 256);
				++bufSlot;
				m_ConstantBufferLayouts[ROOT_GLOBAL].push_back(cbLayout);
				/*				alreadyInserted.insert(cbLayout.Name);*/
			}

			// resource binding reflection
		}
	}
}

const robin_hood::unordered_map<std::string, const CD3DX12_STATIC_SAMPLER_DESC> DX12Shader::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		0, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		0, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC shadow(
		0, // shaderRegister
		D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,                              // mipLODBias
		1,								 // maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
		0.0f,
		D3D12_FLOAT32_MAX
	);

	static const robin_hood::unordered_map<std::string, const CD3DX12_STATIC_SAMPLER_DESC> samplers
	{
		{ "gsamPointWrap", pointWrap },
		{ "gsamPointClamp", pointClamp },
		{ "gsamLinearWrap", linearWrap },
		{ "gsamLinearClamp", linearClamp },
		{ "gsamAnisotropicWrap", anisotropicWrap },
		{ "gsamAnisotropicClamp", anisotropicClamp },
		{ "gsamShadow", shadow}
	};

	return samplers;
}

void DX12Shader::CreateRootSignature(Renderer* renderer)
{
	CreateRootSignatureHelper(renderer, ROOT_GLOBAL, L"GlobalRootSignature");

	if (m_AccelationStructureBindings[ROOT_LOCAL].size() ||
		m_TextureBindings[ROOT_LOCAL].size() ||
		m_UAVBindings[ROOT_LOCAL].size() ||
		m_BufferBindings[ROOT_LOCAL].size() ||
		m_ConstantBufferLayouts[ROOT_LOCAL].size() ||
		m_SamplerBindings[ROOT_LOCAL].size())
		CreateRootSignatureHelper(renderer, ROOT_LOCAL, L"LocalRootSignature");

}

void DX12Shader::CreateRootSignatureHelper(Renderer* renderer, RootType type, const std::wstring& name)
{
	auto device = static_cast<DX12Context*>(renderer->GetContext())->GetDevice();

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	std::vector<CD3DX12_DESCRIPTOR_RANGE1> ranges;
	ranges.resize(m_TextureBindings[type].size());
	for (uint32_t i = 0; i < m_TextureBindings[type].size(); i++)
		ranges[i].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, m_TextureBindings[type][i].Register, type);


	std::vector<CD3DX12_DESCRIPTOR_RANGE1> uavRanges;
	uavRanges.resize(m_UAVBindings[type].size());
	for (uint32_t i = 0; i < m_UAVBindings[type].size(); i++)
		uavRanges[i].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, m_UAVBindings[type][i].Register, type);

	std::vector<CD3DX12_ROOT_PARAMETER1> rootParameters;
	uint32_t size =
		static_cast<uint32_t>(m_BufferBindings[type].size() +
		m_TextureBindings[type].size() +
		m_UAVBindings[type].size() +
		m_AccelationStructureBindings[type].size());

	rootParameters.resize(size);

	uint32_t offset = 0;

	for (uint32_t i = 0; i < m_AccelationStructureBindings[type].size(); ++i)
	{
		m_AccelationStructureBindings[type][i].ParameterIndex = i + offset;
		m_AccelationStructureIndexMap[type].emplace(m_AccelationStructureBindings[type][i].Name, m_AccelationStructureBindings[type][i].ParameterIndex);
		rootParameters[i].InitAsShaderResourceView(m_AccelationStructureBindings[type][i].Register,
			type, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	}

	offset += static_cast<uint32_t>(m_AccelationStructureBindings[type].size());

	for (uint32_t i = 0; i < m_TextureBindings[type].size(); i++)
	{
		m_TextureBindings[type][i].ParameterIndex = i + offset;
		m_TextureIndexMap[type].emplace(m_TextureBindings[type][i].Name, m_TextureBindings[type][i].ParameterIndex);
		rootParameters[i + offset].InitAsDescriptorTable(1, &ranges[i], D3D12_SHADER_VISIBILITY_ALL);
	}

	offset += static_cast<uint32_t>(m_TextureBindings[type].size());

	for (uint32_t i = 0; i < m_UAVBindings[type].size(); i++)
	{
		m_UAVBindings[type][i].ParameterIndex = i + offset;
		m_UAVIndexMap[type].emplace(m_UAVBindings[type][i].Name, m_UAVBindings[type][i].ParameterIndex);
		rootParameters[i + offset].InitAsDescriptorTable(1, &uavRanges[i], D3D12_SHADER_VISIBILITY_ALL);
	}

	offset += static_cast<uint32_t>(m_UAVBindings[type].size());

	// 이부분떄매 반드시 v1를 써야된다... 셰이더 리플렉션때매 어쩔수가 없댜..
	for (uint32_t i = 0; i < m_ConstantBufferLayouts[type].size(); i++)
	{
		m_ConstantBufferLayouts[type][i].ParameterIndex = i + offset;

		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL;
		switch (m_ConstantBufferLayouts[type][i].Stage)
		{
		case ShaderStage::VS: visibility = D3D12_SHADER_VISIBILITY_VERTEX; break;
		case ShaderStage::PS: visibility = D3D12_SHADER_VISIBILITY_PIXEL; break;
		case ShaderStage::GS: visibility = D3D12_SHADER_VISIBILITY_GEOMETRY; break;
		case ShaderStage::CS: visibility = D3D12_SHADER_VISIBILITY_ALL; break;
		case ShaderStage::HS: visibility = D3D12_SHADER_VISIBILITY_HULL; break;
		}
		rootParameters[i + offset].InitAsConstantBufferView(m_ConstantBufferLayouts[type][i].Register, type, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, visibility);
	}

	std::vector<CD3DX12_STATIC_SAMPLER_DESC> samplers;
	for (auto& samplerbinding : m_SamplerBindings[type])
	{
		auto staticSamplers = GetStaticSamplers();
		auto sampler = staticSamplers.find(samplerbinding.first);
		CD3DX12_STATIC_SAMPLER_DESC samplerDesc{};
		if (sampler != staticSamplers.end())
		{
			samplerDesc = sampler->second;
		}
		else
		{
			OutputDebugStringA("Sampler not found\n");
			samplerDesc = staticSamplers.begin()->second;
		}
		samplerDesc.ShaderRegister = samplerbinding.second.Register;
		samplers.push_back(samplerDesc);
	}

	D3D12_ROOT_SIGNATURE_DESC1 rootSignatureDesc = {};
	rootSignatureDesc.NumParameters = size;
	rootSignatureDesc.pParameters = rootParameters.data();
	rootSignatureDesc.NumStaticSamplers = (UINT)m_SamplerBindings[type].size();
	rootSignatureDesc.pStaticSamplers = samplers.data();

	rootSignatureDesc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		| D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
		//| D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;        // Support for BindlessResource

	if (type == ROOT_LOCAL)
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedDesc = { };
	versionedDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	versionedDesc.Desc_1_1 = rootSignatureDesc;

	winrt::com_ptr<ID3DBlob> signature;
	winrt::com_ptr<ID3DBlob> error;

	if (FAILED(D3D12SerializeVersionedRootSignature(&versionedDesc, signature.put(), error.put())))
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		__debugbreak();
	}

	ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(m_RootSignature[type].put())));

	m_RootSignature[type]->SetName(name.c_str());
}

void DX12Shader::DetermineRaytracingShaderType(const std::string& name, RaytracingShaderType& type)
{
	std::string lower = name.data();
	std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

	if (lower.find("raygen") != std::string::npos)
		type = RaytracingShaderType::RayGeneration;
	else if (lower.find("miss") != std::string::npos)
		type = RaytracingShaderType::Miss;
	else if (lower.find("closesthit") != std::string::npos)
		type = RaytracingShaderType::ClosestHit;
	else if (lower.find("anyhit") != std::string::npos)
		type = RaytracingShaderType::AnyHit;
	else if (lower.find("intersection") != std::string::npos)
		type = RaytracingShaderType::Intersection;
}

void DX12Shader::CreateConstantBuffer(Renderer* renderer)
{

	for (auto& bufferBinding : m_BufferBindings[ROOT_GLOBAL])
	{
		bool isFound = false;
		for (auto& layout : m_ConstantBufferLayouts[ROOT_GLOBAL])
		{
			if (bufferBinding.Name == layout.Name)
			{
				isFound = true;
				auto buffer = DX12ConstantBuffer::Create(renderer, layout.SizeAligned256);
				m_ConstantBuffers.push_back(buffer);
				auto cb = m_ConstantBuffers.back().get();
				m_ConstantBufferMap.emplace(layout.Desc.Name, cb);

				cb->m_Register = layout.Register;
				cb->m_ParameterIndex = layout.ParameterIndex;

				for (auto& var : layout.Variables)
				{
					ConstantBufferMapping mapping;
					mapping.Name = var.Name;
					mapping.Offset = var.StartOffset;
					mapping.Size = var.Size;
					mapping.Register = layout.Register;
					mapping.Space = bufferBinding.Space;
					mapping.OriginalBuffer = cb;
					m_ConstantBufferMappings.emplace(var.Name, mapping);
				}
			}
		}
		if (!isFound)
		{
			OutputDebugStringA("Buffer not found\n");
			__debugbreak();
		}
	}

}

void DX12Shader::ReflectResourceBindings(ID3D12ShaderReflection* reflection, ShaderStage stage)
{
	D3D12_SHADER_DESC shaderDesc;
	reflection->GetDesc(&shaderDesc);

	for (uint32_t i = 0; i < shaderDesc.BoundResources; ++i)
	{
		D3D12_SHADER_INPUT_BIND_DESC bindDesc;
		reflection->GetResourceBindingDesc(i, &bindDesc);

		switch (bindDesc.Type)
		{
		case D3D_SIT_TEXTURE:
		case D3D_SIT_STRUCTURED:
		{
			TextureBinding texBinding;
			texBinding.Name = bindDesc.Name;
			texBinding.Register = bindDesc.BindPoint;
			texBinding.Space = bindDesc.Space;
			texBinding.Stage = stage;
			texBinding.TextureDimension = (Texture::Type)bindDesc.Dimension;
			VerifyAndPush(texBinding, m_TextureBindings[bindDesc.Space]);
		}
		break;
		case D3D_SIT_SAMPLER:
		{
			SamplerBinding samplerBinding;
			samplerBinding.Name = bindDesc.Name;
			samplerBinding.Register = bindDesc.BindPoint;
			samplerBinding.Space = bindDesc.Space;
			samplerBinding.Stage = stage;
			m_SamplerBindings[bindDesc.Space].emplace(bindDesc.Name, samplerBinding);
		}
		break;
		case D3D_SIT_BYTEADDRESS:
		case D3D_SIT_UAV_RWTYPED:
		case D3D_SIT_UAV_RWSTRUCTURED:
		case D3D_SIT_UAV_RWBYTEADDRESS:
		case D3D_SIT_UAV_APPEND_STRUCTURED:
		case D3D_SIT_UAV_CONSUME_STRUCTURED:
		case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
		{
			UAVBinding uavBinding;
			uavBinding.Name = bindDesc.Name;
			uavBinding.Register = bindDesc.BindPoint;
			uavBinding.Space = bindDesc.Space;
			uavBinding.Stage = stage;
			uavBinding.TextureDimension = (Texture::Type)bindDesc.Dimension;
			VerifyAndPush(uavBinding, m_UAVBindings[bindDesc.Space]);
		}
		break;
		case D3D_SIT_CBUFFER:
		{
			BufferBinding bufferBinding;
			bufferBinding.Name = bindDesc.Name;
			bufferBinding.Register = bindDesc.BindPoint;
			bufferBinding.Space = bindDesc.Space;
			bufferBinding.Stage = stage;
			VerifyAndPush(bufferBinding, m_BufferBindings[bindDesc.Space]);
		}
		break;

		default:
			OutputDebugStringA("Unknown resource type\n");
		}
	}
}

void DX12Shader::AllocateConstantBuffer()
{
	if (!m_IsAllocated)
	{
		for (auto& cb : m_ConstantBuffers)
		{
			cb->Allocate();
		}
		m_IsAllocated = true;
	}

}

void DX12Shader::ReflectLibraryVariables()
{
	if (m_LibraryReflection)
	{
		D3D12_LIBRARY_DESC libraryDesc;
		m_LibraryReflection->GetDesc(&libraryDesc);

		uint32_t bufSlot = 0;

		for (UINT i = 0; i < libraryDesc.FunctionCount; i++)
		{
			ID3D12FunctionReflection* function = m_LibraryReflection->GetFunctionByIndex(i);
			D3D12_FUNCTION_DESC functionDesc;
			function->GetDesc(&functionDesc);

			std::string name = functionDesc.Name;
			size_t offset = name.find_first_of("?") + 1;
			name = name.substr(offset, name.find_first_of("@") - offset);
			RaytracingShaderType type;
			DetermineRaytracingShaderType(name, type);
			std::wstring wname(name.begin(), name.end());
			m_DxrFunctions.emplace(type, wname);
			// 여기서부터 리소스 바인딩을 만들어보자
			for (UINT j = 0; j < functionDesc.BoundResources; ++j)
			{
				D3D12_SHADER_INPUT_BIND_DESC bindDesc;
				function->GetResourceBindingDesc(j, &bindDesc);

				switch (bindDesc.Type)
				{
				case D3D_SIT_BYTEADDRESS:
				case D3D_SIT_STRUCTURED:
				case D3D_SIT_TEXTURE:
				{
					TextureBinding texBinding;
					texBinding.Name = bindDesc.Name;
					texBinding.Register = bindDesc.BindPoint;
					texBinding.Space = bindDesc.Space;
					texBinding.Stage = RayTracing;
					texBinding.TextureDimension = (Texture::Type)bindDesc.Dimension;
					VerifyAndPush(texBinding, m_TextureBindings[bindDesc.Space]);
				}
				break;
				case D3D_SIT_SAMPLER:
				{
					SamplerBinding samplerBinding;
					samplerBinding.Name = bindDesc.Name;
					samplerBinding.Register = bindDesc.BindPoint;
					samplerBinding.Space = bindDesc.Space;
					samplerBinding.Stage = RayTracing;
					m_SamplerBindings[bindDesc.Space].emplace(bindDesc.Name, samplerBinding);
				}
				break;
				case D3D_SIT_UAV_RWTYPED:
				case D3D_SIT_UAV_RWSTRUCTURED:
				case D3D_SIT_UAV_RWBYTEADDRESS:
				case D3D_SIT_UAV_APPEND_STRUCTURED:
				case D3D_SIT_UAV_CONSUME_STRUCTURED:
				case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
				{
					UAVBinding uavBinding;
					uavBinding.Name = bindDesc.Name;
					uavBinding.Register = bindDesc.BindPoint;
					uavBinding.Space = bindDesc.Space;
					uavBinding.Stage = RayTracing;
					uavBinding.TextureDimension = (Texture::Type)bindDesc.Dimension;
					VerifyAndPush(uavBinding, m_UAVBindings[bindDesc.Space]);
				}
				break;
				case D3D_SIT_RTACCELERATIONSTRUCTURE:
				{
					AccelationStructureBinding asBinding;
					asBinding.Name = bindDesc.Name;
					asBinding.Register = bindDesc.BindPoint;
					asBinding.Space = bindDesc.Space;
					asBinding.Stage = RayTracing;
					asBinding.TextureDimension = (Texture::Type)bindDesc.Dimension;
					VerifyAndPush(asBinding, m_AccelationStructureBindings[bindDesc.Space]);
				}
				break;
				case D3D_SIT_UAV_FEEDBACKTEXTURE:
				case D3D_SIT_CBUFFER:
				{
					BufferBinding bufferBinding;
					bufferBinding.Name = bindDesc.Name;
					bufferBinding.Register = bindDesc.BindPoint;
					bufferBinding.Space = bindDesc.Space;
					bufferBinding.Stage = RayTracing;
					VerifyAndPush(bufferBinding, m_BufferBindings[bindDesc.Space]);
				}

				default:
					OutputDebugStringA("Unknown resource type\n");
				}
			}

			for (UINT j = 0; j < functionDesc.ConstantBuffers; j++)
			{
				ConstantBufferLayout cbLayout;
				ID3D12ShaderReflectionConstantBuffer* constantBuffer = function->GetConstantBufferByIndex(j);
				D3D12_SHADER_BUFFER_DESC bufferDesc;
				constantBuffer->GetDesc(&bufferDesc);
				cbLayout.Size = 0;
				cbLayout.Desc = bufferDesc;

				if (bufferDesc.Type == D3D_CT_RESOURCE_BIND_INFO)
					continue;

				for (UINT k = 0; k < bufferDesc.Variables; k++)
				{
					ID3D12ShaderReflectionVariable* variable = constantBuffer->GetVariableByIndex(k);
					D3D12_SHADER_VARIABLE_DESC variableDesc;
					variable->GetDesc(&variableDesc);
					cbLayout.Variables.push_back(variableDesc);

					ID3D12ShaderReflectionType* variableType = variable->GetType();
					D3D12_SHADER_TYPE_DESC typeDesc;
					variableType->GetDesc(&typeDesc);
					cbLayout.Types.push_back(typeDesc);

					cbLayout.Size += variableDesc.Size;
				}
				cbLayout.Name = bufferDesc.Name;
				BufferBinding bufferBinding;

				bool isFound = false;

				for (auto& bindings : m_BufferBindings)
				{
					for (auto& binding : bindings)
					{
						if (binding.Name == cbLayout.Name)
						{
							bufferBinding = binding;
							isFound = true;
							break;
						}
					}
					if (isFound)
						break;
				}
				assert(isFound);


				cbLayout.Stage = RayTracing;
				cbLayout.Register = bufferBinding.Register;
				cbLayout.SizeAligned256 = Math::AlignUp(cbLayout.Size, 256);

				isFound = false;

				for (auto& layouts : m_ConstantBufferLayouts[bufferBinding.Space])
				{
					if (layouts.Name == cbLayout.Name)
					{
						isFound = true;
						break;
					}
				}
				if (!isFound)
				{
					m_ConstantBufferLayouts[bufferBinding.Space].push_back(cbLayout);
				}


			}
		}

	}

}

