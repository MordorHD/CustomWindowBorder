#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include <stdbool.h>

#define CSBM_SETPOS (WM_USER+1)
#define CSBM_SETMAX (WM_USER+2)
#define CSBM_SETSCROLLINFO (WM_USER+3)
#define WM_UNEMBED (WM_USER+4)

#define CFRAME_MINWIDTH 140
#define CFRAME_MINHEIGHT 80
#define CSCROLL_MINSIZE 10
#define CSCROLL_SIZE 13
#define crstrcpy(str) (strcpy(malloc(strlen(str) + 1), (str)))
HBRUSH BkrBrush;
HBRUSH BkrCmplyBrush;
HBRUSH FgrBrush;
HBRUSH FgrCmplyBrush;
HBRUSH DarkBkrBrush;
HBRUSH PenClrBrush;

struct cscroll {
    RECT rcTrack;
    RECT rcThumb;
    int page;
    int unit;
    int pos, max;
};

struct cscrolls {
    struct cscroll v, h;
};

#define CFRAMEF_MAXIMIZED 1
#define CFRAMEF_EMBEDDED 2
#define CFRAMEF_TOPBORDER (CFRAMEF_MAXIMIZED|CFRAMEF_EMBEDDED)

struct cframe {
    char *title;
    struct cscrolls *scrolls;
    int flags;
    RECT rcRestore;
    RECT rcIcon, rcTitle, rcClose, rcMin, rcMax;
    HICON icon;
};

void Draw3DButton(HDC hdc, bool pressed, int raise, const RECT *rc, HBRUSH erase, HBRUSH fgr, HBRUSH bkr)
{
    RECT r;
    r = *rc;
    if(pressed)
    {
        r.top += raise;
        FillRect(hdc, &r, fgr);
        r.bottom = r.top;
        r.top = rc->top;
        FillRect(hdc, &r, erase);
    }
    else
    {
        r.bottom -= raise;
        FillRect(hdc, &r, fgr);
        r.top = r.bottom;
        r.bottom = rc->bottom;
        FillRect(hdc, &r, bkr);
    }
}

enum {
    CCOLOR_ALPHA,
    CCOLOR_DIGIT,
    CCOLOR_CHAR,
    CCOLOR_QUOTE,
    CCOLOR_DIRECTIVE,
    CCOLOR_COMMENT,
    CCOLOR_KW1,
    CCOLOR_KW2,
};
#define CCOLOR_KWN(n) ((CCOLOR_KW1-1)+(n))
COLORREF DefaultPalette[] = {
    0x969483,
    0x338833,
    0x333388,
    0x883388,
    0x338833,
    0x7C7A6A,
    0x338888,
    0x883388,
};

#define CIDT_LHSCROLL 3
#define CIDT_RHSCROLL 4
#define CIDT_LVSCROLL 5
#define CIDT_RVSCROLL 6

#define CAUTOSCROLL_FACT 20

#define CDELTATIME_SCROLL 100

