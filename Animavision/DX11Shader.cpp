#include "pch.h"
#include "DX11Shader.h"
#include "Renderer.h"
#include "DX11Buffer.h"
#include "Mesh.h"
#include "NeoDX11Context.h"
#include "ShaderResource.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>

#define SHADER_DEBUG_EXTENSION ".mcs11d"
#define SHADER_EXTENSION ".mcs11"

DX11Shader::DX11Shader(RendererContext* context, std::string_view path)
	: m_Path(path)
{
	ID = s_ShaderCount++;

	createVertexShader(context, path);
	createPixelShader(context, path);
	createComputeShader(context, path);
	createGeometryShader(context, path);
	createStreamOutGeometryShader(context, path);
	createHullShader(context, path);
	createDomainShader(context, path);

	reflectShaderVariables();
	createConstantBuffer(context);

	IsValid = true;
}

DX11Shader::~DX11Shader()
{
	Destroy();
}

void DX11Shader::Bind(Renderer* renderer)
{
	//assert(dynamic_cast<NeoDX11Context*>(renderer->GetContext()) != nullptr && "Wrong Context");
	NeoDX11Context* context = static_cast<NeoDX11Context*>(renderer->GetContext());

	if (GetVertexShader())
	{
		context->GetDeviceContext()->VSSetShader(GetVertexShader(), nullptr, 0);
		context->GetDeviceContext()->IASetInputLayout(m_InputLayout.get());
	}

	if (GetPixelShader())
		context->GetDeviceContext()->PSSetShader(GetPixelShader(), nullptr, 0);

	if (GetComputeShader())
		context->GetDeviceContext()->CSSetShader(GetComputeShader(), nullptr, 0);

	if (GetGeometryShader())
		context->GetDeviceContext()->GSSetShader(GetGeometryShader(), nullptr, 0);

	//context->GetDeviceContext()->GSSetShader(GetStreamOutGeometryShader(), nullptr, 0);
	//context->GetDeviceContext()->HSSetShader(GetHullShader(), nullptr, 0);
	//context->GetDeviceContext()->DSSetShader(GetDomainShader(), nullptr, 0);

	for (auto& buffer : m_ConstantBufferMap)
	{
		for (auto& buf : buffer.second)
			buf->BindConstantBuffer(context, static_cast<uint32_t>(buf->m_ShaderType), buf->m_Register);
	}
}

void DX11Shader::Unbind(Renderer* renderer)
{
	//assert(dynamic_cast<NeoDX11Context*>(renderer->GetContext()) != nullptr && "Wrong Context");
	NeoDX11Context* context = static_cast<NeoDX11Context*>(renderer->GetContext());

	for (uint32_t i = static_cast<uint32_t>(ShaderType::Vertex); i < static_cast<uint32_t>(ShaderType::Count); i++)
	{
		if (!m_Shaders[i])
			continue;

		switch (i)
		{
		case static_cast<uint32_t>(ShaderType::Vertex):
			context->GetDeviceContext()->VSSetShader(nullptr, nullptr, 0);
			break;
		case static_cast<uint32_t>(ShaderType::Pixel):
			context->GetDeviceContext()->PSSetShader(nullptr, nullptr, 0);
			break;
		case static_cast<uint32_t>(ShaderType::Compute):
			context->GetDeviceContext()->CSSetShader(nullptr, nullptr, 0);
			break;
		case static_cast<uint32_t>(ShaderType::Geometry):
		case static_cast<uint32_t>(ShaderType::StreamOutGeometry):
			context->GetDeviceContext()->GSSetShader(nullptr, nullptr, 0);
			break;
		case static_cast<uint32_t>(ShaderType::Hull):
			context->GetDeviceContext()->HSSetShader(nullptr, nullptr, 0);
			break;
		case static_cast<uint32_t>(ShaderType::Domain):
			context->GetDeviceContext()->DSSetShader(nullptr, nullptr, 0);
			break;

		default:
			break;
		}
	}

	for (auto& buffers : m_ConstantBufferMap)
	{
		for (auto& buffer : buffers.second)
			buffer->UnbindConstantBuffer(context, static_cast<uint32_t>(buffer->m_ShaderType), buffer->m_Register);
	}
}

void DX11Shader::SetInt(const std::string& name, int value)
{
	setConstantMapping(name, &value);
}

void DX11Shader::SetIntArray(const std::string& name, int* value)
{
	setConstantMapping(name, &value);
}

void DX11Shader::SetFloat(const std::string& name, float value)
{
	setConstantMapping(name, &value);
}

void DX11Shader::SetFloat2(const std::string& name, const Vector2& value)
{
	setConstantMapping(name, &value);
}

void DX11Shader::SetFloat3(const std::string& name, const Vector3& value)
{
	setConstantMapping(name, &value);
}

void DX11Shader::SetFloat4(const std::string& name, const Vector4& value)
{
	setConstantMapping(name, &value);
}

void DX11Shader::SetMatrix(const std::string& name, const Matrix& value)
{
	setConstantMapping(name, &value);
}

void DX11Shader::SetStruct(const std::string& name, const void* value)
{
	// 포인터 주소를 넘겨줘야함
	setConstantMapping(name, value);
}

