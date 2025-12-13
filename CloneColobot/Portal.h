#pragma once

#include "Actor.h"
#include "Audio.h"
#include "Animation.h" // incluir animacion 

class Portal : public Actor
{
	public:
	Portal(float x, float y, Game* game);
	void draw(float scrollX = 0) override; // Va a sobrescribir
	void update();
	Animation* aWaiting;
	Animation* animation; // Referencia a la animación mostrada
};

