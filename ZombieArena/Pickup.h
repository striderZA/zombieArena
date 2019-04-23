#ifndef PICKUP_H
#define PICKUP_H

#include "pch.h"
#include <SFML/Graphics.hpp>
#include "TextureHolder.h"

using namespace sf;

class Pickup
{
private:
	const int HEALTH_START_VALUE = 50;
	const int AMMO_START_VALUE = 16;
	const int START_WAIT_TIME = 10;
	const int START_SECONDS_TO_LIVE = 6;

	Sprite _sprite;
	
	FloatRect _arena;
	int _value;
	int _type;

	bool _spawned;
	float _secondsSinceSpawn;
	float _secondsSinceDespawn;
	float _secondsToLive;
	float _secondsToWait;

public:
	Pickup(int type);
	virtual ~Pickup();

	void setArena(FloatRect arena);
	void spawn();

	FloatRect getPosition();
	Sprite getSprite();
	void update(float elapsedTime);
	bool isSpawned();
	int gotIt();
	void upgrade();
};

#endif /*PICKUP_H*/