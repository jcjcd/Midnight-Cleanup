#pragma once
#include "AnimatorParameter.h"
#include "AnimatorState.h"
#include "ButtonEvent.h"

class Mesh;
class Material;
class Texture;
struct NodeClip;
struct Font;

namespace tool
{
	class AnimatorPanel;
}

namespace core
{
	struct Animator;

	struct LocalTransform;
	struct WorldTransform;

	/*------------------------------
		MeshRenderer
	------------------------------*/
	struct MeshRenderer
	{
		std::string meshString;
		std::shared_ptr<Mesh> mesh;
		std::list<std::string> materialStrings;
		std::vector<std::shared_ptr<Material>> materials;

		bool isOn = true;
		bool receiveShadow = true;
		bool isSkinned = false;
		bool isCulling = true;
		bool canReceivingDecal = true;


		// �ý��ۿ��� ���
		Animator* animator = nullptr;
		uint32_t hitDistribution = 0;;
		bool isCustom = false;
		bool isForward = false;

		// submesh[ bone [] ]
		std::vector<std::vector<WorldTransform*>> bones;
		std::vector<std::vector<Matrix>> boneOffsets;

		float emissiveFactor = 1.0f;
	};

	struct RenderAttributes
	{
		enum Flag
		{
			None = 0,
			CanReflect = (1 << 0),	
			CanRefract = (1 << 1),		
			OutLine = (1 << 2),	
			IsTransparent = (1 << 3),
			IsWater = (1 << 4),
		};

		Flag flags = None;
	};

	/*------------------------------
		Camera
	------------------------------*/
	struct Camera
	{
		bool isActive = true;

		bool isOrthographic = false;

		float fieldOfView = 0.785f;
		float nearClip = 0.01f;
		float farClip = 10000.0f;
		float orthographicSize = 5.0f;

		float aspectRatio = 1.6f;
	};

	struct FreeFly
	{
		bool isActive = true;

		float moveSpeed = 10.0f;
		float rotateSpeed = 0.1f;

		Vector2 prevMousePos;
	};


	/*------------------------------
		Animator
	------------------------------*/
	struct Animator
	{
		// �ý��� ���� ����
		using StateMap = std::unordered_map<std::string, AnimatorState>;

		std::string animatorFileName;

		std::vector<std::pair<LocalTransform*, std::shared_ptr<NodeClip>>>* _currentNodeClips;
		std::vector<Matrix> _boneOffsets;

		// �ִϸ�������Ʈ�ѷ� ���Ͽ� �ִ� �Ķ������ �纻
		std::unordered_map<std::string, AnimatorParameter> parameters;

	private:
		// �ؿ������� �������� ���ε��� ���� �ʿ��Ѱ͵�
		std::unordered_map<std::string, std::pair<LocalTransform*, WorldTransform*>>  _boneMap;

		std::vector<MeshRenderer*> _meshRenderers;

		// ����ȭ�� ��� Ŭ����. �̰ɷ� �ٷ� �����Ҽ� �ֵ��� �������.
		std::unordered_map<std::string, std::vector<std::pair<LocalTransform*, std::shared_ptr<NodeClip>>>> _nodeClips;

		// ���� �ִϸ��̼��� ��� Ŭ���� ĳ�� ������ �Ѵٰ� �����
		std::vector<std::pair<LocalTransform*, std::shared_ptr<NodeClip>>>* _nextNodeClips;

		AnimatorState* _currentState;
		AnimatorState* _nextState;

		AnimatorTransition* _currentTransition;

		StateMap* _states = nullptr;

		AnimatorState* _anyState = nullptr;


		float _currentTimePos = 0.f;
		float _nextTimePos = 0.f;
		float _currentBlendTime = 0.f;

		float _currentDuration = 0.f;
		float _nextDuration = 0.f;

		friend class AnimatorController;
		friend class AnimatorSystem;

		friend class tool::AnimatorPanel;
	};

	/*------------------------------
		Decal
	------------------------------*/
	struct Decal
	{
		float fadeFactor = 1.f;
		Vector2 size = { 1.0f, 1.0f };

		std::string materialString;
		std::shared_ptr<Material> material;
	};

