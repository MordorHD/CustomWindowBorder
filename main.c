#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <windowsx.h>
#include <stdbool.h>
#include "msgtostr.h"

#define CFRAME_MINWIDTH 140
#define CFRAME_MINHEIGHT 80
#define CSCROLL_MINSIZE 10
#define CSCROLL_SIZE 13
#define SIZE_SCROLL 5 // size message was sent from scroll bars

struct cscroll {
    RECT rcTrack;
    RECT rcThumb;
    int page;
    int pos, max;
};

struct cframe {
    RECT rcClient;
    RECT rcWindow;
    RECT rcIcon, rcClose, rcMin, rcMax;
    RECT rcTitle;
    char *title;
    HICON hIcon;
    HBRUSH hbrFrame;
    HBRUSH hbrOutline;
    HBRUSH hbrClose, hbrShadowClose;
    HBRUSH hbrButtons, hbrShadowButtons;
    HBRUSH hbrTrack, hbrThumb;
    COLORREF clrCaption;
    struct cscroll vert, horz;
    LPVOID lpVoid;
};

HWND CCreateFrame(LPCSTR className, LPCSTR title, HICON hIcon, int x, int y, int width, int height, LPVOID lpVoid)
{
    HWND hWnd;
    struct cframe *frame;
    frame = malloc(sizeof(*frame));
    frame->title = strcpy(malloc(strlen(title) + 1), title);
    frame->hIcon = hIcon;
    frame->lpVoid = lpVoid;

    frame->vert.pos = 0;
    frame->vert.max = 2000;
    frame->vert.page = 800;

    frame->horz.pos = 0;
    frame->horz.max = 2000;
    frame->horz.page = 800;

    frame->hbrFrame = CreateSolidBrush(0x333300); // 333300
    frame->hbrOutline = CreateSolidBrush(0x00125B); // 0x00125B
    frame->hbrTrack = CreateSolidBrush(0x241D00);
    frame->hbrClose = CreateSolidBrush(0x0000FF);
    frame->hbrShadowClose = CreateSolidBrush(0x0000AA);
    frame->hbrButtons = CreateSolidBrush(0x9F6060);
    frame->hbrShadowButtons = CreateSolidBrush(0x6C4141);
    frame->hbrThumb = CreateSolidBrush(0x5B4900);
    frame->clrCaption = 0x969483;
    hWnd = CreateWindow(className, title, WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | WS_BORDER | WS_THICKFRAME | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU, x, y, width, height, NULL, NULL, NULL, frame);
    return hWnd;
}

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

LRESULT CFrameNCCalcSize(HWND hWnd, struct cframe *frame, RECT *rect, WPARAM wParam)
{
    RECT last;
    LONG style;
    LONG exStyle;

    if(!rect)
        return 0;

    frame->rcWindow = *rect;

    style = GetWindowLong(hWnd, GWL_STYLE);
    exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);

    if(!(style & WS_MINIMIZE))
    {
        if(style & WS_THICKFRAME)
            InflateRect(rect, -8, -8); // TODO: MAGIC NUMBERS
        else if((exStyle & WS_EX_STATICEDGE) || (style & WS_BORDER))
            InflateRect(rect, -1, -1);

        if((style & WS_CAPTION) == WS_CAPTION)
        {
            last.top = rect->top - frame->rcWindow.top;
            last.left = rect->right - frame->rcWindow.left;
            if(exStyle & WS_EX_TOOLWINDOW)
                rect->top += GetSystemMetrics(SM_CYSMCAPTION);
            else
                rect->top += GetSystemMetrics(SM_CYCAPTION);
            last.bottom = rect->top - frame->rcWindow.top;
            frame->rcIcon = (RECT) { rect->left - frame->rcWindow.left, last.top, rect->left - frame->rcWindow.left + 16, last.top + 16};
            frame->rcClose = (RECT) { last.left - 34, last.top, last.left, last.bottom - 4 };
            last.left -= 36;
        }

        if(style & WS_MAXIMIZEBOX)
        {
            frame->rcMax = (RECT) { last.left - 34, last.top, last.left, last.bottom - 4 };
            last.left -= 36;
        }
        if(style & WS_MINIMIZEBOX)
        {
            frame->rcMin = (RECT) { last.left - 34, last.top, last.left, last.bottom - 4 };
            last.left -= 36;
        }

        if((style & WS_CAPTION) == WS_CAPTION)
            frame->rcTitle = (RECT) { frame->rcIcon.right + 4, last.top, last.left, last.bottom };

        if(exStyle & WS_EX_CLIENTEDGE)
            InflateRect(rect, -2 * GetSystemMetrics(SM_CXBORDER), -2 * GetSystemMetrics(SM_CYBORDER));

        if(style & WS_VSCROLL)
        {
            if(rect->right - rect->left >= GetSystemMetrics(SM_CXVSCROLL))
            {
                frame->vert.rcTrack = (RECT) { rect->right - GetSystemMetrics(SM_CXVSCROLL), rect->top, rect->right, rect->bottom };
                OffsetRect(&frame->vert.rcTrack, -frame->rcWindow.left, -frame->rcWindow.top);

                if(!wParam && (exStyle & WS_EX_LAYOUTRTL))
                    exStyle ^= WS_EX_LEFTSCROLLBAR;

                if(exStyle & WS_EX_LEFTSCROLLBAR)
                    rect->left  += GetSystemMetrics(SM_CXVSCROLL);
                else
                    rect->right -= GetSystemMetrics(SM_CXVSCROLL);
            }
        }

        if(style & WS_HSCROLL)
        {
            if(rect->bottom - rect->top > GetSystemMetrics(SM_CYHSCROLL))
            {
                frame->horz.rcTrack = (RECT) { rect->left, rect->bottom - GetSystemMetrics(SM_CYHSCROLL), rect->right, rect->bottom };
                OffsetRect(&frame->horz.rcTrack, -frame->rcWindow.left, -frame->rcWindow.top);
                frame->vert.rcTrack.bottom -= GetSystemMetrics(SM_CYHSCROLL);

                rect->bottom -= GetSystemMetrics(SM_CYHSCROLL);
            }
        }

        if(rect->top > rect->bottom)
            rect->bottom = rect->top;

        if(rect->left > rect->right)
            rect->right = rect->left;
    }
    else
    {
        rect->right = rect->left;
        rect->bottom = rect->top;
    }
    frame->rcClient = *rect;
    OffsetRect(&frame->rcClient, -frame->rcWindow.left, -frame->rcWindow.top);
    return 0;
}

LRESULT CFrameHitTest(HWND hWnd, struct cframe *frame, POINT p)
{
    POINT relativePoint;
    RECT r;
    LONG style;
    relativePoint = p;
    GetWindowRect(hWnd, &r);
    style = GetWindowLong(hWnd, GWL_STYLE);
    relativePoint.x -= r.left;
    relativePoint.y -= r.top;
    if(PtInRect(&frame->rcClose, relativePoint))
        return HTCLOSE;
    if(PtInRect(&frame->rcMin, relativePoint))
        return HTMINBUTTON;
    if(PtInRect(&frame->rcMax, relativePoint))
        return HTMAXBUTTON;
    if(PtInRect(&frame->rcIcon, relativePoint))
        return HTCAPTION;
    if((style & WS_VSCROLL) && PtInRect(&frame->vert.rcTrack, relativePoint))
        return HTVSCROLL;
    if((style & WS_HSCROLL) && PtInRect(&frame->horz.rcTrack, relativePoint))
        return HTHSCROLL;
    if(style & WS_THICKFRAME)
    {
        if(p.y < r.top + 8)
        {
            if(p.x < r.left + 8)
                return HTTOPLEFT;
            if(p.x >= r.right - 8)
                return HTTOPRIGHT;
            return HTTOP;
        }
        if(p.y >= r.bottom - 8)
        {
            if(p.x < r.left + 8)
                return HTBOTTOMLEFT;
            if(p.x >= r.right - 8)
                return HTBOTTOMRIGHT;
            return HTBOTTOM;
        }
        if(p.x < r.left + 8)
            return HTLEFT;
        if(p.x >= r.right - 8)
            return HTRIGHT;
        if((style & WS_CAPTION) && p.y <= r.top + 30)
            return HTCAPTION;
    }
    else
    {
        if((style & WS_CAPTION) && p.y <= r.top + 22)
            return HTCAPTION;
    }
    return HTCLIENT;
}

