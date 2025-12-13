#include "GameLayer.h"

GameLayer::GameLayer(Game* game)
	: Layer(game) {
	//llama al constructor del padre : Layer(renderer)
	pause = true;
	message = new Actor("res/mensaje_como_jugar.png", WIDTH * 0.5, HEIGHT * 0.5,
		WIDTH, HEIGHT, game);

	gamePad = SDL_GameControllerOpen(0);
	init();
}


void GameLayer::init() {
	pad = new Pad(WIDTH * 0.15, HEIGHT * 0.80, game);

	buttonJump = new Actor("res/boton_salto.png", WIDTH * 0.9, HEIGHT * 0.55, 100, 100, game);
	buttonShoot = new Actor("res/boton_disparo.png", WIDTH * 0.75, HEIGHT * 0.83, 100, 100, game);

	space = new Space(0); // Disable gravity for 4-directional movement
	scrollX = 0;
	tiles.clear();

	audioBackground = Audio::createAudio("res/musica_ambiente.mp3", true);
	audioBackground->play();

	// Separate HUD elements for better organization - positioned to fit in screen
	textMovementsTitle = new Text("", WIDTH * 0.15, HEIGHT * 0.04, game);
	textMovementsTitle->content = "Movimientos:";
	
	textMovementsCounter = new Text("", WIDTH * 0.25, HEIGHT * 0.08, game);
	textMovementsCounter->content = "-";
	
	textMovementsQueue = new Text("", WIDTH * 0.5, HEIGHT * 0.35, game);
	textMovementsQueue->content = "";

	// Initialize key collection HUD
	textKeysCollected = new Text("", WIDTH * 0.5, HEIGHT * 0.04, game);
	textKeysCollected->content = "Llaves: 0/0";

	// Initialize battery HUD in top-right corner (where points used to be)
	textBattery = new Text("", WIDTH * 0.92, HEIGHT * 0.04, game);
	textBattery->content = "Bat: 10";

	background = new Background("res/fondo.png", WIDTH * 0.5, HEIGHT * 0.5, -1, game);


	enemies.clear(); // Vaciar por si reiniciamos el juego
	projectiles.clear(); // Vaciar por si reiniciamos el juego
	keys.clear(); // Vaciar lista de llaves
	boxes.clear(); // Vaciar lista de cajas
	
	// Reset key and door state
	totalKeys = 0;
	keysCollected = 0;
	doorOpen = false;
	door = NULL;
	portal = NULL; // Reset portal
	
	// Reset battery
	battery = 10;

	loadMap("res/" + to_string(game->currentLevel) + ".txt");

	// reset keyboard batching state
	keyboardActive = false;
	keyboardStartTimeMs = 0;
	keyQueue.clear();
	executingQueue = false;
	executingQueueVec.clear();
	lastActionTimeMs = 0;
	
	// Reset last move direction
	lastMoveDirection = 0;
}

void GameLayer::loadMap(string name) {
	char character;
	string line;
	ifstream streamFile(name.c_str());
	if (!streamFile.is_open()) {
		cout << "Falla abrir el fichero de mapa" << endl;
		return;
	}
	else {
		// Por l?nea
		for (int i = 0; getline(streamFile, line); i++) {
			istringstream streamLine(line);
			mapWidth = line.length() * 35; // Ancho del mapa en pixels (ajustado a 35)
			// Por car?cter (en cada l?nea)
			for (int j = 0; !streamLine.eof(); j++) {
				streamLine >> character; // Leer character 
				cout << character;
				float x = 35 / 2 + j * 35; // x central (ajustado a 35)
				float y = (i + 1) * 35; // y central - add 1 to i to push first row down
				loadMapObject(character, x, y);
			}

			cout << character << endl;
		}
	}
	streamFile.close();
}

