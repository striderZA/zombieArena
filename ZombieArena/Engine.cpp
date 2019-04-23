#include "pch.h"
#include "Engine.h"

Engine::Engine() :	_isPlaying(false),
					_remainingTime(10),
					_state(State::GAME_OVER)
{
	Vector2u resolution;
	resolution.x = VideoMode::getDesktopMode().width;
	resolution.y = VideoMode::getDesktopMode().height;

	_window.create(VideoMode(resolution.x, resolution.y), "Zombie Arena", Style::Fullscreen);
	_mainView.setSize(static_cast<Vector2f>(resolution));

	_hudView.reset(FloatRect(0, 0, resolution.x, resolution.y));

	_backgroundTexture = TextureHolder::GetTexture("graphics/background.png");
	_backgroundSprite.setTexture(_backgroundTexture);
}

Engine::~Engine()
{
}

void Engine::handleInput()
{
	Event event;

	while (_window.pollEvent(event))
	{
		if (event.type == Event::KeyPressed)
		{
			if (Keyboard::isKeyPressed(Keyboard::Escape))
			{
				_window.close();
			}
		}
	}
}

void Engine::update(float dtAsSeconds)
{

}

void Engine::draw()
{
}

void Engine::run()
{
	Clock clock;
	while (_window.isOpen())
	{
		Time dt = clock.restart();
		_gameTimeTotal += dt;

		float dtAsSeconds = dt.asSeconds();

		handleInput();
		update(dtAsSeconds);
		draw();
	}
}