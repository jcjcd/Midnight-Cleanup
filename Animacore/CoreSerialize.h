#pragma once
#include "CoreComponents.h"
#include "SerializeHelper.h"
#include "RenderComponents.h"
#include "CoreTagsAndLayers.h"
#include "CorePhysicsComponents.h"

namespace cereal
{
	template <class Archive>
	void serialize(Archive& archive, DirectX::SimpleMath::Vector2& data)
	{
		archive(
			MAKE_NVP(x),
			MAKE_NVP(y)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, DirectX::SimpleMath::Vector3& data)
	{
		archive(
			MAKE_NVP(x),
			MAKE_NVP(y),
			MAKE_NVP(z)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, DirectX::SimpleMath::Vector4& data)
	{
		archive(
			MAKE_NVP(x),
			MAKE_NVP(y),
			MAKE_NVP(z),
			MAKE_NVP(w)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, DirectX::SimpleMath::Quaternion& data)
	{
		archive(
			MAKE_NVP(x),
			MAKE_NVP(y),
			MAKE_NVP(z),
			MAKE_NVP(w)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, DirectX::SimpleMath::Color& data)
	{
		archive(
			MAKE_NVP(x),
			MAKE_NVP(y),
			MAKE_NVP(z),
			MAKE_NVP(w)
		);
	}


	
	template <typename Archive>
	void serialize(Archive& archive, core::Transform& data)
	{
		archive(
			MAKE_NVP(dummy)
		);
	}

	template <typename Archive>
	void serialize(Archive& archive, core::LocalTransform& data)
	{
		archive(
			MAKE_NVP(position),
			MAKE_NVP(rotation),
			MAKE_NVP(scale)
		);
	}

	template <typename Archive>
	void serialize(Archive& archive, core::WorldTransform& data)
	{
		archive(
			MAKE_NVP(position),
			MAKE_NVP(rotation),
			MAKE_NVP(scale)
		);
	}

	template <typename Archive>
	void serialize(Archive& archive, core::Name& data)
	{
		archive(MAKE_NVP(name));
	}

	template <typename Archive>
	void serialize(Archive& archive, core::Relationship& data)
	{
		archive(
			MAKE_NVP(parent),
			MAKE_NVP(children)
		);
	}

	template <typename Archive>
	void serialize(Archive& archive, core::Rigidbody& data)
	{
		archive(
			MAKE_NVP(mass),
			MAKE_NVP(drag),
			MAKE_NVP(angularDrag),
			MAKE_NVP(useGravity),
			MAKE_NVP(isKinematic),
			MAKE_NVP(interpolation),
			MAKE_NVP(constraints)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, core::ColliderCommon::PhysicMaterial& data)
	{
		archive(
			MAKE_NVP(dynamicFriction),
			MAKE_NVP(staticFriction),
			MAKE_NVP(bounciness)
		);
	}

	template <typename Archive>
	void serialize(Archive& archive, core::ColliderCommon& data)
	{
		archive(
			MAKE_NVP(isTrigger),
			MAKE_NVP(materialName),
			MAKE_NVP(center),
			MAKE_NVP(contactOffset)
		);
	}

	template <typename Archive>
	void serialize(Archive& archive, core::BoxCollider& data)
	{
		archive(
			MAKE_NVP(size)
		);
	}

	template <typename Archive>
	void serialize(Archive& archive, core::SphereCollider& data)
	{
		archive(
			MAKE_NVP(radius)
		);
	}

	template <typename Archive>
	void serialize(Archive& archive, core::CapsuleCollider& data)
	{
		archive(
			MAKE_NVP(radius),
			MAKE_NVP(height),
			MAKE_NVP(direction)
		);
	}

	template <typename Archive>
	void serialize(Archive& archive, core::MeshCollider& data)
	{
		archive(
			MAKE_NVP(convex),
			MAKE_NVP(cookingOptions)
		);
	}

	template <typename Archive>
	void serialize(Archive& archive, core::CharacterController& data)
	{
		archive(
			MAKE_NVP(slopeLimit),
			MAKE_NVP(stepOffset),
			MAKE_NVP(skinWidth),
			MAKE_NVP(radius),
			MAKE_NVP(height),
			MAKE_NVP(density)
		);
	}

	template <typename Archive>
	void serialize(Archive& archive, core::Tag& data)
	{
		archive(MAKE_NVP(id));
	}

	template <typename Archive>
	void serialize(Archive& archive, core::Layer& data)
	{
		archive(MAKE_NVP(mask));
	}

	template <typename Archive>
	void serialize(Archive& archive, core::Configuration& data)
	{
		archive(
			MAKE_NVP(width),
			MAKE_NVP(height)
		);
	}

	template <typename Archive>
	void serialize(Archive& archive, core::Sound& data)
	{
		archive(
			MAKE_NVP(path),
			MAKE_NVP(volume),
			MAKE_NVP(pitch),
			MAKE_NVP(minDistance),
			MAKE_NVP(maxDistance),
			MAKE_NVP(isLoop),
			MAKE_NVP(is3D),
			MAKE_NVP(isPlaying)
		);
	}

	template <typename Archive>
	void serialize(Archive& archive, core::SoundListener& data)
	{
		archive(
			MAKE_NVP(applyDoppler)
		);
	}

	/* -------------------
		Render Components
	------------------- */
	template <class Archive>
	void serialize(Archive& archive, core::MeshRenderer& data)
	{
		archive(
			MAKE_NVP(meshString),
			MAKE_NVP(materialStrings),
			MAKE_NVP(isOn),
			MAKE_NVP(receiveShadow),
			MAKE_NVP(isSkinned),
			MAKE_NVP(isCulling),
			MAKE_NVP(canReceivingDecal)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, core::Camera& data)
	{
		archive(
			MAKE_NVP(isActive),
			MAKE_NVP(isOrthographic),
			MAKE_NVP(fieldOfView),
			MAKE_NVP(nearClip),
			MAKE_NVP(farClip),
			MAKE_NVP(orthographicSize),
			MAKE_NVP(aspectRatio)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, core::FreeFly& data)
	{
		archive(
			MAKE_NVP(isActive),
			MAKE_NVP(moveSpeed),
			MAKE_NVP(rotateSpeed)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, core::Decal& data)
	{
		archive(
			MAKE_NVP(fadeFactor),
			MAKE_NVP(size),
			MAKE_NVP(materialString)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, core::Animator& data)
	{
		archive(MAKE_NVP(animatorFileName));
	}

	template <class Archive>
	void serialize(Archive& archive, core::LightCommon& data)
	{
		archive(
			MAKE_NVP(color),
			MAKE_NVP(intensity),
			MAKE_NVP(isOn),
			MAKE_NVP(useShadow),
			MAKE_NVP(lightMode)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, core::DirectionalLight& data)
	{
		archive(MAKE_NVP(cascadeEnds));
	}

	template <class Archive>
	void serialize(Archive& archive, core::PointLight& data)
	{
		archive(
			MAKE_NVP(attenuation),
			MAKE_NVP(range)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, core::SpotLight& data)
	{
		archive(
			MAKE_NVP(attenuation),
			MAKE_NVP(range),
			MAKE_NVP(spotAngle),
			MAKE_NVP(innerAngle)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, core::UICommon& data)
	{
		archive(
			MAKE_NVP(isOn),
			MAKE_NVP(color),
			MAKE_NVP(textureString),
			MAKE_NVP(maskingOption)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, core::UI2D& data)
	{
		archive(
			MAKE_NVP(position),
			MAKE_NVP(rotation),
			MAKE_NVP(size),
			MAKE_NVP(layerDepth),
			MAKE_NVP(isCanvas)
		);
	}
	
	template <class Archive>
	void serialize(Archive& archive, core::UI3D& data)
	{
		archive(
			MAKE_NVP(isBillboard),
			MAKE_NVP(constrainedOption)
		);
	}
	
	template <class Archive>
	void serialize(Archive& archive, core::Text& data)
	{
		archive(
			MAKE_NVP(isOn),
			MAKE_NVP(color),
			MAKE_NVP(position),
			MAKE_NVP(size),
			MAKE_NVP(scale),
			MAKE_NVP(fontString),
			MAKE_NVP(fontSize),
			MAKE_NVP(text),
			MAKE_NVP(textAlign),
			MAKE_NVP(textBoxAlign),
			MAKE_NVP(leftPadding),
			MAKE_NVP(useUnderline),
			MAKE_NVP(useStrikeThrough)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, core::Button& data)
	{
		archive(
			MAKE_NVP(interactable),
			MAKE_NVP(onClickFunctions),
			MAKE_NVP(highlightTextureString),
			MAKE_NVP(pressedTextureString),
			MAKE_NVP(selectedTextureString),
			MAKE_NVP(disabledTextureString)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, core::LightMap& data)
	{
		archive(
			MAKE_NVP(index),
			MAKE_NVP(tilling),
			MAKE_NVP(offset)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, core::Agent& data)
	{
		archive(
			MAKE_NVP(radius),
			MAKE_NVP(height),
			MAKE_NVP(speed),
			MAKE_NVP(acceleration),
			MAKE_NVP(collisionQueryRange),
			MAKE_NVP(isStopped)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, core::NavMeshConfigs& data)
	{
		archive(
			MAKE_NVP(meshName),
			MAKE_NVP(settings)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, core::NavMeshSettings& data)
	{
		archive(
			MAKE_NVP(cellSize),
			MAKE_NVP(cellHeight),
			MAKE_NVP(agentRadius),
			MAKE_NVP(agentHeight),
			MAKE_NVP(agentMaxClimb),
			MAKE_NVP(agentMaxSlope),
			MAKE_NVP(regionMinSize),
			MAKE_NVP(regionMergeSize),
			MAKE_NVP(edgeMaxLen),
			MAKE_NVP(edgeMaxError),
			MAKE_NVP(vertsPerPoly),
			MAKE_NVP(detailSampleDist),
			MAKE_NVP(detailSampleMaxError),
			MAKE_NVP(partitionType),
			MAKE_NVP(navMeshBMin),
			MAKE_NVP(navMeshBMax),
			MAKE_NVP(tileSize)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, core::ParticleSystem& data)
	{
		archive(
			MAKE_NVP(mainData),
			MAKE_NVP(emissionData),
			MAKE_NVP(shapeData),
			MAKE_NVP(velocityOverLifeTimeData),
			MAKE_NVP(limitVelocityOverLifeTimeData),
			MAKE_NVP(forceOverLifeTimeData),
			MAKE_NVP(colorOverLifeTimeData),
			MAKE_NVP(sizeOverLifeTimeData),
			MAKE_NVP(rotationOverLifeTimeData),
			MAKE_NVP(instanceData),
			MAKE_NVP(renderData),
			MAKE_NVP(textureSheetAnimationData),
			MAKE_NVP(isOn)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, core::ParticleSystem::MainModule& data)
	{
		archive(
			MAKE_NVP(duration),
			MAKE_NVP(isLooping),
			MAKE_NVP(startDelayOption),
			MAKE_NVP(startDelay),
			MAKE_NVP(startLifeTimeOption),
			MAKE_NVP(startLifeTime),
			MAKE_NVP(startSpeedOption),
			MAKE_NVP(startSpeed),
			MAKE_NVP(startSizeOption),
			MAKE_NVP(startSize),
			MAKE_NVP(startRotationOption),
			MAKE_NVP(startRotation),
			MAKE_NVP(startColorOption),
			MAKE_NVP(startColor0),
			MAKE_NVP(startColor1),
			MAKE_NVP(gravityModifierOption),
			MAKE_NVP(gravityModifier),
			MAKE_NVP(simulationSpeed),
			MAKE_NVP(maxParticleCounts)
		);
	}
	
	template <class Archive>
	void serialize(Archive& archive, core::ParticleSystem::EmissionModule::Burst& data)
	{
		archive(
			MAKE_NVP(timePos),
			MAKE_NVP(count),
			MAKE_NVP(cycles),
			MAKE_NVP(interval),
			MAKE_NVP(probability)
		);
	}
	
	template <class Archive>
	void serialize(Archive& archive, core::ParticleSystem::EmissionModule& data)
	{
		archive(
			MAKE_NVP(particlePerSecond),
			MAKE_NVP(bursts)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, core::ParticleSystem::ShapeModule& data)
	{
		archive(
			MAKE_NVP(shapeType),
			MAKE_NVP(angleInDegree),
			MAKE_NVP(radius),
			MAKE_NVP(donutRadius),
			MAKE_NVP(arcInDegree),
			MAKE_NVP(spread),
			MAKE_NVP(radiusThickness),
			MAKE_NVP(position),
			MAKE_NVP(rotation),
			MAKE_NVP(scale)
		);
	}
	
	template <class Archive>
	void serialize(Archive& archive, core::ParticleSystem::VelocityOverLifeTimeModule& data)
	{
		archive(
			MAKE_NVP(velocity),
			MAKE_NVP(orbital),
			MAKE_NVP(offset),
			MAKE_NVP(isUsed)
		);
	}
	
	template <class Archive>
	void serialize(Archive& archive, core::ParticleSystem::LimitVelocityOverLifeTimeModule& data)
	{
		archive(
			MAKE_NVP(speed),
			MAKE_NVP(dampen),
			MAKE_NVP(isUsed)
		);
	}
	
	template <class Archive>
	void serialize(Archive& archive, core::ParticleSystem::ForceOverLifeTimeModule& data)
	{
		archive(
			MAKE_NVP(force),
			MAKE_NVP(isUsed)
		);
	}
	
	template <class Archive>
	void serialize(Archive& archive, core::ParticleSystem::ColorOverLifeTimeModule& data)
	{
		archive(
			MAKE_NVP(alphaRatios),
			MAKE_NVP(colorRatios),
			MAKE_NVP(isUsed)
		);
	}
	
	template <class Archive>
	void serialize(Archive& archive, core::ParticleSystem::SizeOverLifeTimeModule& data)
	{
		archive(
			MAKE_NVP(pointA),
			MAKE_NVP(pointB),
			MAKE_NVP(pointC),
			MAKE_NVP(pointD),
			MAKE_NVP(isUsed)
		);
	}
	
	template <class Archive>
	void serialize(Archive& archive, core::ParticleSystem::RotationOverLifeTimeModule& data)
	{
		archive(
			MAKE_NVP(angularVelocityInDegree),
			MAKE_NVP(isUsed)
		);
	}
	
	template <class Archive>
	void serialize(Archive& archive, core::ParticleSystem::InstanceModule& data)
	{
		archive(
			MAKE_NVP(isEmit),
			MAKE_NVP(isReset)
		);
	}
	
	template <class Archive>
	void serialize(Archive& archive, core::ParticleSystem::RenderModule& data)
	{
		archive(
			MAKE_NVP(renderModeType),
			MAKE_NVP(colorModeType),
			MAKE_NVP(baseColor),
			MAKE_NVP(emissiveColor),
			MAKE_NVP(baseColorTextureString),
			MAKE_NVP(emissiveTextureString),
			MAKE_NVP(useBaseColorTexture),
			MAKE_NVP(useEmissiveTexture),
			MAKE_NVP(tiling),
			MAKE_NVP(offset),
			MAKE_NVP(alphaCutOff),
			MAKE_NVP(isTwoSide),
			MAKE_NVP(useMultiplyAlpha)
		);
	}
	
	template <class Archive>
	void serialize(Archive& archive, core::ParticleSystem::TextureSheetAnimationModule& data)
	{
		archive(
			MAKE_NVP(tiles),
			MAKE_NVP(isUsed)
		);
	}

	template <class Archive>
	void serialize(Archive& archive, core::RenderAttributes& data)
	{
		archive(
			MAKE_NVP(flags)
		);
	}
	
	template <class Archive>
	void serialize(Archive& archive, core::ComboBox& data)
	{
		archive(
			MAKE_NVP(isOn),
			MAKE_NVP(comboBoxTexts),
			MAKE_NVP(curIndex)
		);
	}
	
	template <class Archive>
	void serialize(Archive& archive, core::ComboBox::StringWrapper& data)
	{
		archive(
			MAKE_NVP(text)
		);
	}
	
	template <class Archive>
	void serialize(Archive& archive, core::Slider& data)
	{
		archive(
			MAKE_NVP(isOn),
			MAKE_NVP(minMax),
			MAKE_NVP(curWeight),
			MAKE_NVP(sliderLayout),
			MAKE_NVP(sliderHandle),
			MAKE_NVP(isVertical)
		);
	}
	
	template <class Archive>
	void serialize(Archive& archive, core::CheckBox& data)
	{
		archive(
			MAKE_NVP(isChecked),
			MAKE_NVP(checkedTextureString),
			MAKE_NVP(uncheckedTextureString)
		);
	}
	
	template <class Archive>
	void serialize(Archive& archive, core::PostProcessingVolume& data)
	{
		archive(
			MAKE_NVP(useBloom),
			MAKE_NVP(bloomCurve),
			MAKE_NVP(bloomTint),
			MAKE_NVP(bloomIntensity),
			MAKE_NVP(bloomThreshold),
			MAKE_NVP(bloomScatter),
			MAKE_NVP(useBloomScatter),
			MAKE_NVP(useFog),
			MAKE_NVP(fogColor),
			MAKE_NVP(fogDensity),
			MAKE_NVP(fogStart),
			MAKE_NVP(fogRange),
			MAKE_NVP(useExposure),
			MAKE_NVP(exposure),
			MAKE_NVP(contrast),
			MAKE_NVP(saturation)
		);
	}
}
