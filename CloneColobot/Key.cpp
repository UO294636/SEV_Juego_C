#include "Key.h"

Key::Key(float x, float y, Game* game)
	: Actor("res/llave.png", x, y, 20, 20, game) {

	vx = 0;
}

void Key::draw(float scrollX) {
	Actor::draw(scrollX);
}