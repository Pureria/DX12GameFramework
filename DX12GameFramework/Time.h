#pragma once
#include <chrono>

class Time
{
private:
	static std::chrono::time_point<std::chrono::high_resolution_clock> _startTime;
	static std::chrono::time_point<std::chrono::high_resolution_clock> _prevTime;

	static float _deltaTime;
	static float _totalTime;
public:
	static void Initialize();
	static void Update();

	static float GetDeltaTime() { return _deltaTime; }
	static float GetTotalTime() { return _totalTime; }
};

