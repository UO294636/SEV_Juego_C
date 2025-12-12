#pragma once

#include <iostream>
#include <string>
#include <map> 
using namespace std;

#include <SDL.h>;
#include <SDL_mixer.h> // libreria de audio

class Audio
{

	Audio(string filename, bool loop);

public:

	static Audio* createAudio(string filename, bool loop) {
		// Mapa estático
		static map<string, Audio*> instancedAudios;

		if (instancedAudios.find(filename) == instancedAudios.end()) {
			cout << "Audio no cacheado lo creo " + filename << endl;;
			instancedAudios[filename] = new Audio(filename, loop);
		}

		return instancedAudios[filename];
	}
	~Audio(); //destructor
	void play();
	bool loop;
	Mix_Music* mix; // Lib mixer
	SDL_AudioSpec wavSpec; // Lib SDL Standard
	Uint32 wavLength;
	Uint8* wavBuffer;
	SDL_AudioDeviceID deviceId;
};