void GameLayer::loadMapObject(char character, float x, float y)
{
	switch (character) {
	case 'B': {
		Box* box = new Box(x, y, game);
		// modificación para empezar a contar desde el suelo.
		box->y = box->y - box->height / 2;
		boxes.push_back(box);
		space->addStaticActor(box); // Caja empieza siendo sólida
		break;
	}
	case 'P': {
		portal = new Portal(x, y, game);
		// modificación para empezar a contar desde el suelo.
		portal->y = portal->y - portal->height / 2;
		space->addDynamicActor(portal);
		break;
	}
	case 'D': {
		door = new Tile("res/puerta_cerrada.png", x, y, game);
		// modificaci?n para empezar a contar desde el suelo.
		door->y = door->y - door->height / 2;
		tiles.push_back(door);
		space->addStaticActor(door); // La puerta bloquea el paso inicialmente
		break;
	}
	case 'K': {
		Key* key = new Key(x, y, game);
		// modificaci?n para empezar a contar desde el suelo.
		key->y = key->y - key->height/1.5;
		keys.push_back(key);
		totalKeys++;
		space->addDynamicActor(key); // Las llaves son dinámicas para poder recogerlas
		break;
	}
	case '1': {
		player = new Player(x, y, game);
		// modificaci?n para empezar a contar desde el suelo.
		player->y = player->y - player->height / 2;
		space->addDynamicActor(player);
		break;
	}
	case '#': {
		Tile* tile = new Tile("res/bloque_metal.png", x, y, game);
		// modificaci?n para empezar a contar desde el suelo.
		tile->y = tile->y - tile->height / 2;
		tiles.push_back(tile);
		space->addStaticActor(tile);
		break;
	}
	}
}

void GameLayer::processControls() {
	// obtener controles
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_CONTROLLERDEVICEADDED) {
			gamePad = SDL_GameControllerOpen(0);
			if (gamePad == NULL) {
				cout << "error en GamePad" << endl;
			}
			else {
				cout << "GamePad conectado" << endl;
			}
		}

		// Cambio autom?tico de input
		// PONER el GamePad
		if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERAXISMOTION) {
			// if switched away from keyboard reset batching
			if (game->input == game->inputKeyboard) {
				// Only allow reset if not currently executing queue
				if (!executingQueue) {
					keyboardActive = false;
					keyboardStartTimeMs = 0;
					keyQueue.clear();
					textMovementsCounter->content = "-";
					textMovementsQueue->content = "";
				}
			}
			game->input = game->inputGamePad;
		}
		if (event.type == SDL_KEYDOWN) {
			// when any keydown detected, change input to keyboard
			game->input = game->inputKeyboard;
		}
		if (event.type == SDL_MOUSEBUTTONDOWN) {
			if (game->input == game->inputKeyboard) {
				// Only allow reset if not currently executing queue
				if (!executingQueue) {
					keyboardActive = false;
					keyboardStartTimeMs = 0;
					keyQueue.clear();
					textMovementsCounter->content = "-";
					textMovementsQueue->content = "";
				}
			}
			game->input = game->inputMouse;
		}
		// Procesar teclas
		if (game->input == game->inputKeyboard) {
			keysToControls(event);
		}
		if (game->input == game->inputMouse) {
			mouseToControls(event);
		}
		// Procesar Mando
		if (game->input == game->inputGamePad) {  // gamePAD
			gamePadToControls(event);
		}
	}

	// Disparar
	if (controlContinue) {
		pause = false;
		controlContinue = false;
	}

	// If using keyboard batching mode, wait for Enter key to execute queued moves
	// Block input if currently executing queue
	if (game->input == game->inputKeyboard && !pause && !executingQueue) {
		// Update counter text with concise format to fit on screen
		if (keyQueue.empty()) {
			textMovementsCounter->content = "-";
		}
		else {
			textMovementsCounter->content = "(" + to_string(keyQueue.size()) + "/" + to_string(maxQueuedMoves) + ") ENTER";
		}
		
		// Update movement icons in center of screen
		string movementIcons = "";
		for (int code : keyQueue) {
			if (code == SDLK_RIGHT) movementIcons += "=>  ";
			else if (code == SDLK_LEFT) movementIcons += "<=  ";
			else if (code == SDLK_UP) movementIcons += "^|  ";
			else if (code == SDLK_DOWN) movementIcons += "v|  ";
			else if (code == SDLK_d) movementIcons += "khe  ";
		}
		textMovementsQueue->content = movementIcons;
	}
	else if (!executingQueue) {
		// Not using keyboard and not executing: maintain existing behavior for other inputs
		if (controlShoot) {
			Projectile* newProjectile = player->shoot();
			if (newProjectile != NULL) {
				space->addDynamicActor(newProjectile);
				projectiles.push_back(newProjectile);
			}
		}

		// Eje X
		if (controlMoveX > 0) {
			player->moveX(1);
		}
		else if (controlMoveX < 0) {
			player->moveX(-1);
		}
		else {
			player->moveX(0);
		}

		// Eje Y - Use moveY for 4-directional movement
		if (controlMoveY > 0) {
			player->moveY(1); // Move down
		}
		else if (controlMoveY < 0) {
			player->moveY(-1); // Move up
		}
		else {
			player->moveY(0); // Stop vertical movement
		}
	}
}

