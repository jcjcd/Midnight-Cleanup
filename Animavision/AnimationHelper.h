#pragma once

#include "Mesh.h"
#include "RendererDLL.h"

class ANIMAVISION_DLL AnimationHelper
{
public:
	static void FindKeyframe(const std::vector<Keyframe>& keyframes, float timePos, Keyframe* outLhs, Keyframe* outRhs, float* outWeight);
	static void FindKeyframe(const std::vector<Keyframe>& keyframes, const AnimationClip& animationClip, float timePos, Keyframe* outLhs, Keyframe* outRhs, float* outWeight);
	static Matrix CreateMatrix(const Keyframe& keyframe);
	static Keyframe Interpolate(const Keyframe& lhs, const Keyframe& rhs, float weight);
	static Matrix ComputeAnimationTransform(const AnimationClip& animationClip, const std::string& nodeName, float timePos);
	static Matrix BlendTransforms(const AnimationClip& currentClip, const AnimationClip& nextClip, const std::string & nodeName, float blendProgress);;


	static Vector3 interpolateTranslation(const std::vector<Keyframe>& keyframes, float timePos);
	static Quaternion interpolateRotation(const std::vector<Keyframe>& keyframes, float timePos);
	static Vector3 interpolateScale(const std::vector<Keyframe>& keyframes, float timePos);
};

