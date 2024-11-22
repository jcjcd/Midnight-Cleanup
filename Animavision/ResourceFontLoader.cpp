#include "pch.h"
#include "ResourceFontLoader.h"

ResourceFontFileStream::ResourceFontFileStream(const std::wstring& filePath)
{
	// Open the font file in binary mode and read its content into memory
	FILE* file = nullptr;
	_wfopen_s(&file, filePath.c_str(), L"rb");
	if (file)
	{
		fseek(file, 0, SEEK_END);
		size_t size = ftell(file);
		fseek(file, 0, SEEK_SET);

		m_FontData.resize(size);
		fread(m_FontData.data(), 1, size, file);
		fclose(file);
	}
}

HRESULT __stdcall ResourceFontFileStream::QueryInterface(REFIID riid, void** ppvObject)
{
	if (riid == IID_IUnknown || riid == __uuidof(IDWriteFontFileStream))
	{
		*ppvObject = this;
		AddRef();
		return S_OK;
	}
	else
	{
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}
}

ULONG __stdcall ResourceFontFileStream::AddRef(void)
{
	return InterlockedIncrement(&m_RefCount);
}

ULONG __stdcall ResourceFontFileStream::Release(void)
{
	ULONG newCount = InterlockedDecrement(&m_RefCount);
	if (newCount == 0)
		delete this;

	return newCount;
}

HRESULT ResourceFontFileStream::ReadFileFragment(const void** fragmentStart, UINT64 fileOffset, UINT64 fragmentSize, void** fragmentContext) noexcept
{
	if (fileOffset + fragmentSize > m_FontData.size())
		return E_FAIL;

	*fragmentStart = m_FontData.data() + fileOffset;
	*fragmentContext = nullptr;
	return S_OK;
}

void ResourceFontFileStream::ReleaseFileFragment(void* fragmentContext) noexcept
{
	// No action required since we don't use fragment context
}

HRESULT ResourceFontFileStream::GetFileSize(UINT64* fileSize) noexcept
{
	*fileSize = m_FontData.size();
	return S_OK;
}

HRESULT ResourceFontFileStream::GetLastWriteTime(UINT64* lastWriteTime) noexcept
{
	// The concept of last write time does not apply to this loader.
	*lastWriteTime = 0;
	return E_NOTIMPL;
}

HRESULT __stdcall ResourceFontFileLoader::QueryInterface(REFIID riid, void** ppvObject)
{
	if (riid == IID_IUnknown || riid == __uuidof(IDWriteFontFileLoader))
	{
		*ppvObject = this;
		AddRef();
		return S_OK;
	}
	else
	{
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}
}

ULONG __stdcall ResourceFontFileLoader::AddRef(void)
{
	return InterlockedIncrement(&m_RefCount);
}

ULONG __stdcall ResourceFontFileLoader::Release(void)
{
	ULONG newCount = InterlockedDecrement(&m_RefCount);
	if (newCount == 0)
		delete this;

	return newCount;
}

// CreateStreamFromKey
//
//      Creates an IDWriteFontFileStream from a key that identifies the file. The
//      format and meaning of the key is entirely up to the loader implementation.
//      The only requirements imposed by DWrite are that a key must remain valid
//      for as long as the loader is registered. The same key must also uniquely
//      identify the same file, so for example you must not recycle keys so that
//      a key might represent different files at different times.
//
//      In this case the key is a UINT which identifies a font resources.
//
HRESULT ResourceFontFileLoader::CreateStreamFromKey(
	const void* fontFileReferenceKey, // [fontFileReferenceKeySize] in bytes
	UINT32 fontFileReferenceKeySize, IDWriteFontFileStream** fontFileStream) noexcept
{
	*fontFileStream = nullptr;

	// Make sure the key is the right size.
	if (fontFileReferenceKeySize % sizeof(wchar_t) != 0)
		return E_INVALIDARG;

	const wchar_t* filePath = nullptr;
	filePath = reinterpret_cast<const wchar_t*>(fontFileReferenceKey);

	// Check if the filePath is valid
	if (filePath == nullptr || wcslen(filePath) == 0)
		return E_INVALIDARG;

	// Create the stream object.
	ResourceFontFileStream* stream = new(std::nothrow) ResourceFontFileStream(filePath);
	if (!stream)
		return E_OUTOFMEMORY;

	if (!stream->IsInitialized())
	{
		delete stream;
		return E_FAIL;
	}

	*fontFileStream = SafeAcquire(stream);

	return S_OK;
}

// Smart pointer to singleton instance of the font file loader.
IDWriteFontFileLoader* ResourceFontFileLoader::m_Instance(new(std::nothrow) ResourceFontFileLoader());

ResourceFontFileEnumerator::ResourceFontFileEnumerator(IDWriteFactory* factory)
	: m_RefCount(1), m_Factory(SafeAcquire(factory)), m_CurrrentFontFile(nullptr), m_NextIndex(0)
{

}

