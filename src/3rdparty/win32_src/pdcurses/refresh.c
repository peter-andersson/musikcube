/* PDCurses */

#include <curspriv.h>
#include <assert.h>

/*man-start**************************************************************

refresh
-------

### Synopsis

    int refresh(void);
    int wrefresh(WINDOW *win);
    int wnoutrefresh(WINDOW *win);
    int doupdate(void);
    int redrawwin(WINDOW *win);
    int wredrawln(WINDOW *win, int beg_line, int num_lines);

### Description

   wrefresh() copies the named window to the physical terminal screen,
   taking into account what is already there in order to optimize cursor
   movement. refresh() does the same, using stdscr. These routines must
   be called to get any output on the terminal, as other routines only
   manipulate data structures. Unless leaveok() has been enabled, the
   physical cursor of the terminal is left at the location of the
   window's cursor.

   wnoutrefresh() and doupdate() allow multiple updates with more
   efficiency than wrefresh() alone. wrefresh() works by first calling
   wnoutrefresh(), which copies the named window to the virtual screen.
   It then calls doupdate(), which compares the virtual screen to the
   physical screen and does the actual update. A series of calls to
   wrefresh() will result in alternating calls to wnoutrefresh() and
   doupdate(), causing several bursts of output to the screen. By first
   calling wnoutrefresh() for each window, it is then possible to call
   doupdate() only once.

   In PDCurses, redrawwin() is equivalent to touchwin(), and wredrawln()
   is the same as touchline(). In some other curses implementations,
   there's a subtle distinction, but it has no meaning in PDCurses.

### Return Value

   All functions return OK on success and ERR on error.

### Portability
                             X/Open  ncurses  NetBSD
    refresh                     Y       Y       Y
    wrefresh                    Y       Y       Y
    wnoutrefresh                Y       Y       Y
    doupdate                    Y       Y       Y
    redrawwin                   Y       Y       Y
    wredrawln                   Y       Y       Y

**man-end****************************************************************/

#include <string.h>

static void _normalize_cursor( WINDOW *win)
{
    if( win->_cury < 0)
        win->_cury = 0;
    if( win->_cury >= win->_maxy)
        win->_cury = win->_maxy - 1;
    if( win->_curx < 0)
        win->_curx = 0;
    if( win->_curx >= win->_maxx)
        win->_curx = win->_maxx - 1;
}

int PDC_pnoutrefresh_with_stored_params( WINDOW *pad);       /* pad.c */

int wnoutrefresh(WINDOW *win)
{
    int begy, begx;     /* window's place on screen   */
    int i, j;

    PDC_LOG(("wnoutrefresh() - called: win=%p\n", win));

    assert( win);
    if ( !win)
        return ERR;
    if( is_pad( win))
        return PDC_pnoutrefresh_with_stored_params( win);

    begy = win->_begy;
    begx = win->_begx;

    for (i = 0, j = begy; i < win->_maxy && j < curscr->_maxy; i++, j++)
    {
        if (win->_firstch[i] != _NO_CHANGE && j >= 0)
        {
            chtype *src = win->_y[i];
            chtype *dest = curscr->_y[j] + begx;

            int first = win->_firstch[i]; /* first changed */
            int last = win->_lastch[i];   /* last changed */

            if( last > curscr->_maxx - begx - 1)    /* don't run off right-hand */
                last = curscr->_maxx - begx - 1;    /* edge of screen */
            if( first < -begx)       /* ...nor the left edge */
                first = -begx;

            /* ignore areas on the outside that are marked as changed,
               but really aren't */

            while (first <= last && src[first] == dest[first])
                first++;

            while (last >= first && src[last] == dest[last])
                last--;

            /* if any have really changed... */

            if (first <= last)
            {
                memcpy(dest + first, src + first,
                       (last - first + 1) * sizeof(chtype));

                first += begx;
                last += begx;

                if (first < curscr->_firstch[j] ||
                    curscr->_firstch[j] == _NO_CHANGE)
                    curscr->_firstch[j] = first;

                if (last > curscr->_lastch[j])
                    curscr->_lastch[j] = last;
            }
        }
        PDC_set_changed_cells_range( win, i, _NO_CHANGE, _NO_CHANGE);
    }

    if (win->_clear)
        win->_clear = FALSE;

    if (!win->_leaveit)
    {
        curscr->_cury = win->_cury + begy;
        curscr->_curx = win->_curx + begx;
        _normalize_cursor( curscr);
    }

    return OK;
}

