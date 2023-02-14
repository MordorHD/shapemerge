#define FLOATEQUAL(a, b) (fabs((a) - (b)) < 0.0001)

// true if e moved
bool e_checkcollision(e_t *e, float ticks)
{
    // Step 1: Get velocity box
    Rect2 velBox;
    float vx, vy;
    // Step 2: Get all close tiles as rects
    int x1, y1, x2, y2;
    Rect2 *rects;
    int rectCnt = 0;
    int scc = 0;
    // Step 3: Figure out collision between them and the e
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
        velBox.left = e->x;
        velBox.right = e->x + e->width + vx;
    }
    else
    {
        velBox.left = e->x + vx;
        velBox.right = e->x + e->width;
    }
    if(vy > 0.0f)
    {
        velBox.top = e->y;
        velBox.bottom = e->y + e->height + vy;
    }
    else
    {
        velBox.top = e->y + vy;
        velBox.bottom = e->y + e->height;
    }

    // Step 2:
    x1 = velBox.left   - velBox.left % GRID_SPSIZE;
    y1 = velBox.top    - velBox.top  % GRID_SPSIZE;
    x2 = velBox.right  + GRID_SPSIZE - velBox.right  % GRID_SPSIZE;
    y2 = velBox.bottom + GRID_SPSIZE - velBox.bottom % GRID_SPSIZE;
    x1 /= GRID_SPSIZE;
    y1 /= GRID_SPSIZE;
    x2 /= GRID_SPSIZE;
    y2 /= GRID_SPSIZE;
    x1 = max(x1, 0);
    y1 = max(y1, 0);
    x2 = min(x2, GRID_WIDTH - 1);
    y2 = min(y2, GRID_HEIGHT - 1);

    if((x2 - x1 + 1) * (y2 - y1 + 1) <= 0)
        return 0;
    // printf("GOING FROM X %d to %d\n", x1, x2);
    // printf("       AND Y %d to %d\n", y1, y2);
   //  printf("   GOING FOR %d RECTS\n", (x2 - x1 + 1) * (y2 - y1 + 1));

    rects = malloc(sizeof(*rects) * (x2 - x1 + 1) * (y2 - y1 + 1), REASON_COLLIDERRECTS);
    for(; y1 <= y2; y1++)
    {
        int x;
        for(x = x1; x < x2; x++)
        {
            if(Grid[x + y1 * GRID_WIDTH])
            {
                scc++;
            }
            else if(scc)
            {
                int i;
                for(i = 0; i < rectCnt; i++)
                {
                    if(rects[i].left == x - scc && rects[i].right == x && rects[i].bottom == y1)
                    {
                        rects[i].bottom++;
                        break;
                    }
                }
                if(i == rectCnt)
                {
                    rects[rectCnt].left = x - scc;
                    rects[rectCnt].top = y1;
                    rects[rectCnt].right = x;
                    rects[rectCnt].bottom = y1 + 1;
                    rectCnt++;
                }
                scc = 0;
            }
        }
        if(scc)
        {
            int i;
            for(i = 0; i < rectCnt; i++)
            {
                if(rects[i].left == x - scc && rects[i].right == x && rects[i].bottom == y1)
                {
                    rects[i].bottom++;
                    break;
                }
            }
            if(i == rectCnt)
            {
                rects[rectCnt].left = x - scc;
                rects[rectCnt].top = y1;
                rects[rectCnt].right = x;
                rects[rectCnt].bottom = y1 + 1;
                rectCnt++;
            }
            scc = 0;
        }
    }
    // printf("   > GOT %d RECTS\n", rectCnt);
    for(int i = 0; i < rectCnt; i++)
    {
        rects[i].left *= GRID_SPSIZE;
        rects[i].top *= GRID_SPSIZE;
        rects[i].right *= GRID_SPSIZE;
        rects[i].bottom *= GRID_SPSIZE;
    }
    if(rectCnt <= 0)
    {
        free(rects);
        return 0;
    }
    // Step 3:
    for(pr = rects; rectCnt; pr++, rectCnt--)
    {
        if(vx == 0.0f)
        {
            entryX = -1.0f/0.0f;
            exitX = 1.0f/0.0f;
        }
        else
        {
            if(vx > 0.0f)
            {
                entryX = pr->left - (e->x + e->width);
                exitX = pr->right - e->x;
            }
            else
            {
                entryX = pr->right - e->x;
                exitX = pr->left - (e->x + e->width);
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
                entryY = pr->top - (e->y + e->height);
                exitY = pr->bottom - e->y;
            }
            else
            {
                entryY = pr->bottom - e->y;
                exitY = pr->top - (e->y + e->height);
            }
            entryY /= vy;
            exitY /= vy;
        }
        if(fmax(entryX, entryY) > fmin(exitX, exitY) ||
           (entryX < 0.0f && entryY < 0.0f)          ||
            entryX > 1.0f || entryY > 1.0f)
            continue;

        if(entryX > entryY)
        {
            if(!xCnt || (ox = (vx < 0 ? xr.right - pr->right : pr->left - xr.left)) <= 0)
            {
                xr = *pr;
                xCnt = 1;
            }
        }
        else
        {
            if(!yCnt || (oy = (vy < 0 ? yr.bottom - pr->bottom : pr->top - yr.top)) <= 0)
            {
                yr = *pr;
                yCnt = 1;
            }
        }
    }
