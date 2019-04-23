#ifndef ENGINE_H
#define ENGINE_H

#include <SFML/Graphics.hpp>
#include "TextureHolder.h"

using namespace sf;

class Engine
{
private:
	enum class State{PAUSED, LEVELING_UP, GAME_OVER, PLAYING};

	State _state;

	TextureHolder th;
	const int TILE_SIZE = 50;
	const int VERTS_IN_QUAD = 4;

	RenderWindow _window;

	View _mainView;
	View _backgroundView;
	View _hudView;

	Sprite _backgroundSprite;
	Texture _backgroundTexture;
	
	bool _isPlaying;

	Time _gameTimeTotal;
	float _remainingTime;

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
