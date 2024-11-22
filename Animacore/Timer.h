#pragma once

namespace core
{
	class Timer
	{
	public:
		Timer();
		~Timer() = default;

	private:
		LARGE_INTEGER _curCount{};
		LARGE_INTEGER _prevCount{};
		LARGE_INTEGER _frequency{};

		double _deltaTime = 0.f;				// 프레임 간의 시간 값
		double _elapsedTime = 0.f;
		uint32_t _lastFps = 0;
		uint32_t _fps = 0;

	public:
		void Update();

		float GetTick() { return static_cast<float>(_deltaTime); }
		uint32_t GetFPS() { return _lastFps; }
	};

}
