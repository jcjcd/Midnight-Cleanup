#include "pch.h"
#include "TimeLimitSystem.h"
#include "McComponents.h"
#include "McTagsAndLayers.h"

#include <Animacore/Scene.h>
#include <Animacore/RenderComponents.h>

#include "McEvents.h"
#include "Animacore/CoreComponents.h"
#include "Animacore/CoreComponentInlines.h"
#include "Animacore/CoreTagsAndLayers.h"
#include <Animacore/InputSystem.h>


mc::TimeLimitSystem::TimeLimitSystem(core::Scene& scene)
	: ISystem(scene)
{
	_dispatcher = scene.GetDispatcher();
	_dispatcher->sink<core::OnStartSystem>().connect<&TimeLimitSystem::startSystem>(this);
	_dispatcher->sink<core::OnFinishSystem>().connect<&TimeLimitSystem::finishSystem>(this);
	_dispatcher->sink<OnGameOver>().connect<&TimeLimitSystem::gameOver>(this);
	_dispatcher->sink<mc::OnTriggerGhostEvent>().connect<&TimeLimitSystem::triggerGhostEvent>(this);
}

mc::TimeLimitSystem::~TimeLimitSystem()
{
	_dispatcher->disconnect(this);
}

void mc::TimeLimitSystem::operator()(core::Scene& scene, float tick)
{
	auto& registry = *scene.GetRegistry();

	auto& gameManager = registry.get<mc::GameManager>(_gameManager);

	if (!gameManager.hasAttended)
	{
		auto& renderAttributes = registry.get<core::RenderAttributes>(_gameManager);
		_blinkTimeAccum += tick;
		if (_blinkTimeAccum > _blinkInterval)
		{
			if (renderAttributes.flags & core::RenderAttributes::Flag::OutLine &&
				!_isBlinkOn)
			{
				_blinkTimeAccum -= _blinkInterval;
			}
			else
			{
				renderAttributes.flags ^= core::RenderAttributes::Flag::OutLine;
				_isBlinkOn = !_isBlinkOn;
				_blinkTimeAccum -= _blinkInterval;
			}
		}
		return;
	}

	auto& tvTimer = registry.get<TVTimer>(_tv);
	auto& tvText = registry.get<core::Text>(_tv);

	// 제한 시간이 존재
	// 제한 시간을 00 : 00 형식으로 표시 -> ui 렌더로(빌보드)
	// 그러면 tick을 분 : 초 형식으로 그려야함
	uint32_t minute = static_cast<uint32_t>(_timeLimit) / 60;
	uint32_t second = static_cast<uint32_t>(_timeLimit) % 60;
	_timeLimit -= tick * scene.GetTimeScale();

	tvTimer.minute = minute;
	tvTimer.second = second;

	tvText.text = std::format("{:01d}:{:02d}", minute, second);

	// 제한 시간이 0이 되기 전에 퇴근 처리를 하지 않으면 게임 오버 -> 정산 페널티
	// 그 전에 트럭에 들어가있거나 안에서 퇴근 처리를 하면 페널티 없음

	// 제한시간은 현재 10분으로 설정
	// 5분, 3분, 1분, 10 ~ 0초에 사운드 출력

	auto& captionLog = registry.ctx().get<mc::CaptionLog>();

	if (_timeLimit <= 10 && !captionLog.isShown[CaptionLog::Caption_TimeLimit_TenSecond_Left])
	{
		// 10초
		_dispatcher->enqueue<OnShowCaption>(scene, CaptionLog::Caption_TimeLimit_TenSecond_Left, 3.0f);
		captionLog.isShown[CaptionLog::Caption_TimeLimit_TenSecond_Left] = true;
	}
	if (_timeLimit <= 30 && !captionLog.isShown[CaptionLog::Caption_TimeLimit_ThirtySecond_Left])
	{
		// 30초
		_dispatcher->enqueue<OnShowCaption>(scene, CaptionLog::Caption_TimeLimit_ThirtySecond_Left, 3.0f);
		captionLog.isShown[CaptionLog::Caption_TimeLimit_ThirtySecond_Left] = true;
	}
	else if (_timeLimit <= 60 && !captionLog.isShown[CaptionLog::Caption_TimeLimit_OneMinute_Left])
	{
		// 1분
		_dispatcher->enqueue<OnShowCaption>(scene, CaptionLog::Caption_TimeLimit_OneMinute_Left, 3.0f);
		captionLog.isShown[CaptionLog::Caption_TimeLimit_OneMinute_Left] = true;
	}
	else if (_timeLimit <= 180 && !captionLog.isShown[CaptionLog::Caption_TimeLimit_ThreeMinute_Left])
	{
		// 3분
		_dispatcher->enqueue<OnShowCaption>(scene, CaptionLog::Caption_TimeLimit_ThreeMinute_Left, 3.0f);
		captionLog.isShown[CaptionLog::Caption_TimeLimit_ThreeMinute_Left] = true;
	}
	else if (_timeLimit <= 300 && !captionLog.isShown[CaptionLog::Caption_TimeLimit_FiveMinute_Left])
	{
		// 5분
		_dispatcher->enqueue<OnShowCaption>(scene, CaptionLog::Caption_TimeLimit_FiveMinute_Left, 3.0f);
		captionLog.isShown[CaptionLog::Caption_TimeLimit_FiveMinute_Left] = true;
	}
	else if (_timeLimit <= 420 && !captionLog.isShown[CaptionLog::Caption_TimeLimit_SevenMinute_Left])
	{
		// 7분
		_dispatcher->enqueue<OnShowCaption>(scene, CaptionLog::Caption_TimeLimit_SevenMinute_Left, 3.0f);
		captionLog.isShown[CaptionLog::Caption_TimeLimit_SevenMinute_Left] = true;
	}

	if (_timeLimit <= 0 && !gameManager.isGameOver)
		gameManager.isGameOver = true;
}

