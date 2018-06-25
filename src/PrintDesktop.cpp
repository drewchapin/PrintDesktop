/*
 * Copyright 2018 Drew Chapin <drew@drewchapin.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include <Windows.h>

#define APPNAME L"PrintDesktop"

bool g_bUseDefaultPrinter = false;

void PrintDesktop()
{

	PRINTDLG pd = {0};
	DOCINFO di = {0};
	int iResX, iResY, iPageX, iPageY;
	HDC hdcScreen, hdcMem;
	HBITMAP hbmScreen;
	DEVMODE *pDevMode;

	//Copy screen to memory bitmap
	hdcScreen = CreateDC(L"DISPLAY",NULL,NULL,NULL);
	hdcMem = CreateCompatibleDC(hdcScreen);
	iResX = GetDeviceCaps(hdcScreen,HORZRES);
	iResY = GetDeviceCaps(hdcScreen,VERTRES);
	hbmScreen = CreateCompatibleBitmap(hdcScreen,iResX,iResY);
	SelectObject(hdcMem, hbmScreen);
	if( !BitBlt(hdcMem,0,0,iResX,iResY,hdcScreen,0,0,SRCCOPY) )
	{
		MessageBox( NULL, L"There was an error capturing an image of the desktop.", APPNAME, MB_OK | MB_ICONERROR | MB_SYSTEMMODAL );
		return;
	}

	//Show the print selection dialog
	pd.lStructSize = sizeof(pd);
	pd.Flags = PD_RETURNDC;
	di.cbSize = sizeof(di);
	di.lpszDocName = APPNAME;

	// Use default?
	if( g_bUseDefaultPrinter )
		pd.Flags |= PD_RETURNDEFAULT;

	// Show dialog, or get default printer
	if( PrintDlg(&pd) )
	{
		//Set default to landscape
		pDevMode = (DEVMODE*)GlobalLock(pd.hDevMode);
		pDevMode->dmOrientation = DMORIENT_LANDSCAPE;
		ResetDC( pd.hDC, pDevMode );
		GlobalUnlock( pd.hDevMode );

		//Make sure the selected printer can print bitmaps
		if( !(GetDeviceCaps(pd.hDC,RASTERCAPS) & RC_STRETCHBLT) )
		{
			DeleteDC( pd.hDC );
			MessageBox( NULL, L"Unfortunately, the printer you selected can not be used to print the desktop.", APPNAME, MB_OK | MB_ICONERROR | MB_SYSTEMMODAL );
			return;
		}

		//Get document size
		iPageX = GetDeviceCaps(pd.hDC,HORZRES);
		iPageY = GetDeviceCaps(pd.hDC,VERTRES);

		// Create document
		if( !StartDoc(pd.hDC,&di) )
		{
			MessageBox( NULL, L"There was an error starting a new document on the selected printer.", APPNAME, MB_OK | MB_ICONERROR | MB_SYSTEMMODAL );
			return;
		}

		if( !StartPage(pd.hDC) )
		{
			AbortDoc( pd.hDC );
			MessageBox( NULL, L"There was an error starting a new document on the selected printer.", APPNAME, MB_OK | MB_ICONERROR | MB_SYSTEMMODAL );
			return;
		}

		// Render screenshot
		if( !StretchBlt(pd.hDC,0,0,iPageX,iPageY,hdcMem,0,0,iResX,iResY,SRCCOPY) )
		{
			EndPage( pd.hDC );
			AbortDoc( pd.hDC );
			MessageBox( NULL, L"There was an error printing the desktop to the page.", APPNAME, MB_OK | MB_ICONERROR | MB_SYSTEMMODAL );
			return;
		}

		EndPage( pd.hDC );
		EndDoc( pd.hDC );
		DeleteDC( pd.hDC );

	}

	// Free up memory
	if( NULL != pd.hDevMode )
		GlobalFree( pd.hDevMode );
	if( NULL != pd.hDevNames )
		GlobalFree( pd.hDevNames );

	// Release handles
	DeleteDC( hdcMem );
	DeleteObject( hbmScreen );
	ReleaseDC( NULL, hdcScreen );

}

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow )
{


	// Use default printer?
	LPWSTR* szArgs;
	int nArgs;
	szArgs = CommandLineToArgvW(GetCommandLine(),&nArgs);
	if( nArgs == 2 && 0 == wcsicmp(szArgs[1],L"-d") )
		g_bUseDefaultPrinter = true;
	LocalFree(szArgs);

	// Setup PrntScrn Key
	if( !RegisterHotKey(NULL,1,0,VK_SNAPSHOT) ) 
	{
		MessageBox( NULL, L"There was an error registering the hotkey for the PrntScrn key. PrintDesktop may already be running.", APPNAME, MB_OK | MB_ICONERROR | MB_SYSTEMMODAL );
		return 1;
	} 

	// Ctrl+P Hotkey
	if( !RegisterHotKey(NULL,2,MOD_CONTROL,'P') ) 
	{
		MessageBox( NULL, L"There was an error registering the hotkey for Ctrl+P.", APPNAME, MB_OK | MB_ICONERROR | MB_SYSTEMMODAL );
		return 1;
	} 
	
	// Message loop
	MSG msg = {0};
	while( 0 != GetMessage(&msg,NULL,0,0) )
	{
		if( WM_HOTKEY == msg.message )
		{
			if( 1 == msg.wParam || 2 == msg.wParam )
				PrintDesktop();
		}
	}

	return 0;

}