	/*------------------------------
		Light
	------------------------------*/
	struct LightCommon
	{
		Color color = { 1.0f, 1.0f, 1.0f, 1.0f };
		float intensity = 1.0f;
		bool isOn = true;
		bool useShadow = true;

		enum class LightMode
		{
			REALTIME,
			MIXED,
			BAKED,
		};

		LightMode lightMode = LightMode::REALTIME;
	};

	struct DirectionalLight
	{
		Vector3 cascadeEnds = { 0.067f, 0.2f, 0.467f };
	};

	struct PointLight
	{
		Vector3 attenuation = { 1.0f, 0.0f, 0.0f };
		float range = 10.0f;
	};

	struct SpotLight
	{
		Vector3 attenuation = { 1.0f, 0.0f, 0.0f };
		float range = 10.0f;
		float angularAttenuationIndex = 1.0f;
		float spotAngle = 60.0f;
		float innerAngle = 30.0f;
	};

	struct LightMap
	{
		uint32_t index = 0;
		Vector2 tilling = Vector2::Zero;
		Vector2 offset = Vector2::Zero;
		bool isOn = true;
	};

	/*------------------------------
		UI Common
	------------------------------*/

	struct UICommon
	{
		bool isOn = true;
		Color color = { 1.0f, 1.0f, 1.0f, 1.0f };
		std::string textureString;
		std::shared_ptr<Texture> texture;

		// ��ô��, �����
		float percentage = 1.0f;

		enum class MaskingOption
		{
			None = 0,
			Circular,
			Horizontal,
		};
		MaskingOption maskingOption = MaskingOption::None;
	};

	/*------------------------------
		UI
	------------------------------*/
	struct UI2D
	{
		Vector2 position = { 0.0f, 0.0f };
		float rotation = 0.0f;
		Vector2 size = { 100.0f, 100.0f };
		uint32_t layerDepth = 0;
		Vector2 toolVPSize = { 1.0f, 1.0f };
		bool isCanvas = false;
	};

	/*------------------------------
		UI3D
	------------------------------*/

	struct UI3D
	{
		bool isBillboard = false;

		enum class Constrained
		{
			None = 0,
			X,
			Y,
			Z
		};

		Constrained constrainedOption = Constrained::None;
	};

	/*------------------------------
		Text
	------------------------------*/

	struct Text
	{
		bool isOn = true;
		Color color = { 1.0f, 1.0f, 1.0f, 1.0f };
		Vector2 position = { 0.0f, 0.0f };
		Vector2 size = { 100.0f, 100.0f };
		Vector2 scale = { 1.0f, 1.0f };
		std::string fontString;
		std::shared_ptr<Font> font;
		float fontSize = 32.0f;
		std::string text;

		enum class TextAlign
		{
			LeftTop = 0,
			LeftCenter,
			LeftBottom,
			CenterTop,
			CenterCenter,
			CenterBottom,
			RightTop,
			RightCenter,
			RightBottom
		};

		enum class TextBoxAlign
		{
			LeftTop = 0,
			CenterCenter,
		};

		TextAlign textAlign = TextAlign::CenterCenter;
		TextBoxAlign textBoxAlign = TextBoxAlign::CenterCenter;
		float leftPadding = 0.0f;
		bool useUnderline = false;
		bool useStrikeThrough = false;
	};

	/*------------------------------
		Button
	------------------------------*/
	struct Button
	{

		bool interactable = true;
		std::vector<ButtonEvent> onClickFunctions;

		std::string highlightTextureString;
		std::string pressedTextureString;
		std::string selectedTextureString;
		std::string disabledTextureString;
		std::string defaultTextureString;

		// �ø�������� ���� �ʰ� ���������� ��� 
		// �ý����� ��ŸƮ �κп��� �� �ʱ�ȭ ����.
		std::shared_ptr<Texture> highlightTexture;
		std::shared_ptr<Texture> pressedTexture;
		std::shared_ptr<Texture> selectedTexture;
		std::shared_ptr<Texture> disabledTexture;

		// UI2D�� ���� �ؽ���
		std::shared_ptr<Texture> originalTexture;

		bool isHovered = false;
		bool isPressed = false;
	};


	/*------------------------------
		Particle System
	------------------------------*/
	struct ParticleSystem
	{
		enum { MAX_PARTICLE_COUNT = 1000 };

