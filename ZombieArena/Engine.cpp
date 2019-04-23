#include "pch.h"
#include "Engine.h"

#define ENGINE_DEBUG 1

#if ENGINE_DEBUG
#define ENGINE_DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define ENGINE_DEBUG_PRINT(...)
#endif

Engine::Engine() : _isPlaying(false),
_remainingTime(10),
_state(State::GAME_OVER)
{
	
	_resolution.x = VideoMode::getDesktopMode().width;
	_resolution.y = VideoMode::getDesktopMode().height;

	_window.create(VideoMode(_resolution.x, _resolution.y), "Zombie Arena", Style::Fullscreen);
	_mainView.setSize(static_cast<Vector2f>(_resolution));

	_hudView.reset(FloatRect(0, 0, (float)_resolution.x, (float)_resolution.y));

	_backgroundTexture = TextureHolder::GetTexture("graphics/background.png");
	_backgroundSprite.setTexture(_backgroundTexture);

	_player = new Player(_resolution);
	_healthPickup = new Pickup(1);
	_ammoPickup = new Pickup(2);
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
			// handle exiting the game
			if (Keyboard::isKeyPressed(Keyboard::Escape))
			{
				_window.close();
			}

			// handle whatever happens when we press Enter
			// changes depending on the game state
			if (event.key.code == Keyboard::Return)
			{
				if (_state == State::PLAYING)
				{
					_state = State::PAUSED;
				}
				else if (_state == State::PAUSED)
				{
					_state = State::PLAYING;
					_clock.restart();
				}
				else if (_state == State::GAME_OVER)
				{
					_score = 0;
					_wave = 0;
					_player->resetPlayerStats();
				}
			}
		}

		// handle keyboard inputs when we're actively playing a level
		if (_state == State::PLAYING)
		{
			// reloading
			if (event.key.code == Keyboard::R)
			{
				if (_player->getRemainingBullets() >= _player->getMagazineSize())
				{
					_player->setBulletsInMagazine(_player->getMagazineSize());
					_player->setRemainingBullets(_player->getRemainingBullets() - _player->getMagazineSize());
					// play reload sound
				}
				else if (_player->getRemainingBullets() > 0)
				{
					_player->setBulletsInMagazine(_player->getRemainingBullets());
					_player->setRemainingBullets(0);
					// play reload sound
				}
				else
				{
					// reload failed, do nothing (for now)
				}
			}
			// moving around
			if (Keyboard::isKeyPressed(Keyboard::W))
			{
				_player->moveUp();
			}
			else
			{
				_player->stopUp();
			}
			if (Keyboard::isKeyPressed(Keyboard::A))
			{
				_player->moveLeft();
			}
			else
			{
				_player->stopLeft();
			}
			if (Keyboard::isKeyPressed(Keyboard::S))
			{
				_player->moveDown();
			}
			else
			{
				_player->stopDown();
			}
			if (Keyboard::isKeyPressed(Keyboard::D))
			{
				_player->moveRight();
			}
			else
			{
				_player->stopRight();
			}
			// shooting
			if (Mouse::isButtonPressed(Mouse::Left))
			{
				// check that we're within the allotted fire rate
				if ((_gameTimeTotal.asMilliseconds() - _lastPressedFire.asMilliseconds() > 1000 / _player->getRateOfFire()) && (_player->getBulletsInMagazine() > 0))
				{
					_player->_bullets[_player->getCurrentBullet()].shoot(_player->getCenter().x, _player->getCenter().y, _mouseWorldPosition.x, _mouseWorldPosition.y);
					_player->setCurrentBullet(_player->getCurrentBullet() + 1);

					if (_player->getCurrentBullet() > 99)
					{
						_player->setCurrentBullet(0);
					}
					_lastPressedFire = _gameTimeTotal;
					_player->setBulletsInMagazine(_player->getBulletsInMagazine() - 1);
				}
			}
		}

		if (_state == State::LEVELING_UP)
		{
			// increase Player's fire rate
			if (event.key.code == Keyboard::Num1)
			{
				_player->setRateOfFire(_player->getRateOfFire() + 1);
				_state = State::PLAYING;
			}
			// increase magazine size
			if (event.key.code == Keyboard::Num2)
			{
				_player->setMagazineSize(_player->getMagazineSize() + 6);
				_state = State::PLAYING;
			}
			// increase base HP
			if (event.key.code == Keyboard::Num3)
			{
				_player->upgradeHealth();
				_state = State::PLAYING;
			}
			// increase run speed
			if (event.key.code == Keyboard::Num4)
			{
				_player->upgradeSpeed();
				_state = State::PLAYING;
			}
			// increase health pickups
			if (event.key.code == Keyboard::Num5)
			{
				_healthPickup->upgrade();
				_state = State::PLAYING;
			}
			// increase ammo pickups
			if (event.key.code == Keyboard::Num6)
			{
				_ammoPickup->upgrade();
				_state = State::PLAYING;
			}

			if (_state == State::PLAYING)
			{
				_wave++;
				_arena.width = ARENA_START_WIDTH * (1 + _wave * 0.1);
				_arena.height = ARENA_START_HEIGHT * (1 + _wave * 0.1);
				_arena.left = 0;
				_arena.top = 0;

				int tileSize = createBackground(_background, _arena);
				_player->spawn(_arena, _resolution, tileSize);

				_numZombies = 5 + ((_wave - 1) * 2);

				delete[] _zombies;
				_zombies = createHorde(_numZombies, _arena);
				_numZombiesAlive = _numZombies;

				_clock.restart();
				
			}
		}
	}
}

