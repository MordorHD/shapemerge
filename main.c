#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
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
	float ax, ay; // accumulated forces

	if(ev->id == EV_DRAW)
	{
		player_draw(player);
		return 0;
	}

	if(ev->id == EV_KEYUP)
	{
		//if(ev->vkCode == SDLK_UP && )
		return 0;	
	}

	if(ev->id != EV_TICK)
		return 0;

	dt = ev->ticks;

#define G 1780.0f // gravity
#define AX 1200.0f // x acceleration
#define CX 8.8f // turning on floor factor
#define FAX 0.004f // air friction (the closer the value to one, the higher the friction)
#define FX 0.1f // floor friction (...)
#define THRESX 40.0f // if no key is pressed and the player has a velocity less than this, the player stops
#define JY (-500.0f) // jumping force
	
	ax = 0;
	ay = 0;

	if(E_IS(player, ENTITY_GROUNDED))
	{
		if(keys[SDL_SCANCODE_LEFT])
			ax -= player->vel.x > 0 ? CX : 1.0f * AX * dt;
		if(keys[SDL_SCANCODE_RIGHT])
			ax += player->vel.x < 0 ? CX : 1.0f * AX * dt;
		if(keys[SDL_SCANCODE_SPACE])
			ay += JY;
		ax -= FX * player->vel.x;
		if(!keys[SDL_SCANCODE_LEFT] && !keys[SDL_SCANCODE_RIGHT] && fabs(player->vel.x) < THRESX)
			player->vel.x = 0.0;
	}
	else
	{
		if(keys[SDL_SCANCODE_LEFT])
			ax -= 0.1f * AX * dt;
		if(keys[SDL_SCANCODE_RIGHT])
			ax += 0.1f * AX * dt;
		ax -= FAX * player->vel.x;
		ay -= FAX * player->vel.y;
	}
	ay += G * dt;

	player->vel.x += ax;
	player->vel.y += ay;
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
	Uint32 custEventId;
	Uint32 start, end;
	bool running;
	SDL_Event event;
	ev_t ev;

	e_t *e;

	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	custEventId = SDL_RegisterEvents(1);
	
	static ec_t classes[] = {
		{ "Player", sizeof(e_t), player_proc },
		{ "Box", sizeof(e_t), box_proc },
	};
	for(int i = 0; i < sizeof(classes)/sizeof(*classes); i++)
		ec_add(classes + i);

	e = e_create("Player");
	e->vel = (Vec2) { 0.0, 0.0 };
	e->rect = (Rect2) { 30.0, 10.0, 20.0, 50.0 };
	e_add(e);

	e = e_create("Box");
	e->rect = (Rect2) { 0, 460, 10040.0f, 20 };
	e_add(e);

	keys = SDL_GetKeyboardState(NULL);

	start = SDL_GetTicks();
	running = 1;
	while(running)
	{
		while(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
			case SDL_KEYDOWN:
				ev.id = EV_KEYDOWN;
				ev.repeat = event.key.repeat;
				ev.vkCode = event.key.keysym.sym;
				e_dispatch(&ev);
				break;
			case SDL_KEYUP:
				ev.id = EV_KEYUP;
				ev.vkCode = event.key.keysym.sym;
				e_dispatch(&ev);
				break;
			case SDL_QUIT:
				running = 0;
				break;
			default:
				if(event.type == custEventId)
				{
					ev.id = event.user.code;
					ev.data1 = event.user.data1;
					ev.data2 = event.user.data2;
					e_dispatch(&ev);
				}
			}
		}
		end = SDL_GetTicks();
		ev.id = EV_TICK;
		ev.ticks = (end - start) / 1e3f;
		e_dispatch(&ev);
		start = end;
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		ev.id = EV_DRAW;
		e_dispatch(&ev);
		SDL_RenderPresent(renderer);
	}
}