		enum class Option
		{
			Constant,
			RandomBetweenTwoConstants,
		};

		struct MainModule
		{
			float duration = 5.0f;
			bool isLooping = true;

			Option startDelayOption = Option::Constant;
			Vector2 startDelay = Vector2(0.0f, 0.0f);	// ���� ���� �ð�

			Option startLifeTimeOption = Option::Constant;
			Vector2 startLifeTime = Vector2(5.0f, 5.0f);	// ��ƼŬ ���� �ð�

			Option startSpeedOption = Option::Constant;
			Vector2 startSpeed = Vector2(5.0f, 5.0f);	// ���� �ӵ�

			Option startSizeOption = Option::Constant;
			Vector2 startSize = Vector2(1.0f, 1.0f);	// ��ƼŬ ũ��

			Option startRotationOption = Option::Constant;
			Vector2 startRotation = Vector2(0.0f, 0.0f);	// ��ƼŬ ȸ��

			Option startColorOption = Option::Constant;
			Vector4 startColor0 = Vector4(1.0f, 1.0f, 1.0f, 1.0f);	// ��ƼŬ ����
			Vector4 startColor1 = Vector4(1.0f, 1.0f, 1.0f, 1.0f);	// ��ƼŬ ����

			Option gravityModifierOption = Option::Constant;
			Vector2 gravityModifier = Vector2(0.0f, 0.0f);	// �߷� ���ӵ�

			float simulationSpeed = 1.0f;

			uint32_t maxParticleCounts = MAX_PARTICLE_COUNT;
		} mainData;

		struct EmissionModule
		{
			float particlePerSecond = 10.0f;

			struct Burst
			{
				float timePos = 0.0f;	// ó���� �ð�
				uint32_t count = 30;	// ������ ��ƼŬ ��
				uint32_t cycles = 1;	// ����Ʈ �ݺ� Ƚ��
				float interval = 0.01f;	// ����Ʈ ����
				float probability = 1.0f;	// ����Ʈ Ȯ��(0.0f ~ 1.0f)
			};

			std::vector<Burst> bursts;
		} emissionData;

		struct ShapeModule
		{
			enum class Shape
			{
				Sphere,
				Hemisphere,
				Cone,
				Donut,
				Box,
				Circle,
				Rectangle,
				// Edge
			};

			enum class Mode
			{
				Random,
				// Loop,
				// PingPong,
				// BurstSpread
			};

			Shape shapeType = Shape::Cone;
			//Mode modeType = Mode::Random;	// ���� ���
			float angleInDegree = 25.0f;	// ���� ����
			float radius = 1.0f;
			float donutRadius = 0.2f;
			float arcInDegree = 360.0f;
			float radiusThickness = 1.0f;	// �Թ�ü ������ ����� ������(0.0f ~ 1.0f)
			float spread = 0.0f;

			Vector3 position;
			Vector3 rotation;
			Vector3 scale{ 1.0f, 1.0f, 1.0f };

		} shapeData;

		struct VelocityOverLifeTimeModule
		{
			Vector3 velocity = Vector3(0.0f, 0.0f, 0.0f);
			Vector3 orbital = Vector3(0.0f, 0.0f, 0.0f);
			Vector3 offset = Vector3(0.0f, 0.0f, 0.0f);
			bool isUsed = false;

		} velocityOverLifeTimeData;

		struct LimitVelocityOverLifeTimeModule
		{
			float speed = 1.0f;
			float dampen = 0.0f;	// ���� �ӵ� �ʰ� �� ���� ����
			bool isUsed = false;

		} limitVelocityOverLifeTimeData;

		struct ForceOverLifeTimeModule
		{
			Vector3 force = Vector3(0.0f, 0.0f, 0.0f);
			bool isUsed = false;

		} forceOverLifeTimeData;

		struct ColorOverLifeTimeModule
		{
			std::vector<Vector2> alphaRatios{ Vector2(1.0f, 0.0f), };				// value, ratio
			std::vector<Vector4> colorRatios{ Vector4(1.0f, 1.0f, 1.0f, 1.0f), };	// { value }, ratio
			bool isUsed = false;

		} colorOverLifeTimeData;