void CFrameButton(HWND hWnd, struct cframe *frame, WPARAM wParam, LPARAM lParam)
{
    MSG msg;
    HDC hdc;
    BOOL pressed = TRUE;
    WPARAM scMsg;
    HMENU sysMenu;
    UINT menuState;
    POINT p;
    LONG style;

    HBRUSH shadow;
    HBRUSH bkr;
    RECT *rect;

    style = GetWindowLong(hWnd, GWL_STYLE);

    switch(wParam)
    {
    case HTCLOSE:
        // TODO: relevant?
        sysMenu = GetSystemMenu(hWnd, FALSE);
        menuState = GetMenuState(sysMenu ? sysMenu : NULL, SC_CLOSE, MF_BYCOMMAND); /* in case of error MenuState==0xFFFFFFFF */
        if(!(style & WS_SYSMENU) || (menuState & (MF_GRAYED | MF_DISABLED)))
            return;
        scMsg = SC_CLOSE;
        rect = &frame->rcClose;
        shadow = frame->hbrShadowClose;
        bkr = frame->hbrClose;
        break;
    case HTMINBUTTON:
        if(!(style & WS_MINIMIZEBOX))
            return;
        scMsg = ((style & WS_MINIMIZE) ? SC_RESTORE : SC_MINIMIZE);
        rect = &frame->rcMin;
        shadow = frame->hbrShadowButtons;
        bkr = frame->hbrButtons;
        break;
    case HTMAXBUTTON:
        if(!(style & WS_MAXIMIZEBOX))
            return;
        scMsg = (style & WS_MAXIMIZE) ? SC_RESTORE : SC_MAXIMIZE;
        rect = &frame->rcMax;
        shadow = frame->hbrShadowButtons;
        bkr = frame->hbrButtons;
        break;
    default:
        // should never happen
        return;
    }

    /*
    * FIXME: Not sure where to do this, but we must flush the pending
    * window updates when someone clicks on the close button and at
    * the same time the window is overlapped with another one. This
    * looks like a good place for now...
    */
    // TODO: ?? co_IntUpdateWindows(pWnd, RDW_ALLCHILDREN, FALSE);
    RedrawWindow(hWnd, NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW);

    hdc = GetDCEx(hWnd, NULL, DCX_WINDOW);
    Draw3DButton(hdc, pressed, 4, rect, frame->hbrFrame, bkr, shadow);

    SetCapture(hWnd);

    while(GetMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST))
    {
        /* TODO: WHEN IS THIS NEEDED AND WHY? if(CallMsgFilter(&msg, MSGF_MAX))
            continue;*/
        if(msg.message == WM_LBUTTONUP)
            break;
        if(msg.message != WM_MOUSEMOVE)
        {
            // TODO: is this really necessary?
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        p = msg.pt;
        p.x -= frame->rcWindow.left;
        p.y -= frame->rcWindow.top;
        if(pressed != PtInRect(rect, p))
        {
            pressed = !pressed;
            Draw3DButton(hdc, pressed, 4, rect, frame->hbrFrame, bkr, shadow);
        }
    }
    Draw3DButton(hdc, 0, 4, rect, frame->hbrFrame, bkr, shadow);
    ReleaseCapture();
    ReleaseDC(hWnd, hdc);

    if(pressed)
        SendMessage(hWnd, WM_SYSCOMMAND, scMsg, scMsg == SC_CLOSE ? lParam : MAKELONG(msg.pt.x, msg.pt.y));
}

LRESULT CFrameLButtonHScroll(HWND hWnd, struct cframe *frame, LPARAM lParam);
LRESULT CFrameLButtonVScroll(HWND hWnd, struct cframe *frame, LPARAM lParam);

LRESULT CFrameNCLButtonDown(HWND hWnd, struct cframe *frame, WPARAM wParam, LPARAM lParam)
{
    switch(wParam)
    {
    case HTCAPTION:
        /* TODO: WHAT IS THIS FOR?? while(1)
        {
            if((GetWindowStyle(top) & (WS_POPUP | WS_CHILD)) != WS_CHILD)
                break;
            parent = GetAncestor(top, GA_PARENT);
            if(!parent)
                break;
            top = parent;
        }*/
        if(SetForegroundWindow(hWnd) || GetActiveWindow() == hWnd)
            SendMessage(hWnd, WM_SYSCOMMAND, SC_MOVE + HTCAPTION, lParam);
        break;
    case HTSYSMENU:
        // TODO: SYSMENU
        break;
    case HTMENU:
        SendMessage(hWnd, WM_SYSCOMMAND, SC_MOUSEMENU + HTMENU, lParam);
        break;
    case HTHSCROLL:
        return CFrameLButtonHScroll(hWnd, frame, lParam);
    case HTVSCROLL:
        return CFrameLButtonVScroll(hWnd, frame, lParam);
    case HTMINBUTTON:
    case HTMAXBUTTON:
    case HTCLOSE:
        CFrameButton(hWnd, frame, wParam, lParam);
        break;
    case HTLEFT:
    case HTRIGHT:
    case HTTOP:
    case HTBOTTOM:
    case HTTOPLEFT:
    case HTTOPRIGHT:
    case HTBOTTOMLEFT:
    case HTBOTTOMRIGHT:
        SendMessage(hWnd, WM_SYSCOMMAND, SC_SIZE + wParam - (HTLEFT - WMSZ_LEFT), lParam);
        break;
    case HTBORDER:
        break;
    }
    return 0;
}

LRESULT CFrameNCLButtonDblClk(HWND hWnd, struct cframe *frame, WPARAM wParam, LPARAM lParam)
{
    LONG style;
    LONG exStyle;
    RECT mouseRect;

    style = GetWindowLong(hWnd, GWL_STYLE);
    exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
    switch(wParam)
    {
    case HTCAPTION:
        if((style & WS_CAPTION) == WS_CAPTION && (style & WS_MAXIMIZEBOX))
            SendMessage(hWnd, WM_SYSCOMMAND, (style & (WS_MINIMIZE | WS_MAXIMIZE)) ? SC_RESTORE : SC_MAXIMIZE, 0);
        break;
    case HTSYSMENU:
        // TODO: HTSYSMENU
        break;
    case HTTOP:
    case HTBOTTOM:
        if(exStyle & WS_EX_MDICHILD)
            break;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &mouseRect, 0);
        SetWindowPos(hWnd, HWND_TOP, frame->rcWindow.left, mouseRect.top, frame->rcWindow.right - frame->rcWindow.left, mouseRect.bottom - mouseRect.top, 0);
        break;
    default:
        return CFrameNCLButtonDown(hWnd, frame, wParam, lParam);
    }
    return 0;
}

