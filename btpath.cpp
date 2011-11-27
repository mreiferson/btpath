/*
 * Includes
 */
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <ddraw.h>
#include <dinput.h>
#include "btpath.h"
#include "doddefines.h"
#include "dodddraw.h"
#include "doddinput.h"

/*
 * Defines
 */
#define SCREEN_WIDTH        640
#define SCREEN_HEIGHT   480
#define TILESIZE            10
#define MAPWIDTH            SCREEN_WIDTH/TILESIZE
#define MAPHEIGHT           SCREEN_HEIGHT/TILESIZE

void Line(WORD *buf, int x1, int y1, int x2, int y2, WORD color, int pitch);

/*
 * Typedefs
 */
// the structure which holds pixel RGB masks
typedef struct _RGBMASK {
    unsigned long rgbRed; // red component
    unsigned long rgbGreen; // green component
    unsigned long rgbBlue; // blue component
} RGBMASK;

// the structure which holds screen surface info (5,6,5 or 5,5,5 and masking)
typedef struct _RGB16 {
    RGBQUAD depth;
    RGBQUAD Amount;
    RGBQUAD Position;
    RGBMASK Mask;
} RGB16;

// the structure to hold a map coordinate
typedef struct _MapCoord {
    int X,Y;
} MapCoord;

/*
 * Declarations
 */
int CollMap[MAPHEIGHT][MAPWIDTH];
MapCoord Start, End;

BOOL            bActive;                      // is the application active
HWND            main_window_handle;           // Global Main Window Handle
HINSTANCE   main_instance;                // Global Window Instance Handle
RECT            rcWindow;
RGB16                   rgb16;             // Video Card RGB Information Structure
int                     mRed, mGreen,      // Faster values of above structure
                        mBlue, pRed,       // Faster values of above structure
                        pGreen, pBlue;     // Faster values of above structure
int rotarray[8][2] =        {   1,1,
                                -1,1,
                                -1,-1,
                                0,-1,
                                2,0,
                                0,2,
                                -2,0,
                                1,-2
                            };

/*
 * Macros
 */
#define RED(p) (p >> pRed)                                             // Extracts Red Component
#define GREEN(p) ((p & mGreen) >> pGreen)                              // Extracts Green Component
#define BLUE(p) (p & mBlue)                                            // Extracts Blue Component
#define RGB16(r, g, b) ((r << pRed) | (g << pGreen) | b)               // Creates RGB Pixel Value

/*
 * Code
 */
BOOL LineIntersect(MapCoord *LineStartPos, MapCoord *Pos)
{
    int i;
    float x, y;
    float Sx, Sy;
    int ix, iy;
    int Dx, Dy;
    int aDx, aDy;
    int steps;
    BOOL LineIntersect = FALSE;
    
    Dx = LineStartPos->X - Pos->X;
    Dy = LineStartPos->Y - Pos->Y;
    x = (float)LineStartPos->X;
    y = (float)LineStartPos->Y;
    
    aDx = abs(Dx);
    aDy = abs(Dy);
    
    if (aDx > aDy) {
        if (Dx > 0) {
            Sx = -1;
        } else {
            Sx = 1;
        }
        
        if (Dx == 0) {
            Sy = 0;
        } else {
            Sy = Dy/(Dx*Sx);
        }
        
        steps = aDx;
    } else {
        if (Dy > 0) {
            Sy = -1;
        } else {
            Sy = 1;
        }
        
        if (Dy == 0) {
            Sx = 0;
        } else {
            Sx = Dx/(Dy*Sy);
        }
        
        steps = aDy;
    }
    
    for (i=0; i<=steps; i++) {
        ix = (int)x;
        iy = (int)y;
        
        int inrange;
        if ((ix < 0) || (iy < 0) || (ix >= MAPWIDTH) || (iy >= MAPHEIGHT)) {
            inrange = 0;
        } else {
            inrange = 1;
        }
        
        if ((CollMap[iy][ix] > 0) || !inrange) {
            LineIntersect = TRUE;
            break;
        }
        
        y += Sy;
        x += Sx;
    }
    
    return LineIntersect;
}

