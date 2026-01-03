// -------------------------------------------------------------- 
//                      Helbreath Client 						  
//
//                      1998.10 by Soph
//
//         Modernized for Windows 10/11 compatibility
//         High-precision timing and FPS limiter added
//
// --------------------------------------------------------------


#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h> 
#include <winbase.h>
#include <mmsystem.h>
#include <process.h>
#include "resource.h"
#include "XSocket.h"
#include "winmain.h"
#include "Game.h"
#include "GlobalDef.h"
#include "ModernEngine.h"

extern "C" __declspec( dllimport) int __FindHackingDll__(char *);

// --------------------------------------------------------------

#define WM_USER_TIMERSIGNAL		WM_USER + 500
#define WM_USER_CALCSOCKETEVENT WM_USER + 600

int				G_iAddTable31[64][510], G_iAddTable63[64][510]; 
int				G_iAddTransTable31[510][64], G_iAddTransTable63[510][64]; 

long    G_lTransG100[64][64], G_lTransRB100[64][64];
long    G_lTransG70[64][64], G_lTransRB70[64][64];
long    G_lTransG50[64][64], G_lTransRB50[64][64];
long    G_lTransG25[64][64], G_lTransRB25[64][64];
long    G_lTransG2[64][64], G_lTransRB2[64][64];

char			szAppClass[32];
HWND			G_hWnd = 0;
HWND			G_hEditWnd = 0;
HINSTANCE       G_hInstance = 0;
MMRESULT		G_mmTimer;
char   G_cSpriteAlphaDegree;
class CGame * G_pGame;
class XSocket * G_pCalcSocket = 0;
bool  G_bIsCalcSocketConnected = true;
DWORD G_dwCalcSocketTime = 0, G_dwCalcSocketSendTime = 0;

char G_cCmdLine[256], G_cCmdLineTokenA[120], G_cCmdLineTokenA_Lowercase[120], G_cCmdLineTokenB[120], G_cCmdLineTokenC[120], G_cCmdLineTokenD[120], G_cCmdLineTokenE[120];


// --------------------------------------------------------------

