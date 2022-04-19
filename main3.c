#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include <stdbool.h>

#define CSBM_SETPOS (WM_USER+1)
#define CSBM_SETSCROLLINFO (WM_USER+2)

#define CSCROLL_SIZE 13
#define crstrcpy(str) (strcpy(malloc(strlen(str) + 1), (str)))
HBRUSH BkrBrush;
HBRUSH BkrCmplyBrush;
HBRUSH FgrBrush;
HBRUSH FgrCmplyBrush;
HBRUSH DarkBkrBrush;
HBRUSH PenClrBrush;

struct cscroll {
    int trackPos, trackSize; // pixel trackPosition
    int page;
    int unit;
    int val, max;
};

struct cscrolls {
    struct cscroll v, h;
};

#define CFRAMEF_MAXIMIZED 1
#define CFRAMEF_EMBEDDED 2
#define CFRAMEF_TOPBORDER 3

struct cframecr {
    char *className;
    char *text;
    int flags;
    HMENU hMenu;
    LPVOID lpVoidParam;
};

struct cframe {
    union { // hWnd is created from the creation struct
        HWND hWnd;
        struct cframecr *creation;
    };
    char *title;
    struct cscrolls *scrolls;
    int flags;
    RECT rcLast;
};

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

void CDrawScrolls(HDC hdc, RECT clip, const struct cscrolls *scrolls)
{
    RECT r;
    // draw horizontal scroll bar
    if(scrolls->h.trackSize)
    {
        r.left = clip.left;
        r.right = clip.right;
        r.top = clip.bottom;
        r.bottom = clip.bottom + CSCROLL_SIZE;
        FillRect(hdc, &r, DarkBkrBrush);
        r.top++;
        r.bottom--;
        r.left += scrolls->h.trackPos;
        r.right = r.left + scrolls->h.trackSize;
        FillRect(hdc, &r, FgrBrush);
        FrameRect(hdc, &r, FgrCmplyBrush);
    }
    if(scrolls->v.trackSize)
    {
        r.top = clip.top;
        r.bottom = clip.bottom;
        r.left = clip.right;
        r.right = clip.right + CSCROLL_SIZE;
        FillRect(hdc, &r, DarkBkrBrush);
        r.left++;
        r.right--;
        r.top    = clip.top + scrolls->v.trackPos;
        r.bottom = r.top + scrolls->v.trackSize;
        FillRect(hdc, &r, FgrBrush);
        FrameRect(hdc, &r, FgrCmplyBrush);
    }
    if(scrolls->v.trackSize && scrolls->h.trackSize)
    {
        r = (RECT) { clip.right, clip.bottom, clip.right + CSCROLL_SIZE, clip.bottom + CSCROLL_SIZE };
        FillRect(hdc, &r, FgrBrush);
    }
}

