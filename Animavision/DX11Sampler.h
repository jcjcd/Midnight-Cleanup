#pragma once

#include "ShaderResource.h"
#include "DX11Shader.h"

#include "DX11Relatives.h"

class DX11Sampler : public Sampler
{
public:
	/// ���� ����ϴ� shader���� �̸��� �̰� ���ԵǾ� ���� ������ ������ �߻��Ѵ�.
	enum class SamplerType
	{
		None,
		Point,
		Linear,
		Anisotropic,
		Shadow,
		SSAO,
	};

	DX11Sampler(RendererContext* context, bool isReflect, DX11ResourceBinding samplerBinding);
	DX11Sampler(RendererContext* context, D3D11_TEXTURE_ADDRESS_MODE addressMode, DX11ResourceBinding samplerBinding);
	virtual ~DX11Sampler();

	// Sampler��(��) ���� ��ӵ�
	void Bind(RendererContext* context) override;
	ID3D11SamplerState* GetSamplerState() const { return m_SamplerState.Get(); }

	void SetShaderType(ShaderType type) { m_ShaderType = type; }

	uint32_t GetSlot() const { return m_Slot; }
	void SetSlot(uint32_t slot) { m_Slot = slot; }

private:
	Microsoft::WRL::ComPtr<ID3D11SamplerState> m_SamplerState = nullptr;
	SamplerType m_Type = SamplerType::None;
	ShaderType m_ShaderType = ShaderType::Count;
	uint32_t m_Slot = 0;
	bool m_IsReflect = false;
	D3D11_SAMPLER_DESC m_SamplerDesc = {};
	D3D11_TEXTURE_ADDRESS_MODE m_AddressMode = D3D11_TEXTURE_ADDRESS_WRAP;
	std::string m_Name = "";
};