LRESULT CFrameLButtonVScroll(HWND hWnd, struct cframe *frame, LPARAM lParam)
{
    MSG msg;
    BOOL hitThumb;
    int offsetY; // offset from thumb
    POINT p;
    int sMsg;
    p = (POINT) { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    p.x -= frame->rcWindow.left;
    p.y -= frame->rcWindow.top;
    if(PtInRect(&frame->vert.rcThumb, p))
    {
        hitThumb = 1;
        offsetY = p.y - frame->vert.rcThumb.top;
    }
    else
    {
        sMsg = p.y < frame->vert.rcThumb.top ? SB_PAGEUP : SB_PAGEDOWN;
        SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(sMsg, 0), 0);
    }
    SetCapture(hWnd);
    while(GetMessage(&msg, NULL, 0, 0))
    {
        if(msg.message == WM_LBUTTONUP)
            break;
        if(msg.message != WM_MOUSEMOVE)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        p = msg.pt;
        p.y -= frame->rcWindow.top;
        if(hitThumb)
            SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_THUMBTRACK, (p.y - frame->vert.rcTrack.top - offsetY) * frame->vert.max / (frame->vert.page - frame->vert.rcThumb.bottom + frame->vert.rcThumb.top)), 0);
        else if((sMsg == SB_PAGEUP   && p.y < frame->vert.rcThumb.top)
             || (sMsg == SB_PAGEDOWN && p.y > frame->vert.rcThumb.bottom))
            SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(sMsg, 0), 0);
    }
    ReleaseCapture();
    return 0;
}

LRESULT CFrameLButtonHScroll(HWND hWnd, struct cframe *frame, LPARAM lParam)
{
    MSG msg;
    BOOL hitThumb;
    int offsetX; // offset from thumb
    POINT p;
    int sMsg;
    p = (POINT) { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    p.x -= frame->rcWindow.left;
    p.y -= frame->rcWindow.top;
    if(PtInRect(&frame->horz.rcThumb, p))
    {
        hitThumb = 1;
        offsetX = p.x - frame->horz.rcThumb.left;
    }
    else
    {
        sMsg = p.x < frame->horz.rcThumb.left ? SB_PAGELEFT : SB_PAGERIGHT;
        SendMessage(hWnd, WM_HSCROLL, MAKEWPARAM(sMsg, 0), 0);
    }
    SetCapture(hWnd);
    while(GetMessage(&msg, NULL, 0, 0))
    {
        if(msg.message == WM_LBUTTONUP)
            break;
        if(msg.message != WM_MOUSEMOVE)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        p = msg.pt;
        p.x -= frame->rcWindow.left;
        if(hitThumb)
            SendMessage(hWnd, WM_HSCROLL, MAKEWPARAM(SB_THUMBTRACK, (p.x - frame->horz.rcTrack.left - offsetX) * frame->horz.max / (frame->horz.page - frame->horz.rcThumb.right + frame->horz.rcThumb.left)), 0);
        else if((sMsg == SB_PAGELEFT  && p.x < frame->horz.rcThumb.left)
             || (sMsg == SB_PAGERIGHT && p.x > frame->horz.rcThumb.right))
            SendMessage(hWnd, WM_HSCROLL, MAKEWPARAM(sMsg, 0), 0);
    }
    ReleaseCapture();
    return 0;
}

void CFrameNCPaint(HWND hWnd, struct cframe *frame, HDC hdc, int flags)
{
    RECT rect;
    RECT r;
    LONG style, exStyle;
    rect = frame->rcWindow;
    rect.right -= rect.left;
    rect.bottom -= rect.top;
    rect.left = 0;
    rect.top = 0;
    r = rect;
    style = GetWindowLong(hWnd, GWL_STYLE);
    exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
    if(!(style & WS_CAPTION))
        return;
    if(!(style & WS_THICKFRAME))
    {
        // north
        r.bottom = rect.top + 22;
        FillRect(hdc, &r, frame->hbrFrame);
    }
    else
    {
        // north
        r.bottom = rect.top + 30;
        FillRect(hdc, &r, frame->hbrFrame);
        // south
        r.top = rect.bottom - 8;
        r.bottom = rect.bottom;
        FillRect(hdc, &r, frame->hbrFrame);
        // east
        r.top = rect.top + 30;
        r.right = rect.left + 8;
        r.bottom = rect.bottom - 8;
        FillRect(hdc, &r, frame->hbrFrame);
        // west
        r.left = rect.right - 8;
        r.right = rect.right;
        FillRect(hdc, &r, frame->hbrFrame);
    }
    if((exStyle & WS_EX_STATICEDGE) || (style & WS_BORDER))
        FrameRect(hdc, &rect, frame->hbrOutline);
    // draw icon
    DrawIconEx(hdc, frame->rcIcon.left, frame->rcIcon.top, frame->hIcon, frame->rcIcon.right - frame->rcIcon.left, frame->rcIcon.bottom - frame->rcIcon.top, 0, NULL, DI_NORMAL);
    // draw title
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, frame->clrCaption);
    DrawText(hdc, frame->title, strlen(frame->title), &frame->rcTitle, DT_LEFT | DT_TOP | DT_SINGLELINE);
    // draw scroll bars
    if(style & WS_VSCROLL)
    {
        FillRect(hdc, &frame->vert.rcTrack, frame->hbrTrack);
        FillRect(hdc, &frame->vert.rcThumb, frame->hbrThumb);
        FrameRect(hdc, &frame->vert.rcThumb, frame->hbrOutline);
    }
    if(style & WS_HSCROLL)
    {
        FillRect(hdc, &frame->horz.rcTrack, frame->hbrTrack);
        FillRect(hdc, &frame->horz.rcThumb, frame->hbrThumb);
        FrameRect(hdc, &frame->horz.rcThumb, frame->hbrOutline);
        if(style & WS_VSCROLL)
        {
            r = (RECT) { frame->horz.rcTrack.right, frame->vert.rcTrack.bottom, frame->vert.rcTrack.right, frame->horz.rcTrack.bottom };
            FillRect(hdc, &r, frame->hbrThumb);
        }
    }
    Draw3DButton(hdc, 0, 4, &frame->rcClose, frame->hbrFrame, frame->hbrClose, frame->hbrShadowClose);
    Draw3DButton(hdc, 0, 4, &frame->rcMax, frame->hbrFrame, frame->hbrButtons, frame->hbrShadowButtons);
    Draw3DButton(hdc, 0, 4, &frame->rcMin, frame->hbrFrame, frame->hbrButtons, frame->hbrShadowButtons);
}

LONG CFrameStartMoveSize(HWND hWnd, struct cframe *frame, WPARAM wParam, POINT *capturePoint)
{
    LONG hitTest = 0;
    POINT p;
    MSG msg;
    RECT rect;
    ULONG style;

    rect = frame->rcWindow;
    style = GetWindowLong(hWnd, GWL_STYLE);

    if((wParam & 0xfff0) == SC_MOVE)
    {
        /* Move pointer at the center of the caption */
        if(style & WS_SYSMENU)
            rect.left += GetSystemMetrics(SM_CXSIZE) + 1;
        if(style & WS_MINIMIZEBOX)
            rect.right -= GetSystemMetrics(SM_CXSIZE) + 1;
        if(style & WS_MAXIMIZEBOX)
            rect.right -= GetSystemMetrics(SM_CXSIZE) + 1;
        p.x = (rect.right + rect.left) / 2;
        p.y = rect.top + GetSystemMetrics(SM_CYSIZE) / 2;
        hitTest = HTCAPTION;
        *capturePoint = p;
    }
    else  /* SC_SIZE */
    {
        p.x = p.y = 0;
        while(!hitTest && GetMessage(&msg, NULL, 0, 0))
        {
            switch(msg.message)
            {
            case WM_MOUSEMOVE:
                p.x = min(max(msg.pt.x, rect.left), rect.right - 1 );
                p.y = min(max(msg.pt.y, rect.top), rect.bottom - 1 );
                hitTest = CFrameHitTest(hWnd, frame, p);
                if(hitTest < HTLEFT || hitTest > HTBOTTOMRIGHT)
                    hitTest = 0;
                break;
            case WM_LBUTTONUP:
                return 0;
            case WM_KEYDOWN:
                switch(msg.wParam)
                {
                case VK_UP:
                    hitTest = HTTOP;
                    p.x = (rect.left + rect.right) / 2;
                    p.y = rect.top + GetSystemMetrics(SM_CYFRAME) / 2;
                    break;
                case VK_DOWN:
                    hitTest = HTBOTTOM;
                    p.x = (rect.left + rect.right) / 2;
                    p.y = rect.bottom - GetSystemMetrics(SM_CYFRAME) / 2;
                    break;
                case VK_LEFT:
                    hitTest = HTLEFT;
                    p.x = rect.left + GetSystemMetrics(SM_CXFRAME) / 2;
                    p.y = (rect.top + rect.bottom) / 2;
                    break;
                case VK_RIGHT:
                    hitTest = HTRIGHT;
                    p.x = rect.right - GetSystemMetrics(SM_CXFRAME) / 2;
                    p.y = (rect.top + rect.bottom) / 2;
                    break;
                case VK_RETURN:
                case VK_ESCAPE:
                    return 0;
                }
                break;
            default:
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                break;
            }
        }
        *capturePoint = p;
    }
    SetCursorPos(p.x, p.y);
    SendMessage(hWnd, WM_SETCURSOR, (WPARAM) hWnd, MAKELONG(hitTest, WM_MOUSEMOVE));
    return hitTest;
}

