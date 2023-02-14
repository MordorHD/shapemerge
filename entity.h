#ifndef INCLUDED_ENTITY
#define INCLUDED_ENTITY

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_rect.h>

#define Vec2 SDL_FPoint
#define Rect2 SDL_FRect

enum {
	EV_DRAW,
	EV_TICK,
};
typedef struct event {
	int id;
	union {
		float ticks;
	};
} ev_t;

struct entity;
typedef int (*ep_t)(struct entity *e, ev_t *ev);

typedef struct entityclass {
	char name[32];
	size_t size;
	ep_t proc;
} ec_t;

int ec_add(const ec_t *class);

#define ENTITY_GROUNDED (1<<1)
#define ENTITY_WALLED (1<<2)
#define ENTITY_CEILED (1<<4)
typedef struct entity {
	ep_t proc;
	int flags;
	union {
		struct {
			float x, y;
			float w, h;
		};
		Rect2 rect;
	};
	union {
		struct {
			float vx, vy;
		};
		Vec2 vel;
	};
} e_t;

void e_init(void);

e_t *e_create(const char *className);
void e_add(e_t *e);

bool e_checkcollision(e_t *e, float ticks);

void e_tickall(float tick);
void e_drawall(SDL_Renderer *r);

typedef struct entitylist {
	e_t **entities;
	int nEntities, szEntities;
	Rect2 rect;
} el_t;

#define el_init(el, r, sz) ({ \
		el_t *_el = (el); \
		int _sz = (sz); \
		_el->entities = _sz ? malloc(_sz * sizeof*_el->entities) : 0; \
		_el->szEntities = _sz; \
		_el->nEntities = 0; \
		_el->rect = (r); \
		1; })
#define el_discard(el) ({ free((el)->entities); 1; })
void el_add(el_t *el, e_t *e);

#define ENTITY_LOWERTHRESHOLD 7
#define ENTITY_UPPERTHRESHOLD 15
#define ENTITY_AREA_THRESHOLD 40
typedef struct quadtree {
	union { // if the first node is null (isParent==0),
			// this node is valid for the 2nd struct
		struct quadtree *nodes[4];
		struct {
			void *isParent;
			e_t **entities;
			int nEntities, szEntities;
		};
	};
	Rect2 rect;
} qt_t;

int qt_add(qt_t *qt, e_t *e);
int qt_remove(qt_t *qt, e_t *e);
int qt_subdivide(qt_t *qt);
int qt_collect(const qt_t *qt, el_t *el);
e_t *qt_collectfirst(const qt_t *qt, const Rect2 *rect);

#endif