LRESULT CALLBACK CFrameProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int ht = -1, htp = -1;
    static int offX, offY;

    static HBRUSH h1, h2;// MARKINGINIGNIGNG
    static HBRUSH h3, h4;// MARKINGINIGNIGNG

    PAINTSTRUCT ps;
    HDC hdc;
    POINT p;
    RECT r, rc;
    RECT rcScroll, rcClip;
    WINDOWPOS wp, *pwp;
    NCCALCSIZE_PARAMS *nccp;
    HRGN hRgn;
    short newVScroll, newHScroll;
    struct cscroll *scroll;
    struct cscroll *tsc;
    int dx, dy;
    int size;
    struct cframe *frame = (struct cframe*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    char *textBuf;
    int len;
    HMONITOR hMon;
    MONITORINFO mi;
    switch(msg)
    {
    case WM_TIMER:
        GetCursorPos(&p);
        ScreenToClient(hWnd, &p);
        switch(wParam)
        {
        case CIDT_LHSCROLL:
            if(p.x >= frame->scrolls->h.rcThumb.left)
                KillTimer(hWnd, CIDT_LHSCROLL);
            else
                SendMessage(hWnd, WM_HSCROLL, SB_PAGERIGHT, 0);
            break;
        case CIDT_RHSCROLL:
            if(p.x < frame->scrolls->h.rcThumb.right)
                KillTimer(hWnd, CIDT_RHSCROLL);
            else
                SendMessage(hWnd, WM_HSCROLL, SB_PAGELEFT, 0);
            break;
        case CIDT_LVSCROLL:
            if(p.y >= frame->scrolls->v.rcThumb.top)
                KillTimer(hWnd, CIDT_LVSCROLL);
            else
                SendMessage(hWnd, WM_VSCROLL, SB_PAGEDOWN, 0);
            break;
        case CIDT_RVSCROLL:
            if(p.y < frame->scrolls->v.rcThumb.bottom)
                KillTimer(hWnd, CIDT_RVSCROLL);
            else
                SendMessage(hWnd, WM_VSCROLL, SB_PAGEUP, 0);
            break;
        }
        return 0;
    case WM_NCCREATE: // this is the first call to this frame
        if(!((CREATESTRUCT*) lParam)->lpCreateParams)
            return 0;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) (frame = ((CREATESTRUCT*) lParam)->lpCreateParams));
        // TODO: BRUSHES FOR TESTING
        h1 = CreateSolidBrush(0x0000FF);
        h2 = CreateSolidBrush(0x0000AA);

        h3 = CreateSolidBrush(0x9F6060);
        h4 = CreateSolidBrush(0x6C4141);

        SetWindowText(hWnd, crstrcpy(((CREATESTRUCT*) lParam)->lpszName));
        if(frame->flags & CFRAMEF_MAXIMIZED)
        {
            hMon = MonitorFromWindow(NULL, MONITOR_DEFAULTTOPRIMARY);
            mi.cbSize = sizeof(mi);
            GetMonitorInfo(hMon, &mi);
            MoveWindow(hWnd, mi.rcWork.left, mi.rcWork.top, mi.rcWork.right - mi.rcWork.left, mi.rcWork.bottom - mi.rcWork.top, 1);
        }
        else
        {
            wp.flags = 0;
            wp.hwnd = hWnd;
            wp.x = ((CREATESTRUCT*) lParam)->x;
            wp.y = ((CREATESTRUCT*) lParam)->x;
            wp.cx = ((CREATESTRUCT*) lParam)->cx;
            wp.cy = ((CREATESTRUCT*) lParam)->cy;
            wp.hwndInsertAfter = HWND_TOP;
            SendMessage(hWnd, WM_WINDOWPOSCHANGED, 0, (LPARAM) &wp);
        }
        frame->rcRestore.left   = ((CREATESTRUCT*) lParam)->x;
        frame->rcRestore.top    = ((CREATESTRUCT*) lParam)->y;
        frame->rcRestore.right  = frame->rcRestore.left + ((CREATESTRUCT*) lParam)->cx;
        frame->rcRestore.bottom = frame->rcRestore.top + ((CREATESTRUCT*) lParam)->cy;
        return 1;
    case WM_NCACTIVATE:
        return 1;
    case WM_SETTEXT:
        frame->title = (char*) lParam;
        return 0;
    case WM_GETTEXT:
        textBuf = (char*) lParam;
        len = wParam;
        len = min(len, strlen(frame->title));
        memcpy(textBuf, frame->title, len);
        return len;
    case WM_WINDOWPOSCHANGING:
        return 0;
    case WM_WINDOWPOSCHANGED:
        pwp = (WINDOWPOS*) lParam;
        if(!(pwp->flags & SWP_NOMOVE))
            SendMessage(hWnd, WM_MOVE, 0, MAKELPARAM(pwp->x, pwp->y));
        if(!(pwp->flags & SWP_NOSIZE))
        {
            r = (RECT) { 0, 0, pwp->cx, pwp->cy }; // relative window rect
            if(frame->flags & CFRAMEF_TOPBORDER)
            {
                frame->rcIcon = (RECT) { r.left, r.top, r.left + 16, r.top + 16 };
                frame->rcClose = (RECT) { r.right - 30, r.top, r.right, r.top + 16 };
            }
            else
            {
                frame->rcIcon = (RECT) { r.left + 8, r.top + 8, r.left + 24, r.top + 24 };
                frame->rcClose = (RECT) { r.right - 38, r.top + 8, r.right - 8, r.top + 24 };
            }
            frame->rcMax = (RECT) { frame->rcClose.left - 34, frame->rcClose.top, frame->rcClose.left - 4, frame->rcClose.bottom };
            frame->rcMin = (RECT) { frame->rcMax.left - 34, frame->rcClose.top, frame->rcMax.left - 4, frame->rcClose.bottom };
            frame->rcTitle = (RECT) { frame->rcIcon.right + 4, frame->rcIcon.top, frame->rcMin.left - 4, frame->rcIcon.bottom };
            SendMessage(hWnd, WM_NCCALCSIZE, 0, (LPARAM) &r);
            SendMessage(hWnd, WM_SIZE, 0, MAKELPARAM(r.right - r.left, r.bottom - r.top));
            SendMessage(hWnd, WM_NCPAINT, 1, 0);
        }
        return 0;
    case WM_SYNCPAINT: // note: other threads call sync paint to quick redraw
        // for instance, you have one window hiding another window and you minimize the window that hides it,
        // it sends a sync paint message
        // TODO: Taskbar??? wtf are you doing??? (taskbar is not calling syncpaint)
        GetWindowRect(hWnd, &r);
        GetWindowRect(GetWindow(hWnd, GW_HWNDNEXT), &rc);
        SubtractRect(&r, &r, &rc);
        hRgn = CreateRectRgnIndirect(&r);
        SendMessage(hWnd, WM_NCPAINT, (WPARAM) hRgn, 0);
        return 0;
    case WM_SETCURSOR:
        switch(LOWORD(lParam))
        {
        case HTCLOSE: case HTMINBUTTON: case HTMAXBUTTON: SetCursor(LoadCursor(NULL, IDC_HAND)); break;
        case HTNOWHERE:
        case HTCAPTION: case HTVSCROLL: case HTHSCROLL: case HTBORDER: SetCursor(LoadCursor(NULL, IDC_CROSS)); break;
        case HTBOTTOM: case HTTOP: SetCursor(LoadCursor(NULL, IDC_SIZENS)); break;
        case HTRIGHT: case HTLEFT: SetCursor(LoadCursor(NULL, IDC_SIZEWE)); break;
        case HTTOPRIGHT: case HTBOTTOMLEFT: SetCursor(LoadCursor(NULL, IDC_SIZENESW)); break;
        case HTTOPLEFT: case HTBOTTOMRIGHT: SetCursor(LoadCursor(NULL, IDC_SIZENWSE)); break;
        }
        return 0;
    case WM_GETMINMAXINFO:
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_NCCALCSIZE:
        nccp = (NCCALCSIZE_PARAMS*) lParam;
        nccp->rgrc[0].top += 22;
        if(!(frame->flags & CFRAMEF_TOPBORDER))
        {
            nccp->rgrc[0].top += 8;
            nccp->rgrc[0].left += 8;
            nccp->rgrc[0].right -= 8;
            nccp->rgrc[0].bottom -= 8;
        }
        if(frame->scrolls->h.max)
            nccp->rgrc[0].bottom -= CSCROLL_SIZE;
        if(frame->scrolls->v.max)
            nccp->rgrc[0].right -= CSCROLL_SIZE;
        return 0;
    case WM_NCPAINT:
        hdc = wParam != 1 ? GetDCEx(hWnd, (HRGN) wParam, DCX_WINDOW | DCX_INTERSECTRGN) : GetDCEx(hWnd, NULL, DCX_WINDOW);

        GetWindowRect(hWnd, &rc);
        rc.right -= rc.left;
        rc.bottom -= rc.top;
        rc.left = 0;
        rc.top = 0;
        if(frame->flags & CFRAMEF_TOPBORDER)
        {
            // north
            r.left = rc.left;
            r.top = rc.top;
            r.right = rc.right;
            r.bottom = rc.top += 22;
            FillRect(hdc, &r, PenClrBrush);
        }
        else
        {
            // north
            r.left = rc.left;
            r.top = rc.top;
            r.right = rc.right;
            r.bottom = rc.top + 30;
            FillRect(hdc, &r, PenClrBrush);
            // south
            r.top = rc.bottom - 8;
            r.bottom = rc.bottom + 1;
            FillRect(hdc, &r, PenClrBrush);
            // east
            r.left = rc.left;
            r.top = rc.top + 30;
            r.right = r.left + 8;
            r.bottom = rc.bottom - 8;
            FillRect(hdc, &r, PenClrBrush);
            // west
            r.left = rc.right - 8;
            r.right = rc.right;
            FillRect(hdc, &r, PenClrBrush);

            FrameRect(hdc, &rc, BkrCmplyBrush);
        }
        // draw icon
        DrawIconEx(hdc, frame->rcIcon.left, frame->rcIcon.top, frame->icon, frame->rcIcon.right - frame->rcIcon.left, frame->rcIcon.bottom - frame->rcIcon.top, 0, NULL, DI_NORMAL);
        // draw title
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, DefaultPalette[CCOLOR_ALPHA]);
        DrawText(hdc, frame->title, strlen(frame->title), &frame->rcTitle, DT_LEFT | DT_TOP | DT_SINGLELINE);
        // draw scroll bars
        if(frame->scrolls->v.max)
        {
            FillRect(hdc, &frame->scrolls->v.rcTrack, BkrBrush);
            FillRect(hdc, &frame->scrolls->v.rcThumb, FgrBrush);
            FrameRect(hdc, &frame->scrolls->v.rcThumb, FgrCmplyBrush);
        }
        if(frame->scrolls->h.max)
        {
            FillRect(hdc, &frame->scrolls->h.rcTrack, BkrBrush);
            FillRect(hdc, &frame->scrolls->h.rcThumb, FgrBrush);
            FrameRect(hdc, &frame->scrolls->h.rcThumb, FgrCmplyBrush);
            if(frame->scrolls->v.max)
            {
                r = (RECT) { frame->scrolls->h.rcTrack.right, frame->scrolls->v.rcTrack.bottom, frame->scrolls->v.rcTrack.right, frame->scrolls->h.rcTrack.bottom };
                FillRect(hdc, &r, FgrBrush);
            }
        }
        Draw3DButton(hdc, 0, 4, &frame->rcClose, PenClrBrush, h1, h2);
        if(frame->flags & CFRAMEF_EMBEDDED)
        {
            FillRect(hdc, &frame->rcMax, h4);
            FillRect(hdc, &frame->rcMin, h4);
        }
        else
        {
            Draw3DButton(hdc, 0, 4, &frame->rcMax, PenClrBrush, h3, h4);
            Draw3DButton(hdc, 0, 4, &frame->rcMin, PenClrBrush, h3, h4);
        }
        ReleaseDC(hWnd, hdc);
        return 0;
    case WM_NCHITTEST:
        p.x = GET_X_LPARAM(lParam);
        p.y = GET_Y_LPARAM(lParam);
        GetWindowRect(hWnd, &r);
        if(PtInRect(&frame->rcClose, (POINT) { p.x - r.left, p.y - r.top }))
            return HTCLOSE;
        if(PtInRect(&frame->rcMin, (POINT) { p.x - r.left, p.y - r.top }))
            return !(frame->flags & CFRAMEF_EMBEDDED) << 3; // equivalent to: () ? HTNOWHERE : HTMINBUTTON
        if(PtInRect(&frame->rcMax, (POINT) { p.x - r.left, p.y - r.top }))
            return (frame->flags & CFRAMEF_EMBEDDED) ? HTNOWHERE : HTMAXBUTTON;
        if(PtInRect(&frame->rcIcon, (POINT) { p.x - r.left, p.y - r.top }))
            return HTCAPTION;
        if(!(frame->flags & CFRAMEF_TOPBORDER))
        {
            if(p.y < r.top + 8)
            {
                offY = p.y - r.top;
                if(p.x < r.left + 8)
                {
                    offX = p.x - r.left;
                    return HTTOPLEFT;
                }
                if(p.x >= r.right - 8)
                {
                    offX = p.x - r.right;
                    return HTTOPRIGHT;
                }
                return HTTOP;
            }
            if(p.y >= r.bottom - 8)
            {
                offY = p.y - r.bottom;
                if(p.x < r.left + 8)
                {
                    offX = p.x - r.left;
                    return HTBOTTOMLEFT;
                }
                if(p.x >= r.right - 8)
                {
                    offX = p.x - r.right;
                    return HTBOTTOMRIGHT;
                }
                return HTBOTTOM;
            }
            if(p.x < r.left + 8)
            {
                offX = p.x - r.left;
                return HTLEFT;
            }
            if(p.x >= r.right - 8)
            {
                offX = p.x - r.right;
                return HTRIGHT;
            }
            if(p.y <= r.top + 30)
            {
                offX = p.x - r.left;
                offY = p.y - r.top;
                return HTBORDER;
            }
        }
        else
        {
            if(p.y <= r.top + 22)
            {
                offX = p.x - r.left;
                offY = p.y - r.top;
                return HTBORDER;
            }
        }
        p.x -= r.left;
        p.y -= r.top;
        if(frame->scrolls->v.max && PtInRect(&frame->scrolls->v.rcTrack, p))
        {
            offY = p.y + frame->scrolls->v.rcTrack.top - frame->scrolls->v.rcThumb.top;
            return HTVSCROLL;
        }
        if(frame->scrolls->h.max && PtInRect(&frame->scrolls->h.rcTrack, p))
        {
            offX = p.x + frame->scrolls->h.rcTrack.left - frame->scrolls->h.rcThumb.left;
            return HTHSCROLL;
        }
        return HTCLIENT;
    case CSBM_SETPOS:
        hdc = GetDCEx(hWnd, NULL, DCX_WINDOW);
        if(wParam)
        {
            r.top = frame->scrolls->h.rcThumb.top;
            r.bottom = frame->scrolls->h.rcThumb.bottom;
            r.left = frame->scrolls->h.rcThumb.left;

            frame->scrolls->h.pos = lParam;
            frame->scrolls->h.rcThumb.right -= frame->scrolls->h.rcThumb.left;
            frame->scrolls->h.rcThumb.left = frame->scrolls->h.rcTrack.left + frame->scrolls->h.pos * (frame->scrolls->h.page - frame->scrolls->h.rcThumb.right) / frame->scrolls->h.max;
            frame->scrolls->h.rcThumb.right += frame->scrolls->h.rcThumb.left;

            dx = frame->scrolls->h.rcThumb.left - r.left;
            if(dx < 0)
            {
                r.right = frame->scrolls->h.rcThumb.right;
                r.left += dx;
                FillRect(hdc, &r, FgrBrush);
                r.left = r.right;
                r.right = r.left - dx;
                FillRect(hdc, &r, BkrBrush);
            }
            else
            {
                r.right = frame->scrolls->h.rcThumb.right;
                r.left += dx;
                FillRect(hdc, &r, FgrBrush);
                r.right = r.left;
                r.left = r.right - dx;
                FillRect(hdc, &r, BkrBrush);
            }
            FrameRect(hdc, &frame->scrolls->h.rcThumb, FgrCmplyBrush);
        }
        else
        {
            r.left = frame->scrolls->v.rcThumb.left;
            r.right = frame->scrolls->v.rcThumb.right;
            r.top = frame->scrolls->v.rcThumb.top;

            frame->scrolls->v.pos = lParam;
            frame->scrolls->v.rcThumb.bottom -= frame->scrolls->v.rcThumb.top;
            frame->scrolls->v.rcThumb.top = frame->scrolls->v.rcTrack.top + frame->scrolls->v.pos * (frame->scrolls->v.page - frame->scrolls->v.rcThumb.bottom) / frame->scrolls->v.max;
            frame->scrolls->v.rcThumb.bottom += frame->scrolls->v.rcThumb.top;

            dy = frame->scrolls->v.rcThumb.top - r.top;
            if(dy < 0)
            {
                r.bottom = frame->scrolls->v.rcThumb.bottom;
                r.top += dy;
                FillRect(hdc, &r, FgrBrush);
                r.top = r.bottom;
                r.bottom = r.top - dy;
                FillRect(hdc, &r, BkrBrush);
            }
            else
            {
                r.bottom = frame->scrolls->v.rcThumb.bottom;
                r.top += dy;
                FillRect(hdc, &r, FgrBrush);
                r.bottom = r.top;
                r.top = r.bottom - dy;
                FillRect(hdc, &r, BkrBrush);
            }
            FrameRect(hdc, &frame->scrolls->v.rcThumb, FgrCmplyBrush);
        }
        ReleaseDC(hWnd, hdc);
        return 0;
    case CSBM_SETMAX:
        tsc = wParam ? &frame->scrolls->h : &frame->scrolls->v;
        if(tsc->max == lParam)
            return 0;
        tsc->max = ((int) lParam * tsc->unit - tsc->page) / tsc->unit;
        tsc->pos = max(0, min(tsc->pos, tsc->max));
        if(tsc->max <= 0)
            tsc->max = 0;
        else
        {
            GetWindowRect(hWnd, &rc);
            r = rc;
            SendMessage(hWnd, WM_NCCALCSIZE, 0, (LPARAM) &r);
            if(wParam) // change properties of horizontal bar
            {
                frame->scrolls->h.rcTrack.left = r.left - rc.left;
                frame->scrolls->h.rcTrack.top = r.bottom - rc.top;
                frame->scrolls->h.rcTrack.right = r.right - rc.left;
                frame->scrolls->h.rcTrack.bottom = r.bottom - rc.top + CSCROLL_SIZE;
                frame->scrolls->h.rcThumb.top = frame->scrolls->h.rcTrack.top + 1;
                frame->scrolls->h.rcThumb.bottom = frame->scrolls->h.rcTrack.bottom - 1;
                frame->scrolls->h.rcThumb.right = max((r.right - r.left) * tsc->page / ((int) lParam * tsc->unit), CSCROLL_MINSIZE);
                frame->scrolls->h.rcThumb.left = r.left - rc.left + frame->scrolls->h.pos * (tsc->page - frame->scrolls->h.rcThumb.right) / frame->scrolls->h.max;
                frame->scrolls->h.rcThumb.right += frame->scrolls->h.rcThumb.left;
            }
            else // change properties of vertical bar
            {
                frame->scrolls->v.rcTrack.left = r.right - rc.left;
                frame->scrolls->v.rcTrack.top = r.top - rc.top;
                frame->scrolls->v.rcTrack.right = r.right - rc.left + CSCROLL_SIZE;
                frame->scrolls->v.rcTrack.bottom = r.bottom - rc.top;
                frame->scrolls->v.rcThumb.left = frame->scrolls->v.rcTrack.left + 1;
                frame->scrolls->v.rcThumb.right = frame->scrolls->v.rcTrack.right - 1;
                frame->scrolls->v.rcThumb.bottom = max((r.bottom - r.top) * tsc->page / ((int) lParam * tsc->unit), CSCROLL_MINSIZE);
                frame->scrolls->v.rcThumb.top = r.top - rc.top + frame->scrolls->v.pos * (tsc->page - frame->scrolls->v.rcThumb.bottom) / frame->scrolls->v.max;
                frame->scrolls->v.rcThumb.bottom += frame->scrolls->v.rcThumb.top;
            }
        }
        return 0;
    case CSBM_SETSCROLLINFO:
        scroll = (struct cscroll*) lParam;
        tsc = wParam ? &frame->scrolls->h : &frame->scrolls->v;
        tsc->page = scroll->page;
        tsc->unit = scroll->unit;
        tsc->max = (scroll->max * scroll->unit - scroll->page) / scroll->unit;
        tsc->pos = max(0, min(tsc->pos, tsc->max));
        if(tsc->max <= 0)
            tsc->max = 0;
        else
        {
            GetWindowRect(hWnd, &rc);
            r = rc;
            SendMessage(hWnd, WM_NCCALCSIZE, 0, (LPARAM) &r);
            if(wParam) // change properties of horizontal bar
            {
                frame->scrolls->h.rcTrack.left = r.left - rc.left;
                frame->scrolls->h.rcTrack.top = r.bottom - rc.top;
                frame->scrolls->h.rcTrack.right = r.right - rc.left;
                frame->scrolls->h.rcTrack.bottom = r.bottom - rc.top + CSCROLL_SIZE;
                frame->scrolls->h.rcThumb.top = frame->scrolls->h.rcTrack.top + 1;
                frame->scrolls->h.rcThumb.bottom = frame->scrolls->h.rcTrack.bottom - 1;
                frame->scrolls->h.rcThumb.right = max((r.right - r.left) * scroll->page / (scroll->max * scroll->unit), CSCROLL_MINSIZE);
                frame->scrolls->h.rcThumb.left = r.left - rc.left + frame->scrolls->h.pos * (scroll->page - frame->scrolls->h.rcThumb.right) / frame->scrolls->h.max;
                frame->scrolls->h.rcThumb.right += frame->scrolls->h.rcThumb.left;
            }
            else // change properties of vertical bar
            {
                frame->scrolls->v.rcTrack.left = r.right - rc.left;
                frame->scrolls->v.rcTrack.top = r.top - rc.top;
                frame->scrolls->v.rcTrack.right = r.right - rc.left + CSCROLL_SIZE;
                frame->scrolls->v.rcTrack.bottom = r.bottom - rc.top;
                frame->scrolls->v.rcThumb.left = frame->scrolls->v.rcTrack.left + 1;
                frame->scrolls->v.rcThumb.right = frame->scrolls->v.rcTrack.right - 1;
                frame->scrolls->v.rcThumb.bottom = max((r.bottom - r.top) * scroll->page / (scroll->max * scroll->unit), CSCROLL_MINSIZE);
                frame->scrolls->v.rcThumb.top = r.top - rc.top + frame->scrolls->v.pos * (scroll->page - frame->scrolls->v.rcThumb.bottom) / frame->scrolls->v.max;
                frame->scrolls->v.rcThumb.bottom += frame->scrolls->v.rcThumb.top;
            }
        }
        return 0;
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONDBLCLK:
        htp = wParam;
        GetWindowRect(hWnd, &r);
        p.x = GET_X_LPARAM(lParam);
        p.y = GET_Y_LPARAM(lParam);
        p.x -= r.left;
        p.y -= r.top;
        switch(wParam)
        {
        case HTHSCROLL:
            if(p.x < frame->scrolls->h.rcThumb.left)
            {
                SendMessage(hWnd, WM_HSCROLL, SB_PAGERIGHT, 0);
                SetTimer(hWnd, CIDT_LHSCROLL, CDELTATIME_SCROLL, NULL);
            }
            else if(p.x > frame->scrolls->h.rcThumb.right)
            {
                SendMessage(hWnd, WM_HSCROLL, SB_PAGELEFT, 0);
                SetTimer(hWnd, CIDT_RHSCROLL, CDELTATIME_SCROLL, NULL);
            }
            else
            {
                ht = wParam;
            }
            break;
        case HTVSCROLL:
            if(p.y < frame->scrolls->v.rcThumb.top)
            {
                SendMessage(hWnd, WM_VSCROLL, SB_PAGEDOWN, 0);
                SetTimer(hWnd, CIDT_LVSCROLL, CDELTATIME_SCROLL, NULL);
            }
            else if(p.y > frame->scrolls->v.rcThumb.bottom)
            {
                SendMessage(hWnd, WM_VSCROLL, SB_PAGEUP, 0);
                SetTimer(hWnd, CIDT_RVSCROLL, CDELTATIME_SCROLL, NULL);
            }
            else
            {
                ht = wParam;
            }
            break;
        case HTCLOSE:
            hdc = GetDCEx(hWnd, NULL, DCX_WINDOW);
            Draw3DButton(hdc, 1, 4, &frame->rcClose, PenClrBrush, h1, h2);
            ReleaseDC(hWnd, hdc);
            break;
        case HTMAXBUTTON:
            hdc = GetDCEx(hWnd, NULL, DCX_WINDOW);
            Draw3DButton(hdc, 1, 4, &frame->rcMax, PenClrBrush, h3, h4);
            ReleaseDC(hWnd, hdc);
            break;
        case HTMINBUTTON:
            hdc = GetDCEx(hWnd, NULL, DCX_WINDOW);
            Draw3DButton(hdc, 1, 4, &frame->rcMin, PenClrBrush, h3, h4);
            ReleaseDC(hWnd, hdc);
            break;
        default:
            ht = wParam;
        }
        SetCapture(hWnd);
        return 0;
    case WM_MOUSEMOVE:
        if(ht < 0)
            return 0;
        GetCursorPos(&p);
        GetWindowRect(hWnd, &r);
        p.x -= r.left + offX;
        p.y -= r.top + offY;
        switch(ht)
        {
        case HTBORDER:
            if(frame->flags & CFRAMEF_MAXIMIZED)
            {
                rc.top = r.top + p.y;
                rc.bottom = rc.top + (frame->rcRestore.bottom - frame->rcRestore.top);
                rc.left = r.left + p.x + offX - offX * (frame->rcRestore.right - frame->rcRestore.left) / (r.right - r.left);
                rc.right = rc.left + (frame->rcRestore.right - frame->rcRestore.left);
                offX = offX * (frame->rcRestore.right - frame->rcRestore.left) / (r.right - r.left) - p.x;
                frame->flags ^= CFRAMEF_MAXIMIZED;
            }
            else if(frame->flags & CFRAMEF_EMBEDDED)
            {
                rc = (RECT) { r.left + p.x, r.top + p.y, p.x + r.right, p.y + r.bottom };
                rc.right -= rc.left;
                rc.bottom -= rc.top;
                frame->flags ^= CFRAMEF_EMBEDDED;
                SetWindowPos(hWnd, HWND_TOP, rc.left, rc.top, rc.right + 1, rc.bottom + 1, 0); // TODO: adding 1 is just a hack
                SendMessage(hWnd, WM_UNEMBED, 0, 0);
                return 0;
            }
            else
            {
                rc = (RECT) { r.left + p.x, r.top + p.y, p.x + r.right, p.y + r.bottom };
            }
            break;
        case HTTOP:         rc = (RECT) { r.left, min(r.top + p.y, r.bottom - CFRAME_MINHEIGHT), r.right, r.bottom }; break;
        case HTBOTTOM:      rc = (RECT) { r.left, r.top, r.right, r.top + max(p.y, CFRAME_MINHEIGHT) }; break;
        case HTLEFT:        rc = (RECT) { min(r.left + p.x, r.right - CFRAME_MINWIDTH), r.top, r.right, r.bottom }; break;
        case HTRIGHT:       rc = (RECT) { r.left, r.top, r.left + max(p.x, CFRAME_MINWIDTH), r.bottom }; break;
        case HTTOPLEFT:     rc = (RECT) { min(r.left + p.x, r.right - CFRAME_MINWIDTH), min(r.top + p.y, r.bottom - CFRAME_MINHEIGHT), r.right, r.bottom }; break;
        case HTTOPRIGHT:    rc = (RECT) { r.left, min(r.top + p.y, r.bottom - CFRAME_MINHEIGHT), r.left + max(p.x, CFRAME_MINWIDTH), r.bottom }; break;
        case HTBOTTOMLEFT:  rc = (RECT) { min(r.left + p.x, r.right - CFRAME_MINWIDTH), r.top, r.right, r.top + max(p.y, CFRAME_MINHEIGHT) }; break;
        case HTBOTTOMRIGHT: rc = (RECT) { r.left, r.top, r.left + max(p.x, CFRAME_MINWIDTH), r.top + max(p.y, CFRAME_MINHEIGHT) }; break;
        case HTHSCROLL:
            SendMessage(hWnd, WM_HSCROLL, MAKEWPARAM(SB_THUMBTRACK, p.x * frame->scrolls->h.max / (frame->scrolls->h.page - frame->scrolls->h.rcThumb.right + frame->scrolls->h.rcThumb.left)), 0);
            return 0;
        case HTVSCROLL:
            SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_THUMBTRACK, p.y * frame->scrolls->v.max / (frame->scrolls->v.page - frame->scrolls->v.rcThumb.bottom + frame->scrolls->v.rcThumb.top)), 0);
            return 0;
        default:
            return 0;
        }
        rc.right -= rc.left;
        rc.bottom -= rc.top;
        MoveWindow(hWnd, rc.left, rc.top, rc.right, rc.bottom, 1);
        return 0;
    case WM_LBUTTONUP:
        GetCursorPos(&p);
        GetWindowRect(hWnd, &r);
        p.x -= r.left;
        p.y -= r.top;
        switch(htp)
        {
        case HTCLOSE:
            hdc = GetDCEx(hWnd, NULL, DCX_WINDOW);
            Draw3DButton(hdc, 0, 4, &frame->rcClose, PenClrBrush, h1, h2);
            ReleaseDC(hWnd, hdc);
            if(PtInRect(&frame->rcClose, p))
                if(!SendMessage(hWnd, WM_CLOSE, 0, 0))
                {
                    SendMessage(hWnd, WM_DESTROY, 0, 0);
                    DestroyWindow(hWnd);
                }
            break;
        case HTMAXBUTTON:
            hdc = GetDCEx(hWnd, NULL, DCX_WINDOW);
            Draw3DButton(hdc, 0, 4, &frame->rcMax, PenClrBrush, h3, h4);
            ReleaseDC(hWnd, hdc);
            if(!(frame->flags & CFRAMEF_EMBEDDED) && PtInRect(&frame->rcMax, p))
            {
                if(frame->flags & CFRAMEF_MAXIMIZED)
                {
                    frame->flags ^= CFRAMEF_MAXIMIZED;
                    MoveWindow(hWnd, frame->rcRestore.left, frame->rcRestore.top, frame->rcRestore.right - frame->rcRestore.left, frame->rcRestore.bottom - frame->rcRestore.top, 1);
                }
                else
                {
                    hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
                    mi.cbSize = sizeof(mi);
                    GetMonitorInfo(hMon, &mi);
                    frame->flags |= CFRAMEF_MAXIMIZED;
                    GetWindowRect(hWnd, &frame->rcRestore);
                    MoveWindow(hWnd, mi.rcWork.left, mi.rcWork.top, mi.rcWork.right - mi.rcWork.left, mi.rcWork.bottom - mi.rcWork.top, 1);
                }
            }
            break;
        case HTMINBUTTON:
            hdc = GetDCEx(hWnd, NULL, DCX_WINDOW);
            Draw3DButton(hdc, 0, 4, &frame->rcMin, PenClrBrush, h3, h4);
            ReleaseDC(hWnd, hdc);
            break;
        case HTBORDER:
            GetCursorPos(&p);
            hMon = MonitorFromPoint(p, MONITOR_DEFAULTTONEAREST);
            mi.cbSize = sizeof(mi);
            GetMonitorInfo(hMon, &mi);
            if(p.y == mi.rcWork.top)
            {
                frame->flags |= CFRAMEF_MAXIMIZED;
                GetWindowRect(hWnd, &frame->rcRestore);
                MoveWindow(hWnd, mi.rcWork.left, mi.rcWork.top, mi.rcWork.right - mi.rcWork.left, mi.rcWork.bottom - mi.rcWork.top, 1);
            }
            break;
        case HTHSCROLL:
            KillTimer(hWnd, CIDT_LHSCROLL);
            KillTimer(hWnd, CIDT_RHSCROLL);
            break;
        case HTVSCROLL:
            KillTimer(hWnd, CIDT_LVSCROLL);
            KillTimer(hWnd, CIDT_RVSCROLL);
            break;
        }
        ht = htp = -1;
        ReleaseCapture();
        return 0;
    case WM_MOUSEWHEEL:
        newVScroll = frame->scrolls->v.pos - GET_WHEEL_DELTA_WPARAM(wParam) / 30;
        goto set_v_scroll;
    case WM_VSCROLL:
        newVScroll = frame->scrolls->v.pos;
        switch(LOWORD(wParam))
        {
        case SB_PAGEDOWN: newVScroll -= frame->scrolls->v.page; break;
        case SB_PAGEUP: newVScroll += frame->scrolls->v.page; break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            newVScroll = HIWORD(wParam);
            break;
        }
    set_v_scroll:
        newVScroll = max(newVScroll, 0);
        newVScroll = min(newVScroll, frame->scrolls->v.max);
        if(newVScroll == frame->scrolls->v.pos)
            return 0;
        ScrollWindow(hWnd, 0, -(newVScroll - frame->scrolls->v.pos) * frame->scrolls->v.unit, NULL, NULL);
        SendMessage(hWnd, CSBM_SETPOS, 0, newVScroll);
        return 0;
    case WM_HSCROLL:
        newHScroll = frame->scrolls->h.pos;
        switch(LOWORD(wParam))
        {
        case SB_PAGEDOWN: newHScroll -= frame->scrolls->h.page; break;
        case SB_PAGEUP: newHScroll += frame->scrolls->h.page; break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            newHScroll = HIWORD(wParam);
            break;
        }
        newHScroll = max(newHScroll, 0);
        newHScroll = min(newHScroll, frame->scrolls->h.max);
        if(newHScroll == frame->scrolls->h.pos)
            return 0;
        ScrollWindow(hWnd, -(newHScroll - frame->scrolls->h.pos) * frame->scrolls->h.unit, 0, NULL, NULL);
        SendMessage(hWnd, CSBM_SETPOS, 1, newHScroll);
        return 0;
    }
    return 0;
}

