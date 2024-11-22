#include "pch.h"
#include "AnimationHelper.h"

void AnimationHelper::FindKeyframe(const std::vector<Keyframe>& keyframes, float timePos, Keyframe* outLhs, Keyframe* outRhs, float* outWeight)
{
	for (uint32_t i = 0; i < keyframes.size(); i++)
	{
		if (keyframes[i].timePos < timePos)
			continue;

		if (i == 0)
		{
			*outLhs = keyframes[i];
			*outRhs = keyframes.size() > 1 ? keyframes[i + 1] : keyframes[i];
		}
	}

	for (auto& keyframe : keyframes)
	{
		if (keyframe.timePos > timePos)
		{
			*outRhs = keyframe;
			break;
		}
	}

}

void AnimationHelper::FindKeyframe(const std::vector<Keyframe>& keyframes, const AnimationClip& animationClip, float timePos, Keyframe* outLhs, Keyframe* outRhs, float* outWeight)
{
	uint32_t prevIndex = static_cast<uint32_t>(floor(timePos / animationClip.framePerSecond));

	if (prevIndex < keyframes.size())
	{
		*outLhs = keyframes[prevIndex];
		*outRhs = prevIndex == keyframes.size() - 1 ? keyframes.back() : keyframes[prevIndex + 1];
		*outWeight = fmod(timePos, animationClip.framePerSecond) / animationClip.framePerSecond;
	}
	else
	{
		*outLhs = keyframes.back();
		*outRhs = keyframes.back();
		*outWeight = 1.0f;
	}
}

Matrix AnimationHelper::CreateMatrix(const Keyframe& keyframe)
{
	return Matrix::CreateScale(keyframe.scale) * Matrix::CreateFromQuaternion(keyframe.rotation) * Matrix::CreateTranslation(keyframe.translation);
}

Keyframe AnimationHelper::Interpolate(const Keyframe& lhs, const Keyframe& rhs, float weight)
{
	Keyframe keyframe;

	keyframe.scale = Vector3::Lerp(lhs.scale, rhs.scale, weight);
	keyframe.rotation = Quaternion::Slerp(lhs.rotation, rhs.rotation, weight);
	keyframe.translation = Vector3::Lerp(lhs.translation, rhs.translation, weight);

	return keyframe;
}

Matrix AnimationHelper::ComputeAnimationTransform(const AnimationClip& animationClip, const std::string& nodeName, float timePos)
{
	for (auto& nodeClip : animationClip.nodeClips)
	{
		if (nodeClip->nodeName == nodeName)
		{
			auto translation = interpolateTranslation(nodeClip->keyframes, timePos);
			auto rotation = interpolateRotation(nodeClip->keyframes, timePos);
			rotation.Normalize();
			auto scale = interpolateScale(nodeClip->keyframes, timePos);

			auto T = Matrix::CreateTranslation(translation);
			auto R = Matrix::CreateFromQuaternion(rotation);
			auto S = Matrix::CreateScale(scale);

			return S * R * T;
		}
	}

	// If the node is not animated, return identity matrix
	return Matrix::Identity;
}

Matrix AnimationHelper::BlendTransforms(const AnimationClip& currentClip, const AnimationClip& nextClip, const std::string & nodeName, float blendProgress)
{
	auto& curNodeClips = currentClip.nodeClips;
	auto& nextNodeClips = nextClip.nodeClips;

	for (uint32_t i = 0; i < curNodeClips.size(); i++)
	{
		if(curNodeClips[i]->nodeName != nodeName || nextNodeClips[i]->nodeName != nodeName)
			continue;

		auto& curNodeClip = curNodeClips[i];
		auto& nextNodeClip = nextNodeClips[i];

		auto translation = Vector3::Lerp(interpolateTranslation(curNodeClip->keyframes, currentClip.currentTimePos), interpolateTranslation(nextNodeClip->keyframes, nextClip.currentTimePos), blendProgress);
		auto rotation = Quaternion::Slerp(interpolateRotation(curNodeClip->keyframes, currentClip.currentTimePos), interpolateRotation(nextNodeClip->keyframes, nextClip.currentTimePos), blendProgress);
		auto scale = Vector3::Lerp(interpolateScale(curNodeClip->keyframes, currentClip.currentTimePos), interpolateScale(nextNodeClip->keyframes, nextClip.currentTimePos), blendProgress);
		
		rotation.Normalize();

		auto T = Matrix::CreateTranslation(translation);
		auto R = Matrix::CreateFromQuaternion(rotation);
		auto S = Matrix::CreateScale(scale);

		return S * R * T;
	}

	// If the node is not animated, return identity matrix
	return Matrix::Identity;
}

Vector3 AnimationHelper::interpolateTranslation(const std::vector<Keyframe>& keyframes, float timePos)
{
	for (uint32_t i = 0; i < keyframes.size() - 1; i++)
	{
		if (timePos < keyframes[i + 1].timePos)
		{
			float lerpFactor = (timePos - keyframes[i].timePos) / (keyframes[i + 1].timePos - keyframes[i].timePos);
			return Vector3::Lerp(keyframes[i].translation, keyframes[i + 1].translation, lerpFactor);
		}
	}
	return keyframes.back().translation;
}

Quaternion AnimationHelper::interpolateRotation(const std::vector<Keyframe>& keyframes, float timePos)
{
	for (uint32_t i = 0; i < keyframes.size() - 1; i++)
	{
		if (timePos < keyframes[i + 1].timePos)
		{
			float lerpFactor = (timePos - keyframes[i].timePos) / (keyframes[i + 1].timePos - keyframes[i].timePos);
			return Quaternion::Slerp(keyframes[i].rotation, keyframes[i + 1].rotation, lerpFactor);
		}
	}
	return keyframes.back().rotation;
}

Vector3 AnimationHelper::interpolateScale(const std::vector<Keyframe>& keyframes, float timePos)
{
	for (uint32_t i = 0; i < keyframes.size() - 1; i++)
	{
		if (timePos < keyframes[i + 1].timePos)
		{
			float lerpFactor = (timePos - keyframes[i].timePos) / (keyframes[i + 1].timePos - keyframes[i].timePos);
			return Vector3::Lerp(keyframes[i].scale, keyframes[i + 1].scale, lerpFactor);
		}
	}
	return keyframes.back().scale;
}
