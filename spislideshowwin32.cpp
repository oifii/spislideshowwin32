/*
 * Copyright (c) 2016-2017 Stephane Poirier
 *
 * stephane.poirier@oifii.org
 *
 * Stephane Poirier
 * 3532 rue Ste-Famille, #3
 * Montreal, QC, H2X 2L1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// spislideshowwin32.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "spislideshowwin32.h"
#include <mmsystem.h> //for timeSetEvent()
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include "FreeImage.h"
#include <cstdlib>     /* srand, rand */
#include <ctime>       /* time */
using namespace std;


#define MAX_LOADSTRING 1024
#define MAX_PRELOADEDIMAGE 10

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING]={"spislideshowwin32title"};					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING]={"spislideshowwin32class"};			// the main window class name

//new parameters
//string global_begin="begin.ahk";
//string global_end="end.ahk";
string global_begin="";
string global_end="";

int global_randomstartflag=0; //0 to start at begining, 1 to start with random offset 

int global_titlebardisplay=1; //0 for off, 1 for on
int global_acceleratoractive=0; //0 for off, 1 for on
int global_menubardisplay=0; //0 for off, 1 for on

DWORD global_startstamp_ms;
string global_imagefolder="."; //the local folder of the .scr
string global_imagefilefilter="*.jpg"; 
int global_imagetype = -1;
const UINT_PTR IDT_DISPLAYTIMER=1;
bool global_aborting=false;
//string global_imagefolder="c:\\temp\\spivideocaptureframetodisk\\";
//string global_imagefolder="c:\\temp\\spissmandelbrotmidi\\";
//string global_imagefolder="c:\\temp\\spicapturewebcam\\";
float global_duration_sec=180;
//float global_sleeptimeperload_sec=1.0/30.0;
float global_sleeptimeperload_sec=1.0/30.0;
float global_displayrate_sec=1.0/5.0;
int global_x=100;
int global_y=100;
int global_xwidth=300;
int global_yheight=300;
BYTE global_alpha=255;

int global_imageid=0;
HWND global_hwnd=NULL;
MMRESULT global_timer=0;
int global_imageheight=-1; //will be computed within WM_SIZE handler
int global_imagewidth=-1; //will be computed within WM_SIZE handler 
vector<string> global_txtfilenames;
list<FIBITMAP*> global_preloadedimageslist;

float RandomFloat(float a, float b)
{
	float random = ((float)rand()) / (float)RAND_MAX;
	float diff = b - a;
	float r = random * diff;
	return a + r;
}

int RandomInt(int lowest, int highest)
{
	int range = (highest - lowest) + 1;
	int random_integer = lowest + int(range*rand() / (RAND_MAX + 1.0));
	return random_integer;
}

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

