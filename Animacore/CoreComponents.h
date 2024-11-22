#pragma once
#include "NavConfigs.h"

#define KEY(keyCode) static_cast<int>(keyCode)

namespace FMOD
{
	class Sound;
	class Channel;
}

namespace tool
{
	class ComponentDataPanel;
	class Inspector;
}

namespace core
{
	/*------------------------------
		Transform
	------------------------------*/
	struct Transform { int dummy = 0; };

	struct LocalTransform
	{
		Vector3 position;
		Quaternion rotation;
		Vector3 scale = Vector3::One;
		Matrix matrix;
	};

	struct WorldTransform
	{
		Vector3 position;
		Quaternion rotation;
		Vector3 scale = Vector3::One;
		Matrix matrix;
	};



	/*------------------------------
		Relationship
	------------------------------*/
	struct Relationship
	{
		entt::entity parent = entt::null;
		std::vector<entt::entity> children;
	};


	/*------------------------------
		Name
	------------------------------*/
	struct Name
	{
		std::string name = "New Entity";
	};

	/*------------------------------
		NavMeshConfigs
	------------------------------*/
	// NavMesh 관련 설정 및 데이터
	struct NavMeshConfigs
	{
		std::string meshName;
		NavMeshSettings settings;

	private:
		NavMeshData _meshData;

		friend class PathFindingSystem;
	};


	/*------------------------------
		Agent
	------------------------------*/
	// NavMesh 상에 존재하는 객체 생성
	struct Agent
	{
		float radius = 0.5f;				// 에이전트 반지름
		float height = 2.0f;				// 에이전트 높이
		float speed = 1.0f;					// 이동 속도
		float acceleration = 8.0f;			// 가속도
		float collisionQueryRange = 0.5f;	// 충돌 탐지 거리
		bool isStopped = false;				// 에이전트가 멈추는지 여부

		Vector3 destination;           // 목표 위치

	private:
		unsigned int _polyRef = 0;

		friend class PathFindingSystem;
	};


	/*------------------------------
		Sound
	------------------------------*/
	struct Sound
	{
		std::string path;
		float volume = 1.0f;     // 사운드 볼륨 (0.0 ~ 1.0)
		float pitch = 1.0f;      // 사운드의 피치 (기본값 1.0)

		float minDistance = 1.f;	// 최소 감쇠 거리
		float maxDistance = 1000.f;	// 최대 감쇠 거리

		bool isLoop = false;
		bool is3D = false;
		bool isPlaying = false;

	private:
		FMOD::Channel* _channel = nullptr;

		friend class SoundSystem;
	};

	struct SoundListener
	{

		bool applyDoppler = true;

	private:
		int _index = -1;

		friend class SoundSystem;
	};



	/*------------------------------
		Input
	------------------------------*/
	// 키보드/마우스 입력, 싱글톤 컴포넌트
	struct Input
	{
		enum class State
		{
			None,
			Down,
			Hold,
			Up,
		};

		std::array<State, 256> keyStates;
		
		Vector2 viewportPosition;
		Vector2 viewportSize;

		int mouseWheel;
		Vector2 mousePosition;
		Vector2 mouseDeltaPosition;
		Vector2 mouseDeltaRotation;

	private:
		std::array<bool, 256> _prevPushed;
		Vector2 _prevMousePosition;

		friend class InputSystem;
	};


	/*------------------------------
		Configuration
	------------------------------*/
	// 게임 관련 전역적 설정, 싱글톤 컴포넌트
	struct Configuration
	{
		uint32_t width = 1920;
		uint32_t height = 1080;

		bool isFullScreen = false;
		bool isWindowedFullScreen = false;

		bool enableVSync = false;
	};

	struct BGM
	{
		core::Sound sound;
	};
}





