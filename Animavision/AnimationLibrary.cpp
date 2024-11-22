#include "pch.h"
#include "AnimationLibrary.h"
#include "Mesh.h"
#include "ModelLoader.h"
#include <fstream>

void AnimationLibrary::LoadAnimationClipsFromFile(const std::string& path)
{
	std::filesystem::path filePath(path);

	if (filePath.extension() != ".fbx")
		return;

	std::filesystem::path mcaPath = filePath;

	mcaPath.replace_extension(".mca");

	MCAFormat* mca = nullptr;

	if (std::filesystem::exists(mcaPath))
	{
		mca = loadAnimationClipsFromMCA(mcaPath.string());
	}
	else
	{
		mca = loadAnimationClipsFromFBX(path);
		if (mca)
		{
			saveAnimationClipsToMCA(mcaPath.string(), mca);
		}
	}

	// mca에서 읽어서 애니메이션 클립을 저장한다.
	if (mca)
	{
		for (auto& animationClip : mca->animationClips)
		{
			m_AnimationClips[animationClip->name] = animationClip;
		}

		delete mca;
	}
}

void AnimationLibrary::LoadAnimationClipsFromDirectory(const std::string& path)
{
	auto start = std::chrono::high_resolution_clock::now();


	std::filesystem::path directory(path);
	if (!std::filesystem::exists(directory))
		return;

	for (const auto& entry : std::filesystem::directory_iterator(directory))
	{
		if (entry.is_regular_file())
			LoadAnimationClipsFromFile(entry.path().string());
	}

	auto end = std::chrono::high_resolution_clock::now();

	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

	OutputDebugStringA("LoadAnimationClipsFromDirectory: ");
	OutputDebugStringA(std::to_string(duration).c_str());
	OutputDebugStringA("ms\n");

}

void AnimationLibrary::AddAnimationClip(std::shared_ptr<AnimationClip> animationClip)
{
	m_AnimationClips[animationClip->name] = animationClip;
}

std::shared_ptr<AnimationClip> AnimationLibrary::GetAnimationClip(const std::string& name)
{
	auto findIt = m_AnimationClips.find(name);
	if (findIt == m_AnimationClips.end())
		return nullptr;

	return findIt->second;
}

MCAFormat* AnimationLibrary::loadAnimationClipsFromMCA(const std::string& path)
{
	std::ifstream is(path, std::ios::binary);
	cereal::BinaryInputArchive archive(is);

	MCAFormat* mca = new MCAFormat;

	archive(*mca);

	return mca;
}

MCAFormat* AnimationLibrary::loadAnimationClipsFromFBX(const std::string& path)
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

	std::vector<AnimationClip*> animationClips;

	ModelLoader::LoadAnimations(path, animationClips, flags);

	if (animationClips.empty())
		return nullptr;

	MCAFormat* mca = new MCAFormat;

	for (auto& animationClip : animationClips)
	{
		OutputDebugStringA(animationClip->name.c_str());
		OutputDebugStringA("\n");
		mca->animationClips.push_back(std::shared_ptr<AnimationClip>(animationClip));
	}

	return mca;
}

void AnimationLibrary::saveAnimationClipsToMCA(const std::string& path, MCAFormat* mca)
{
	std::ofstream os(path, std::ios::binary);
	cereal::BinaryOutputArchive archive(os);

	archive(*mca);
}