void mc::TimeLimitSystem::operator()(core::Scene& scene)
{
	auto& registry = *scene.GetRegistry();

	auto& gameManager = registry.get<mc::GameManager>(_gameManager);

	if (!gameManager.hasAttended)
		return;

	if (!gameManager.isTruckOpen)
	{
		_openTimeElapsed += FIXED_TIME_STEP;

		if (_openTimeElapsed <= 2.0f)
			openDoor(registry, _openTimeElapsed);
		else
		{
			gameManager.isTruckOpen = true;
			registry.patch<mc::GameManager>(_gameManager);
		}
	}

	if (gameManager.isGameOver)
	{
		auto& input = registry.ctx().get<core::Input>();

		_closeTimeElapsed += FIXED_TIME_STEP;

		if (_closeTimeElapsed < 6.0f)
			closeDoor(registry, _closeTimeElapsed);
		else
		{
			if (!_isTriggered)
			{
				_dispatcher->trigger<OnGameOver>(scene);
				_isTriggered = true;
			}

			if (input.keyStates[VK_ESCAPE] == core::Input::State::Down)
				_dispatcher->enqueue<core::OnChangeScene>(core::OnChangeScene{ "./Resources/Scenes/title.scene" });
		}
	}
}

void mc::TimeLimitSystem::startSystem(const core::OnStartSystem& event)
{
	{
		auto view = event.scene->GetRegistry()->view<mc::GameManager>();
		for (auto&& [entity, gameManager] : view.each())
		{
			_gameManager = entity;
			_timeLimit = gameManager.timeLimit * 60;
			gameManager.isGameOver = false;
		}
	}
	if (_gameManager == entt::null)
		LOG_ERROR(*event.scene, "GameManager Component is not found");

	{
		auto view = event.scene->GetRegistry()->view<mc::TVTimer>();
		for (auto&& [entity, tvTimer] : view.each())
			_tv = entity;
	}
	if (_tv == entt::null)
		LOG_ERROR(*event.scene, "TVTimer Component is not found");

	{
		for (auto&& [entity, tag] : event.scene->GetRegistry()->view<core::Tag>().each())
		{
			if (tag.id == tag::TruckDoorL::id)
				_truckDoorL = entity;
			else if (tag.id == tag::TruckDoorR::id)
				_truckDoorR = entity;
			else if (tag.id == tag::EndingCamera::id)
				_endingCamera = entity;
			if (tag.id == tag::TruckPedestal::id)
			{
				_truckPedestal = entity;
				_defaultPosittionZ = event.scene->GetRegistry()->get<core::WorldTransform>(entity).position.z;
				_defaultRotation = event.scene->GetRegistry()->get<core::WorldTransform>(entity).rotation;
			}
		}
	}
	if (_truckDoorL == entt::null || _truckDoorR == entt::null || _endingCamera == entt::null || _truckPedestal == entt::null)
		LOG_ERROR(*event.scene, "TruckDoor or EndingCamera is not found");
}

