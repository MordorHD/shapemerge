#include "entity.h"

#define fail printf

void el_add(el_t *el, e_t *e)
{
	e_t **es;
	int n;

	for(es = el->entities, n = el->nEntities; n; n--, es++)
		if(*es == e)
			return;
	if(el->nEntities == el->szEntities)
	{
		el->szEntities++;
		el->szEntities *= 2;
		el->entities = realloc(el->entities, el->szEntities * sizeof*el->entities);
	}
	el->entities[el->nEntities] = e;
	el->nEntities++;
}

int qt_add(qt_t *qt, e_t *e)
{
	if(!qt || !e)
	{
		fail("qadd > error: tree or entity is null\n");
		return -1;
	}
	if(!SDL_HasIntersectionF(&qt->rect, &e->rect))
		return 1;
	if(!qt->isParent && qt_subdivide(qt))
	{
		// this list grows like this: 1, 3, 7, 15, ... the threshold falls within this list
		if(qt->nEntities == qt->szEntities)
		{
			qt->szEntities *= 2;
			qt->szEntities++;
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
	return 0;
}

int qt_remove(qt_t *qt, e_t *e)
{
	e_t **es;
	int n;

	if(!qt || !e)
	{
		fail("qremove > error: tree or entity is null\n");
		return -1;
	}
	if(!SDL_HasIntersectionF(&qt->rect, &e->rect))
		return 1;
	if(!qt->isParent)
	{
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
		if(!qt->nodes[0]->isParent && 
				!qt->nodes[1]->isParent && 
				!qt->nodes[2]->isParent && 
				!qt->nodes[3]->isParent)
		{
			int sum = qt->nodes[0]->nEntities + 
				qt->nodes[1]->nEntities + 
				qt->nodes[2]->nEntities + 
				qt->nodes[3]->nEntities;
			if(sum < ENTITY_LOWERTHRESHOLD)
			{
				e_t **joined = malloc(sizeof(*joined) * 7);
				e_t **iter = es;
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
	return 0;
}

int qt_subdivide(qt_t *qt)
{
	if(!qt)
	{
		fail("qsubdivide > error: qt is null\n");
		return -1;
	}
	if(qt->isParent)
	{
		fail("qsubdivide > error: qt is already subdivided\n");
		return -1;
	}
	if(qt->nEntities < ENTITY_UPPERTHRESHOLD)
		return 1;
	if(qt->rect.h * qt->rect.w < 4 * ENTITY_AREA_THRESHOLD) 
		return 1;
	int halfX = qt->rect.x + qt->rect.w / 2.0f;
	int halfY = qt->rect.y + qt->rect.h / 2.0f;
	Rect2 rcSub[4] = {
		{ qt->rect.x, qt->rect.y, halfX, halfY }, // top left
		{ halfX, qt->rect.y, qt->rect.x + qt->rect.w, halfY }, // top right
		{ qt->rect.x, halfY, halfX, qt->rect.y + qt->rect.h }, // bottom left
		{ halfX, halfY, qt->rect.x + qt->rect.w, qt->rect.y + qt->rect.h }, // bottom right
	};
	e_t *entities[ENTITY_UPPERTHRESHOLD];
	memcpy(entities, qt->entities, sizeof entities);
	free(qt->entities);
	for(int n = 0; n < 4; n++)
	{
		qt->nodes[n] = memset(malloc(sizeof*qt), 0, sizeof*qt); 
		qt->nodes[n]->rect = rcSub[n];
		for(int i = 0; i < ENTITY_UPPERTHRESHOLD; i++)
			qt_add(qt->nodes[n], entities[i]);
	}
	return 0;
}

int qt_collect(const qt_t *qt, el_t *el)
{
	e_t **es;
	int n;
	e_t *e;
	if(!qt || !el)
	{
		fail("qcollect > error: qt or entity is null\n");
		return -1;
	}
	if(!SDL_HasIntersectionF(&qt->rect, &el->rect))
		return 1;
	if(!qt->isParent)
	{
		for(es = qt->entities, n = qt->nEntities; n; n--, es++)
		{
			e = *es;
			if(SDL_HasIntersectionF(&el->rect, &e->rect))
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
	return 0;
}

e_t *qt_collectfirst(const qt_t *qt, const Rect2 *rect)
{
	e_t *e;
	if(!qt || !rect)
	{
		fail("qcollectfirst > error: qt or rect is null\n");
		return NULL;
	}
	if(!SDL_HasIntersectionF(&qt->rect, rect))
		return NULL;
	if(!qt->isParent)
	{
		for(int i = 0; i < qt->nEntities; i++)
		{
			e = qt->entities[i];
			if(SDL_HasIntersectionF(&e->rect, rect))
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
