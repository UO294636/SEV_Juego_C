#include "GameLayer.h"

GameLayer::GameLayer(Game* game)
	: Layer(game) {
	//llama al constructor del padre : Layer(renderer)
	pause = true;
	// Select tutorial message based on current level
	string tutorialImage = "res/mensaje_como_jugar_" + to_string(game->currentLevel) + ".png";
	message = new Actor(tutorialImage, WIDTH * 0.5, HEIGHT * 0.5,
		WIDTH, HEIGHT, game);

	gamePad = SDL_GameControllerOpen(0);
	audioPortal = NULL;
	audioDoor = NULL;
	init();
}


void GameLayer::init() {
	pad = new Pad(WIDTH * 0.15, HEIGHT * 0.80, game);

	space = new Space(0); // Disable gravity for 4-directional movement
	scrollX = 0;
	tiles.clear();

	audioBackground = Audio::createAudio("res/musica_ambiente.mp3", true);
	audioBackground->play();
	audioBackground->setVolume(60); // Set initial volume to 60 after playing

	// Separate HUD elements for better organization - positioned to fit in screen
	textMovementsTitle = new Text("", WIDTH * 0.15, HEIGHT * 0.04, game);
	textMovementsTitle->content = "Movimientos:";
	
	textMovementsCounter = new Text("", WIDTH * 0.25, HEIGHT * 0.08, game);
	textMovementsCounter->content = "-";
	
	// Clear movement sprites list
	clearMovementSprites();

	// Initialize key collection HUD
	textKeysCollected = new Text("", WIDTH * 0.5, HEIGHT * 0.04, game);
	textKeysCollected->content = "Llaves: 0/0";

	// Initialize battery HUD in top-right corner (where points used to be)
	textBattery = new Text("", WIDTH * 0.92, HEIGHT * 0.04, game);
	textBattery->content = "Bat: 10";

	background = new Background("res/fondo.png", WIDTH * 0.5, HEIGHT * 0.5, -1, game);


	keys.clear(); // Vaciar lista de llaves
	boxes.clear(); // Vaciar lista de cajas
	batteries.clear(); // Vaciar lista de baterías
	
	// Reset key and door state
	totalKeys = 0;
	keysCollected = 0;
	doorOpen = false;
	door = NULL;
	portal = NULL; // Reset portal
	
	// Reset battery
	battery = 10;
	
	// Activar modo invisible solo si es el nivel final
	invisibleWallsMode = (game->currentLevel == game->finalLevel);

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
	
	// Reset death sound flag for next level/life
	deathSoundPlayed = false;

	// Reset portal sound flag for next level
	portalSoundPlayed = false;
	
	// Reset door sound flag for next level
	doorPassSoundPlayed = false;
	
	// Update tutorial message for current level
	string tutorialImage = "res/como_jugar_" + to_string(game->currentLevel) + ".png";
	message = new Actor(tutorialImage, WIDTH * 0.5, HEIGHT * 0.5,
		WIDTH, HEIGHT, game);
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
	case 'X': {
		Battery* battery = new Battery(x, y, game);
		// modificación para empezar a contar desde el suelo.
		battery->y = battery->y - battery->height / 1.5;
		batteries.push_back(battery);
		space->addDynamicActor(battery); // Las baterías son dinámicas para poder recogerlas
		break;
	}
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

		// Cambio automático de input
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
					clearMovementSprites();
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
					clearMovementSprites();
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

	// If using keyboard or gamepad batching mode, wait for Enter key/Button A to execute queued moves
	// Block input if currently executing queue
	if ((game->input == game->inputKeyboard || game->input == game->inputGamePad) && !pause && !executingQueue) {
		// Update counter text with concise format to fit on screen
		if (keyQueue.empty()) {
			textMovementsCounter->content = "-";
		}
		else {
			// Show different message based on input type
			if (game->input == game->inputKeyboard) {
				textMovementsCounter->content = "(" + to_string(keyQueue.size()) + "/" + to_string(maxQueuedMoves) + ") ENTER";
			}
			else {
				// GamePad input
				textMovementsCounter->content = "(" + to_string(keyQueue.size()) + "/" + to_string(maxQueuedMoves) + ") Btn A";
			}
		}
		
		// Update movement sprites instead of text
		updateMovementSprites();
	}
	else if (!executingQueue) {
		// Not using keyboard/gamepad and not executing: maintain existing behavior for mouse input

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
		// Play death sound only once when dying starts
		if (!deathSoundPlayed) {
			Audio* deathSound = Audio::createAudio("res/derrota.wav", false);
			deathSound->play();
			deathSoundPlayed = true;
		}
		
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

	// Check for level completion FIRST - player reaches portal
	// This needs to be checked before queue execution so it works during movement
	if (portal != NULL && player->isOverlap(portal)) {
		// Play portal sound (but don't wait for it)
		if (!portalSoundPlayed) {
			audioPortal = Audio::createAudio("res/portal.wav", false);
			audioPortal->play();
			portalSoundPlayed = true;
		}
		
		// Immediately change level without waiting for audio
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
	// Reset portal audio if player leaves
	else {
		audioPortal = NULL;
		portalSoundPlayed = false;
	}

	// Check if player passes through open door
	if (doorOpen && door != NULL && player->isOverlap(door)) {
		// Play door sound when player passes through open door
		if (!doorPassSoundPlayed) {
			audioDoor = Audio::createAudio("res/open_door.wav", false);
			audioDoor->play();
			doorPassSoundPlayed = true;
		}
	}
	else {
		// Reset door audio if player leaves door area
		audioDoor = NULL;
		doorPassSoundPlayed = false;
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
				
				// Check if battery reached 0 - player should die
				if (battery <= 0) {
					// Stop executing queue
					executingQueue = false;
					executingQueueVec.clear();
					lastActionTimeMs = 0;
					textMovementsCounter->content = "-";
					clearMovementSprites();
					player->moveX(0);
					player->moveY(0);
					
					// Set player to dying state
					player->state = game->stateDying;
					return; // Exit update to start death animation
				}
				
				// Update HUD displays during execution
				if (executingQueueVec.empty()) {
					textMovementsCounter->content = "Completo!";
					clearMovementSprites();
				}
				else {
					textMovementsCounter->content = "(" + to_string(executingQueueVec.size()) + " quedan)";
					
					// Update remaining movements display with sprites
					updateMovementSprites();
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
		clearMovementSprites();
		player->moveX(0); // Stop player when queue is empty (updates both vx and inputVx)
		player->moveY(0); // Stop player when queue is empty (updates both vy and inputVy)
		
		// Check if player reached the portal during execution
		// Portal collision is now handled above, so we just check if NOT at portal = death
		if (portal == NULL || !player->isOverlap(portal)) {
			// Didn't reach portal, set player state to dying
			player->state = game->stateDying;
			return; // Return here to start death animation in next frame
		}
		// If at portal, the check above will handle the level transition
	}

	// Check for key collection
	list<Key*> keysToRemove;
	for (auto const& key : keys) {
		if (player->isOverlap(key)) {
			keysToRemove.push_back(key);
			keysCollected++;
			textKeysCollected->content = "Llaves: " + to_string(keysCollected) + "/" + to_string(totalKeys);
			
			// Play sound when key is collected
			Audio* pickKeySound = Audio::createAudio("res/pick_key.wav", false);
			pickKeySound->play();
			
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

	// Check for battery collection
	list<Battery*> batteriesToRemove;
	for (auto const& batteryItem : batteries) {
		if (player->isOverlap(batteryItem)) {
			batteriesToRemove.push_back(batteryItem);
			// Add 5 to current battery
			if (battery + 5 > 10) {
				battery = 10;
			}
			else {
				battery += 5;
			}
			
			textBattery->content = "Bat: " + to_string(battery);
			
			// Play sound when battery is collected
			Audio* pickBatterySound = Audio::createAudio("res/pick_battery.wav", false);
			pickBatterySound->play();
		}
	}
	
	// Remove collected batteries
	for (auto const& batteryToRemove : batteriesToRemove) {
		batteries.remove(batteryToRemove);
		space->removeDynamicActor(batteryToRemove);
		delete batteryToRemove;
	}
	batteriesToRemove.clear();


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

void GameLayer::updateTileVisibility() {
	if (!invisibleWallsMode) {
		// Si el modo está desactivado, todos los tiles son visibles
		for (auto const& tile : tiles) {
			tile->visible = true;
		}
		return;
	}

	// Primero hacer todos los tiles invisibles (excepto la puerta)
	for (auto const& tile : tiles) {
		if (tile == door) {
			tile->visible = true; // La puerta siempre visible
		} else {
			tile->visible = false;
		}
	}

	// Hacer visibles solo los tiles que colisionan con el jugador
	for (auto const& tile : tiles) {
		if (tile == door) continue; // Ya procesada arriba
		
		// Verificar si el jugador está colisionando con este tile
		if (player->isOverlap(tile)) {
			tile->visible = true;
		}
	}
}

void GameLayer::draw() {
	calculateScroll();

	// Actualizar visibilidad de los tiles antes de dibujar
	updateTileVisibility();

	// Draw background without any scroll offset (static camera)
	background->draw(0);
	for (auto const& tile : tiles) {
		tile->draw(0); // Pass 0 for static camera
	}

	// Draw keys
	for (auto const& key : keys) {
		key->draw(0);
	}

	// Draw batteries
	for (auto const& batteryItem : batteries) {
		batteryItem->draw(0);
	}

	// Draw boxes
	for (auto const& box : boxes) {
		box->draw(0);
	}

	// Draw portal if it exists
	if (portal != NULL) {
		portal->draw(0);
	}

	// Draw player
	player->draw(0);

	textBattery->draw(); // Draw battery level in top-right corner
	
	// Draw separated HUD elements
	textMovementsTitle->draw();
	textMovementsCounter->draw();
	
	// Draw movement sprites instead of text
	for (auto const& sprite : movementSprites) {
		sprite->draw(0);
	}
	
	textKeysCollected->draw(); // Draw keys collected counter

	// HUD
	if (pause) {
		message->draw(0);
	}

	SDL_RenderPresent(game->renderer); // Renderiza
}

void GameLayer::gamePadToControls(SDL_Event event) {
	// Solo procesar eventos de botones presionados
	if (event.type != SDL_CONTROLLERBUTTONDOWN) {
		return;
	}

	// Obtener qué botón fue presionado
	SDL_GameControllerButton button = (SDL_GameControllerButton)event.cbutton.button;
	
	cout << "GamePad button pressed: " << button << endl;

	// Si está en pausa, cualquier botón principal (A, B, X, Y, START) sirve para continuar
	if (pause) {
		if (button == SDL_CONTROLLER_BUTTON_A || 
			button == SDL_CONTROLLER_BUTTON_B ||
			button == SDL_CONTROLLER_BUTTON_X ||
			button == SDL_CONTROLLER_BUTTON_Y ||
			button == SDL_CONTROLLER_BUTTON_START) {
			controlContinue = true;
		}
		return;
	}

	// Botón A (X en PlayStation / A en Xbox) - Ejecutar cola de movimientos
	if (button == SDL_CONTROLLER_BUTTON_A) {
		// Execute queued movements when Button A is pressed (only if not already executing)
		if (!executingQueue && !keyQueue.empty() && (int)keyQueue.size() <= maxQueuedMoves) {
			executingQueueVec = keyQueue;
			executingQueue = true;
			lastActionTimeMs = 0; // force immediate first action in update
			
			// clear input queue
			keyQueue.clear();
			textMovementsCounter->content = "Ejecutando...";
			clearMovementSprites(); // Clear sprites during execution
		}
		return;
	}

	// Botón B - Borrar último movimiento de la cola
	if (button == SDL_CONTROLLER_BUTTON_B) {
		// Remove last movement from queue (only if not executing)
		if (!executingQueue && !keyQueue.empty()) {
			keyQueue.pop_back();
		}
		return;
	}

	// D-pad (Cruceta) - Registrar movimientos en la cola
	// Solo registrar si no estamos ejecutando y no hemos alcanzado el máximo
	if (!executingQueue && (int)keyQueue.size() < maxQueuedMoves) {
		if (button == SDL_CONTROLLER_BUTTON_DPAD_UP) {
			keyQueue.push_back(SDLK_UP);
			cout << "D-pad UP pressed - added to queue" << endl;
		}
		else if (button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
			keyQueue.push_back(SDLK_DOWN);
			cout << "D-pad DOWN pressed - added to queue" << endl;
		}
		else if (button == SDL_CONTROLLER_BUTTON_DPAD_LEFT) {
			keyQueue.push_back(SDLK_LEFT);
			cout << "D-pad LEFT pressed - added to queue" << endl;
		}
		else if (button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) {
			keyQueue.push_back(SDLK_RIGHT);
			cout << "D-pad RIGHT pressed - added to queue" << endl;
		}
	}
}

void GameLayer::mouseToControls(SDL_Event event) {
	// Modificación de coordinadas por posible escalado
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
			pad->clicked = false; // han sacado el ratón del pad
			controlMoveX = 0;
		}
	}
	// Cada vez que levantan el click
	if (event.type == SDL_MOUSEBUTTONUP) {
		if (pad->containsPoint(motionX, motionY)) {
			pad->clicked = false;
			// LEVANTAR EL CLICK TAMBIEN TE PARA
			controlMoveX = 0;
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
				clearMovementSprites(); // Clear sprites during execution
			}
			break;
			// For movement/shooting in keyboard mode we enqueue keys
		case SDLK_RIGHT:
		case SDLK_d: // D para derecha
			// Only record if under max and not currently executing
			if (!executingQueue && (int)keyQueue.size() < maxQueuedMoves) {
				keyQueue.push_back(SDLK_RIGHT); // Normalizar a SDLK_RIGHT
			}
			break;
		case SDLK_LEFT:
		case SDLK_a: // A para izquierda
			// Only record if under max and not currently executing
			if (!executingQueue && (int)keyQueue.size() < maxQueuedMoves) {
				keyQueue.push_back(SDLK_LEFT); // Normalizar a SDLK_LEFT
			}
			break;
		case SDLK_UP:
		case SDLK_w: // W para arriba
			// Only record if under max and not currently executing
			if (!executingQueue && (int)keyQueue.size() < maxQueuedMoves) {
				keyQueue.push_back(SDLK_UP); // Normalizar a SDLK_UP
			}
			break;
		case SDLK_DOWN:
		case SDLK_s: // S para abajo
			// Only record if under max and not currently executing
			if (!executingQueue && (int)keyQueue.size() < maxQueuedMoves) {
				keyQueue.push_back(SDLK_DOWN); // Normalizar a SDLK_DOWN
			}
			break;
		// Other keys ignored here
		}
	}
	// We don't need to handle KEYUP for the queued keyboard interaction
}

void GameLayer::clearMovementSprites() {
	// Delete all movement sprites
	for (auto sprite : movementSprites) {
		delete sprite;
	}
	movementSprites.clear();
}

void GameLayer::updateMovementSprites() {
	// Clear existing sprites
	clearMovementSprites();
	
	// Determine which queue to visualize
	std::vector<int>* queueToShow = nullptr;
	if (executingQueue && !executingQueueVec.empty()) {
		queueToShow = &executingQueueVec;
	}
	else if (!keyQueue.empty()) {
		queueToShow = &keyQueue;
	}
	
	if (queueToShow == nullptr || queueToShow->empty()) {
		return;
	}
	
	// Constants for sprite layout (40x40 sprites)
	const int spriteSize = 40;
	const int spacing = 5; // Space between sprites
	const int spritesPerRow = 10; // Max sprites per row
	
	// Calculate starting position for centered display
	int totalSprites = queueToShow->size();
	int firstRowCount = (totalSprites > spritesPerRow) ? spritesPerRow : totalSprites;
	int secondRowCount = (totalSprites > spritesPerRow) ? (totalSprites - spritesPerRow) : 0;
	
	// Starting X for first row (center it)
	float firstRowStartX = WIDTH * 0.5 - (firstRowCount * (spriteSize + spacing) - spacing) / 2.0f;
	
	// Starting X for second row (center it)
	float secondRowStartX = 0;
	if (secondRowCount > 0) {
		secondRowStartX = WIDTH * 0.5 - (secondRowCount * (spriteSize + spacing) - spacing) / 2.0f;
	}
	
	// Y positions for rows
	float firstRowY = HEIGHT * 0.35;
	float secondRowY = HEIGHT * 0.35 + spriteSize + spacing;
	
	// Create sprites for each movement
	for (int i = 0; i < totalSprites; i++) {
		int code = (*queueToShow)[i];
		string spriteFile = "";
		
		// Select sprite based on key code
		if (code == SDLK_RIGHT) {
			spriteFile = "res/tecla_derecha.png";
		}
		else if (code == SDLK_LEFT) {
			spriteFile = "res/tecla_izquierda.png";
		}
		else if (code == SDLK_UP) {
			spriteFile = "res/tecla_arriba.png";
		}
		else if (code == SDLK_DOWN) {
			spriteFile = "res/tecla_abajo.png";
		}
		else if (code == SDLK_d) {
			spriteFile = "res/tecla_disparo.png";
		}
		
		if (spriteFile != "") {
			float posX, posY;
			
			// Determine position based on row
			if (i < spritesPerRow) {
				// First row
				posX = firstRowStartX + i * (spriteSize + spacing) + spriteSize / 2;
				posY = firstRowY;
			}
			else {
				// Second row
				int secondRowIndex = i - spritesPerRow;
				posX = secondRowStartX + secondRowIndex * (spriteSize + spacing) + spriteSize / 2;
				posY = secondRowY;
			}
			
			Actor* sprite = new Actor(spriteFile, posX, posY, spriteSize, spriteSize, game);
			movementSprites.push_back(sprite);
		}
	}
}