LRESULT CALLBACK WndProc(HWND hWnd,UINT message,WPARAM wParam, LPARAM lParam)
{ 
	if(G_pGame->GetText( hWnd, message, wParam, lParam)) return 0;

	switch (message) {
	case WM_USER_CALCSOCKETEVENT:
		G_pGame->_CalcSocketClosed();
		break;
	
	case WM_CLOSE:
		if ( (G_pGame->m_cGameMode == DEF_GAMEMODE_ONMAINGAME) && ( G_pGame->m_bForceDisconn == false ) )
		{

#ifdef _DEBUG
			if (G_pGame->m_cLogOutCount == -1 || G_pGame->m_cLogOutCount > 2) G_pGame->m_cLogOutCount = 1; 
#else
			if (G_pGame->m_cLogOutCount == -1 || G_pGame->m_cLogOutCount > 11) G_pGame->m_cLogOutCount = 11; 
#endif

		}
		else if (G_pGame->m_cGameMode == DEF_GAMEMODE_ONLOADING) return (DefWindowProc(hWnd, message, wParam, lParam));
		else if (G_pGame->m_cGameMode == DEF_GAMEMODE_ONMAINMENU) G_pGame->ChangeGameMode(DEF_GAMEMODE_ONQUIT);
		break;
	
	case WM_SYSCOMMAND:
		if((wParam&0xFFF0)==SC_SCREENSAVE || (wParam&0xFFF0)==SC_MONITORPOWER) 
			return 0; 
		return DefWindowProc(hWnd, message, wParam, lParam);
			
	case WM_USER_TIMERSIGNAL:
		G_pGame->OnTimer();
		break;

	case WM_KEYDOWN:
		G_pGame->OnKeyDown(wParam);
		return (DefWindowProc(hWnd, message, wParam, lParam));
		
	case WM_KEYUP:
		G_pGame->OnKeyUp(wParam);
		return (DefWindowProc(hWnd, message, wParam, lParam));

	case WM_SYSKEYDOWN:
		G_pGame->OnSysKeyDown(wParam);
		return (DefWindowProc(hWnd, message, wParam, lParam));
		break;

	case WM_SYSKEYUP:
		G_pGame->OnSysKeyUp(wParam);
		return (DefWindowProc(hWnd, message, wParam, lParam));
		break;

	case WM_ACTIVATEAPP:
		if( wParam == 0 ) 
		{	G_pGame->m_bIsProgramActive = false;
			G_pGame->m_DInput.SetAcquire(false);
		}else 
		{	G_pGame->m_bIsProgramActive = true;
			G_pGame->m_DInput.SetAcquire(true);
			G_pGame->m_bCtrlPressed = false;
			
			G_pGame->m_bIsRedrawPDBGS = true;
			G_pGame->m_DDraw.ChangeDisplayMode(G_hWnd);

			if (G_pGame->bCheckImportantFile() == false) 
			{	MessageBox(G_pGame->m_hWnd, "File checksum error! Get Update again please!", "ERROR1", MB_ICONEXCLAMATION | MB_OK);
				PostQuitMessage(0);
				return 0;
			}			
			if (__FindHackingDll__("CRCCHECK") != 1) 
			{	G_pGame->ChangeGameMode(DEF_GAMEMODE_ONQUIT);
				return 0;
		}	}
		return DefWindowProc(hWnd, message, wParam, lParam);

	case WM_SETCURSOR:
		SetCursor(0);
		return true;

	case WM_DESTROY:
		OnDestroy();
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
		
	case WM_USER_GAMESOCKETEVENT:
		G_pGame->OnGameSocketEvent(wParam, lParam);
		break;

	case WM_USER_LOGSOCKETEVENT:
		G_pGame->OnLogSocketEvent(wParam, lParam);
		break;
		
	default: 
		return (DefWindowProc(hWnd, message, wParam, lParam));
	}	
	return 0;
}

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
               LPSTR lpCmdLine, int nCmdShow )
{HINSTANCE hDll;
 char cSearchDll[] = "rd`qbg-ckk";
 char cRealName[12];

	// Initialize modern engine systems (DPI awareness, priority, etc.)
	InitializeModernEngine();

	srand((unsigned)time(0));
	char *pJammer = new char[(rand() % 100) +1];
	G_pGame = new class CGame;
	ZeroMemory(cRealName, sizeof(cRealName));
	strcpy(cRealName, cSearchDll);
	for (WORD i = 0; i < strlen(cRealName); i++)
	if (cRealName[i] != 0) cRealName[i]++;

	hDll = LoadLibrary(cRealName);
	if( hDll == 0 ) 
	{	MessageBox(0, "don't find search.dll", "ERROR!", MB_OK);
		return 0;
	}

#ifdef DEF_USING_WIN_IME
	HINSTANCE hRichDll = LoadLibrary( "Riched20.dll" );
#endif

	typedef int (MYPROC)(char *) ;
	MYPROC *pFindHook; 
	pFindHook = (MYPROC *) GetProcAddress(hDll, "__FindHackingDll__") ;

	if (pFindHook== 0) 
	{	MessageBox(0, "can't find search.dll", "ERROR!", MB_OK);
		return 0 ;
	}else if ((*pFindHook)("CRCCHECK") != 1) 
	{	return 0 ;
	}
	FreeLibrary(hDll);
	sprintf( szAppClass, "Client-I%d", hInstance);
	if (!InitApplication( hInstance))		return (false);
    if (!InitInstance(hInstance, nCmdShow)) return (false);

	Initialize((char *)lpCmdLine);

#ifndef _DEBUG
	if (OpenMutex(MUTEX_ALL_ACCESS, false, "0543kjg3j31%") != 0) {
		MessageBox(0, "Only one Helbreath client program allowed!", "ERROR!", MB_OK);
		return 0;
	}
	HANDLE hMutex = CreateMutex(0, false, "0543kjg3j31%");
#endif
	
	EventLoop();

#ifndef _DEBUG
	ReleaseMutex(hMutex);
	CloseHandle(hMutex);
#endif

	// Cleanup modern engine systems
	ShutdownModernEngine();

	delete[] pJammer;
	delete G_pGame;

#ifdef DEF_USING_WIN_IME
	FreeLibrary(hRichDll);
#endif

	return 0;
}