//HRESULT ResourceFontFileEnumerator::Initialize(
//	uint32_t const* resourceIDs, // [resourceCount]
//	uint32_t resourceCount)
//{
//	try
//	{
//		m_ResourceIDs.assign(resourceIDs, resourceIDs + resourceCount);
//	}
//	catch (...)
//	{
//		return ExceptionToHResult();
//	}
//	return S_OK;
//}

HRESULT ResourceFontFileEnumerator::Initialize(const std::wstring* filePaths, uint32_t fileCount)
{
	try
	{
		m_FilePaths.assign(filePaths, filePaths + fileCount);
	}
	catch (...)
	{
		return ExceptionToHResult();
	}
	return S_OK;
}

ResourceFontFileEnumerator::~ResourceFontFileEnumerator()
{
	SafeRelease(&m_CurrrentFontFile);
	SafeRelease(&m_Factory);
}

HRESULT __stdcall ResourceFontFileEnumerator::QueryInterface(REFIID riid, void** ppvObject)
{
	if (riid == IID_IUnknown || riid == __uuidof(IDWriteFontFileEnumerator))
	{
		*ppvObject = this;
		AddRef();
		return S_OK;
	}
	else
	{
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}
}

ULONG __stdcall ResourceFontFileEnumerator::AddRef(void)
{
	return InterlockedIncrement(&m_RefCount);
}

ULONG __stdcall ResourceFontFileEnumerator::Release(void)
{
	ULONG newCount = InterlockedDecrement(&m_RefCount);
	if (newCount == 0)
		delete this;

	return newCount;
}

HRESULT ResourceFontFileEnumerator::MoveNext(BOOL* hasCurrentFile) noexcept
{
	HRESULT hr = S_OK;

	*hasCurrentFile = false;
	SafeRelease(&m_CurrrentFontFile);

	if (m_NextIndex < m_FilePaths.size())
	{
		const std::wstring& filePath = m_FilePaths[m_NextIndex];
		const wchar_t* key = filePath.c_str();
		uint32_t keySize = static_cast<uint32_t>((filePath.length() + 1) * sizeof(wchar_t));

		hr = m_Factory->CreateCustomFontFileReference(
			key,
			keySize,
			ResourceFontFileLoader::GetLoader(),
			&m_CurrrentFontFile
		);

		if (SUCCEEDED(hr))
		{
			*hasCurrentFile = true;

			++m_NextIndex;
		}
	}

	return hr;
}

HRESULT ResourceFontFileEnumerator::GetCurrentFontFile(IDWriteFontFile** fontFile) noexcept
{
	*fontFile = SafeAcquire(m_CurrrentFontFile);

	return (m_CurrrentFontFile != nullptr) ? S_OK : E_FAIL;
}

HRESULT __stdcall ResourceFontCollectionLoader::QueryInterface(REFIID riid, void** ppvObject)
{
	if (riid == IID_IUnknown || riid == __uuidof(IDWriteFontCollectionLoader))
	{
		*ppvObject = this;
		AddRef();
		return S_OK;
	}
	else
	{
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}
}

ULONG __stdcall ResourceFontCollectionLoader::AddRef(void)
{
	return InterlockedIncrement(&m_RefCount);
}

ULONG __stdcall ResourceFontCollectionLoader::Release(void)
{
	ULONG newCount = InterlockedDecrement(&m_RefCount);
	if (newCount == 0)
		delete this;

	return newCount;
}

HRESULT ResourceFontCollectionLoader::CreateEnumeratorFromKey(
	IDWriteFactory* factory,
	const void* collectionKey, // [collectionKeySize] in bytes
	UINT32 collectionKeySize, IDWriteFontFileEnumerator** fontFileEnumerator) noexcept
{
	*fontFileEnumerator = nullptr;

	HRESULT hr = S_OK;

	if (collectionKeySize % sizeof(wchar_t) != 0)
		return E_INVALIDARG;

	const wchar_t* keyData = reinterpret_cast<const wchar_t*>(collectionKey);
	size_t actualLength = wcsnlen(keyData, collectionKeySize / sizeof(wchar_t));

	if (actualLength == 0 || actualLength > collectionKeySize / sizeof(wchar_t))
		return E_INVALIDARG;

	std::wstring filePath(keyData, actualLength);

	ResourceFontFileEnumerator* enumerator = new(std::nothrow) ResourceFontFileEnumerator(factory);
	if (!enumerator)
		return E_OUTOFMEMORY;

	hr = enumerator->Initialize(&filePath, 1);

	if (FAILED(hr))
	{
		delete enumerator;
		return hr;
	}

	*fontFileEnumerator = SafeAcquire(enumerator);

	return hr;
}

IDWriteFontCollectionLoader* ResourceFontCollectionLoader::m_Instance(new(std::nothrow) ResourceFontCollectionLoader());

