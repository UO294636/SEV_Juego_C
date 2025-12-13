#include "Portal.h"

Portal::Portal(float x, float y, Game* game)
	: Actor("res/portal.png", x, y, 20, 20, game) {

	aWaiting = new Animation("res/portal_animado.png", width, height,
		96, 16, 8, 4, true, game);
	animation = aWaiting;

	vx = 0;
}

void Portal::update() {
	// Actualizar la animación
	animation->update();
}


void Portal::draw(float scrollX) {
	animation->draw(x - scrollX, y);
}