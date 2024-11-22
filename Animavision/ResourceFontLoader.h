#pragma once

#include "DX11Relatives.h"

struct Font
{
	std::string fontPath;
	std::wstring fontName;
	float fontSize = 32.0f;
	Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat = nullptr;
	Microsoft::WRL::ComPtr<IDWriteFontCollection> fontCollection = nullptr;
	Microsoft::WRL::ComPtr<IDWriteFontFamily> fontFamily = nullptr;
	Microsoft::WRL::ComPtr<IDWriteFontFile> fontFile = nullptr;
	Microsoft::WRL::ComPtr<IDWriteFont> font = nullptr;
	Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout = nullptr;
	Microsoft::WRL::ComPtr<IDWriteFontFace> fontFace = nullptr;
	Microsoft::WRL::ComPtr<IDWriteLocalizedStrings> localizedString = nullptr;
	Microsoft::WRL::ComPtr<ID2D1PathGeometry1> pathGeometry = nullptr;
	Microsoft::WRL::ComPtr<ID2D1GeometrySink> geometrySink = nullptr;
};

class ResourceFontFileStream : public IDWriteFontFileStream
{
public:
	ResourceFontFileStream(const std::wstring& filePath);

	// IDWriteFontFileStream을(를) 통해 상속됨
	HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override;
	ULONG __stdcall AddRef(void) override;
	ULONG __stdcall Release(void) override;
	HRESULT ReadFileFragment(const void** fragmentStart, UINT64 fileOffset, UINT64 fragmentSize, void** fragmentContext) noexcept override;
	void ReleaseFileFragment(void* fragmentContext) noexcept override;
	HRESULT GetFileSize(UINT64* fileSize) noexcept override;
	HRESULT GetLastWriteTime(UINT64* lastWriteTime) noexcept override;

	bool IsInitialized() { return !m_FontData.empty(); }

private:
	ULONG m_RefCount = 1;
	std::vector<BYTE> m_FontData;
};

class ResourceFontFileLoader : public IDWriteFontFileLoader
{
public:
	// IDWriteFontFileLoader을(를) 통해 상속됨
	HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override;
	ULONG __stdcall AddRef(void) override;
	ULONG __stdcall Release(void) override;
	HRESULT CreateStreamFromKey(const void* fontFileReferenceKey, UINT32 fontFileReferenceKeySize, IDWriteFontFileStream** fontFileStream) noexcept override;

	static IDWriteFontFileLoader* GetLoader() { return m_Instance; }

	static bool IsLoaderInitialized() { return m_Instance != nullptr; }

private:
	ULONG m_RefCount = 1;

	static IDWriteFontFileLoader* m_Instance;
};

class ResourceFontFileEnumerator : public IDWriteFontFileEnumerator
{
public:
	ResourceFontFileEnumerator(IDWriteFactory* factory);

	//HRESULT Initialize(uint32_t const* resourceIDs, uint32_t resourceCount);
	HRESULT Initialize(const std::wstring* filePaths, uint32_t fileCount);

	~ResourceFontFileEnumerator();

	// IDWriteFontFileEnumerator을(를) 통해 상속됨
	HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override;
	ULONG __stdcall AddRef(void) override;
	ULONG __stdcall Release(void) override;
	HRESULT MoveNext(BOOL* hasCurrentFile) noexcept override;
	HRESULT GetCurrentFontFile(IDWriteFontFile** fontFile) noexcept override;

private:
	ULONG m_RefCount = 1;

	IDWriteFactory* m_Factory = nullptr;
	IDWriteFontFile* m_CurrrentFontFile = nullptr;
	std::vector<uint32_t> m_ResourceIDs;
	std::vector<std::wstring> m_FilePaths;
	size_t m_NextIndex = 0;
};

class ResourceFontCollectionLoader : public IDWriteFontCollectionLoader
{
public:
	// IDWriteFontCollectionLoader을(를) 통해 상속됨
	HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override;
	ULONG __stdcall AddRef(void) override;
	ULONG __stdcall Release(void) override;
	HRESULT CreateEnumeratorFromKey(IDWriteFactory* factory, const void* collectionKey, UINT32 collectionKeySize, IDWriteFontFileEnumerator** fontFileEnumerator) noexcept override;

	static IDWriteFontCollectionLoader* GetLoader() { return m_Instance; }

	static bool IsLoaderInitialized() { return m_Instance != nullptr; }

private:
	ULONG m_RefCount = 1;

	static IDWriteFontCollectionLoader* m_Instance;
};