int doupdate(void)
{
    int y;
    bool clearall;

    PDC_LOG(("doupdate() - called\n"));

    assert( SP);
    assert( curscr);
    if (!SP || !curscr)
        return ERR;

    if (isendwin())         /* coming back after endwin() called */
    {
        reset_prog_mode();
        clearall = TRUE;
        SP->alive = TRUE;   /* so isendwin() result is correct */
    }
    else
        clearall = curscr->_clear;

    for (y = 0; y < SP->lines; y++)
    {
        PDC_LOG(("doupdate() - Transforming line %d of %d: %s\n",
                 y, SP->lines, (curscr->_firstch[y] != _NO_CHANGE) ?
                 "Yes" : "No"));

        if (clearall || curscr->_firstch[y] != _NO_CHANGE)
        {
            int first, last;

            chtype *src = curscr->_y[y];
            chtype *dest = SP->lastscr->_y[y];

            if (clearall)
            {
                first = 0;
                last = COLS - 1;
            }
            else
            {
                first = curscr->_firstch[y];
                last = curscr->_lastch[y];
            }

            while (first <= last)
            {
                int len = 0;

                /* build up a run of changed cells; if two runs are
                   separated by a single unchanged cell, ignore the
                   break */

                if (clearall)
                    len = last - first + 1;
                else
                    while (first + len <= last &&
                           (src[first + len] != dest[first + len] ||
                            (len && first + len < last &&
                             src[first + len + 1] != dest[first + len + 1])
                           )
                          )
                        len++;

                /* update the screen, and SP->lastscr */

                if (len)
                {
                    PDC_transform_line(y, first, len, src + first);
                    memcpy(dest + first, src + first, len * sizeof(chtype));
                    first += len;
                }

                /* skip over runs of unchanged cells */

                while (first <= last && src[first] == dest[first])
                    first++;
            }

            PDC_set_changed_cells_range( curscr, y, _NO_CHANGE, _NO_CHANGE);
        }
    }

    curscr->_clear = FALSE;

    if (SP->visibility)
        PDC_gotoyx(curscr->_cury, curscr->_curx);

    SP->cursrow = curscr->_cury;
    SP->curscol = curscr->_curx;

    PDC_doupdate();

    return OK;
}

int wrefresh(WINDOW *win)
{
    bool save_clear;

    PDC_LOG(("wrefresh() - called\n"));

    assert( win);
    if ( !win || (win->_flags & (_PAD|_SUBPAD)) )
        return ERR;

    save_clear = win->_clear;

    if (win == curscr)
        curscr->_clear = TRUE;
    else
        wnoutrefresh(win);

    if (save_clear && win->_maxy == SP->lines && win->_maxx == SP->cols)
        curscr->_clear = TRUE;

    return doupdate();
}

int refresh(void)
{
    PDC_LOG(("refresh() - called\n"));

    return wrefresh(stdscr);
}

int wredrawln(WINDOW *win, int start, int num)
{
    int i;

    PDC_LOG(("wredrawln() - called: win=%p start=%d num=%d\n",
        win, start, num));

    assert( win);
    if (!win || start > win->_maxy || start + num > win->_maxy)
        return ERR;

    for (i = start; i < start + num; i++)
        PDC_mark_line_as_changed( win, i);

    return OK;
}

int redrawwin(WINDOW *win)
{
    PDC_LOG(("redrawwin() - called: win=%p\n", win));

    assert( win);
    if (!win)
        return ERR;

    return wredrawln(win, 0, win->_maxy);
}