void DX11Shader::SetConstant(const std::string& name, const void* value, uint32_t size)
{
	//// find by variable name
	//robin_hood::unordered_map<std::string, std::vector<CBMapping>>::iterator iter = m_CBMapping.find(name.data());
	//if (iter != m_CBMapping.end())
	//{
	//	for (auto& mapping : iter->second)
	//	{
	//		auto temp = *mapping.Data;
	//		temp += mapping.Offset;
	//		memcpy(temp, value, size);
	//		m_ConstantBuffers[mapping.Index]->m_IsDirty = true;
	//		return;
	//	}
	//}

	// Find by buffer name
	if (auto bufIter = m_ConstantBufferMap.find(name); bufIter != m_ConstantBufferMap.end())
	{
		for (auto& buffer : bufIter->second)
		{
			auto temp = buffer->m_MappedResource.pData;
			memcpy(temp, value, size);
			buffer->m_IsDirty = true;
		}
	}
}

void DX11Shader::UnmapConstantBuffer(RendererContext* context)
{
	//assert(dynamic_cast<NeoDX11Context*>(context) != nullptr && "Wrong Context");
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	for (auto& buffer : m_ConstantBuffers)
	{
		if (buffer->m_IsDirty)
		{
			dx11Context->GetDeviceContext()->Unmap(buffer->m_Buffer.Get(), 0);
			buffer->m_IsDirty = false;
			buffer->m_IsMapped = false;
		}
	}
}

void DX11Shader::UnmapConstantBuffer(RendererContext* context, std::string bufName)
{
	//assert(dynamic_cast<NeoDX11Context*>(context) != nullptr && "Wrong Context");
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	auto findIt = m_ConstantBufferMap.find(bufName);
	if (findIt == m_ConstantBufferMap.end())
	{
		OutputDebugStringA("Buffer not found\n");
		return;
	}

	for (auto& buffers = findIt->second; auto & buffer : buffers)
	{
		if (buffer->m_IsDirty)
		{
			dx11Context->GetDeviceContext()->Unmap(buffer->m_Buffer.Get(), 0);
			buffer->m_IsDirty = false;
			buffer->m_IsMapped = false;
		}
	}
}

void DX11Shader::Recompile(RendererContext* context)
{
	this->Destroy();

	createVertexShader(context, m_Path);
	createPixelShader(context, m_Path);
	createComputeShader(context, m_Path);
	createGeometryShader(context, m_Path);
	createStreamOutGeometryShader(context, m_Path);
	createHullShader(context, m_Path);
	createDomainShader(context, m_Path);

	reflectShaderVariables();
	createConstantBuffer(context);

}

Buffer* DX11Shader::GetConstantBuffer(const std::string& name, int index)
{
	if (m_ConstantBufferMap.contains(name))
		return m_ConstantBufferMap[name][index];
	else
		return nullptr;
}

void DX11Shader::MapConstantBuffer(RendererContext* context)
{
	//assert(dynamic_cast<NeoDX11Context*>(context) != nullptr && "Wrong Context");
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	for (auto& buffer : m_ConstantBuffers)
	{
		if (!buffer->m_IsMapped)
		{
			HRESULT hr = dx11Context->GetDeviceContext()->Map(buffer->m_Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &buffer->m_MappedResource);
			assert(SUCCEEDED(hr));
			buffer->m_IsMapped = true;
		}
	}
}

void DX11Shader::MapConstantBuffer(RendererContext* context, std::string bufName)
{
	//assert(dynamic_cast<NeoDX11Context*>(context) != nullptr && "Wrong Context");
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	auto findIt = m_ConstantBufferMap.find(bufName);
	if (findIt == m_ConstantBufferMap.end())
	{
		OutputDebugStringA("Buffer not found\n");
		return;
	}

	for (auto& buffer : findIt->second)
	{
		if (!buffer->m_IsMapped)
		{
			HRESULT hr = dx11Context->GetDeviceContext()->Map(buffer->m_Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &buffer->m_MappedResource);
			assert(SUCCEEDED(hr));
			buffer->m_IsMapped = true;
		}
	}
}

ID3DBlob* DX11Shader::compileShaderFromFile(std::string_view path, std::string_view name, std::string_view version)
{
	ID3DBlob* blob = nullptr;

	std::filesystem::path filePath = std::filesystem::current_path();
	filePath = filePath / path;

	m_Path = filePath.string();

	Name = path;

	bool isExist = false;

	std::filesystem::path savedPath = filePath.replace_extension(".hlsl");
	std::string savedPathStr = savedPath.string();
	std::string curName = std::string(name.begin(), name.end());
#ifdef _DEBUG
	savedPathStr = savedPathStr + curName + SHADER_DEBUG_EXTENSION;
	isExist = loadShaderBlobFromFile(&blob, savedPathStr);
#else
	savedPathStr = savedPathStr + curName + SHADER_EXTENSION;
	isExist = loadShaderBlobFromFile(&blob, savedPathStr);
#endif

	if (!isExist)
	{
		ID3DBlob* errorBlob = nullptr;
		ShaderIncludeHandler includeHandler;

		std::ifstream shaderFile(m_Path);
		if (!shaderFile.is_open())
			return nullptr;
		else
		{
			std::stringstream strStream;
			strStream << shaderFile.rdbuf();

			const std::string shaderCode = strStream.str();

			uint32_t debugFlag = 0;

			/// 이거 없으면 그래픽 디버깅 시에 코드를 볼 수 없음
#ifdef _DEBUG
			debugFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
#else
			debugFlag = D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_SKIP_VALIDATION | D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
#endif

			// shaderCode를 blob으로 변환
			HRESULT hr = D3DCompile(shaderCode.c_str(), shaderCode.size(), Name.c_str(), nullptr, &includeHandler, name.data(), version.data(), debugFlag, 0, &blob, &errorBlob);
			if (FAILED(hr))
			{
				if (errorBlob)
				{
					OutputDebugStringA((char*)errorBlob->GetBufferPointer());
					errorBlob->Release();
					return nullptr;
				}
			}

			saveShaderBlobToFile(blob, savedPathStr);

			return blob;
		}
	}

	return blob;
}