#ifdef DEBUG
    if(xCnt)
    {
        HIGHLIGHTER h = (HIGHLIGHTER) ObjectCreate("Highlighter");
        h->bounds = xr;
        h->highlight = 0xFF7F00;
    }
    if(yCnt)
    {
        HIGHLIGHTER h = (HIGHLIGHTER) ObjectCreate("Highlighter");
        h->bounds = yr;
        h->highlight = 0x007FFF;
    }
    // printf("%d, %d\n", xCnt, yCnt);
#endif
    e->flags &= ~(ENTITYF_CEILED | ENTITYF_GROUNDED | ENTITYF_WALLED);
    if(xCnt)
    {
        // prev = e->velocity.x;
        if(vx < 0.0f)
        {
            // place on right
            e->x = xr.right;
            if(FLOATEQUAL(e->r, 0.5d * M_PI))
                e->flags |= ENTITYF_GROUNDED;
            else if(FLOATEQUAL(e->r, 1.5d * M_PI))
                e->flags |= ENTITYF_CEILED;
            else
                e->flags |= ENTITYF_WALLED;
        }
        else
        {
            // place on left
            e->x = xr.left - e->width;
            if(FLOATEQUAL(e->r, 0.5d * M_PI))
                e->flags |= ENTITYF_CEILED;
            else if(FLOATEQUAL(e->r, 1.5d * M_PI))
                e->flags |= ENTITYF_GROUNDED;
            else
                e->flags |= ENTITYF_WALLED;
        }
        e->vx = vx = 0.0f;
    }
    if(yCnt)
    {
        // prev = e->velocity.y;
        if(vy < 0.0f)
        {
            // place on bottom
            e->y = yr.bottom;
            if(FLOATEQUAL(e->r, 0.5d * M_PI) || FLOATEQUAL(e->r, 1.5d * M_PI))
                e->flags |= ENTITYF_WALLED;
            else if(FLOATEQUAL(e->r, M_PI))
                e->flags |= ENTITYF_GROUNDED;
            else
                e->flags |= ENTITYF_CEILED;
        }
        else
        {
            // place on top
            e->y = yr.top - e->height;
            if(FLOATEQUAL(e->r, 0.5d * M_PI) || FLOATEQUAL(e->r, 1.5d * M_PI))
                e->flags |= ENTITYF_WALLED;
            else if(FLOATEQUAL(e->r, M_PI))
                e->flags |= ENTITYF_CEILED;
            else
                e->flags |= ENTITYF_GROUNDED;
        }
        //if(FLOATEQUAL(prev, e->velocity.y))
        e->vy = vy = 0.0f;
        //else
            //vy = e->velocity.y * (1.0f - entryTime);
    }
    free(rects);
    return xCnt || yCnt;
}
