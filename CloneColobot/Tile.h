#pragma once
#include "Actor.h"

class Tile : public Actor
{
public:
	Tile(string filename, float x, float y, Game* game);
	void draw(float scrollX = 0) override;
	bool visible = true; // Control de visibilidad del tile
};

