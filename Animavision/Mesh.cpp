#include "pch.h"
#include "Mesh.h"
#include "ShaderResource.h"
#include "Renderer.h"

void Mesh::CreateBuffers(Renderer* renderer)
{
	vertexBuffer = Buffer::Create(renderer, this, Buffer::Usage::Vertex);
	indexBuffer = Buffer::Create(renderer, this, Buffer::Usage::Index);

	if (renderer->IsRayTracing())
	{
		// blas를 추가한다.
		renderer->AddBottomLevelAS(*this, true);
	}
}

std::shared_ptr<Mesh> MeshGenerator::CreateCube(float width, float height, float depth)
{
	auto mesh = std::make_shared<Mesh>();

	mesh->vertices = {
		{ -width, -height, -depth },
		{ -width, +height, -depth },
		{ +width, +height, -depth },
		{ +width, -height, -depth },
		{ -width, -height, +depth },
		{ -width, +height, +depth },
		{ +width, +height, +depth },
		{ +width, -height, +depth }
	};

	mesh->normals = {
		{ 0.0f, 0.0f, -1.0f },
		{ 0.0f, 0.0f, -1.0f },
		{ 0.0f, 0.0f, -1.0f },
		{ 0.0f, 0.0f, -1.0f },
		{ 0.0f, 0.0f, 1.0f },
		{ 0.0f, 0.0f, 1.0f },
		{ 0.0f, 0.0f, 1.0f },
		{ 0.0f, 0.0f, 1.0f },
	};

	mesh->uv = {
		{ 0.0f, 0.0f },
		{ 0.0f, 1.0f },
		{ 1.0f, 1.0f },
		{ 1.0f, 0.0f },
		{ 0.0f, 0.0f },
		{ 0.0f, 1.0f },
		{ 1.0f, 1.0f },
		{ 1.0f, 0.0f },
	};

	mesh->tangents = {
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f },
	};

	mesh->bitangents = {
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
	};


	mesh->indices = {
		0, 1, 2, 0, 2, 3,
		4, 6, 5, 4, 7, 6,
		4, 5, 1, 4, 1, 0,
		3, 2, 6, 3, 6, 7,
		1
	};

	mesh->subMeshCount = 1;
	mesh->subMeshDescriptors.push_back({ "Cube", static_cast<uint32_t>(mesh->indices.size()), 0, static_cast<uint32_t>(mesh->vertices.size()), 0 });

	return mesh;
}

std::shared_ptr<Mesh> MeshGenerator::CreateSphere(float radius, uint32_t sliceCount, uint32_t stackCount)
{
	auto mesh = std::make_shared<Mesh>();

	mesh->vertices.push_back({ 0.0f, radius, 0.0f });
	mesh->normals.push_back({ 0.0f, 1.0f, 0.0f });
	mesh->uv.push_back({ 0.0f, 0.0f });
	mesh->tangents.push_back({ 1.0f, 0.0f, 0.0f });
	mesh->bitangents.push_back({ 0.0f, 0.0f, 1.0f });

	float phiStep = DirectX::XM_PI / stackCount;
	float thetaStep = 2.0f * DirectX::XM_PI / sliceCount;

	for (uint32_t i = 1; i <= stackCount - 1; ++i)
	{
		float phi = i * phiStep;

		for (uint32_t j = 0; j <= sliceCount; ++j)
		{
			float theta = j * thetaStep;

			Vector3 p;
			p.x = radius * sinf(phi) * cosf(theta);
			p.y = radius * cosf(phi);
			p.z = radius * sinf(phi) * sinf(theta);

			Vector3 t = { -radius * sinf(phi) * sinf(theta), 0.0f, radius * sinf(phi) * cosf(theta) };

			float u = theta / DirectX::XM_2PI;
			float v = phi / DirectX::XM_PI;

			mesh->vertices.push_back(p);
			mesh->normals.push_back(p);
			mesh->uv.push_back({ u, v });
			mesh->tangents.push_back(t);
			mesh->bitangents.push_back(mesh->normals.back().Cross(t));
		}
	}

	mesh->vertices.push_back({ 0.0f, -radius, 0.0f });
	mesh->normals.push_back({ 0.0f, -1.0f, 0.0f });
	mesh->uv.push_back({ 0.0f, 1.0f });
	mesh->tangents.push_back({ 1.0f, 0.0f, 0.0f });
	mesh->bitangents.push_back({ 0.0f, 0.0f, 1.0f });


	for (uint32_t i = 1; i <= sliceCount; ++i)
	{
		mesh->indices.push_back(0);
		mesh->indices.push_back(i + 1);
		mesh->indices.push_back(i);
	}

	uint32_t baseIndex = 1;
	uint32_t ringVertexCount = sliceCount + 1;

	for (uint32_t i = 0; i < stackCount - 2; ++i)
	{
		for (uint32_t j = 0; j < sliceCount; ++j)
		{
			mesh->indices.push_back(baseIndex + i * ringVertexCount + j);
			mesh->indices.push_back(baseIndex + i * ringVertexCount + j + 1);
			mesh->indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);

			mesh->indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
			mesh->indices.push_back(baseIndex + i * ringVertexCount + j + 1);
			mesh->indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
		}
	}

	uint32_t southPoleIndex = static_cast<uint32_t>(mesh->vertices.size()) - 1;

	baseIndex = southPoleIndex - ringVertexCount;

	for (uint32_t i = 0; i < sliceCount; ++i)
	{
		mesh->indices.push_back(southPoleIndex);
		mesh->indices.push_back(baseIndex + i);
		mesh->indices.push_back(baseIndex + i + 1);
	}

	mesh->subMeshCount = 1;
	mesh->subMeshDescriptors.push_back({ "Sphere", static_cast<uint32_t>(mesh->indices.size()), 0, static_cast<uint32_t>(mesh->vertices.size()), 0 });

	return mesh;

}


