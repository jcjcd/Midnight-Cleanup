#include "pch.h"
#include "DX12ShaderCompiler.h"

#include <fstream>
#include <set>
#include <filesystem>


DXC::DXC()
{
	m_hModule = LoadLibrary(L"dxcompiler.dll");

	if (m_hModule)
	{
		m_DxcCreateInstance = (DxcCreateInstanceProc)GetProcAddress(m_hModule, "DxcCreateInstance");
		m_DxcCreateInstance2 = (DxcCreateInstance2Proc)GetProcAddress(m_hModule, "DxcCreateInstance2");
	}
}

DXC::~DXC()
{
	if (m_hModule)
	{
		m_DxcCreateInstance = nullptr;
		m_DxcCreateInstance2 = nullptr;
		FreeLibrary(m_hModule);
		m_hModule = nullptr;
	}
}

HRESULT DXC::CreateInstance(REFCLSID InCLSID, REFIID InIID, IUnknown** pResult) const
{
	if (!pResult)
		return E_POINTER;

	if (!m_hModule)
		return E_FAIL;

	if (!m_DxcCreateInstance)
		return E_FAIL;

	return m_DxcCreateInstance(InCLSID, InIID, (LPVOID*)pResult);
}

HRESULT DXC::CreateInstance2(IMalloc* pMalloc, REFCLSID InCLSID, REFIID InIID, IUnknown** pResult) const
{
	if (!pResult)
		return E_POINTER;

	if (!m_hModule)
		return E_FAIL;

	if (!m_DxcCreateInstance2)
		return E_FAIL;

	return m_DxcCreateInstance2(pMalloc, InCLSID, InIID, (LPVOID*)pResult);
}


winrt::com_ptr<IDxcResult> DX12ShaderCompiler::CompileFromFile(const std::wstring& InFileName, const std::wstring& InShaderModel, const std::wstring& InEntryPoint, bool InRowMajorMatrix /*= false*/) const
{
	if (!m_DXC.IsEnable())
	{
		OutputDebugStringA("Failed to load dxcompiler.dll\n");
		return nullptr;
	}

	if (InFileName.empty())
	{
		OutputDebugStringA("Invalid file name\n");
		return nullptr;
	}

	if (InFileName.find(L".hlsl") == std::wstring::npos)
	{
		OutputDebugStringA("Invalid file extension\n");
		return nullptr;
	}

	std::ifstream shaderFile(InFileName);
	if (!shaderFile.good())
	{
		OutputDebugStringA("Failed to open shader file\n");
		return nullptr;
	}

	std::stringstream strStream;
	strStream << shaderFile.rdbuf();

	std::string shaderCode = strStream.str();

	// 인클루드를 여기서 처리한다.
	constexpr char include[] = "#include \"";
	constexpr size_t includeLength = sizeof(include) - 1;

	const std::filesystem::path filePath(InFileName.c_str());
	const std::string directory = filePath.has_parent_path() ? (filePath.parent_path().string() + "/") : "";

	std::set<std::string> alreadyIncluded;
	while (1)
	{
		size_t includePos = shaderCode.find(include);
		if (includePos == std::string::npos)
			break;

		size_t includeEndPos = shaderCode.find("\"", includePos + includeLength);
		if (includeEndPos == std::string::npos)
			break;

		const std::string includeFileName = shaderCode.substr(includePos + includeLength, includeEndPos - includePos - includeLength);
		if (alreadyIncluded.find(includeFileName) != alreadyIncluded.end())
		{
			shaderCode.erase(includePos, includeEndPos - includePos + 1);
			continue;
		}

		std::ifstream includeFile(directory + includeFileName);
		if (!includeFile.good())
		{
			OutputDebugStringA("Failed to open include file\n");
			break;
		}

		std::stringstream includeStream;
		includeStream << includeFile.rdbuf();
		const std::string includeCode = includeStream.str();

		shaderCode.replace(includePos, includeEndPos - includePos + 1, includeCode);
		alreadyIncluded.insert(includeFileName);
	}
	

	return Compile(shaderCode.c_str(), (uint32_t)shaderCode.size(), InShaderModel.c_str(), InEntryPoint.c_str(), InRowMajorMatrix);
}