LRESULT CALLBACK Proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int ht = -1;
    static int offX, offY;
    static int sx, sy;
    static bool ditch; // mouse moved against top of screen

    PAINTSTRUCT ps;
    HDC hdc;
    POINT p;
    RECT r, rc, *pr;
    WINDOWPOS *wp;
    MINMAXINFO *mmi;
    NCCALCSIZE_PARAMS *nccp;
    struct cscroll *scroll;
    struct cscroll *tsc;
    int size;
    struct cframe *frame = (struct cframe*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    int dx, dy;
    char *textBuf;
    int len;
    switch(msg)
    {
    case WM_CREATE:
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) (frame = ((CREATESTRUCT*) lParam)->lpCreateParams));
        SetWindowText(hWnd, crstrcpy(((CREATESTRUCT*) lParam)->lpszName));
        if(frame->flags & CFRAMEF_MAXIMIZED)
            SetWindowPos(hWnd, HWND_TOP, 0, 0, 1920, 1040, 0);
        frame->rcLast.left   = ((CREATESTRUCT*) lParam)->x;
        frame->rcLast.top    = ((CREATESTRUCT*) lParam)->y;
        frame->rcLast.right  = frame->rcLast.left + ((CREATESTRUCT*) lParam)->cx;
        frame->rcLast.bottom = frame->rcLast.top + ((CREATESTRUCT*) lParam)->cy;
        frame->hWnd = CreateWindow(frame->creation->className, frame->creation->text, frame->creation->flags | WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hWnd, frame->creation->hMenu, NULL, frame->creation->lpVoidParam);
        return 0;
    case WM_TIMER:
        GetCursorPos(&p);
        ScreenToClient(hWnd, &p);
        switch(wParam)
        {
        case CIDT_LHSCROLL:
            if(p.x >= frame->scrolls->h.trackPos)
                KillTimer(hWnd, CIDT_LHSCROLL);
            else
                SendMessage(frame->hWnd, WM_HSCROLL, SB_PAGELEFT, 0);
            break;
        case CIDT_RHSCROLL:
            if(p.x < frame->scrolls->h.trackPos + frame->scrolls->h.trackSize)
                KillTimer(hWnd, CIDT_RHSCROLL);
            else
                SendMessage(frame->hWnd, WM_HSCROLL, SB_PAGERIGHT, 0);
            break;
        case CIDT_LVSCROLL:
            if(p.y >= frame->scrolls->v.trackPos)
                KillTimer(hWnd, CIDT_LVSCROLL);
            else
                SendMessage(frame->hWnd, WM_VSCROLL, SB_PAGEDOWN, 0);
            break;
        case CIDT_RVSCROLL:
            if(p.y < frame->scrolls->v.trackPos + frame->scrolls->v.trackSize)
                KillTimer(hWnd, CIDT_RVSCROLL);
            else
                SendMessage(frame->hWnd, WM_VSCROLL, SB_PAGEUP, 0);
            break;
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_CLOSE:
        DestroyWindow(frame->hWnd);
        DestroyWindow(hWnd);
        return 0;
    case WM_NCCREATE:
        return !!((CREATESTRUCT*) lParam)->lpCreateParams;
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
        wp = (WINDOWPOS*) lParam;
        if(!(wp->flags & SWP_NOMOVE))
            SendMessage(hWnd, WM_MOVE, 0, MAKELPARAM(wp->x, wp->y));
        if(!(wp->flags & SWP_NOSIZE))
        {
            r = (RECT) { 0, 0, wp->cx, wp->cy };
            SendMessage(hWnd, WM_NCCALCSIZE, 0, (LPARAM) &r);
            r.right -= r.left;
            r.bottom -= r.top;
            MoveWindow(frame->hWnd, 0, 0, r.right, r.bottom, 1);
        }
        return 0;
    case WM_SYNCPAINT: // note: other threads call sync paint to quick redraw
        // for instance, you have one window hiding another window and you minimize the window that hides it,
        // it sends a sync paint message
        // TODO: DO CORRECT CLIPPING (maybe with active window? or top window?)
        SendMessage(hWnd, WM_NCPAINT, 1, 0);
        return 0;
    case WM_SIZE:
        MoveWindow(frame->hWnd, 0, 0, LOWORD(lParam), HIWORD(lParam), 1);
        return 0;
    case WM_PAINT:
        BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        return 0;
    case WM_SETCURSOR:
        switch(LOWORD(lParam))
        {
        case HTVSCROLL: case HTHSCROLL: case HTBORDER: SetCursor(LoadCursor(NULL, IDC_ARROW)); break;
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
        if(!frame || !(frame->flags & CFRAMEF_TOPBORDER))
        {
            nccp->rgrc[0].top += 8;
            nccp->rgrc[0].left += 8;
            nccp->rgrc[0].right -= 8;
            nccp->rgrc[0].bottom -= 8;
        }
        if(frame)
        {
            if(frame->scrolls->h.trackSize)
                nccp->rgrc[0].bottom -= CSCROLL_SIZE;
            if(frame->scrolls->v.trackSize)
                nccp->rgrc[0].right -= CSCROLL_SIZE;
        }
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
            // draw title
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, DefaultPalette[CCOLOR_ALPHA]);
            TextOut(hdc, r.left, r.top, frame->title , strlen(frame->title ));
        }
        else
        {
            // north
            r.left = rc.left;
            r.top = rc.top;
            r.right = rc.right;
            r.bottom = rc.top += 30;
            FillRect(hdc, &r, PenClrBrush);
            // draw title
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, DefaultPalette[CCOLOR_ALPHA]);
            TextOut(hdc, r.left + 8, r.top + 8, frame->title , strlen(frame->title ));
            // south
            r.top = rc.bottom - 8;
            r.bottom = rc.bottom + 1;
            FillRect(hdc, &r, PenClrBrush);
            // east
            r.left = rc.left;
            r.top = rc.top;
            r.right = r.left + 8;
            r.bottom = rc.bottom - 8;
            FillRect(hdc, &r, PenClrBrush);
            // west
            r.left = rc.right - 8;
            r.right = rc.right;
            FillRect(hdc, &r, PenClrBrush);

            rc.left += 8;
            rc.right -= 8;
            rc.bottom -= 8;
        }
        if(frame->scrolls->v.trackSize)
            rc.bottom -= CSCROLL_SIZE;
        if(frame->scrolls->h.trackSize)
            rc.right -= CSCROLL_SIZE;
        CDrawScrolls(hdc, rc, frame->scrolls);
        ReleaseDC(hWnd, hdc);
        return 0;
    case WM_NCHITTEST:
        p.x = GET_X_LPARAM(lParam);
        p.y = GET_Y_LPARAM(lParam);
        GetWindowRect(hWnd, &r);
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
            if(frame->scrolls->h.trackSize && p.y >= r.bottom - 8 - CSCROLL_SIZE)
            {
                offX = p.x - r.left - (sx = frame->scrolls->h.trackPos);
                return HTHSCROLL;
            }
            if(frame->scrolls->v.trackSize && p.x >= r.right - 8 - CSCROLL_SIZE)
            {
                offY = p.y - r.top - (sy = frame->scrolls->v.trackPos);
                return HTVSCROLL;
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
            if(frame->scrolls->h.trackSize && p.y >= r.bottom - CSCROLL_SIZE)
            {
                offX = p.x - r.left - (sx = frame->scrolls->h.trackPos);
                return HTHSCROLL;
            }
            if(frame->scrolls->v.trackSize && p.x >= r.right - CSCROLL_SIZE)
            {
                offY = p.y - r.top - (sy = frame->scrolls->v.trackPos);
                return HTVSCROLL;
            }
        }
        return HTCLIENT;
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONDBLCLK:
        switch(wParam)
        {
        case HTHSCROLL:
            p.x = GET_X_LPARAM(lParam);
            p.y = GET_Y_LPARAM(lParam);
            ScreenToClient(hWnd, &p);
            GetWindowRect(hWnd, &r);
            if(p.x < frame->scrolls->h.trackPos)
            {
                SendMessage(frame->hWnd, WM_HSCROLL, SB_PAGELEFT, 0);
                SetTimer(hWnd, CIDT_LHSCROLL, CDELTATIME_SCROLL, NULL);
            }
            else if(p.x >= frame->scrolls->h.trackPos + frame->scrolls->h.trackSize)
            {
                SendMessage(frame->hWnd, WM_HSCROLL, SB_PAGERIGHT, 0);
                SetTimer(hWnd, CIDT_RHSCROLL, CDELTATIME_SCROLL, NULL);
            }
            else
                break;
            return 0;
        case HTVSCROLL:
            p.x = GET_X_LPARAM(lParam);
            p.y = GET_Y_LPARAM(lParam);
            ScreenToClient(hWnd, &p);
            GetWindowRect(hWnd, &r);
            if(p.y < frame->scrolls->v.trackPos)
            {
                SendMessage(frame->hWnd, WM_VSCROLL, SB_PAGEDOWN, 0);
                SetTimer(hWnd, CIDT_LVSCROLL, CDELTATIME_SCROLL, NULL);
            }
            else if(p.y >= frame->scrolls->v.trackPos + frame->scrolls->v.trackSize)
            {
                SendMessage(frame->hWnd, WM_VSCROLL, SB_PAGEUP, 0);
                SetTimer(hWnd, CIDT_RVSCROLL, CDELTATIME_SCROLL, NULL);
            }
            else
                break;
            return 0;
        }
        SetCapture(hWnd);
        SetFocus(frame->hWnd);
        ht = wParam;
        return 0;
    case WM_NCLBUTTONUP:
        KillTimer(hWnd, CIDT_LHSCROLL);
        KillTimer(hWnd, CIDT_RHSCROLL);
        KillTimer(hWnd, CIDT_LVSCROLL);
        KillTimer(hWnd, CIDT_RVSCROLL);
        return 0;
    case WM_NCMOUSEMOVE:
        return 0;
    case CSBM_SETPOS:
        tsc = wParam ? &frame->scrolls->h : &frame->scrolls->v;
        tsc->val = lParam;
        tsc->trackPos = tsc->val * (tsc->page - tsc->trackSize) / tsc->max;

        /*hdc = GetDCEx(hWnd, NULL, DCX_WINDOW);
        GetWindowRect(hWnd, &r);
        SendMessage(hWnd, WM_NCHITTEST, 0, (LPARAM) &r);
        CDrawScrolls(hdc, r, frame->scrolls);
        ReleaseDC(hWnd, hdc);*/
        SendMessage(hWnd, WM_NCPAINT, 1, 0);
        return 0;
    case CSBM_SETSCROLLINFO:
        scroll = (struct cscroll*) lParam;
        tsc = wParam ? &frame->scrolls->h : &frame->scrolls->v;
        tsc->val = scroll->val;
        tsc->unit = scroll->unit;
        if(scroll->page >= scroll->max)
            tsc->trackSize = 0;
        else
        {
            GetWindowRect(hWnd, &r);
            SendMessage(hWnd, WM_NCCALCSIZE, 0, (LPARAM) &r);
            size = wParam ? r.right - r.left : r.bottom - r.top;
            tsc->trackSize = size * scroll->page / (scroll->max * tsc->unit);
            tsc->trackPos = tsc->val * (scroll->page - tsc->trackSize) / scroll->max;
            tsc->max = scroll->max - scroll->page;
            tsc->page = scroll->page;
        }
        GetWindowRect(hWnd, &r);
        SendMessage(hWnd, WM_NCCALCSIZE, 0, (LPARAM) &r);
        MoveWindow(frame->hWnd, 0, 0, r.right - r.left, r.bottom - r.top, 1);
        SendMessage(hWnd, WM_NCPAINT, 1, 0);
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
            if(frame->flags & CFRAMEF_TOPBORDER)
            {
                rc.top = r.top + p.y;
                rc.bottom = rc.top + (frame->rcLast.bottom - frame->rcLast.top);
                rc.left = r.left + p.x + offX - offX * (frame->rcLast.right - frame->rcLast.left) / (r.right - r.left);
                rc.right = rc.left + (frame->rcLast.right - frame->rcLast.left);
                offX = offX * (frame->rcLast.right - frame->rcLast.left) / (r.right - r.left) - p.x;
            }
            else
            {
                rc = (RECT) { r.left + p.x, r.top + p.y, p.x + r.right, p.y + r.bottom };
                GetCursorPos(&p);
                ditch = !p.y;
            }
            frame->flags = 0;
            break;
        case HTTOP:         rc = (RECT) { r.left,                           min(r.top + p.y, r.bottom - 50), r.right,                r.bottom }; break;
        case HTBOTTOM:      rc = (RECT) { r.left,                           r.top,                           r.right,                r.top + max(p.y, 50) }; break;
        case HTLEFT:        rc = (RECT) { min(r.left + p.x, r.right - 100), r.top,                           r.right,                r.bottom }; break;
        case HTRIGHT:       rc = (RECT) { r.left,                           r.top,                           r.left + max(p.x, 100), r.bottom }; break;
        case HTTOPLEFT:     rc = (RECT) { min(r.left + p.x, r.right - 100), min(r.top + p.y, r.bottom - 50), r.right,                r.bottom }; break;
        case HTTOPRIGHT:    rc = (RECT) { r.left,                           min(r.top + p.y, r.bottom - 50), r.left + max(p.x, 100), r.bottom }; break;
        case HTBOTTOMLEFT:  rc = (RECT) { min(r.left + p.x, r.right - 100), r.top,                           r.right,                r.top + max(p.y, 50) }; break;
        case HTBOTTOMRIGHT: rc = (RECT) { r.left,                           r.top,                           r.left + max(p.x, 100), r.top + max(p.y, 50) }; break;
        case HTHSCROLL:
            SendMessage(frame->hWnd, WM_HSCROLL, MAKEWPARAM(SB_THUMBTRACK, p.x * frame->scrolls->h.max / (frame->scrolls->h.page - frame->scrolls->h.trackSize)), 0);
            return 0;
        case HTVSCROLL:
            SendMessage(frame->hWnd, WM_VSCROLL, MAKEWPARAM(SB_THUMBTRACK, p.y * frame->scrolls->v.max / (frame->scrolls->v.page - frame->scrolls->v.trackSize)), 0);
            return 0;
        }
        rc.right -= rc.left;
        rc.bottom -= rc.top;
        MoveWindow(hWnd, rc.left, rc.top, rc.right, rc.bottom, 1);
        return 0;
    case WM_LBUTTONUP:
        if(ditch)
        {
            frame->flags |= CFRAMEF_MAXIMIZED;
            MoveWindow(hWnd, 0, 0, 1920, 1040, 1);
        }
        ditch = 0;
        ht = -1;
        ReleaseCapture();
        return 0;
    }
    return 0;
}

