#ifndef TEXTUREHOLDER_H
#define TEXTUREHOLDER_H

#include <SFML/Graphics.hpp>
#include <map>

using namespace std;
using namespace sf;

class TextureHolder
{
private:
	map<string, Texture> Textures;
	static TextureHolder *Instance;

public:
	TextureHolder();
	virtual ~TextureHolder();

	static Texture& GetTexture(string const &fileName);
};

#endif /*TEXTUREHOLDER_H*/