bool DX11Shader::saveShaderBlobToFile(ID3DBlob* blob, std::string_view path)
{
	// 부모 디렉토리의 존재 확인
	std::filesystem::path curPath = path;
	if (!std::filesystem::exists(curPath))
		std::filesystem::create_directories(curPath.parent_path());

	std::ofstream file(curPath, std::ios::binary);
	if (!file.is_open())
		return false;

	file.write(static_cast<const char*>(blob->GetBufferPointer()), blob->GetBufferSize());
	file.close();

	return true;
}

bool DX11Shader::loadShaderBlobFromFile(ID3DBlob** blob, std::string_view path)
{
	std::filesystem::path curPath = path;
	std::ifstream file(curPath, std::ios::binary | std::ios::ate);
	if (!file.is_open())
		return false;

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	// 데이터 읽을 버퍼 생성
	std::vector<char> buffer(size);
	if (!file.read(buffer.data(), size))
		return false;

	ThrowIfFailed(D3DCreateBlob(size, blob));

	memcpy((*blob)->GetBufferPointer(), buffer.data(), size);

	return true;
}

void DX11Shader::reflectShaderVariables()
{

	for (uint32_t typeIndex = 0; typeIndex < static_cast<uint32_t>(ShaderType::Count); typeIndex++)
	{
		if (!m_ShaderReflections[typeIndex])
			continue;

		uint32_t bufSlot = 0;
		ID3D11ShaderReflection* shaderReflection = m_ShaderReflections[typeIndex].Get();
		D3D11_SHADER_DESC shaderDesc = {};
		shaderReflection->GetDesc(&shaderDesc);

		// constant buffer reflection
		for (uint32_t cbindex = 0; cbindex < shaderDesc.ConstantBuffers; cbindex++)
		{
			DX11Shader::CBLayout constantBufferLayout = {};
			ID3D11ShaderReflectionConstantBuffer* constantBuffer = shaderReflection->GetConstantBufferByIndex(cbindex);
			constantBuffer->GetDesc(&constantBufferLayout.Desc);

			if (constantBufferLayout.Desc.Type != D3D_CT_CBUFFER)
				continue;

			for (uint32_t variableIndex = 0; variableIndex < constantBufferLayout.Desc.Variables; variableIndex++)
			{
				ID3D11ShaderReflectionVariable* variable = constantBuffer->GetVariableByIndex(variableIndex);
				D3D11_SHADER_VARIABLE_DESC variableDesc = {};
				variable->GetDesc(&variableDesc);
				constantBufferLayout.Variables.push_back(variableDesc);

				ID3D11ShaderReflectionType* variableType = variable->GetType();
				D3D11_SHADER_TYPE_DESC typeDesc = {};
				variableType->GetDesc(&typeDesc);
				constantBufferLayout.Types.push_back(typeDesc);

				constantBufferLayout.Size += variableDesc.Size;
			}

			constantBufferLayout.Type = static_cast<ShaderType>(typeIndex);
			constantBufferLayout.Register = bufSlot;
			bufSlot++;
			m_CBLayouts.push_back(constantBufferLayout);
		}

		// WOO : uav 등도 reflection 해야함
		for (uint32_t boundResouceIndex = 0; boundResouceIndex < shaderDesc.BoundResources; boundResouceIndex++)
		{
			D3D11_SHADER_INPUT_BIND_DESC bindDesc = {};
			shaderReflection->GetResourceBindingDesc(boundResouceIndex, &bindDesc);

			switch (bindDesc.Type)
			{
				// constant buffer, 위에서 이미 reflection 했음
			case D3D_SIT_CBUFFER:
				break;
				// texture
			case D3D_SIT_STRUCTURED:
			case D3D_SIT_TEXTURE:
			{
				DX11ResourceBinding textureBinding = {};
				textureBinding.Type = static_cast<ShaderType>(typeIndex);
				textureBinding.Name = bindDesc.Name;
				textureBinding.Register = bindDesc.BindPoint;
				textureBinding.BindCount = bindDesc.BindCount;
				textureBinding.Flags = bindDesc.uFlags;
				textureBinding.ReturnType = bindDesc.ReturnType;
				textureBinding.Dimension = bindDesc.Dimension;
				textureBinding.NumSamples = bindDesc.NumSamples;
				textureBinding.ResourceType = bindDesc.Type;

				m_TextureBindings.emplace_back(textureBinding);
				m_TextureRegisterMap.insert({ bindDesc.Name, textureBinding.Register });

			}
			break;

			// sampler
			case D3D_SIT_SAMPLER:
			{
				DX11ResourceBinding samplerBinding = {};
				samplerBinding.Type = static_cast<ShaderType>(typeIndex);
				samplerBinding.Name = bindDesc.Name;
				samplerBinding.Register = bindDesc.BindPoint;
				samplerBinding.BindCount = bindDesc.BindCount;
				samplerBinding.Flags = bindDesc.uFlags;
				samplerBinding.ReturnType = bindDesc.ReturnType;
				samplerBinding.Dimension = bindDesc.Dimension;
				samplerBinding.NumSamples = bindDesc.NumSamples;
				samplerBinding.ResourceType = bindDesc.Type;

				//if (samplerBinding.Name == "gsamLinearWrap" || samplerBinding.Name == "gsamPointWrap" || samplerBinding.Name == "gsamAnisotropicWrap")
				//{
				//	sampler = m_ResourceManager->CreateSampler(D3D11_TEXTURE_ADDRESS_WRAP, samplerBinding).get();
				//}
				//else if (samplerBinding.Name == "gsamPointClamp" || samplerBinding.Name == "gsamLinearClamp" || samplerBinding.Name == "gsamAnisotropicClamp")
				//{
				//	sampler = m_ResourceManager->CreateSampler(D3D11_TEXTURE_ADDRESS_CLAMP, samplerBinding).get();
				//}
				//else if (samplerBinding.Name == "gsamShadow")
				//{
				//	sampler = m_ResourceManager->CreateSampler(D3D11_TEXTURE_ADDRESS_BORDER, samplerBinding).get();
				//}

					// 위 양식으로 enum class로 집어넣는다.
				if (samplerBinding.Name == "gsamLinearWrap")
					samplerBinding.samplerType = SamplerType::gsamLinearWrap;
				else if (samplerBinding.Name == "gsamPointWrap")
					samplerBinding.samplerType = SamplerType::gsamPointWrap;
				else if (samplerBinding.Name == "gsamAnisotropicWrap")
					samplerBinding.samplerType = SamplerType::gsamAnisotropicWrap;
				else if (samplerBinding.Name == "gsamPointClamp")
					samplerBinding.samplerType = SamplerType::gsamPointClamp;
				else if (samplerBinding.Name == "gsamLinearClamp")
					samplerBinding.samplerType = SamplerType::gsamLinearClamp;
				else if (samplerBinding.Name == "gsamAnisotropicClamp")
					samplerBinding.samplerType = SamplerType::gsamAnisotropicClamp;
				else if (samplerBinding.Name == "gsamShadow")
					samplerBinding.samplerType = SamplerType::gsamShadow;

				m_SamplerBindings.emplace_back(samplerBinding);
				m_SamplerRegisterMap.insert({ bindDesc.Name, samplerBinding.Register });
			}
			break;
			case D3D_SIT_TBUFFER:
				break;

			case D3D_SIT_BYTEADDRESS:
				break;
			case D3D_SIT_UAV_RWBYTEADDRESS:
				break;
			case D3D_SIT_UAV_RWTYPED:
			case D3D_SIT_UAV_RWSTRUCTURED:
			case D3D_SIT_UAV_APPEND_STRUCTURED:
			case D3D_SIT_UAV_CONSUME_STRUCTURED:
			case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
			{
				DX11ResourceBinding uavBinding = {};
				uavBinding.Type = static_cast<ShaderType>(typeIndex);
				uavBinding.Name = bindDesc.Name;
				uavBinding.Register = bindDesc.BindPoint;
				uavBinding.BindCount = bindDesc.BindCount;
				uavBinding.Flags = bindDesc.uFlags;
				uavBinding.ReturnType = bindDesc.ReturnType;
				uavBinding.Dimension = bindDesc.Dimension;
				uavBinding.NumSamples = bindDesc.NumSamples;
				uavBinding.ResourceType = bindDesc.Type;

				m_UAVBindings.emplace_back(uavBinding);
				m_UAVRegisterMap.insert({ bindDesc.Name, uavBinding.Register });
			}
			break;
			case D3D_SIT_RTACCELERATIONSTRUCTURE:
				break;
			case D3D_SIT_UAV_FEEDBACKTEXTURE:
				break;
			default:
				break;
			}
		}
	}
}

