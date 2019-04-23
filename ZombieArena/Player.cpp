#include "pch.h"
#include "Player.h"
#include "TextureHolder.h"

Player::Player()
{
	_speed = START_SPEED;
	_currentHealth = START_HEALTH;
	_maxHealth = START_HEALTH;

	_sprite = Sprite(TextureHolder::GetTexture("graphics/player.png"));
	_sprite.setOrigin(25, 25);
}

Player::~Player()
{
}

void Player::spawn(FloatRect arena, Vector2f resolution, int tileSize)
{
	_position.x = arena.width / 2.f;
	_position.y = arena.height / 2.f;

	_arena.left = arena.left;
	_arena.width = arena.width;
	_arena.top = arena.top;
	_arena.height = arena.height;

	_tileSize = tileSize;
	_resolution.x = resolution.x;
	_resolution.y = resolution.y;
}

void Player::resetPlayerStats()
{
	_speed = START_SPEED;
	_currentHealth = START_HEALTH;
	_maxHealth = START_HEALTH;
}

bool Player::hit(Time timeHit)
{
	if (timeHit.asMilliseconds() - _lastHit.asMilliseconds() > 250)
	{
		_lastHit = timeHit;
		_currentHealth -= 10;
		return true;
	}
	else
	{
		return false;
	}
}

Time Player::getLastHitTime()
{
	return _lastHit;
}

FloatRect Player::getPosition()
{
	return _sprite.getGlobalBounds();
}

Vector2f Player::getCenter()
{
	return _position;
}

float Player::getRotation()
{
	return _sprite.getRotation();
}

Sprite Player::getSprite()
{
	return _sprite;
}

void Player::moveLeft()
{
	_leftPressed = true;
}

void Player::moveRight()
{
	_rightPressed = true;
}

void Player::moveUp()
{
	_upPressed = true;
}

void Player::moveDown()
{
	_downPressed = true;
}

void Player::stopLeft()
{
	_leftPressed = false;
}

void Player::stopRight()
{
	_rightPressed = false;
}

void Player::stopUp()
{
	_upPressed = false;
}

void Player::stopDown()
{
	_downPressed = false;
}

void Player::update(float elapsedTime, Vector2i mousePosition)
{
	if (_upPressed)
		_position.y -= _speed * elapsedTime;

	if (_downPressed)
		_position.y += _speed * elapsedTime;

	if (_leftPressed)
		_position.x -= _speed * elapsedTime;

	if (_rightPressed)
		_position.x += _speed * elapsedTime;

	_sprite.setPosition(_position);

	if (_position.x > _arena.width - _tileSize)
		_position.x = _arena.width - _tileSize;

	if (_position.x < _arena.left + _tileSize)
		_position.x = _arena.left + _tileSize;

	if (_position.y > _arena.height - _tileSize)
		_position.y = _arena.height - _tileSize;

	if (_position.y < _arena.top + _tileSize)
		_position.y = _arena.top + _tileSize;

	_sprite.setRotation(atan2(mousePosition.y - _resolution.y / 2.f, mousePosition.x - _resolution.x / 2.f) * 180 / 3.141);
}

void Player::upgradeSpeed()
{
	_speed += START_HEALTH * (float)0.1;
}

void Player::upgradeHealth()
{
	_maxHealth += START_HEALTH * (float)0.1;
}

void Player::increaseHealthLevel(float amount)
{
	_currentHealth += amount;

	if (_currentHealth > _maxHealth)
		_currentHealth = _maxHealth;
}

float Player::getHealth()
{
	return _currentHealth;
}