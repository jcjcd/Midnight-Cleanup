#include "pch.h"
#include "FontRenderer.h"

FontRenderer::FontRenderer(ID2D1Factory* pD2DFactory, ID2D1RenderTarget* pRT, ID2D1SolidColorBrush* pOutlineBrush, ID2D1SolidColorBrush* pFillBrush)
	: m_D2DFactory(pD2DFactory),
	m_RT(pRT),
	m_OutlineBrush(pOutlineBrush),
	m_FillBrush(pFillBrush)
{
	if (m_D2DFactory)
		m_D2DFactory->AddRef();
	if (m_RT)
		m_RT->AddRef();
	if (m_OutlineBrush)
		m_OutlineBrush->AddRef();
	if (m_FillBrush)
		m_FillBrush->AddRef();
}

FontRenderer::~FontRenderer()
{
	//SafeRelease(&m_D2DFactory);
	SafeRelease(&m_RT);
	SafeRelease(&m_OutlineBrush);
	SafeRelease(&m_FillBrush);
}

HRESULT __stdcall FontRenderer::QueryInterface(REFIID riid, void** ppvObject)
{
	if (__uuidof(IDWriteTextRenderer) == riid)
	{
		*ppvObject = this;
	}
	else if (__uuidof(IDWritePixelSnapping) == riid)
	{
		*ppvObject = this;
	}
	else if (__uuidof(IUnknown) == riid)
	{
		*ppvObject = this;
	}
	else
	{
		*ppvObject = NULL;
		return E_FAIL;
	}

	this->AddRef();

	return S_OK;
}

ULONG __stdcall FontRenderer::AddRef(void)
{
	return InterlockedIncrement(&m_RefCount);
}

ULONG __stdcall FontRenderer::Release(void)
{
	unsigned long newCount = InterlockedDecrement(&m_RefCount);
	if (newCount == 0)
	{
		delete this;
		return 0;
	}
	return newCount;
}

HRESULT FontRenderer::IsPixelSnappingDisabled(void* clientDrawingContext, BOOL* isDisabled) noexcept
{
	*isDisabled = false;
	return S_OK;
}

HRESULT FontRenderer::GetCurrentTransform(void* clientDrawingContext, DWRITE_MATRIX* transform) noexcept
{
	//forward the render target's transform
	m_RT->GetTransform(reinterpret_cast<D2D1_MATRIX_3X2_F*>(transform));
	return S_OK;
}

HRESULT FontRenderer::GetPixelsPerDip(void* clientDrawingContext, FLOAT* pixelsPerDip) noexcept
{
	*pixelsPerDip = 1.0f;
	return S_OK;
}

HRESULT FontRenderer::DrawGlyphRun(void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY, DWRITE_MEASURING_MODE measuringMode, const DWRITE_GLYPH_RUN* glyphRun, const DWRITE_GLYPH_RUN_DESCRIPTION* glyphRunDescription, IUnknown* clientDrawingEffect) noexcept
{
	HRESULT hr = S_OK;

	// Create the path geometry.
	ID2D1PathGeometry* pPathGeometry = NULL;
	hr = m_D2DFactory->CreatePathGeometry(
		&pPathGeometry
	);

	// Write to the path geometry using the geometry sink.
	ID2D1GeometrySink* pSink = NULL;
	if (SUCCEEDED(hr))
	{
		hr = pPathGeometry->Open(
			&pSink
		);
	}

	// Get the glyph run outline geometries back from DirectWrite and place them within the
	// geometry sink.
	if (SUCCEEDED(hr))
	{
		hr = glyphRun->fontFace->GetGlyphRunOutline(
			glyphRun->fontEmSize,
			glyphRun->glyphIndices,
			glyphRun->glyphAdvances,
			glyphRun->glyphOffsets,
			glyphRun->glyphCount,
			glyphRun->isSideways,
			glyphRun->bidiLevel % 2,
			pSink
		);
	}

	// Close the geometry sink
	if (SUCCEEDED(hr))
	{
		hr = pSink->Close();
	}

	// Initialize a matrix to translate the origin of the glyph run.
	D2D1::Matrix3x2F const matrix = D2D1::Matrix3x2F(
		1.0f, 0.0f,
		0.0f, 1.0f,
		baselineOriginX, baselineOriginY
	);

	// Create the transformed geometry
	ID2D1TransformedGeometry* pTransformedGeometry = NULL;
	if (SUCCEEDED(hr))
	{
		hr = m_D2DFactory->CreateTransformedGeometry(
			pPathGeometry,
			&matrix,
			&pTransformedGeometry
		);
	}

	// Draw the outline of the glyph run
	m_RT->DrawGeometry(
		pTransformedGeometry,
		m_OutlineBrush
	);

	// Fill in the glyph run
	m_RT->FillGeometry(
		pTransformedGeometry,
		m_FillBrush
	);

	SafeRelease(&pPathGeometry);
	SafeRelease(&pSink);
	SafeRelease(&pTransformedGeometry);

	return hr;
}