BOOL InitApplication( HINSTANCE hInstance)
{WNDCLASS  wc;
	wc.style = (CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS);
	wc.lpfnWndProc   = (WNDPROC)WndProc;             
	wc.cbClsExtra    = 0;                            
	wc.cbWndExtra    = sizeof (int);
	wc.hInstance     = hInstance;
	wc.hIcon         = 0;
	wc.hCursor       = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = szAppClass;        
	return (RegisterClass(&wc));
}

bool InitInstance( HINSTANCE hInstance, int nCmdShow )
{	// Get primary monitor info
	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	HMONITOR hMonitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY);
	GetMonitorInfo(hMonitor, &mi);
	
	int monitorWidth = mi.rcMonitor.right - mi.rcMonitor.left;
	int monitorHeight = mi.rcMonitor.bottom - mi.rcMonitor.top;
	int cx = mi.rcMonitor.left + monitorWidth / 2;
	int cy = mi.rcMonitor.top + monitorHeight / 2;
	if(cy - mi.rcMonitor.top > 340) cy -= 40;
	
	G_hWnd = CreateWindowEx(0, szAppClass, "Helbreath", WS_POPUP, cx - 400, cy - 300,
							800, 600, 0, 0, hInstance, 0);  
    if (!G_hWnd) return false;
    G_hInstance	= hInstance;
	ShowWindow(G_hWnd, SW_SHOWDEFAULT);
	UpdateWindow(G_hWnd);
	return true;
}


void EventLoop()
{ 
    MSG msg;
    
    // Initialize high-precision timer for accurate frame timing
    g_HighPrecisionTimer.Reset();
    
    while( true ) 
    {
        // Process all pending Windows messages first
        while( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) ) 
        {
            if( msg.message == WM_QUIT ) return;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        if (G_pGame->m_bIsProgramActive) 
        {
            // Wait for next frame (FPS limiter with CPU optimization)
            g_FPSLimiter.WaitForNextFrame();
            G_pGame->UpdateScreen();
        }
        else if (G_pGame->m_cGameMode == DEF_GAMEMODE_ONLOADING) 
        {
            G_pGame->UpdateScreen_OnLoading( false );
        }
        else 
        {
            // When inactive, use WaitMessage to minimize CPU usage
            // But add a small timeout to remain responsive
            MsgWaitForMultipleObjects(0, NULL, FALSE, 100, QS_ALLINPUT);
        }
    }
}

void OnDestroy()
{	G_pGame->m_bIsProgramActive = false;		
	_StopTimer(G_mmTimer);
	G_pGame->Quit();
	WSACleanup();
	PostQuitMessage(0);
}

void CALLBACK _TimerFunc(UINT wID, UINT wUser, DWORD dwUSer, DWORD dw1, DWORD dw2)
{	PostMessage(G_hWnd, WM_USER_TIMERSIGNAL, wID, 0);
}


MMRESULT _StartTimer(DWORD dwTime)
{TIMECAPS caps;
	timeGetDevCaps(&caps, sizeof(caps));
	timeBeginPeriod(caps.wPeriodMin);
	return timeSetEvent(dwTime,0,_TimerFunc,0, (UINT)TIME_PERIODIC);
}


