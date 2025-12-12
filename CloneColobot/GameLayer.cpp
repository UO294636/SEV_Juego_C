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

	space = new Space(1);
	scrollX = 0;
	tiles.clear();

	audioBackground = Audio::createAudio("res/musica_ambiente.mp3", true);
	audioBackground->play();

	points = 0;
	textPoints = new Text("hola", WIDTH * 0.92, HEIGHT * 0.04, game);
	textPoints->content = to_string(points);

	// countdown text in HUD (top-left)
	textCountdown = new Text("30", WIDTH * 0.05, HEIGHT * 0.04, game);
	textCountdown->content = to_string(keyboardDurationSeconds);

	// queued actions HUD (below points)
	textQueue = new Text("", WIDTH * 0.92, HEIGHT * 0.08, game);
	textQueue->content = "-";

	
	background = new Background("res/fondo_2.png", WIDTH * 0.5, HEIGHT * 0.5, -1, game);
	backgroundPoints = new Actor("res/icono_puntos.png",
		WIDTH * 0.85, HEIGHT * 0.05, 24, 24, game);

	enemies.clear(); // Vaciar por si reiniciamos el juego
	projectiles.clear(); // Vaciar por si reiniciamos el juego

	loadMap("res/" + to_string(game->currentLevel) + ".txt");

	// reset keyboard batching state
	keyboardActive = false;
	keyboardStartTimeMs = 0;
	keyQueue.clear();
	executingQueue = false;
	executingQueueVec.clear();
	lastActionTimeMs = 0;
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
			mapWidth = line.length() * 40; // Ancho del mapa en pixels
			// Por car?cter (en cada l?nea)
			for (int j = 0; !streamLine.eof(); j++) {
				streamLine >> character; // Leer character 
				cout << character;
				float x = 40 / 2 + j * 40; // x central
				float y = 32 + i * 32; // y suelo
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
	case 'C': {
		cup = new Tile("res/copa.png", x, y, game);
		// modificaci?n para empezar a contar desde el suelo.
		cup->y = cup->y - cup->height / 2;
		space->addDynamicActor(cup); // Realmente no hace falta
		break;
	}
	case 'E': {
		Enemy* enemy = new Enemy(x, y, game);
		// modificaci?n para empezar a contar desde el suelo.
		enemy->y = enemy->y - enemy->height / 2;
		enemies.push_back(enemy);
		space->addDynamicActor(enemy);
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
		Tile* tile = new Tile("res/bloque_tierra.png", x, y, game);
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
				keyboardActive = false;
				keyboardStartTimeMs = 0;
				keyQueue.clear();
				textCountdown->content = to_string(keyboardDurationSeconds);
				textQueue->content = "-";
			}
			game->input = game->inputGamePad;
		}
		if (event.type == SDL_KEYDOWN) {
			// when any keydown detected, change input to keyboard
			game->input = game->inputKeyboard;
		}
		if (event.type == SDL_MOUSEBUTTONDOWN) {
			if (game->input == game->inputKeyboard) {
				keyboardActive = false;
				keyboardStartTimeMs = 0;
				keyQueue.clear();
				textCountdown->content = to_string(keyboardDurationSeconds);
				textQueue->content = "-";
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
	//procesar controles
	//procesar controles
	// Disparar
	if (controlContinue) {
		pause = false;
		controlContinue = false;
	}

	// If using keyboard batching mode, count down and execute queued keys when time expires
	if (game->input == game->inputKeyboard && !pause) {
		// update queued actions HUD
		{
			string q = "";
			for (int code : keyQueue) {
				if (code == SDLK_RIGHT) q += "R ";
				else if (code == SDLK_LEFT) q += "L ";
				else if (code == SDLK_UP) q += "U ";
				else if (code == SDLK_d) q += "D ";
			}
			if (q.empty()) q = "-";
			textQueue->content = q;
		}

		// start timer when first key is enqueued
		if (!keyQueue.empty() && !keyboardActive) {
			keyboardActive = true;
			keyboardStartTimeMs = SDL_GetTicks();
		}

		int remainingSeconds = 0;
		if (keyboardActive) {
			Uint32 elapsedMs = SDL_GetTicks() - keyboardStartTimeMs;
			int elapsedSec = elapsedMs / 1000;
			remainingSeconds = keyboardDurationSeconds - elapsedSec;
			if (remainingSeconds < 0) remainingSeconds = 0;
		}
		// update HUD text for countdown
		textCountdown->content = to_string(remainingSeconds);

		if ((keyboardActive && remainingSeconds <= 0) || (int)keyQueue.size() >= maxQueuedMoves) {
			// transfer queued keys to executing vector (one-by-one)
			executingQueueVec = keyQueue;
			executingQueue = true;
			lastActionTimeMs = 0; // force immediate first action in update

			// clear input queue and reset HUD
			keyQueue.clear();
			keyboardActive = false;
			keyboardStartTimeMs = 0;
			textCountdown->content = to_string(keyboardDurationSeconds);
			// textQueue will be updated to reflect executingQueueVec in update()
		}
	}
	else {
		// Not using keyboard: maintain existing behavior for other inputs
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

		// Eje Y
		if (controlMoveY > 0) {
			
		}
		else if (controlMoveY < 0) {
			player->jump();
		}
		else {

		}
	}



}

void GameLayer::update() {
	if (pause) {
		return;
	}

	// Executing queued actions one-by-one
	if (executingQueue && !executingQueueVec.empty()) {
		Uint32 now = SDL_GetTicks();
		if (lastActionTimeMs == 0 || now - lastActionTimeMs >= (Uint32)actionDelayMs) {
			int code = executingQueueVec.front();
			// perform action
			if (code == SDLK_RIGHT) {
				player->vx = 40;
				space->updateMoveRight(player);
				player->vx = 0;
			}
			else if (code == SDLK_LEFT) {
				player->vx = -40;
				space->updateMoveLeft(player);
				player->vx = 0;
			}
			else if (code == SDLK_UP) {
				player->jump();
			}
			else if (code == SDLK_d) {
				Projectile* newProjectile = player->shoot();
				if (newProjectile != NULL) {
					space->addDynamicActor(newProjectile);
					projectiles.push_back(newProjectile);
				}
			}

			// pop executed action
			executingQueueVec.erase(executingQueueVec.begin());
			lastActionTimeMs = now;
			// update HUD queue display
			{
				string q = "";
				for (int c : executingQueueVec) {
					if (c == SDLK_RIGHT) q += "R ";
					else if (c == SDLK_LEFT) q += "L ";
					else if (c == SDLK_UP) q += "U ";
					else if (c == SDLK_d) q += "D ";
				}
				if (q.empty()) q = "-";
				textQueue->content = q;
			}
		}
	}
	else if (executingQueue && executingQueueVec.empty()) {
		// finished
		executingQueue = false;
		lastActionTimeMs = 0;
		textQueue->content = "-";
	}

	// Nivel superado
	if (cup->isOverlap(player)) {
		game->currentLevel++;
		if (game->currentLevel > game->finalLevel) {
			game->currentLevel = 0;
		}
		message = new Actor("res/mensaje_ganar.png", WIDTH * 0.5, HEIGHT * 0.5,
			WIDTH, HEIGHT, game);
		pause = true;
		init();
	}

	// Jugador se cae
	if (player->y > HEIGHT + 80) {
		init();
	}

	space->update();
	background->update();
	player->update();
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
		if (projectile->isInRender(scrollX) == false || projectile->vx == 0) {

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
				points++;
				textPoints->content = to_string(points);


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
	// limite izquierda
	if (player->x > WIDTH * 0.3) {
		if (player->x - scrollX < WIDTH * 0.3) {
			scrollX = player->x - WIDTH * 0.3;
		}
	}

	// limite derecha
	if (player->x < mapWidth - WIDTH * 0.3) {
		if (player->x - scrollX > WIDTH * 0.7) {
			scrollX = player->x - WIDTH * 0.7;
		}
	}
}


void GameLayer::draw() {
	calculateScroll();

	background->draw();
	for (auto const& tile : tiles) {
		tile->draw(scrollX);
	}

	for (auto const& projectile : projectiles) {
		projectile->draw(scrollX);
	}
	cup->draw(scrollX);
	player->draw(scrollX);
	for (auto const& enemy : enemies) {
		enemy->draw(scrollX);
	}

	backgroundPoints->draw();
	textPoints->draw();
	// draw countdown and queued actions in HUD
	textCountdown->draw();
	textQueue->draw();

	// HUD
	if (game->input == game->inputMouse) {
		buttonJump->draw(); // NO TIENEN SCROLL, POSICION FIJA
		buttonShoot->draw(); // NO TIENEN SCROLL, POSICION FIJA
		pad->draw(); // NO TIENEN SCROLL, POSICION FIJA
	}
	if (pause) {
		message->draw();
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
		controlMoveY = -1; // Saltar
	}
	else {
		controlMoveY = 0;
	}
}


void GameLayer::mouseToControls(SDL_Event event) {
	// Modificaci?n de coordenadas por posible escalado
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
			// LEVANTAR EL CLICK TAMBIEN TE PARA
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
		// For movement/shooting in keyboard mode we enqueue keys during countdown
		case SDLK_RIGHT:
		case SDLK_LEFT:
		case SDLK_UP:
		case SDLK_d:
			// Only record if under max
			if ((int)keyQueue.size() < maxQueuedMoves) {
				keyQueue.push_back(code);
				// if this is the first key, start timer in processControls
			}
			break;
		// Other keys ignored here
		}


	}
	// We don't need to handle KEYUP for the queued keyboard interaction
}

