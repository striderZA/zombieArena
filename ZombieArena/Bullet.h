#ifndef BULLET_H
#define BULLET_H

#include <SFML/Graphics.hpp>

using namespace sf;

class Bullet
{
private:
	Vector2f _position;
	RectangleShape _bulletShape;
	bool _inFlight = false;
	float _bulletSpeed = 1000;
	float _bulletDistanceX;
	float _bulletDistanceY;
	float _maxX;
	float _maxY;
	float _minX;
	float _minY;

public:
	Bullet();
	virtual ~Bullet();

	void stop();
	bool isInFlight();
	void shoot(float startX, float startY, float targetX, float targetY);
	FloatRect getPosition();
	RectangleShape getShape();
	void update(float elapsedTime);
};

#endif /*BULLET_H*/