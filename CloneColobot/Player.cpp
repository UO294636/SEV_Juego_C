#include "Player.h"

Player::Player(float x, float y, Game* game)
	: Actor("res/colobot.png", x, y, 30, 30, game) {

	onAir = false;
	orientation = game->orientationRight;
	state = game->stateMoving;
	audioShoot = Audio::createAudio("res/efecto_disparo.wav", false);

	aIdle = new Animation("res/colobot_idle.png", width, height,
		128, 48, 6, 2, true, game);
	aWalkingRight = new Animation("res/colobot_caminando_derecha.png", width, height,
		566, 50, 6, 8, true, game);
	aWalkingLeft = new Animation("res/colobot_caminando_izquierda.png", width, height,
		508, 49, 6, 8, true, game);
	aWalkingUp = new Animation("res/colobot_caminando_arriba.png", width, height,
		512, 49, 6, 8, true, game);
	aWalkingDown = new Animation("res/colobot_caminando_abajo.png", width, height,
		512, 50, 6, 8, true, game);
	aDie = new Animation("res/colobot_muerte.png", width, height,
		389, 51, 6, 6, true, game);

	animation = aIdle;

}


void Player::update() {
	// En el aire y moviéndose, PASA a estar saltando
	if (onAir && state == game->stateMoving) {
		state = game->stateJumping;
	}
	// No está en el aire y estaba saltando, PASA a moverse
	if (!onAir && state == game->stateJumping) {
		state = game->stateMoving;
	}


	if (invulnerableTime > 0) {
		invulnerableTime--;
	}

	bool endAnimation = animation->update();

	if (collisionDown == true) {
		onAir = false;
	}
	else {
		onAir = true;
	}


	// Acabo la animación, no sabemos cual
	if (endAnimation) {
		// Estaba disparando
		if (state == game->stateShooting) {
			state = game->stateMoving;
		}
	}


	// Establecer orientación
	if (vx > 0) {
		orientation = game->orientationRight;
	}
	if (vx < 0) {
		orientation = game->orientationLeft;
	}


	// Selección de animación basada en estados
	//if (state == game->stateJumping) {
	//	if (orientation == game->orientationRight) {
	//		animation = aJumpingRight;
	//	}
	//	if (orientation == game->orientationLeft) {
	//		animation = aJumpingLeft;
	//	}
	//}
	//if (state == game->stateShooting) {
	//	if (orientation == game->orientationRight) {
	//		animation = aShootingRight;
	//	}
	//	if (orientation == game->orientationLeft) {
	//		animation = aShootingLeft;
	//	}
	//}
	//if (state == game->stateMoving) {
	//	if (vx != 0) {
	//		if (orientation == game->orientationRight) {
	//			animation = aRunningRight;
	//		}
	//		if (orientation == game->orientationLeft) {
	//			animation = aRunningLeft;
	//		}
	//	}
	//	if (vx == 0) {
	//		if (orientation == game->orientationRight) {
	//			animation = aIdleRight;
	//		}
	//		if (orientation == game->orientationLeft) {
	//			animation = aIdleLeft;
	//		}
	//	}
	//}

	animation = aDie; // temporal


	if (shootTime > 0) {
		shootTime--;
	}

}

void Player::moveX(float axis) {
	vx = axis * 3;
}

void Player::moveY(float axis) {
	vy = axis * 3;
}

Projectile* Player::shoot() {

	if (shootTime == 0) {
		state = game->stateShooting;
		audioShoot->play();
		//aShootingLeft->currentFrame = 0; //"Rebobinar" aniamción
		//aShootingRight->currentFrame = 0; //"Rebobinar" aniamción
		shootTime = shootCadence;
		Projectile* projectile = new Projectile(x, y, game);
		if (orientation == game->orientationLeft) {
			projectile->vx = projectile->vx * -1; // Invertir
		}
		return projectile;
	}
	else {
		return NULL;
	}
}

void Player::draw(float scrollX) {
	if (invulnerableTime == 0) {
		animation->draw(x - scrollX, y);
	}
	else {
		if (invulnerableTime % 10 >= 0 && invulnerableTime % 10 <= 5) {
			animation->draw(x - scrollX, y);
		}
	}
}

void Player::jump() {
	if (!onAir) {
		vy = -16;
		onAir = true;
	}
}

void Player::loseLife() {
	if (invulnerableTime <= 0) {
		if (lifes > 0) {
			lifes--;
			invulnerableTime = 100;
			// 100 actualizaciones 
		}
	}
}
