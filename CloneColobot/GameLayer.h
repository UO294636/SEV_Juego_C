#pragma once

#include "Layer.h"
#include "Player.h"
#include "Background.h"

#include "Enemy.h"
#include "Projectile.h"
#include "Text.h"
#include "Tile.h"
#include "Pad.h"
#include "Portal.h"
#include "Key.h"

#include "Audio.h"
#include "Space.h" // importar

#include <fstream> // Leer ficheros
#include <sstream> // Leer líneas / String
#include <list>
#include <vector>

class GameLayer : public Layer
{
public:
	GameLayer(Game* game);
	void init() override;
	void processControls() override;
	void update() override;
	void draw() override;
	void keysToControls(SDL_Event event);
	void mouseToControls(SDL_Event event); // USO DE MOUSE
	void gamePadToControls(SDL_Event event); // USO DE GAMEPAD
	void loadMap(string name);
	void loadMapObject(char character, float x, float y);
	void calculateScroll(); // Now sets scrollX to 0 for static camera
	Actor* message;
	bool pause;
	// Elementos de interfaz
	SDL_GameController* gamePad;
	Pad* pad;
	Actor* buttonJump;
	Actor* buttonShoot;

	Tile* cup; // Elemento de final de nivel
	Tile* door; // La puerta que se abre al recoger todas las llaves
	Portal* portal; // Portal de salida del nivel
	list<Key*> keys; // Lista de llaves en el mapa
	int totalKeys = 0; // Total de llaves en el nivel
	int keysCollected = 0; // Llaves recogidas por el jugador
	bool doorOpen = false; // Estado de la puerta
	
	Space* space;
	float scrollX;
	int mapWidth;
	list<Tile*> tiles;

	Audio* audioBackground;
	Text* textMovementsTitle; // Title "Movimientos:" in top-left
	Text* textMovementsCounter; // Counter "(X/10) [ENTER para ejecutar]" below title
	Text* textMovementsQueue; // Movement icons in center of screen
	Text* textKeysCollected; // Display keys collected
	Text* textBattery; // Display battery level
	int battery = 10; // Battery starts at 10
	int newEnemyTime = 0;
	Player* player;
	Background* background;
	Actor* backgroundBattery;
	list<Enemy*> enemies;
	list<Projectile*> projectiles;

	bool controlContinue = false;
	bool controlShoot = false;
	int controlMoveY = 0;
	int controlMoveX = 0;

	// Keyboard batching: countdown and queued keys (seconds)
	int keyboardDurationSeconds = 30; // seconds allowed to input
	Uint32 keyboardStartTimeMs = 0; // SDL_GetTicks() at start
	bool keyboardActive = false; // true when countdown running
	std::vector<int> keyQueue; // store key actions in arrival order
	int maxQueuedMoves = 10;

	// Executing queued actions one-by-one
	bool executingQueue = false;
	std::vector<int> executingQueueVec;
	Uint32 lastActionTimeMs = 0;
	int actionDelayMs = 300; // ms between actions

};

