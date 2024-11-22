#pragma once

#include <Animacore/RaycastHit.h>

#include "Animatool/SceneViewNavBar.h"

struct Font;

namespace mc
{
	/*------------------------------
		CharacterMovement
	------------------------------*/
	struct CharacterMovement
	{
		float speed = 5.0f;
		float jump = 2.0f;
		float dropAcceleration = 0.5f;
		float jumpAcceleration = 0.5f;
		Vector3 hDir;
		Vector3 vDir;

		float currentJumpVelocity; // 현재 점프 속도
		float currentJumpHeight;   // 현재 점프한 높이
		bool isJumping; // 점프 중 여부
	};

	/*------------------------------
		Tool
	------------------------------*/
	struct Tool
	{
		enum Type
		{
			DEFAULT,	// 빈 손
			AXE,		// 도끼
			MOP,		// 대걸레
			SPONGE,		// 스펀지
			FLASHLIGHT,	// 손전등
		};

		Type type = Type::DEFAULT;

		Tool& operator=(const Type type) { this->type = type; return *this; }
		bool operator==(const Type type) const { return this->type == type; }
	};

	/*------------------------------
		Durability
	------------------------------*/
	struct Durability
	{
		// -1 : 무한대
		int maxDurability = 0;

		// 현재 내구도
		int currentDurability = 0;
	};

	/*------------------------------
		WaterBucket
	------------------------------*/
	struct WaterBucket
	{
		bool isFilled = false;
	};

	/*------------------------------
		Inventory
	------------------------------*/
	struct Inventory
	{
		union ToolSlot
		{
			struct
			{
				entt::entity tool1;
				entt::entity tool2;
				entt::entity tool3;
				entt::entity tool4;
			};
			entt::entity tools[4];
		};

		ToolSlot toolSlot;
		Tool::Type toolType[4];
		uint32_t currentTool = 0;
	};

	/*------------------------------
		Picking
	------------------------------*/
	struct Picking
	{
		entt::entity handSpot = entt::null;
		entt::entity pickingObject = entt::null;
		bool isPickingStart = false;

		entt::id_type priorityTarget = entt::null;
	};

	/*------------------------------
		Radial UI
	------------------------------*/
	struct RadialUI
	{
		// 비트 연산
		enum Type
		{
			DEFAULT = 0 << 0,
			TEXT = 1 << 0,
			ARROW = 1 << 1,
			LEFT = 1 << 2,
			RIGHT = 1 << 3,
			TOP = 1 << 4,
			BOTTOM = 1 << 5,
			INVENTORY = 1 << 6,
			DIRECTION = 1 << 7,
			INVENTORY_LEFT = INVENTORY | LEFT,
			INVENTORY_RIGHT = INVENTORY | RIGHT,
			INVENTORY_TOP = INVENTORY | TOP,
			INVENTORY_BOTTOM = INVENTORY | BOTTOM,
			DIRECTION_LEFT = DIRECTION | LEFT,
			DIRECTION_RIGHT = DIRECTION | RIGHT,
			DIRECTION_TOP = DIRECTION | TOP,
			DIRECTION_BOTTOM = DIRECTION | BOTTOM,
		};

		Type type = Type::DEFAULT;
	};

	/*------------------------------
		RayCastingInfo
	------------------------------*/
	struct RayCastingInfo
	{
		std::vector<physics::RaycastHit> hits;
		float interactableRange = 10.0f;

		bool rayHologram = false;
	};

	/*------------------------------
		PivotSlot
	------------------------------*/
	struct PivotSlot
	{
		bool isUsing = true;

		Vector3 pivotPosition;
		Quaternion pivotRotation;
		Vector3 pivotScale;
	};

	/*------------------------------
		Stain
	-------------------------------*/
	struct Stain
	{
		bool isSpongeStain = false;
		bool isMopStain = false;
	};

	/*------------------------------
		GameManager
	-------------------------------*/
	struct GameManager
	{
		float timeLimit = 5.0f;
		bool hasAttended = false;
		bool isGameOver = false;
		bool isTruckOpen = false;
	};

	/*------------------------------
		Snap
	-------------------------------*/
	// 피킹 가능한 물체에 붙이는 컴포넌트
	struct SnapAvailable
	{
		entt::entity placedSpot = entt::null;		// 스냅될 위치
		float length = 1.0f;						// 홀로그램 활성화 거리
		bool isSnapped = false;						// 스냅 상태
	};

	/*------------------------------
		TrashCommon
	-------------------------------*/
	struct DestroyableTrash
	{
		bool isGrabbed = false;
		bool inTrashBox = false;
		float remainTime = 5.0f;

		static constexpr float destroyTime = 5.0f;
	};


