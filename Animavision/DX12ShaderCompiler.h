#pragma once

// jEngine 코드를 거의 참고하면서 만들었다.. 너무 어렵다 다음에 배우면 내가 배운대로 짜보기로 하자
// 링크 : 

#include <dxcapi.h>

class DXC
{
public:
	DXC();
	~DXC();

	template <typename T>
	HRESULT CreateInstance(REFCLSID InCLSID, T** pResult) const
	{
		return CreateInstance(InCLSID, __uuidof(T), (IUnknown**)pResult);
	}

	template <typename T>
	HRESULT CreateInstance2(IMalloc* pMalloc, REFCLSID InCLSID, T** pResult) const
	{
		return CreateInstance2(pMalloc, InCLSID, __uuidof(T), (IUnknown**)pResult);
	}

	HRESULT CreateInstance(REFCLSID InCLSID, REFIID InIID, IUnknown** pResult) const;
	HRESULT CreateInstance2(IMalloc* pMalloc, REFCLSID InCLSID, REFIID InIID, IUnknown** pResult) const;

	bool IsEnable() const { return m_hModule != nullptr; }

private:
	HMODULE m_hModule = nullptr;
	DxcCreateInstanceProc m_DxcCreateInstance = nullptr;
	DxcCreateInstance2Proc m_DxcCreateInstance2 = nullptr;
};

// 싱글톤으로 만들어보자
class DX12ShaderCompiler
{
public:
	static DX12ShaderCompiler* GetInstance()
	{
		if (atexit(ReleaseInstance))
		{
			ReleaseInstance();
		}
		if (m_Instance == nullptr)
		{
			m_Instance = new DX12ShaderCompiler();
		}

		return m_Instance;
	}

	static void ReleaseInstance()
	{
		if (m_Instance != nullptr)
		{
			delete m_Instance;
			m_Instance = nullptr;
		}
	}

	winrt::com_ptr<IDxcResult> CompileFromFile(const std::wstring& InFileName, const std::wstring& InShaderModel,
		const std::wstring& InEntryPoint, bool InRowMajorMatrix = true) const;

	winrt::com_ptr<IDxcResult> Compile(LPCSTR InShaderCode, uint32_t InShaderCodeLength, LPCWSTR InShaderModel,
		LPCWSTR InEntryPoint, bool InRowMajorMatrix = true, std::vector<LPCWSTR> InCompileOptions = {}) const;

	winrt::com_ptr<ID3D12ShaderReflection> Reflect(IDxcResult* InResult) const;

	winrt::com_ptr<ID3D12LibraryReflection> ReflectLibrary(IDxcResult* InResult) const;

	winrt::com_ptr<IDxcBlob> GetBlob(IDxcResult* InResult) const;

	winrt::com_ptr<IDxcBlob> CreateBlob(const void* InData, uint32_t InSize) const;



public:
public:
	DX12ShaderCompiler(const DX12ShaderCompiler&) = delete;
	DX12ShaderCompiler& operator=(const DX12ShaderCompiler&) = delete;
	DX12ShaderCompiler(DX12ShaderCompiler&&) = delete;
	DX12ShaderCompiler& operator=(DX12ShaderCompiler&&) = delete;

private:
	DXC m_DXC;


	DX12ShaderCompiler() {}
	static DX12ShaderCompiler* m_Instance;

};