void CFrameMoveSize(HWND hWnd, struct cframe *frame, WPARAM wParam)
{
    LONG hitTest = wParam & 0xF;
    LONG sysCmd = wParam & 0xFFF0;
    LONG style, exStyle;
    BOOL thickFrame;
    BOOL iconic;
    HWND parent = NULL;
    MINMAXINFO mmi;
    MSG msg;
    POINT p;
    POINT capturePoint;
    RECT mouseRect;
    RECT newRect;
    RECT origRect;
    RECT sizingRect;
    int dx, dy;
    BOOL moved = 0;
    BOOL dragFullWindows = FALSE;
    WPARAM wpSizingHit;
    WINDOWPLACEMENT wndpl;
    // all below variables are for snapping
    RECT snapRect;
    HMONITOR hMon;
    MONITORINFO mi;

    style = GetWindowLong(hWnd, GWL_STYLE);
    exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
    iconic = (style & WS_MINIMIZE) != 0;

    if(!IsWindowVisible(hWnd))
        return;

    thickFrame = (style & WS_THICKFRAME) && !iconic;

    //
    // Show window contents while dragging the window, get flag from registry data.
    //
    SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &dragFullWindows, 0);

    GetCursorPos(&p);
    capturePoint = p;

    ClipCursor(NULL);

    if(sysCmd == SC_MOVE)
    {
        if(!hitTest)
            hitTest = CFrameStartMoveSize(hWnd, frame, wParam, &capturePoint);
        if(!hitTest)
            return;
    }
    else  /* SC_SIZE */
    {
        if(!thickFrame)
            return;
        if(hitTest && sysCmd != SC_MOUSEMENU)
        {
            hitTest += HTLEFT - WMSZ_LEFT;
        }
        else
        {
            SetCapture(hWnd);
            hitTest = CFrameStartMoveSize(hWnd, frame, wParam, &capturePoint);
            if(!hitTest)
            {
                ReleaseCapture();
                return;
            }
        }
    }

    sizingRect = frame->rcWindow;
    origRect = sizingRect;

    snapRect.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    snapRect.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    snapRect.right = snapRect.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
    snapRect.bottom = snapRect.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);

    if(style & WS_CHILD)
    {
        parent = GetParent(hWnd);
        GetClientRect(parent, &mouseRect);
        MapWindowPoints(parent, HWND_DESKTOP, (LPPOINT) &mouseRect, 2);
        MapWindowPoints(HWND_DESKTOP, parent, (LPPOINT) &sizingRect, 2);
    }
    else
    {
        if(!(exStyle & WS_EX_TOPMOST))
            mouseRect = snapRect;
        else
            SetRect(&mouseRect, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    }

    SendMessage(hWnd, WM_GETMINMAXINFO, 0, (LPARAM) &mmi);
    if(hitTest == HTLEFT || hitTest == HTTOPLEFT || hitTest == HTBOTTOMLEFT)
    {
       mouseRect.left  = max(mouseRect.left, sizingRect.right - mmi.ptMaxTrackSize.x + capturePoint.x - sizingRect.left);
       mouseRect.right = min(mouseRect.right, sizingRect.right - mmi.ptMinTrackSize.x + capturePoint.x - sizingRect.left);
    }
    else if(hitTest == HTRIGHT || hitTest == HTTOPRIGHT || hitTest == HTBOTTOMRIGHT)
    {
       mouseRect.left  = max(mouseRect.left, sizingRect.left + mmi.ptMinTrackSize.x + capturePoint.x - sizingRect.right);
       mouseRect.right = min(mouseRect.right, sizingRect.left + mmi.ptMaxTrackSize.x + capturePoint.x - sizingRect.right);
    }
    if(hitTest == HTTOP || hitTest == HTTOPLEFT || hitTest == HTTOPRIGHT)
    {
       mouseRect.top    = max(mouseRect.top, sizingRect.bottom - mmi.ptMaxTrackSize.y + capturePoint.y - sizingRect.top);
       mouseRect.bottom = min(mouseRect.bottom, sizingRect.bottom - mmi.ptMinTrackSize.y + capturePoint.y - sizingRect.top);
    }
    else if(hitTest == HTBOTTOM || hitTest == HTBOTTOMLEFT || hitTest == HTBOTTOMRIGHT)
    {
       mouseRect.top    = max(mouseRect.top, sizingRect.top + mmi.ptMinTrackSize.y + capturePoint.y - sizingRect.bottom);
       mouseRect.bottom = min(mouseRect.bottom, sizingRect.top + mmi.ptMaxTrackSize.y + capturePoint.y - sizingRect.bottom);
    }

    /* repaint the window before moving it around */
    RedrawWindow(hWnd, NULL, NULL, RDW_UPDATENOW | RDW_ALLCHILDREN);

    NotifyWinEvent(EVENT_SYSTEM_MOVESIZESTART, hWnd, OBJID_WINDOW, CHILDID_SELF);

    SendMessage(hWnd, WM_ENTERSIZEMOVE, 0, 0);

    SetCapture(hWnd);

    while(GetMessage(&msg, NULL, 0, 0))
    {
        p = msg.pt;
        if(msg.message == WM_LBUTTONUP)
        {
            // check for snapping if was moved by caption
            if(hitTest == HTCAPTION && thickFrame && !(exStyle & WS_EX_MDICHILD))
            {
                // snap to left
                if(p.x <= snapRect.left || p.x >= snapRect.right - 1)
                {
                    hMon = MonitorFromPoint(p, MONITOR_DEFAULTTONEAREST);
                    mi.cbSize = sizeof(mi);
                    GetMonitorInfo(hMon, &mi);
                    if(p.x <= snapRect.left)
                    {
                        snapRect = mi.rcWork;
                        snapRect.right = (snapRect.right - snapRect.left) / 2 + snapRect.left;
                    }
                    // snap to right
                    else
                    {
                        snapRect = mi.rcWork;
                        snapRect.left = (snapRect.right - snapRect.left) / 2 + snapRect.left;
                    }
                    SetWindowPos(hWnd, HWND_TOP, snapRect.left, snapRect.top, snapRect.right - snapRect.left, snapRect.bottom - snapRect.top, 0);
                }
                else if(p.y <= snapRect.top) // maximize if dragged to top
                    SendMessage(hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
            }
            break;
        }
        if(msg.message == WM_KEYDOWN && (msg.wParam == VK_RETURN || msg.wParam == VK_ESCAPE))
            break;

        if(msg.message != WM_KEYDOWN && msg.message != WM_MOUSEMOVE)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        if(msg.message == WM_KEYDOWN) switch(msg.wParam)
        {
        case VK_UP:    p.y -= 8; break;
        case VK_DOWN:  p.y += 8; break;
        case VK_LEFT:  p.x -= 8; break;
        case VK_RIGHT: p.x += 8; break;
        }

        p.x = max(p.x, mouseRect.left);
        p.x = min(p.x, mouseRect.right - 1);
        p.y = max(p.y, mouseRect.top);
        p.y = min(p.y, mouseRect.bottom - 1);

        dx = p.x - capturePoint.x;
        dy = p.y - capturePoint.y;

        if(dx || dy)
        {
            if(msg.message == WM_KEYDOWN)
                SetCursorPos(p.x, p.y);
            else
            {
                if(!moved && hitTest == HTCAPTION && (style & WS_MAXIMIZE))
                {
                    wndpl.length = sizeof(wndpl);
                    wndpl.flags = WPF_RESTORETOMAXIMIZED;
                    GetWindowPlacement(hWnd, &wndpl);
                    newRect.top = sizingRect.top;
                    newRect.left = sizingRect.left
                    + (p.x - sizingRect.left)
                        * (sizingRect.right - sizingRect.left - wndpl.rcNormalPosition.right + wndpl.rcNormalPosition.left)
                        / (sizingRect.right - sizingRect.left);
                    newRect.right = wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left;
                    newRect.bottom = wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top;
                    SetWindowLong(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) ^ WS_MAXIMIZE);
                    SetWindowPos(hWnd, HWND_TOP, 0, 0, newRect.right, newRect.bottom, SWP_NOMOVE);
                    newRect.right += newRect.left;
                    newRect.bottom += newRect.top;
                }
                else
                    newRect = sizingRect;

                if(!iconic && !dragFullWindows)
                    ; // TODO: UserDrawMovingFrame(hdc, &sizingRect, thickframe);
                if(hitTest == HTCAPTION)
                    OffsetRect(&newRect, dx, dy);
                if(hitTest == HTLEFT || hitTest == HTTOPLEFT || hitTest == HTBOTTOMLEFT)
                    newRect.left += dx;
                if(hitTest == HTRIGHT || hitTest == HTTOPRIGHT || hitTest == HTBOTTOMRIGHT)
                    newRect.right += dx;
                if(hitTest == HTTOP || hitTest == HTTOPLEFT || hitTest == HTTOPRIGHT)
                    newRect.top += dy;
                if(hitTest == HTBOTTOM || hitTest == HTBOTTOMLEFT || hitTest == HTBOTTOMRIGHT)
                    newRect.bottom += dy;

                capturePoint = p;

                /* determine the hit location */
                if(sysCmd == SC_SIZE)
                {
                    if(hitTest >= HTLEFT && hitTest <= HTBOTTOMRIGHT)
                        wpSizingHit = WMSZ_LEFT + (hitTest - HTLEFT);
                    SendMessage(hWnd, WM_SIZING, wpSizingHit, (LPARAM) &newRect);
                }
                else
                    SendMessage(hWnd, WM_MOVING, 0, (LPARAM) &newRect);

                if(!iconic)
                {
                    if(!dragFullWindows)
                        ;//TODO: UserDrawMovingFrame(hdc, &newRect, thickframe);
                    else
                        SetWindowPos(hWnd, HWND_TOP,
                                     newRect.left, newRect.top,
                                     newRect.right - newRect.left, newRect.bottom - newRect.top,
                                     (hitTest == HTCAPTION) ? SWP_NOSIZE : SWP_NOCOPYBITS);
                }
                sizingRect = newRect;
            }
            moved = 1;
        }
    }
    ReleaseCapture();

    NotifyWinEvent(EVENT_SYSTEM_MOVESIZEEND, hWnd, OBJID_WINDOW, CHILDID_SELF);

    SendMessage(hWnd, WM_EXITSIZEMOVE, 0, 0);
    if(moved)
    {
        /* if the moving/resizing isn't canceled call SetWindowPos
        * with the new position or the new size of the window
        */
        if(!(msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE))
        {
            /* NOTE: SWP_NOACTIVATE prevents document window activation in Word 6 */
            if(!dragFullWindows || iconic)
                SetWindowPos(hWnd, HWND_TOP,
                             sizingRect.left, sizingRect.top,
                             sizingRect.right - sizingRect.left, sizingRect.bottom - sizingRect.top,
                             (hitTest == HTCAPTION) ? SWP_NOSIZE : 0);
        }
        else
        { /* restore previous size/position */
            if(dragFullWindows)
                SetWindowPos(hWnd, HWND_TOP,
                             origRect.left, origRect.top,
                             origRect.right - origRect.left, origRect.bottom - origRect.top,
                             (hitTest == HTCAPTION) ? SWP_NOSIZE : 0);
        }
    }
    /* Single click brings up the system menu when iconized */
    if(iconic && !moved && (style & WS_SYSMENU))
        SendMessage(hWnd, WM_SYSCOMMAND, SC_MOUSEMENU + HTSYSMENU, MAKELONG(p.x, p.y));
}

