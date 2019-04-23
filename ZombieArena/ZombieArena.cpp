#include "pch.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "Player.h"
#include "ZombieArena.h"
#include "TextureHolder.h"
#include "Bullet.h"
#include "Pickup.h"

using namespace sf;
using namespace std;

int main()
{
	// Here is the instance of TextureHolder
	TextureHolder holder;

	// The game will always be in one of four states
	enum class State { PAUSED, LEVELING_UP, GAME_OVER, PLAYING };
	// Start with the GAME_OVER state
	State state = State::GAME_OVER;

	// Get the screen resolution and create an SFML window
	Vector2f resolution;
	resolution.x = VideoMode::getDesktopMode().width;
	resolution.y = VideoMode::getDesktopMode().height;

	RenderWindow window(VideoMode(resolution.x, resolution.y), "Zombie Arena", Style::Fullscreen);

	// main game view, half of the y resoultion
	View mainView(FloatRect(0, 0, resolution.x, resolution.y));
	Clock clock;
	Time gameTimeTotal;

	Vector2f mouseWorldPosition;
	Vector2i mouseScreenPosition;

	Player player;

	FloatRect arena;

	Texture textureBackground = TextureHolder::GetTexture("graphics/background_sheet.png");
	VertexArray background;

	int numZombies;
	int numZombiesAlive;

	Zombie *zombies = nullptr;

	Bullet bullets[100];
	int currentBullet = 0;
	int bulletsSpare = 36;
	int bulletsInClip = 6;
	int clipSize = 6;
	float fireRate = 1;

	// When was the fire button last pressed?
	Time lastPressed;

	// Hide the mouse pointer and replace it with crosshair
	window.setMouseCursorVisible(false);
	Sprite spriteCrosshair;
	Texture textureCrosshair = TextureHolder::GetTexture("graphics/crosshair.png");
	spriteCrosshair.setTexture(textureCrosshair);
	spriteCrosshair.setOrigin(25, 25);

	Pickup healthPickup(1);
	Pickup ammoPickup(2);

	int score = 0;
	int hiScore = 0;

	// home screen / game over screen
	Sprite spriteGameOver;
	Texture textureGameOver = TextureHolder::GetTexture("graphics/background3.png");
	spriteGameOver.setTexture(textureGameOver);
	spriteGameOver.setPosition(0, 0);

	Vector2f targetSize = window.getView().getSize();
	spriteGameOver.setScale(targetSize.x / spriteGameOver.getLocalBounds().width, targetSize.y / spriteGameOver.getLocalBounds().height);

	// HUD overlay
	View hudView(FloatRect(0, 0, resolution.x, resolution.y));

	// ammo overlay
	Sprite spriteAmmoIcon;
	Texture textureAmmoIcon = TextureHolder::GetTexture("graphics/ammo_icon.png");
	spriteAmmoIcon.setTexture(textureAmmoIcon);
	spriteAmmoIcon.setPosition(20, 0.9*resolution.y);

	Font font;
	font.loadFromFile("fonts/big_noodle_titling.ttf");

	Text titleText;
	titleText.setCharacterSize(150);
	titleText.setFont(font);
	titleText.setStyle(Text::Italic);
	titleText.setFillColor(Color::White);
	string titleString = "Zombies";
	titleText.setPosition(hudView.getCenter().x, hudView.getCenter().y + 100);
	titleText.setString(titleString);

	// game paused overlay
	Text pausedText;
	pausedText.setFont(font);
	pausedText.setCharacterSize(100);
	pausedText.setStyle(Text::Italic);
	pausedText.setFillColor(Color::White);
	string pausedString = "Press Enter to Resume";
	pausedText.setPosition(hudView.getCenter().x, hudView.getCenter().y);
	pausedText.setString(pausedString);

	// game over
	Text gameOverText;
	gameOverText.setFont(font);
	gameOverText.setCharacterSize(100);
	gameOverText.setStyle(Text::Italic);
	gameOverText.setFillColor(Color::White);
	string gameOverString = "Press Enter to Play";
	gameOverText.setPosition(hudView.getCenter().x, hudView.getCenter().y);
	gameOverText.setString(gameOverString);

	Text levelUpText;
	levelUpText.setFont(font);
	levelUpText.setCharacterSize(80);
	levelUpText.setStyle(Text::Italic);
	levelUpText.setFillColor(Color::White);
	levelUpText.setPosition(hudView.getCenter().x - 200, hudView.getCenter().y - 200);

	stringstream levelUpStream;
	levelUpStream << "1: Increased rate of fire\n" <<
		"2: Increased magazine size\n" <<
		"3: Increased maximum health\n" <<
		"4: Increased run speed\n" <<
		"5: Improved health pickups\n" <<
		"6: Improved ammo pickups";

	levelUpText.setString(levelUpStream.str());

	// ammo text
	Text ammoText;
	ammoText.setFont(font);
	ammoText.setCharacterSize(55);
	ammoText.setStyle(Text::Italic);
	ammoText.setFillColor(Color::White);
	ammoText.setPosition(200, 0.9*resolution.y);

	// score text
	Text scoreText;
	scoreText.setFont(font);
	scoreText.setCharacterSize(55);
	scoreText.setStyle(Text::Italic);
	scoreText.setFillColor(Color::White);
	scoreText.setPosition(20, 0);

	ifstream inputFile("gamedata/scores.txt");
	if (inputFile.is_open())
	{
		inputFile >> hiScore;
		inputFile.close();
	}

	// High score text
	Text hiScoreText;
	hiScoreText.setFont(font);
	hiScoreText.setCharacterSize(55);
	hiScoreText.setStyle(Text::Italic);
	hiScoreText.setFillColor(Color::White);
	hiScoreText.setPosition(0.85*resolution.x, 0);
	stringstream s;
	s << "Hi Score: " << hiScore;
	hiScoreText.setString(s.str());

	// Remaining Zombies
	Text remainingZombiesText;
	remainingZombiesText.setFont(font);
	remainingZombiesText.setCharacterSize(55);
	remainingZombiesText.setStyle(Text::Italic);
	remainingZombiesText.setFillColor(Color::White);
	remainingZombiesText.setPosition(0.7*resolution.x, 0.9*resolution.y);
	remainingZombiesText.setString("Zombies: 100");

	// wave number
	int wave = 0;
	Text waveNumberText;
	waveNumberText.setFont(font);
	waveNumberText.setCharacterSize(55);
	waveNumberText.setStyle(Text::Italic);
	waveNumberText.setFillColor(Color::White);
	waveNumberText.setPosition(0.85*resolution.x, 0.9*resolution.y);
	waveNumberText.setString("Wave: 0");

	// health bar
	RectangleShape healthBar;
	healthBar.setFillColor(Color::Red);
	healthBar.setPosition(450, 0.9*resolution.y);

	int frameSinceLastHudUpdate = 0;
	int fpsMeasureFrameInterval = 1000;

	// sound for getting hit
	SoundBuffer hitBuffer;
	hitBuffer.loadFromFile("sound/hit.wav");
	Sound hit;
	hit.setBuffer(hitBuffer);

	// zombie splat sound
	SoundBuffer splatBuffer;
	splatBuffer.loadFromFile("sound/splat.wav");
	Sound splat;
	splat.setBuffer(splatBuffer);

	// dying sound
	SoundBuffer dyingBuffer;
	dyingBuffer.loadFromFile("sound/deathscream.wav");
	Sound dying;
	dying.setBuffer(dyingBuffer);

	// shooting sound
	SoundBuffer shootBuffer;
	shootBuffer.loadFromFile("sound/shoot.wav");
	Sound shoot;
	shoot.setBuffer(shootBuffer);

	// reload sound
	SoundBuffer reloadBuffer;
	reloadBuffer.loadFromFile("sound/reload.wav");
	Sound reload;
	reload.setBuffer(reloadBuffer);

	// reload failed
	SoundBuffer reloadFailedBuffer;
	reloadFailedBuffer.loadFromFile("sound/reload_failed.wav");
	Sound reloadFailed;
	reloadFailed.setBuffer(reloadFailedBuffer);

	// powerup
	SoundBuffer powerupBuffer;
	powerupBuffer.loadFromFile("sound/powerup.wav");
	Sound powerUp;
	powerUp.setBuffer(powerupBuffer);

	// pickup
	SoundBuffer pickupBuffer;
	pickupBuffer.loadFromFile("sound/pickup.wav");
	Sound pickup;
	pickup.setBuffer(pickupBuffer);

	// music
	Music menuMusic;
	menuMusic.openFromFile("sound/music.wav");
	menuMusic.play();
	menuMusic.setLoop(true);

	Music gameMusic;
	gameMusic.openFromFile("sound/gamemusic.wav");
	gameMusic.setLoop(true);

	Music levelupMusic;
	levelupMusic.openFromFile("sound/levelupmusic.wav");
	levelupMusic.setLoop(true);

	//Music gameOverMusic;
	//gameOverMusic.openFromFile("sound/gameovermusic.wav");
	//gameOverMusic.setLoop(true);

	// game loop
	while (window.isOpen())
	{
		// Handle events
		Event event;

		while (window.pollEvent(event))
		{
			if (event.type == Event::KeyPressed)
			{
				if (event.key.code == Keyboard::Return && state == State::PLAYING)
				{
					state = State::PAUSED;
				}
				else if (event.key.code == Keyboard::Return && state == State::PAUSED)
				{
					state = State::PLAYING;
					clock.restart();
				}
				else if (event.key.code == Keyboard::Return && state == State::GAME_OVER)
				{
					state = State::LEVELING_UP;
					wave = 0;
					score = 0;
					currentBullet = 0;
					bulletsSpare = 48;
					bulletsInClip = 6;
					clipSize = 6;
					fireRate = 1;

					player.resetPlayerStats();
				}
				if (state == State::PLAYING)
				{
					if (event.key.code == Keyboard::R)
					{
						if (bulletsSpare >= clipSize)
						{
							bulletsInClip = clipSize;
							bulletsSpare -= clipSize;
							reload.play();
						}
						else if (bulletsSpare > 0)
						{
							bulletsInClip = bulletsSpare;
							bulletsSpare = 0;
							reload.play();
						}
						else
						{
							reloadFailed.play();
						}
					}
				}
			}
		}

		if (Keyboard::isKeyPressed(Keyboard::Escape))
		{
			window.close();
		}

		if (state == State::PLAYING)
		{
			if (Keyboard::isKeyPressed(Keyboard::W))
			{
				player.moveUp();
			}
			else
			{
				player.stopUp();
			}
			if (Keyboard::isKeyPressed(Keyboard::A))
			{
				player.moveLeft();
			}
			else
			{
				player.stopLeft();
			}
			if (Keyboard::isKeyPressed(Keyboard::S))
			{
				player.moveDown();
			}
			else
			{
				player.stopDown();
			}
			if (Keyboard::isKeyPressed(Keyboard::D))
			{
				player.moveRight();
			}
			else
			{
				player.stopRight();
			}
			if (Mouse::isButtonPressed(sf::Mouse::Left))
			{
				if (gameTimeTotal.asMilliseconds() - lastPressed.asMilliseconds() > 1000 / fireRate && bulletsInClip > 0)
				{
					bullets[currentBullet].shoot(player.getCenter().x, player.getCenter().y, mouseWorldPosition.x, mouseWorldPosition.y);
					currentBullet++;
					if (currentBullet > 99)
					{
						currentBullet = 0;
					}
					lastPressed = gameTimeTotal;
					shoot.play();
					bulletsInClip--;
				}
			}
		}

		if (state == State::LEVELING_UP)
		{
			// increase Player's fire rate
			if (event.key.code == Keyboard::Num1)
			{
				fireRate++;
				state = State::PLAYING;
			}

			// increase magazine size
			if (event.key.code == Keyboard::Num2)
			{
				clipSize += clipSize;
				state = State::PLAYING;
			}

			// increase base HP
			if (event.key.code == Keyboard::Num3)
			{
				player.upgradeHealth();
				state = State::PLAYING;
			}

			// increase run speed
			if (event.key.code == Keyboard::Num4)
			{
				player.upgradeSpeed();
				state = State::PLAYING;
			}

			// increase health pickups
			if (event.key.code == Keyboard::Num5)
			{
				healthPickup.upgrade();
				state = State::PLAYING;
			}

			// increase ammo pickups
			if (event.key.code == Keyboard::Num6)
			{
				ammoPickup.upgrade();
				state = State::PLAYING;
			}

			if (state == State::PLAYING)
			{
				// start playing level music
				gameMusic.stop();
				gameMusic.play();

				wave++;
				arena.width = 650 * (1 + wave * 0.1);
				arena.height = 650 * (1 + wave * 0.1);
				arena.left = 0;
				arena.top = 0;

				int tileSize = createBackground(background, arena);

				player.spawn(arena, resolution, tileSize);

				healthPickup.setArena(arena);
				ammoPickup.setArena(arena);

				numZombies = 5 + ((wave - 1) * 2);

				delete[] zombies;
				zombies = createHorde(numZombies, arena);
				numZombiesAlive = numZombies;

				zombies->upgradeSpeed(1, wave);
				zombies->upgradeSpeed(2, wave);
				zombies->upgradeSpeed(3, wave);

				powerUp.play();

				clock.restart();
			}
		}

		if (state == State::PLAYING)
		{
			
			if (menuMusic.getStatus() == Music::Playing)
			{
				menuMusic.stop();
				gameMusic.play();
			}

			Time dt = clock.restart();
			gameTimeTotal += dt;

			float dtAsSeconds = dt.asSeconds();

			mouseScreenPosition = Mouse::getPosition();
			mouseWorldPosition = window.mapPixelToCoords(Mouse::getPosition(), mainView);

			spriteCrosshair.setPosition(mouseWorldPosition);

			player.update(dtAsSeconds, Mouse::getPosition());

			Vector2f playerPosition(player.getCenter());
			mainView.setCenter(player.getCenter());

			for (int i = 0; i < numZombies; ++i)
			{
				if (zombies[i].isAlive())
				{
					zombies[i].update(dt.asSeconds(), playerPosition);
				}
			}

			for (int i = 0; i < 100; ++i)
			{
				if (bullets[i].isInFlight())
				{
					bullets[i].update(dtAsSeconds);
				}
			}

			healthPickup.update(dtAsSeconds);
			ammoPickup.update(dtAsSeconds);

			for (int i = 0; i < 100; ++i)
			{
				for (int j = 0; j < numZombies; ++j)
				{
					if (bullets[i].isInFlight() && zombies[j].isAlive())
					{
						if (bullets[i].getPosition().intersects(zombies[j].getPosition()))
						{
							bullets[i].stop();
							if (zombies[j].hit())
							{
								score += 10;
								if (score > hiScore)
								{
									hiScore = score;
								}
								numZombiesAlive--;
								if (numZombiesAlive == 0)
								{
									state = State::LEVELING_UP;
								}
							}
							splat.play();
						}
					}
				}
			}

			for (int i = 0; i < numZombies; ++i)
			{
				if (player.getPosition().intersects(zombies[i].getPosition()) && zombies[i].isAlive())
				{
					if (player.hit(gameTimeTotal))
					{
						hit.play();
					}
					if (player.getHealth() <= 0)
					{
						dying.play();
						state = State::GAME_OVER;
						ofstream outputFile("gamedata/scores.txt");
						outputFile << hiScore;
						outputFile.close();
					}
				}
			}

			if (player.getPosition().intersects(healthPickup.getPosition()) && healthPickup.isSpawned())
			{
				player.increaseHealthLevel(healthPickup.gotIt());
				pickup.play();
			}

			if (player.getPosition().intersects(ammoPickup.getPosition()) && ammoPickup.isSpawned())
			{
				bulletsSpare += ammoPickup.gotIt();
				reload.play();
			}

			healthBar.setSize(Vector2f(player.getHealth() * 3, 50));
			frameSinceLastHudUpdate++;

			if (frameSinceLastHudUpdate > fpsMeasureFrameInterval)
			{
				// time to update the HUD
				stringstream ssAmmo;
				stringstream ssScore;
				stringstream ssHiScore;
				stringstream ssWave;
				stringstream ssremainingZombiesText;

				ssAmmo << bulletsInClip << "/" << bulletsSpare;
				ammoText.setString(ssAmmo.str());

				ssScore << "Score: " << score;
				scoreText.setString(ssScore.str());

				ssHiScore << "Hi Score: " << hiScore;
				hiScoreText.setString(ssHiScore.str());

				ssWave << "Wave: " << wave;
				waveNumberText.setString(ssWave.str());

				ssremainingZombiesText << "Zombies: " << numZombiesAlive;
				remainingZombiesText.setString(ssremainingZombiesText.str());

				frameSinceLastHudUpdate = 0;
			}
		}

		// draw scene
		if (state == State::PLAYING)
		{
			window.clear();
			window.setView(mainView);
			window.draw(background, &textureBackground);

			for (int i = 0; i < numZombies; ++i)
			{
				window.draw(zombies[i].getSprite());
			}

			for (int i = 0; i < 100; ++i)
			{
				if (bullets[i].isInFlight())
					window.draw(bullets[i].getShape());
			}

			window.draw(player.getSprite());

			if (ammoPickup.isSpawned())
			{
				window.draw(ammoPickup.getSprite());
			}

			if (healthPickup.isSpawned())
			{
				window.draw(healthPickup.getSprite());
			}

			window.draw(spriteCrosshair);

			window.setView(hudView);

			// draw HUD
			window.draw(spriteAmmoIcon);
			window.draw(ammoText);
			window.draw(scoreText);
			window.draw(hiScoreText);
			window.draw(healthBar);
			window.draw(waveNumberText);
			window.draw(remainingZombiesText);
		}

		if (state == State::LEVELING_UP)
		{
			window.draw(spriteGameOver);
			window.draw(levelUpText);
		}
		if (state == State::PAUSED)
		{
			window.draw(pausedText);
		}

		if (state == State::GAME_OVER)
		{
			window.draw(spriteGameOver);
			window.draw(gameOverText);
			window.draw(scoreText);
			window.draw(hiScoreText);
		}

		window.display();
	} // end game loop

	delete[] zombies;

	return 0;
}