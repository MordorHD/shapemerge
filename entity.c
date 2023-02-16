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

void e_setrect(e_t *e, const r_t *r)
{
	free(e->rects);
	e->rects = malloc(sizeof*e->rects);
	e->rects[0] = *r;
	e->nRects = 1;
	e->bounds = *r;
}

void e_addrect(e_t *e, const r_t *r)
{
	e->rects = realloc(e->rects, (e->nRects + 1) * sizeof*e->rects);
	e->rects[e->nRects] = *r;
	e->nRects++;
	if(r->x > e->x)
		e->w = fmax(e->x + e->w, r->x + r->w) - e->x;
	else
	{
		e->x = r->x;
		e->w += e->x - r->x;
	}
	if(r->y > e->y)
		e->h = fmax(e->y + e->w, r->y + r->h) - e->y;
	else
	{
		e->y = r->x;
		e->h += e->y - r->y;
	}
}

void e_setvel(e_t *e, const v_t *v)
{
	e->vel = *v;
}

void e_translate(e_t *e, float x, float y)
{
	r_t *rs;
	int n;

	for(rs = e->rects, n = e->nRects; n; n--, rs++)
	{
		rs->x += x;
		rs->y += y;
	}
	e->x += x;
	e->y += y;
}

struct {
	e_t **d;
	int n, sz;
	qt_t qt;
} Entities = {
	NULL, 0, 0, { .isParent = NULL, .bounds = (r_t) { -1e10f, -1e10f, 2e10f, 2e10f } }
};

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

void e_dispatch(ev_t *ev)
{
	e_t **es, *e;
	int n;

	for(es = Entities.d, n = Entities.n; n; n--, es++)
	{
		e = *es;
		e->proc(e, ev);
	}
}

bool e_checkcollision(e_t *e, float ticks)
{
	// Step 1: Get velocity box
	r_t velBox;
	float vx, vy;
	// Step 2: Collect all potential colliding entities
	el_t el;
	// Step 3: Figure out collision between them and the e
	e_t **es;
	int n;
	e_t *other;
	r_t *this_rs, *other_rs;
	int this_n, other_n;
	r_t this_xr, this_yr;
	r_t other_xr, other_yr;
	float ox, oy;
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
		other = *es;
		if(other == e)
			continue;
		for(this_rs = e->rects, this_n = e->nRects; 
				this_n; 
				this_n--, this_rs++)
		for(other_rs = other->rects, other_n = other->nRects;
				other_n;
				other_n--, other_rs++)
		{
			if(!SDL_HasIntersectionF(&velBox, other_rs))
				continue;
			if(vx == 0.0f)
			{
				entryX = -1.0f/0.0f;
				exitX = 1.0f/0.0f;
			}
			else
			{
				if(vx > 0.0f)
				{
					entryX = other_rs->x - (this_rs->x + this_rs->w);
					exitX = other_rs->x + other_rs->w - this_rs->x;
				}
				else
				{
					entryX = other_rs->x + other_rs->w - this_rs->x;
					exitX = other_rs->x - (this_rs->x + this_rs->w);
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
					entryY = other_rs->y - (this_rs->y + this_rs->h);
					exitY = other_rs->y + other_rs->h - this_rs->y;
				}
				else
				{
					entryY = other_rs->y + other_rs->h - this_rs->y;
					exitY = other_rs->y - (this_rs->y + this_rs->h);
				}
				entryY /= vy;
				exitY /= vy;
			}
			if(fmax(entryX, entryY) > fmin(exitX, exitY) ||
			   (entryX < 0.0f && entryY < 0.0f) ||
				entryX > 1.0f || entryY > 1.0f)
				continue;
	
			if(entryX > entryY)
			{
				ox = vx < 0 ? other_xr.x + other_xr.w - (other_rs->x + other_rs->w) : other_rs->x - other_xr.x;
				if(!xCnt || ox <= 0)
				{
					other_xr = *other_rs;
					this_xr = *this_rs;
					xCnt = 1;
				}
			}
			else
			{
				oy = vy < 0 ? other_yr.y + other_yr.h - (other_rs->y + other_rs->h) : other_rs->y - other_yr.y;
				if(!yCnt || oy <= 0)
				{
					other_yr = *other_rs;
					this_yr = *this_rs;
					yCnt = 1;
				}
			}
		}
	}
	e->flags &= ~(ENTITY_CEILED | ENTITY_GROUNDED | ENTITY_WALLED);
	if(xCnt)
	{
		// other_rsev = e->velocity.x;
		if(vx < 0.0f)
		{
			// place on right
			e_translate(e, other_xr.x + other_xr.w - this_xr.x, 0);
			e->flags |= ENTITY_WALLED;
		}
		else
		{
			// place on left
			e_translate(e, other_xr.x - this_xr.w - this_xr.x, 0);
			e->flags |= ENTITY_WALLED;
		}
		e->vx = 0.0f;
	}
	if(yCnt)
	{
		// other_rsev = e->velocity.y;
		if(vy < 0.0f)
		{
			// place on bottom
			e_translate(e, 0, other_yr.y + other_yr.h - this_yr.y);
			e->flags |= ENTITY_CEILED;
		}
		else
		{
			// place on top
			e_translate(e, 0, other_yr.y - this_yr.h - this_yr.y);
			e->flags |= ENTITY_GROUNDED;
		}
		//if(FLOATEQUAL(other_rsev, e->velocity.y))
		e->vy = 0.0f;
		//else
			//vy = e->velocity.y * (1.0f - entryTime);
	}
	el_discard(&el);
	return xCnt || yCnt;
}
