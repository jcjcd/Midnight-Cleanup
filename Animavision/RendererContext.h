#pragma once
class RendererContext
{
public:
	RendererContext() = default;
	virtual ~RendererContext() = default;

	virtual void Resize(uint32_t width, uint32_t height) = 0;

	static std::unique_ptr<RendererContext> Create(HWND hwnd, uint32_t width, uint32_t height);

	virtual uint32_t GetWidth() const { return 0; }
	virtual uint32_t GetHeight() const { return 0; }
};

