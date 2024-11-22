#pragma once

#include "DX11Relatives.h"

class FontRenderer : public IDWriteTextRenderer
{
public:
	FontRenderer(ID2D1Factory* pD2DFactory,
		ID2D1RenderTarget* pRT,          // 여기서 ID2D1RenderTarget으로 변경
		ID2D1SolidColorBrush* pOutlineBrush,
		ID2D1SolidColorBrush* pFillBrush);

	~FontRenderer();

	// IDWriteTextRenderer을(를) 통해 상속됨
	HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override;
	ULONG __stdcall AddRef(void) override;
	ULONG __stdcall Release(void) override;
	HRESULT IsPixelSnappingDisabled(void* clientDrawingContext, BOOL* isDisabled) noexcept override;
	HRESULT GetCurrentTransform(void* clientDrawingContext, DWRITE_MATRIX* transform) noexcept override;
	HRESULT GetPixelsPerDip(void* clientDrawingContext, FLOAT* pixelsPerDip) noexcept override;
	HRESULT DrawGlyphRun(void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY, DWRITE_MEASURING_MODE measuringMode, const DWRITE_GLYPH_RUN* glyphRun, const DWRITE_GLYPH_RUN_DESCRIPTION* glyphRunDescription, IUnknown* clientDrawingEffect) noexcept override;
	HRESULT DrawUnderline(void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY, const DWRITE_UNDERLINE* underline, IUnknown* clientDrawingEffect) noexcept override;
	HRESULT DrawStrikethrough(void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY, const DWRITE_STRIKETHROUGH* strikethrough, IUnknown* clientDrawingEffect) noexcept override;
	HRESULT DrawInlineObject(void* clientDrawingContext, FLOAT originX, FLOAT originY, IDWriteInlineObject* inlineObject, BOOL isSideways, BOOL isRightToLeft, IUnknown* clientDrawingEffect) noexcept override;

private:
	unsigned long m_RefCount = 1;
	ID2D1Factory* m_D2DFactory = nullptr;
	ID2D1RenderTarget* m_RT = nullptr;            // ID2D1RenderTarget으로 변경
	ID2D1SolidColorBrush* m_OutlineBrush = nullptr;
	ID2D1SolidColorBrush* m_FillBrush = nullptr;
};