void GameLayer::update() {
	if (pause) {
		return;
	}

	// If player is dying, wait for death animation to finish before restarting
	if (player->state == game->stateDying) {
		// Update player to advance death animation
		player->update();
		
		// Check if death animation has finished
		if (player->isDeathAnimationFinished()) {
			// Animation finished, now restart level
			message = new Actor("res/mensaje_como_jugar.png", WIDTH * 0.5, HEIGHT * 0.5,
				WIDTH, HEIGHT, game);
			pause = true;
			init();
			return;
		}
		
		// Don't process anything else while death animation is playing
		return;
	}

	// Executing queued actions one-by-one with continuous movement until collision
	if (executingQueue && !executingQueueVec.empty()) {
		Uint32 now = SDL_GetTicks();
		
		// Check if we need to start a new action
		if (lastActionTimeMs == 0) {
			// First action - start immediately
			if (!executingQueueVec.empty()) {
				int code = executingQueueVec.front();
				
				// Set velocity for continuous movement in 4 directions
				if (code == SDLK_RIGHT) {
					player->moveX(1); // Use moveX to update both vx and inputVx
					player->moveY(0);
				}
				else if (code == SDLK_LEFT) {
					player->moveX(-1); // Use moveX to update both vx and inputVx
					player->moveY(0);
				}
				else if (code == SDLK_UP) {
					player->moveX(0);
					player->moveY(-1); // Use moveY to update both vy and inputVy
				}
				else if (code == SDLK_DOWN) {
					player->moveX(0);
					player->moveY(1); // Use moveY to update both vy and inputVy
				}
				else if (code == SDLK_d) {
					Projectile* newProjectile = player->shoot();
					if (newProjectile != NULL) {
						space->addDynamicActor(newProjectile);
						projectiles.push_back(newProjectile);
					}
				}
				
				lastActionTimeMs = now;
			}
		}
		else {
			// Check if current movement has finished
			int currentCode = executingQueueVec.front();
			bool shouldMoveToNext = false;
			
			// Check for box collision early, before checking if blocked
			// This allows removing the box before collision detection
			if (currentCode == SDLK_RIGHT || currentCode == SDLK_LEFT || 
				currentCode == SDLK_UP || currentCode == SDLK_DOWN) {
				checkBoxCollision(currentCode);
			}
			
			if (currentCode == SDLK_RIGHT) {
				// Player wanted to move right - check if blocked
				if (player->vx <= 0) {
					shouldMoveToNext = true;
				}
			}
			else if (currentCode == SDLK_LEFT) {
				// Player wanted to move left - check if blocked
				if (player->vx >= 0) {
					shouldMoveToNext = true;
				}
			}
			else if (currentCode == SDLK_UP) {
				// Player wanted to move up - check if blocked
				if (player->vy >= 0) {
					shouldMoveToNext = true;
				}
			}
			else if (currentCode == SDLK_DOWN) {
				// Player wanted to move down - check if blocked
				if (player->vy <= 0) {
					shouldMoveToNext = true;
				}
			}
			else if (currentCode == SDLK_d) {
				// Shoot is instantaneous
				shouldMoveToNext = true;
			}
			
			// Move to next action if current one finished and delay passed
			if (shouldMoveToNext && now - lastActionTimeMs >= (Uint32)actionDelayMs) {
				// Update last move direction for next iteration
				if (currentCode == SDLK_RIGHT || currentCode == SDLK_LEFT || 
					currentCode == SDLK_UP || currentCode == SDLK_DOWN) {
					lastMoveDirection = currentCode;
				}
				
				// Pop current action
				executingQueueVec.erase(executingQueueVec.begin());
				
				// Decrease battery for the completed action
				if (battery > 0) {
					battery--;
					textBattery->content = "Bat: " + to_string(battery);
				}
				
				// Update HUD displays during execution
				if (executingQueueVec.empty()) {
					textMovementsCounter->content = "Completo!";
					textMovementsQueue->content = "";
				}
				else {
					textMovementsCounter->content = "(" + to_string(executingQueueVec.size()) + " quedan)";
					
					// Update remaining movements display in center
					string remainingMovements = "";
					for (int c : executingQueueVec) {
						if (c == SDLK_RIGHT) remainingMovements += "→  ";
						else if (c == SDLK_LEFT) remainingMovements += "←  ";
						else if (c == SDLK_UP) remainingMovements += "↑  ";
						else if (c == SDLK_DOWN) remainingMovements += "↓  ";
						else if (c == SDLK_d) remainingMovements += "🔫  ";
					}
					textMovementsQueue->content = remainingMovements;
				}
				
				// Start next action if available
				if (!executingQueueVec.empty()) {
					player->moveX(0); // Reset both vx and inputVx
					player->moveY(0); // Reset both vy and inputVy
					
					int nextCode = executingQueueVec.front();
					
					if (nextCode == SDLK_RIGHT) {
						player->moveX(1);
						player->moveY(0);
					}
					else if (nextCode == SDLK_LEFT) {
						player->moveX(-1);
						player->moveY(0);
					}
					else if (nextCode == SDLK_UP) {
						player->moveX(0);
						player->moveY(-1);
					}
					else if (nextCode == SDLK_DOWN) {
						player->moveX(0);
						player->moveY(1);
					}
					else if (nextCode == SDLK_d) {
						Projectile* newProjectile = player->shoot();
						if (newProjectile != NULL) {
							space->addDynamicActor(newProjectile);
							projectiles.push_back(newProjectile);
						}
					}
					
					lastActionTimeMs = now;
				}
			}
		}
	}
	else if (executingQueue && executingQueueVec.empty()) {
		// finished - stop player movement
		executingQueue = false;
		lastActionTimeMs = 0;
		textMovementsCounter->content = "-";
		textMovementsQueue->content = "";
		player->moveX(0); // Stop player when queue is empty (updates both vx and inputVx)
		player->moveY(0); // Stop player when queue is empty (updates both vy and inputVy)
		
		// Check if player reached the portal during execution
		bool reachedPortal = false;
		if (portal != NULL && player->isOverlap(portal)) {
			reachedPortal = true;
		}
		
		// If didn't reach portal, set player state to dying (Player class handles animation)
		if (!reachedPortal) {
			player->state = game->stateDying;
			return; // Return here to start death animation in next frame
		}
	}

	// Check for key collection
	list<Key*> keysToRemove;
	for (auto const& key : keys) {
		if (player->isOverlap(key)) {
			keysToRemove.push_back(key);
			keysCollected++;
			textKeysCollected->content = "Llaves: " + to_string(keysCollected) + "/" + to_string(totalKeys);
			
			// Check if all keys collected
			if (keysCollected >= totalKeys && door != NULL && !doorOpen) {
				doorOpen = true;
				// Change door appearance to open door using cached texture
				door->texture = game->getTexture("res/puerta_abierta.png");
				// Remove door from static actors so it doesn't block anymore
				space->removeStaticActor(door);
			}
		}
	}
	
	// Remove collected keys
	for (auto const& keyToRemove : keysToRemove) {
		keys.remove(keyToRemove);
		space->removeDynamicActor(keyToRemove);
		delete keyToRemove;
	}
	keysToRemove.clear();



	// Check for level completion - player reaches portal
	if (portal != NULL && player->isOverlap(portal)) {
		game->currentLevel++;
		if (game->currentLevel > game->finalLevel) {
			game->currentLevel = 0;
		}
		message = new Actor("res/mensaje_ganar.png", WIDTH * 0.5, HEIGHT * 0.5,
			WIDTH, HEIGHT, game);
		pause = true;
		init();
		return;
	}

	// Jugador se cae
	if (player->y > HEIGHT + 80) {
		init();
	}

	space->update();
	player->update();
	// Update portal animation if it exists
	if (portal != NULL) {
		portal->update();
	}
	// Update boxes
	for (auto const& box : boxes) {
		box->update();
	}
	for (auto const& enemy : enemies) {
		enemy->update();
	}
	for (auto const& projectile : projectiles) {
		projectile->update();
	}

	// Colisiones
	for (auto const& enemy : enemies) {
		if (player->isOverlap(enemy)) {
			player->loseLife();
			if (player->lifes <= 0) {
				init();
				return;
			}
		}
	}

	// Colisiones , Enemy - Projectile
	list<Enemy*> deleteEnemies;
	list<Projectile*> deleteProjectiles;
	for (auto const& projectile : projectiles) {
		// With static camera, check if projectile is still in render bounds (scrollX is always 0)
		if (projectile->isInRender(0) == false || projectile->vx == 0) {
			bool pInList = std::find(deleteProjectiles.begin(),
				deleteProjectiles.end(),
				projectile) != deleteProjectiles.end();

			if (!pInList) {
				deleteProjectiles.push_back(projectile);
			}
		}
	}

	for (auto const& enemy : enemies) {
		for (auto const& projectile : projectiles) {
			if (enemy->isOverlap(projectile)) {
				bool pInList = std::find(deleteProjectiles.begin(),
					deleteProjectiles.end(),
					projectile) != deleteProjectiles.end();

				if (!pInList) {
					deleteProjectiles.push_back(projectile);
				}

				enemy->impacted();
				// No longer tracking points
			}
		}
	}

	for (auto const& enemy : enemies) {
		if (enemy->state == game->stateDead) {
			bool eInList = std::find(deleteEnemies.begin(),
				deleteEnemies.end(),
				enemy) != deleteEnemies.end();

			if (!eInList) {
				deleteEnemies.push_back(enemy);
			}
		}
	}

	for (auto const& delEnemy : deleteEnemies) {
		enemies.remove(delEnemy);
		space->removeDynamicActor(delEnemy);
	}
	deleteEnemies.clear();

	for (auto const& delProjectile : deleteProjectiles) {
		projectiles.remove(delProjectile);
		space->removeDynamicActor(delProjectile);
		delete delProjectile;
	}
	deleteProjectiles.clear();

	cout << "update GameLayer" << endl;
}

