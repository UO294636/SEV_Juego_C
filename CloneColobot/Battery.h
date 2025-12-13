#include "Actor.h"
#include "Audio.h"
#include "Animation.h" // incluir animacion 

class Battery : public Actor
{
public:
	Battery(float x, float y, Game* game);
	void draw(float scrollX = 0) override; // Va a sobrescribir
	void update();
};