winrt::com_ptr<IDxcResult> DX12ShaderCompiler::Compile(LPCSTR InShaderCode, uint32_t InShaderCodeLength, LPCWSTR InShaderModel, LPCWSTR InEntryPoint, bool InRowMajorMatrix, std::vector<LPCWSTR> InCompileOptions) const
{
	if (!m_DXC.IsEnable())
		return nullptr;

	winrt::com_ptr<IDxcUtils> utils;
	if (FAILED(m_DXC.CreateInstance(CLSID_DxcUtils, utils.put())))
		return nullptr;

	winrt::com_ptr<IDxcBlobEncoding> source;
	// Shadercode, length, codepage, IDxcBlobEncoding
	if (FAILED(utils->CreateBlob((LPCVOID)InShaderCode, InShaderCodeLength, 0, source.put())))
		return nullptr;

	winrt::com_ptr<IDxcCompiler3> compiler;
	if (FAILED(m_DXC.CreateInstance(CLSID_DxcCompiler, compiler.put())))
		return nullptr;

	std::vector<LPCWSTR> compileOptions;

	if (InRowMajorMatrix)
		compileOptions.push_back(L"-Zpr");			// Pack matrix in row-major order

	// -E for the entry point (eg. 'main')
	compileOptions.push_back(L"-E");
	compileOptions.push_back(InEntryPoint);

	// -T for the shader model (eg. 'vs_6_0')
	compileOptions.push_back(L"-T");
	compileOptions.push_back(InShaderModel);

 	// 리플렉션 데이터 이런것들을 여기서 넣나보다.. 아닌가..?
 	// TODO 확인해보고 이거 디버그 전처리기로 빼자 
 	// -Qstrip_reflect
 	// -Qstrip_debug
 	// -Qstrip_priv
 	// -Qstrip_rootsignature
// 	compileOptions.push_back(L"-Qstrip_reflect");
// 	compileOptions.push_back(L"-Qstrip_debug");
	// 이 qstrip 쓰는 순간 pix에서 난리난다..

#if _DEBUG
	compileOptions.push_back(TEXT("-Zi"));				// Enable debug information
	compileOptions.push_back(TEXT("-Qembed_debug"));	// Embed debug information
#else
	//compileOptions.push_back(TEXT("-O3"));				// Enable optimization
#endif
	compileOptions.push_back(TEXT("-Od"));				// Disable optimization
	compileOptions.insert(compileOptions.end(), InCompileOptions.begin(), InCompileOptions.end());

	DxcBuffer sourceBuffer = {};
	sourceBuffer.Ptr = source->GetBufferPointer();
	sourceBuffer.Size = source->GetBufferSize();
	sourceBuffer.Encoding = 0;

	winrt::com_ptr<IDxcResult> result;
	if (FAILED(compiler->Compile(&sourceBuffer, compileOptions.data(), (UINT32)compileOptions.size(), nullptr, IID_PPV_ARGS(result.put()))))
		return nullptr;

	// 리플렉션 같은거 하려면 이걸 따로 빼야하네..
	winrt::com_ptr<IDxcBlobUtf8> errorBlob;
	winrt::com_ptr<IDxcBlobWide> errorBlobWide;
	result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errorBlob.put()), errorBlobWide.put());
	if (errorBlob && errorBlob->GetStringLength() > 0)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		return nullptr;
	}
	// 


	return result;
}

winrt::com_ptr<ID3D12ShaderReflection> DX12ShaderCompiler::Reflect(IDxcResult* InResult) const
{
	if (!InResult)
		return nullptr;

	HRESULT hr = S_OK;

	winrt::com_ptr<IDxcBlob> reflectionData;
	InResult->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(reflectionData.put()), nullptr);
	DxcBuffer reflectionBuffer = {};
	reflectionBuffer.Ptr = reflectionData->GetBufferPointer();
	reflectionBuffer.Size = reflectionData->GetBufferSize();
	reflectionBuffer.Encoding = 0;

	winrt::com_ptr<ID3D12ShaderReflection> reflection;
	winrt::com_ptr<IDxcUtils> utils;
	hr = m_DXC.CreateInstance(CLSID_DxcUtils, utils.put());
	if (FAILED(hr))
		return nullptr;
	hr = utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(reflection.put()));

	if (FAILED(hr))
		return nullptr;

	return reflection;
}

winrt::com_ptr<ID3D12LibraryReflection> DX12ShaderCompiler::ReflectLibrary(IDxcResult* InResult) const
{
	if (!InResult)
		return nullptr;

	HRESULT hr = S_OK;

	winrt::com_ptr<IDxcBlob> reflectionData;
	InResult->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(reflectionData.put()), nullptr);
	DxcBuffer reflectionBuffer = {};
	reflectionBuffer.Ptr = reflectionData->GetBufferPointer();
	reflectionBuffer.Size = reflectionData->GetBufferSize();
	reflectionBuffer.Encoding = 0;

	winrt::com_ptr<ID3D12LibraryReflection> reflection;
	winrt::com_ptr<IDxcUtils> utils;
	hr = m_DXC.CreateInstance(CLSID_DxcUtils, utils.put());
	if (FAILED(hr))
		return nullptr;
	hr = utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(reflection.put()));
	if (FAILED(hr))
		return nullptr;

	return reflection;


}

winrt::com_ptr<IDxcBlob> DX12ShaderCompiler::GetBlob(IDxcResult* InResult) const
{
	winrt::com_ptr<IDxcBlob> compiledBlob;
	InResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(compiledBlob.put()), nullptr);
	return compiledBlob;
}

winrt::com_ptr<IDxcBlob> DX12ShaderCompiler::CreateBlob(const void* InData, uint32_t InSize) const
{
	HRESULT hr = S_OK;

	winrt::com_ptr<IDxcUtils> utils;
	hr = m_DXC.CreateInstance(CLSID_DxcUtils, utils.put());
	if (FAILED(hr))
		return nullptr;

	winrt::com_ptr<IDxcBlobEncoding> blob;
	// Shadercode, length, codepage, IDxcBlobEncoding
	utils->CreateBlob((LPCVOID)InData, InSize, 0, blob.put());

	return blob;
}

DX12ShaderCompiler* DX12ShaderCompiler::m_Instance = nullptr;
