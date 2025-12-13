#include "Battery.h"

Battery::Battery(float x, float y, Game* game)
	: Actor("res/bateria.png", x, y, 20, 20, game) {

	vx = 0;
}

void Battery::update() {
	// No special update logic needed for batteries
}

void Battery::draw(float scrollX) {
	Actor::draw(scrollX);
}