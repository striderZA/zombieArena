#include "pch.h"
#include "Zombie.h"
#include "TextureHolder.h"
#include <cstdlib>
#include <ctime>
#include <iostream>

using namespace std;

#define ZOMBIE_DEBUG 0

#if ZOMBIE_DEBUG
#define ZOMBIE_DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define ZOMBIE_DEBUG_PRINT(...)
#endif

Zombie::Zombie()
{
}

Zombie::~Zombie()
{
}

bool Zombie::hit()
{
	_health--;

	if (_health < 0)
	{
		_alive = false;
		_sprite.setTexture(TextureHolder::GetTexture("graphics/blood.png"));
		return true;
	}

	return false;
}

bool Zombie::isAlive()
{
	return _alive;
}

void Zombie::spawn(float startX, float startY, int type, int seed)
{
	switch (type)
	{
	case 0: // bloater
		_sprite = Sprite(TextureHolder::GetTexture("graphics/bloater.png"));
		_speed = 40;
		_health = 5;
		break;
	case 1: // chaser
		_sprite = Sprite(TextureHolder::GetTexture("graphics/chaser.png"));
		_speed = 70;
		_health = 1;
		break;
	case 2: // crawler
		_sprite = Sprite(TextureHolder::GetTexture("graphics/crawler.png"));
		_speed = 20;
		_health = 3;
		break;
	default:
		break;
	}

	srand((int)time(0) * seed);

	float modifier = (float)(rand() % MAX_VARIANCE) + OFFSET;
	modifier /= 100;
	_speed *= modifier;
	_position.x = startX;
	_position.y = startY;
	_sprite.setOrigin(25, 25);
	_sprite.setPosition(_position);
}

FloatRect Zombie::getPosition()
{
	return _sprite.getGlobalBounds();
}

Sprite Zombie::getSprite()
{
	return _sprite;
}

void Zombie::update(float elapsedTime, Vector2f playerLocation)
{
	float playerX = playerLocation.x;
	float playerY = playerLocation.y;

	if (playerX > _position.x)
		_position.x += _speed * elapsedTime;

	if (playerY > _position.y)
		_position.y += _speed * elapsedTime;

	if (playerX < _position.x)
		_position.x -= _speed * elapsedTime;

	if (playerY < _position.y)
		_position.y -= _speed * elapsedTime;

	_sprite.setPosition(_position);

	_sprite.setRotation((atan2(playerY - _position.y, playerX - _position.x) * 180) / 3.141);
}

void Zombie::upgradeSpeed(int type, int waveNumber)
{
	switch (type)
	{
	case 0: // bloater
		_speed += BLOATER_SPEED*(0.05*(float)waveNumber);
		break;
	case 1: // chaser
		_speed += CHASER_SPEED*(0.05*(float)waveNumber);
		break;
	case 2: // crawler
		_speed += CRAWLER_SPEED* (0.05*(float)waveNumber);
		break;
	default:
		break;
	}

	ZOMBIE_DEBUG_PRINT("Increasing speed to: %.2f\n", _speed);
}

void Zombie::upgradeHealth(int type)
{
}

void Zombie::resetZombieStats(int type)
{
	switch (type)
	{
	case 0: // bloater
		_sprite = Sprite(TextureHolder::GetTexture("graphics/bloater.png"));
		_speed = BLOATER_SPEED;
		_health = BLOATER_HEALTH;
		break;
	case 1: // chaser
		_sprite = Sprite(TextureHolder::GetTexture("graphics/chaser.png"));
		_speed = CHASER_SPEED;
		_health = CHASER_HEALTH;
		break;
	case 2: // crawler
		_sprite = Sprite(TextureHolder::GetTexture("graphics/crawler.png"));
		_speed = CRAWLER_SPEED;
		_health = CRAWLER_HEALTH;
		break;
	default:
		break;
	}
}
