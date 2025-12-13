#include "Text.h"

Text::Text(string content, float x, float y, Game* game) {
	this->content = content;
	this->x = x;
	this->y = y;
	this->game = game;
}

void Text::draw() {
	if (game == nullptr) return;
	if (game->font == nullptr) {
		cout << "Text::draw: font is null, skipping text render for '" << content << "'" << endl;
		return;
	}

	// Don't draw empty or whitespace-only content
	if (content.empty()) {
		return;
	}
	
	// Check if content contains only whitespace
	bool hasNonWhitespace = false;
	for (char c : content) {
		if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
			hasNonWhitespace = true;
			break;
		}
	}
	if (!hasNonWhitespace) {
		return;
	}

	SDL_Color color;
	color.r = 255;
	color.g = 255;
	color.b = 255;
	color.a = 255; //transparente

	SDL_Surface* surface = TTF_RenderText_Blended(game->font, content.c_str(), color);
	if (surface == nullptr) {
		cout << "Text::draw: TTF_RenderText_Blended failed: " << TTF_GetError() << endl;
		return;
	}

	// Don't draw if surface has zero width or height
	if (surface->w <= 0 || surface->h <= 0) {
		SDL_FreeSurface(surface);
		return;
	}

	SDL_Texture* texture = SDL_CreateTextureFromSurface(game->renderer, surface);
	if (texture == nullptr) {
		cout << "Text::draw: SDL_CreateTextureFromSurface failed: " << SDL_GetError() << endl;
		SDL_FreeSurface(surface);
		return;
	}

	SDL_Rect rect; // Base de coordenadas esquina superior izquierda
	rect.x = x - surface->w / 2;
	rect.y = y - surface->h / 2;
	rect.w = surface->w;
	rect.h = surface->h;

	SDL_FreeSurface(surface);
	SDL_RenderCopy(game->renderer, texture, NULL, &rect);
	SDL_DestroyTexture(texture);
}
