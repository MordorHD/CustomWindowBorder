#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <windowsx.h>

#define CSCROLL_SIZE 18
#define crstrcpy(str) (strcpy(malloc(strlen(str) + 1), (str)))
HBRUSH BkrBrush;
HBRUSH DarkBkrBrush;
HBRUSH PenClrBrush;

struct cscroll {
    int y, height, vert, maxVert; // vertical scroll bar, unit of "y" is pixels and "v" is lines
    int x, width, horz, maxHorz; // horizontal scroll bar, unit of "x" is pixels and "h" is also pixels
};

struct cframe {
    HWND hWnd;
    char *title;
    struct cscroll *scroll;
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

void CSetHScroll2(HWND hWnd, short hScroll)
{
    RECT r, rc;
    int x;
    HDC hdc;
    struct cframe *frame = (struct cframe*) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    hScroll = max(0, hScroll);
    hScroll = min(frame->scroll->maxHorz, hScroll);
    if(hScroll == frame->scroll->horz)
        return;

    hdc = GetDC(hWnd);

    GetClientRect(hWnd, &rc);

    ScrollWindow(frame->hWnd, -(hScroll - frame->scroll->horz), 0, NULL, NULL);
    frame->scroll->horz = hScroll;
    // moving scroll bar
    r = rc;
    r.top = r.bottom - CSCROLL_SIZE;
    x = frame->scroll->x;
    x *= -1;
    x += hScroll * (r.right - frame->scroll->width) / frame->scroll->maxHorz;
    ScrollDC(hdc, x, 0, &r, &r, NULL, &r);
    FillRect(hdc, &r, DarkBkrBrush);
    frame->scroll->x += x;

    ReleaseDC(hWnd, hdc);
}

LRESULT CALLBACK Proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int ht;
    static int offX, offY;