void GameLayer::calculateScroll() {
	// Camera is now static - no scroll calculation needed
	scrollX = 0;
}

void GameLayer::checkBoxCollision(int currentDirection) {
	// Only destroy box if moving in same direction as last time
	if (currentDirection != lastMoveDirection) {
		return; // Different direction, don't destroy box
	}
	
	// Check collision with boxes
	for (auto const& box : boxes) {
		if (box->animation == NULL && player->isOverlap(box)) {
			// Player is colliding with a solid box while moving same direction twice
			// Activate box death animation
			box->animation = box->aDie;
			// Remove box from static actors so it no longer blocks
			space->removeStaticActor(box);
			
			// Re-apply movement to player since box is now gone
			if (currentDirection == SDLK_RIGHT) {
				player->moveX(1);
			}
			else if (currentDirection == SDLK_LEFT) {
				player->moveX(-1);
			}
			else if (currentDirection == SDLK_UP) {
				player->moveY(-1);
			}
			else if (currentDirection == SDLK_DOWN) {
				player->moveY(1);
			}
			
			break; // Only one box at a time
		}
	}
}

void GameLayer::draw() {
	calculateScroll();

	// Draw background without any scroll offset (static camera)
	background->draw(0);
	for (auto const& tile : tiles) {
		tile->draw(0); // Pass 0 for static camera
	}

	// Draw keys
	for (auto const& key : keys) {
		key->draw(0);
	}

	// Draw boxes
	for (auto const& box : boxes) {
		box->draw(0);
	}

	// Draw portal if it exists
	if (portal != NULL) {
		portal->draw(0);
	}

	for (auto const& projectile : projectiles) {
		projectile->draw(0); // Pass 0 for static camera
	}
	//cup->draw(0); // Pass 0 for static camera
	player->draw(0); // Pass 0 for static camera
	for (auto const& enemy : enemies) {
		enemy->draw(0); // Pass 0 for static camera
	}


	textBattery->draw(); // Draw battery level in top-right corner
	
	// Draw separated HUD elements
	textMovementsTitle->draw();
	textMovementsCounter->draw();
	textMovementsQueue->draw();
	textKeysCollected->draw(); // Draw keys collected counter

	// HUD
	if (game->input == game->inputMouse) {
		buttonJump->draw(0); // NO TIENEN SCROLL, POSICION FIJA
		buttonShoot->draw(0); // NO TIENEN SCROLL, POSICION FIJA
		pad->draw(0); // NO TIENEN SCROLL, POSICION FIJA
	}
	if (pause) {
		message->draw(0);
	}

	SDL_RenderPresent(game->renderer); // Renderiza
}