std::shared_ptr<Mesh> MeshGenerator::CreateQuad(Vector2 leftBottom, Vector2 rightTop, float z)
{
	auto mesh = std::make_shared<Mesh>();

	mesh->name = "Quad";

	mesh->vertices = {
		{ leftBottom.x, leftBottom.y, z },
		{ leftBottom.x, rightTop.y, z },
		{ rightTop.x, rightTop.y, z },
		{ rightTop.x, leftBottom.y, z }
	};

	mesh->normals = {
		{ 0.0f, 0.0f, -1.0f },
		{ 0.0f, 0.0f, -1.0f },
		{ 0.0f, 0.0f, -1.0f },
		{ 0.0f, 0.0f, -1.0f }
	};

	mesh->tangents = {
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f }
	};

	mesh->bitangents = {
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f }
	};

	mesh->uv = {
		{ 0.0f, 1.0f },
		{ 0.0f, 0.0f },
		{ 1.0f, 0.0f },
		{ 1.0f, 1.0f },
	};

	mesh->indices = {
		0, 1, 2, 0, 2, 3
	};


	mesh->boundingBox.Center = { (leftBottom.x + rightTop.x) * 0.5f, (leftBottom.y + rightTop.y) * 0.5f, z };
	mesh->boundingBox.Extents = { (rightTop.x - leftBottom.x) * 0.5f, (rightTop.y - leftBottom.y) * 0.5f, 0.0f };

	mesh->subMeshCount = 1;
	mesh->subMeshDescriptors.push_back({ "Quad", static_cast<uint32_t>(mesh->indices.size()), 0, static_cast<uint32_t>(mesh->vertices.size()), 0 });

	return mesh;
}

std::shared_ptr<Mesh> MeshGenerator::CreateTriangle()
{
	auto mesh = std::make_shared<Mesh>();

	mesh->vertices = {
		{ 0.0f, 1.0f, 0.0f },
		{ 1.0f, -1.0f, 0.0f },
		{ -1.0f, -1.0f, 0.0f }
	};

	// 이걸 컬러로 쓸거다.
	mesh->normals = {
		{ 1.f, 0.f, 0.f },
		{ 0.f, 1.f, 0.f },
		{ 0.f, 0.f, 1.f }
	};

	mesh->tangents = {
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f }
	};

	mesh->bitangents = {
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f }
	};

	mesh->uv = {
		{ 0.5f, 0.0f },
		{ 1.0f, 1.0f },
		{ 0.0f, 1.0f }
	};

	mesh->indices = {
		0, 1, 2
	};

	mesh->subMeshCount = 1;
	mesh->subMeshDescriptors.push_back({ "Triangle", static_cast<uint32_t>(mesh->indices.size()), 0, static_cast<uint32_t>(mesh->vertices.size()), 0 });

	return mesh;

}