void DX11Shader::setConstantMapping(const std::string& name, const void* value)
{
	/*auto iter = m_CBMapping.find(name);
	if (iter != m_CBMapping.end())
	{
		auto& mapping = iter->second;

		auto temp = *mapping.Data;
		temp += mapping.Offset;
		memcpy(temp, value, mapping.Size);
		m_ConstantBuffers[mapping.Index]->m_IsDirty = true;
	}*/

	auto bufIter = m_CBMapping.find(name);
	if (bufIter != m_CBMapping.end())
	{
		for (auto& mapping : bufIter->second)
		{
			auto temp = *mapping.Data;
			temp += mapping.Offset;
			memcpy(temp, value, mapping.Size);
			float dbg = *reinterpret_cast<float*>(temp);
			m_ConstantBuffers[mapping.Index]->m_IsDirty = true;
		}
	}
}

void DX11Shader::createInputLayout(RendererContext* context)
{
	if (m_ShaderReflections[static_cast<uint32_t>(ShaderType::Vertex)])
	{
		D3D11_SHADER_DESC shaderDesc = {};
		m_ShaderReflections[static_cast<uint32_t>(ShaderType::Vertex)]->GetDesc(&shaderDesc);

		for (uint32_t i = 0; i < shaderDesc.InputParameters; i++)
		{
			D3D11_SIGNATURE_PARAMETER_DESC paramDesc = {};
			m_ShaderReflections[static_cast<uint32_t>(ShaderType::Vertex)]->GetInputParameterDesc(i, &paramDesc);

			D3D11_INPUT_ELEMENT_DESC inputElementDesc = {};
			inputElementDesc.SemanticName = paramDesc.SemanticName;
			inputElementDesc.SemanticIndex = paramDesc.SemanticIndex;
			inputElementDesc.InputSlot = paramDesc.Stream;
			inputElementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			inputElementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

			if (paramDesc.Mask == 1)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) inputElementDesc.Format = DXGI_FORMAT_R32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) inputElementDesc.Format = DXGI_FORMAT_R32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) inputElementDesc.Format = DXGI_FORMAT_R32_FLOAT;

				if (inputElementDesc.SemanticName == "POSITION")
				{
					m_InputLayoutFlags.usePosition = true;
					m_InputLayoutFlags.positionStride = 4;
				}

				if (inputElementDesc.SemanticName == "NORMAL")
				{
					m_InputLayoutFlags.useNormal = true;
					m_InputLayoutFlags.normalStride = 4;
				}

				if (inputElementDesc.SemanticName == "TEXCOORD0")
				{
					m_InputLayoutFlags.useUV1 = true;
					m_InputLayoutFlags.uv1Stride = 4;
				}

				if (inputElementDesc.SemanticName == "TEXCOORD1")
				{
					m_InputLayoutFlags.useUV2 = true;
					m_InputLayoutFlags.uv2Stride = 4;
				}

				if (inputElementDesc.SemanticName == "TANGENT")
				{
					m_InputLayoutFlags.useTangent = true;
					m_InputLayoutFlags.tangentStride = 4;
				}

				if (inputElementDesc.SemanticName == "BITANGENT")
				{
					m_InputLayoutFlags.useBiTangent = true;
					m_InputLayoutFlags.biTangentStride = 4;
				}

				if (inputElementDesc.SemanticName == "COLOR")
				{
					m_InputLayoutFlags.useColor = true;
					m_InputLayoutFlags.colorStride = 4;
				}
			}
			else if (paramDesc.Mask <= 3)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) inputElementDesc.Format = DXGI_FORMAT_R32G32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) inputElementDesc.Format = DXGI_FORMAT_R32G32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) inputElementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;

				if (inputElementDesc.SemanticName == "POSITION")
				{
					m_InputLayoutFlags.usePosition = true;
					m_InputLayoutFlags.positionStride = 8;
				}

				if (inputElementDesc.SemanticName == "NORMAL")
				{
					m_InputLayoutFlags.useNormal = true;
					m_InputLayoutFlags.normalStride = 8;
				}

				if (inputElementDesc.SemanticName == "TEXCOORD0")
				{
					m_InputLayoutFlags.useUV1 = true;
					m_InputLayoutFlags.uv1Stride = 8;
				}

				if (inputElementDesc.SemanticName == "TEXCOORD1")
				{
					m_InputLayoutFlags.useUV2 = true;
					m_InputLayoutFlags.uv2Stride = 8;
				}

				if (inputElementDesc.SemanticName == "TANGENT")
				{
					m_InputLayoutFlags.useTangent = true;
					m_InputLayoutFlags.tangentStride = 8;
				}

				if (inputElementDesc.SemanticName == "BITANGENT")
				{
					m_InputLayoutFlags.useBiTangent = true;
					m_InputLayoutFlags.biTangentStride = 8;
				}

				if (inputElementDesc.SemanticName == "COLOR")
				{
					m_InputLayoutFlags.useColor = true;
					m_InputLayoutFlags.colorStride = 8;
				}
			}
			else if (paramDesc.Mask <= 7)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) inputElementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) inputElementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) inputElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;

				if (inputElementDesc.SemanticName == "POSITION")
				{
					m_InputLayoutFlags.usePosition = true;
					m_InputLayoutFlags.positionStride = 12;
				}

				if (inputElementDesc.SemanticName == "NORMAL")
				{
					m_InputLayoutFlags.useNormal = true;
					m_InputLayoutFlags.normalStride = 12;
				}

				if (inputElementDesc.SemanticName == "TEXCOORD0")
				{
					m_InputLayoutFlags.useUV1 = true;
					m_InputLayoutFlags.uv1Stride = 12;
				}

				if (inputElementDesc.SemanticName == "TEXCOORD1")
				{
					m_InputLayoutFlags.useUV2 = true;
					m_InputLayoutFlags.uv2Stride = 12;
				}

				if (inputElementDesc.SemanticName == "TANGENT")
				{
					m_InputLayoutFlags.useTangent = true;
					m_InputLayoutFlags.tangentStride = 12;
				}

				if (inputElementDesc.SemanticName == "BITANGENT")
				{
					m_InputLayoutFlags.useBiTangent = true;
					m_InputLayoutFlags.biTangentStride = 12;
				}

				if (inputElementDesc.SemanticName == "COLOR")
				{
					m_InputLayoutFlags.useColor = true;
					m_InputLayoutFlags.colorStride = 12;
				}

			}
			else if (paramDesc.Mask <= 15)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) inputElementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) inputElementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) inputElementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

				if (inputElementDesc.SemanticName == "POSITION")
				{
					m_InputLayoutFlags.usePosition = true;
					m_InputLayoutFlags.positionStride = 16;
				}

				if (inputElementDesc.SemanticName == "NORMAL")
				{
					m_InputLayoutFlags.useNormal = true;
					m_InputLayoutFlags.normalStride = 16;
				}

				if (inputElementDesc.SemanticName == "TEXCOORD0")
				{
					m_InputLayoutFlags.useUV1 = true;
					m_InputLayoutFlags.uv1Stride = 16;
				}

				if (inputElementDesc.SemanticName == "TEXCOORD1")
				{
					m_InputLayoutFlags.useUV2 = true;
					m_InputLayoutFlags.uv2Stride = 16;
				}

				if (inputElementDesc.SemanticName == "TANGENT")
				{
					m_InputLayoutFlags.useTangent = true;
					m_InputLayoutFlags.tangentStride = 16;
				}

				if (inputElementDesc.SemanticName == "BITANGENT")
				{
					m_InputLayoutFlags.useBiTangent = true;
					m_InputLayoutFlags.biTangentStride = 16;
				}

				if (inputElementDesc.SemanticName == "COLOR")
				{
					m_InputLayoutFlags.useColor = true;
					m_InputLayoutFlags.colorStride = 16;
				}
			}

			m_InputLayouts.push_back(inputElementDesc);
		}

		assert(m_InputLayouts.size() > 0);
		//assert(dynamic_cast<NeoDX11Context*>(context) != nullptr);

		NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

		HRESULT hr = dx11Context->GetDevice()->CreateInputLayout(
			m_InputLayouts.data(),
			static_cast<uint32_t>(m_InputLayouts.size()),
			getVertexBlob()->GetBufferPointer(),
			getVertexBlob()->GetBufferSize(),
			m_InputLayout.put());

		assert(SUCCEEDED(hr));
	}
}

