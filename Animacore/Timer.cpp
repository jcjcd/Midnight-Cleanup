#include "pch.h"
#include "Timer.h"

core::Timer::Timer()
{
	// 현재 카운트
	QueryPerformanceCounter(&_prevCount);

	// 초당 카운트 횟수
	QueryPerformanceFrequency(&_frequency);
}

void core::Timer::Update()
{
	QueryPerformanceCounter(&_curCount);

	_deltaTime = static_cast<double>(_curCount.QuadPart - _prevCount.QuadPart) / static_cast<double>(_frequency.QuadPart);

	_prevCount = _curCount;
	_elapsedTime += _deltaTime;
	_fps++;

	if (_elapsedTime > 1.0) 
	{
		_elapsedTime -= 1.0;
		_lastFps = _fps;
		_fps = 0;
	}
}
