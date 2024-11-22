#pragma once

class Mesh;
class Renderer;

class MCMFormat
{
public:
	std::vector<std::shared_ptr<Mesh>> meshes;

	template <class Archive>
	void serialize(Archive& ar)
	{
		ar(meshes);
	}
};

class MeshLibrary
{
public:
	MeshLibrary(Renderer* renderer);

	void LoadMeshesFromFile(const std::string& path);
	void LoadMeshesFromDirectory(const std::string& path);


	void AddMesh(std::shared_ptr<Mesh> mesh);

	std::shared_ptr<Mesh> GetMesh(const std::string& name);

	std::map<std::string, std::shared_ptr<Mesh>>& GetMeshes() { return _meshes; }

private:
	MCMFormat* loadMeshesFromFBX(const std::string& path);
	MCMFormat* loadMeshesFromMCM(const std::string& path);
	void saveMeshesToMCM(const std::string& path, MCMFormat* mcm);

private:
	Renderer* _renderer = nullptr;
	std::map<std::string, std::shared_ptr<Mesh>> _meshes;
};