void DX11Shader::createConstantBuffer(RendererContext* context)
{
	m_ConstantBuffers.reserve(m_CBLayouts.size());

	uint32_t cbMappingIndex = 0;

	for (auto& layout : m_CBLayouts)
	{
		auto findIt = m_ConstantBufferMap.find(layout.Desc.Name);
		if (findIt == m_ConstantBufferMap.end())
		{
			m_ConstantBufferMap.insert({ layout.Desc.Name, std::vector<DX11ConstantBuffer*>() });
			findIt = m_ConstantBufferMap.find(layout.Desc.Name);
		}
		m_ConstantBuffers.emplace_back(DX11ConstantBuffer::Create(context, layout.Size));
		findIt->second.emplace_back(m_ConstantBuffers.back().get());

		auto buffer = findIt->second.back();
		buffer->m_ParameterIndex = layout.ParameterIndex;
		buffer->m_Register = layout.Register;
		buffer->m_ShaderType = layout.Type;

		for (auto& var : layout.Variables)
		{
			CBMapping mapping = {};
			mapping.Type = layout.Type;
			mapping.Offset = var.StartOffset;
			mapping.BufferName = layout.Desc.Name;
			mapping.VariableName = var.Name;
			mapping.Size = var.Size;
			mapping.Register = layout.Register;
			mapping.Index = cbMappingIndex;

			mapping.Data = reinterpret_cast<BYTE**>(&buffer->m_MappedResource.pData);

			m_CBMapping[var.Name].emplace_back(mapping);
		}

		cbMappingIndex++;
	}
}

