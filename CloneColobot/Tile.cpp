#include "Tile.h"

Tile::Tile(string filename, float x, float y, Game* game)
	: Actor(filename, x, y, 35, 35, game) {

}

void Tile::draw(float scrollX) {
	if (!visible) {
		return; // No dibujar si no es visible
	}
	Actor::draw(scrollX);
}