	/*------------------------------
		TrashCommon
	-------------------------------*/
	struct Room
	{
		enum Type
		{
			None,
			Bathroom,
			BreakRoom,
			Corridor,
			GuestRoom,
			Kitchen,
			LaundryRoom,
			LivingRoom,
			Outdoor,
			Porch,
			SmallRoom,
			Storage,
			End
		};

		static constexpr const char* name[]
		{
			"없음",
			"화장실",
			"휴게실",
			"복도",
			"응접실",
			"부엌",
			"세탁실",
			"거실",
			"야외",
			"현관",
			"작은방",
			"창고",
		};

		Type type = None;
	};

	/*------------------------------
		TrashCommon
	-------------------------------*/
	struct Door
	{
		enum State
		{
			None,
			OpenPositiveX,
			OpenPositiveZ,
			OpenNegativeX,
			OpenNegativeZ,
			ClosePositiveX,
			ClosePositiveZ,
			CloseNegativeX,
			CloseNegativeZ
		};

		State state = None;

		bool isMoving = false;
		Vector3 destination;
		Vector3 direction;

		static constexpr float SPEED_WEIGHT = 2.0f;
	};

	/*------------------------------
		Quest - 전역 컴포넌트
	-------------------------------*/
	struct Quest
	{
		// 방 구분
		uint32_t cleanedMopStainsInRoom[Room::End] = {};
		uint32_t totalMopStainsInRoom[Room::End] = {};
		uint32_t cleanedSpongeStainInRoom[Room::End] = {};
		uint32_t totalSpongeStainInRoom[Room::End] = {};
		uint32_t cleanedFurnitureInRoom[Room::End] = {};
		uint32_t totalFurnitureInRoom[Room::End] = {};
		uint32_t cleanedMessInRoom[Room::End] = {};
		uint32_t totalMessInRoom[Room::End] = {};

		// 전체
		uint32_t cleanedTrashes = 0;
		uint32_t totalTrashes = 0;

		float progressPercentage = 0.f;

	private:
		entt::entity _trashBar = entt::null;
		entt::entity _trashCounter = entt::null;

		entt::entity _mopStainBar = entt::null;
		entt::entity _mopStainCounter = entt::null;

		entt::entity _spongeStainBar = entt::null;
		entt::entity _spongeStainCounter = entt::null;

		entt::entity _messBar = entt::null;
		entt::entity _messCounter = entt::null;

		entt::entity _furnitureBar = entt::null;
		entt::entity _furnitureCounter = entt::null;

		entt::entity _progressBar = entt::null;
		entt::entity _progressCounter = entt::null;

		friend class InGameUiSystem;
		friend class RoomEnterSystem;
	};

	struct DissolveRenderer
	{
		float dissolveFactor = 0.0f;
		float dissolveSpeed = 1.0f;
		Vector3 dissolveColor = Vector3(1.0f, 0.0f, 0.0f);
		float edgeWidth = 0.5f;

		bool isPlaying = false;
	};

	struct Switch
	{
		struct Lights
		{
			entt::entity light0 = entt::null;
			entt::entity light1 = entt::null;
			entt::entity light2 = entt::null;
		};

		bool isOn = false;
		Lights lights;
		entt::entity lever = entt::null;
	};

	struct Flashlight
	{
		bool isOn = false;
	};

	struct TrashGenerator
	{
		enum Trash
		{
			Bag1,
			Bag2,
			Bottle1,
			Bottle2,
			Can1,
			Can2,
			Can3,
			CupNoodle,
			Milk,
			End,
		};

		static constexpr const char* trashPrefabs[End]
		{
			"./Resources/Prefabs/Trash/Bag1.prefab",
			"./Resources/Prefabs/Trash/Bag2.prefab",
			"./Resources/Prefabs/Trash/Bottle1.prefab",
			"./Resources/Prefabs/Trash/Bottle2.prefab",
			"./Resources/Prefabs/Trash/Can1.prefab",
			"./Resources/Prefabs/Trash/Can2.prefab",
			"./Resources/Prefabs/Trash/Can3.prefab",
			"./Resources/Prefabs/Trash/CupNoodle.prefab",
			"./Resources/Prefabs/Trash/Milk.prefab",
		};

		uint32_t trashCount = 3;
	};

	struct Mop
	{
		// 한번에 지워질 데칼의 수
		int eraseCount = 3;

		// 내부 변수
		std::vector<entt::entity> interactingEntities;

	};

	struct Sponge
	{
		int spongePower = 100;
	};

	struct TVTimer
	{
		uint32_t minute = 0;
		uint32_t second = 0;
	};