void FindTheWay(MapCoord *StartPos, MapCoord *EndPos)
{
    int LenMap[MAPHEIGHT][MAPWIDTH];
    MapCoord NewPos, Pos, ShPos, BestLine, LineStartPos;
    int i;
    char Dir;
    MapCoord PList[500];
    MapCoord WList[500];
    int LList[500];
    int WayCnt=0;
    int PCnt;
    BOOL GoalReached;
    int Shortest;
    int DiffX, DiffY;
    int ShStep;
    int StepCnt;
    int inrange;
    
    for (int y=0; y<MAPHEIGHT; y++)
        for (int x=0; x<MAPWIDTH; x++) {
            LenMap[y][x] = 65535;
        }
        
    PCnt = 1;
    PList[0] = (*StartPos);
    DiffX = StartPos->X - EndPos->X;
    DiffY = StartPos->Y - EndPos->Y;
    LList[0] = (DiffX * DiffX) + (DiffY * DiffY);
    
    LenMap[StartPos->Y][StartPos->X] = 1;
    Pos = (*StartPos);
    NewPos = Pos;
    GoalReached = FALSE;
    
    do {
        Shortest = 0;
        
        for (i=0; i<PCnt; i++) {
            if (LList[i] < LList[Shortest]) {
                Shortest = i;
            }
        }
        
        Pos = PList[Shortest];
        PList[Shortest] = PList[PCnt-1];
        LList[Shortest] = LList[PCnt-1];
        PCnt--;
        
        NewPos = Pos;
        Dir = -1;
        NewPos.Y--;
        
        for (i=0; i<8; i++) {
            if ((NewPos.X < 0) || (NewPos.Y < 0) || (NewPos.X >= MAPWIDTH) || (NewPos.Y >= MAPHEIGHT)) {
                inrange = 0;
            } else {
                inrange = 1;
            }
            
            if ((LenMap[NewPos.Y][NewPos.X]==65535) && (CollMap[NewPos.Y][NewPos.X] == 0) && inrange) {
                StepCnt = LenMap[Pos.Y][Pos.X]+1;
                LenMap[NewPos.Y][NewPos.X] = StepCnt;
                PList[PCnt] = NewPos;
                DiffX = NewPos.X - EndPos->X;
                DiffY = NewPos.Y - EndPos->Y;
                LList[PCnt] = (DiffX * DiffX) + (DiffY * DiffY);
                PCnt++;
            }
            
            if ((NewPos.X == EndPos->X) && (NewPos.Y == EndPos->Y)) {
                GoalReached = TRUE;
                break;
            }
            
            Dir++;
            NewPos.X += rotarray[Dir][0];
            NewPos.Y += rotarray[Dir][1];
        }
    } while (!GoalReached && !(PCnt == 0));
    
    if (GoalReached) {
        WList[WayCnt] = (*EndPos);
        WayCnt++;
        Pos = (*EndPos);
        LineStartPos = Pos;
        
        while (!((Pos.X == StartPos->X) && (Pos.Y == StartPos->Y))) {
            NewPos = Pos;
            ShStep = 65535;
            Dir = -1;
            NewPos.Y--;
            
            for (i=0; i<8; i++) {
                if ((NewPos.X < 0) || (NewPos.Y < 0) || (NewPos.X >= MAPWIDTH) || (NewPos.Y >= MAPHEIGHT)) {
                    inrange = 0;
                } else {
                    inrange = 1;
                }
                
                if ((LenMap[NewPos.Y][NewPos.X] < ShStep) && inrange) {
                    ShStep = LenMap[NewPos.Y][NewPos.X];
                    ShPos = NewPos;
                }
                
                Dir++;
                NewPos.X += rotarray[Dir][0];
                NewPos.Y += rotarray[Dir][1];
            }
            
            Pos = ShPos;
            
            if (!LineIntersect(&LineStartPos, &Pos)) {
                BestLine = Pos;
            }
            
            if ((Pos.X == StartPos->X) && (Pos.Y == StartPos->Y)) {
                Pos = BestLine;
                LineStartPos = Pos;
                WList[WayCnt] = LineStartPos;
                WayCnt++;
            }
        }
        
        for (i=0; i<(WayCnt/2); i++) {
            MapCoord tmp = WList[i];
            WList[i] = WList[(WayCnt-1)-i];
            WList[(WayCnt-1)-i] = tmp;
        }
        
        int pitch;
        WORD *tmp;
        tmp = DD_Lock_Surface(lpddsback, &pitch);
        for (i=0; i<(WayCnt-1); i++) {
            Line(tmp, WList[i].X*TILESIZE+(TILESIZE/2), WList[i].Y*TILESIZE+(TILESIZE/2), WList[i+1].X*TILESIZE+(TILESIZE/2), WList[i+1].Y*TILESIZE+(TILESIZE/2), 2047, pitch);
        }
        DD_Unlock_Surface(lpddsback, tmp);
    }
}