static struct cscrolls scrolls;
LRESULT CALLBACK IVProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HDC imageDc;
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
        SelectObject(imageDc, LoadImage(NULL, "TestImage.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE));
        ReleaseDC(hWnd, hdc);
        return 0;
    case WM_SIZE:
        scroll.page = HIWORD(lParam);
        scroll.max = 1080;
        scroll.unit = 1;
        scroll.val = scrolls.v.val;
        SendMessage(GetParent(hWnd), CSBM_SETSCROLLINFO, 0, (LPARAM) &scroll);
        GetClientRect(hWnd, &r);
        scroll.page = r.right;
        scroll.max = 1920;
        scroll.val = scrolls.h.val;
        SendMessage(GetParent(hWnd), CSBM_SETSCROLLINFO, 1, (LPARAM) &scroll);
        return 0;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        GetClientRect(hWnd, &r);
        BitBlt(hdc, 0, 0, r.right, r.bottom, imageDc, scrolls.h.val, scrolls.v.val, SRCCOPY);
        EndPaint(hWnd, &ps);
        return 0;
    case WM_VSCROLL:
        newVScroll = scrolls.v.val;
        switch(LOWORD(wParam))
        {
        case SB_PAGEDOWN: newVScroll -= scrolls.v.page; break;
        case SB_PAGEUP: newVScroll += scrolls.v.page; break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            newVScroll = HIWORD(wParam);
            break;
        }
        newVScroll = max(newVScroll, 0);
        newVScroll = min(newVScroll, scrolls.v.max);
        if(newVScroll == scrolls.v.val)
            return 0;
        ScrollWindow(hWnd, 0, -(newVScroll - scrolls.v.val), NULL, NULL);
        SendMessage(GetParent(hWnd), CSBM_SETPOS, 0, newVScroll);
        return 0;
    case WM_HSCROLL:
        newHScroll = scrolls.h.val;
        switch(LOWORD(wParam))
        {
        case SB_PAGEDOWN: newHScroll -= scrolls.h.page; break;
        case SB_PAGEUP: newHScroll += scrolls.h.page; break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            newHScroll = HIWORD(wParam);
            break;
        }
        newHScroll = max(newHScroll, 0);
        newHScroll = min(newHScroll, scrolls.h.max);
        if(newHScroll == scrolls.h.val)
            return 0;
        ScrollWindow(hWnd, -(newHScroll - scrolls.h.val), 0, NULL, NULL);
        SendMessage(GetParent(hWnd), CSBM_SETPOS, 1, newHScroll);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int main(void)
{
    WNDCLASS wc = {0};
    wc.lpszClassName = "MainWindow";
    wc.lpfnWndProc = Proc;
    wc.style = CS_OWNDC;
    RegisterClass(&wc);
    wc.style = 0;
    wc.lpszClassName = "ImageView";
    wc.lpfnWndProc = IVProc;
    RegisterClass(&wc);

    BkrBrush = CreateSolidBrush(0x352A00);
    BkrCmplyBrush = CreateSolidBrush(0x000B35);
    FgrBrush = CreateSolidBrush(0x5B4900);
    FgrCmplyBrush = CreateSolidBrush(0x00125B);
    DarkBkrBrush = CreateSolidBrush(0x241D00);
    PenClrBrush = CreateSolidBrush(0x333300);

    struct cframe *frame = malloc(sizeof(*frame));
    struct cframecr creation;
    creation.className = "ImageView";
    creation.text = "TestImage.bmp";
    creation.lpVoidParam = NULL;
    creation.flags = 0;
    creation.hMenu = NULL;
    frame->creation = &creation;
    frame->scrolls = &scrolls;

    frame->flags |= CFRAMEF_MAXIMIZED;
    HWND hWnd = CreateWindow("MainWindow", "Title", WS_POPUP | WS_VISIBLE, 30, 30, 300, 300,
                             NULL, NULL, NULL, frame);
    UpdateWindow(hWnd);
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
