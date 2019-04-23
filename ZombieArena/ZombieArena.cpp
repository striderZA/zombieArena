#include "pch.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "Engine.h"

using namespace sf;
using namespace std;

int main()
{
	Engine engine;

	engine.run();

	return 0;
}