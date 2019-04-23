#ifndef ZOMBIEARENA_H
#define ZOMBIEARENA_H

#include <SFML/Graphics.hpp>
#include "Zombie.h"

using namespace sf;

int createBackground(VertexArray &rVA, FloatRect arena);
Zombie* createHorde(int numZombies, FloatRect arena);

#endif /*ZOMBIEARENA_H*/