void mc::TimeLimitSystem::finishSystem(const core::OnFinishSystem& event)
{
	_timeLimit = 0;
	_tv = entt::null;
	_gameManager = entt::null;
	_truckDoorL = entt::null;
	_truckDoorR = entt::null;
	_endingCamera = entt::null;
}

void mc::TimeLimitSystem::gameOver(const OnGameOver& event)
{
	auto& registry = *event.scene->GetRegistry();

	auto& gameManger = registry.get<mc::GameManager>(_gameManager);
	gameManger.isGameOver = true;
	const auto& score = registry.get<mc::StandardScore>(_gameManager);

	for (auto&& [entity, sound] : registry.view<core::Sound>().each())
	{
		sound.isPlaying = false;
		sound.isLoop = false;
		registry.patch<core::Sound>(entity);
	}

	// 관련 팝업 프리팹 생성
	entt::entity popup = event.scene->LoadPrefab("Resources/Prefabs/reward.prefab");
	auto& relationship = registry.get<core::Relationship>(popup);
	core::UICommon* stars[5] = { nullptr, };
	core::Text* workPayText = nullptr;
	core::Text* totalText = nullptr;
	core::Text* commentText = nullptr;
	core::Text* finalPercentage = nullptr;
	core::Text* cleanBonusText = nullptr;
	core::Text* messBounusText = nullptr;
	core::Text* abandonedBonusText = nullptr;
	core::Text* ghostEventBonusText = nullptr;
	core::Text* percentageBonusText = nullptr;
	float percentage = 0.0f;

	uint32_t completedCount = 0;

	const auto quest = registry.ctx().get<mc::Quest>();
	auto percentMulHundred = quest.progressPercentage * 100;

	for (auto& child : relationship.children)
	{
		if (auto tag = registry.try_get<core::Tag>(child))
		{
			if (tag->id == tag::Star0::id)
				stars[0] = registry.try_get<core::UICommon>(child);
			else if (tag->id == tag::Star1::id)
				stars[1] = registry.try_get<core::UICommon>(child);
			else if (tag->id == tag::Star2::id)
				stars[2] = registry.try_get<core::UICommon>(child);
			else if (tag->id == tag::Star3::id)
				stars[3] = registry.try_get<core::UICommon>(child);
			else if (tag->id == tag::Star4::id)
				stars[4] = registry.try_get<core::UICommon>(child);
			else if (tag->id == tag::WorkPay::id)
				workPayText = registry.try_get<core::Text>(child);
			else if (tag->id == tag::TotalPay::id)
				totalText = registry.try_get<core::Text>(child);
			else if (tag->id == tag::TotalComment::id)
				commentText = registry.try_get<core::Text>(child);
			else if (tag->id == tag::FinalPercentage::id)
				finalPercentage = registry.try_get<core::Text>(child);
			else if (tag->id == tag::CleanPlusWage::id)
				cleanBonusText = registry.try_get<core::Text>(child);
			else if (tag->id == tag::MessBonus::id)
				messBounusText = registry.try_get<core::Text>(child);
			else if (tag->id == tag::AbandonedBonus::id)
				abandonedBonusText = registry.try_get<core::Text>(child);
			else if (tag->id == tag::GhostEventBonus::id)
				ghostEventBonusText = registry.try_get<core::Text>(child);
			else if (tag->id == tag::PercentageBonus::id)
				percentageBonusText = registry.try_get<core::Text>(child);
		}
	}

	int trashBonus = quest.cleanedTrashes * score.bonusPerTrash;
	int percentageBonus = percentMulHundred * score.bonusPerPercentage;
	int messBonus;
	int abandonedBonus;
	int ghostBonus = 0;

	int cleanedMessCount = 0;
	int totalMessCount = 0;
	for (auto i = 0 ; i < Room::End; ++i)
	{
		cleanedMessCount += quest.cleanedMessInRoom[i];
		totalMessCount += quest.totalMessInRoom[i];
	}
	messBonus = (cleanedMessCount - totalMessCount) * score.penaltyPerMess;

	for (auto&& [entity, movement, world] : registry.view<CharacterMovement, core::WorldTransform>().each())
	{
		if (world.position.x < -4.0f)
			abandonedBonus = 0;
		else
			abandonedBonus = -score.penaltyAbandoned;
	}

	int timeBonus;
	if (_timeLimit > score.timeBonusMaxLimitTime)
		timeBonus = score.timeBonusMaxLimitTime * score.bonusPerTime;
	else if (_timeLimit >= score.timeBonusTimeMinLimitTime && _timeLimit <= score.timeBonusMaxLimitTime)
		timeBonus = static_cast<int>(_timeLimit) * score.bonusPerTime;
	else if (abandonedBonus == 0)
		timeBonus = score.minTimeBonus;
	else
		timeBonus = 0;

	for (auto isTriggered : _isGhostEventTriggered)
	{
		if (isTriggered)
			ghostBonus += score.bonusPerGhostEvent;
	}

	if (workPayText)
		workPayText->text = std::to_string(timeBonus);

	if (cleanBonusText)
		cleanBonusText->text = std::to_string(trashBonus);

	if (messBounusText)
		messBounusText->text = std::to_string(messBonus);

	if (abandonedBonusText)
		abandonedBonusText->text = std::to_string(abandonedBonus);

	if (ghostEventBonusText)
		ghostEventBonusText->text = std::to_string(ghostBonus);

	if (percentageBonusText)
		percentageBonusText->text = std::to_string(percentageBonus);

	int totalMoney = timeBonus + trashBonus + messBonus + abandonedBonus + ghostBonus + percentageBonus;

	if (totalText)
		totalText->text = std::to_string(totalMoney);

	if (finalPercentage)
	{
		std::string percentageText = "Total : ";
		percentageText += std::to_string(static_cast<int>(percentMulHundred));
		percentageText += "%";
		finalPercentage->text = percentageText;
	}

	if (totalMoney >= score.fiveStarMinScore)
		completedCount = 5;
	else if (totalMoney >= score.fourStarMinScore)
		completedCount = 4;
	else if (totalMoney >= score.threeStarMinScore)
		completedCount = 3;
	else if (totalMoney >= score.twoStarMinScore)
		completedCount = 2;
	else if (totalMoney >= score.oneStarMinScore)
		completedCount = 1;
	else
		completedCount = 0;

	for (int i = 0; i < completedCount; i++)
		stars[i]->textureString = "T_StarOn.dds";

	if (commentText)
	{
		if (completedCount <= 0)
			commentText->text = "일은 한건가?";
		else if (completedCount <= 1)
			commentText->text = "더 열심히 하도록.";
		else if (completedCount <= 2)
			commentText->text = "청소 결과가 아쉽군.";
		else if (completedCount <= 3)
			commentText->text = "수고했다.";
		else if (completedCount <= 4)
			commentText->text = "잘했다.";
		else if (completedCount <= 5)
			commentText->text = "아주 훌륭하군.";
	}

	auto& sound = registry.get<core::Sound>(popup);
	sound.isPlaying = true;
	registry.patch<core::Sound>(popup);

	::ShowCursor(true);
}

