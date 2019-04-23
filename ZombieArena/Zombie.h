#ifndef ZOMBIE_H
#define ZOMBIE_H

#include <SFML/Graphics.hpp>

using namespace std;
using namespace sf;

class Zombie
{
private:
	const float BLOATER_SPEED = 50;
	const float CHASER_SPEED = 70;
	const float CRAWLER_SPEED = 30;

	const float BLOATER_HEALTH = 6;
	const float CHASER_HEALTH = 2;
	const float CRAWLER_HEALTH = 4;

	const int MAX_VARIANCE = 30;
	const int OFFSET = 101 - MAX_VARIANCE;

	Vector2f _position;
	Sprite _sprite;
	float _speed;
	float _health;
	bool _alive;

public:
	Zombie();
	virtual ~Zombie();

	bool hit();
	bool isAlive();
	void spawn(float startX, float startY, int type, int seed);
	FloatRect getPosition();
	Sprite getSprite();
	void update(float elapsedTime, Vector2f playerLocation);

	void upgradeSpeed(int type, int waveNumber);
	void upgradeHealth(int type);
	void resetZombieStats(int type);
};

#endif /*ZOMBIE_H*/