    PAINTSTRUCT ps;
    HDC hdc;
    POINT p;
    RECT r, rc, *pr;
    WINDOWPOS *wp;
    MINMAXINFO *mmi;
    NCCALCSIZE_PARAMS *nccp;
    struct cframe *frame = (struct cframe*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    int dx, dy;
    fprintf(stderr, "%d\n", msg);
    switch(msg)
    {
    case WM_CREATE:
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) (frame = ((CREATESTRUCT*) lParam)->lpCreateParams));
        SetWindowText(hWnd, crstrcpy(((CREATESTRUCT*) lParam)->lpszName));
        SetParent(frame->hWnd, hWnd);
        ShowWindow(frame->hWnd, 1);
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
            SendMessage(hWnd, WM_SIZE, 0, MAKELPARAM(r.right - r.left, r.bottom - r.top));
        }
        return 0;
    case WM_SYNCPAINT:
        return DefWindowProc(hWnd, msg, wParam, lParam);
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
    case WM_NCCALCSIZE:
        nccp = (NCCALCSIZE_PARAMS*) lParam;
        nccp->rgrc[0].left += 8;
        nccp->rgrc[0].top += 30;
        nccp->rgrc[0].right -= 8;
        nccp->rgrc[0].bottom -= 8;
        return 0;
    case WM_NCPAINT:
        hdc = GetDCEx(hWnd, (HRGN) wParam, DCX_WINDOW | DCX_INTERSECTRGN);

        GetWindowRect(hWnd, &rc);
        rc.right -= rc.left;
        rc.bottom -= rc.top;
        rc.left = 0;
        rc.top = 0;
        // north
        r.left = rc.left;
        r.top = rc.top;
        r.right = rc.right;
        r.bottom = r.top + 30;
        FillRect(hdc, &r, PenClrBrush);
        // draw title
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, DefaultPalette[CCOLOR_ALPHA]);
        TextOut(hdc, r.left + 8, r.top + 8, frame->title , strlen(frame->title ));
        // south
        r.top = rc.bottom - 8;
        r.bottom = rc.bottom + 1;
        FillRect(hdc, &r, PenClrBrush);
        // draw horizontal scroll bar
        if(frame->scroll->maxHorz)
        {
            r.left += 8;
            r.right -= 8;
            r.bottom = r.top;
            r.top = r.bottom - CSCROLL_SIZE;
            FillRect(hdc, &r, DarkBkrBrush);
            r.top++;
            r.bottom--;
            r.left += frame->scroll->x;
            r.right = r.left + frame->scroll->width;
            FillRect(hdc, &r, BkrBrush);
        }
        // east
        r.left = rc.left;
        r.top = rc.top + 30;
        r.right = r.left + 9;
        r.bottom = rc.bottom - 8;
        FillRect(hdc, &r, PenClrBrush);
        // west
        r.left = rc.right - 8;
        r.right = rc.right;
        FillRect(hdc, &r, PenClrBrush);
        if(frame->scroll->maxVert)
        {
            r.left -= CSCROLL_SIZE;
            r.right -= 8;
            FillRect(hdc, &r, DarkBkrBrush);
            r.left++;
            r.right--;
            r.top    = frame->scroll->y + 30;
            r.bottom = r.top + frame->scroll->height;
            FillRect(hdc, &r, BkrBrush);
        }
        ReleaseDC(hWnd, hdc);
        return 0;
    case WM_NCHITTEST:
        p.x = GET_X_LPARAM(lParam);
        p.y = GET_Y_LPARAM(lParam);
        GetWindowRect(hWnd, &r);
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
        if(frame->scroll->maxHorz && p.y >= r.bottom - 8 - CSCROLL_SIZE)
        {
            offX = p.x - r.left - frame->scroll->x;
            return HTHSCROLL;
        }
        if(frame->scroll->maxVert && p.x >= r.right - 8 - CSCROLL_SIZE)
        {
            offY = p.y - r.top - frame->scroll->y;
            return HTVSCROLL;
        }
        return HTCLIENT;
    case WM_NCLBUTTONDOWN:
        SetCapture(hWnd);
        SetFocus(frame->hWnd);
        ht = wParam;
        return 0;
    case WM_NCLBUTTONDBLCLK:
        return 0;
    case WM_NCLBUTTONUP:
        return 0;
    case WM_NCMOUSEMOVE:
        return 0;
    case WM_MOUSEMOVE:
        if(!IsLButtonDown())
            return 0;
        GetCursorPos(&p);
        GetWindowRect(hWnd, &r);
        p.x -= r.left + offX;
        p.y -= r.top + offY;
        switch(ht)
        {
        case HTBORDER:      rc = (RECT) { r.left + p.x, r.top + p.y, p.x + r.right, p.y + r.bottom }; break;
        case HTTOP:         rc = (RECT) { r.left,                           min(r.top + p.y, r.bottom - 50), r.right,                r.bottom }; break;
        case HTBOTTOM:      rc = (RECT) { r.left,                           r.top,                           r.right,                r.top + max(p.y, 50) }; break;
        case HTLEFT:        rc = (RECT) { min(r.left + p.x, r.right - 100), r.top,                           r.right,                r.bottom }; break;
        case HTRIGHT:       rc = (RECT) { r.left,                           r.top,                           r.left + max(p.x, 100), r.bottom }; break;
        case HTTOPLEFT:     rc = (RECT) { min(r.left + p.x, r.right - 100), min(r.top + p.y, r.bottom - 50), r.right,                r.bottom }; break;
        case HTTOPRIGHT:    rc = (RECT) { r.left,                           min(r.top + p.y, r.bottom - 50), r.left + max(p.x, 100), r.bottom }; break;
        case HTBOTTOMLEFT:  rc = (RECT) { min(r.left + p.x, r.right - 100), r.top,                           r.right,                r.top + max(p.y, 50) }; break;
        case HTBOTTOMRIGHT: rc = (RECT) { r.left,                           r.top,                           r.left + max(p.x, 100), r.top + max(p.y, 50) }; break;
        case HTHSCROLL: SendMessage(frame->hWnd, WM_HSCROLL, MAKEWPARAM(SB_THUMBTRACK, p.x * frame->scroll->maxHorz / (r.right - r.left - 16 - frame->scroll->width)), (LPARAM) hWnd); return 0;
        case HTVSCROLL: SendMessage(frame->hWnd, WM_VSCROLL, MAKEWPARAM(SB_THUMBTRACK, p.y * frame->scroll->maxVert / (r.bottom - r.top - 38 - frame->scroll->height)), (LPARAM) hWnd); return 0;
        }
        rc.right -= rc.left;
        rc.bottom -= rc.top;
        MoveWindow(hWnd, rc.left, rc.top, rc.right, rc.bottom, 1);
        return 0;
    case WM_LBUTTONUP:
        ReleaseCapture();
        return 0;
    }
    return 0;
}


int main(void)
{
    WNDCLASS wc = {0};
    wc.lpszClassName = "MainWindow";
    wc.lpfnWndProc = Proc;
    wc.style = CS_OWNDC;
    RegisterClass(&wc);

    BkrBrush = CreateSolidBrush(0x352A00);
    DarkBkrBrush = CreateSolidBrush(0x241D00);
    PenClrBrush = CreateSolidBrush(0x333300);

    struct cframe *frame = malloc(sizeof(*frame));
    frame->hWnd = CreateWindow("Button", "YO", WS_VISIBLE | WS_POPUP, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    frame->scroll = malloc(sizeof(*frame->scroll));
    frame->scroll->maxVert = frame->scroll->maxHorz = 0;
    HWND hWnd = CreateWindow("MainWindow", "Title", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 30, 30, 300, 300,
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
