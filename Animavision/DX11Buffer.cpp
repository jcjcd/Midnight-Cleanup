#include "pch.h"
#include "DX11Buffer.h"
#include "Mesh.h"
#include "DX11Shader.h"
#include "NeoDX11Context.h"

#pragma region Deprecated
//DX11VertexBuffer::DX11VertexBuffer(DX11Context& context, const std::vector<Vector3>& vertices, uint32_t slot /*= 0*/, bool doesCPUWrite /*= false*/, bool doesGPUWrite /*= false*/)
//	: m_Slot(slot), m_DoesCPUWrite(doesCPUWrite), m_DoesGPUWrite(doesGPUWrite)
//{
//	m_Stride = sizeof(Vector3);
//	m_Count = static_cast<uint32_t>(vertices.size());
//
//	D3D11_BUFFER_DESC desc = {};
//	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
//	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
//	desc.ByteWidth = m_Stride * m_Count;
//
//	if (doesCPUWrite == false && doesGPUWrite == false)
//	{
//		desc.Usage = D3D11_USAGE_IMMUTABLE; // CPU Read, GPU Read
//	}
//	else if (doesCPUWrite == true && doesGPUWrite == false)
//	{
//		desc.Usage = D3D11_USAGE_DYNAMIC; // CPU Write, GPU Read
//		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
//	}
//	else if (doesCPUWrite == false && doesGPUWrite == true)
//	{
//		desc.Usage = D3D11_USAGE_DEFAULT; // CPU Read, GPU Write
//	}
//	else
//	{
//		desc.Usage = D3D11_USAGE_STAGING; // CPU Write, GPU Read
//		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
//	}
//
//	D3D11_SUBRESOURCE_DATA data = {};
//	ZeroMemory(&data, sizeof(D3D11_SUBRESOURCE_DATA));
//	data.pSysMem = vertices.data();
//
//	HRESULT hr = GetDevice(context)->CreateBuffer(&desc, &data, m_Buffer.put());
//	assert(SUCCEEDED(hr));
//}
//
//void DX11VertexBuffer::Bind(RendererContext* context)
//{
//	assert(static_cast<DX11Context*>(context) != nullptr);
//
//	DX11Context* dx11Context = static_cast<DX11Context*>(context);
//	ID3D11Buffer* Buffer = m_Buffer.get();
//	GetDeviceContext(*dx11Context)->IASetVertexBuffers(m_Slot, 1, &Buffer, &m_Stride, &m_Offset);
//}
//
//DX11IndexBuffer::DX11IndexBuffer(DX11Context& context, const std::vector<uint32_t>& indices)
//{
//	m_Stride = sizeof(uint32_t);
//	m_Count = static_cast<uint32_t>(indices.size());
//
//	D3D11_BUFFER_DESC desc = {};
//	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
//	desc.Usage = D3D11_USAGE_IMMUTABLE;
//	desc.ByteWidth = m_Stride * m_Count;
//	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
//
//	D3D11_SUBRESOURCE_DATA data = {};
//	ZeroMemory(&data, sizeof(D3D11_SUBRESOURCE_DATA));
//	data.pSysMem = indices.data();
//
//	HRESULT hr = GetDevice(context)->CreateBuffer(&desc, &data, m_Buffer.put());
//	assert(SUCCEEDED(hr));
//}
//
//void DX11IndexBuffer::Bind(RendererContext* context)
//{
//	assert(static_cast<DX11Context*>(context) != nullptr);
//
//	DX11Context* dx11Context = static_cast<DX11Context*>(context);
//	GetDeviceContext(*dx11Context)->IASetIndexBuffer(m_Buffer.get(), DXGI_FORMAT_R32_UINT, m_Offset);
//}
#pragma endregion

static uint32_t makeMultipleOf16(uint32_t num)
{
	// num이 16의 배수인지 확인
	if (num % 16 == 0) {
		return num;
	}
	// num을 16의 배수로 만들기 위해 필요한 값 계산
	uint32_t remainder = num % 16;
	uint32_t result = num + (16 - remainder);
	return result;
}

