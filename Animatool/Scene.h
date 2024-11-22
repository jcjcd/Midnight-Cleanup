#pragma once
#include "Panel.h"
#include <Animacore/LightStructure.h>

class Renderer;
class Texture;
class Material;
class Mesh;


namespace core
{
	class Scene;
	class RenderSystem;
}

namespace tool
{
	class Camera
	{
	public:
		constexpr static float PI = 3.141592654f;
		Camera() = default;
		~Camera() = default;

		void SetLens();

		void SetProperty(float nNearClipPlane, float nFarClipPlane, float nAspect, float nFieldOfView, bool nOrthographic)
		{
			NearClipPlaneValue = nNearClipPlane;
			FarClipPlaneValue = nFarClipPlane;
			AspectValue = nAspect;
			FieldOfViewValue = nFieldOfView;
			OrthographicValue = nOrthographic;
			SetLens();
		}

		void putNearClipPlane(float nVal) { NearClipPlaneValue = nVal; SetLens(); }
		float getNearClipPlane() { return NearClipPlaneValue; }

		void putFarClipPlane(float nVal) { FarClipPlaneValue = nVal; SetLens(); }
		float getFarClipPlane() { return FarClipPlaneValue; }

		void putAspect(float nVal) { AspectValue = nVal; SetLens(); }
		float getAspect() { return AspectValue; }

		void putFieldOfView(float nVal) { FieldOfViewValue = nVal; SetLens(); }
		float getFieldOfView() { return FieldOfViewValue; }

		void putOrthographic(bool nVal) { OrthographicValue = nVal; SetLens(); }
		bool getOrthographic() { return OrthographicValue; }

		Matrix getViewMatrix() {
			Matrix world = Matrix::CreateTranslation(Position);
			Matrix rotation = Matrix::CreateFromQuaternion(Rotation);
			m_WorldMatrix = rotation * world;
			return m_WorldMatrix.Invert();
		}

		Matrix GetWorldMatrix() { return m_WorldMatrix; }


		void CameraMove(float deltaTime);
		void CameraDrag(float deltaTime);
		void CameraMoveRotate(float deltaTime);
		void UpdateMousePos(Vector2 mousePos) { m_PrevCur = mousePos; }

		__declspec(property(get = getNearClipPlane, put = putNearClipPlane)) float NearClipPlane;
		__declspec(property(get = getFarClipPlane, put = putFarClipPlane)) float FarClipPlane;
		__declspec(property(get = getAspect, put = putAspect)) float Aspect;
		__declspec(property(get = getFieldOfView, put = putFieldOfView)) float FieldOfView;
		__declspec(property(get = getOrthographic, put = putOrthographic)) bool Orthographic;

		__declspec(property(get = getViewMatrix)) Matrix ViewMatrix;


		Matrix ProjectionMatrix = Matrix::Identity;

		Vector3 Position = Vector3::Zero;
		Quaternion Rotation = Quaternion::Identity;

		float size = 2;

		// culling
		DirectX::BoundingFrustum frustum;

		Vector3 targetPosition;
		bool isMoving = false;

		float Speed = 20.f;
		float SpeedDesplacement = 1.f;
		float MinSpeed = 0.1f;
		float MaxSpeed = 30.f;

	private:
		float NearClipPlaneValue = 0.001f;
		float FarClipPlaneValue = 500.f;
		float AspectValue = 0.1f;
		float FieldOfViewValue = 45.0f * (PI / 180.0f);

		bool OrthographicValue = false;

		Matrix m_WorldMatrix = Matrix::Identity;

		Vector2 m_PrevCur = {};

	};

	class SceneViewNavBar;

	class Scene : public Panel
	{
	public:
		constexpr static inline uint32_t MAX_POINTSHADOW_COUNT = 3;
		constexpr static inline uint32_t MAX_SHADOW_COUNT = 1;

		enum GizmoOperation
		{
			Hand = 0,
			Move = 7,
			Rotate = 120,
			Scale = 896
		};