		struct SizeOverLifeTimeModule
		{
			Vector2 pointA;		// { ratio, size }
			Vector2 pointB;
			Vector2 pointC;
			Vector2 pointD;
			bool isUsed = false;

		} sizeOverLifeTimeData;

		struct RotationOverLifeTimeModule
		{
			float angularVelocityInDegree = 45.0f;
			bool isUsed = false;

		} rotationOverLifeTimeData;

		struct InstanceModule
		{
			bool isEmit = true;
			bool isReset = true;

		} instanceData;

		struct RenderModule
		{
			enum class RenderMode
			{
				Additive,
				Subtractive,
				Modulate,
				AlphaBlend,
			} renderModeType = RenderMode::Additive;

			enum class ColorMode
			{
				Multiply,
				Additive,
				Subtractive,
				Overlay, // 
				Color, // ���� �ؽ�ó ���Ŀ� ������ �˺��� ����
				Difference // 
			} colorModeType = ColorMode::Multiply;

			Color baseColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
			Color emissiveColor = Color(0.0f, 0.0f, 0.0f, 0.0f);

			std::string baseColorTextureString;
			std::string emissiveTextureString;

			bool useBaseColorTexture = true;
			bool useEmissiveTexture = false;

			Vector2 tiling = Vector2(1.0f, 1.0f);
			Vector2 offset = Vector2(0.0f, 0.0f);

			float alphaCutOff = 0.1f;
			bool isTwoSide = false;
			bool useMultiplyAlpha = true;
		} renderData;

		struct TextureSheetAnimationModule
		{
			Vector2 tiles = Vector2(1.0f, 1.0f);
			bool isUsed = false;
		} textureSheetAnimationData;

		std::shared_ptr<Texture> baseColorTexture;
		std::shared_ptr<Texture> emissiveTexture;

		bool isOn = true;
	};

	/*------------------------------
		����Ÿ�� �̱��� ������Ʈ
	------------------------------*/
	// ����Ÿ���� ������ ������ ����, �̱��� ������Ʈ
	struct RenderResources
	{
		std::shared_ptr<Texture> renderTarget;
		std::shared_ptr<Texture> depthTexture;

		std::vector<std::shared_ptr<Texture>> deferredTextures;

		// ī�޶� ĳ�̿�
		Matrix viewMatrix;
		Matrix projectionMatrix;
		Matrix viewProjectionMatrix;
		Camera* mainCamera;
		WorldTransform* cameraTransform;

		// �޽� ĳ�̿�
		std::shared_ptr<Material> quadMaterial;
		std::shared_ptr<Mesh> quadMesh;
		std::shared_ptr<Mesh> cubeMesh;

		// ȭ�� ũ��
		UINT width;
		UINT height;
	};

	/*------------------------------
		ComboBox
	------------------------------*/
	struct ComboBox
	{
		struct StringWrapper
		{
			std::string text;
		};

		bool isOn = false;
		std::vector<StringWrapper> comboBoxTexts;
		uint32_t curIndex = 0;
	};

	/*------------------------------
		Slider
	------------------------------*/
	struct Slider
	{
		bool isOn = true;
		Vector2 minMax = { 0.1f,2.0f };
		float curWeight = 1.0f;
		entt::entity sliderLayout = entt::null;
		entt::entity sliderHandle = entt::null;
		bool isVertical = false;
	};

	/*------------------------------
		Check Box
	------------------------------*/
	struct CheckBox
	{
		bool isChecked = false;
		std::string checkedTextureString;
		std::string uncheckedTextureString;
	};
	
	/*------------------------------
		Post Processing Volume
	------------------------------*/
	struct PostProcessingVolume
	{
		enum class BloomCurve
		{
			Linear,
			Exponential,
			Threshold,
		};

		bool useBloom = false;
		BloomCurve bloomCurve = BloomCurve::Linear;
		Color bloomTint = { 1.0f, 1.0f, 1.0f, 1.0f };
		float bloomIntensity = 1.0f;
		float bloomThreshold = 1.0f;
		float bloomScatter = 0.5f;
		bool useBloomScatter = false;
		bool useFog = false;
		Color fogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
		float fogDensity = 0.01f;
		float fogStart = 10.0f;
		float fogRange = 300.0f;
		bool useExposure = false;
		float exposure = 1.0f;
		float contrast = 0.0f;
		float saturation = 0.0f;
	};
};