struct SkinnedVertex
{
	Vector3 position;
	Vector3 normal;
	Vector3 tangent;
	Vector2 uv;
	//Vector2 uv2;
	uint32_t boneIndices[4]{};
	float boneWeights[4]{};
	Vector3 bitangent;
};

struct Vertex
{
	Vector3 position;
	Vector3 normal;
	Vector3 tangent;
	Vector2 uv;
	Vector2 uv2;
	Vector3 bitangent;
};

std::shared_ptr<DX11Buffer> DX11Buffer::Create(RendererContext* context, Mesh* mesh, Usage usage)
{
	switch (usage)
	{
		case Buffer::Usage::Vertex:
			return DX11VertexBuffer::Create(context, mesh);
			break;
		case Buffer::Usage::Index:
			return DX11IndexBuffer::Create(context, mesh);
			break;
		default:
			assert(false && "invalid buffer usage");
			return nullptr;
			break;
	}
}

DX11VertexBuffer::DX11VertexBuffer(RendererContext* context, Mesh* mesh)
	: DX11VertexBuffer(context, mesh, 0, false, false)
{

}

DX11VertexBuffer::DX11VertexBuffer(RendererContext* context, Mesh* mesh, uint32_t slot /*= 0*/, bool doesCPUWrite /*= false*/, bool doesGPUWrite /*= false*/)
	: m_Slot(slot), m_DoesCPUWrite(doesCPUWrite), m_DoesGPUWrite(doesGPUWrite)
{
	//assert(dynamic_cast<NeoDX11Context*>(context) != nullptr && "invalid context");
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	if (mesh->boneWeights.size() > 0)
		m_Stride = sizeof(SkinnedVertex);
	else
		m_Stride = sizeof(Vertex);

	m_Count = static_cast<uint32_t>(mesh->vertices.size());

	D3D11_BUFFER_DESC desc = {};
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.ByteWidth = m_Stride * m_Count;

	if (doesCPUWrite == false && doesGPUWrite == false)
	{
		desc.Usage = D3D11_USAGE_IMMUTABLE; // CPU Read, GPU Read
	}
	else if (doesCPUWrite == true && doesGPUWrite == false)
	{
		desc.Usage = D3D11_USAGE_DYNAMIC; // CPU Write, GPU Read
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else if (doesCPUWrite == false && doesGPUWrite == true)
	{
		desc.Usage = D3D11_USAGE_DEFAULT; // CPU Read, GPU Write
	}
	else
	{
		desc.Usage = D3D11_USAGE_STAGING; // CPU Write, GPU Read
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
	}

	if (mesh->boneWeights.size() > 0)
	{
		std::vector<SkinnedVertex> testVertices;
		testVertices.reserve(mesh->vertices.size());
		for (int i = 0; i < mesh->vertices.size(); i++)
		{
			SkinnedVertex vertex;
			vertex.position = mesh->vertices[i];
			vertex.uv = mesh->uv.size() > 0 ? mesh->uv[i] : Vector2::Zero;
			//vertex.uv2 = mesh->uv2.size() > 0 ? mesh->uv2[i] : Vector2::Zero;
			vertex.normal = mesh->normals.size() > 0 ? mesh->normals[i] : Vector3::Zero;
			vertex.tangent = mesh->tangents.size() > 0 ? mesh->tangents[i] : Vector3::Zero;
			vertex.bitangent = mesh->bitangents.size() > 0 ? mesh->bitangents[i] : Vector3::Zero;

			if (mesh->boneWeights.size() > 0)
			{
				memcpy(&vertex.boneIndices, mesh->boneWeights[i].boneIndex, sizeof(uint32_t) * 4);
				memcpy(&vertex.boneWeights, mesh->boneWeights[i].weight, sizeof(float) * 4);
			}

			testVertices.emplace_back(vertex);
		}

		D3D11_SUBRESOURCE_DATA data = {};
		ZeroMemory(&data, sizeof(D3D11_SUBRESOURCE_DATA));
		data.pSysMem = testVertices.data();

		HRESULT hr = dx11Context->GetDevice()->CreateBuffer(&desc, &data, m_Buffer.GetAddressOf());
		assert(SUCCEEDED(hr));
	}
	else
	{
		std::vector<Vertex> testVertices;
		testVertices.reserve(mesh->vertices.size());
		for (int i = 0; i < mesh->vertices.size(); i++)
		{
			Vertex vertex;
			vertex.position = mesh->vertices[i];
			vertex.uv = mesh->uv.size() > 0 ? mesh->uv[i] : Vector2::Zero;
			vertex.uv2 = mesh->uv2.size() > 0 ? mesh->uv2[i] : Vector2::Zero;
			vertex.normal = mesh->normals.size() > 0 ? mesh->normals[i] : Vector3::Zero;
			vertex.tangent = mesh->tangents.size() > 0 ? mesh->tangents[i] : Vector3::Zero;
			vertex.bitangent = mesh->bitangents.size() > 0 ? mesh->bitangents[i] : Vector3::Zero;

			testVertices.emplace_back(vertex);
		}

		D3D11_SUBRESOURCE_DATA data = {};
		ZeroMemory(&data, sizeof(D3D11_SUBRESOURCE_DATA));
		data.pSysMem = testVertices.data();

		HRESULT hr = dx11Context->GetDevice()->CreateBuffer(&desc, &data, m_Buffer.GetAddressOf());
		assert(SUCCEEDED(hr));
	}

}

DX11VertexBuffer::~DX11VertexBuffer()
{
	if (m_Buffer)
		m_Buffer.Reset();
}

void DX11VertexBuffer::Bind(RendererContext* context)
{
	//assert(dynamic_cast<NeoDX11Context*>(context) != nullptr && "invalid context");
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);
	dx11Context->GetDeviceContext()->IASetVertexBuffers(m_Slot, 1, m_Buffer.GetAddressOf(), &m_Stride, &m_Offset);
}