		enum Mode
		{
			Local,
			World
		};

		explicit Scene(entt::dispatcher& dispatcher, Renderer* renderer);
		~Scene() override;

		void InitializeShaderResources();
		void InitializeRaytracingResources();
		void RenderPanel(float deltaTime) override;
		PanelType GetType() override { return PanelType::Scene; }

		void RenderBoundingBox(core::Scene& scene);
		void RenderScene(float deltaTime, core::Scene& scene);
		void RenderGizmo(core::Scene& scene);
		void RenderRaytracingScene(core::Scene& scene);

		void Resize(LONG width, LONG height);

		Vector2 viewportPos = { 0, 0 };
		Vector2 viewportSize = { 0, 0 };

	private:
		void pickItemByCPU(int mouseXPos, int mouseYPos);
		void renderProfiler(float deltaTime);

		void selectEntity(const OnToolSelectEntity& event);
		void playScene(const OnToolPlayScene& event);
		void stopScene(const OnToolStopScene& event);
		void doubleClickEntity(const OnToolDoubleClickEntity& event);
		void modifyRenderer();
		void applyCamera(const OnToolApplyCamera& event);

		void initDeferredResources();
		void finishDeferredResources();

		void initLightResources();
		void finishLightResources();

		void initPostProcessResources();
		void finishPostProcessResources();



		void shadowPass(core::Scene& scene, const Matrix& view, const Matrix& proj, const Matrix& viewProj);
		void deferredGeometryPass(core::Scene& scene, const Matrix& view, const Matrix& proj, const Matrix& viewProj);

		// Transparent
		void initOITResources();
		void oitPass(core::Scene& scene, const Matrix& view, const Matrix& proj, const Matrix& viewProj);
		void oitCompositePass(core::Scene& scene, const Matrix& view, const Matrix& proj, const Matrix& viewProj);


		// Post Process
		void decalPass(core::Scene& scene, const Matrix& view, const Matrix& proj, const Matrix& viewProj);
		void reflectionPass(core::Scene& scene, const Matrix& view, const Matrix& proj, const Matrix& viewProj);
		


	private:
		Renderer* _renderer = nullptr;

		bool _isPlaying = false;
		bool _isFocused = false;

		SceneViewNavBar* _navBar = nullptr;
		GizmoOperation _currentGizmoOperation = GizmoOperation::Move;
		Mode _currentMode = Mode::Local;

		Camera _camera;		

		bool _isSceneRightClicked = false;
		bool _isRightClicked = false;

		bool _isOrthographic = false;

		// 더블클릭해서 슝 날아가는거
		bool _isDoubleClicked = false;
		float _doubleClickTime = 0.0f;
		float _scaleAvg = 0.0f;
		Vector3 _targetWorldPosition{};

		bool _useSnap = false;
		Vector3 _snap = Vector3::One;

		bool _manageUI = true;
		bool _useLightMap = true;
		bool _showCollider = true;

		// dxr
		bool _isRendererModified = false;

		std::vector<core::DirectionalLightStructure> _directionalLights;
		//std::vector<core::DirectionalLightStructure> _nonShadowDirectionalLights;
		std::vector<core::PointLightStructure> _pointLights;
		std::vector<core::PointLightStructure> _nonShadowPointLights;
		std::unordered_map<entt::entity, core::PointLightStructure> _pointLightMap;
		std::set<entt::entity> _pointLightEntities;
		std::queue<entt::entity> _pointLightQueue;
		std::vector<core::SpotLightStructure> _spotLights;
		//std::vector<core::SpotLightStructure> _nonShadowSpotLights;
		int _directionalShadowCount = 0;
		int _pointShadowCount = 0;
		int _spotShadowCount = 0;


