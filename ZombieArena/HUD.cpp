#include "pch.h"
#include "HUD.h"

HUD::HUD()
{
	Vector2u resolution;
	resolution.x = VideoMode::getDesktopMode().width;
	resolution.y = VideoMode::getDesktopMode().height;

	_font.loadFromFile("fonts/big_noodle_titling.ttf");

	FloatRect textRect = _titleText.getLocalBounds();
	_titleText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);

	Vector2f textPosition(0.0f, 0.0f);
	initializeText(_titleText, TITLE_TEXT_SIZE, "Zombies", textPosition);
	initializeText(_pausedText, TITLE_TEXT_SIZE, "Press Enter to Resume", textPosition);
	initializeText(_gameOverText, TITLE_TEXT_SIZE, "Press Enter to Play", textPosition);
	initializeText(_levelUpText, DEFAULT_TEXT_SIZE, "", textPosition);

	std::stringstream levelUpStream;
	levelUpStream << "1: Increased rate of fire\n" <<
		"2: Increased magazine size\n" <<
		"3: Increased maximum health\n" <<
		"4: Increased run speed\n" <<
		"5: Improved health pickups\n" <<
		"6: Improved ammo pickups";

	_levelUpText.setString(levelUpStream.str());

	initializeText(_ammoText, DEFAULT_TEXT_SIZE, "", textPosition);
	initializeText(_scoreText, DEFAULT_TEXT_SIZE, "", textPosition);
	initializeText(_hiScoreText, DEFAULT_TEXT_SIZE, "", textPosition);
	initializeText(_remainingZombiesText, DEFAULT_TEXT_SIZE, "", textPosition);
	initializeText(_waveNumberText, DEFAULT_TEXT_SIZE, "", textPosition);

	_healthBar.setFillColor(Color::Red);

	
}

HUD::~HUD()
{
}

void HUD::initializeText(Text input, unsigned int characterSize, std::string displayText, Vector2f position)
{
	input.setFont(_font);
	input.setCharacterSize(characterSize);
	input.setFillColor(Color::White);
	input.setString(displayText);
	input.setStyle(Text::Italic);
}

Text HUD::getTitleText()
{
	return _titleText;
}

Text HUD::getPausedText()
{
	return _pausedText;
}

Text HUD::getGameOverText()
{
	return _gameOverText;
}

Text HUD::getLevelUpText()
{
	return _levelUpText;
}

Text HUD::getAmmoText()
{
	return _ammoText;
}

Text HUD::getScoreText()
{
	return _scoreText;
}

Text HUD::getHiScoreText()
{
	return _hiScoreText;
}

Text HUD::getRemainingZombiesText()
{
	return _remainingZombiesText;
}

Text HUD::getWaveNumberText()
{
	return _waveNumberText;
}

void HUD::setTitleText(std::string text)
{
	_titleText.setString(text);
}

void HUD::setPausedText(std::string text)
{
	_pausedText.setString(text);
}

void HUD::setGameOverText(std::string text)
{
	_gameOverText.setString(text);
}

void HUD::setLevelUpText(std::string text)
{
	_levelUpText.setString(text);
}

void HUD::setAmmoText(std::string text)
{
	_ammoText.setString(text);
}

void HUD::setScoreText(std::string text)
{
	_scoreText.setString(text);
}

void HUD::setHiScoreText(std::string text)
{
	_hiScoreText.setString(text);
}

void HUD::setRemainingZombiesText(std::string text)
{
	_remainingZombiesText.setString(text);
}

void HUD::setWaveNumberText(std::string text)
{
	_waveNumberText.setString(text);
}