	// singleton
	struct GameSetting
	{
		int masterVolume = 100;
		int mouseSensitivity = 5;
		int fov = 135;
		int invertYAxis = 0;
	};

	struct GameSettingController
	{
		entt::entity screenResolution = entt::null;
		entt::entity vSync = entt::null;

		entt::entity masterVolume = entt::null;
		entt::entity mouseSensitivity = entt::null;
		entt::entity fov = entt::null;
		entt::entity invertYAxis = entt::null;

	};

	struct MopStainGenerator
	{
		enum Stain
		{
			Decal_Mop_Additional_Mop,

			Decal_Mop_Big_Additional_Bucket,
			Decal_Mop_Big_Beginning_01,
			Decal_Mop_Big_Beginning_02,
			Decal_Mop_Big_Beginning_03,
			Decal_Mop_Big_Beginning_04,
			Decal_Mop_Big_Beginning_06,
			Decal_Mop_Big_Rubbed_01,

			Decal_Mop_Mid_Beginning_05,
			Decal_Mop_Mid_Drip_03,
			Decal_Mop_Mid_Mold_01,
			Decal_Mop_Mid_Splash_03,

			Decal_Mop_Small_Drip_01,
			Decal_Mop_Small_Drip_02,
			Decal_Mop_Small_Foot_Left,
			Decal_Mop_Small_Foot_Right,
			Decal_Mop_Small_Hand_Left,
			Decal_Mop_Small_Splash_01,

			End
		};

		static constexpr const char* stainPrefabs[End]
		{
			"./Resources/Prefabs/Stain/Decal_Mop_Additional_Mop.prefab",

			"./Resources/Prefabs/Stain/Decal_Mop_Big_Additional_Bucket.prefab",
			"./Resources/Prefabs/Stain/Decal_Mop_Big_Beginning_01.prefab",
			"./Resources/Prefabs/Stain/Decal_Mop_Big_Beginning_02.prefab",
			"./Resources/Prefabs/Stain/Decal_Mop_Big_Beginning_03.prefab",
			"./Resources/Prefabs/Stain/Decal_Mop_Big_Beginning_04.prefab",
			"./Resources/Prefabs/Stain/Decal_Mop_Big_Beginning_06.prefab",
			"./Resources/Prefabs/Stain/Decal_Mop_Big_Rubbed_01.prefab",

			"./Resources/Prefabs/Stain/Decal_Mop_Mid_Beginning_05.prefab",
			"./Resources/Prefabs/Stain/Decal_Mop_Mid_Drip_03.prefab",
			"./Resources/Prefabs/Stain/Decal_Mop_Mid_Mold_01.prefab",
			"./Resources/Prefabs/Stain/Decal_Mop_Mid_Splash_03.prefab",

			"./Resources/Prefabs/Stain/Decal_Mop_Small_Drip_01.prefab",
			"./Resources/Prefabs/Stain/Decal_Mop_Small_Drip_02.prefab",
			"./Resources/Prefabs/Stain/Decal_Mop_Small_Foot_Left.prefab",
			"./Resources/Prefabs/Stain/Decal_Mop_Small_Foot_Right.prefab",
			"./Resources/Prefabs/Stain/Decal_Mop_Small_Hand_Left.prefab",
			"./Resources/Prefabs/Stain/Decal_Mop_Small_Splash_01.prefab",
		};

		uint32_t stainCount = 3;
	};

	struct SoundMaterial
	{
		enum Type
		{
			DEFAULT,
			GROUND,
			WOODEN,
			TRUCK,
			TATAMI
		};

		Type type = Type::DEFAULT;
	};

	struct CaptionLog
	{
		enum Text
		{
			Caption_GameIn,
			Caption_ClickTerminal,
			Caption_AfterClickTerminal,

			Caption_NotTakeAnyTool,

			Caption_FurnitureFirst,
			Caption_FurnitureNotTakeAxe,
			Caption_MopStainFirst,
			Caption_MopStainNotTakeMop,
			Caption_MopDirty,
			Caption_MopGotoOutdoor,
			Caption_MopGetWater,
			Caption_MopMakeClean,
			Caption_MopDirtyWater,
			Caption_SpongeStainFirst,
			Caption_SpongeStainNotTakeSponge,
			Caption_TrashFirst,
			Caption_TrashWithNoHands,
			Caption_AfterPickingTrash,
			Caption_MessFirst,

			Caption_HouseEnter,

			Caption_TimeLimit_SevenMinute_Left,
			Caption_TimeLimit_FiveMinute_Left,
			Caption_TimeLimit_ThreeMinute_Left,
			Caption_TimeLimit_OneMinute_Left,
			Caption_TimeLimit_ThirtySecond_Left,
			Caption_TimeLimit_TenSecond_Left,
			Caption_End
		};

