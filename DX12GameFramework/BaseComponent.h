#pragma once

class Actor;

class BaseComponent
{
protected:
	Actor* _owner;

public:
	BaseComponent(Actor* owner) : _owner(owner) {}
	virtual ~BaseComponent() = default;

	virtual void Update() = 0;
};