HRESULT FontRenderer::DrawUnderline(void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY, const DWRITE_UNDERLINE* underline, IUnknown* clientDrawingEffect) noexcept
{
	HRESULT hr;

	D2D1_RECT_F rect = D2D1::RectF(
		0,
		underline->offset,
		underline->width,
		underline->offset + underline->thickness
	);

	ID2D1RectangleGeometry* pRectangleGeometry = NULL;
	hr = m_D2DFactory->CreateRectangleGeometry(
		&rect,
		&pRectangleGeometry
	);

	// Initialize a matrix to translate the origin of the underline
	D2D1::Matrix3x2F const matrix = D2D1::Matrix3x2F(
		1.0f, 0.0f,
		0.0f, 1.0f,
		baselineOriginX, baselineOriginY
	);

	ID2D1TransformedGeometry* pTransformedGeometry = NULL;
	if (SUCCEEDED(hr))
	{
		hr = m_D2DFactory->CreateTransformedGeometry(
			pRectangleGeometry,
			&matrix,
			&pTransformedGeometry
		);
	}

	// Draw the outline of the rectangle
	m_RT->DrawGeometry(
		pTransformedGeometry,
		m_OutlineBrush
	);

	// Fill in the rectangle
	m_RT->FillGeometry(
		pTransformedGeometry,
		m_FillBrush
	);

	SafeRelease(&pRectangleGeometry);
	SafeRelease(&pTransformedGeometry);

	return S_OK;
}

HRESULT FontRenderer::DrawStrikethrough(void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY, const DWRITE_STRIKETHROUGH* strikethrough, IUnknown* clientDrawingEffect) noexcept
{
	HRESULT hr;

	D2D1_RECT_F rect = D2D1::RectF(
		0,
		strikethrough->offset,
		strikethrough->width,
		strikethrough->offset + strikethrough->thickness
	);

	ID2D1RectangleGeometry* pRectangleGeometry = NULL;
	hr = m_D2DFactory->CreateRectangleGeometry(
		&rect,
		&pRectangleGeometry
	);

	// Initialize a matrix to translate the origin of the strikethrough
	D2D1::Matrix3x2F const matrix = D2D1::Matrix3x2F(
		1.0f, 0.0f,
		0.0f, 1.0f,
		baselineOriginX, baselineOriginY
	);

	ID2D1TransformedGeometry* pTransformedGeometry = NULL;
	if (SUCCEEDED(hr))
	{
		hr = m_D2DFactory->CreateTransformedGeometry(
			pRectangleGeometry,
			&matrix,
			&pTransformedGeometry
		);
	}

	// Draw the outline of the rectangle
	m_RT->DrawGeometry(
		pTransformedGeometry,
		m_OutlineBrush
	);

	// Fill in the rectangle
	m_RT->FillGeometry(
		pTransformedGeometry,
		m_FillBrush
	);

	SafeRelease(&pRectangleGeometry);
	SafeRelease(&pTransformedGeometry);

	return S_OK;
}

HRESULT FontRenderer::DrawInlineObject(void* clientDrawingContext, FLOAT originX, FLOAT originY, IDWriteInlineObject* inlineObject, BOOL isSideways, BOOL isRightToLeft, IUnknown* clientDrawingEffect) noexcept
{
	// Not implemented
	return E_NOTIMPL;
}