void mc::TimeLimitSystem::triggerGhostEvent(const OnTriggerGhostEvent& event)
{
	_isGhostEventTriggered[event.room] = true;
}

void mc::TimeLimitSystem::openDoor(entt::registry& registry, float animationTime)
{
	if (animationTime <= 2.0f)
	{
		auto& truckLTransform = registry.get<core::LocalTransform>(_truckDoorL);
		auto& truckRTransform = registry.get<core::LocalTransform>(_truckDoorR);

		float radianL = -(_defaultYAngle[0] * DirectX::XM_PI / 180.0f - DirectX::XM_PIDIV2);
		float radianR = (_defaultYAngle[1] * DirectX::XM_PI / 180.0f);

		Matrix goalMatrixL = Matrix::CreateRotationY(radianL);
		Matrix goalMatrixR = Matrix::CreateRotationY(radianR);
		Quaternion goalQuaternionL = Quaternion::CreateFromRotationMatrix(goalMatrixL);
		Quaternion goalQuaternionR = Quaternion::CreateFromRotationMatrix(goalMatrixR);
		Quaternion pidiv2 = Quaternion::CreateFromRotationMatrix(Matrix::CreateRotationY(DirectX::XM_PIDIV2));

		truckLTransform.rotation = Quaternion::Slerp(pidiv2, goalQuaternionL, animationTime / 2);
		truckRTransform.rotation = Quaternion::Slerp(pidiv2, goalQuaternionR, animationTime / 2);
	}
	registry.patch<core::LocalTransform>(_truckDoorL);
	registry.patch<core::LocalTransform>(_truckDoorR);

	if (!_isDoorOpen)
	{
		_isDoorOpen = true;

		auto& soundL = registry.get<core::Sound>(_truckDoorL);
		auto& soundR = registry.get<core::Sound>(_truckDoorR);

		soundL.isPlaying = true;
		soundR.isPlaying = true;

		registry.patch<core::Sound>(_truckDoorL);
		registry.patch<core::Sound>(_truckDoorR);
	}
}

