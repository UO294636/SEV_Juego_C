#pragma once

#include "Actor.h"
#include "Audio.h"
#include "Animation.h" // incluir animacion 

class Box : public Actor
{
public:
	Box(float x, float y, Game* game);
	void draw(float scrollX = 0) override; // Va a sobrescribir
	void update();
	Animation* aDie;
	Animation* animation; // Referencia a la animación mostrada
};

