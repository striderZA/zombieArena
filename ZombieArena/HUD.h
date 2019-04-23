#ifndef HUD_H
#define HUD_H

#include <SFML/Graphics.hpp>
#include <sstream>

using namespace sf;

class HUD
{
private:

	const unsigned int DEFAULT_TEXT_SIZE = 55;
	const unsigned int TITLE_TEXT_SIZE = 100;
	Font _font;

	Text _titleText;
	Text _pausedText;
	Text _gameOverText;
	Text _levelUpText;
	Text _ammoText;
	Text _scoreText;
	Text _hiScoreText;
	Text _remainingZombiesText;
	Text _waveNumberText;

	RectangleShape _healthBar;

	void initializeText(Text input, unsigned int characterSize, std::string displayText, Vector2f position);

public:
	HUD();
	virtual ~HUD();

	Text getTitleText();
	Text getPausedText();
	Text getGameOverText();
	Text getLevelUpText();
	Text getAmmoText();
	Text getScoreText();
	Text getHiScoreText();
	Text getRemainingZombiesText();
	Text getWaveNumberText();

	void setTitleText(std::string text);
	void setPausedText(std::string text);
	void setGameOverText(std::string text);
	void setLevelUpText(std::string text);
	void setAmmoText(std::string text);
	void setScoreText(std::string text);
	void setHiScoreText(std::string text);
	void setRemainingZombiesText(std::string text);
	void setWaveNumberText(std::string text);
	
};

#endif /*HUD_H*/