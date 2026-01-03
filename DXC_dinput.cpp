// DXC_dinput.cpp: implementation of the DXC_dinput class.
//
//////////////////////////////////////////////////////////////////////

#include "DXC_dinput.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DXC_dinput::DXC_dinput()
{
	m_pDI    = 0;
	m_pMouse = 0;
	m_sX     = 0;
	m_sY     = 0;
	m_sZ     = 0;
	m_hWnd   = 0;
}

DXC_dinput::~DXC_dinput()
{
	if (m_pMouse != 0) {
		m_pMouse->Unacquire();
        m_pMouse->Release();
        m_pMouse = 0;
	}
	if (m_pDI != 0) {
		m_pDI->Release();
        m_pDI = 0;
	}
}

bool DXC_dinput::bInit(HWND hWnd, HINSTANCE hInst)
{
 HRESULT hr;
 DIMOUSESTATE dims;	
 POINT Point;
	RECT rc;
	int clientW, clientH;

	GetCursorPos(&Point);
	m_hWnd   = hWnd;
	// Initialize mouse position in game coordinates (0..799, 0..599)
	if (m_hWnd != 0) {
		POINT ptClient = Point;
		ScreenToClient(m_hWnd, &ptClient);
		GetClientRect(m_hWnd, &rc);
		clientW = max(1, rc.right - rc.left);
		clientH = max(1, rc.bottom - rc.top);
		m_sX = (short)max(0, min(799, (ptClient.x * 800) / clientW));
		m_sY = (short)max(0, min(599, (ptClient.y * 600) / clientH));
	} else {
		m_sX = (short)0;
		m_sY = (short)0;
	}

	hr = DirectInputCreate( hInst, DIRECTINPUT_VERSION, &m_pDI, 0 );
    if (hr != DI_OK) return false;
	hr = m_pDI->CreateDevice( GUID_SysMouse, &m_pMouse, 0 );
	if (hr != DI_OK) return false;
	hr = m_pMouse->SetDataFormat( &c_dfDIMouse );
	if (hr != DI_OK) return false;
	// Non-exclusive improves Alt+Tab behavior in borderless mode
	hr = m_pMouse->SetCooperativeLevel( hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	if (hr != DI_OK) return false;

//	m_pMouse->GetDeviceState( sizeof(DIMOUSESTATE), &dims );
	if ( m_pMouse->GetDeviceState( sizeof(DIMOUSESTATE), &dims ) != DI_OK )
	{
		m_pMouse->Acquire();
		//return true;
	}

	return true;
}


void DXC_dinput::SetAcquire(bool bFlag)
{
 DIMOUSESTATE dims;

	if (m_pMouse == 0) return;
	if (bFlag == true) {
		m_pMouse->Acquire();
		m_pMouse->GetDeviceState( sizeof(DIMOUSESTATE), &dims );
	}
	else m_pMouse->Unacquire();
}

void DXC_dinput::UpdateMouseState(short * pX, short * pY, short * pZ, char * pLB, char * pRB)
{
	if ( m_pMouse->GetDeviceState( sizeof(DIMOUSESTATE), &dims ) != DI_OK )
	{
		m_pMouse->Acquire();
		return;
	}

	// In borderless fullscreen, map OS cursor position to game coordinates.
	if (m_hWnd != 0) {
		POINT pt;
		RECT rc;
		GetCursorPos(&pt);
		ScreenToClient(m_hWnd, &pt);
		GetClientRect(m_hWnd, &rc);
		int clientW = max(1, rc.right - rc.left);
		int clientH = max(1, rc.bottom - rc.top);
		m_sX = (short)max(0, min(799, (pt.x * 800) / clientW));
		m_sY = (short)max(0, min(599, (pt.y * 600) / clientH));
	} else {
		// Fallback to relative movement
		m_sX += (short)dims.lX;
		m_sY += (short)dims.lY;
	}

	if( (short)dims.lZ != 0 )m_sZ = (short)dims.lZ;
	if (m_sX < 0) m_sX = 0;
	if (m_sY < 0) m_sY = 0;
	if (m_sX > 799) m_sX = 799;
	if (m_sY > 599) m_sY = 599;
	*pX = m_sX;
	*pY = m_sY;
	*pZ = m_sZ;
	*pLB = (char)dims.rgbButtons[0];
	*pRB = (char)dims.rgbButtons[1];
}