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
	for(int i = 0; i < player->nRects; i++)
		SDL_RenderDrawRectF(renderer, player->rects + i);
}

int player_proc(e_t *player, ev_t *ev)
{
#define G 1780.0f // gravity
#define AX 1200.0f // x acceleration
#define CX 8.8f // turning on floor factor
#define FAX 0.004f // air friction (the closer the value to one, the higher the friction)
#define FX 0.1f // floor friction (...)
#define THRESX 40.0f // if no key is pressed and the player has a velocity less than this, the player stops
#define JY (-500.0f) // jumping force
#define JUMP_CUT 0.45f	
	float dt;
	float ax, ay; // accumulated forces

	if(ev->id == EV_DRAW)
	{
		player_draw(player);
		return 0;
	}

	if(ev->id == EV_KEYUP)
	{
		if(ev->vkCode == SDLK_SPACE && player->vel.y < 0)
			player->vel.y *= JUMP_CUT;
		return 0;	
	}

	if(ev->id != EV_TICK)
		return 0;

	dt = ev->ticks;

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
	e_translate(player, player->vel.x * dt, player->vel.y * dt);
	return 0;
}

int box_proc(e_t *box, ev_t *ev)
{
	switch(ev->id)
	{
	case EV_DRAW:
		SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
		for(int i = 0; i < box->nRects; i++)
			SDL_RenderDrawRectF(renderer, box->rects + i);
		break;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	Uint32 custEventId;
	cam_t cam;
	e_t *e;
	Uint32 start, end;
	bool running;
	SDL_Event event;
	ev_t ev;
	int w, h;

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

	cam.x = 0.0f;
	cam.y = 0.0f;
	cam.cstr_l = 0;
	cam.cstr_t = 2e6;
	cam.cstr_r = 2e6;
	cam.cstr_b = 480;
	cam.sx = 1.0f;
	cam.sy = 1.0f;
	cam.flags = CAM_HCENTER | CAM_VCENTER;

	e = e_create("Player");
	e_setrect(e, &(r_t) { 30.0f, 10.0f, 20.0f, 50.0f });
	e_addrect(e, &(r_t) { 10.0f, 60.0f, 20.0f, 20.0f });
	e_addrect(e, &(r_t) { 50.0f, 60.0f, 20.0f, 20.0f });
	printf("%f, %f, %f, %f\n", e->x, e->y, e->w, e->h);
	e_add(e);

	cam.target = &e->bounds;

	e = e_create("Box");
	e_setrect(e, &(r_t) { 0.0f, 460.0f, 1e6f, 20.0f });
	for(int i = 0; i < 100; i++)
		e_addrect(e, &(r_t) { 100.0f + i * 100.0f, 440.0f, 19.0f, 20.0f });
	e_addrect(e, &(r_t) { 80.0f, 400.0f, 100.0f, 10.0f });
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
			case SDL_MOUSEBUTTONDOWN:
				e = e_create("Box");
				e_setrect(e, &(r_t) { event.button.x - 120, event.button.y - 20,
					   240, 40 });
				e_add(e);
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
		SDL_GetWindowSize(window, &w, &h);
		if(cam.target)
        {
			double tx, ty;
			int x_align, y_align;
			int l, t, r, b, drl, dbt;
			float tu;
			tx = cam.target->x * cam.sx;
			ty = cam.target->y * cam.sy;
			x_align = (cam.flags & CAM_HCENTER) ? w / 2 : (cam.flags & CAM_RIGHT) ? w : 0;
			y_align = (cam.flags & CAM_VCENTER) ? h / 2 : (cam.flags & CAM_BOTTOM) ? h : 0;
			l = cam.cstr_l * cam.sx;
			r = cam.cstr_r * cam.sx;
			t = cam.cstr_t * cam.sy;
			b = cam.cstr_b * cam.sy;
			drl = r + l;
			dbt = b + t;
			tx = drl <= w ? 0
				: -tx - x_align >= l - w ? -l
				: x_align + tx > r ? r - w
				: tx - x_align;
			ty = dbt <= h ? 0
				: -ty - y_align >= t - h ? -t
				: y_align + ty > b ? b - h
				: ty - y_align;
			tu = 2.0f * ev.ticks;
			tu = fmin(tu, 1.0f);
            cam.x += (tx - cam.x * cam.sx) * tu;
            cam.y += (ty - cam.y * cam.sy) * tu;
        }
		SDL_Rect vp;
		vp.x = -cam.x;
		vp.y = -cam.y;
		vp.w = w + cam.x;
		vp.h = h + cam.y;
		SDL_RenderSetViewport(renderer, &vp);
		SDL_RenderSetScale(renderer, cam.sx, cam.sy);
		start = end;
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		ev.id = EV_DRAW;
		e_dispatch(&ev);
		SDL_RenderPresent(renderer);
	}
}
