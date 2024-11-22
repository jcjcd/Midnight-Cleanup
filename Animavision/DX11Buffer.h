#pragma once

#include "ShaderResource.h"

#include "DX11Relatives.h"
#include "DX11Shader.h"


class DX11Buffer : public Buffer
{
public:
	// Buffer을(를) 통해 상속됨
	void Bind(RendererContext* context) abstract;

	static std::shared_ptr<DX11Buffer> Create(RendererContext* context, Mesh* mesh, Usage usage);

	uint32_t GetStride() const { return m_Stride; }
	uint32_t GetCount() const { return m_Count; }
	uint32_t GetOffset() const { return m_Offset; }
	ID3D11Buffer* GetBuffer() const { return m_Buffer.Get(); }
	ID3D11Buffer** GetBufferAddress() { return m_Buffer.GetAddressOf(); }

protected:
	uint32_t m_Stride = 0;
	uint32_t m_Count = 0;
	uint32_t m_Offset = 0;

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_Buffer = nullptr;
	Usage m_Usage = Usage::Vertex;

};

class DX11VertexBuffer : public DX11Buffer
{ 
public:
	DX11VertexBuffer() = default;
	DX11VertexBuffer(RendererContext* context, Mesh* mesh);
	DX11VertexBuffer(RendererContext* context, Mesh* mesh, uint32_t slot = 0, bool doesCPUWrite = false, bool doesGPUWrite = false);
	virtual ~DX11VertexBuffer();

	// DX11Buffer을(를) 통해 상속됨
	void Bind(RendererContext* context) override;

	static std::shared_ptr<DX11VertexBuffer> Create(RendererContext* context, Mesh* mesh, uint32_t slot = 0, bool doesCPUWrite = false, bool doesGPUWrite = false);

private:
	uint32_t m_Slot = 0;
	bool m_DoesCPUWrite = false;
	bool m_DoesGPUWrite = false;

};

class DX11IndexBuffer : public DX11Buffer
{
public:
	DX11IndexBuffer(RendererContext* context, Mesh* mesh);
	virtual ~DX11IndexBuffer();

	// DX11Buffer을(를) 통해 상속됨
	void Bind(RendererContext* context) override;

	static std::shared_ptr<DX11IndexBuffer> Create(RendererContext* context, Mesh* mesh);
};

class DX11ConstantBuffer : public DX11Buffer
{
	friend class DX11Shader;

public:
	DX11ConstantBuffer(RendererContext* context, uint32_t byteWidth);
	~DX11ConstantBuffer();

	// DX11Buffer을(를) 통해 상속됨
	// 상속을 해치는 구조, 상수버퍼를 bind하는 함수는 shader에 종속되어 있음
	// 일단 빈 함수로 두고 밑에 함수를 사용하도록 함
	void Bind(RendererContext* context) override {}
	void BindConstantBuffer(RendererContext* context, uint32_t shaderType, uint32_t slot);
	void UnbindConstantBuffer(RendererContext* context, uint32_t shaderType, uint32_t slot);

	static std::shared_ptr<DX11ConstantBuffer> Create(RendererContext* context, uint32_t byteWidth);

private:
	ShaderType m_ShaderType = ShaderType::Count;
	uint32_t m_ParameterIndex = 0;
	uint32_t m_Register = 0;
	uint32_t m_ByteWidth = 0;
	bool m_IsDirty = false;
	D3D11_MAPPED_SUBRESOURCE m_MappedResource = {};
	bool m_IsMapped = false;
};


#pragma region Deprecated
//class DX11VertexBuffer : public DX11Buffer
//{
//public:
//	DX11VertexBuffer(DX11Context& context, const std::vector<Vector3>& vertices, uint32_t slot = 0, bool doesCPUWrite = false, bool doesGPUWrite = false);
//
//	// Buffer을(를) 통해 상속됨
//	void Bind(RendererContext* context) override;
//
//	uint32_t GetSlot() const { return m_Slot; }
//private:
//	uint32_t m_Slot = 0;
//	bool m_DoesCPUWrite = false;
//	bool m_DoesGPUWrite = false;
//};
//
//class DX11IndexBuffer : public DX11Buffer
//{
//public:
//	DX11IndexBuffer(DX11Context& context, const std::vector<uint32_t>& indices);
//
//	// Buffer을(를) 통해 상속됨
//	void Bind(RendererContext* context) override;
//};
#pragma endregion