#pragma once

#include "Actor.h"
#include "Projectile.h" 
#include "Audio.h"
#include "Animation.h" // incluir animacion 

class Player : public Actor
{
public:
	Player(float x, float y, Game* game);
	Projectile* shoot();
	void update();
	void jump();
	void moveX(float axis);
	void moveY(float axis);
	void draw(float scrollX = 0) override; // Va a sobrescribir
	void loseLife();
	bool isDeathAnimationFinished(); // Check if death animation has completed
	int lifes = 3;
	int invulnerableTime = 0;
	bool onAir;
	int orientation;
	int state;
	Animation* aIdle;
	Animation* aWalkingRight;
	Animation* aWalkingLeft;
	Animation* aWalkingUp;
	Animation* aWalkingDown;
	Animation* aDie;
	Animation* animation; // Referencia a la animaci?n mostrada
	Audio* audioShoot;
	int shootCadence = 30;
	int shootTime = 0;
	
	// Input direction - not affected by collisions
	float inputVx = 0;
	float inputVy = 0;
};

