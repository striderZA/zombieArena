#include "pch.h"
#include "Bullet.h"

using namespace sf;

Bullet::Bullet()
{
	_bulletShape.setSize(Vector2f(4, 2));
	_bulletShape.setFillColor(sf::Color::White);
}


Bullet::~Bullet()
{
}

void Bullet::stop()
{
	_inFlight = false;
}

bool Bullet::isInFlight()
{
	return _inFlight;
}

void Bullet::shoot(float startX, float startY, float targetX, float targetY)
{
	_inFlight = true;
	_position.x = startX;
	_position.y = startY;

	float gradient = (startY - targetY) / (startX - targetX);

	if (gradient < 0)
		gradient *= -1;

	float ratioXY = _bulletSpeed / (1 + gradient);

	_bulletDistanceX = ratioXY;
	_bulletDistanceY = ratioXY * gradient;

	if (targetX < startX)
	{
		_bulletDistanceX *= -1;
	}		

	if (targetY < startY)
	{
		_bulletDistanceY *= -1;
	}
		

	float range = 1000;

	_minX = startX - range;
	_minY = startY - range;
	_maxX = startX + range;
	_maxY = startY + range;

	_bulletShape.setPosition(_position);
}

FloatRect Bullet::getPosition()
{
	return _bulletShape.getGlobalBounds();
}

RectangleShape Bullet::getShape()
{
	return _bulletShape;
}

void Bullet::update(float elapsedTime)
{
	_position.x += _bulletDistanceX * elapsedTime;
	_position.y += _bulletDistanceY * elapsedTime;

	_bulletShape.setPosition(_position);

	if ((_position.x < _minX) || 
		(_position.x > _maxX) || 
		(_position.y < _minY) ||
		(_position.y > _maxY))
	{
		_inFlight = false;
	}
		
}