/*
 * RestoreGraphics:
 *      Restores the directdraw surfaces
 */
void RestoreGraphics(void)
{
    if (lpddsprimary) {
        lpddsprimary->Restore();
    }
    if (lpddsback) {
        lpddsback->Restore();
    }
}

/*
 * WinProc:
 *    Processes windows messages.
 */
long PASCAL WinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // determine which message was sent in order to handle it
    switch (message) {
        // the app is being activated
    case WM_ACTIVATEAPP:
        bActive = wParam; // is it active, TRUE or FALSE?
        break;
        // it's telling us it's time to go, quit.....
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
        // update the client area rectangle
    case WM_MOVE:
        GetClientRect(hWnd, &rcWindow);
        ClientToScreen(hWnd, (LPPOINT)&rcWindow);
        ClientToScreen(hWnd, (LPPOINT)&rcWindow+1);
        break;
    case WM_MOUSEMOVE:
        int xPos = LOWORD(lParam);  // horizontal position of cursor
        int yPos = HIWORD(lParam);  // vertical position of cursor
        int locx = xPos/TILESIZE;
        int locy = yPos/TILESIZE;
        if ((wParam == MK_LBUTTON)) {
            if (!((locx == Start.X) && (locy == Start.Y)) && !((locx == End.X) && (locy == End.Y))) {
                CollMap[yPos/TILESIZE][xPos/TILESIZE] = 1;
            }
        } else if ((wParam == MK_MBUTTON)) {
            if (CollMap[locy][locx] < 1) {
                Start.X = xPos/TILESIZE;
                Start.Y = yPos/TILESIZE;
            }
        } else if ((wParam == MK_RBUTTON)) {
            if (CollMap[locy][locx] < 1) {
                End.X = xPos/TILESIZE;
                End.Y = yPos/TILESIZE;
            }
        }
        break;
    }
    
    // return to caller
    return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
 * FiniApp:
 *    Cleans up the application, frees memory.
 */
void FiniApp(void)
{
    // shutdown directdraw
    DD_Shutdown();
    
    // release all input devices
    DI_Release_Keyboard();
    DI_Release_Mouse();
    
    // shutdown directinput
    DInput_Shutdown();
}

/*
 * GetRGB16:
 *    Must run this function to fill the RGB16 struct with the information needed to plot a pixel
 * To call this, you must have rgb16 defined as a global (unless you want to modify this) variable
 * RGB16 rgb16;
 */
BOOL GetRGB16(LPDIRECTDRAWSURFACE Surface)
{
    DDSURFACEDESC   ddsd;       // DirectDraw Surface Description
    BYTE            shiftcount; // Shift Counter
    
    // get a surface despriction
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_PIXELFORMAT;
    if (Surface->GetSurfaceDesc(&ddsd) != DD_OK) {
        return FALSE;
    }
    
    // Fill in the masking values for extracting colors
    rgb16.Mask.rgbRed = ddsd.ddpfPixelFormat.dwRBitMask;
    rgb16.Mask.rgbGreen = ddsd.ddpfPixelFormat.dwGBitMask;
    rgb16.Mask.rgbBlue = ddsd.ddpfPixelFormat.dwBBitMask;
    
    // get red surface information
    shiftcount = 0;
    while (!(ddsd.ddpfPixelFormat.dwRBitMask & 1)) {
        ddsd.ddpfPixelFormat.dwRBitMask >>= 1;
        shiftcount++;
    }
    rgb16.depth.rgbRed = (BYTE)ddsd.ddpfPixelFormat.dwRBitMask;
    rgb16.Position.rgbRed = shiftcount;
    rgb16.Amount.rgbRed = (ddsd.ddpfPixelFormat.dwRBitMask == 0x1f) ? 3 : 2;
    
    // get green surface information
    shiftcount = 0;
    while (!(ddsd.ddpfPixelFormat.dwGBitMask & 1)) {
        ddsd.ddpfPixelFormat.dwGBitMask >>= 1;
        shiftcount++;
    }
    rgb16.depth.rgbGreen =(BYTE)ddsd.ddpfPixelFormat.dwGBitMask;
    rgb16.Position.rgbGreen = shiftcount;
    rgb16.Amount.rgbGreen = (ddsd.ddpfPixelFormat.dwGBitMask == 0x1f) ? 3 : 2;
    
    // get Blue surface information
    shiftcount = 0;
    while (!(ddsd.ddpfPixelFormat.dwBBitMask & 1)) {
        ddsd.ddpfPixelFormat.dwBBitMask >>= 1;
        shiftcount++;
    }
    rgb16.depth.rgbBlue =(BYTE)ddsd.ddpfPixelFormat.dwBBitMask;
    rgb16.Position.rgbBlue = shiftcount;
    rgb16.Amount.rgbBlue = (ddsd.ddpfPixelFormat.dwBBitMask == 0x1f) ? 3 : 2;
    
    // fill in variables so we dont' have to access the structure anymore
    mRed = rgb16.Mask.rgbRed;         // Red Mask
    mGreen = rgb16.Mask.rgbGreen;     // Green Mask
    mBlue = rgb16.Mask.rgbBlue;       // Blue Mask
    pRed = rgb16.Position.rgbRed;     // Red Position
    pGreen = rgb16.Position.rgbGreen; // Green Position
    pBlue = rgb16.Position.rgbBlue;   // Blue Position
    
    // return to caller
    return TRUE;
}

