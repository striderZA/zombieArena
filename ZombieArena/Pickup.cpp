#include "pch.h"
#include "Pickup.h"

Pickup::Pickup(int type)
{
	_type = type;
	if (_type == 1)
	{
		_sprite = Sprite(TextureHolder::GetTexture("graphics/health_pickup.png"));
		_value = HEALTH_START_VALUE;
	}
	else
	{
		_sprite = _sprite = Sprite(TextureHolder::GetTexture("graphics/ammo_pickup.png"));
		_value = AMMO_START_VALUE;
	}

	_sprite.setOrigin(25, 25);

	_secondsToLive = START_SECONDS_TO_LIVE;
	_secondsToWait = START_SECONDS_TO_LIVE;
}

Pickup::~Pickup()
{
}

void Pickup::setArena(FloatRect arena)
{
	_arena.left = arena.left + 50;
	_arena.width = arena.width - 50;
	_arena.top = arena.top + 50;
	_arena.height = arena.height - 50;

	this->spawn();
}

void Pickup::spawn()
{
	srand((int)time(0) / _type);
	int x = (rand() % (int)_arena.width);
	srand((int)time(0) * _type);
	int y = (rand() % (int)_arena.height);

	_secondsSinceSpawn = 0;
	_spawned = true;
	_sprite.setPosition(x, y);
}

FloatRect Pickup::getPosition()
{
	return _sprite.getGlobalBounds();
}

Sprite Pickup::getSprite()
{
	return _sprite;
}

void Pickup::update(float elapsedTime)
{
	if (_spawned)
	{
		_secondsSinceSpawn += elapsedTime;
	}
	else
	{
		_secondsSinceDespawn += elapsedTime;
	}

	if (_secondsSinceSpawn > _secondsToLive && _spawned)
	{
		_spawned = false;
		_secondsSinceDespawn = 0;
	}

	if (_secondsSinceDespawn > _secondsToWait && !_spawned)
	{
		this->spawn();
	}
}

bool Pickup::isSpawned()
{
	return _spawned;
}

int Pickup::gotIt()
{
	_spawned = false;
	_secondsSinceDespawn = 0;
	return _value;
}

void Pickup::upgrade()
{
	if (_type == 1)
	{
		_value += (HEALTH_START_VALUE * 0.5);
	}
	else
	{
		_value += (AMMO_START_VALUE * 0.5);
	}

	_secondsToLive += (START_SECONDS_TO_LIVE / 10);
	_secondsToWait -= (START_WAIT_TIME / 10);
}
