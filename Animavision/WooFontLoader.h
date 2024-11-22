#pragma once

#include "DX11Relatives.h"

class WooFontLoader : public virtual IDWriteFontCollectionLoader, public virtual IDWriteFontFileLoader
{
public:
	// IDWriteFontCollectionLoader을(를) 통해 상속됨
	HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override;
	ULONG __stdcall AddRef(void) override;
	ULONG __stdcall Release(void) override;
	HRESULT __stdcall CreateEnumeratorFromKey(IDWriteFactory* factory, void const* collectionKey, UINT32 collectionKeySize, IDWriteFontFileEnumerator** fontFileEnumerator) override;

	// IDWriteFontFileLoader을(를) 통해 상속됨
	HRESULT __stdcall CreateStreamFromKey(void const* fontFileReferenceKey, UINT32 fontFileReferenceKeySize, IDWriteFontFileStream** fontFileStream) override;

	ULONG m_RefCount = 0;

	struct FontData
	{
		std::vector<BYTE> data;
	};

	class WooFontFileStream : public IDWriteFontFileStream
	{
	public:
		WooFontFileStream(const std::vector<BYTE>& fontData);

		// IDWriteFontFileStream을(를) 통해 상속됨
		HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override;
		ULONG __stdcall AddRef(void) override;
		ULONG __stdcall Release(void) override;
		HRESULT __stdcall ReadFileFragment(void const** fragmentStart, UINT64 fileOffset, UINT64 fragmentSize, void** fragmentContext) override;
		void __stdcall ReleaseFileFragment(void* fragmentContext) override;
		HRESULT __stdcall GetFileSize(UINT64* fileSize) override;
		HRESULT __stdcall GetLastWriteTime(UINT64* lastWriteTime) override;

		ULONG m_refCount = 1;
		std::vector<BYTE> m_FontData;
	};

	class WooFontFileEnumerator : public IDWriteFontFileEnumerator
	{
	public:
		WooFontFileEnumerator(const FontData* fontData, IDWriteFactory* factory, IDWriteFontFileLoader* fontFileLoader);

		// IDWriteFontFileEnumerator을(를) 통해 상속됨
		HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override;
		ULONG __stdcall AddRef(void) override;
		ULONG __stdcall Release(void) override;
		HRESULT __stdcall MoveNext(BOOL* hasCurrentFile) override;
		HRESULT __stdcall GetCurrentFontFile(IDWriteFontFile** fontFile) override;

		ULONG m_refCount = 1;
		Microsoft::WRL::ComPtr<IDWriteFontFile> m_FontFile = nullptr;
		Microsoft::WRL::ComPtr<IDWriteFactory> m_Factory = nullptr;
		FontData m_FontData = {};
		Microsoft::WRL::ComPtr<IDWriteFontFileLoader> m_FontFileLoader = nullptr;
	};
};

struct Font
{
	std::string fontPath;
	std::wstring fontName;
	float fontSize = 32.0f;
	Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat = nullptr;
	Microsoft::WRL::ComPtr<IDWriteFontCollection> fontCollection = nullptr;
	Microsoft::WRL::ComPtr<IDWriteFontFamily> fontFamily = nullptr;
	Microsoft::WRL::ComPtr<IDWriteFontFile> fontFile = nullptr;
	Microsoft::WRL::ComPtr<IDWriteFontFace> fontFace = nullptr;
	Microsoft::WRL::ComPtr<IDWriteLocalizedStrings> localizedString = nullptr;
};
