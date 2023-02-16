#ifndef INCLUDED_ENTITY
#define INCLUDED_ENTITY

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_rect.h>

#define v_t SDL_FPoint
#define r_t SDL_FRect

enum {
	EV_DRAW,
	EV_TICK,
	EV_KEYDOWN,
	EV_KEYUP,
	EV_MAX,
};
typedef struct event {
	int id;
	union {
		float ticks;
		struct {
			int vkCode;
			int repeat;
		};
		struct {
			void *data1;
			void *data2;
		};
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
		r_t bounds;
	};
	r_t *rects;
	int nRects;
	union {
		struct {
			float vx, vy;
		};
		v_t vel;
	};
} e_t;

e_t *e_create(const char *className);
void e_setrect(e_t *e, const r_t *r);
void e_addrect(e_t *e, const r_t *r);
void e_setvel(e_t *e, const v_t *v);
void e_translate(e_t *e, float x, float y);
void e_add(e_t *e);
void e_dispatch(ev_t *ev);
bool e_checkcollision(e_t *e, float ticks);

typedef struct entitylist {
	e_t **entities;
	int nEntities, szEntities;
	r_t bounds;
} el_t;

#define el_init(el, r, sz) ({ \
		el_t *_el = (el); \
		int _sz = (sz); \
		_el->entities = _sz ? malloc(_sz * sizeof*_el->entities) : 0; \
		_el->szEntities = _sz; \
		_el->nEntities = 0; \
		_el->bounds = (r); \
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
	r_t bounds;
} qt_t;

void qt_add(qt_t *qt, e_t *e);
void qt_remove(qt_t *qt, e_t *e);
bool qt_subdivide(qt_t *qt);
void qt_collect(const qt_t *qt, el_t *el);
e_t *qt_collectfirst(const qt_t *qt, const r_t *bounds);

#define CAM_LEFT 0
#define CAM_HCENTER (1 << 0)
#define CAM_RIGHT (1 << 1)
#define CAM_TOP 0
#define CAM_VCENTER (1 << 2)
#define CAM_BOTTOM (1 << 3)
typedef struct camera {
    // camera position
    float x;
    float y;
    // camera scaling
    float sx;
    float sy;
    // constraints
    float cstr_l;
    float cstr_t;
    float cstr_r;
    float cstr_b;
    // camera align
    int flags;
    struct {
        float x, y;
    } *target;
} cam_t;

#endif
