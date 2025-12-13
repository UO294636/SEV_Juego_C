#include "Box.h"

Box::Box(float x, float y, Game* game)
	: Actor("res/caja.png", x, y, 35, 35, game) {

	aDie = new Animation("res/caja_animada.png", width, height,
		490, 35, 0.5, 14, false, game);
	animation = NULL;

	vx = 0;
}

void Box::update() {
	// Actualizar la animación
	if (animation != NULL) {
		animation->update();
	}
	
}


void Box::draw(float scrollX) {
	if (animation != NULL) {
		animation->draw(x - scrollX, y);
	}
	else {
		Actor::draw(scrollX);
	}
}