LRESULT CFrameSysCommand(HWND hWnd, struct cframe *frame, WPARAM wParam, LPARAM lParam)
{
    LONG style;
    style = GetWindowLong(hWnd, GWL_STYLE);
    switch(wParam & 0xFFF0)
    {
    case SC_MOVE:
    case SC_SIZE:
        CFrameMoveSize(hWnd, frame, wParam);
        break;
    case SC_MINIMIZE:
        if(hWnd == GetActiveWindow())
            ShowOwnedPopups(hWnd, FALSE);
        ShowWindow(hWnd, SW_MINIMIZE);
        break;
    case SC_MAXIMIZE:
        if(!(style & WS_MINIMIZE) && hWnd == GetActiveWindow())
            ShowOwnedPopups(hWnd, TRUE);
        ShowWindow(hWnd, SW_MAXIMIZE);
        break;
    case SC_RESTORE:
        if(hWnd == GetActiveWindow())
            ShowOwnedPopups(hWnd, TRUE);
        ShowWindow(hWnd, SW_RESTORE);
        break;
    case SC_CLOSE:
        return SendMessage(hWnd, WM_CLOSE, 0, 0);
    case SC_MOUSEMENU:
        // TrackPopupMenu(GetSystemMenu(hWnd, 0), TPM_LEFTALIGN, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, hWnd, NULL);
        break;
    // TODO: SC_SCREENSAVE, SC_MOUSEMENU, SC_KEYMENU
    // case SC_DEFAULT:
    default:
        return 0;
    }
    return 0;
}

int CSetScrollPos(HWND hWnd, int bar, int pos);
int CSetScrollRange(HWND hWnd, int bar, int max);

