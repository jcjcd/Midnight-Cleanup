#include "pch.h"
#include "WooFontLoader.h"

#include <initguid.h>

HRESULT __stdcall WooFontLoader::QueryInterface(REFIID riid, void** ppvObject)
{
	if (!ppvObject)
		return E_POINTER;

	wchar_t buffer[64] = {};
	StringFromGUID2(riid, buffer, ARRAYSIZE(buffer));
	OutputDebugStringW(L"Requested IID: ");
	OutputDebugStringW(buffer);
	OutputDebugStringW(L"\n");

	if (riid == __uuidof(IDWriteFontCollectionLoader) || riid == __uuidof(IUnknown))
		*ppvObject = static_cast<IDWriteFontCollectionLoader*>(this);
	else if (riid == __uuidof(IDWriteFontFileLoader))
		*ppvObject = static_cast<IDWriteFontFileLoader*>(this);
	else
	{
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}

ULONG __stdcall WooFontLoader::AddRef(void)
{
	return InterlockedIncrement(&m_RefCount);
}

ULONG __stdcall WooFontLoader::Release(void)
{
	ULONG newCount = InterlockedDecrement(&m_RefCount);
	if (newCount == 0)
		delete this;
	return newCount;
}

HRESULT __stdcall WooFontLoader::CreateEnumeratorFromKey(IDWriteFactory* factory, void const* collectionKey, UINT32 collectionKeySize, IDWriteFontFileEnumerator** fontFileEnumerator)
{
	if (!factory || !collectionKey || !fontFileEnumerator)
		return E_INVALIDARG;

	if (collectionKeySize != sizeof(FontData))
		return E_INVALIDARG;

	const FontData* fontData = reinterpret_cast<const FontData*>(collectionKey);
	WooFontFileEnumerator* enumerator = new WooFontFileEnumerator(fontData, factory, this);
	if (!enumerator)
		return E_OUTOFMEMORY;

	*fontFileEnumerator = enumerator;
	(*fontFileEnumerator)->AddRef();

	return S_OK;
}

HRESULT __stdcall WooFontLoader::CreateStreamFromKey(void const* fontFileReferenceKey, UINT32 fontFileReferenceKeySize, IDWriteFontFileStream** fontFileStream)
{
	if (fontFileReferenceKeySize != sizeof(FontData))
		return E_INVALIDARG;

	const FontData* fontData = reinterpret_cast<const FontData*>(fontFileReferenceKey);
	Microsoft::WRL::ComPtr<WooFontFileStream> stream = new WooFontFileStream(fontData->data);
	*fontFileStream = stream.Detach();

	return S_OK;
}

WooFontLoader::WooFontFileEnumerator::WooFontFileEnumerator(const FontData* fontData, IDWriteFactory* factory, IDWriteFontFileLoader* fontFileLoader)
	: m_Factory(factory), m_FontFileLoader(fontFileLoader)
{
	m_FontData.data = fontData->data;
}

HRESULT __stdcall WooFontLoader::WooFontFileEnumerator::QueryInterface(REFIID riid, void** ppvObject)
{
	if (!ppvObject)
		return E_POINTER;

	if (riid == __uuidof(IDWriteFontFileEnumerator) || riid == __uuidof(IUnknown))
		*ppvObject = static_cast<IDWriteFontFileEnumerator*>(this);
	else
	{
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}

ULONG __stdcall WooFontLoader::WooFontFileEnumerator::AddRef(void)
{
	return InterlockedIncrement(&m_refCount);
}

ULONG __stdcall WooFontLoader::WooFontFileEnumerator::Release(void)
{
	ULONG newCount = InterlockedDecrement(&m_refCount);
	if (newCount == 0)
		delete this;
	return newCount;
}

HRESULT __stdcall WooFontLoader::WooFontFileEnumerator::MoveNext(BOOL* hasCurrentFile)
{
	if (!m_FontFile)
	{
		HRESULT hr = m_Factory->CreateCustomFontFileReference(
			m_FontData.data.data(),
			m_FontData.data.size(),
			m_FontFileLoader.Get(),
			&m_FontFile
		);
		if (FAILED(hr))
		{
			*hasCurrentFile = false;
			return hr;
		}
	}
	*hasCurrentFile = m_FontFile != nullptr;

	return S_OK;
}

HRESULT __stdcall WooFontLoader::WooFontFileEnumerator::GetCurrentFontFile(IDWriteFontFile** fontFile)
{
	if (!m_FontFile)
		return E_FAIL;

	*fontFile = m_FontFile.Get();
	(*fontFile)->AddRef();

	return S_OK;
}

WooFontLoader::WooFontFileStream::WooFontFileStream(const std::vector<BYTE>& fontData)
	: m_FontData(fontData)
{

}

HRESULT __stdcall WooFontLoader::WooFontFileStream::QueryInterface(REFIID riid, void** ppvObject)
{
	if (riid == __uuidof(IDWriteFontFileStream) || riid == __uuidof(IUnknown))
		*ppvObject = static_cast<IDWriteFontFileStream*>(this);
	else
	{
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}

ULONG __stdcall WooFontLoader::WooFontFileStream::AddRef(void)
{
	return InterlockedIncrement(&m_refCount);
}

ULONG __stdcall WooFontLoader::WooFontFileStream::Release(void)
{
	ULONG newCount = InterlockedDecrement(&m_refCount);
	if (newCount == 0)
		delete this;
	return newCount;
}

HRESULT __stdcall WooFontLoader::WooFontFileStream::ReadFileFragment(void const** fragmentStart, UINT64 fileOffset, UINT64 fragmentSize, void** fragmentContext)
{
	if (fileOffset + fragmentSize > m_FontData.size())
		return E_FAIL;

	*fragmentStart = m_FontData.data() + fileOffset;
	*fragmentContext = nullptr;
	return S_OK;
}

void __stdcall WooFontLoader::WooFontFileStream::ReleaseFileFragment(void* fragmentContext)
{
	// No dynamic memory to release as the fragment points directly to the internal vector memory.
}

HRESULT __stdcall WooFontLoader::WooFontFileStream::GetFileSize(UINT64* fileSize)
{
	*fileSize = m_FontData.size();// Return the size of the in-memory font data
	return S_OK;
}

HRESULT __stdcall WooFontLoader::WooFontFileStream::GetLastWriteTime(UINT64* lastWriteTime)
{
	*lastWriteTime = 0;// In-memory fonts do not have a last write time
	return S_OK;
}
