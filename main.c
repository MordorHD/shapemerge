#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>

#include "entity.h"

SDL_Window *window;
SDL_Renderer *renderer;
const Uint8 *keys; 

// define gravity in terms of jump height and distance.
// TODO: jump distance should be tweaked by how fast you are currently going.

// #define T 0.5
#define H 256.0
#define F 256.0
#define L 500.0
#define G (-2.0 * H * L * L / (F * F))
#define V (2.0 * H * L / F)

#define F2 (F * 0.65)

#define E_IS(e, f) ((e)->flags&(f))
#define E_SET(e, f) ((e)->flags|=(f))

void player_draw(e_t *player)
{
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	SDL_RenderDrawRectF(renderer, &player->rect);
}

int player_proc(e_t *player, ev_t *ev)
{
	float dt;

	if(ev->id == EV_DRAW)
	{
		player_draw(player);
		return 0;
	}

	if(ev->id != EV_TICK)
		return 0;

	dt = ev->ticks;

	if(keys[SDL_SCANCODE_LEFT]) {
		if(E_IS(player, ENTITY_GROUNDED)) {
			player->vel.x = -L * 0.75;
		} else {
			player->vel.x = -L * 0.8;
		}
	}
	if(keys[SDL_SCANCODE_RIGHT]) {
		if(E_IS(player, ENTITY_GROUNDED)) {
			player->vel.x = L * 0.75;
		} else {
			player->vel.x = L * 0.8;
		}
	}
	if(keys[SDL_SCANCODE_SPACE] && E_IS(player, ENTITY_GROUNDED))
		player->vel.y -= V;
	if(player->vel.y > 0 || !keys[SDL_SCANCODE_SPACE])
		player->vel.y -= G * dt;
	else
		player->vel.y -= G * dt;

	e_checkcollision(player, dt);
	player->rect.x += player->vel.x * dt;
	player->rect.y += player->vel.y * dt;
	return 0;
}

int box_proc(e_t *box, ev_t *ev)
{
	switch(ev->id)
	{
	case EV_DRAW:
		SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
		SDL_RenderDrawRectF(renderer, &box->rect);
		break;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	static ec_t classes[] = {
		{ "Player", sizeof(e_t), player_proc },
		{ "Box", sizeof(e_t), box_proc },
	};
	for(int i = 0; i < sizeof(classes)/sizeof(*classes); i++)
		ec_add(classes + i);

	e_init();

	e_t *e;

	e = e_create("Player");
	e->vel = (Vec2) { 0.0, 0.0 };
	e->rect = (Rect2) { 30.0, 10.0, 20.0, 50.0 };
	e_add(e);

	e = e_create("Box");
	e->rect = (Rect2) { 0, 460, 640, 20 };
	e_add(e);

	keys = SDL_GetKeyboardState(NULL);

	int start;
	int end;

	int running = 1;
	while (running)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_QUIT:
				running = 0;
				break;
			}
		}
		end = SDL_GetTicks();
		e_tickall((end - start) / 1000.0f);
		start = end;
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		e_drawall(renderer);
		SDL_RenderPresent(renderer);
	}
}