void mc::TimeLimitSystem::closeDoor(entt::registry& registry, float animationTime)
{
	for (auto&& [entity, common, ui2D] : registry.view<core::UICommon, core::UI2D>().each())
		common.isOn = false;

	for (auto&& [entity, text] : registry.view<core::Text>().each())
		text.isOn = false;

	//::ShowCursor(true);
	global::mouseMode = global::MouseMode::None;
	_dispatcher->trigger<mc::OnInputLock>(OnInputLock{ true, true });
	_dispatcher->trigger<core::OnSetTimeScale>(core::OnSetTimeScale{ 0.0f });

	for (auto&& [entity, animator] : registry.view<core::Animator>().each())
	{
		auto& relationship = registry.get<core::Relationship>(entity);

		for (auto& child : relationship.children)
		{
			if (auto meshRenderer = registry.try_get<core::MeshRenderer>(child))
				meshRenderer->isOn = false;
		}
	}

	for (auto&& [entity, flashlight] : registry.view<mc::Flashlight>().each())
		registry.get<core::LightCommon>(entity).isOn = false;

	for (auto&& [entity, tag] : registry.view<core::Tag>().each())
	{
		if (tag.id == tag::Player::id)
			registry.get<core::Camera>(registry.get<core::Relationship>(entity).children[0]).isActive = false;
	}

	auto& camera = registry.get<core::Camera>(_endingCamera);
	auto& truckPedestalTransform = registry.get<core::WorldTransform>(_truckPedestal);

	camera.isActive = true;

	auto& gameManger = registry.get<mc::GameManager>(_gameManager);

	if (animationTime <= 2.0f)
	{
		auto& truckLTransform = registry.get<core::LocalTransform>(_truckDoorL);
		auto& truckRTransform = registry.get<core::LocalTransform>(_truckDoorR);

		float radianL = -(_defaultYAngle[0] * DirectX::XM_PI / 180.0f - DirectX::XM_PIDIV2);
		float radianR = (_defaultYAngle[1] * DirectX::XM_PI / 180.0f);

		Matrix goalMatrixL = Matrix::CreateRotationY(radianL);
		Matrix goalMatrixR = Matrix::CreateRotationY(radianR);
		Quaternion goalQuaternionL = Quaternion::CreateFromRotationMatrix(goalMatrixL);
		Quaternion goalQuaternionR = Quaternion::CreateFromRotationMatrix(goalMatrixR);

		Matrix goalMatrix = Matrix::CreateRotationY(DirectX::XM_PIDIV2);
		Quaternion goalQuaternion = Quaternion::CreateFromRotationMatrix(goalMatrix);

		truckLTransform.rotation = Quaternion::Slerp(goalQuaternionL, goalQuaternion, animationTime / 2);
		truckRTransform.rotation = Quaternion::Slerp(goalQuaternionR, goalQuaternion, animationTime / 2);

		if (_isDoorOpen)
		{
			_isDoorOpen = false;

			auto& soundL = registry.get<core::Sound>(_truckDoorL);
			auto& soundR = registry.get<core::Sound>(_truckDoorR);

			soundL.isPlaying = true;
			soundR.isPlaying = true;

			registry.patch<core::Sound>(_truckDoorL);
			registry.patch<core::Sound>(_truckDoorR);
		}
	}
	else if (animationTime <= 4.0f)
	{

		Matrix goalMatrix = Matrix::CreateRotationY(DirectX::XM_PIDIV2);
		Quaternion goalQuaternion = Quaternion::CreateFromRotationMatrix(goalMatrix);

		truckPedestalTransform.rotation = Quaternion::Slerp(_defaultRotation, goalQuaternion, (animationTime - 2) / 2);

		if (_isPedestralOpen)
		{
			_isPedestralOpen = false;

			auto& sound = registry.get<core::Sound>(_truckPedestal);
			sound.isPlaying = true;
			registry.patch<core::Sound>(_truckPedestal);
		}
	}
	else
		truckPedestalTransform.position.z = std::lerp(_defaultPosittionZ, -11.161f, (animationTime - 4) / 2);

	registry.patch<core::LocalTransform>(_truckDoorL);
	registry.patch<core::LocalTransform>(_truckDoorR);
	registry.patch<core::WorldTransform>(_truckPedestal);
}