// Convert a wide Unicode string to an UTF8 string
std::string utf8_encode(const std::wstring &wstr)
{
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo( size_needed, 0 );
    WideCharToMultiByte                  (CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// Convert an UTF8 string to a wide Unicode String
std::wstring utf8_decode(const std::string &str)
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo( size_needed, 0 );
    MultiByteToWideChar                  (CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}


void CALLBACK StartGlobalProcess(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	//WavSetLib_Initialize(global_hwnd, IDC_MAIN_STATIC, global_staticwidth, global_staticheight, global_fontwidth, global_fontheight, global_staticalignment);

	DWORD nowstamp_ms = GetTickCount();
	while( !global_aborting && ((global_duration_sec<0.0f) || ((nowstamp_ms-global_startstamp_ms)/1000.0f)<global_duration_sec) )
	{

		if(global_txtfilenames.size()>0)
		{
			//reset image id if end of file
			if(global_imageid>=global_txtfilenames.size()) global_imageid=0;

			if(global_preloadedimageslist.size()<MAX_PRELOADEDIMAGE)
			{
				//load new image
				//global_dib = FreeImage_Load(FIF_BMP, (global_txtfilenames[global_imageid]).c_str(), BMP_DEFAULT);

				FIBITMAP* dib = NULL;
				if(global_imagetype==0)
				{
					dib = FreeImage_Load(FIF_JPEG, (global_txtfilenames[global_imageid]).c_str(), JPEG_DEFAULT);
				}
				else if(global_imagetype==1)
				{
					dib = FreeImage_Load(FIF_BMP, (global_txtfilenames[global_imageid]).c_str(), BMP_DEFAULT);
				}
				else if(global_imagetype==2)
				{
					dib = FreeImage_Load(FIF_TIFF, (global_txtfilenames[global_imageid]).c_str(), TIFF_DEFAULT);
				}
				if(dib) 
				{
					global_preloadedimageslist.push_back(dib);
					global_imageid++;
				}
				/* //this is not the proper fix for preventing freeze when spislideshowwin32 used in multiple instances all sharing the same image folder
				//spi, 2016oct28 fix, begin
				else
				{
					global_imageid++; //jump frame
				}
				//spi, 2016oct28 fix, end
				*/
			}

			//global_imageid++;

		}
		Sleep((int)(global_sleeptimeperload_sec*1000));
		//StatusAddText(global_line.c_str());

		nowstamp_ms = GetTickCount();
	}
	if(!global_aborting) PostMessage(global_hwnd, WM_DESTROY, 0, 0);
}




PCHAR*
    CommandLineToArgvA(
        PCHAR CmdLine,
        int* _argc
        )
    {
        PCHAR* argv;
        PCHAR  _argv;
        ULONG   len;
        ULONG   argc;
        CHAR   a;
        ULONG   i, j;

        BOOLEAN  in_QM;
        BOOLEAN  in_TEXT;
        BOOLEAN  in_SPACE;

        len = strlen(CmdLine);
        i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

        argv = (PCHAR*)GlobalAlloc(GMEM_FIXED,
            i + (len+2)*sizeof(CHAR));

        _argv = (PCHAR)(((PUCHAR)argv)+i);

        argc = 0;
        argv[argc] = _argv;
        in_QM = FALSE;
        in_TEXT = FALSE;
        in_SPACE = TRUE;
        i = 0;
        j = 0;

        while( a = CmdLine[i] ) {
            if(in_QM) {
                if(a == '\"') {
                    in_QM = FALSE;
                } else {
                    _argv[j] = a;
                    j++;
                }
            } else {
                switch(a) {
                case '\"':
                    in_QM = TRUE;
                    in_TEXT = TRUE;
                    if(in_SPACE) {
                        argv[argc] = _argv+j;
                        argc++;
                    }
                    in_SPACE = FALSE;
                    break;
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    if(in_TEXT) {
                        _argv[j] = '\0';
                        j++;
                    }
                    in_TEXT = FALSE;
                    in_SPACE = TRUE;
                    break;
                default:
                    in_TEXT = TRUE;
                    if(in_SPACE) {
                        argv[argc] = _argv+j;
                        argc++;
                    }
                    _argv[j] = a;
                    j++;
                    in_SPACE = FALSE;
                    break;
                }
            }
            i++;
        }
        _argv[j] = '\0';
        argv[argc] = NULL;

        (*_argc) = argc;
        return argv;
    }


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	global_startstamp_ms = GetTickCount();

	//LPWSTR *szArgList;
	LPSTR *szArgList;
	int nArgs;

	//szArgList = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	szArgList = CommandLineToArgvA(GetCommandLineA(), &nArgs);
	if( NULL == szArgList )
	{
		//wprintf(L"CommandLineToArgvW failed\n");
		return FALSE;
	}
	if(nArgs>1)
	{
		global_imagefolder = szArgList[1];
	}
	if(nArgs>2)
	{
		global_imagefilefilter = szArgList[2];
	}
	if(nArgs>3)
	{
		global_duration_sec = atof(szArgList[3]);
	}
	if(nArgs>4)
	{
		global_sleeptimeperload_sec = atof(szArgList[4]);
	}
	if(nArgs>5)
	{
		global_displayrate_sec = atof(szArgList[5]);
	}
	if(nArgs>6)
	{
		global_x = atoi(szArgList[6]);
	}
	if(nArgs>7)
	{
		global_y = atoi(szArgList[7]);
	}
	if(nArgs>8)
	{
		global_xwidth = atoi(szArgList[8]);
	}
	if(nArgs>9)
	{
		global_yheight = atoi(szArgList[9]);
	}
	if(nArgs>10)
	{
		global_alpha = atoi(szArgList[10]);
	}
	if(nArgs>11)
	{
		global_titlebardisplay = atoi(szArgList[11]);
	}
	if(nArgs>12)
	{
		global_menubardisplay = atoi(szArgList[12]);
	}
	if(nArgs>13)
	{
		global_acceleratoractive = atoi(szArgList[13]);
	}
	//new parameters
	if(nArgs>14)
	{
		strcpy(szWindowClass, szArgList[14]); 
	}
	if(nArgs>15)
	{
		strcpy(szTitle, szArgList[15]); 
	}
	if(nArgs>16)
	{
		global_begin = szArgList[16]; 
	}
	if(nArgs>17)
	{
		global_end = szArgList[17]; 
	}

	if(nArgs>18)
	{
		global_randomstartflag = atoi(szArgList[18]);
	}

	LocalFree(szArgList);


	int nShowCmd = false;
	//ShellExecuteA(NULL, "open", "begin.bat", "", NULL, nShowCmd);
	ShellExecuteA(NULL, "open", global_begin.c_str(), "", NULL, nCmdShow);

	//dynamically detect image type
	if( (global_imagefilefilter.find(".jpg")!=string::npos) || 
		(global_imagefilefilter.find(".JPG")!=string::npos) || 
		(global_imagefilefilter.find(".jpeg")!=string::npos) || 
		(global_imagefilefilter.find(".JPEG")!=string::npos) )
	{
		global_imagetype = 0;
	}
	else if( (global_imagefilefilter.find(".bmp")!=string::npos) || 
			 (global_imagefilefilter.find(".BMP")!=string::npos) )
	{
		global_imagetype = 1;
	}
	else if( (global_imagefilefilter.find(".tiff")!=string::npos) || 
			 (global_imagefilefilter.find(".TIFF")!=string::npos) || 
			 (global_imagefilefilter.find(".tif")!=string::npos) || 
			 (global_imagefilefilter.find(".TIF")!=string::npos) )
	{
		global_imagetype = 2;
	}
	else
	{
		global_imagetype = 0;
	}

	//////////////////////////
	//initialize random number
	//////////////////////////
	//srand(time(NULL));
	srand(GetTickCount());

	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	//LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	//LoadString(hInstance, IDC_SPISLIDESHOWWIN32, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	if(global_acceleratoractive)
	{
		hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SPISLIDESHOWWIN32));
	}
	else
	{
		hAccelTable = NULL;
	}
	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SPISLIDESHOWWIN32));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);

	if(global_menubardisplay)
	{
		wcex.lpszMenuName = MAKEINTRESOURCE(IDC_SPISLIDESHOWWIN32);
	}
	else
	{
		wcex.lpszMenuName = NULL; //no menu
	}
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

	//global_hFont=CreateFontW(global_fontheight,0,0,0,FW_NORMAL,0,0,0,0,0,0,2,0,L"SYSTEM_FIXED_FONT");
	//global_hFont=CreateFontW(global_fontheight,0,0,0,FW_NORMAL,0,0,0,0,0,0,2,0,L"Segoe Script");

	if(global_titlebardisplay)
	{
		hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, //original with WS_CAPTION etc.
			global_x, global_y, global_xwidth, global_yheight, NULL, NULL, hInstance, NULL);
	}
	else
	{
		hWnd = CreateWindow(szWindowClass, szTitle, WS_POPUP | WS_VISIBLE, //no WS_CAPTION etc.
			global_x, global_y, global_xwidth, global_yheight, NULL, NULL, hInstance, NULL);
	}
	if (!hWnd)
	{
		return FALSE;
	}
	global_hwnd = hWnd;

	SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	SetLayeredWindowAttributes(hWnd, 0, global_alpha, LWA_ALPHA);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	//SetTimer(hWnd, IDT_DISPLAYTIMER, (1.0/5.0)*1000, NULL);
	SetTimer(hWnd, IDT_DISPLAYTIMER, (global_displayrate_sec)*1000, NULL);

	//global_timer=timeSetEvent(100,25,(LPTIMECALLBACK)&StartGlobalProcess,0,TIME_ONESHOT);
	global_timer=timeSetEvent(25,25,(LPTIMECALLBACK)&StartGlobalProcess,0,TIME_ONESHOT);
	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_CREATE:
		{
			//0) execute cmd line to get all folder's .bmp filenames
			string quote = "\"";
			string pathfilter;
			string path=global_imagefolder;
			//pathfilter = path + "\\*.bmp";
			//pathfilter = path + "\\*.jpg";
			pathfilter = path + "\\" + global_imagefilefilter;
			string systemcommand;
			//systemcommand = "DIR " + quote + pathfilter + quote + "/B /O:N > wsic_filenames.txt"; //wsip tag standing for wav set (library) instrumentset (class) populate (function)
			systemcommand = "DIR " + quote + pathfilter + quote + "/B /S /O:N > spislideshow_filenames.txt"; // /S for adding path into "wsic_filenames.txt"
			system(systemcommand.c_str());

			//2) load in all "spiss_filenames.txt" file
			//vector<string> global_txtfilenames;
			ifstream ifs("spislideshow_filenames.txt");
			string temp;
			while(getline(ifs,temp))
			{
				//txtfilenames.push_back(path + "\\" + temp);
				global_txtfilenames.push_back(temp);
			}

			//3)
			if(global_randomstartflag==1)
			{
				if(global_txtfilenames.size()>1)
				{
					global_imageid = RandomInt(0, global_txtfilenames.size()-1);
				}
			}
			
		}
		break;
	case WM_SIZE:
		{
			RECT rcClient;
			GetClientRect(hWnd, &rcClient);

			global_imagewidth = rcClient.right - 0;
			global_imageheight = rcClient.bottom - 0; 
		}
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		{
			hdc = BeginPaint(hWnd, &ps);

			if(!global_preloadedimageslist.empty() && !global_aborting)
			{
				FIBITMAP* dib = global_preloadedimageslist.front();
				global_preloadedimageslist.pop_front();
				if(dib)
				{
					SetStretchBltMode(hdc, COLORONCOLOR);
					//StretchDIBits(hdc, 0, 0, global_imagewidth, global_imageheight,
					//				0, 0, FreeImage_GetWidth(global_dib), FreeImage_GetHeight(global_dib),
					//				FreeImage_GetBits(global_dib), FreeImage_GetInfo(global_dib), DIB_RGB_COLORS, SRCCOPY);
					StretchDIBits(hdc, 0, 0, global_imagewidth, global_imageheight,
									0, 0, FreeImage_GetWidth(dib), FreeImage_GetHeight(dib),
									FreeImage_GetBits(dib), FreeImage_GetInfo(dib), DIB_RGB_COLORS, SRCCOPY);
					FreeImage_Unload(dib);
				}
				//global_preloadedimageslist.pop_front();
			}
			/* //this is not the proper fix for preventing freeze when spislideshowwin32 used in multiple instances all sharing the same image folder
			//spi, 2016oct28 fix, begin
			else
			{
				global_imageid++; //jump one frame
			}
			//spi, 2016oct28 fix, end
			*/
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_DESTROY:
		{
			//kill all timers
			global_aborting=true;
			KillTimer(hWnd, IDT_DISPLAYTIMER); 
			if (global_timer) timeKillEvent(global_timer);
			Sleep((global_displayrate_sec)*1000);
			//unload all preloaded images
			for(list<FIBITMAP*>::iterator iterator = global_preloadedimageslist.begin(); iterator != global_preloadedimageslist.end(); iterator++)
			{
				FIBITMAP* dib = *iterator;
				if(dib!=NULL) FreeImage_Unload(dib);
			}		
			int nShowCmd = false;
			//ShellExecuteA(NULL, "open", "end.bat", "", NULL, nShowCmd);
			ShellExecuteA(NULL, "open", global_end.c_str(), "", NULL, 0);
			PostQuitMessage(0);
		}
		break;
	case WM_TIMER:
		switch(wParam)
		{
			case IDT_DISPLAYTIMER:
				InvalidateRect(hWnd, NULL, false);
				break;
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
