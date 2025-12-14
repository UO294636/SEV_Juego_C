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
		508, 49, 6, 8, true, game);
	aWalkingLeft = new Animation("res/colobot_caminando_izquierda.png", width, height,
		508, 49, 6, 8, true, game);
	aWalkingUp = new Animation("res/colobot_caminando_arriba.png", width, height,
		512, 49, 6, 8, true, game);
	aWalkingDown = new Animation("res/colobot_caminando_abajo.png", width, height,
		512, 50, 6, 8, true, game);
	aDie = new Animation("res/colobot_muerte.png", width, height,
		389, 51, 6, 6, false, game);

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

	// Update animation frame
	bool endAnimation = animation->update();

	if (collisionDown == true) {
		onAir = false;
	}
	else {
		onAir = true;
	}

	// Acabo la animación
	if (endAnimation) {
		// Estaba disparando
		if (state == game->stateShooting) {
			state = game->stateMoving;
		}
		// Death animation finished - state remains stateDying
		// GameLayer will check isDeathAnimationFinished() to restart level
	}

	// If player is in dying state, use death animation and stop movement
	if (state == game->stateDying) {
		animation = aDie;
		vx = 0;
		vy = 0;
		inputVx = 0;
		inputVy = 0;
		// Don't process other animation logic
		return;
	}

	// Establecer orientación basada en el INPUT de movimiento (no la velocidad real)
	if (inputVx > 0) {
		orientation = game->orientationRight;
	}
	else if (inputVx < 0) {
		orientation = game->orientationLeft;
	}

	// Selección de animación basada en INPUT de movimiento
	// Si no hay movimiento de input, usar animación idle
	if (inputVx == 0 && inputVy == 0) {
		animation = aIdle;
	}
	// Si hay movimiento horizontal, priorizar animaciones izquierda/derecha
	else if (inputVx != 0) {
		if (inputVx > 0) {
			animation = aWalkingRight;
		}
		else {
			animation = aWalkingLeft;
		}
	}
	// Si solo hay movimiento vertical (sin horizontal)
	else if (inputVy != 0) {
		if (inputVy > 0) {
			animation = aWalkingDown; // Moviendo hacia abajo
		}
		else {
			animation = aWalkingUp; // Moviendo hacia arriba
		}
	}

	if (shootTime > 0) {
		shootTime--;
	}
}

void Player::moveX(float axis) {
	inputVx = axis; // Store input direction
	vx = axis * 3;  // Set actual velocity
}

void Player::moveY(float axis) {
	inputVy = axis; // Store input direction
	vy = axis * 3;  // Set actual velocity
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
		}
	}
}

bool Player::isDeathAnimationFinished() {
	// Check if in dying state and animation is death animation
	// Animation update returns true when it completes a cycle
	if (state == game->stateDying && animation == aDie) {
		// Check if animation has completed by calling update
		// This is checked in GameLayer after player update
		return animation->update();
	}
	return false;
}