		std::shared_ptr<Material> _pickingMaterial;
		std::shared_ptr<Material> _outlineComputeMaterial;
		std::shared_ptr<Material> _quadMaterial;
		std::shared_ptr<Material> _dDepthMaterial;
		std::shared_ptr<Material> _sDepthMaterial;
		std::shared_ptr<Material> _pDepthMaterial;
		std::shared_ptr<Material> _deferredMaterial;
		std::shared_ptr<Material> _uiMaterial;

		std::shared_ptr<Texture> _renderTarget;
		std::shared_ptr<Texture> _deferredOutputTexture;
		std::shared_ptr<Texture> _pickingTexture;
		std::shared_ptr<Texture> _depthTexture;

		/// 반사
		std::shared_ptr<Texture> _reflectionUVTexture;
		std::shared_ptr<Material> _reflectionUVMaterial;

		std::shared_ptr<Texture> _reflectionColorTexture;
		std::shared_ptr<Material> _reflectionColorMaterial;

		std::shared_ptr<Texture> _reflectionBlurTexture;
		std::shared_ptr<Material> _reflectionBlurMaterial;

		std::shared_ptr<Texture> _reflectionCombineTexture;
		std::shared_ptr<Material> _reflectionCombineMaterial;

		/// OIT
		std::shared_ptr<Texture> _revealageTexture;
		std::shared_ptr<Texture> _accumulationTexture;

		/// OIT - 합치기
		std::shared_ptr<Material> _oitCombineMaterial;

		/// 3D text
		std::shared_ptr<Texture> _textTexture;

		// 합치기 
		std::shared_ptr<Material> _combineMaterial;

		std::vector<entt::entity> _selectedEntities;

		std::shared_ptr<Mesh> _quadMesh;

		std::shared_ptr<Texture> _dLightDepthTexture;
		std::shared_ptr<Texture> _sLightDepthTexture;
		std::shared_ptr<Texture> _pLightDepthTexture;
		std::shared_ptr<Texture> _pLightDepthTextureRTs;

		std::vector<std::shared_ptr<Texture>> _deferredTextures;

		std::shared_ptr<Texture> _decalOutputAlbedo;
		std::shared_ptr<Texture> _decalOutputORM;

		// IBL
		std::shared_ptr<Texture> _irradianceTexture;
		std::shared_ptr<Texture> _prefilteredTexture;
		std::shared_ptr<Texture> _brdfTexture;


		// 프레임 측정
		bool _isProfiling = false;
		float _totalTime = 0.f;

		// 프레임 측정을 위한 변수 추가
		int _frameCount = 0;
		float _frameTime = 0.0f;
		float _fps = 0.0f;
		float _minFrameTime = FLT_MAX;
		float _maxFrameTime = 0.0f;


		inline static constexpr float _refreshTime = 1 / 60.f;
		float _refreshTimer = 0.0f;

		// resize
		LONG _width = 1920;
		LONG _height = 1080;		// 디폴트 머티리얼을 그냥 박아버리기위한 변수
		std::string _defaultMaterialName;

		// tool option
		bool _isMouseCursorPrevVisible = true;

		friend class SceneViewNavBar;

#pragma region Deprecated
		// 굴절 // 이번 프로젝트에서 굴절은 뺀다.
		// 속도가 느려진다. 그리고 딱히 필요가 없어보인다.
// 		std::shared_ptr<Texture> _refractionPositionTexture;
// 
// 		std::shared_ptr<Texture> _refractionUVTexture;
// 		std::shared_ptr<Material> _refractionUVMaterial;
// 		std::shared_ptr<Texture> _positionFromTexture;
// 		std::shared_ptr<Texture> _normalFromTexture;
// 		std::shared_ptr<Texture> _dummyAlbedoTexture;
// 		std::shared_ptr<Texture> _dummy1;
// 		std::shared_ptr<Texture> _dummy2;
// 		std::shared_ptr<Texture> _dummy3;
// 		std::shared_ptr<Texture> _dummy4;
// 
// 		std::shared_ptr<Material> _refractionColorMaterial;
// 		std::shared_ptr<Texture> _refractionColorTexture;
#pragma endregion
	};
}