void DX11Shader::createVertexShader(RendererContext* context, std::string_view path)
{
	//assert(dynamic_cast<NeoDX11Context*>(context) != nullptr && "Wrong Context");
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	Microsoft::WRL::ComPtr<ID3DBlob> blobPtr = nullptr;
	blobPtr.Attach(compileShaderFromFile(path, "VSMain", "vs_5_0"));

	if (blobPtr)
	{
		m_ShaderBlobs[static_cast<uint32_t>(ShaderType::Vertex)] = blobPtr;
		Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShaderPtr = nullptr;
		HRESULT hr = dx11Context->GetDevice()->CreateVertexShader(
			getVertexBlob()->GetBufferPointer(),
			getVertexBlob()->GetBufferSize(),
			nullptr,
			vertexShaderPtr.GetAddressOf());

		if (SUCCEEDED(hr))
			m_Shaders[static_cast<uint32_t>(ShaderType::Vertex)] = vertexShaderPtr;

		ID3D11ShaderReflection* shaderReflection = nullptr;
		hr = D3DReflect(
			getVertexBlob()->GetBufferPointer(),
			getVertexBlob()->GetBufferSize(),
			IID_ID3D11ShaderReflection,
			reinterpret_cast<void**>(&shaderReflection));

		assert(SUCCEEDED(hr));

		m_ShaderReflections[static_cast<uint32_t>(ShaderType::Vertex)].Attach(shaderReflection);

		createInputLayout(context);
	}
}