		static constexpr const char* captions[Caption_End]
		{
			"청소 의뢰 들어왔다. 잘 해봐.",			// Caption_GameIn				게임 시작시 1	OK 
			"출구 왼쪽 단말기에서 청소 시작 등록해.",	// Caption_ClickTerminal		게임 시작시 2	OK
			"뒤에 시간 보이지? 늦지 않게 돌아와.",		// Caption_AfterClickTerminal	단말기 클릭시		OK

			"도구는 챙겼어? 후레쉬도 챙겨.",			// Caption_NotTakeTool			트럭에서 처음 나갈시					OK

			"스티커 붙은 가구는 폐기할거야 도끼로 부숴.",	// Caption_FurnitureFirst	가구를 처음 보았을 경우				OK
			"도끼를 안 가져왔다고? 빨리 트럭에 가서 챙겨.",	// Caption_FurnitureNotTakeAxe	도끼를 안가져 왔을 경우			OK
			"걸레질 좀 해야겠어.",						// Caption_MopStainFirst	걸레 얼룩을 처음 보았을 경우			OK
			"걸레 안 챙겼어? 트럭에 있으니 가져와.",		// Caption_MopStainNotTakeMop	걸레를 안가져 왔을 경우			OK
			"그런 더러운 걸레로는 안 돼. 빨아와.",			// Caption_MopDirty				더러운 걸레로 얼룩을 생성했을 경우 1		OK
			"걸레는 밖에서 빨 수 있어. 마당으로 가봐.",	// Caption_MopGotoOutdoor		더러운 걸레로 얼룩을 생성했을 경우 2		OK
			"수돗가 옆의 물통을 집어서 물을 받아.",		// Caption_MopGetWater			얼룩을 생성한 이후에 수돗가로 왔을 경우		OK
			"이제 걸레를 빨아봐.",						// Caption_MopMakeClean			물통을 받아서 새로운 물을 받았을 경우		OK
			"물이 더럽잖아. 새로 받아.",					// Caption_MopDirtyWater		더러운 물통에 걸레를 빨았을 경우				
			"낙서는 스펀지로 닦아야겠지?",					// Caption_SpongeStainFirst		스펀지 얼룩을 처음 보았을 경우		OK
			"스펀지도 챙겼어야지. 빨리 트럭에 갔다오도록.",	// Caption_SpongeStainNotTakeSponge	스펀지를 안가져 왔을 경우		OK
			"바닥에 쓰레기가 보이면 꼭 집어서 버려.",		// Caption_TrashFirst		쓰레기를 처음 보았을 경우				OK
			"빈 손이어야 쓰레기를 줍지",					// Caption_TrashWithNoHands	무언가를 든 손으로 처음 쓰레기를 주으려 했을 경우	OK
			"쓰레기는 야외 초록색 쓰레기통에 버려.",		// Caption_AfterPickingTrash	쓰레기를 처음 집었을 경우			OK
			"어질러진 물건은 제자리에 정리해야해.",		// Caption_MessFirst		물건을 처음 보았을 경우				OK	

			"어두워서 안 보이지? 벽에 스위치 찾아봐.",		// Caption_HouseEnter		건물에 처음 들어갔을 경우				OK

			"7분 남았어. 시간 확인해.",				// Caption_TimeLimit_SevenMinute_Left
			"5분 남았어. 서둘러야지.",				// Caption_TimeLimit_FiveMinute_Left
			"3분 남았어. 정신 바짝 차려.",			// Caption_TimeLimit_ThreeMinute_Left
			"1분 남았어. 마무리해.",					// Caption_TimeLimit_OneMinute_Left
			"30초 남았어. 갈 준비 해.",			// Caption_TimeLimit_ThirtySecond_Left
			"10초 남았어. 이제 끝내야 해.",			// Caption_TimeLimit_TenSecond_Left
		};

		bool isShown[Caption_End] = { false };
	};

	struct VideoRenderer
	{
		std::string videoPath;
		bool isPlaying = false;
		bool isLoop = false;
		bool isFinished = false;
	};

	struct StandardScore
	{
		float timeBonusMaxLimitTime = 200.f;
		float timeBonusTimeMinLimitTime = 100.f;

		int bonusPerGhostEvent = 300;
		int bonusPerTrash = 300;
		int bonusPerPercentage = 20;
		int bonusPerTime = 10;
		int minTimeBonus = 500;

		int penaltyAbandoned = 1000;
		int penaltyPerMess = 40;

		int fiveStarMinScore = 10500;
		int fourStarMinScore = 9000;
		int threeStarMinScore = 7000;
		int twoStarMinScore = 5500;
		int oneStarMinScore = 3200;
	};
}

