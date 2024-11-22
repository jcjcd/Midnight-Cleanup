#include "pch.h"
#include "DX12Texture.h"
#include "DX12Context.h"
#include "ChangDXII.h"


std::shared_ptr<DX12Texture> DX12Texture::Create(Renderer* renderer, const std::string& path)
{
	ChangDXII* changDxii = static_cast<ChangDXII*>(renderer);
	return static_pointer_cast<DX12Texture>(changDxii->CreateTexture(path.c_str()));
}

std::shared_ptr<DX12Texture> DX12Texture::CreateCube(Renderer* renderer, const std::string& path)
{
	ChangDXII* changDxii = static_cast<ChangDXII*>(renderer);
	return static_pointer_cast<DX12Texture>(changDxii->CreateTextureCube(path.c_str()));
}

void DX12Texture::ResourceBarrier(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES state)
{
	if (m_UsageState != state)
	{
		D3D12_RESOURCE_BARRIER barrierDesc = {};
		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Transition.pResource = m_Texture.get();
		barrierDesc.Transition.StateBefore = m_UsageState;
		barrierDesc.Transition.StateAfter = state;
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		commandList->ResourceBarrier(1, &barrierDesc);

		m_UsageState = state;
	}
}


