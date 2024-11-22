#pragma once

struct AnimationClip;

class MCAFormat
{
public:
	std::vector<std::shared_ptr<AnimationClip>> animationClips;

	template <class Archive>
	void serialize(Archive& ar)
	{
		ar(animationClips);
	}

};

class AnimationLibrary
{
public:
	AnimationLibrary() = default;
	~AnimationLibrary() = default;

	void LoadAnimationClipsFromFile(const std::string& path);
	void LoadAnimationClipsFromDirectory(const std::string& path);
	void AddAnimationClip(std::shared_ptr<AnimationClip> animationClip);

	std::shared_ptr<AnimationClip> GetAnimationClip(const std::string& name);
	std::map<std::string, std::shared_ptr<AnimationClip>>& GetAnimationClips() { return m_AnimationClips; }

private:
	MCAFormat* loadAnimationClipsFromMCA(const std::string& path);
	MCAFormat* loadAnimationClipsFromFBX(const std::string& path);
	void saveAnimationClipsToMCA(const std::string& path, MCAFormat* mca);

private:
	std::map<std::string, std::shared_ptr<AnimationClip>> m_AnimationClips;
};

