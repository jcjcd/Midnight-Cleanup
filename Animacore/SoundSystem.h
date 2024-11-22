#pragma once
#include "SystemTraits.h"
#include "SystemInterface.h"

#include <fmod/fmod.hpp>

namespace core
{
	struct Sound;
	struct OnSetVolume;
	struct OnUpdateBGMState;

	class SoundSystem : public ISystem, public IUpdateSystem
	{
    public:
        SoundSystem(Scene& scene);
    	~SoundSystem() override;

        void operator()(Scene& scene, float tick) override;

        FMOD::Sound* loadSound(const Sound& sound, FMOD_RESULT& result);

    private:
        void updateSound(entt::registry& registry, entt::entity entity);
        void updateListener(entt::registry& registry, entt::entity entity);
        void destroyListener(entt::registry& registry, entt::entity entity);

        void setVolume(const OnSetVolume& event);
        void startSystem(const OnStartSystem& event);
        void finishSystem(const OnFinishSystem& event);

		void updateBGMState(const OnUpdateBGMState& event);

        FMOD::System* _fmodSystem = nullptr;
        std::unordered_map<std::string, FMOD::Sound*> _soundMap;
        entt::dispatcher* _dispatcher = nullptr;
        float _masterVolume = 1.f;
    };
}
DEFINE_SYSTEM_TRAITS(core::SoundSystem)
