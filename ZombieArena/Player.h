#ifndef PLAYER_H
#define PLAYER_H

#include <SFML/Graphics.hpp>
#include <iostream>
#include "Bullet.h"

using namespace sf;

class Player
{
private:
	const float START_SPEED = 200;
	const float START_HEALTH = 100;
	const unsigned int MAGAZINE_SIZE = 6;
	const unsigned int SPARE_MAGAZINES = 6;

	Vector2f _position;
	Vector2u _resolution;

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

	// bullet class is tied to the player
	// won't ever exist if we don't have a player instantiated

	struct _Weapon
	{
		unsigned int currentBullet;
		unsigned int bulletsRemaining;
		unsigned int magazineSize;
		unsigned int bulletsInMagazine;
		float rateOfFire;
	} _weapon;

public:
	Player(Vector2u resolution);
	virtual ~Player();

	void spawn(FloatRect arena, Vector2u resolution, int tileSize);
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

	unsigned int getCurrentBullet();
	unsigned int getRemainingBullets();
	unsigned int getMagazineSize();
	unsigned int getBulletsInMagazine();
	float getRateOfFire();

	void setCurrentBullet(unsigned int currentBullet);
	void setRemainingBullets(unsigned int bulletsRemaining);
	void setMagazineSize(unsigned int magazineSize);
	void setBulletsInMagazine(unsigned int bulletsInMagazine);
	void setRateOfFire(float rateOfFire);

	Bullet _bullets[100];
};

#endif /*PLAYER_H*/