/*
 * InitGame:
 *    Initializes core game components
 */
void InitGame()
{
    // Initialize directdraw
#ifdef DOD_WINDOWED
    DD_Init_Windowed(640, 480, 16);
#else
    DD_Init_Fullscreen(640, 480, 16);
#endif
    // fill the RGB16 structure with surface info
    GetRGB16(lpddsprimary);
    
    // initialize Directinput
    DInput_Init();
    
    // initialize all input devices
    DI_Init_Keyboard();
    DI_Init_Mouse();
    
    // init data
    for (int y=0; y<MAPHEIGHT; y++)
        for (int x=0; x<MAPWIDTH; x++) {
            CollMap[y][x] = 0;
        }
        
    Start.X = 5;
    Start.Y = 5;
    End.X = 10;
    End.Y = 10;
}

/*
 * InitApp:
 *    Initializes the application window.
 */
BOOL InitApp(HINSTANCE hInst, int nCmdShow)
{
    WNDCLASS WndClass;
    HWND hWnd;
    
    WndClass.style = CS_HREDRAW | CS_VREDRAW;
    WndClass.lpfnWndProc = WinProc;
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0;
    WndClass.hInstance = hInst;
    WndClass.hIcon = LoadIcon(0, IDI_APPLICATION);
    WndClass.hCursor = LoadCursor(0, IDC_ARROW);
    WndClass.hbrBackground = (HBRUSH__ *)GetStockObject(BLACK_BRUSH);
    WndClass.lpszMenuName = 0;
    WndClass.lpszClassName = "BTPath TestApp";
    RegisterClass(&WndClass);
    
#ifndef DOD_WINDOWED
    // create the window
    hWnd = CreateWindowEx(
               WS_EX_TOPMOST,
               "BTPath TestApp",
               "BTPath TestApp",
               WS_POPUP,
               0,0,
               64,
               48,
               NULL,
               NULL,
               hInst,
               NULL);
#else
    // create window
    hWnd = CreateWindow("BTPath TestApp",
                        "BTPath TestApp",
                        WS_VISIBLE | WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        640,
                        480,
                        NULL,
                        NULL,
                        hInst,
                        NULL);
    GetClientRect(hWnd, &rcWindow);
    int width = rcWindow.right - rcWindow.left;
    int height = rcWindow.bottom - rcWindow.top;
    MoveWindow(hWnd, CW_USEDEFAULT, CW_USEDEFAULT, 640+(640-width), 480+(480-height), TRUE);
#endif
               
    if (!hWnd) {
        return FALSE;
    }
    
    // initialize global window variables
    main_window_handle = hWnd;
    main_instance      = hInst;
    
    // Show the Window
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    
    // Initialize the game
    InitGame();
    
    return TRUE;
}

/*
 * Line:
 *      Draws a line of any slope using bresenhams algorithm
 */