void GameLayer::gamePadToControls(SDL_Event event) {
	// Leer los botones
	bool buttonA = SDL_GameControllerGetButton(gamePad, SDL_CONTROLLER_BUTTON_A);
	bool buttonB = SDL_GameControllerGetButton(gamePad, SDL_CONTROLLER_BUTTON_B);
	// SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B
	// SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_Y
	cout << "botones:" << buttonA << "," << buttonB << endl;
	int stickX = SDL_GameControllerGetAxis(gamePad, SDL_CONTROLLER_AXIS_LEFTX);
	cout << "stickX" << stickX << endl;

	// Retorna aproximadamente entre [-32800, 32800], el centro deber?a estar en 0
	// Si el mando tiene "holgura" el centro varia [-4000 , 4000]
	if (stickX > 4000) {
		controlMoveX = 1;
	}
	else if (stickX < -4000) {
		controlMoveX = -1;
	}
	else {
		controlMoveX = 0;
	}

	if (buttonA) {
		controlShoot = true;
	}
	else {
		controlShoot = false;
	}

	if (buttonB) {
		controlMoveY = -1; // Move up
	}
	else {
		controlMoveY = 0;
	}
}

void GameLayer::mouseToControls(SDL_Event event) {
	// Modificaci?n de coordinadas por posible escalado
	float motionX = event.motion.x / game->scaleLower;
	float motionY = event.motion.y / game->scaleLower;
	// Cada vez que hacen click
	if (event.type == SDL_MOUSEBUTTONDOWN) {
		controlContinue = true;
		if (pad->containsPoint(motionX, motionY)) {
			pad->clicked = true;
			// CLICK TAMBIEN TE MUEVE
			controlMoveX = pad->getOrientationX(motionX);
		}
		if (buttonShoot->containsPoint(motionX, motionY)) {
			controlShoot = true;
		}
		if (buttonJump->containsPoint(motionX, motionY)) {
			controlMoveY = -1;
		}
	}
	// Cada vez que se mueve
	if (event.type == SDL_MOUSEMOTION) {
		if (pad->clicked && pad->containsPoint(motionX, motionY)) {
			controlMoveX = pad->getOrientationX(motionX);
			// Rango de -20 a 20 es igual que 0
			if (controlMoveX > -20 && controlMoveX < 20) {
				controlMoveX = 0;
			}
		}
		else {
			pad->clicked = false; // han sacado el rat?n del pad
			controlMoveX = 0;
		}
		if (buttonShoot->containsPoint(motionX, motionY) == false) {
			controlShoot = false;
		}
		if (buttonJump->containsPoint(motionX, motionY) == false) {
			controlMoveY = 0;
		}
	}
	// Cada vez que levantan el click
	if (event.type == SDL_MOUSEBUTTONUP) {
		if (pad->containsPoint(motionX, motionY)) {
			pad->clicked = false;
			// LEVANTAR EL CLICK TAMBIANA TE PARA
			controlMoveX = 0;
		}

		if (buttonShoot->containsPoint(motionX, motionY)) {
			controlShoot = false;
		}
		if (buttonJump->containsPoint(motionX, motionY)) {
			controlMoveY = 0;
		}
	}
}

