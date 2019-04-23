#ifndef PLAYER_H
#define PLAYER_H

#include <SFML/Graphics.hpp>
#include <iostream>

using namespace sf;

class Player
{
private:
	const float START_SPEED = 200;
	const float START_HEALTH = 100;

	Vector2f _position;
	Vector2f _resolution;

	Sprite _sprite;
	Texture _texture;

	FloatRect _arena;

	int _tileSize;
	float _currentHealth;
	float _maxHealth;

	Time _lastHit;

	float _speed;

	bool _upPressed;
	bool _downPressed;
	bool _leftPressed;
	bool _rightPressed;

public:
	Player();
	virtual ~Player();

	void spawn(FloatRect arena, Vector2f resolution, int tileSize);
	void resetPlayerStats();
	bool hit(Time timeHit);
	Time getLastHitTime();
	FloatRect getPosition();
	Vector2f getCenter();
	float getRotation();
	Sprite getSprite();

	void moveLeft();
	void moveRight();
	void moveUp();
	void moveDown();

	void stopLeft();
	void stopRight();
	void stopUp();
	void stopDown();

	void update(float elapsedTime, Vector2i mousePosition);
	void upgradeSpeed();
	void upgradeHealth();
	void increaseHealthLevel(float amount);
	float getHealth();

};

#endif /*PLAYER_H*/