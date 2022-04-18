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

    ScrollWindow(frame->hWnd, -(hScroll - frame->scroll->horz), 0, NULL, NULL);
    frame->scroll->horz = hScroll;
    // moving scroll bar
    GetWindowRect(hWnd, &rc);
    r = rc;
    SendMessage(hWnd, WM_NCCALCSIZE, 0, (LPARAM) &r);
    r.bottom -= rc.top;
    r.top = r.bottom;
    r.bottom += CSCROLL_SIZE;
    x = frame->scroll->x;
    x *= -1;
    x += hScroll * (r.right - r.left - frame->scroll->width) / frame->scroll->maxHorz;

    r.right -= rc.left;
    r.left -= rc.left;

    hdc = GetDCEx(hWnd, NULL, DCX_WINDOW);

    ScrollDC(hdc, x, 0, &r, &r, NULL, &r);
    FillRect(hdc, &r, DarkBkrBrush);
    frame->scroll->x += x;

    ReleaseDC(hWnd, hdc);
}

void CSetVScroll2(HWND hWnd, short vScroll)
{
    RECT r, rc;
    int y;
    HDC hdc;
    struct cframe *frame = (struct cframe*) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    vScroll = max(0, vScroll);
    vScroll = min(frame->scroll->maxVert, vScroll);
    if(vScroll == frame->scroll->vert)
        return;

    ScrollWindow(frame->hWnd, 0, -(vScroll - frame->scroll->vert), NULL, NULL);
    frame->scroll->vert = vScroll;
    // moving scroll bar
    GetWindowRect(hWnd, &rc);
    r = rc;
    SendMessage(hWnd, WM_NCCALCSIZE, 0, (LPARAM) &r);
    r.right -= rc.left;
    r.left = r.right;
    r.right += CSCROLL_SIZE;
    y = frame->scroll->y;
    y *= -1;
    y += vScroll * (r.bottom - r.top - frame->scroll->height) / frame->scroll->maxVert;

    r.bottom -= rc.top;
    r.top -= rc.top;

    hdc = GetDCEx(hWnd, NULL, DCX_WINDOW);

    ScrollDC(hdc, 0, y, &r, &r, NULL, &r);
    FillRect(hdc, &r, DarkBkrBrush);
    frame->scroll->y += y;

    ReleaseDC(hWnd, hdc);
}

#define CIDT_LHSCROLL 3
#define CIDT_RHSCROLL 4
#define CIDT_LVSCROLL 5
#define CIDT_RVSCROLL 6

#define CAUTOSCROLL_FACT 20

#define CDELTATIME_SCROLL 100

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
    char *textBuf;
    int len;
    //fprintf(stderr, "%d\n", msg);
    switch(msg)
    {
    case WM_CREATE:
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) (frame = ((CREATESTRUCT*) lParam)->lpCreateParams));
        SetWindowText(hWnd, crstrcpy(((CREATESTRUCT*) lParam)->lpszName));
        SetParent(frame->hWnd, hWnd);
        ShowWindow(frame->hWnd, 1);
        return 0;
    case WM_TIMER:
        GetCursorPos(&p);
        ScreenToClient(hWnd, &p);
        switch(wParam)
        {
        case CIDT_LHSCROLL: if(p.x >= frame->scroll->x) KillTimer(hWnd, CIDT_LHSCROLL); else CSetHScroll2(hWnd, frame->scroll->horz - max(frame->scroll->maxHorz / 20, 1)); break;
        case CIDT_RHSCROLL: if(p.x < frame->scroll->x + frame->scroll->width) KillTimer(hWnd, CIDT_RHSCROLL); else CSetHScroll2(hWnd, frame->scroll->horz + max(frame->scroll->maxHorz / 20, 1)); break;
        case CIDT_LVSCROLL: if(p.y >= frame->scroll->y) KillTimer(hWnd, CIDT_LVSCROLL); else CSetVScroll2(hWnd, frame->scroll->vert - max(frame->scroll->maxVert / 20, 1)); break;
        case CIDT_RVSCROLL: if(p.y < frame->scroll->y + frame->scroll->height) KillTimer(hWnd, CIDT_RVSCROLL); else CSetVScroll2(hWnd, frame->scroll->vert + max(frame->scroll->maxVert / 20, 1)); break;
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
            SendMessage(hWnd, WM_SIZE, 0, MAKELPARAM(r.right - r.left, r.bottom - r.top));
        }
        return 0;
    case WM_SYNCPAINT: // TODO: I don't know how to handle this yet, is it even necessary?
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
        if(frame)
        {
            if(frame->scroll->maxHorz)
                nccp->rgrc[0].bottom -= CSCROLL_SIZE;
            if(frame->scroll->maxVert)
                nccp->rgrc[0].right -= CSCROLL_SIZE;
        }
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
    case WM_NCLBUTTONDBLCLK:
        switch(wParam)
        {
        case HTHSCROLL:
            p.x = GET_X_LPARAM(lParam);
            p.y = GET_Y_LPARAM(lParam);
            ScreenToClient(hWnd, &p);
            GetWindowRect(hWnd, &r);
            if(p.x < frame->scroll->x)
            {
                CSetHScroll2(hWnd, frame->scroll->horz - max(frame->scroll->maxHorz / CAUTOSCROLL_FACT, 1));
                SetTimer(hWnd, CIDT_LHSCROLL, CDELTATIME_SCROLL, NULL);
            }
            else if(p.x >= frame->scroll->x + frame->scroll->width)
            {
                CSetHScroll2(hWnd, frame->scroll->horz + max(frame->scroll->maxHorz / CAUTOSCROLL_FACT, 1));
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
            if(p.y < frame->scroll->y)
            {
                CSetVScroll2(hWnd, frame->scroll->vert - max(frame->scroll->maxVert / CAUTOSCROLL_FACT, 1));
                SetTimer(hWnd, CIDT_LVSCROLL, CDELTATIME_SCROLL, NULL);
            }
            else if(p.y >= frame->scroll->y + frame->scroll->height)
            {
                CSetVScroll2(hWnd, frame->scroll->vert + max(frame->scroll->maxVert / CAUTOSCROLL_FACT, 1));
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
    case WM_MOUSEMOVE:
        if(ht < 0)
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
        case HTHSCROLL:
            GetClientRect(hWnd, &rc);
            CSetHScroll2(hWnd, p.x * frame->scroll->maxHorz / (rc.right - frame->scroll->width));
            return 0;
        case HTVSCROLL:
            GetClientRect(hWnd, &rc);
            CSetVScroll2(hWnd, p.y * frame->scroll->maxVert / (rc.bottom - frame->scroll->height));
            return 0;
        }
        rc.right -= rc.left;
        rc.bottom -= rc.top;
        MoveWindow(hWnd, rc.left, rc.top, rc.right, rc.bottom, 1);
        return 0;
    case WM_LBUTTONUP:
        ht = -1;
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
    frame->hWnd = CreateWindow("Edit", "YO", WS_VISIBLE | WS_POPUP, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    frame->scroll = malloc(sizeof(*frame->scroll));
    frame->scroll->maxHorz = 300;
    frame->scroll->horz = 0;
    frame->scroll->width = 80;
    frame->scroll->x = 0;

    frame->scroll->maxVert = 300;
    frame->scroll->vert = 0;
    frame->scroll->height = 80;
    frame->scroll->y = 0;
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