void Line(WORD *buf, int x1, int y1, int x2, int y2, WORD color, int pitch)
{
    int dx, dy, sdx, sdy, x, y, px, py;
    
    dx = x2 - x1;
    dy = y2 - y1;
    
    sdx = (dx < 0) ? -1 : 1;
    sdy = (dy < 0) ? -1 : 1;
    
    dx = sdx * dx + 1;
    dy = sdy * dy + 1;
    
    x = y = 0;
    
    px = x1;
    py = y1;
    
    if (dx >= dy) {
        for (x = 0; x < dx; x++) {
            buf[(py * pitch) + px] = color;
            y += dy;
            if (y >= dx) {
                y -= dx;
                py += sdy;
            }
            px += sdx;
        }
    } else {
        for (y = 0; y < dy; y++) {
            buf[(py * pitch) + px] = color;
            x += dx;
            if (x >= dy) {
                x -= dy;
                px += sdx;
            }
            py += sdy;
        }
    }
}

/*
 * DrawGrid:
 *      Draws the map grid
 */
void DrawGrid(void)
{
    int pitch;
    WORD *tmp;
    
    tmp = DD_Lock_Surface(lpddsback, &pitch);
    
    for (int x=0; x<MAPWIDTH; x++) {
        Line(tmp, x*TILESIZE, 0, x*TILESIZE, SCREEN_HEIGHT-1, 63488, pitch);
    }
    
    for (int y=0; y<MAPHEIGHT; y++) {
        Line(tmp, 0, y*TILESIZE, SCREEN_WIDTH-1, y*TILESIZE, 63488, pitch);
    }
    
    DD_Unlock_Surface(lpddsback, tmp);
}

/*
 * DDFillArea:
 *      Fills a dd surface area with color in the specified rect
 */
int DDFillArea(LPDIRECTDRAWSURFACE lpdds, WORD color, RECT *DesRect)
{
    DDBLTFX ddbltfx; // this contains the DDBLTFX structure
    
    // clear out the structure and set the size field
    DD_INIT_STRUCT(ddbltfx);
    
    // set the dwfillcolor field to the desired color
    ddbltfx.dwFillColor = color;
    
    // ready to blt to surface
    lpdds->Blt(DesRect,                           // ptr to dest rectangle
               NULL,                           // ptr to source surface, NA
               NULL,                           // ptr to source rectangle, NA
               DDBLT_COLORFILL | DDBLT_WAIT,   // fill and wait
               &ddbltfx);                      // ptr to DDBLTFX structure
               
    // return success
    return(1);
}

/*
 * DrawMap:
 *      Draws the map
 */
void DrawMap(void)
{
    RECT DesRect;
    
    for (int y=0; y<MAPHEIGHT; y++) {
        for (int x=0; x<MAPWIDTH; x++) {
            if (CollMap[y][x]>0) {
                SetRect(&DesRect, x*TILESIZE+1, y*TILESIZE+1, x*TILESIZE+TILESIZE, y*TILESIZE+TILESIZE);
                DDFillArea(lpddsback, 63488, &DesRect);
            }
        }
    }
    
    SetRect(&DesRect, Start.X*TILESIZE+1, Start.Y*TILESIZE+1, Start.X*TILESIZE+TILESIZE, Start.Y*TILESIZE+TILESIZE);
    if (pRed == 11) {
        DDFillArea(lpddsback, 2016, &DesRect);
    } else {
        DDFillArea(lpddsback, 992, &DesRect);
    }
    
    SetRect(&DesRect, End.X*TILESIZE+1, End.Y*TILESIZE+1, End.X*TILESIZE+TILESIZE, End.Y*TILESIZE+TILESIZE);
    DDFillArea(lpddsback, 31, &DesRect);
}

/*
 * GameMain:
 *      Game Loop
 */
void GameMain(void)
{
    RECT DesRect = { 0,0,640,480 };
    DDFillArea(lpddsback, 0, &DesRect);
    DrawGrid();
    DrawMap();
    FindTheWay(&Start, &End);
    DD_Flip();
}

/*
 * WinMain:
 *    Contains the message loop.
 */
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    
    // initialize the app
    if (!InitApp(hInst, nCmdShow)) {
        return FALSE;
    }
    
    // infinite loop
    while (1) {
        // peekmessage so as not to interrupt processes
        if (PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
            // if the message said to quit, get the hell outta here
            if (msg.message == WM_QUIT) {
                break;
            }
            
            // translate and dispatch the message
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        // if there are no messages, update the game
        if (bActive) {
            GameMain();
        }
    }
    
    // shutdown game and release all resources
    FiniApp();
    
    // return to Windows like this
    return(msg.wParam);
}