void DX11Shader::createPixelShader(RendererContext* context, std::string_view path)
{
	//assert(dynamic_cast<NeoDX11Context*>(context) != nullptr && "Wrong Context");
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	Microsoft::WRL::ComPtr<ID3DBlob> blobPtr = nullptr;
	blobPtr.Attach(compileShaderFromFile(path, "PSMain", "ps_5_0"));

	if (blobPtr)
	{
		m_ShaderBlobs[static_cast<uint32_t>(ShaderType::Pixel)] = blobPtr;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShaderPtr = nullptr;
		HRESULT hr = dx11Context->GetDevice()->CreatePixelShader(
			getPixelBlob()->GetBufferPointer(),
			getPixelBlob()->GetBufferSize(),
			nullptr,
			pixelShaderPtr.GetAddressOf());

		if (SUCCEEDED(hr))
			m_Shaders[static_cast<uint32_t>(ShaderType::Pixel)] = pixelShaderPtr;

		ID3D11ShaderReflection* shaderReflection = nullptr;
		hr = D3DReflect(
			getPixelBlob()->GetBufferPointer(),
			getPixelBlob()->GetBufferSize(),
			IID_ID3D11ShaderReflection,
			reinterpret_cast<void**>(&shaderReflection));

		m_ShaderReflections[static_cast<uint32_t>(ShaderType::Pixel)].Attach(shaderReflection);

		assert(SUCCEEDED(hr));
	}
}

void DX11Shader::createComputeShader(RendererContext* context, std::string_view path)
{
	//assert(dynamic_cast<NeoDX11Context*>(context) != nullptr && "Wrong Context");
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	Microsoft::WRL::ComPtr<ID3DBlob> blobPtr = nullptr;
	blobPtr.Attach(compileShaderFromFile(path, "CSMain", "cs_5_0"));

	if (blobPtr)
	{
		m_ShaderBlobs[static_cast<uint32_t>(ShaderType::Compute)] = blobPtr;
		Microsoft::WRL::ComPtr<ID3D11ComputeShader> computeShaderPtr = nullptr;
		HRESULT hr = dx11Context->GetDevice()->CreateComputeShader(
			getComputeBlob()->GetBufferPointer(),
			getComputeBlob()->GetBufferSize(),
			nullptr,
			computeShaderPtr.GetAddressOf());

		if (SUCCEEDED(hr))
			m_Shaders[static_cast<uint32_t>(ShaderType::Compute)] = computeShaderPtr;

		ID3D11ShaderReflection* shaderReflection = nullptr;
		hr = D3DReflect(
			getComputeBlob()->GetBufferPointer(),
			getComputeBlob()->GetBufferSize(),
			IID_ID3D11ShaderReflection,
			reinterpret_cast<void**>(&shaderReflection));

		m_ShaderReflections[static_cast<uint32_t>(ShaderType::Compute)].Attach(shaderReflection);

		assert(SUCCEEDED(hr));
	}
}

void DX11Shader::createGeometryShader(RendererContext* context, std::string_view path)
{
	//assert(dynamic_cast<NeoDX11Context*>(context) != nullptr && "Wrong Context");
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	Microsoft::WRL::ComPtr<ID3DBlob> blobPtr = nullptr;
	blobPtr.Attach(compileShaderFromFile(path, "GSMain", "gs_5_0"));

	if (blobPtr)
	{
		m_ShaderBlobs[static_cast<uint32_t>(ShaderType::Geometry)] = blobPtr;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader> geometryShaderPtr = nullptr;
		HRESULT hr = dx11Context->GetDevice()->CreateGeometryShader(
			getGeometryBlob()->GetBufferPointer(),
			getGeometryBlob()->GetBufferSize(),
			nullptr,
			geometryShaderPtr.GetAddressOf());

		if (SUCCEEDED(hr))
			m_Shaders[static_cast<uint32_t>(ShaderType::Geometry)] = geometryShaderPtr;

		ID3D11ShaderReflection* shaderReflection = nullptr;
		hr = D3DReflect(
			getGeometryBlob()->GetBufferPointer(),
			getGeometryBlob()->GetBufferSize(),
			IID_ID3D11ShaderReflection,
			reinterpret_cast<void**>(&shaderReflection));

		m_ShaderReflections[static_cast<uint32_t>(ShaderType::Geometry)].Attach(shaderReflection);

		assert(SUCCEEDED(hr));
	}
}

void DX11Shader::createStreamOutGeometryShader(RendererContext* context, std::string_view path)
{
	//assert(dynamic_cast<NeoDX11Context*>(context) != nullptr && "Wrong Context");
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	D3D11_SO_DECLARATION_ENTRY desc[] =
	{
		{ 0, "POSITION", 0, 0, 3, 0},
			{ 0, "VELOCITY", 0, 0, 3, 0},
			{ 0, "SIZE", 0, 0, 2, 0},
			{ 0, "AGE", 0, 0, 1, 0},
			{ 0, "TYPE", 0, 0, 1, 0},
	};

	uint32_t bufferStrides[] = { sizeof(uint32_t) };

	Microsoft::WRL::ComPtr<ID3DBlob> blobPtr = nullptr;
	blobPtr.Attach(compileShaderFromFile(path, "StreamOutGS", "gs_5_0"));

	if (blobPtr)
	{
		m_ShaderBlobs[static_cast<uint32_t>(ShaderType::StreamOutGeometry)] = blobPtr;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader> geometryShaderPtr = nullptr;
		HRESULT hr = dx11Context->GetDevice()->CreateGeometryShaderWithStreamOutput(
			getStreamOutGeometryBlob()->GetBufferPointer(),	// compile된 진입점
			getStreamOutGeometryBlob()->GetBufferSize(),		// compile된 사이즈
			desc,	// vertex 형식
			ARRAYSIZE(desc),	// vertex 형식 개수
			bufferStrides,	// vertex size
			ARRAYSIZE(bufferStrides),
			D3D11_SO_NO_RASTERIZED_STREAM,	/// rasterizer 단계로 보낼 stream index 번호, 없으면 D3D11_SO_NO_RASTERIZED_STREAM
			nullptr,
			geometryShaderPtr.GetAddressOf());

		if (SUCCEEDED(hr))
			m_Shaders[static_cast<uint32_t>(ShaderType::StreamOutGeometry)] = geometryShaderPtr;

		ID3D11ShaderReflection* shaderReflection = nullptr;
		hr = D3DReflect(
			getStreamOutGeometryBlob()->GetBufferPointer(),
			getStreamOutGeometryBlob()->GetBufferSize(),
			IID_ID3D11ShaderReflection,
			reinterpret_cast<void**>(&shaderReflection));

		m_ShaderReflections[static_cast<uint32_t>(ShaderType::StreamOutGeometry)].Attach(shaderReflection);

		assert(SUCCEEDED(hr));
	}
}

