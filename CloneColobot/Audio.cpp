#include "Audio.h"

Audio::Audio(string filename, bool loop) {
	this->loop = loop;

	if (loop || filename.find(".mp3") != string::npos) {
		// Uso la Librer?a Mixer - mp3 o loops
		Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096); // 2 canales
		mix = Mix_LoadMUS(filename.c_str());
	}
	else {
		// Uso SDL audio standard para archivos WAV
		SDL_LoadWAV(filename.c_str(), &wavSpec, &wavBuffer, &wavLength);
		deviceId = SDL_OpenAudioDevice(NULL, 0, &wavSpec, NULL, 0);
	}
}

Audio::~Audio() {
	if (loop) {
		Mix_FreeMusic(mix);
		Mix_CloseAudio();
	}
	else {
		SDL_CloseAudioDevice(deviceId);
		SDL_FreeWAV(wavBuffer);
	}
}

void Audio::play() {
	if (loop) {
		Mix_PlayMusic(mix, -1);
		// -1 se repite sin parar
	}
	else {
		// Reproducir música MP3 una sola vez (0 = no repetir)
		if (mix != NULL) {
			Mix_PlayMusic(mix, 0);
		}
		else {
			// Reproducir archivo WAV una sola vez con SDL audio standard
			if (SDL_GetQueuedAudioSize(deviceId) > wavLength * 4) {
				SDL_ClearQueuedAudio(deviceId);
			}
			SDL_QueueAudio(deviceId, wavBuffer, wavLength);
			SDL_PauseAudioDevice(deviceId, 0);
		}
	}
}

void Audio::setVolume(int volume) {
	// volume: 0-128 (128 es volumen máximo)
	if (loop || mix != NULL) {
		Mix_VolumeMusic(volume);
	}
}

bool Audio::isPlaying() {
	if (loop || mix != NULL) {
		// Para música cargada con Mix_LoadMUS
		return Mix_PlayingMusic() == 1;
	}
	else {
		// Para archivos WAV con SDL audio standard
		return SDL_GetAudioDeviceStatus(deviceId) == SDL_AUDIO_PLAYING;
	}
}
