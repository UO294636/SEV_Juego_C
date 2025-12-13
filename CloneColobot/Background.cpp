#include "Background.h"

Background::Background(string filename, float x, float y, Game* game)
	: Actor(filename, x, y, WIDTH, HEIGHT, game) {

}

Background::Background(string filename, float x, float y, float vx, Game* game)
	: Actor(filename, x, y, WIDTH, HEIGHT, game) {

	this->vx = vx;
	if (vx != 0) {
		backgroundAux = new Background(filename, x + WIDTH, y, game);
	}
}

void Background::update() {
	if (vx != 0) {
		x = x + vx;

		// se salio por la izquierda
		if (x + width / 2 < 0) {
			// vuelve a aparecer por la derecha
			x = WIDTH + width / 2;
		}
		// se salio por la derecha
		if (x - width / 2 > WIDTH) {
			// vuelve por la izquierda
			x = 0 - width / 2;
		}

	}
}

void Background::draw(float scrollX) {
	// Apply scroll offset to background position for parallax effect
	float originalX = x;
	x = x - scrollX;

	Actor::draw(0); // Pass 0 as scrollX since we already adjusted position

	// Restore original position
	x = originalX;

	if (backgroundAux != NULL) {
		// zona sin cubrir por la izquierda
		if (x - width / 2 > scrollX) {
			// pintar aux por la izquierda
			backgroundAux->x = x - width;
			float auxOriginalX = backgroundAux->x;
			backgroundAux->x = backgroundAux->x - scrollX;
			backgroundAux->draw(0);
			backgroundAux->x = auxOriginalX;
		}
		// zona sin cubrir por la derecha
		if (x + width / 2 < WIDTH + scrollX) {
			// pintar aux por la derecha
			backgroundAux->x = x + width;
			float auxOriginalX = backgroundAux->x;
			backgroundAux->x = backgroundAux->x - scrollX;
			backgroundAux->draw(0);
			backgroundAux->x = auxOriginalX;
		}
	}
}