void _StopTimer(MMRESULT timerid)
{TIMECAPS caps;
	if (timerid != 0) 
	{	timeKillEvent(timerid);
		timerid = 0;
		timeGetDevCaps(&caps, sizeof(caps));
		timeEndPeriod(caps.wPeriodMin);
	}
}


void Initialize(char * pCmdLine)
{int iX, iY, iSum;
 int     iErrCode;
 WORD	 wVersionRequested;
 WSADATA wsaData;
	wVersionRequested = MAKEWORD( 2, 2 ); 
	iErrCode = WSAStartup( wVersionRequested, &wsaData );
	if ( iErrCode ) 
	{	MessageBox(G_hWnd, "Winsock-V1.1 not found! Cannot execute program.","ERROR",MB_ICONEXCLAMATION | MB_OK);
		PostQuitMessage(0);
		return;
	}
	if (G_pGame->bInit(G_hWnd, G_hInstance, pCmdLine) == false) 
	{	PostQuitMessage(0);
		return;
	}	
	G_mmTimer = _StartTimer(1000);
	for (iX = 0; iX < 64; iX++)
	for (iY = 0; iY < 510; iY++) 
	{	iSum = iX + (iY - 255);
		if (iSum <= 0)  iSum = 1;
		if (iSum >= 31) iSum = 31;
		G_iAddTable31[iX][iY] = iSum; 
		iSum = iX + (iY - 255);
		if (iSum <= 0)  iSum = 1;
		if (iSum >= 63) iSum = 63;
		G_iAddTable63[iX][iY] = iSum; 
		if ((iY - 255) < iX) G_iAddTransTable31[iY][iX] = iX;
		else if ((iY - 255) > 31) G_iAddTransTable31[iY][iX] = 31;
		else G_iAddTransTable31[iY][iX] = iY-255;
		if ((iY - 255) < iX) G_iAddTransTable63[iY][iX] = iX;
		else if ((iY - 255) > 63) G_iAddTransTable63[iY][iX] = 63;
		else G_iAddTransTable63[iY][iX] = iY-255;
	}
}

LONG GetRegKey(HKEY key, LPCTSTR subkey, LPTSTR retdata)
{   HKEY hkey;
    LONG retval = RegOpenKeyEx(key, subkey, 0, KEY_QUERY_VALUE, &hkey);
    if (retval == ERROR_SUCCESS) 
	{	long datasize = MAX_PATH;
        TCHAR data[MAX_PATH];
        RegQueryValue(hkey, 0, data, &datasize);
        lstrcpy(retdata,data);
        RegCloseKey(hkey);
    }
    return retval;
}

void GoHomepage() 
{	LPCTSTR	url = MSG_HOMEPAGE;

#ifdef _DEBUG
	return;
#endif

	int		showcmd = SW_SHOW;
	char	key[MAX_PATH + MAX_PATH];	
    // First try ShellExecute()
    HINSTANCE result = ShellExecute(0, "open", url, 0,0, showcmd);

    // If it failed, get the .htm regkey and lookup the program
    if ((UINT)result <= HINSTANCE_ERROR) 
	{  if (GetRegKey(HKEY_CLASSES_ROOT, ".htm", key) == ERROR_SUCCESS) 
		{  lstrcat(key, "\\shell\\open\\command");
            if (GetRegKey(HKEY_CLASSES_ROOT,key,key) == ERROR_SUCCESS)
			{   char *pos;
                pos = strstr(key, "\"%1\"");
                if (pos == 0) {                     // No quotes found
                    pos = strstr(key, "%1");           // Check for %1, without quotes 
                    if (pos == 0)                   // No parameter at all...
                          pos = key+lstrlen(key)-1;
                    else *pos = '\0';                   // Remove the parameter
                }else    *pos = '\0';                   // Remove the parameter
                lstrcat(pos, " ");
                lstrcat(pos, url);
                result = (HINSTANCE) WinExec(key,showcmd);
    }	}	}
}
////////////////////////////////////////////////////////////////////////
