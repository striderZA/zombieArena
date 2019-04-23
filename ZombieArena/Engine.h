#ifndef ENGINE_H
#define ENGINE_H

#include <SFML/Graphics.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include "TextureHolder.h"
#include "Player.h"
#include "Zombie.h"
#include "Bullet.h"
#include "Pickup.h"
#include "ZombieArena.h"
#include "HUD.h"

using namespace sf;

class Engine
{
private:
	// Game states
	enum class State{PAUSED, LEVELING_UP, GAME_OVER, PLAYING};

	// Texture holder constants
	const int TILE_SIZE = 50;
	const int VERTS_IN_QUAD = 4;

	// arena initialization
	const unsigned int ARENA_START_WIDTH = 500;
	const unsigned int ARENA_START_HEIGHT = 500;

	State _state;
	Player *_player;
	Zombie *_zombies = nullptr;
	Pickup *_healthPickup;
	Pickup *_ammoPickup;
	unsigned int _wave;
	unsigned int _score;
	unsigned int _hiScore;

	TextureHolder th;	
	RenderWindow _window;

	View _mainView;
	View _backgroundView;
	View _hudView;

	HUD _hud;
	int _framesSinceLastHudUpdate = 0;
	int _framesPerHudUpdate = 500;

	Sprite _backgroundSprite;
	Texture _backgroundTexture;
	VertexArray _background;
	FloatRect _arena;

	int _numZombies;
	int _numZombiesAlive;
	
	bool _isPlaying;

	Time _gameTimeTotal;
	float _remainingTime;
	Time _lastPressedFire;
	Clock _clock;

	Vector2f _mouseWorldPosition;
	Vector2i _mouseScreenPosition;
	Vector2u _resolution;

	// three components of the game loop
	void handleInput();
	void update(float dtAsSeconds);
	void draw();
public:
	Engine();
	virtual ~Engine();

	void run();
};

#endif /*ENGINE_H*/