std::shared_ptr<DX11VertexBuffer> DX11VertexBuffer::Create(RendererContext* context, Mesh* mesh, uint32_t slot /*= 0*/, bool doesCPUWrite /*= false*/, bool doesGPUWrite /*= false*/)
{
	return std::make_shared<DX11VertexBuffer>(context, mesh, slot, doesCPUWrite, doesGPUWrite);
}

DX11IndexBuffer::DX11IndexBuffer(RendererContext* context, Mesh* mesh)
{
	//assert(dynamic_cast<NeoDX11Context*>(context) != nullptr && "invalid context");
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);
	m_Stride = sizeof(uint32_t);
	m_Count = static_cast<uint32_t>(mesh->indices.size());

	D3D11_BUFFER_DESC desc = {};
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.ByteWidth = m_Stride * m_Count;
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA data = {};
	ZeroMemory(&data, sizeof(D3D11_SUBRESOURCE_DATA));
	data.pSysMem = mesh->indices.data();

	HRESULT hr = dx11Context->GetDevice()->CreateBuffer(&desc, &data, m_Buffer.GetAddressOf());
	assert(SUCCEEDED(hr));
}

DX11IndexBuffer::~DX11IndexBuffer()
{
	if (m_Buffer)
		m_Buffer.Reset();
}

void DX11IndexBuffer::Bind(RendererContext* context)
{
	//assert(dynamic_cast<NeoDX11Context*>(context) != nullptr && "invalid context");
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);
	dx11Context->GetDeviceContext()->IASetIndexBuffer(m_Buffer.Get(), DXGI_FORMAT_R32_UINT, m_Offset);
}

std::shared_ptr<DX11IndexBuffer> DX11IndexBuffer::Create(RendererContext* context, Mesh* mesh)
{
	return std::make_shared<DX11IndexBuffer>(context, mesh);
}

DX11ConstantBuffer::DX11ConstantBuffer(RendererContext* context, uint32_t byteWidth)
	: m_ByteWidth(byteWidth)
{
	//assert(dynamic_cast<NeoDX11Context*>(context) != nullptr && "invalid context");
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	D3D11_BUFFER_DESC desc = {};
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = makeMultipleOf16(byteWidth);
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	HRESULT hr = dx11Context->GetDevice()->CreateBuffer(&desc, nullptr, m_Buffer.GetAddressOf());

	/*hr = dx11Context->GetContext()->Map(m_Buffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &m_MappedResource);
	m_IsMapped = true;*/

	assert(SUCCEEDED(hr));
}