int CSetScrollInfo(HWND hWnd, int bar, SCROLLINFO *si)
{
    struct cframe *frame;
    LONG style;
    struct cscroll *scroll;
    const int styleFlag = WS_HSCROLL << bar;
    HDC hdc;
    if(!si->fMask)
        return 0;
    if(si->fMask == SIF_POS)
        return CSetScrollPos(hWnd, bar, si->nPos);
    frame = (struct cframe*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    style = GetWindowLong(hWnd, GWL_STYLE);
    scroll = bar == SB_HORZ ? &frame->horz : &frame->vert;
    if(si->fMask & SIF_PAGE)
        scroll->page = si->nPage;
    if(si->fMask & SIF_RANGE)
        scroll->max = max(si->nMax, 0);
    if(si->fMask & SIF_POS)
        scroll->pos = max(0, min(si->nPos, scroll->max));
    if(!scroll->max)
    {
        if(style & styleFlag)
        {
            SetWindowLong(hWnd, GWL_STYLE, style ^ styleFlag);
            RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_FRAME);
            if(bar == SB_HORZ)
                frame->rcClient.bottom += GetSystemMetrics(SM_CYHSCROLL);
            else
                frame->rcClient.right += GetSystemMetrics(SM_CXVSCROLL);
            SendMessage(hWnd, WM_SIZE, SIZE_SCROLL, MAKELPARAM(frame->rcClient.right - frame->rcClient.left, frame->rcClient.bottom - frame->rcClient.top));
        }
    }
    else
    {
        if(!(style & styleFlag))
        {
            if(bar == SB_HORZ)
                frame->rcClient.bottom -= GetSystemMetrics(SM_CYHSCROLL);
            else
                frame->rcClient.right -= GetSystemMetrics(SM_CXVSCROLL);
        }
        if(bar == SB_HORZ) // change properties of horizontal bar
        {
            scroll->rcTrack.left = frame->rcClient.left;
            scroll->rcTrack.top = frame->rcClient.bottom;
            scroll->rcTrack.right = frame->rcClient.right;
            scroll->rcTrack.bottom = frame->rcClient.bottom + GetSystemMetrics(SM_CYHSCROLL);
            scroll->rcThumb.top = scroll->rcTrack.top + 1;
            scroll->rcThumb.bottom = scroll->rcTrack.bottom - 1;
            scroll->rcThumb.right = max((scroll->rcTrack.right - scroll->rcTrack.left) * scroll->page / (scroll->max + scroll->page), CSCROLL_MINSIZE);
            scroll->rcThumb.left = scroll->rcTrack.left + scroll->pos * (scroll->page - scroll->rcThumb.right) / scroll->max;
            scroll->rcThumb.right += scroll->rcThumb.left;
        }
        else // change properties of vertical bar
        {
            scroll->rcTrack.left = frame->rcClient.right;
            scroll->rcTrack.top = frame->rcClient.top;
            scroll->rcTrack.right = frame->rcClient.right + GetSystemMetrics(SM_CXVSCROLL);
            scroll->rcTrack.bottom = frame->rcClient.bottom;
            scroll->rcThumb.left = scroll->rcTrack.left + 1;
            scroll->rcThumb.right = scroll->rcTrack.right - 1;
            scroll->rcThumb.bottom = max((scroll->rcTrack.bottom - scroll->rcTrack.top) * scroll->page / (scroll->max + scroll->page), CSCROLL_MINSIZE);
            scroll->rcThumb.top = scroll->rcTrack.top + scroll->pos * (scroll->page - scroll->rcThumb.bottom) / scroll->max;
            scroll->rcThumb.bottom += scroll->rcThumb.top;
        }
        if(!(style & styleFlag))
        {
            SetWindowLong(hWnd, GWL_STYLE, style | styleFlag);
            RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
            SendMessage(hWnd, WM_SIZE, SIZE_SCROLL, MAKELPARAM(frame->rcClient.right - frame->rcClient.left, frame->rcClient.bottom - frame->rcClient.top));
        }
        hdc = GetDCEx(hWnd, NULL, DCX_WINDOW);
        FillRect(hdc, &scroll->rcTrack, frame->hbrTrack);
        FillRect(hdc, &scroll->rcThumb, frame->hbrThumb);
        FrameRect(hdc, &scroll->rcThumb, frame->hbrOutline);
        ReleaseDC(hWnd, hdc);
    }
    return scroll->pos;
}