static struct cscrolls scrolls;
LRESULT CALLBACK IVProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HDC imageDc;
    static int width, height;
    HBITMAP hBmp;
    BITMAP bmp;
    PAINTSTRUCT ps;
    HDC hdc;
    RECT r;
    short newVScroll, newHScroll;
    struct cscroll scroll;
    switch(msg)
    {
    case WM_CREATE:
        hdc = GetDC(hWnd);
        imageDc = CreateCompatibleDC(hdc);
        hBmp = LoadImage(NULL, "TestImage.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        GetObject(hBmp, sizeof(bmp), &bmp);
        width = bmp.bmWidth;
        height = bmp.bmHeight;
        SelectObject(imageDc, hBmp);
        ReleaseDC(hWnd, hdc);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_SIZE:
        scroll.page = HIWORD(lParam);
        scroll.max = height;
        scroll.unit = 1;
        scroll.pos = scrolls.v.pos;
        SendMessage(hWnd, CSBM_SETSCROLLINFO, 0, (LPARAM) &scroll);
        GetClientRect(hWnd, &r);
        scroll.page = r.right;
        scroll.max = width;
        scroll.pos = scrolls.h.pos;
        SendMessage(hWnd, CSBM_SETSCROLLINFO, 1, (LPARAM) &scroll);
        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
        return 0;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        GetClientRect(hWnd, &r);
        BitBlt(hdc, 0, 0, r.right, r.bottom, imageDc, scrolls.h.pos, scrolls.v.pos, SRCCOPY);
        EndPaint(hWnd, &ps);
        return 0;
    case WM_LBUTTONDOWN:
        SetFocus(hWnd);
        return 0;
    }
    return CFrameProc(hWnd, msg, wParam, lParam);
}

int main(void)
{
    WNDCLASS wc;
    memset(&wc, 0 , sizeof(wc));
    wc.lpszClassName = "ImageView";
    wc.lpfnWndProc = IVProc;
    wc.style = CS_OWNDC;
    RegisterClass(&wc);

    BkrBrush = CreateSolidBrush(0x352A00);
    BkrCmplyBrush = CreateSolidBrush(0x000B35);
    FgrBrush = CreateSolidBrush(0x5B4900);
    FgrCmplyBrush = CreateSolidBrush(0x00125B);
    DarkBkrBrush = CreateSolidBrush(0x241D00);
    PenClrBrush = CreateSolidBrush(0x333300);

    struct cframe *frame = malloc(sizeof(*frame));
    frame->scrolls = &scrolls;
    frame->icon = LoadImage(NULL, "Icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
    frame->flags = CFRAMEF_EMBEDDED;

    //frame->flags |= CFRAMEF_MAXIMIZED;
    HWND hWnd = CreateWindow("ImageView", "Title", WS_POPUP | WS_VISIBLE, 30, 30, 300, 300, NULL, NULL, NULL, frame);
    UpdateWindow(hWnd);
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