void DX11Shader::createHullShader(RendererContext* context, std::string_view path)
{
	//assert(dynamic_cast<NeoDX11Context*>(context) != nullptr && "Wrong Context");
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	Microsoft::WRL::ComPtr<ID3DBlob> blobPtr = nullptr;
	blobPtr.Attach(compileShaderFromFile(path, "HSMain", "hs_5_0"));

	if (blobPtr)
	{
		m_ShaderBlobs[static_cast<uint32_t>(ShaderType::Hull)] = blobPtr;
		Microsoft::WRL::ComPtr<ID3D11HullShader> hullShaderPtr = nullptr;
		HRESULT hr = dx11Context->GetDevice()->CreateHullShader(
			getHullBlob()->GetBufferPointer(),
			getHullBlob()->GetBufferSize(),
			nullptr,
			hullShaderPtr.GetAddressOf());

		if (SUCCEEDED(hr))
			m_Shaders[static_cast<uint32_t>(ShaderType::Hull)] = hullShaderPtr;

		ID3D11ShaderReflection* shaderReflection = nullptr;
		hr = D3DReflect(
			getHullBlob()->GetBufferPointer(),
			getHullBlob()->GetBufferSize(),
			IID_ID3D11ShaderReflection,
			reinterpret_cast<void**>(&shaderReflection));

		m_ShaderReflections[static_cast<uint32_t>(ShaderType::Hull)].Attach(shaderReflection);

		assert(SUCCEEDED(hr));
	}
}

void DX11Shader::createDomainShader(RendererContext* context, std::string_view path)
{
	//assert(dynamic_cast<NeoDX11Context*>(context) != nullptr && "Wrong Context");
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	Microsoft::WRL::ComPtr<ID3DBlob> blobPtr = nullptr;
	blobPtr.Attach(compileShaderFromFile(path, "DSMain", "ds_5_0"));

	if (blobPtr)
	{
		m_ShaderBlobs[static_cast<uint32_t>(ShaderType::Domain)] = blobPtr;
		Microsoft::WRL::ComPtr<ID3D11DomainShader> domainShaderPtr = nullptr;
		HRESULT hr = dx11Context->GetDevice()->CreateDomainShader(
			getDomainBlob()->GetBufferPointer(),
			getDomainBlob()->GetBufferSize(),
			nullptr,
			domainShaderPtr.GetAddressOf());

		if (SUCCEEDED(hr))
			m_Shaders[static_cast<uint32_t>(ShaderType::Domain)] = domainShaderPtr;

		ID3D11ShaderReflection* shaderReflection = nullptr;
		hr = D3DReflect(
			getDomainBlob()->GetBufferPointer(),
			getDomainBlob()->GetBufferSize(),
			IID_ID3D11ShaderReflection,
			reinterpret_cast<void**>(&shaderReflection));

		m_ShaderReflections[static_cast<uint32_t>(ShaderType::Domain)].Attach(shaderReflection);

		assert(SUCCEEDED(hr));
	}
}

void DX11Shader::Destroy()
{
	for (auto& shader : m_Shaders)
		if (shader)
			shader = nullptr;

	for (auto& blob : m_ShaderBlobs)
		if (blob)
			blob = nullptr;

	for (auto& reflection : m_ShaderReflections)
		if (reflection)
			reflection = nullptr;

	m_CBLayouts.clear();
	m_InputLayouts.clear();
	m_ConstantBufferMap.clear();
	m_CBMapping.clear();
	m_ConstantBuffers.clear();
	m_Shaders.fill(nullptr);
	m_ShaderBlobs.fill(nullptr);
	m_ShaderReflections.fill(nullptr);
	m_InputLayout = nullptr;
}

STDMETHODIMP ShaderIncludeHandler::Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
{
	std::filesystem::path includePath = std::filesystem::current_path() / "Shaders" / pFileName;
	std::ifstream file(includePath, std::ios::binary);
	if (!file.is_open())
	{
		std::cerr << "Failed to open file: " << includePath << std::endl;
		return E_FAIL;
	}

	file.seekg(0, std::ios::end);
	uint32_t size = static_cast<uint32_t>(file.tellg());
	file.seekg(0, std::ios::beg);

	char* buffer = new char[size];
	file.read(buffer, size);
	file.close();

	*ppData = buffer;
	*pBytes = size;

	return S_OK;
}

STDMETHODIMP ShaderIncludeHandler::Close(LPCVOID pData)
{
	char* buffer = const_cast<char*>(reinterpret_cast<const char*>(pData));
	delete[] buffer;
	return S_OK;
}
