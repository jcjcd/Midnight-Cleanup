#pragma once
#include "PxUtils.h"
#include "MetaCtxs.h"
#include "MetaHelpers.h"
#include "CoreSerialize.h"
#include "SystemTemplates.h"
#include "RenderComponents.h"
#include "CoreTagsAndLayers.h"
#include "ComponentTemplates.h"
#include "CorePhysicsComponents.h"

// core systems
#include "InputSystem.h"
#include "SoundSystem.h"
#include "RenderSystems.h"
#include "PhysicsSystem.h"
#include "TransformSystem.h"
#include "AnimatorSystem.h"
#include "PathFindingSystem.h"
#include "ButtonSystem.h"
#include "RaytraceRenderSystem.h"
#include "FreeFlyCameraSystem.h"
#include "PreRenderSystem.h"
#include "PostRenderSystem.h"


// temp
#include "PlayerTestSystem.h"

namespace core
{
	inline void RegisterCoreMetaData()
	{
		// 시스템 메타 데이터 등록
		{
			REGISTER_SYSTEM_META(InputSystem);
			REGISTER_SYSTEM_META(RenderSystem);
			REGISTER_SYSTEM_META(PhysicsSystem);
			REGISTER_SYSTEM_META(TransformSystem);
			REGISTER_SYSTEM_META(AnimatorSystem);
			REGISTER_SYSTEM_META(RaytraceRenderSystem);
			REGISTER_SYSTEM_META(FreeFlyCameraSystem);
			REGISTER_SYSTEM_META(PathFindingSystem);
			REGISTER_SYSTEM_META(SoundSystem);
			REGISTER_SYSTEM_META(ButtonSystem);
			REGISTER_SYSTEM_META(PreRenderSystem);
			REGISTER_SYSTEM_META(PostRenderSystem);


			// 임시로 만든 유저 시스템
			REGISTER_SYSTEM_META(PlayerTestSystem);
		}

		// 이벤트 메타 데이터 등록
		{
			REGISTER_EVENT_META(OnStartSystem);
			REGISTER_EVENT_META(OnFinishSystem);
			REGISTER_EVENT_META(OnCreateEntity);
			REGISTER_EVENT_META(OnDestroyEntity);
		}

		// 컴포넌트 메타 데이터 등록
		{
			REGISTER_COMPONENT_META(Name)
				SET_DEFAULT_FUNC(Name)
				SET_MEMBER(Name::name, "Name");

			REGISTER_COMPONENT_META(Transform)
				NEED_COMPONENTS(LocalTransform, WorldTransform)
				SET_DEFAULT_FUNC(Transform);

			REGISTER_COMPONENT_META(LocalTransform)
				NEED_COMPONENTS(LocalTransform, WorldTransform)
				IS_HIDDEN()
				SET_DEFAULT_FUNC(LocalTransform)
				SET_MEMBER(LocalTransform::position, "Position")
				SET_DRAG_SPEED(0.01f)
				SET_MEMBER(LocalTransform::rotation, "Rotation")
				SET_DRAG_SPEED(0.1f)
				SET_MEMBER(LocalTransform::scale, "Scale")
				SET_DRAG_SPEED(0.01f);

			REGISTER_COMPONENT_META(WorldTransform)
				NEED_COMPONENTS(LocalTransform, WorldTransform)
				IS_HIDDEN()
				SET_DEFAULT_FUNC(WorldTransform)
				SET_MEMBER(WorldTransform::position, "Position")
				SET_MEMBER(WorldTransform::rotation, "Rotation")
				SET_MEMBER(WorldTransform::scale, "Scale");

			REGISTER_COMPONENT_META(Relationship)
				SET_DEFAULT_FUNC(Relationship)
				SET_MEMBER(Relationship::parent, "Parent");
			//SET_MEMBER(Relationship::children, "Children");

			REGISTER_COMPONENT_META(Rigidbody)
				NEED_COMPONENTS(ColliderCommon, Rigidbody)
				SET_DEFAULT_FUNC(Rigidbody)
				SET_MEMBER(Rigidbody::mass, "Mass")
				SET_DRAG_SPEED(0.01f)
				SET_MEMBER(Rigidbody::drag, "Drag")
				SET_DRAG_SPEED(0.01f)
				SET_MEMBER(Rigidbody::angularDrag, "Angular Drag")
				SET_DRAG_SPEED(0.01f)
				SET_MEMBER(Rigidbody::useGravity, "Use Gravity")
				SET_MEMBER(Rigidbody::isKinematic, "Is Kinematic")
				SET_DESC("설명: 이 플래그를 설정하면 객체를 키네틱 모드로 설정합니다. 키네틱 객체는 힘(예: 중력)의 영향을 받지 않으며, 운동량이 없습니다.\n무한한 질량을 가지고 있으며 특정 메서드를 사용하여 월드에서 이동할 수 있습니다.\n키네틱 객체는 일반 동적 객체를 밀어낼 수 있지만, 정적 객체나 다른 키네틱 객체와는 충돌하지 않습니다.\n\n용도: 움직이는 플랫폼이나 캐릭터와 같이 직접적인 이동 제어가 필요한 경우에 사용됩니다.")
				SET_MEMBER(Rigidbody::interpolation, "Interpolation")
				SET_MEMBER(Rigidbody::constraints, "Constraints");

			REGISTER_COMPONENT_META(ColliderCommon)
				IS_HIDDEN()
				SET_DEFAULT_FUNC(ColliderCommon)
				SET_MEMBER(ColliderCommon::isTrigger, "Is Trigger")
				SET_DESC("콜라이더가 충돌 대신 트리거로 작동할지 여부")
				SET_MEMBER(ColliderCommon::materialName, "Physic Material")
				SET_MEMBER(ColliderCommon::center, "Center")
				SET_DRAG_SPEED(0.01f)
				SET_DESC("콜라이더의 로컬 좌표계에서의 중심 위치")
				SET_MEMBER(ColliderCommon::contactOffset, "Contact Offset")
				SET_DESC("콜라이더의 실제 충돌 감지 경계 외부에 추가적인 여유 공간");

			REGISTER_COMPONENT_META(BoxCollider)
				NEED_COMPONENTS(ColliderCommon, BoxCollider)
				SET_DEFAULT_FUNC(BoxCollider)
				SET_MEMBER(BoxCollider::size, "Size")
				SET_DRAG_SPEED(0.01f);

			REGISTER_COMPONENT_META(SphereCollider)
				NEED_COMPONENTS(ColliderCommon, SphereCollider)
				SET_DEFAULT_FUNC(SphereCollider)
				SET_MEMBER(SphereCollider::radius, "Radius")
				SET_DRAG_SPEED(0.01f);

			REGISTER_COMPONENT_META(CapsuleCollider)
				NEED_COMPONENTS(ColliderCommon, CapsuleCollider)
				SET_DEFAULT_FUNC(CapsuleCollider)
				SET_MEMBER(CapsuleCollider::radius, "Radius")
				SET_DRAG_SPEED(0.01f)
				SET_MEMBER(CapsuleCollider::height, "Height")
				SET_DRAG_SPEED(0.01f)
				SET_MEMBER(CapsuleCollider::direction, "Direction")
				SET_MIN_MAX(0, 2);

			REGISTER_COMPONENT_META(MeshCollider)
				NEED_COMPONENTS(ColliderCommon, MeshCollider)
				SET_DEFAULT_FUNC(MeshCollider)
				SET_MEMBER(MeshCollider::cookingOptions, "Cooking Options")
				SET_MEMBER(MeshCollider::convex, "Convex");

			REGISTER_COMPONENT_META(CharacterController)
				NEED_COMPONENTS(ColliderCommon, CharacterController)
				SET_DEFAULT_FUNC(CharacterController)
				SET_MEMBER(CharacterController::slopeLimit, "Slope Limit")
				SET_DESC("캐릭터가 오를 수 있는 최대 경사각")
				SET_MIN_MAX(0.f, 360.f)
				SET_MEMBER(CharacterController::stepOffset, "Step Offset")
				SET_DESC("캐릭터가 오를 수 있는 최대 계단 높이")
				SET_MIN_MAX(0.f, FLT_MAX)
				SET_MEMBER(CharacterController::skinWidth, "Skin Width")
				SET_DESC("캐릭터 컨트롤러의 외부에 추가적인 여유 공간(충돌 감지의 정확성을 높이기 위해 사용)")
				SET_MIN_MAX(0.f, FLT_MAX)
				SET_MEMBER(CharacterController::radius, "Radius")
				SET_DESC("캐릭터 컨트롤러의 반지름")
				SET_MIN_MAX(0.f, FLT_MAX)
				SET_MEMBER(CharacterController::height, "Height")
				SET_DESC("캐릭터 컨트롤러의 높이")
				SET_MIN_MAX(0.f, FLT_MAX)
				SET_MEMBER(CharacterController::density, "Density")
				SET_DESC("캐릭터 컨트롤러의 밀도(밀도는 질량 계산의 기본값으로 사용)")
				SET_MIN_MAX(0.f, FLT_MAX);

			REGISTER_COMPONENT_META(Tag)
				SET_DEFAULT_FUNC(Tag)
				SET_MEMBER(Tag::id, "ID");

			REGISTER_COMPONENT_META(Layer)
				SET_DEFAULT_FUNC(Layer)
				SET_MEMBER(Layer::mask, "Mask");

			REGISTER_COMPONENT_META(MeshRenderer)
				SET_DEFAULT_FUNC(MeshRenderer)
				SET_MEMBER(MeshRenderer::meshString, "Mesh")
				SET_MEMBER(MeshRenderer::materialStrings, "Materials")
				SET_MEMBER(MeshRenderer::isOn, "Is On")
				SET_MEMBER(MeshRenderer::receiveShadow, "Receive Shadow")
				SET_MEMBER(MeshRenderer::isSkinned, "Is Skinned")
				SET_MEMBER(MeshRenderer::isCulling, "Is Culling")
				SET_DESC("unity, unreal의 plane에 적용하기 위해서 추가한 옵션")
				SET_MEMBER(MeshRenderer::canReceivingDecal, "Render Decal")
				SET_DESC("decal이 묻을 수 있는 meshRenderer");

			REGISTER_COMPONENT_META(Decal)
				SET_DEFAULT_FUNC(Decal)
				SET_MEMBER(Decal::fadeFactor, "Fade Factor")
				SET_MIN_MAX(0.0f, 1.0f)
				SET_MEMBER(Decal::size, "Size")
				SET_MEMBER(Decal::materialString, "Material");

			REGISTER_COMPONENT_META(Camera)
				SET_DEFAULT_FUNC(Camera)
				SET_MEMBER(Camera::isActive, "Is Active")
				SET_MEMBER(Camera::isOrthographic, "Is Orthographic")
				SET_MEMBER(Camera::fieldOfView, "Field Of View")
				SET_MEMBER(Camera::nearClip, "Near Clip")
				SET_MIN_MAX(0.01f, 9999.9f)
				SET_MEMBER(Camera::farClip, "Far Clip")
				SET_MIN_MAX(0.02f, 10000.0f)
				SET_MEMBER(Camera::orthographicSize, "Orthographic Size")
				SET_MEMBER(Camera::aspectRatio, "Aspect Ratio")
				SET_MIN_MAX(0.01f, 100.0f);

			REGISTER_COMPONENT_META(FreeFly)
				SET_DEFAULT_FUNC(FreeFly)
				SET_MEMBER(FreeFly::isActive, "Is Active")
				SET_MEMBER(FreeFly::moveSpeed, "Move Speed")
				SET_MEMBER(FreeFly::rotateSpeed, "Rotate Speed");

			REGISTER_COMPONENT_META(Animator)
				SET_DEFAULT_FUNC(Animator)
				SET_MEMBER(Animator::animatorFileName, "Controller");

			REGISTER_COMPONENT_META(LightCommon)
				IS_HIDDEN()
				SET_DEFAULT_FUNC(LightCommon)
				SET_MEMBER(LightCommon::color, "Color")
				SET_MEMBER(LightCommon::intensity, "Intensity")
				SET_DRAG_SPEED(0.01f)
				SET_MEMBER(LightCommon::isOn, "Is On")
				SET_MEMBER(LightCommon::useShadow, "Use Shadow")
				SET_MEMBER(LightCommon::lightMode, "Light Mode");

			REGISTER_COMPONENT_META(DirectionalLight)
				NEED_COMPONENTS(LightCommon, DirectionalLight)
				SET_DEFAULT_FUNC(DirectionalLight)
				SET_MEMBER(DirectionalLight::cascadeEnds, "Cascade Ends");

			REGISTER_COMPONENT_META(PointLight)
				NEED_COMPONENTS(LightCommon, PointLight)
				SET_DEFAULT_FUNC(PointLight)
				SET_MEMBER(PointLight::attenuation, "Attenuation by distance")
				SET_MEMBER(PointLight::range, "Range")
				SET_MIN_MAX(2.0f, 10000.0f)
				SET_DRAG_SPEED(0.1f);

			REGISTER_COMPONENT_META(SpotLight)
				NEED_COMPONENTS(LightCommon, SpotLight)
				SET_DEFAULT_FUNC(SpotLight)
				SET_MEMBER(SpotLight::attenuation, "Attenuation by distance")
				SET_MEMBER(SpotLight::range, "Range")
				SET_MIN_MAX(10.0f, 10000.0f)
				SET_DRAG_SPEED(10.0f)
				SET_MEMBER(SpotLight::angularAttenuationIndex, "Attenuation by angle")
				SET_MEMBER(SpotLight::spotAngle, "Spot Angle")
				SET_MIN_MAX(1.0f, 179.0f)
				SET_DRAG_SPEED(1.0f)
				SET_MEMBER(SpotLight::innerAngle, "Inner Angle")
				SET_MIN_MAX(1.0f, 179.0f)
				SET_DRAG_SPEED(1.0f);

			REGISTER_COMPONENT_META(UICommon)
				IS_HIDDEN()
				SET_DEFAULT_FUNC(UICommon)
				SET_MEMBER(UICommon::isOn, "Is On")
				SET_MEMBER(UICommon::color, "Color")
				SET_MEMBER(UICommon::textureString, "UI Texture")
				SET_MEMBER(UICommon::maskingOption, "Masking Option");

			REGISTER_COMPONENT_META(UI2D)
				NEED_COMPONENTS(UICommon, UI2D)
				SET_DEFAULT_FUNC(UI2D)
				SET_MEMBER(UI2D::position, "Position")
				SET_MEMBER(UI2D::rotation, "Z Rotation")
				SET_MIN_MAX(-179.999f, 180.0f)
				SET_MEMBER(UI2D::size, "Size")
				SET_MEMBER(UI2D::layerDepth, "Layer Depth")
				SET_MEMBER(UI2D::isCanvas, "Is BG");

			REGISTER_COMPONENT_META(UI3D)
				NEED_COMPONENTS(UICommon, UI3D)
				SET_DEFAULT_FUNC(UI3D)
				SET_MEMBER(UI3D::isBillboard, "Is Billboard")
				SET_MEMBER(UI3D::constrainedOption, "Constrained Option")
				SET_DESC("빌보드 회전 축 고정");

			REGISTER_COMPONENT_META(Text)
				SET_DEFAULT_FUNC(Text)
				SET_MEMBER(Text::isOn, "Is On")
				SET_MEMBER(Text::color, "Color")
				SET_MEMBER(Text::position, "Position")
				SET_MEMBER(Text::size, "Text Box Size")
				SET_MEMBER(Text::scale, "Scale")
				SET_MEMBER(Text::fontString, "Font")
				SET_MEMBER(Text::fontSize, "Font Size")
				SET_MEMBER(Text::text, "Text")
				SET_MEMBER(Text::textAlign, "Text Align")
				SET_MEMBER(Text::textBoxAlign, "TextBox Align")
				SET_MEMBER(Text::leftPadding, "Left Padding")
				SET_MEMBER(Text::useUnderline, "Use Underline")
				SET_MEMBER(Text::useStrikeThrough, "Use StrikeThrough");

			REGISTER_COMPONENT_META(Button)
				SET_DEFAULT_FUNC(Button)
				SET_MEMBER(Button::interactable, "Interactable")
				SET_MEMBER(Button::onClickFunctions, "On Click Functions")
				SET_MEMBER(Button::highlightTextureString, "Highlight Texture")
				SET_MEMBER(Button::pressedTextureString, "Pressed Texture")
				SET_MEMBER(Button::selectedTextureString, "Selected Texture")
				SET_MEMBER(Button::disabledTextureString, "Disabled Texture");

			REGISTER_COMPONENT_META(LightMap)
				SET_DEFAULT_FUNC(LightMap)
				SET_MEMBER(LightMap::index, "Index")
				SET_MEMBER(LightMap::tilling, "Tilling")
				SET_MEMBER(LightMap::offset, "Offset");

			REGISTER_COMPONENT_META(NavMeshSettings)
				SET_DEFAULT_FUNC(NavMeshSettings)
				SET_MEMBER(NavMeshSettings::cellSize, "Cell Size")
				SET_MEMBER(NavMeshSettings::cellHeight, "Cell Height")
				SET_MEMBER(NavMeshSettings::agentRadius, "Agent Radius")
				SET_MEMBER(NavMeshSettings::agentHeight, "Agent Height")
				SET_MEMBER(NavMeshSettings::agentMaxClimb, "Agent Max Climb")
				SET_MEMBER(NavMeshSettings::agentMaxSlope, "Agent Max Slope")
				SET_MEMBER(NavMeshSettings::regionMinSize, "Region Min Size")
				SET_MEMBER(NavMeshSettings::regionMergeSize, "Region Merge Size")
				SET_MEMBER(NavMeshSettings::edgeMaxLen, "Edge Max Length")
				SET_MEMBER(NavMeshSettings::edgeMaxError, "Edge Max Error")
				SET_MEMBER(NavMeshSettings::vertsPerPoly, "Vertex Per Poly")
				SET_MEMBER(NavMeshSettings::detailSampleDist, "Detail Sample Distance")
				SET_MEMBER(NavMeshSettings::detailSampleMaxError, "Detail Sample Max Error")
				SET_MEMBER(NavMeshSettings::partitionType, "Parition Type")
				SET_MEMBER(NavMeshSettings::navMeshBMin, "NavMesh Box Min")
				SET_MEMBER(NavMeshSettings::navMeshBMax, "NavMesh Box Max")
				SET_MEMBER(NavMeshSettings::tileSize, "Tile Size");

			REGISTER_COMPONENT_META(Agent)
				SET_DEFAULT_FUNC(Agent)
				SET_MEMBER(Agent::radius, "Radius")
				SET_MEMBER(Agent::height, "Height")
				SET_MEMBER(Agent::speed, "Speed")
				SET_MEMBER(Agent::acceleration, "Acceleration")
				SET_MEMBER(Agent::collisionQueryRange, "Stopping Distance")
				SET_MEMBER(Agent::destination, "Destination")
				SET_MEMBER(Agent::isStopped, "Is Stopped");

			REGISTER_COMPONENT_META(ParticleSystem)
				SET_DEFAULT_FUNC(ParticleSystem)
				SET_MEMBER(ParticleSystem::mainData, "Main Data")
				SET_MEMBER(ParticleSystem::emissionData, "Emission Data")
				SET_MEMBER(ParticleSystem::shapeData, "Shape Data")
				SET_MEMBER(ParticleSystem::velocityOverLifeTimeData, "Velocity Over Life Time Data")
				SET_MEMBER(ParticleSystem::limitVelocityOverLifeTimeData, "Limit Velocity Over Life Time Data")
				SET_MEMBER(ParticleSystem::forceOverLifeTimeData, "Force Over Life Time Data")
				SET_MEMBER(ParticleSystem::colorOverLifeTimeData, "Color Over Life Time Data")
				SET_MEMBER(ParticleSystem::sizeOverLifeTimeData, "Size Over Life Time Data")
				SET_MEMBER(ParticleSystem::rotationOverLifeTimeData, "Rotation Over Life Time Data")
				SET_MEMBER(ParticleSystem::instanceData, "Instance Module")
				SET_MEMBER(ParticleSystem::renderData, "Render Module")
				SET_MEMBER(ParticleSystem::textureSheetAnimationData, "Texture Sheet Animation Data")
				SET_MEMBER(ParticleSystem::isOn, "Is On");

			REGISTER_COMPONENT_META(RenderAttributes)
				SET_DEFAULT_FUNC(RenderAttributes)
				SET_MEMBER(RenderAttributes::flags, "Flags");

			REGISTER_COMPONENT_META(Sound)
				SET_DEFAULT_FUNC(Sound)
				SET_MEMBER(Sound::path, "Path")
				SET_MEMBER(Sound::volume, "Volume")
				SET_MIN_MAX(0.0f, 1.0f)
				SET_DRAG_SPEED(0.01f)
				SET_MEMBER(Sound::pitch, "Pitch")
				SET_MIN_MAX(0.0f, 1.0f)
				SET_DRAG_SPEED(0.01f)
				SET_MEMBER(Sound::minDistance, "Min Distance")
				SET_DESC("최소 감쇠 거리, 거리 이내 최대 볼륨 유지")
				SET_MIN_MAX(1.0f, FLT_MAX)
				SET_MEMBER(Sound::maxDistance, "Max Distance")
				SET_DESC("최대 감쇠 거리, 거리 이후 최소 볼륨 유지")
				SET_MIN_MAX(1.0f, FLT_MAX)
				SET_MEMBER(Sound::isLoop, "Is Loop")
				SET_MEMBER(Sound::is3D, "Is 3D")
				SET_MEMBER(Sound::isPlaying, "Is Playing");

			REGISTER_COMPONENT_META(SoundListener)
				SET_DEFAULT_FUNC(SoundListener)
				SET_MEMBER(SoundListener::applyDoppler, "Apply Doppler")
				SET_DESC("도플러 효과 적용");

			REGISTER_COMPONENT_META(ComboBox)
				SET_DEFAULT_FUNC(ComboBox)
				SET_MEMBER(ComboBox::isOn, "Is On")
				SET_MEMBER(ComboBox::comboBoxTexts, "Texts")
				SET_MEMBER(ComboBox::curIndex, "Cur Index");

			REGISTER_COMPONENT_META(Slider)
				SET_DEFAULT_FUNC(Slider)
				SET_MEMBER(Slider::isOn, "Is On")
				SET_MEMBER(Slider::minMax, "Min Max")
				SET_MIN_MAX(0.001f, 10.0f)
				SET_DRAG_SPEED(0.01f)
				SET_MEMBER(Slider::curWeight, "Cur Weight")
				SET_DRAG_SPEED(0.01f)
				SET_MEMBER(Slider::sliderLayout, "Slider Layout")
				SET_MEMBER(Slider::sliderHandle, "Slider Handle")
				SET_MEMBER(Slider::isVertical, "Is Vertical");

			REGISTER_COMPONENT_META(CheckBox)
				NEED_COMPONENTS(Button, CheckBox)
				SET_DEFAULT_FUNC(CheckBox)
				SET_MEMBER(CheckBox::isChecked, "Is Checked")
				SET_MEMBER(CheckBox::checkedTextureString, "Checked Texture")
				SET_MEMBER(CheckBox::uncheckedTextureString, "Unchecked Texture");

			REGISTER_COMPONENT_META(PostProcessingVolume)
				SET_DEFAULT_FUNC(PostProcessingVolume)
				SET_MEMBER(PostProcessingVolume::useBloom, "Is On")
				SET_MEMBER(PostProcessingVolume::bloomCurve, "Bloom curve")
				SET_MEMBER(PostProcessingVolume::bloomTint, "Bloom Tint")
				SET_MEMBER(PostProcessingVolume::bloomIntensity, "Bloom Intensity")
				SET_MEMBER(PostProcessingVolume::bloomThreshold, "Bloom Threshold")
				SET_MIN_MAX(0.0f, 1.0f)
				SET_DRAG_SPEED(0.1f)
				SET_MEMBER(PostProcessingVolume::bloomScatter, "Bloom Scatter")
				SET_MIN_MAX(0.0f, 1.0f)
				SET_DRAG_SPEED(0.1f)
				SET_MEMBER(PostProcessingVolume::useBloomScatter, "Use Scatter")
				SET_MEMBER(PostProcessingVolume::useFog, "Use Fog")
				SET_MEMBER(PostProcessingVolume::fogColor, "Fog Color")
				SET_MEMBER(PostProcessingVolume::fogDensity, "Fog Density")
				SET_DESC("미구현")
				SET_MEMBER(PostProcessingVolume::fogStart, "Fog Start")
				SET_MEMBER(PostProcessingVolume::fogRange, "Fog Range")
				SET_MEMBER(PostProcessingVolume::useExposure, "Use Exposure")
				SET_MEMBER(PostProcessingVolume::exposure, "Exposure")
				SET_MIN_MAX(0.1f, 4.0f)
				SET_DRAG_SPEED(0.1f)
				SET_MEMBER(PostProcessingVolume::contrast, "Contrast")
				SET_MIN_MAX(-50.0f, 50.0f)
				SET_MEMBER(PostProcessingVolume::saturation, "Saturation")
				SET_MIN_MAX(-50.0f, 50.0f);
		}

		// 사용자 정의 struct 및 enum 등록
		{
			// struct
			{
				REGISTER_CUSTOM_STRUCT_META(ColliderCommon::PhysicMaterial)
					SET_NAME(ColliderCommon::PhysicMaterial)
					SET_MEMBER(ColliderCommon::PhysicMaterial::dynamicFriction, "Dynamic Friction")
					SET_DESC("설명: 물체가 움직일 때 표면 간의 마찰 계수\n\n용도: 객체가 다른 표면과 접촉할 때 마찰력을 결정합니다. 높은 값은 움직임을 저항합니다.")
					SET_MEMBER(ColliderCommon::PhysicMaterial::staticFriction, "Static Friction")
					SET_DESC("설명: 물체가 정지해 있을 때 표면 간의 마찰 계수\n\n용도: 객체가 다른 표면 위에 정지 상태를 유지하는 데 필요한 마찰력을 결정합니다. 높은 값은 객체가 쉽게 미끄러지지 않도록 합니다.")
					SET_MEMBER(ColliderCommon::PhysicMaterial::bounciness, "Bounciness")
					SET_DESC("설명: 표면 간의 반발력 계수\n\n용도: 충돌 시 물체가 얼마나 튀어오르는지를 결정합니다. 0이면 충돌 후 반발이 없고, 1이면 완전히 반발합니다.");

				REGISTER_CUSTOM_STRUCT_META(ParticleSystem::MainModule)
					SET_NAME(ParticleSystem::MainModule)
					SET_MEMBER(ParticleSystem::MainModule::duration, "Duration")
					SET_DESC("파티클 시스템의 지속 시간")
					SET_MEMBER(ParticleSystem::MainModule::isLooping, "Is Looping")
					SET_DESC("파티클 시스템이 루프되는지 여부")
					SET_MEMBER(ParticleSystem::MainModule::startDelayOption, "Start Delay Option")
					SET_DESC("파티클 방출 시작 시간 옵션")
					SET_MEMBER(ParticleSystem::MainModule::startDelay, "Start Delay")
					SET_DESC("파티클 방출 시작 시간")
					SET_MEMBER(ParticleSystem::MainModule::startLifeTimeOption, "Start Life Time Option")
					SET_DESC("파티클 생존 시간 옵션")
					SET_MEMBER(ParticleSystem::MainModule::startLifeTime, "Start Life Time")
					SET_DESC("파티클 생존 시간")
					SET_MEMBER(ParticleSystem::MainModule::startSpeedOption, "Start Speed Option")
					SET_DESC("파티클 방출 속도 옵션")
					SET_MEMBER(ParticleSystem::MainModule::startSpeed, "Start Speed")
					SET_DESC("파티클 방출 속도")
					SET_MEMBER(ParticleSystem::MainModule::startSizeOption, "Start Size Option")
					SET_DESC("파티클 크기 옵션")
					SET_MEMBER(ParticleSystem::MainModule::startSize, "Start Size")
					SET_DESC("파티클 크기")
					SET_MEMBER(ParticleSystem::MainModule::startRotationOption, "Start Rotation Option")
					SET_DESC("파티클 회전 옵션")
					SET_MEMBER(ParticleSystem::MainModule::startRotation, "Start Rotation")
					SET_DESC("파티클 회전")
					SET_MEMBER(ParticleSystem::MainModule::startColorOption, "Start Color Option")
					SET_DESC("파티클 색상 옵션")
					SET_MEMBER(ParticleSystem::MainModule::startColor0, "Start Color0")
					SET_DESC("파티클 색상")
					SET_MEMBER(ParticleSystem::MainModule::startColor1, "Start Color1")
					SET_DESC("파티클 색상")
					SET_MEMBER(ParticleSystem::MainModule::gravityModifierOption, "Gravity Modifier Option")
					SET_DESC("중력 가속도 옵션")
					SET_MEMBER(ParticleSystem::MainModule::gravityModifier, "Gravity Modifier")
					SET_DESC("중력 가속도")
					SET_MEMBER(ParticleSystem::MainModule::simulationSpeed, "Simulation Speed")
					SET_DESC("파티클 시스템의 시뮬레이션 속도")
					SET_MEMBER(ParticleSystem::MainModule::maxParticleCounts, "Max Particle Counts");

				REGISTER_CUSTOM_STRUCT_META(ParticleSystem::EmissionModule::Burst)
					SET_NAME(ParticleSystem::EmissionModule::Burst)
					SET_MEMBER(ParticleSystem::EmissionModule::Burst::timePos, "Time Pos")
					SET_DESC("처리할 시간")
					SET_MEMBER(ParticleSystem::EmissionModule::Burst::count, "Count")
					SET_DESC("방출할 파티클 수")
					SET_MEMBER(ParticleSystem::EmissionModule::Burst::cycles, "Cycles")
					SET_DESC("버스트 반복 횟수")
					SET_MEMBER(ParticleSystem::EmissionModule::Burst::interval, "Interval")
					SET_DRAG_SPEED(0.01f)
					SET_DESC("버스트 간격")
					SET_MEMBER(ParticleSystem::EmissionModule::Burst::probability, "Probability")
					SET_MIN_MAX(0.0f, 1.0f)
					SET_DRAG_SPEED(0.01f)
					SET_DESC("버스트 확률(0.0f ~ 1.0f)");

				REGISTER_CUSTOM_STRUCT_META(ParticleSystem::EmissionModule)
					SET_NAME(ParticleSystem::EmissionModule)
					SET_MEMBER(ParticleSystem::EmissionModule::particlePerSecond, "Particle Per Second")
					SET_DESC("초당 방출할 파티클 수")
					SET_MEMBER(ParticleSystem::EmissionModule::bursts, "Bursts");

				REGISTER_CUSTOM_STRUCT_META(ParticleSystem::ShapeModule)
					SET_NAME(ParticleSystem::ShapeModule)
					SET_MEMBER(ParticleSystem::ShapeModule::shapeType, "Shape Type")
					SET_MEMBER(ParticleSystem::ShapeModule::angleInDegree, "Angle In Degree")
					SET_MEMBER(ParticleSystem::ShapeModule::radius, "Radius")
					SET_MEMBER(ParticleSystem::ShapeModule::donutRadius, "Donut Radius")
					SET_MEMBER(ParticleSystem::ShapeModule::arcInDegree, "Arc In Degree")
					SET_MEMBER(ParticleSystem::ShapeModule::spread, "Spread")
					SET_MEMBER(ParticleSystem::ShapeModule::radiusThickness, "Radius Thickness")
					SET_MEMBER(ParticleSystem::ShapeModule::position, "Position")
					SET_MEMBER(ParticleSystem::ShapeModule::rotation, "Rotation")
					SET_MEMBER(ParticleSystem::ShapeModule::scale, "Scale");

				REGISTER_CUSTOM_STRUCT_META(ParticleSystem::VelocityOverLifeTimeModule)
					SET_NAME(ParticleSystem::VelocityOverLifeTimeModule)
					SET_MEMBER(ParticleSystem::VelocityOverLifeTimeModule::velocity, "Velocity")
					SET_MEMBER(ParticleSystem::VelocityOverLifeTimeModule::orbital, "Orbital")
					SET_MEMBER(ParticleSystem::VelocityOverLifeTimeModule::offset, "Offset")
					SET_MEMBER(ParticleSystem::VelocityOverLifeTimeModule::isUsed, "Is Used");

				REGISTER_CUSTOM_STRUCT_META(ParticleSystem::LimitVelocityOverLifeTimeModule)
					SET_NAME(ParticleSystem::LimitVelocityOverLifeTimeModule)
					SET_MEMBER(ParticleSystem::LimitVelocityOverLifeTimeModule::speed, "Speed")
					SET_MEMBER(ParticleSystem::LimitVelocityOverLifeTimeModule::dampen, "Dampen")
					SET_MEMBER(ParticleSystem::LimitVelocityOverLifeTimeModule::isUsed, "Is Used");

				REGISTER_CUSTOM_STRUCT_META(ParticleSystem::ForceOverLifeTimeModule)
					SET_NAME(ParticleSystem::ForceOverLifeTimeModule)
					SET_MEMBER(ParticleSystem::ForceOverLifeTimeModule::force, "Force")
					SET_MEMBER(ParticleSystem::ForceOverLifeTimeModule::isUsed, "Is Used");

				REGISTER_CUSTOM_STRUCT_META(ParticleSystem::ColorOverLifeTimeModule)
					SET_NAME(ParticleSystem::ColorOverLifeTimeModule)
					SET_MEMBER(ParticleSystem::ColorOverLifeTimeModule::alphaRatios, "Alpha Ratios")
					SET_MEMBER(ParticleSystem::ColorOverLifeTimeModule::colorRatios, "Color Ratios")
					SET_MEMBER(ParticleSystem::ColorOverLifeTimeModule::isUsed, "Is Used");

				REGISTER_CUSTOM_STRUCT_META(ParticleSystem::SizeOverLifeTimeModule)
					SET_NAME(ParticleSystem::SizeOverLifeTimeModule)
					SET_MEMBER(ParticleSystem::SizeOverLifeTimeModule::pointA, "Point A")
					SET_MEMBER(ParticleSystem::SizeOverLifeTimeModule::pointB, "Point B")
					SET_MEMBER(ParticleSystem::SizeOverLifeTimeModule::pointC, "Point C")
					SET_MEMBER(ParticleSystem::SizeOverLifeTimeModule::pointD, "Point D")
					SET_MEMBER(ParticleSystem::SizeOverLifeTimeModule::isUsed, "Is Used");

				REGISTER_CUSTOM_STRUCT_META(ParticleSystem::RotationOverLifeTimeModule)
					SET_NAME(ParticleSystem::RotationOverLifeTimeModule)
					SET_MEMBER(ParticleSystem::RotationOverLifeTimeModule::angularVelocityInDegree, "Angular Velocity in Degree")
					SET_MEMBER(ParticleSystem::RotationOverLifeTimeModule::isUsed, "Is Used");

				REGISTER_CUSTOM_STRUCT_META(ParticleSystem::InstanceModule)
					SET_NAME(ParticleSystem::InstanceModule)
					SET_MEMBER(ParticleSystem::InstanceModule::isEmit, "Is Emit")
					SET_MEMBER(ParticleSystem::InstanceModule::isReset, "Is Reset");

				REGISTER_CUSTOM_STRUCT_META(ParticleSystem::RenderModule)
					SET_NAME(ParticleSystem::RenderModule)
					SET_MEMBER(ParticleSystem::RenderModule::renderModeType, "Render Mode")
					SET_MEMBER(ParticleSystem::RenderModule::colorModeType, "Color Mode")
					SET_MEMBER(ParticleSystem::RenderModule::baseColor, "Base Color")
					SET_MEMBER(ParticleSystem::RenderModule::emissiveColor, "Emissive Color")
					SET_MEMBER(ParticleSystem::RenderModule::baseColorTextureString, "Particle Base Color Texture")
					SET_MEMBER(ParticleSystem::RenderModule::emissiveTextureString, "Particle Emissive Texture")
					SET_MEMBER(ParticleSystem::RenderModule::useBaseColorTexture, "Use Base Color Texture")
					SET_MEMBER(ParticleSystem::RenderModule::useEmissiveTexture, "Use Emissive Texture")
					SET_MEMBER(ParticleSystem::RenderModule::tiling, "Tiling")
					SET_MEMBER(ParticleSystem::RenderModule::offset, "Offset")
					SET_MEMBER(ParticleSystem::RenderModule::alphaCutOff, "Alpha Cutoff")
					SET_DESC("알파 컷오프 값")
					SET_MIN_MAX(0.0f, 1.0f)
					SET_DRAG_SPEED(0.01f)
					SET_MEMBER(ParticleSystem::RenderModule::isTwoSide, "Is Two Side")
					SET_DESC("양면 렌더링 여부")
					SET_MEMBER(ParticleSystem::RenderModule::useMultiplyAlpha, "Use Multiply Alpha");

				REGISTER_CUSTOM_STRUCT_META(ParticleSystem::TextureSheetAnimationModule)
					SET_NAME(ParticleSystem::TextureSheetAnimationModule)
					SET_MEMBER(ParticleSystem::TextureSheetAnimationModule::tiles, "Tiles")
					SET_MEMBER(ParticleSystem::TextureSheetAnimationModule::isUsed, "Is Used");

				REGISTER_CUSTOM_STRUCT_META(Vector2)
					SET_NAME(Vector2)
					SET_MEMBER(Vector2::x, "X")
					SET_MEMBER(Vector2::y, "Y");

				REGISTER_CUSTOM_STRUCT_META(Vector4)
					SET_NAME(Vector4)
					SET_MEMBER(Vector4::x, "X")
					SET_MEMBER(Vector4::y, "Y")
					SET_MEMBER(Vector4::z, "Z")
					SET_MEMBER(Vector4::w, "W");

				REGISTER_CUSTOM_STRUCT_META(Vector3)
					SET_NAME(Vector3)
					SET_MEMBER(Vector3::x, "X")
					SET_MEMBER(Vector3::y, "Y")
					SET_MEMBER(Vector3::z, "Z");

				REGISTER_CUSTOM_STRUCT_META(ComboBox::StringWrapper)
					SET_NAME(ComboBox::StringWrapper)
					SET_MEMBER(ComboBox::StringWrapper::text, "text");
			}

			// enum
			{
				REGISTER_CUSTOM_ENUM_META(CapsuleCollider::Axis)
					SET_NAME(CapsuleCollider::Axis)
					SET_VALUE(CapsuleCollider::Axis::X, "X")
					SET_VALUE(CapsuleCollider::Axis::Y, "Y")
					SET_VALUE(CapsuleCollider::Axis::Z, "Z");

				REGISTER_CUSTOM_ENUM_META(Rigidbody::Interpolation)
					SET_NAME(Rigidbody::Interpolation)
					SET_VALUE(Rigidbody::Interpolation::None, "None")
					SET_DESC("보간 없음")
					SET_VALUE(Rigidbody::Interpolation::Interpolate, "Interpolate")
					SET_DESC("Extrapolation 보다 부드럽지만 약간의 지연이 발생")
					SET_VALUE(Rigidbody::Interpolation::Extrapolate, "Extrapolate")
					SET_DESC("현재 속도를 기반으로 리지드바디의 위치를 예측");

				REGISTER_CUSTOM_ENUM_META(Rigidbody::Constraints)
					SET_TYPE(Rigidbody::Constraints)
					SET_VALUE(Rigidbody::Constraints::None, "None")
					SET_VALUE(Rigidbody::Constraints::FreezePositionX, "Freeze Position X")
					SET_VALUE(Rigidbody::Constraints::FreezePositionY, "Freeze Position Y")
					SET_VALUE(Rigidbody::Constraints::FreezePositionZ, "Freeze Position Z")
					SET_VALUE(Rigidbody::Constraints::FreezeRotationX, "Freeze Rotation X")
					SET_VALUE(Rigidbody::Constraints::FreezeRotationY, "Freeze Rotation Y")
					SET_VALUE(Rigidbody::Constraints::FreezeRotationZ, "Freeze Rotation Z")
					SET_VALUE(Rigidbody::Constraints::FreezePosition, "Freeze Position")
					SET_VALUE(Rigidbody::Constraints::FreezeRotation, "Freeze Rotation")
					SET_VALUE(Rigidbody::Constraints::FreezeAll, "Freeze All");

				REGISTER_CUSTOM_ENUM_META(MeshCollider::MeshColliderCookingOptions)
					SET_NAME(MeshCollider::MeshColliderCookingOptions)
					SET_VALUE(MeshCollider::CookForFasterSimulation, "Cook For Faster Simulation")
					SET_DESC("빠른 콜라이더 생성, 런타임 충돌 처리 성능 저하")
					SET_VALUE(MeshCollider::DisableMeshCleaning, "Disable Mesh Cleaning")
					SET_DESC("임의 메시 삭제 기능 제거, 빠른 콜라이더 생성")
					SET_VALUE(MeshCollider::WeldColocatedVertices, "Weld Colocated Vertices")
					SET_DESC("겹쳐지는 버텍스 병합 (DisableMeshCleaning 과 함께 사용 불가)")
					SET_VALUE(MeshCollider::UseLegacyMidphase, "Use Legacy Midphase")
					SET_DESC("기존 eBVH33 메시 구조로 사용 (호환성)")
					SET_VALUE(MeshCollider::BuildGPUData, "Build GPU Data")
					SET_DESC("GPU 가속 활성화, 런타임 성능 향상, 콜라이더 생성 속도 저하(Convex 플래그 필요)");

				REGISTER_CUSTOM_ENUM_META(LightCommon::LightMode)
					SET_NAME(LightCommon::LightMode)
					SET_VALUE(LightCommon::LightMode::REALTIME, "Realtime")
					SET_VALUE(LightCommon::LightMode::MIXED, "Mixed")
					SET_VALUE(LightCommon::LightMode::BAKED, "Baked");

				REGISTER_CUSTOM_ENUM_META(ParticleSystem::Option)
					SET_NAME(ParticleSystem::Option)
					SET_VALUE(ParticleSystem::Option::Constant, "Constant")
					SET_VALUE(ParticleSystem::Option::RandomBetweenTwoConstants, "Random Between Two Constants");

				REGISTER_CUSTOM_ENUM_META(ParticleSystem::ShapeModule::Shape)
					SET_NAME(ParticleSystem::ShapeModule::Shape)
					SET_VALUE(ParticleSystem::ShapeModule::Shape::Sphere, "Sphere")
					SET_VALUE(ParticleSystem::ShapeModule::Shape::Hemisphere, "Hemisphere")
					SET_VALUE(ParticleSystem::ShapeModule::Shape::Cone, "Cone")
					SET_VALUE(ParticleSystem::ShapeModule::Shape::Donut, "Donut")
					SET_VALUE(ParticleSystem::ShapeModule::Shape::Box, "Box")
					SET_VALUE(ParticleSystem::ShapeModule::Shape::Circle, "Circle")
					SET_VALUE(ParticleSystem::ShapeModule::Shape::Rectangle, "Rectangle");

				REGISTER_CUSTOM_ENUM_META(ParticleSystem::RenderModule::RenderMode)
					SET_NAME(ParticleSystem::RenderModule::RenderMode)
					SET_VALUE(ParticleSystem::RenderModule::RenderMode::Additive, "Additive")
					SET_VALUE(ParticleSystem::RenderModule::RenderMode::Subtractive, "Subtractive")
					SET_VALUE(ParticleSystem::RenderModule::RenderMode::Modulate, "Modulate")
					SET_VALUE(ParticleSystem::RenderModule::RenderMode::AlphaBlend, "Alpha Blend");

				REGISTER_CUSTOM_ENUM_META(ParticleSystem::RenderModule::ColorMode)
					SET_NAME(ParticleSystem::RenderModule::ColorMode)
					SET_VALUE(ParticleSystem::RenderModule::ColorMode::Multiply, "Multiply")
					SET_VALUE(ParticleSystem::RenderModule::ColorMode::Additive, "Additive")
					SET_VALUE(ParticleSystem::RenderModule::ColorMode::Subtractive, "Subtractive")
					SET_VALUE(ParticleSystem::RenderModule::ColorMode::Overlay, "Overlay")
					SET_VALUE(ParticleSystem::RenderModule::ColorMode::Color, "Color")
					SET_VALUE(ParticleSystem::RenderModule::ColorMode::Difference, "Difference");

				REGISTER_CUSTOM_ENUM_META(UICommon::MaskingOption)
					SET_NAME(UICommon::MaskingOption)
					SET_VALUE(UICommon::MaskingOption::None, "None")
					SET_VALUE(UICommon::MaskingOption::Circular, "Circular")
					SET_VALUE(UICommon::MaskingOption::Horizontal, "Horizontal");

				REGISTER_CUSTOM_ENUM_META(UI3D::Constrained)
					SET_NAME(UI3D::Constrained)
					SET_VALUE(UI3D::Constrained::None, "None")
					SET_VALUE(UI3D::Constrained::X, "X")
					SET_VALUE(UI3D::Constrained::Y, "Y")
					SET_VALUE(UI3D::Constrained::Z, "Z");

				REGISTER_CUSTOM_ENUM_META(Text::TextAlign)
					SET_NAME(Text::TextAlign)
					SET_VALUE(Text::TextAlign::LeftTop, "Left Top")
					SET_VALUE(Text::TextAlign::LeftCenter, "Left Center")
					SET_VALUE(Text::TextAlign::LeftBottom, "Left Bottom")
					SET_VALUE(Text::TextAlign::CenterTop, "Center Top")
					SET_VALUE(Text::TextAlign::CenterCenter, "Center Center")
					SET_VALUE(Text::TextAlign::CenterBottom, "Center Bottom")
					SET_VALUE(Text::TextAlign::RightTop, "Right Top")
					SET_VALUE(Text::TextAlign::RightCenter, "Right Center")
					SET_VALUE(Text::TextAlign::RightBottom, "Right Bottom");

				REGISTER_CUSTOM_ENUM_META(Text::TextBoxAlign)
					SET_NAME(Text::TextBoxAlign)
					SET_VALUE(Text::TextBoxAlign::LeftTop, "Left Top")
					SET_VALUE(Text::TextBoxAlign::CenterCenter, "Center Center");

				REGISTER_CUSTOM_ENUM_META(RenderAttributes::Flag)
					SET_NAME(RenderAttributes::Flag)
					SET_VALUE(RenderAttributes::Flag::CanReflect, "Can Reflect")
					SET_VALUE(RenderAttributes::Flag::CanRefract, "Can Refract")
					SET_VALUE(RenderAttributes::Flag::OutLine, "OutLine")
					SET_VALUE(RenderAttributes::Flag::IsTransparent, "Is Transparent")
					SET_VALUE(RenderAttributes::Flag::IsWater, "Is Water");

				REGISTER_CUSTOM_ENUM_META(PostProcessingVolume::BloomCurve)
					SET_NAME(PostProcessingVolume::BloomCurve)
					SET_VALUE(PostProcessingVolume::BloomCurve::Linear, "Linear")
					SET_VALUE(PostProcessingVolume::BloomCurve::Exponential, "Exponential")
					SET_VALUE(PostProcessingVolume::BloomCurve::Threshold, "Threshold");
			}
		}

		// 태그 메타 데이터 등록
		{
			REGISTER_TAG_META(tag::Untagged);
			REGISTER_TAG_META(tag::Respawn);
			REGISTER_TAG_META(tag::Finish);
			REGISTER_TAG_META(tag::MainCamera);
			REGISTER_TAG_META(tag::Player);
		}

		// 레이어 메타 데이터 등록 및 충돌 설정
		{
			REGISTER_LAYER_META(layer::Default);
			REGISTER_LAYER_META(layer::IgnoreRaycast);
			REGISTER_LAYER_META(layer::TransparentFX);
			REGISTER_LAYER_META(layer::UI);
		}
	}
}