DX11ConstantBuffer::~DX11ConstantBuffer()
{
	if (m_Buffer)
		m_Buffer.Reset();
}

void DX11ConstantBuffer::BindConstantBuffer(RendererContext* context, uint32_t shaderType, uint32_t slot)
{
	//assert(dynamic_cast<NeoDX11Context*>(context) != nullptr && "invalid context");
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	switch (shaderType)
	{
		case static_cast<uint32_t>(ShaderType::Vertex):
			dx11Context->GetDeviceContext()->VSSetConstantBuffers(slot, 1, m_Buffer.GetAddressOf());
			break;
		case static_cast<uint32_t>(ShaderType::Pixel):
			dx11Context->GetDeviceContext()->PSSetConstantBuffers(slot, 1, m_Buffer.GetAddressOf());
			break;
		case static_cast<uint32_t>(ShaderType::Compute):
			dx11Context->GetDeviceContext()->CSSetConstantBuffers(slot, 1, m_Buffer.GetAddressOf());
			break;
		case static_cast<uint32_t>(ShaderType::Geometry):
		case static_cast<uint32_t>(ShaderType::StreamOutGeometry):
			dx11Context->GetDeviceContext()->GSSetConstantBuffers(slot, 1, m_Buffer.GetAddressOf());
			break;
		case static_cast<uint32_t>(ShaderType::Hull):
			dx11Context->GetDeviceContext()->HSSetConstantBuffers(slot, 1, m_Buffer.GetAddressOf());
			break;
		case static_cast<uint32_t>(ShaderType::Domain):
			dx11Context->GetDeviceContext()->DSSetConstantBuffers(slot, 1, m_Buffer.GetAddressOf());
			break;
		case static_cast<uint32_t>(ShaderType::Count):
		default:
			assert(false && "invalid shader type");
			break;
	}
}

void DX11ConstantBuffer::UnbindConstantBuffer(RendererContext* context, uint32_t shaderType, uint32_t slot)
{
	//assert(dynamic_cast<NeoDX11Context*>(context) != nullptr && "invalid context");
	NeoDX11Context* dx11Context = static_cast<NeoDX11Context*>(context);

	ID3D11Buffer* nullBuffers[] = { nullptr };

	switch (shaderType)
	{
		case static_cast<uint32_t>(ShaderType::Vertex):
			dx11Context->GetDeviceContext()->VSSetConstantBuffers(slot, 1, nullBuffers);
			break;
		case static_cast<uint32_t>(ShaderType::Pixel):
			dx11Context->GetDeviceContext()->PSSetConstantBuffers(slot, 1, nullBuffers);
			break;
		case static_cast<uint32_t>(ShaderType::Compute):
			dx11Context->GetDeviceContext()->CSSetConstantBuffers(slot, 1, nullBuffers);
			break;
		case static_cast<uint32_t>(ShaderType::Geometry):
		case static_cast<uint32_t>(ShaderType::StreamOutGeometry):
			dx11Context->GetDeviceContext()->GSSetConstantBuffers(slot, 1, nullBuffers);
			break;
		case static_cast<uint32_t>(ShaderType::Hull):
			dx11Context->GetDeviceContext()->HSSetConstantBuffers(slot, 1, nullBuffers);
			break;
		case static_cast<uint32_t>(ShaderType::Domain):
			dx11Context->GetDeviceContext()->DSSetConstantBuffers(slot, 1, nullBuffers);
			break;
		case static_cast<uint32_t>(ShaderType::Count):
		default:
			assert(false && "invalid shader type");
			break;
	}
}

std::shared_ptr<DX11ConstantBuffer> DX11ConstantBuffer::Create(RendererContext* context, uint32_t size)
{
	return std::make_shared<DX11ConstantBuffer>(context, size);
}