int CSetScrollPos(HWND hWnd, int bar, int pos)
{
    HDC hdc; // We can optimize painting by directly doing it here
    struct cframe *frame;
    LONG style;
    struct cscroll *scroll;
    const int styleFlag = WS_HSCROLL << bar;
    RECT r;
    int dx, dy;
    frame = (struct cframe*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    style = GetWindowLong(hWnd, GWL_STYLE);
    if(!(style & styleFlag))
        return 0; // wtf?
    scroll = bar == SB_HORZ ? &frame->horz : &frame->vert;
    hdc = GetDCEx(hWnd, NULL, DCX_WINDOW);
    if(bar == SB_HORZ)
    {
        r.top = scroll->rcThumb.top;
        r.bottom = scroll->rcThumb.bottom;
        r.left = scroll->rcThumb.left;

        scroll->pos = pos;
        scroll->rcThumb.right -= scroll->rcThumb.left;
        scroll->rcThumb.left = scroll->rcTrack.left + scroll->pos * (scroll->page - scroll->rcThumb.right) / scroll->max;
        scroll->rcThumb.right += scroll->rcThumb.left;

        dx = scroll->rcThumb.left - r.left;
        if(dx < 0)
        {
            r.right = scroll->rcThumb.right;
            r.left += dx;
            FillRect(hdc, &r, frame->hbrThumb);
            r.left = r.right;
            r.right = r.left - dx;
            FillRect(hdc, &r, frame->hbrTrack);
        }
        else
        {
            r.right = scroll->rcThumb.right;
            r.left += dx;
            FillRect(hdc, &r, frame->hbrThumb);
            r.right = r.left;
            r.left = r.right - dx;
            FillRect(hdc, &r, frame->hbrTrack);
        }
        FrameRect(hdc, &scroll->rcThumb, frame->hbrOutline);
    }
    else
    {
        r.left = scroll->rcThumb.left;
        r.right = scroll->rcThumb.right;
        r.top = scroll->rcThumb.top;

        scroll->pos = pos;
        scroll->rcThumb.bottom -= scroll->rcThumb.top;
        scroll->rcThumb.top = scroll->rcTrack.top + scroll->pos * (scroll->page - scroll->rcThumb.bottom) / scroll->max;
        scroll->rcThumb.bottom += scroll->rcThumb.top;

        dy = scroll->rcThumb.top - r.top;
        if(dy < 0)
        {
            r.bottom = scroll->rcThumb.bottom;
            r.top += dy;
            FillRect(hdc, &r, frame->hbrThumb);
            r.top = r.bottom;
            r.bottom = r.top - dy;
            FillRect(hdc, &r, frame->hbrTrack);
        }
        else
        {
            r.bottom = scroll->rcThumb.bottom;
            r.top += dy;
            FillRect(hdc, &r, frame->hbrThumb);
            r.bottom = r.top;
            r.top = r.bottom - dy;
            FillRect(hdc, &r, frame->hbrTrack);
        }
        FrameRect(hdc, &scroll->rcThumb, frame->hbrOutline);
    }
    ReleaseDC(hWnd, hdc);
    return scroll->pos;
}

int CSetScrollRange(HWND hWnd, int bar, int max) // we only need max, min is unnecessary here
{
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE;
    si.nMin = 0;
    si.nMax = max;
    return CSetScrollInfo(hWnd, bar, &si);
}

LRESULT CALLBACK CFrameProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct cframe *frame;
    MINMAXINFO mmi;
    MINMAXINFO *pmmi;
    WINDOWPOS *wp;
    LONG style;
    RECT rect;
    HMONITOR hMon;
    MONITORINFO mi;
    HDC hdc;
    HICON hIcon;
    LRESULT ret;
    HRGN hRgn;
    PAINTSTRUCT ps;
    HBRUSH hBrush;
    frame = (struct cframe*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    //CPrintWinMsg(msg, wParam, lParam);
    switch(msg)
    {
    case WM_DEVICECHANGE:
        return 1;
    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;
    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
        hBrush = (HBRUSH) GetClassLongPtr(hWnd, GCLP_HBRBACKGROUND);
        if(!hBrush)
            return 0;
        if(GetClassLong(hWnd, GCL_STYLE) & CS_PARENTDC)
        {
            /* can't use GetClipBox with a parent DC or we fill the whole parent */
            GetClientRect(hWnd, &rect);
            DPtoLP((HDC) wParam, (LPPOINT) &rect, 2);
        }
        else
        {
            GetClipBox((HDC) wParam, &rect);
        }
        FillRect((HDC) wParam, &rect, hBrush);
        return 1;
    case WM_QUERYOPEN:
        return 1;
    case WM_GETTEXT:
        if(!wParam)
            return 0;
        wParam = min(wParam, strlen(frame->title) + 1);
        ((LPSTR) memcpy((LPSTR) lParam, frame->title, wParam - 1))[wParam - 1] = 0;
        return wParam;
    case WM_GETTEXTLENGTH:
        return !frame->title ? 0 : strlen(frame->title);
    case WM_SETTEXT:
        frame->title = strcpy(malloc(strlen((LPCSTR) lParam) + 1), (LPCSTR) lParam);
        return 1;
    case WM_PAINTICON:
    case WM_PAINT:
        BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        return 0;
    case WM_SETCURSOR:
        switch(LOWORD(lParam))
        {
        case HTCLOSE: case HTMINBUTTON: case HTMAXBUTTON: SetCursor(LoadCursor(NULL, IDC_HAND)); break;
        case HTNOWHERE:
        case HTCAPTION: case HTVSCROLL: case HTHSCROLL: case HTBORDER: SetCursor(LoadCursor(NULL, IDC_ARROW)); break;
        case HTBOTTOM: case HTTOP: SetCursor(LoadCursor(NULL, IDC_SIZENS)); break;
        case HTRIGHT: case HTLEFT: SetCursor(LoadCursor(NULL, IDC_SIZEWE)); break;
        case HTTOPRIGHT: case HTBOTTOMLEFT: SetCursor(LoadCursor(NULL, IDC_SIZENESW)); break;
        case HTTOPLEFT: case HTBOTTOMRIGHT: SetCursor(LoadCursor(NULL, IDC_SIZENWSE)); break;
        }
        return 0;
    case WM_SYSCOMMAND:
        return CFrameSysCommand(hWnd, frame, wParam, lParam);
    case WM_SHOWWINDOW:
        if(!!(GetWindowLong(hWnd, GWL_STYLE) & WS_VISIBLE) ^ !!wParam)
            break;
        if(LOWORD(lParam))
            ShowWindow(hWnd, wParam ? SW_SHOWNOACTIVATE : SW_HIDE);
        return 0;
    case WM_GETMINMAXINFO:
        pmmi = (MINMAXINFO*) lParam;
        hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
        mi.cbSize = sizeof(mi);
        GetMonitorInfo(hMon, &mi);
        pmmi->ptMaxPosition = (POINT) { mi.rcWork.left, mi.rcWork.top };
        pmmi->ptMaxSize = (POINT) { mi.rcWork.right - mi.rcWork.left, mi.rcWork.bottom - mi.rcWork.top };
        printf("%ld, %ld, %ld, %ld\n", pmmi->ptMaxPosition.x, pmmi->ptMaxPosition.y, pmmi->ptMaxSize.x, pmmi->ptMaxSize.y);
        pmmi->ptMinTrackSize = (POINT) { GetSystemMetrics(SM_CXMINTRACK), GetSystemMetrics(SM_CYMINTRACK) };
        pmmi->ptMaxTrackSize = (POINT) { GetSystemMetrics(SM_CXMAXTRACK), GetSystemMetrics(SM_CYMAXTRACK) };
        return 0;
    case WM_SETICON:
        hIcon = frame->hIcon;
        frame->hIcon = (HICON) lParam;
        return (LRESULT) hIcon;
    case WM_GETICON:
        return (LRESULT) frame->hIcon;
    case WM_SYSKEYUP:
        switch(wParam)
        {
        case VK_F4: // ALT + F4
            SendMessage(hWnd, WM_CLOSE, 0, 0);
            break;
        }
        return 0;
    case WM_NCCREATE:
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) ((CREATESTRUCT*) lParam)->lpCreateParams);
        return 1;
    case WM_NCCALCSIZE:
        return CFrameNCCalcSize(hWnd, frame, (RECT*) lParam, wParam);
    case WM_NCHITTEST:
        return CFrameHitTest(hWnd, frame, (POINT) { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
    case WM_NCPAINT:
        hdc = wParam != NULLREGION ? GetDCEx(hWnd, (HRGN) wParam, DCX_WINDOW | DCX_INTERSECTRGN) : GetDCEx(hWnd, NULL, DCX_WINDOW);
        CFrameNCPaint(hWnd, frame, hdc, -1);
        ReleaseDC(hWnd, hdc);
        return 0;
    case WM_SYNCPAINT:
        hRgn = CreateRectRgn(0, 0, 0, 0);
        if(hRgn)
        {
            if(GetUpdateRgn(hWnd, hRgn, FALSE) != NULLREGION)
            {
                if(!wParam)
                    wParam = RDW_ERASENOW | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN;
                RedrawWindow(hWnd, NULL, hRgn, wParam);
            }
            DeleteObject(hRgn);
        }
        return 0;
    case WM_NCLBUTTONDOWN:
        CFrameNCLButtonDown(hWnd, frame, wParam, lParam);
        return 0;
    case WM_NCLBUTTONDBLCLK:
        CFrameNCLButtonDblClk(hWnd, frame, wParam, lParam);
        return 0;
    case WM_NCRBUTTONDOWN:
        // CFrameSysCommand(hWnd, frame, SC_MOUSEMENU + HTCAPTION, lParam);
        return 0;
    case WM_NCACTIVATE:
        if(!(GetWindowLong(hWnd, GWL_STYLE) & WS_VISIBLE))
            return 1;
        if(lParam != -1)
        {
            hRgn = CreateRectRgn(0, 0, 0, 0);
            if(CombineRgn(hRgn, (HRGN) lParam, 0, RGN_COPY) == ERROR)
            {
                DeleteObject(hRgn);
                hRgn = NULL;
            }

            if((hdc = GetDCEx(hWnd, hRgn, DCX_WINDOW | 0x1000)))
            {
                CFrameNCPaint(hWnd, frame, hdc, 0);
                ReleaseDC(hWnd, hdc);
            }
            else
                DeleteObject(hRgn);
        }
        return 1;
    case WM_MOUSEACTIVATE:
        if(GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD)
        {
            ret = SendMessage(GetParent(hWnd), WM_MOUSEACTIVATE, wParam, lParam);
            if(ret)
                return ret;
        }
        return HIWORD(lParam) == WM_LBUTTONDOWN && LOWORD(lParam) == HTCAPTION ? MA_NOACTIVATE : MA_ACTIVATE;
    case WM_ACTIVATE:
        if(LOWORD(wParam) != WA_INACTIVE && !(GetWindowLong(hWnd, GWL_STYLE) & WS_MINIMIZE))
            SetFocus(hWnd);
        return 0;
    case WM_MOUSEWHEEL:
        if(GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD)
            return SendMessage(GetParent(hWnd), WM_MOUSEWHEEL, wParam, lParam);
        return 0;
    case WM_WINDOWPOSCHANGING:
        wp = (WINDOWPOS*) lParam;
        style = GetWindowLong(hWnd, GWL_STYLE);

        if(wp->flags & SWP_NOSIZE)
        {
            if(!(wp->flags & SWP_NOMOVE))
                GetWindowRect(hWnd, &frame->rcWindow);
            return 0;
        }
        if((style & WS_THICKFRAME) || !(style & (WS_POPUP | WS_CHILD)))
        {
            SendMessage(hWnd, WM_GETMINMAXINFO, 0, (LPARAM) &mmi);
            wp->cx = min(wp->cx, mmi.ptMaxTrackSize.x);
            wp->cy = min(wp->cy, mmi.ptMaxTrackSize.y);
            if(!(style & WS_MINIMIZE))
            {
                if(wp->cx < mmi.ptMinTrackSize.x)
                    wp->cx = mmi.ptMinTrackSize.x;
                if(wp->cy < mmi.ptMinTrackSize.y)
                    wp->cy = mmi.ptMinTrackSize.y;
            }
        }
        else
        {
            wp->cx = max(wp->cx, 0);
            wp->cy = max(wp->cy, 0);
        }
        return 0;
    case WM_WINDOWPOSCHANGED:
        wp = (WINDOWPOS*) lParam;
        style = GetWindowLong(hWnd, GWL_STYLE);

        GetClientRect(hWnd, &rect);
        MapWindowPoints(hWnd, GetParent(hWnd), (LPPOINT) &rect, 2);

        if(wp->flags & SWP_NOSIZE)
        {
            if(!(wp->flags & SWP_NOMOVE))
                GetWindowRect(hWnd, &frame->rcWindow);
            return 0;
        }

        if(!(wp->flags & 0x1000)) // SWP_NOCLIENTMOVE
            SendMessage(hWnd, WM_MOVE, 0, MAKELONG(rect.left, rect.top));

        // SWP_NOCLIENTSIZE // SWP_STATECHANGED
        if(!(wp->flags & 0x800) || (wp->flags & 0x8000))
        {
            if(style & WS_MINIMIZE)
                SendMessage(hWnd, WM_SIZE, SIZE_MINIMIZED, 0);
            else
                SendMessage(hWnd, WM_SIZE, (style & WS_MAXIMIZE) ? SIZE_MAXIMIZED : SIZE_RESTORED, MAKELONG(rect.right - rect.left, rect.bottom - rect.top));
        }
        return 0;
    }
    return 0;//DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK IVProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HDC imageDc;
    static int width, height;
    static int lastX = -1, lastY;
    static float zoom = 1.0f;
    float ratio;
    SCROLLINFO si;
    int x, y, dx, dy;
    HBITMAP hBmp;
    BITMAP bmp;
    PAINTSTRUCT ps;
    HDC hdc;
    RECT r;
    int newVScroll, newHScroll;
    struct cframe *frame = (struct cframe*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
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
    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE:
        if(wParam == SIZE_SCROLL)
            return 0;
        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;
        si.nPage = HIWORD(lParam);
        si.nMax = height * zoom - si.nPage;
        si.nPos = newVScroll = min(frame->vert.pos, si.nMax);
        dy = newVScroll - frame->vert.pos;
        CSetScrollInfo(hWnd, SB_VERT, &si);

        GetClientRect(hWnd, &r);
        si.nPage = r.right;
        si.nMax = width * zoom - si.nPage;
        si.nPos = newHScroll = min(frame->horz.pos, si.nMax);
        dx = newHScroll - frame->horz.pos;
        CSetScrollInfo(hWnd, SB_HORZ, &si);

        ScrollWindow(hWnd, -dx, -dy, NULL, NULL);
        return 0;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        GetClientRect(hWnd, &r);
        StretchBlt(hdc, 0, 0, width * zoom, height * zoom, imageDc, frame->horz.pos / zoom, frame->vert.pos / zoom, width, height, SRCCOPY);
        EndPaint(hWnd, &ps);
        return 0;
    case WM_SETCURSOR:
        if(LOWORD(lParam) == HTCLIENT)
        {
            SetCursor(LoadCursor(NULL, IDC_CROSS));
            return 0;
        }
        break;
    case WM_LBUTTONDOWN:
        lastX = GET_X_LPARAM(lParam);
        lastY = GET_Y_LPARAM(lParam);
        SetCapture(hWnd);
        return 0;
    case WM_LBUTTONUP:
        ReleaseCapture();
        return 0;
    case WM_MOUSEMOVE:
        x = GET_X_LPARAM(lParam);
        y = GET_Y_LPARAM(lParam);
        if(wParam == MK_LBUTTON)
        {
            newVScroll = frame->vert.pos + lastY - y;
            newVScroll = max(newVScroll, 0);
            newVScroll = min(newVScroll, frame->vert.max);

            newHScroll = frame->horz.pos + lastX - x;
            newHScroll = max(newHScroll, 0);
            newHScroll = min(newHScroll, frame->horz.max);

            dy = newVScroll - frame->vert.pos;
            dx = newHScroll - frame->horz.pos;

            if(dx || dy)
            {
                CSetScrollPos(hWnd, SB_VERT, newVScroll);
                CSetScrollPos(hWnd, SB_HORZ, newHScroll);
                ScrollWindow(hWnd, -dx, -dy, NULL, NULL);
            }
        }
        lastX = x;
        lastY = y;
        return 0;
    case WM_MOUSEWHEEL:
        if(wParam & MK_CONTROL)
        {
            x = GET_X_LPARAM(lParam);
            y = GET_Y_LPARAM(lParam);
            ratio = zoom;
            zoom *= GET_WHEEL_DELTA_WPARAM(wParam) < 0 ? 9.0f / 10.0f : 10.0f / 9.0f;
            ratio /= zoom;
            GetClientRect(hWnd, &r);
            CSetScrollRange(hWnd, SB_VERT, height * zoom - r.bottom);
            CSetScrollRange(hWnd, SB_HORZ, width * zoom - r.right);
            RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
            return 0;
        }
        newVScroll = frame->vert.pos - GET_WHEEL_DELTA_WPARAM(wParam) / 10;
        goto do_v_scroll;
    case WM_HSCROLL:
        newHScroll = frame->horz.pos;
        switch(LOWORD(wParam))
        {
        case SB_PAGELEFT: newHScroll -= 50; break;
        case SB_PAGERIGHT: newHScroll += 50; break;
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            newHScroll = (short) HIWORD(wParam);
            break;
        }
        newHScroll = max(newHScroll, 0);
        newHScroll = min(newHScroll, frame->horz.max);
        if(newHScroll == frame->horz.pos)
            return 0;
        dx = newHScroll - frame->horz.pos;
        CSetScrollPos(hWnd, SB_HORZ, newHScroll);
        ScrollWindow(hWnd, -dx, 0, NULL, NULL);
        return 0;
    case WM_VSCROLL:
        newVScroll = frame->vert.pos;
        switch(LOWORD(wParam))
        {
        case SB_PAGEUP: newVScroll -= 50; break;
        case SB_PAGEDOWN: newVScroll += 50; break;
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            newVScroll = (short) HIWORD(wParam);
            break;
        }
    do_v_scroll:
        newVScroll = max(newVScroll, 0);
        newVScroll = min(newVScroll, frame->vert.max);
        if(newVScroll == frame->vert.pos)
            return 0;
        dy = newVScroll - frame->vert.pos;
        CSetScrollPos(hWnd, SB_VERT, newVScroll);
        ScrollWindow(hWnd, 0, -dy, NULL, NULL);
        return 0;
    }
    return CFrameProc(hWnd, msg, wParam, lParam);
}

int main(void)
{
    WNDCLASS wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpszClassName = "CFrame";
    wc.lpfnWndProc = CFrameProc;
    wc.style = CS_OWNDC;
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    RegisterClass(&wc);
    wc.lpszClassName = "ImageView";
    wc.lpfnWndProc = IVProc;
    RegisterClass(&wc);
    wc.lpszClassName = "DEFWINDOW";
    wc.lpfnWndProc = DefWindowProc;
    RegisterClass(&wc);
    //CCreateFrame("CFrame", "Title", LoadImage(NULL, "Icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE), 100, 100, 300, 400, NULL);
    CCreateFrame("ImageView", "Title", LoadImage(NULL, "Icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE), 190, 100, 300, 400, NULL);
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