void Engine::update(float dtAsSeconds)
{

	_mouseScreenPosition = Mouse::getPosition();
	_mouseWorldPosition = _window.mapPixelToCoords(_mouseScreenPosition, _mainView);

	_player->update(dtAsSeconds, _mouseScreenPosition);

	Vector2f playerPosition(_player->getCenter());
	_mainView.setCenter(playerPosition);

	for (int i = 0; i < _numZombies; ++i)
	{
		if (_zombies[i].isAlive())
		{
			_zombies[i].update(dtAsSeconds, playerPosition);
		}
	}

	for (int i = 0; i < 100; ++i)
	{
		if (_player->_bullets[i].isInFlight())
		{
			_player->_bullets[i].update(dtAsSeconds);
		}
	}

	_healthPickup->update(dtAsSeconds);
	_ammoPickup->update(dtAsSeconds);

	for (int i = 0; i < 100; ++i)
	{
		for (int j = 0; j < _numZombies; ++j)
		{
			if (_player->_bullets[i].isInFlight() && _zombies[j].isAlive())
			{
				if (_player->_bullets[i].getPosition().intersects(_zombies[j].getPosition()))
				{
					_player->_bullets[i].stop();
					if (_zombies[j].hit())
					{
						_score += 10;
						if (_score > _hiScore)
						{
							_hiScore = _score;
						}
						_numZombiesAlive--;
						if (_numZombiesAlive == 0)
						{
							_state = State::LEVELING_UP;
						}
					}
					//play splat sound
				}
			}
		}
	}

	for (int i = 0; i < _numZombies; ++i)
	{
		if (_player->getPosition().intersects(_zombies[i].getPosition()) && _zombies[i].isAlive())
		{
			if (_player->hit(_gameTimeTotal))
			{
				// play hit sound
			}
			if (_player->getHealth() <= 0)
			{
				// play death sound
				_state = State::GAME_OVER;
				ofstream outputFile("gamedata/scores.txt");
				outputFile << _hiScore;
				outputFile.close();
			}
		}
	}

	if (_player->getPosition().intersects(_healthPickup->getPosition()) && _healthPickup->isSpawned())
	{
		_player->increaseHealthLevel(_healthPickup->gotIt());
		// play health pack sound
	}

	if (_player->getPosition().intersects(_ammoPickup->getPosition()) && _ammoPickup->isSpawned())
	{
		_player->setRemainingBullets(_ammoPickup->gotIt() + _player->getRemainingBullets());
		// play reload sound
	}

}

void Engine::draw()
{
	if (_state == State::PLAYING)
	{
		_window.clear();
		_window.setView(_mainView);
		_window.draw(_background, &_backgroundTexture);
	}

	if (_state == State::LEVELING_UP)
	{
		
	}
	if (_state == State::PAUSED)
	{
		
	}

	if (_state == State::GAME_OVER)
	{
		
	}
}

void Engine::run()
{
	while (_window.isOpen())
	{
		Time dt = _clock.restart();
		_gameTimeTotal += dt;

		float dtAsSeconds = dt.asSeconds();

		handleInput();
		//update(dtAsSeconds);
		draw();
	}
}