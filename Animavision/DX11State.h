#pragma once

#include "DX11GraphicsResource.h"
#include "Renderer.h"

class NeoDX11Context;

class DX11RasterizerState : public DX11GraphicsResource
{
public:
	DX11RasterizerState(NeoDX11Context& context, RasterizerState state);

	void Bind(NeoDX11Context& context);
	void UnBind(NeoDX11Context& context);
	ID3D11RasterizerState* GetState() { return m_State.Get(); }

private:
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_State = nullptr;
};

class DX11DepthStencilState : public DX11GraphicsResource
{
public:
	DX11DepthStencilState(NeoDX11Context& context, DepthStencilState state);

	void Bind(NeoDX11Context& context);
	void UnBind(NeoDX11Context& context);
	ID3D11DepthStencilState* GetState() { return m_State.Get(); }

private:
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_State = nullptr;
};

class DX11BlendState : public DX11GraphicsResource
{
public:
	DX11BlendState(NeoDX11Context& context, BlendState state);

	void Bind(NeoDX11Context& context);
	ID3D11BlendState* GetState() { return m_State.Get(); }

private:
	Microsoft::WRL::ComPtr<ID3D11BlendState> m_State = nullptr;
};

class DX11PipelineState
{
public:
	DX11PipelineState() = default;

	void Initialize(std::shared_ptr<DX11RasterizerState> rasterizerOrNull, std::shared_ptr<DX11DepthStencilState> depthStencilOrNull, std::shared_ptr<DX11BlendState> blendOrNull);
	void Bind(NeoDX11Context& context);

private:
	std::shared_ptr<DX11RasterizerState> m_RasterizerState = nullptr;
	std::shared_ptr<DX11DepthStencilState> m_DepthStencilState = nullptr;
	std::shared_ptr<DX11BlendState> m_BlendState = nullptr;

};

