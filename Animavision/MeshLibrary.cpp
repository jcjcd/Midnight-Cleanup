#include "pch.h"
#include "MeshLibrary.h"

#include "Mesh.h"
#include "ModelLoader.h"

#include <fstream>


MeshLibrary::MeshLibrary(Renderer* renderer) :
	_renderer(renderer)
{

}

void MeshLibrary::LoadMeshesFromFile(const std::string& path)
{
	std::filesystem::path filePath = path;

	if (filePath.extension() != ".fbx")
	{
		return;
	}

	std::filesystem::path mcmPath = filePath;

	mcmPath.replace_extension(".mcm");

	MCMFormat* mcm = nullptr;

	if (std::filesystem::exists(mcmPath))
	{
		mcm = loadMeshesFromMCM(mcmPath.string());
	}
	else
	{
		mcm = loadMeshesFromFBX(path);
		if (mcm)
		{
			saveMeshesToMCM(mcmPath.string(), mcm);
		}
	}

	// mcm에서 읽어서 메시를 저장한다.
	if (mcm)
	{
		for (auto& mesh : mcm->meshes)
		{
			mesh->CreateBuffers(_renderer);
			assert(!_meshes.contains(mesh->name));
			_meshes[mesh->name] = mesh;
		}

		delete mcm;
	}
}

MCMFormat* MeshLibrary::loadMeshesFromMCM(const std::string& path)
{
	std::ifstream is(path, std::ios::binary);
	cereal::BinaryInputArchive archive(is);

	MCMFormat* mcm = new MCMFormat;
	archive(*mcm);

	return mcm;
}

void MeshLibrary::saveMeshesToMCM(const std::string& path, MCMFormat* mcm)
{
	std::ofstream os(path, std::ios::binary);
	cereal::BinaryOutputArchive archive(os);

	archive(*mcm);
}

/// mcm에 넣어서 리턴해준다.
MCMFormat* MeshLibrary::loadMeshesFromFBX(const std::string& path)
{
	ModelParserFlags flags = ModelParserFlags::NONE;
	flags |= ModelParserFlags::TRIANGULATE;
	//flags |= ModelParserFlags::GEN_NORMALS;
	flags |= ModelParserFlags::GEN_SMOOTH_NORMALS;
	flags |= ModelParserFlags::GEN_UV_COORDS;
	flags |= ModelParserFlags::CALC_TANGENT_SPACE;
	flags |= ModelParserFlags::GEN_BOUNDING_BOXES;
	flags |= ModelParserFlags::MAKE_LEFT_HANDED;
	flags |= ModelParserFlags::FLIP_UVS;
	flags |= ModelParserFlags::FLIP_WINDING_ORDER;
	flags |= ModelParserFlags::LIMIT_BONE_WEIGHTS;
	flags |= ModelParserFlags::JOIN_IDENTICAL_VERTICES;
	flags |= ModelParserFlags::GLOBAL_SCALE;


	auto model = ModelLoader::Create(_renderer, path, flags);

	if (model == nullptr)
	{
		return nullptr;
	}

	MCMFormat* mcm = new MCMFormat;

	for (auto& mesh : model->GetMeshes())
	{
		//_meshes[mesh->name] = std::shared_ptr<Mesh>(mesh);
		mcm->meshes.push_back(std::shared_ptr<Mesh>(mesh));
#ifdef _DEBUG
		OutputDebugStringA(mesh->name.c_str());
		OutputDebugStringA("\n");
#endif
	}

	return mcm;
}

void MeshLibrary::LoadMeshesFromDirectory(const std::string& path)
{
	// 이 함수 시간측정 해보자

	std::filesystem::path directory(path);
	if (!std::filesystem::exists(directory))
	{
		return;
	}

	for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
	{
		if (entry.is_regular_file())
		{
			LoadMeshesFromFile(entry.path().string());
		}
	}
}

void MeshLibrary::AddMesh(std::shared_ptr<Mesh> mesh)
{
	mesh->CreateBuffers(_renderer);
	_meshes[mesh->name] = mesh;
}

std::shared_ptr<Mesh> MeshLibrary::GetMesh(const std::string& name)
{
	auto it = _meshes.find(name);
	if (it != _meshes.end())
	{
		return it->second;
	}

	return nullptr;
}
