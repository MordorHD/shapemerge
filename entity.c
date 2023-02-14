#include "entity.h"
#include <math.h>

struct {
	ec_t *d;
	int n, sz;
} Classes;

int ec_add(const ec_t *class)
{
	ec_t *d;
	int n;

	for(d = Classes.d, n = Classes.n; n; n--, d++)
		if(!strcmp(d->name, class->name))
			return -1;

	if(Classes.n == Classes.sz)
	{
		Classes.sz *= 2;
		Classes.sz++;
		Classes.d = realloc(Classes.d, Classes.sz * sizeof*Classes.d);
	}
	d = Classes.d + Classes.n;
	memcpy(d, class, sizeof*d);
	Classes.n++;

	return 0;
}

e_t *e_create(const char *className)
{
	ec_t *ec;
	int n;
	e_t *e;

	for(ec = Classes.d, n = Classes.n; n; n--, ec++)
		if(!strcmp(ec->name, className))
			goto found;
	return NULL;
found:
	e = malloc(sizeof*e);
	memset(e, 0, sizeof*e);
	e->proc = ec->proc;
	return e;
}

struct {
	e_t **d;
	int n, sz;
	qt_t qt;
} Entities;

void e_init(void)
{
	Entities.qt.rect = (Rect2) { 0, 0, 640, 480 };
}

void e_add(e_t *e)
{
	if(Entities.n == Entities.sz)
	{
		Entities.sz *= 2;
		Entities.sz++;
		Entities.d = realloc(Entities.d, Entities.sz*sizeof*Entities.d);
	}
	Entities.d[Entities.n] = e;
	Entities.n++;
	qt_add(&Entities.qt, e);
}

void dispatch(ev_t *ev)
{
	e_t **es, *e;
	int n;

	for(es = Entities.d, n = Entities.n; n; n--, es++)
	{
		e = *es;
		e->proc(e, ev);
	}
}

void e_tickall(float ticks)
{
	ev_t ev;

	ev.id = EV_TICK;
	ev.ticks = ticks;
	dispatch(&ev);
}

void e_drawall(SDL_Renderer *r)
{
	ev_t ev;

	ev.id = EV_DRAW;
	dispatch(&ev);
}

#define FLOATEQUAL(a, b) (fabs((a) - (b)) < 0.0001)

// true if e moved
bool e_checkcollision(e_t *e, float ticks)
{
	// Step 1: Get velocity box
	Rect2 velBox;
	float vx, vy;
	// Step 2: Collect all potential colliding entities
	el_t el;
	// Step 3: Figure out collision between them and the e
	e_t **es;
	int n;
	Rect2 *pr;
	Rect2 xr, yr;
	int ox, oy;
	float entryX, entryY;
	float exitX, exitY;
	int xCnt = 0, yCnt = 0;
	// Step 1:
	vx = e->vx * ticks;
	vy = e->vy * ticks;
	if(vx > 0.0f)
	{
		velBox.x = e->x;
		velBox.w = e->w + vx;
	}
	else
	{
		velBox.x = e->x + vx;
		velBox.w = e->w;
	}
	if(vy > 0.0f)
	{
		velBox.y = e->y;
		velBox.h = e->h + vy;
	}
	else
	{
		velBox.y = e->y + vy;
		velBox.h = e->h;
	}

	// Step 2:
	el_init(&el, velBox, 7);
	qt_collect(&Entities.qt, &el);
	if(!el.nEntities)
	{
		el_discard(&el);
		return 0;
	}
	// Step 3:
	for(es = el.entities, n = el.nEntities; n; n--, es++)
	{
		if(*es == e)
			continue;
		pr = &((*es)->rect);
		if(vx == 0.0f)
		{
			entryX = -1.0f/0.0f;
			exitX = 1.0f/0.0f;
		}
		else
		{
			if(vx > 0.0f)
			{
				entryX = pr->x - (e->x + e->w);
				exitX = pr->x + pr->w - e->x;
			}
			else
			{
				entryX = pr->x + pr->w - e->x;
				exitX = pr->x - (e->x + e->w);
			}
			entryX /= vx;
			exitX /= vx;
		}
		if(vy == 0.0f)
		{
			entryY = -1.0f/0.0f;
			exitY = 1.0f/0.0f;
		}
		else
		{
			if(vy > 0.0f)
			{
				entryY = pr->y - (e->y + e->h);
				exitY = pr->y + pr->h - e->y;
			}
			else
			{
				entryY = pr->y + pr->h - e->y;
				exitY = pr->y - (e->y + e->h);
			}
			entryY /= vy;
			exitY /= vy;
		}
		if(fmax(entryX, entryY) > fmin(exitX, exitY) ||
		   (entryX < 0.0f && entryY < 0.0f)		  ||
			entryX > 1.0f || entryY > 1.0f)
			continue;

		if(entryX > entryY)
		{
			ox = vx < 0 ? xr.x + xr.w - (pr->x + pr->w) : pr->x - xr.x;
			if(!xCnt || ox <= 0)
			{
				xr = *pr;
				xCnt = 1;
			}
		}
		else
		{
			oy = vy < 0 ? yr.y + yr.h - (pr->y + pr->h) : pr->y - yr.y;
			if(!yCnt || oy <= 0)
			{
				yr = *pr;
				yCnt = 1;
			}
		}
	}
	e->flags &= ~(ENTITY_CEILED | ENTITY_GROUNDED | ENTITY_WALLED);
	if(xCnt)
	{
		// prev = e->velocity.x;
		if(vx < 0.0f)
		{
			// place on right
			e->x = xr.x + xr.w;
			e->flags |= ENTITY_WALLED;
		}
		else
		{
			// place on left
			e->x = xr.x - e->w;
			e->flags |= ENTITY_WALLED;
		}
		e->vx = vx = 0.0f;
	}
	if(yCnt)
	{
		// prev = e->velocity.y;
		if(vy < 0.0f)
		{
			// place on bottom
			e->y = yr.y + yr.h;
			e->flags |= ENTITY_CEILED;
		}
		else
		{
			// place on top
			e->y = yr.y - e->h;
			e->flags |= ENTITY_GROUNDED;
		}
		//if(FLOATEQUAL(prev, e->velocity.y))
		e->vy = vy = 0.0f;
		//else
			//vy = e->velocity.y * (1.0f - entryTime);
	}
	el_discard(&el);
	return xCnt || yCnt;
}
