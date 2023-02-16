#include "entity.h"

void el_add(el_t *el, e_t *e)
{
	e_t **es;
	int n;

	for(es = el->entities, n = el->nEntities; n; n--, es++)
		if(*es == e)
			return;
	if(el->nEntities == el->szEntities)
	{
		el->szEntities = (el->szEntities + 1) * 2;
		el->entities = realloc(el->entities, el->szEntities * sizeof*el->entities);
	}
	el->entities[el->nEntities] = e;
	el->nEntities++;
}

void qt_add(qt_t *qt, e_t *e)
{
	if(!SDL_HasIntersectionF(&qt->bounds, &e->bounds))
		return;
	if(!qt->isParent && !qt_subdivide(qt))
	{
		// this list grows like this: 1, 3, 7, 15, ... the threshold falls within this list
		if(qt->nEntities == qt->szEntities)
		{
			qt->szEntities = (qt->szEntities + 1) * 2;
			qt->entities = realloc(qt->entities, qt->szEntities * sizeof*qt->entities);
		}
		qt->entities[qt->nEntities] = e;
		qt->nEntities++;
	}
	else
	{
		qt_add(qt->nodes[0], e);
		qt_add(qt->nodes[1], e);
		qt_add(qt->nodes[2], e);
		qt_add(qt->nodes[3], e);
	}
}

void qt_remove(qt_t *qt, e_t *e)
{
	if(!SDL_HasIntersectionF(&qt->bounds, &e->bounds))
		return;
	if(!qt->isParent)
	{
		e_t **es;
		int n;

		for(es = qt->entities, n = qt->nEntities; n--; es++)
			if(*es == e)
			{
				qt->nEntities--;
				memmove(es, es + 1, n * sizeof*es);
				break;
			}
	}
	else
	{
		qt_remove(qt->nodes[0], e);
		qt_remove(qt->nodes[1], e);
		qt_remove(qt->nodes[2], e);
		qt_remove(qt->nodes[3], e);
		if(!qt->nodes[0]->isParent
		&& !qt->nodes[1]->isParent
		&& !qt->nodes[2]->isParent
		&& !qt->nodes[3]->isParent)
		{
			int sum;
		   	
			sum = qt->nodes[0]->nEntities
				+ qt->nodes[1]->nEntities
				+ qt->nodes[2]->nEntities
				+ qt->nodes[3]->nEntities;
			if(sum < ENTITY_LOWERTHRESHOLD)
			{
				e_t **joined, **iter;

				joined = malloc(7 * sizeof*joined);
				iter = joined;
				for(int i = 0; i < 4; i++)
				{
					memcpy(iter, qt->nodes[i]->entities, sizeof(*iter) * qt->nodes[i]->nEntities);
					iter += qt->nodes[i]->nEntities;
					free(qt->nodes[i]->entities);
					free(qt->nodes[i]);
				}
				qt->isParent = 0;
				qt->entities = joined;
				qt->nEntities = sum;
				qt->szEntities = 7;
			}
		}
	}
}

bool qt_subdivide(qt_t *qt)
{
	float halfW, halfH;
	int midX, midY;
	r_t rcSub[4];
	e_t **es;

	if(qt->nEntities < ENTITY_UPPERTHRESHOLD)
		return false;
	if(qt->bounds.h * qt->bounds.w < 4 * ENTITY_AREA_THRESHOLD) 
		return false;
	halfW = qt->bounds.w / 2;
	halfH = qt->bounds.h / 2;
	midX = qt->bounds.x + qt->bounds.w / 2.0f;
	midY = qt->bounds.y + qt->bounds.h / 2.0f;

	rcSub[0] = (r_t) { qt->bounds.x, qt->bounds.y,	halfW, halfH }; // top left
	rcSub[1] = (r_t) { midX, qt->bounds.y,			halfW, halfH }; // top right
	rcSub[2] = (r_t) { qt->bounds.x, midY,			halfW, halfH }; // bottom left
	rcSub[3] = (r_t) { midX, midY,					halfW, halfH }; // bottom right
	es = qt->entities;
	for(int n = 0; n < 4; n++)
	{
		qt->nodes[n] = memset(malloc(sizeof*qt), 0, sizeof*qt); 
		qt->nodes[n]->bounds = rcSub[n];
		for(int i = 0; i < ENTITY_UPPERTHRESHOLD; i++)
			qt_add(qt->nodes[n], es[i]);
	}
	free(es);
	return true;
}

void qt_collect(const qt_t *qt, el_t *el)
{
	e_t **es;
	int n;
	e_t *e;

	if(!SDL_HasIntersectionF(&qt->bounds, &el->bounds))
		return;
	if(!qt->isParent)
	{
		for(es = qt->entities, n = qt->nEntities; n; n--, es++)
		{
			e = *es;
			if(SDL_HasIntersectionF(&el->bounds, &e->bounds))
				el_add(el, e);
		}
	}
	else
	{
		qt_collect(qt->nodes[0], el);
		qt_collect(qt->nodes[1], el);
		qt_collect(qt->nodes[2], el);
		qt_collect(qt->nodes[3], el);
	}
}

e_t *qt_collectfirst(const qt_t *qt, const r_t *rect)
{
	e_t *e;
	
	if(!SDL_HasIntersectionF(&qt->bounds, rect))
		return NULL;
	if(!qt->isParent)
	{
		for(int i = 0; i < qt->nEntities; i++)
		{
			e = qt->entities[i];
			if(SDL_HasIntersectionF(&e->bounds, rect))
				return e;
		}
	}
	else
	{
		for(int i = 0; i < 4; i++)
			if((e = qt_collectfirst(qt->nodes[i], rect)))
				return e;
	}
	return NULL;
}
