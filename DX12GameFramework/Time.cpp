#include "pch.h"
#include "Time.h"

std::chrono::time_point<std::chrono::high_resolution_clock> Time::_startTime;
std::chrono::time_point<std::chrono::high_resolution_clock> Time::_prevTime;
float Time::_deltaTime = 0.0f;
float Time::_totalTime = 0.0f;

void Time::Initialize(){
	_deltaTime = 0;
	_totalTime = 0;

	_startTime = std::chrono::high_resolution_clock::now();
	_prevTime = std::chrono::high_resolution_clock::now();
}

void Time::Update() {
	auto beforeTime = _prevTime;
	_prevTime = std::chrono::high_resolution_clock::now();
	
	_deltaTime = std::chrono::duration<float>(_prevTime - beforeTime).count();
	_totalTime = std::chrono::duration<float>(_prevTime - _startTime).count();
}