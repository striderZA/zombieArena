#include "pch.h"
#include "TextureHolder.h"
#include <assert.h>

TextureHolder *TextureHolder::Instance = nullptr;

TextureHolder::TextureHolder()
{
	assert(Instance == nullptr);
	Instance = this;
}

TextureHolder::~TextureHolder()
{
}

Texture & TextureHolder::GetTexture(string const & fileName)
{
	auto &m = Instance->Textures;
	auto keyValuePair = m.find(fileName);

	if (keyValuePair != m.end())
		return keyValuePair->second;
	else
	{
		auto &texture = m[fileName];
		texture.loadFromFile(fileName);
		return texture;
	}
}