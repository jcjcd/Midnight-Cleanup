#include "pch.h"
#include "AnimatorController.h"

#include <fstream>
#include "AnimatorState.h"


void core::AnimatorController::Save(const std::string& filePath, AnimatorController& controller)
{
	std::ofstream os(filePath);
	cereal::JSONOutputArchive archive(os);
	archive(controller);
}


core::AnimatorController core::AnimatorController::Load(const std::string& filePath)
{
	std::ifstream is(filePath);
	cereal::JSONInputArchive archive(is);

	AnimatorController controller;
	archive(controller);


	return controller;
}