void GameLayer::keysToControls(SDL_Event event) {
	if (event.type == SDL_KEYDOWN) {
		int code = event.key.keysym.sym;
		// Pulsada
		switch (code) {
		case SDLK_ESCAPE:
			game->loopActive = false;
			break;
		case SDLK_1:
			game->scale();
			break;
		case SDLK_BACKSPACE:
			// Remove last movement from queue (only if not executing)
			if (!executingQueue && !keyQueue.empty()) {
				keyQueue.pop_back();
			}
			break;
		case SDLK_RETURN: // Tecla ENTER
		case SDLK_KP_ENTER: // ENTER del teclado numérico
			// Execute queued movements when ENTER is pressed (only if not already executing)
			if (!executingQueue && !keyQueue.empty() && (int)keyQueue.size() <= maxQueuedMoves) {
				executingQueueVec = keyQueue;
				executingQueue = true;
				lastActionTimeMs = 0; // force immediate first action in update
				
				// clear input queue
				keyQueue.clear();
				textMovementsCounter->content = "Ejecutando...";
				textMovementsQueue->content = ""; // Clear center display during execution
			}
			break;
			// For movement/shooting in keyboard mode we enqueue keys
		case SDLK_RIGHT:
		case SDLK_LEFT:
		case SDLK_UP:
		case SDLK_DOWN:
		case SDLK_d:
			// Only record if under max and not currently executing
			if (!executingQueue && (int)keyQueue.size() < maxQueuedMoves) {
				keyQueue.push_back(code);
			}
			break;
		// Other keys ignored here
		}
	}
	// We don't need to handle KEYUP for the queued keyboard interaction
}