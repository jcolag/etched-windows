#define	FD_SETSIZE		2048

#include <windows.h>
#include <commdlg.h>
#include <time.h>
#include <stdlib.h>
#include <winsock.h>
#include <shellapi.h>
#include <shlobj.h>

#include "etchrc.h"
#include "dial.h"

#define	DEFAULT_PORT	2342
#define	POLLSOCK			2342
#define	DIALTIMER		2814
#define	SOCK_ORIG		123

#define	WM_MOUSEWHEEL	(WM_MOUSELAST + 1)

#define	CM_A_FILE		(WM_USER + 500)

long			FAR PASCAL _export WndProc (HWND, UINT, UINT, LONG);
long			FAR PASCAL _export SimpleProc (HWND, UINT, UINT, LONG);
BOOL			CALLBACK AboutProc (HWND, UINT, WPARAM, LPARAM);
void			Shake (HWND);
int			GetFromSocket (int, char *, int);
void			ChangeTitle (HWND, char *);
int			ReadEtchFile (HANDLE, LPSTR, HGLOBAL, HWND);
void			WriteDXF (HANDLE);
int			GetRegValue (HKEY, LPCSTR, LPCSTR, LPBYTE, DWORD);
int			GetFormattedError (LPTSTR, int, int);
DWORD			ToRGB (char *);
BOOL			AddMRU (HMENU, int, LPSTR);
void			MRUtoFile (HMENU, int, LPSTR);
void			MRUfromFile (HMENU, int, LPSTR);
int			GetStringFromMenuID (HMENU, LONG, LPSTR, int);
HGLOBAL		LibInterpret (LPSTR, LPSTR);

typedef BOOL (FAR PASCAL intfunc)(HGLOBAL, HGLOBAL);

struct	dest
		{
		 long	ip;
		 short	port;
		};

int			 sock,
				 nIPs = 2,
				 iTimeout,
				 iLenMRU;
long			 histlen = 1,
				 histmax = 8192;
DWORD			 localhost = 0;
short			 flagCenter = 1,
				 flagShake = 1,
				 flagModified = 0,
				 flagAsk = 1,
				 flagSockFail = 0,
				 flagWheel = 0,
				 port = 0;
char			 szInFilter[256] = "Etch-A-Sketch Files (*.eas)\000*.eas\000All Files (*.*)\000*.*\000\000",
				 szOutFilter[256] = "Etch-A-Sketch Files (*.eas)\000*.eas\000Drawing Interchange Format Files (*.dxf)\000*.dxf\000All Files (*.*)\000*.*\000\000",
				 szFilename[256] = "\0",
				 szFileTitle[256] = "\0",
				 szDirName[256] = "\0",
				 hpBuf[16384],
				 netaddr[128],
				 szINIFile[128],
				 szProgDir[128],
				*szTitleBase = "Virtual Etch-A-Sketch",
				 szIntroduction[] = "Virtual Etch-A-Sketch v1.02";
HWND			 hVDial, hHDial;
HBRUSH		 Gray, Black, White, BkColor;
HANDLE		 hProgInst;
HGLOBAL		 hMem,
				 hIPs = NULL;
POINT	FAR	*history = NULL;
HWND			 dlg, c1, c2, c3;
struct dest	*iplist = NULL;
char			*dxftitle[] =	{
					 "  0\nSECTION\n",
					 "  2\nENTITIES\n",
					 "  0\nLINE\n  8\n0\n",
					 " 10\n%d\n 20\n%d\n 30\n0.0\n 11\n%d\n 21\n%d\n% 31\n0.0\n",
					 "  0\nENDSEC\n  0\nEOF\n"
					};

int WINAPI WinMain (HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpszCmdLine,
							int nShowCmd)
{
 static 	char			 szAppName[] = "VEtch";
 HWND						 hwnd;
 MSG						 msg;
 WNDCLASS				 wndclass;
 WSADATA					 wsadata;
 int						 i, timer;
 struct	sockaddr_in	 saddr;
 struct	hostent		*host;
 struct	protoent		*proto;
 struct	servent		*service;
 char						*c,
							 regloc[] = "Software\\Colagioia\\Etch-A-Sketch",
							 reginfo[32];

 if (WSAStartup (MAKEWORD (2,0), &wsadata))
	flagSockFail = 1;
 randomize ();
 hProgInst = hInstance;
 White = GetStockObject (WHITE_BRUSH);

 GetCurrentDirectory (sizeof (szProgDir), szProgDir);
 wsprintf (szINIFile, "%s\\%s", szProgDir, "VETCH.INI");

 if (!GetRegValue (HKEY_LOCAL_MACHINE, regloc, "Color", (LPBYTE)reginfo, sizeof (reginfo)))
	BkColor = CreateSolidBrush (ToRGB (reginfo));
 else
	BkColor = CreateSolidBrush (RGB (180, 0, 0));

 if (!GetRegValue (HKEY_LOCAL_MACHINE, regloc, "BkColor", (LPBYTE)reginfo, sizeof (reginfo)))
	Gray = CreateSolidBrush (ToRGB (reginfo));
 else
	Gray = GetStockObject (LTGRAY_BRUSH);

 if (!GetRegValue (HKEY_LOCAL_MACHINE, regloc, "DrawColor", (LPBYTE)reginfo, sizeof (reginfo)))
	Black = CreateSolidBrush (ToRGB (reginfo));
 else
	Black = GetStockObject (BLACK_BRUSH);

 if (!GetRegValue (HKEY_LOCAL_MACHINE, regloc, "PollRate", (LPBYTE)reginfo, sizeof (reginfo)))
	iTimeout = *(int *)(reginfo);
 else
	iTimeout = 250;

 if (!GetRegValue (HKEY_LOCAL_MACHINE, regloc, "DialRate", (LPBYTE)reginfo, sizeof (reginfo)))
	timer = *(int *)(reginfo);
 else
	timer = 256;

 if (!GetRegValue (HKEY_LOCAL_MACHINE, regloc, "MRULength", (LPBYTE)reginfo, sizeof (reginfo)))
	iLenMRU = *(int *)(reginfo);
 else
	iLenMRU = 4;

 if (!hPrevInstance)
	{
	 wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	 wndclass.lpfnWndProc   = WndProc;
	 wndclass.cbClsExtra    = 0;
	 wndclass.cbWndExtra    = 0;
	 wndclass.hInstance     = hInstance;
	 wndclass.hIcon         = LoadIcon (hInstance, "I_ETCH");
	 wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
	 wndclass.hbrBackground = Black;
	 wndclass.lpszMenuName  = "EtchMenu";
	 wndclass.lpszClassName = szAppName;
	 RegisterClass (&wndclass);

	 wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	 wndclass.lpfnWndProc   = SimpleProc;
	 wndclass.cbClsExtra    = 0;
	 wndclass.cbWndExtra    = 0;
	 wndclass.hInstance     = hInstance;
	 wndclass.hIcon         = LoadIcon (hInstance, "I_ETCH");
	 wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
	 wndclass.hbrBackground = BkColor;
	 wndclass.lpszMenuName  = NULL;
	 wndclass.lpszClassName = "Empty";
	 RegisterClass (&wndclass);
	}
 RegisterDial (hInstance, hPrevInstance, BkColor, timer);

 for (c=lpszCmdLine;*c;c++)
	{
	 if (isspace (*(c-1)) && ((*c == '-') || (*c == '/')))
		{
		 ++c;
		 switch (*c)
			{
			 case 'c':		// Center sketches on new?
				++c;
				if (*c == '-' || *c == 'n')
					{
					 ++c;
					 flagCenter = 0;
					}
				else if (!isspace (*c))
					++c;
				break;
			 case 'f':		// Open a file on startup
				++c;
				while (isspace (*c)) ++c;
				i = 0;
				if (*c == '\"')
					{
					 ++c;
					 while (*c != '\"')
						szFilename[i++] = *c++;
					}
				else while (*c && !isspace (*c))
					szFilename[i++] = *c++;
				szFilename[i] = '\000';
				break;
			 case 'o':		// Change the service port
				++c;
				while (isspace (*c)) ++c;
				if (!isdigit (*c))
					{
					 service = getservbyname (c, "tcp");
					 if (service)
						port = ntohs (service->s_port);
					 else
						MessageBox (NULL, "Invalid Port--Using Default",
							"Error", MB_OK | MB_ICONWARNING);
					}
				else
					port = (short) atoi (c);
				break;
			 case 's':		// Shake the Etch-A-Sketch?
				++c;
				if (*c == '-' || *c == 'n')
					{
					 ++c;
					 flagShake = 0;
					}
				else if (!isspace (*c))
					++c;
				break;
			 case 'y':		// Always answer "Yes" to popup windows
				++c;
				flagAsk = 0;
				break;
			 default:
				++c;
				MessageBox (NULL, "Invalid Option", "Error", MB_OK | MB_ICONWARNING);
				break;
			}
		}
	 else if (isalpha(*c) || *c == '_' || *c == '\"')
		{
		 while (isspace (*c)) ++c;
		 i = 0;
		 if (*c == '\"')
			{
			 ++c;
			 while (*c != '\"')
				szFilename[i++] = *c++;
			}
		 else while (*c && !isspace (*c))
			szFilename[i++] = *c++;
		 szFilename[i] = '\000';
		}
	}
 hwnd = CreateWindow (szAppName,				// window class name
				szTitleBase,				// window caption
				WS_OVERLAPPEDWINDOW |
				WS_CLIPCHILDREN |
				WS_CLIPSIBLINGS,			// window style
				CW_USEDEFAULT,				// initial x position
				CW_USEDEFAULT,				// initial y position
				CW_USEDEFAULT,				// initial x size
				CW_USEDEFAULT,				// initial y size
				NULL,					// parent window handle
				NULL,					// window menu handle
				hInstance,				// program instance handle
				NULL);					// creation parameters

 ShowWindow (hwnd, nShowCmd);
 UpdateWindow (hwnd);
 if (flagShake) Shake (hwnd);

 if (port == 0 && !flagSockFail)
	{
	 service = getservbyname ("etch", "tcp");
	 if (service)
		port = ntohs (service->s_port);
	 else
		port = DEFAULT_PORT;
	}

 if (!flagSockFail)
	{
	 host = gethostbyname ("localhost");
	 host = gethostbyname (host->h_name);
	 _fmemcpy ((char *)&localhost, host->h_addr_list[0], host->h_length);
	 hIPs = GlobalAlloc (GPTR, nIPs * sizeof (struct dest));
	 iplist = GlobalLock (hIPs);
	 _fmemcpy ((char *)&iplist[0].ip, (char *)&localhost, sizeof (localhost));
	 iplist[0].port = port;
	 GlobalUnlock (hIPs);

	 _fmemset (&saddr, 0, sizeof (saddr));
	 saddr.sin_family = AF_INET;
	 saddr.sin_addr.s_addr = INADDR_ANY;
	 saddr.sin_port = htons (port);
	 proto = getprotobyname ("tcp");
	 i = sock = socket (PF_INET, SOCK_STREAM, proto->p_proto);
	 if (i < 0) flagSockFail = 1;
	 else
		{
		 i = bind (sock, (struct sockaddr *)&saddr, sizeof (saddr));
		 if (i < 0) flagSockFail = 1;
		 else
			{
			 i = listen (sock, 5);
			 if (i < 0) flagSockFail = 1;
			}
		}
	}
 if (flagSockFail)
	closesocket (sock);

 while (GetMessage (&msg, NULL, 0, 0))
	{
	 TranslateMessage (&msg);
	 DispatchMessage (&msg);
	}
 return (msg.wParam);
}

long FAR PASCAL _export WndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
 HDC							 hdc;
 PAINTSTRUCT				 ps;
 RECT							 rect;
 OPENFILENAME				 ofn;
 HANDLE						 hFile;
 DWORD						 i,
								 j,
								 k,
								 menustate;
 HMENU						 hSub;
 char							 temp[256],
								 tmpstr[32],
								 sockstr[256],
								*ctmp;
 short						 zDelta;
 int							 count,
								 a,
								 b;
 static	long				 currX,
								 currY,
								 cenX,
								 cenY;
 static	int				 idx,
								 len,
								 first = 1,
								 olddiff;
 static	short				 flagCtrl = 0;
 struct	sockaddr_in		 sin;
 struct	hostent	FAR	*hostlist;
 struct	dest				*iplist;
 static	HMENU				 hMenu;

 switch (message)
	{
	 case WM_CREATE:
			histmax = 128;
			GetClientRect (hwnd, &rect);
			hVDial = CreateDial (hwnd, hProgInst, rect.right - 80, rect.bottom - 80, 80, 80, VSCROLL);
			hHDial = CreateDial (hwnd, hProgInst, rect.left, rect.bottom - 80, 80, 80, HSCROLL);
			if (hMem != NULL)
				{
				 GlobalUnlock (hMem);
				 GlobalFree (hMem);
				}
			hMem = GlobalAlloc (GMEM_FIXED, sizeof (POINT) * histmax);
			if (szFilename[0])
				{
				 hFile = CreateFile (szFilename, GENERIC_READ, 0, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL |
					FILE_FLAG_SEQUENTIAL_SCAN, NULL);
				 if (hFile != INVALID_HANDLE_VALUE)
					{
					 i = ReadEtchFile (hFile, szFilename, hMem, hwnd);
					 if ((signed int)i < 0)
						break;
					}
				 else MessageBox (NULL, szFilename, "Bad File Name",
						MB_OK | MB_ICONERROR);
				}
			if (!flagSockFail)
				SetTimer (hwnd, POLLSOCK, iTimeout, NULL);

			hMenu = GetMenu (hwnd);
			menustate = flagCenter?MF_CHECKED:MF_UNCHECKED;
			CheckMenuItem (hMenu, CM_OPTCENTER, menustate);
			menustate = flagShake?MF_CHECKED:MF_UNCHECKED;
			CheckMenuItem (hMenu, CM_OPTSHAKE, menustate);
			hSub = GetSubMenu (hMenu, 0);
			MRUfromFile (hSub, iLenMRU, szINIFile);
			break;

	 case WM_SIZE:
			GetClientRect (hwnd, &rect);
			MoveWindow (hVDial, rect.right - 80, rect.bottom - 80, 80, 80, TRUE);
			SetDialRange (hVDial, SB_CTL, rect.top, rect.bottom, TRUE);
			SetDialPos (hVDial, SB_CTL, ((rect.top + rect.bottom) / 2), TRUE);
			MoveWindow (hHDial, rect.left, rect.bottom - 80, 80, 80, TRUE);
			SetDialRange (hHDial, SB_CTL, rect.left, rect.right, TRUE);
			SetDialPos (hVDial, SB_CTL, ((rect.left + rect.right) / 2), TRUE);

			if (flagCenter || first)
				{
				 currX = cenX = (rect.left + rect.right) / 2;
				 currY = cenY = (rect.top + rect.bottom - 40) / 2;
				 first = 0;
				}

			if (histlen < 2)
				{
				 history = GlobalLock (hMem);
				 history[0].x = 0;
				 history[0].y = 0;
				 GlobalUnlock (hMem);
				}

			InvalidateRect (hwnd, &rect, TRUE);
			SendMessage (hwnd, WM_COMMAND, CM_OPTREF, 0L);
			return 0 ;

	 case WM_PAINT:
			hdc = BeginPaint (hwnd, &ps);
			GetClientRect (hwnd, &rect);

			FillRect (hdc, &rect, BkColor);
			rect.top += 40;
			rect.bottom -= 80;
			rect.left += 40;
			rect.right -= 40;
			FillRect (hdc, &rect, Gray);

			history = GlobalLock (hMem);
			for (i=0;i<histlen;i++)
				{
				 currX = history[i].x + cenX;
				 currY = history[i].y + cenY;
				 rect.top = currY - 1;
				 rect.bottom = currY + 1;
				 rect.left = currX - 1;
				 rect.right = currX + 1;
				 FillRect (hdc, &rect, Black);
				}
			GlobalUnlock (hMem);

			EndPaint (hwnd, &ps);
			break;

	 case WM_COMMAND:
			switch (wParam)
				{
				 case CM_FILENEW:
						if (flagModified && flagAsk)
							{
							 i = MessageBox (hwnd,
								"The sketch has changed!\n\n\
Do you want to save the changes\nbefore clearing?",
								"Unsaved Sketch",
								MB_YESNOCANCEL | MB_TASKMODAL | MB_ICONEXCLAMATION);
							 switch (i)
								{
								 case IDYES:
									i = SendMessage (hwnd, WM_COMMAND, CM_FILESAVE, 0L);
									if (i) return (1);
									break;
								 case IDCANCEL:
									return (1);
								 case IDNO:
									break;
								}
							}
						if (flagShake)
							Shake (hwnd);
						if (!flagCenter)
							{
							 cenX = currX;
							 cenY = currY;
							}
						histlen = 1;
						if (hMem != NULL)
							{
							 GlobalUnlock (hMem);
							 hMem = GlobalFree (hMem);
							}
						histmax = 128;
						hMem = GlobalAlloc (GMEM_FIXED, sizeof (POINT) * histmax);
						if (hMem == NULL)
							{
							 MessageBox (hwnd, "Out of Memory",
								"Internal Error", MB_OK | MB_ICONERROR);
							 PostQuitMessage (0);
							}
						history = GlobalLock (hMem);
						history[0].x = history[0].y = 0;
						GlobalUnlock (hMem);
						ChangeTitle (hwnd, NULL);
						SendMessage (hwnd, WM_SIZE, 0, 0L);
						flagModified = 0;
						strcpy (szFilename, "\000");
						break;

				 case CM_FILEOPEN:
						_fmemset (&ofn, 0, sizeof (OPENFILENAME));
						_fmemset (szFilename, 0, sizeof (szFilename));
						ofn.lStructSize = sizeof (OPENFILENAME);
						ofn.hwndOwner = hwnd;
						ofn.lpstrFilter = szInFilter;
						ofn.lpstrFile = szFilename;
						ofn.nMaxFile = sizeof (szFilename);
						ofn.lpstrFileTitle = szFileTitle;
						ofn.nMaxFileTitle = sizeof (szFileTitle);
						ofn.lpstrInitialDir = szProgDir;
						ofn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT;
						do	{
							 i = GetOpenFileName (&ofn);
							 hFile = CreateFile (szFilename, GENERIC_READ,
								0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL |
								FILE_FLAG_SEQUENTIAL_SCAN, NULL);
							 if (hFile == INVALID_HANDLE_VALUE && i)
								MessageBox (hwnd, "File Not Found!  Try Again.",
									szFilename, MB_OK | MB_ICONEXCLAMATION);
							}
						while (hFile == INVALID_HANDLE_VALUE && i);
						if (flagModified && flagAsk &&
							(hFile != INVALID_HANDLE_VALUE))
							{
							 i = MessageBox (hwnd,
								"The sketch has changed!\n\n\
Do you want to save the changes\nbefore opening another?",
								"Unsaved Sketch",
								MB_YESNOCANCEL | MB_TASKMODAL | MB_ICONEXCLAMATION);
							 switch (i)
								{
								 case IDYES:
									i = SendMessage (hwnd, WM_COMMAND, CM_FILESAVE, 0L);
									if (i) return (1);
									break;
								 case IDCANCEL:
									return (1);
								 case IDNO:
									break;
								}
							}
						if (!flagCenter)
							{
							 cenX = currX;
							 cenY = currY;
							}
						if (hFile != INVALID_HANDLE_VALUE)
							{
							 SHAddToRecentDocs (SHARD_PATH, szFilename);
							 hSub = GetSubMenu (hMenu, 0);
							 AddMRU (hSub, iLenMRU, szFilename);
							 MRUtoFile (hSub, iLenMRU, szINIFile);
							 i = ReadEtchFile (hFile, szFilename, hMem, hwnd);
							 if ((signed int)i < 0)
								break;
							}
						else	if (i)
							MessageBox (NULL, "An error has been encountered while opening the file.",
								"File Error", MB_OK | MB_ICONERROR);
						SendMessage (hwnd, WM_COMMAND, CM_OPTREF, 0L);
						flagModified = 0;
						break;

				 case CM_FILESAVE:
						if (lstrcmp (szFilename, "\000"))
							{
							 hFile = CreateFile (szFilename, GENERIC_WRITE,
								0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
								NULL);
							 if (hFile == INVALID_HANDLE_VALUE)
								return (1);
							 history = GlobalLock (hMem);
							 ctmp = strchr (szFilename, '\000');
							 do	--ctmp;
								while (*ctmp != '.');
							 if (!stricmp (ctmp, ".dxf"))
								WriteDXF (hFile);
							 else
								{
								 history[0].y = histlen;
								 WriteFile (hFile, "Etch-A-Sketch Save File\x1A",
									24, &i, NULL);
								 WriteFile (hFile, (char *)history, histmax *
									sizeof (POINT), &i, NULL);
								}
							 GlobalUnlock (hMem);
							 CloseHandle (hFile);
							 flagModified = 0;
							}
						else
							{
							 i = SendMessage (hwnd, WM_COMMAND, CM_FILESAVEAS, 0L);
							 flagModified = (short) i;
							}
						if (!flagModified)
							MessageBox (hwnd, "File Saved", "Status",
								MB_TASKMODAL | MB_ICONINFORMATION);
						return (flagModified);

				 case CM_FILESAVEAS:
						_fmemset (&ofn, 0, sizeof (OPENFILENAME));
						szFilename[0] = '\0';
						ofn.lStructSize = sizeof (OPENFILENAME);
						ofn.hwndOwner = hwnd;
						ofn.lpstrFilter = szOutFilter;
						ofn.lpstrFile = szFilename;
						ofn.nMaxFile = sizeof (szFilename);
						ofn.lpstrFileTitle = szFileTitle;
						ofn.nMaxFileTitle = sizeof (szFileTitle);
						ofn.lpstrInitialDir = szProgDir;
						strcpy (tmpstr, "eas");
						ofn.lpstrDefExt = tmpstr;
						ofn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT
									| OFN_HIDEREADONLY;
						i = GetSaveFileName (&ofn);
						if (!i)
							return (1);
						hFile = CreateFile (ofn.lpstrFile, GENERIC_WRITE,
							0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
							NULL);
						if (hFile == INVALID_HANDLE_VALUE)
							return (1);
						history = GlobalLock (hMem);
						if (!stricmp (&ofn.lpstrFile[ofn.nFileExtension], "dxf"))
							WriteDXF (hFile);
						else	{
							 history[0].y = histlen;
							 WriteFile (hFile, "Etch-A-Sketch Save File\x1A",
								24, &i, NULL);
							 WriteFile (hFile, (char *)history, histmax *
								sizeof (POINT), &i, NULL);
							}
						GlobalUnlock (hMem);
						CloseHandle (hFile);
						ChangeTitle (hwnd, szFilename);
						SHAddToRecentDocs (SHARD_PATH, ofn.lpstrFile);
						hSub = GetSubMenu (hMenu, 0);
						AddMRU (hSub, iLenMRU, szFilename);
						MRUtoFile (hSub, iLenMRU, szINIFile);
						flagModified = 0;
						return (0);

				 case CM_FILEEXIT:
						if (flagModified && flagAsk)
							{
							 i = MessageBox (hwnd,
								"The sketch has changed!\n\n\
Do you want to save the changes\nbefore exiting?",
								"Unsaved Sketch",
								MB_YESNOCANCEL | MB_TASKMODAL | MB_ICONEXCLAMATION);
							 switch (i)
								{
								 case IDYES:
									i = SendMessage (hwnd, WM_COMMAND, CM_FILESAVE, 0L);
									if (i) return (1);
									break;
								 case IDCANCEL:
									return (1);
								 case IDNO:
									break;
								}
							}
						GlobalUnlock (hMem);
						GlobalFree (hMem);
						PostMessage (hwnd, WM_DESTROY, 0, 0L);
						break;

				 case CM_OPTCENTER:
						hMenu = GetMenu (hwnd);
						menustate = flagCenter?MF_UNCHECKED:MF_CHECKED;
						CheckMenuItem (hMenu, CM_OPTCENTER, menustate);
						flagCenter ^= 1;
						break;

				 case CM_OPTSHAKE:
						hMenu = GetMenu (hwnd);
						menustate = flagShake?MF_UNCHECKED:MF_CHECKED;
						CheckMenuItem (hMenu, CM_OPTSHAKE, menustate);
						flagShake ^= 1;
						break;

				 case CM_OPTFLASH:
					history = GlobalLock (hMem);
					for (i=0;i<2500;i++)
						{
						 hdc = BeginPaint (hwnd, &ps);
						 GetClientRect (hwnd, &rect);
						 rect.top = cenY + history[histlen-1].y - 1;
						 rect.bottom = rect.top + 2;
						 rect.left += cenX + history[histlen-1].x - 1;
						 rect.right = rect.left + 2;
						 FillRect (hdc, &rect, i%2?White:Black);
						 InvalidateRect (hwnd, &rect, TRUE);
						 EndPaint (hwnd, &ps);
						}
					GlobalUnlock (hMem);
					break;

				 case CM_OPTREF:
						GetClientRect (hwnd, &rect);
						InvalidateRect (hwnd, &rect, TRUE);
						ShowWindow (hwnd, SW_SHOW);
						UpdateWindow (hwnd);
						break;

				 case CM_OPTUNDO:
						if (histlen > 1) --histlen;
						SendMessage (hwnd, WM_COMMAND, CM_OPTREF, 0L);
						flagModified = 1;
						break;

				 case CM_NETSHOW:
						if (flagSockFail)
							{
							 wsprintf (temp, "No networking available.");
							 MessageBox (hwnd, temp, "Host IP Addresses",
								MB_OK | MB_ICONINFORMATION);
							 break;
							}
						hostlist = gethostbyname ("localhost");
						wsprintf (temp, "* %s\n* %s", "localhost", hostlist->h_name);
						for (i=0;hostlist->h_aliases[i];i++)
							{
							 lstrcat (temp, "\n* ");
							 lstrcat (temp, hostlist->h_aliases[i]);
							}
						for (i=0;hostlist->h_addr_list[i];i++)
							{
							 ctmp = hostlist->h_addr_list[i];
							 wsprintf (tmpstr, "\n* %d.%d.%d.%d",
								(unsigned char) ctmp[0],
								(unsigned char) ctmp[1],
								(unsigned char) ctmp[2],
								(unsigned char) ctmp[3]);
							 lstrcat (temp, tmpstr);
							}
						hostlist = gethostbyname (hostlist->h_name);
						for (i=0;hostlist->h_aliases[i];i++)
							{
							 lstrcat (temp, "\n* ");
							 lstrcat (temp, hostlist->h_aliases[i]);
							}
						for (i=0;hostlist->h_addr_list[i];i++)
							{
							 ctmp = hostlist->h_addr_list[i];
							 wsprintf (tmpstr, "\n* %d.%d.%d.%d",
								(unsigned char) ctmp[0],
								(unsigned char) ctmp[1],
								(unsigned char) ctmp[2],
								(unsigned char) ctmp[3]);
							 lstrcat (temp, tmpstr);
							}
						wsprintf (tmpstr, "\n\n* (port = %u)", port);
						lstrcat (temp, tmpstr);
						MessageBox (hwnd, temp, "Host IP Addresses",
							MB_OK | MB_ICONINFORMATION);
						break;

				 case CM_NETADD:
						if (flagSockFail)
							{
							 MessageBox (hwnd, "Networking is unavailable in this session.",
								"Status", MB_OK | MB_ICONINFORMATION);
							 break;
							}
						dlg = CreateWindow ("Empty", "Enter Address to Add",
							WS_POPUP | WS_VISIBLE | WS_BORDER |
							WS_CAPTION, CW_USEDEFAULT, CW_USEDEFAULT,
							185, 125, hwnd, NULL, hProgInst, NULL);
						c1 = CreateWindowEx (WS_EX_CONTROLPARENT |
							WS_EX_TOPMOST | WS_EX_STATICEDGE, "Edit",
							NULL, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
							10, 10, 160, 20, dlg, NULL, hProgInst, NULL);
						c2 = CreateWindowEx (WS_EX_CONTROLPARENT |
							WS_EX_TOPMOST, "button", "Add", WS_CHILD |
							WS_VISIBLE, 40, 40, 100, 25, dlg, NULL,
							hProgInst, NULL);
						c3 = CreateWindowEx (WS_EX_CONTROLPARENT |
							WS_EX_TOPMOST, "button", "Cancel", WS_CHILD
							| WS_VISIBLE, 40, 70, 100, 25, dlg, NULL,
							hProgInst, NULL);
						ShowWindow (dlg, SW_SHOWNORMAL);
						UpdateWindow (dlg);
						SetFocus (c1);
						break;

				 case CM_NETDEL:
						if (flagSockFail)
							{
							 MessageBox (hwnd, "Networking is unavailable in this session.",
								"Status", MB_OK | MB_ICONINFORMATION);
							 break;
							}
						dlg = CreateWindow ("Empty", "Select Address to Delete",
							WS_POPUP | WS_VISIBLE | WS_BORDER |
							WS_CAPTION, CW_USEDEFAULT, CW_USEDEFAULT,
							185, 125, hwnd, NULL, hProgInst, NULL);
						c1 = CreateWindowEx (WS_EX_CONTROLPARENT |
							WS_EX_TOPMOST | WS_EX_STATICEDGE |
							WS_VSCROLL, "ComboBox",
							NULL, WS_CHILD | WS_VISIBLE | CBS_SORT | CBS_AUTOHSCROLL |
							CBS_DROPDOWN | CBS_HASSTRINGS | WS_VSCROLL |
							CBS_LOWERCASE | CBS_DISABLENOSCROLL,
							10, 10, 160, 80, dlg, NULL, hProgInst, NULL);
						c2 = CreateWindowEx (WS_EX_CONTROLPARENT,
							"button", "Delete", WS_CHILD |
							WS_VISIBLE, 40, 40, 100, 25, dlg, NULL,
							hProgInst, NULL);
						c3 = CreateWindowEx (WS_EX_CONTROLPARENT,
							"button", "Cancel", WS_CHILD
							| WS_VISIBLE, 40, 70, 100, 25, dlg, NULL,
							hProgInst, NULL);
						iplist = GlobalLock (hIPs);
						for (count=0;iplist[count].ip;++count)
							{
							 hostlist = gethostbyaddr ((char FAR *)&iplist[count].ip,
								sizeof (iplist[count].ip), PF_INET);
							 wsprintf (temp, "%s:%d", hostlist->h_name,
								iplist[count].port);
							 SendMessage (c1, CB_INSERTSTRING, -1, (LPARAM)temp);
							}
						GlobalUnlock (hIPs);
						ShowWindow (dlg, SW_SHOWNORMAL);
						UpdateWindow (dlg);
						SetFocus (c1);
						break;

				 case CM_HELPCONTENTS:
						i = WinHelp (hwnd, "ETCH.HLP", HELP_CONTENTS, 0L);
						if ((int) i < 0)
							MessageBox (hwnd, "It's an Etch-A-Sketch.  Do you REALLY need help?",
										"Etch-A-Sketch Help", MB_OK | MB_ICONQUESTION);
						break;

				 case CM_HELPABOUT:
						i = DialogBox (hProgInst, "ETCH_DLGABOUT", hwnd, AboutProc);
						if ((int) i < 0)
							MessageBox (hwnd, "\
The Virtual Etch-A-Sketch for Windows\'95, v1.02, (c)1997 by John Colagioia, \
Any Available Rights Reserved (if any).  Any resemblance to any element of \
this program (including the Etch-A-Sketch) to any children\'s toy (including \
the Ohio Arts Etch-A-Sketch), living or dead, is purely coincidental--so don\'t \
anybody sue me, OK?", "About The Virtual Etch-A-Sketch", MB_SYSTEMMODAL);
						break;
				 default:
						hSub = GetSubMenu (hMenu, 0);
						GetStringFromMenuID (hSub, wParam, temp, sizeof(temp));
						hFile = CreateFile (&temp[3], GENERIC_READ, 0, NULL,
							OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL |
							FILE_FLAG_SEQUENTIAL_SCAN, NULL);
						if (hFile == INVALID_HANDLE_VALUE)
							MessageBox (hwnd, &temp[3], "Cannot Find File", MB_TASKMODAL);
						else
							{
							 ReadEtchFile (hFile, &temp[3], hMem, hwnd);
							 SendMessage (hwnd, WM_COMMAND, CM_OPTREF, 0L);
							 flagModified = 0;
							}
						break;
				}

	 case WM_KEYDOWN:
			switch (wParam)
				{
				 case VK_CONTROL:
						flagCtrl = 1;
						break;
				 case VK_LEFT:
						PostMessage (hHDial, WM_LBUTTONDOWN, 0, 0L);
						PostMessage (hHDial, WM_LBUTTONUP, 0, 0L);
						break;
				 case VK_RIGHT:
						PostMessage (hHDial, WM_RBUTTONDOWN, 0, 0L);
						PostMessage (hHDial, WM_RBUTTONUP, 0, 0L);
						break;
				 case VK_UP:
						PostMessage (hVDial, WM_LBUTTONDOWN, 0, 0L);
						PostMessage (hVDial, WM_LBUTTONUP, 0, 0L);
						break;
				 case VK_DOWN:
						PostMessage (hVDial, WM_RBUTTONDOWN, 0, 0L);
						PostMessage (hVDial, WM_RBUTTONUP, 0, 0L);
						break;
				 case VK_INSERT:
						PostMessage (hwnd, WM_COMMAND, CM_NETADD, 0L);
						break;
				 case VK_DELETE:
						PostMessage (hwnd, WM_COMMAND, CM_NETDEL, 0L);
						break;
				 case VK_F1:
						PostMessage (hwnd, WM_COMMAND, CM_HELPCONTENTS, 0L);
						break;
				 case VK_F2:
						PostMessage (hwnd, WM_COMMAND, CM_FILENEW, 0L);
						break;
				 case VK_F4:
						if (lParam & 2000L)
							PostMessage (hwnd, WM_COMMAND, CM_FILEEXIT, 0L);
						else
							PostMessage (hwnd, WM_COMMAND, CM_OPTREF, 0L);
						break;
				 case VK_F5:
						PostMessage (hwnd, WM_COMMAND, CM_OPTFLASH, 0L);
						break;
				 case 'A':
						if (flagCtrl)
							PostMessage (hwnd, WM_COMMAND, CM_FILESAVEAS, 0L);
						break;
				 case 'O':
						if (flagCtrl)
							PostMessage (hwnd, WM_COMMAND, CM_FILEOPEN, 0L);
						break;
				 case 'S':
						if (flagCtrl)
							PostMessage (hwnd, WM_COMMAND, CM_FILESAVE, 0L);
						break;
				 case 'Z':
						if (flagCtrl)
							PostMessage (hwnd, WM_COMMAND, CM_OPTUNDO, 0L);
						break;
				}
			break;

	 case WM_KEYUP:
			if (wParam == VK_CONTROL)
				flagCtrl = 0;
			break;

	 case WM_VSCROLL:
			history = GlobalLock (hMem);
			i = 0;
			if (histlen + 3 >= histmax)
				{
				 histmax *= 2;
				 GlobalUnlock (hMem);
MessageBox (hwnd, "About to ReAlloc(1)", "status", MB_SYSTEMMODAL);
				 hMem = GlobalReAlloc (hMem, sizeof (POINT) * histmax, 0);
MessageBox (hwnd, "Finished ReAlloc(1)", "status", MB_SYSTEMMODAL);
				 if (hMem == NULL)
					MessageBox (hwnd, "Out of Memory", "Error", MB_OK | MB_ICONERROR);
				 history = GlobalLock (hMem);
				}
			hdc = BeginPaint (hwnd, &ps);
			GetClientRect (hwnd, &rect);
			switch (wParam)
				{
				 case SB_LINEDOWN:
						if (currY < rect.bottom - 81)
							{
							 ++currY;
							 i = 1;
							}
						break;
				 case SB_LINEUP:
						if (currY > rect.top + 41)
							{
							 --currY;
							 i = 1;
							}
						break;
				}
			if (i)
				{
				 rect.left = currX - 1;
				 rect.right = currX + 1;
				 rect.top = currY - 1;
				 rect.bottom = currY + 1;
				 FillRect (hdc, &rect, Black);
				 InvalidateRect (hwnd, &rect, TRUE);
				 EndPaint (hwnd, &ps);
				 history[histlen].x = currX - cenX;
				 history[histlen].y = currY - cenY;
				 ++histlen;
				}
			GlobalUnlock (hMem);
			flagModified = 1;
			if (lParam != SOCK_ORIG)
				{
				 memset (&sin, 0, sizeof (sin));
				 sin.sin_family = AF_INET;
				 iplist = GlobalLock (hIPs);
				 wsprintf (temp, "%cQ", wParam==SB_LINEUP?'U':'D');
				 a = 1;
				 for (i=1;iplist[i].port && a;i++)
					{
					 a = socket (PF_INET, SOCK_STREAM, 6);
					 sin.sin_port = ntohs (iplist[i].port);
					 sin.sin_addr.s_addr = iplist[i].ip;
					 b = connect (a, (struct sockaddr *)&sin, sizeof (sin));
					 if (b >= 0)
						send (a, temp, 2, 0);
					 else
						{
						 GetFormattedError (sockstr, sizeof (sockstr), TRUE);
						 wsprintf (temp, "Connection Error: \"%s\"", sockstr);
						 MessageBox (hwnd, temp, "Error", MB_SYSTEMMODAL);
						}
					 closesocket (a);
					 Yield ();
					}
				 GlobalUnlock (hIPs);
				}
			break;

	 case WM_HSCROLL:
	 case 0x625A:
			history = GlobalLock (hMem);
			i = 0;
			if (histlen + 3 == histmax)
				{
				 histmax *= 2;
				 GlobalUnlock (hMem);
MessageBox (hwnd, "About to ReAlloc(2)", "status", MB_SYSTEMMODAL);
				 history = GlobalReAlloc (hMem, sizeof (POINT) * histmax, 0);
MessageBox (hwnd, "Finished ReAlloc(2)", "status", MB_SYSTEMMODAL);
				 if (history == NULL)
					{
MessageBox (hwnd, "Failed ReAlloc(2)", "status", MB_SYSTEMMODAL);
//					 i = GetLastError ();
					 GetFormattedError (sockstr, sizeof (sockstr), FALSE);
					 wsprintf (temp, "Internal Error: \"%s\"", sockstr);
					 MessageBox (hwnd, temp, "Memory Error", MB_OK | MB_ICONERROR);
					 history = hMem;
					}
				 history = GlobalLock (hMem);
				}
			hdc = BeginPaint (hwnd, &ps);
			GetClientRect (hwnd, &rect);
			switch (wParam)
				{
				 case SB_LINEDOWN:
						if (currX < rect.right - 41)
							{
							 ++currX;
							 i = 1;
							}
						break;
				 case SB_LINEUP:
						if (currX > rect.left + 41)
							{
							 --currX;
							 i = 1;
							}
						break;
				}
			if (i)
				{
				 rect.left = currX - 1;
				 rect.right = currX + 1;
				 rect.top = currY - 1;
				 rect.bottom = currY + 1;
				 FillRect (hdc, &rect, Black);
				 InvalidateRect (hwnd, &rect, TRUE);
				 EndPaint (hwnd, &ps);
				 history[histlen].x = currX - cenX;
				 history[histlen].y = currY - cenY;
				 ++histlen;
				}
			GlobalUnlock (hMem);
			flagModified = 1;
			if (lParam != SOCK_ORIG)
				{
				 memset (&sin, 0, sizeof (sin));
				 sin.sin_family = AF_INET;
				 iplist = GlobalLock (hIPs);
				 wsprintf (temp, "%cQ", wParam==SB_LINEUP?'L':'R');
				 a = 1;
				 for (i=1;iplist[i].port && a;i++)
					{
					 a = socket (PF_INET, SOCK_STREAM, 6);
					 sin.sin_port = ntohs (iplist[i].port);
					 sin.sin_addr.s_addr = iplist[i].ip;
					 b = connect (a, (struct sockaddr *)&sin, sizeof (sin));
					 if (b >= 0)
						send (a, temp, 2, 0);
					 else
						{
						 GetFormattedError (sockstr, sizeof (sockstr), TRUE);
						 wsprintf (temp, "Connection Error: \"%s\"", sockstr);
						 MessageBox (hwnd, temp, "Error", MB_SYSTEMMODAL);
						}
					 closesocket (a);
					 Yield ();
					}
				 GlobalUnlock (hIPs);
				}
			break;

	 case WM_MOUSEWHEEL:
			olddiff = SendMessage (hHDial, WM_DIAL_SETTIMERDELTA, 0, 100L);
			if (LOWORD (wParam) & MK_MBUTTON)
				j = 5;
			else
				j = 1;
			zDelta = HIWORD (wParam);
			if (zDelta > 0)
				k = SB_LINEUP;
			else
				k = SB_LINEDOWN;
			for (i=0;i<abs(zDelta)*j/120;i++)
				PostMessage (hwnd, WM_VSCROLL, k, 0L);
			SetTimer (hwnd, DIALTIMER, 200, NULL);
			break;

	 case WM_TIMER:
			switch (wParam)
				{
				 case POLLSOCK:
						KillTimer (hwnd, POLLSOCK);
						if (flagSockFail)
							return (0);
						_fmemset (sockstr, 0, sizeof (sockstr));
						len = GetFromSocket (sock, sockstr, sizeof (sockstr));
						if (len > 0)
							{
							 for (idx=0;idx<len;idx++)
								{
								 switch (sockstr[idx])
									{
									 case 'u': case 'U':
										i = WM_VSCROLL;
										j = SB_LINEUP;
										break;
									 case 'd': case 'D':
										i = WM_VSCROLL;
										j = SB_LINEDOWN;
										break;
									 case 'l': case 'L':
										i = WM_HSCROLL;
										j = SB_LINEUP;
										break;
									 case 'r': case 'R':
										i = WM_HSCROLL;
										j = SB_LINEDOWN;
										break;
									 case 'z': case 'Z':
										i = WM_COMMAND;
										j = CM_OPTUNDO;
										break;
									 default:
										i = 0;
										break;
									}
								 if (i) SendMessage (hwnd, i, j, SOCK_ORIG);
								}
							}
						SetTimer (hwnd, POLLSOCK, iTimeout, NULL);
						break;

				 case DIALTIMER:
						SendMessage (hHDial, WM_DIAL_SETTIMERDELTA, olddiff, 1L);
						break;

				 default:
						break;
				}
			break;

	 case WM_CLOSE:
			if (flagModified && flagAsk)
				{
				 i = MessageBox (hwnd,
					"The sketch has changed!\n\n\
Do you want to save the changes\nbefore closing?",
					"Unsaved Sketch",
					MB_YESNOCANCEL | MB_TASKMODAL | MB_ICONEXCLAMATION);
				 switch (i)
					{
					 case IDYES:
						i = SendMessage (hwnd, WM_COMMAND, CM_FILESAVE, 0L);
						if (i) return (1);
						break;
					 case IDCANCEL:
						return (1);
					 case IDNO:
						break;
					}
				}
			if (hIPs) GlobalFree (hIPs);
			if (flagSockFail)
				GetFromSocket (sock, NULL, -1);
			WSACleanup ();
			DeleteObject ((HANDLE) GetClassWord (hwnd, -10 /*GCW_HBRBACKGROUND*/ ));
			break;

	 case WM_DESTROY:
			PostQuitMessage (0);
			return 0 ;
	}

 return (DefWindowProc (hwnd, message, wParam, lParam));
}

long FAR PASCAL _export SimpleProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
 long					 addr;
 short				 i = 0,
						 port = 2342;
 int					 status,
						 idx,
						 count;
 char					*c;
 struct	hostent	*host;
 struct	servent	*serv;
 struct	dest		*iplist;

 if ((message == WM_COMMAND) && !wParam)
	{
	 GetWindowText (c2, netaddr, sizeof (netaddr));
	 if (!strcmp (netaddr, "Delete"))
		i = 1;
	 else if (!strcmp (netaddr, "Add"))
		i = 2;
	 _fmemset (netaddr, 0, sizeof (netaddr));
	 if ((DWORD) lParam == (DWORD) c2)
		{
		 if (i == 2)
			{
			 GetWindowText (c1, netaddr, sizeof (netaddr));
			 DestroyWindow (c1);
			 DestroyWindow (c2);
			 DestroyWindow (c3);
			 status = strlen (netaddr);
			 if (status)
				{
				 c = strchr (netaddr, ':');
				 if (!c)
					c = strchr (netaddr, ' ');
				 if (c)
					{
					 *c = '\000';
					 ++c;
					 if (isdigit (*c))
						port = (short) atoi (c);
					 else if (isalpha (*c))
						{
						 serv = getservbyname (c, "tcp");
						 if (serv)
							port = ntohs (serv->s_port);
						 else
							{
							 MessageBox (hwnd, "Invalid Port", "Error", MB_OK | MB_ICONWARNING);
							 return (1);
							}
						}
					 else
						port = DEFAULT_PORT;
					}
				 c = netaddr;
				 while (isspace (*c)) ++c;
				 if (isalpha (*c))
					host = gethostbyname (c);
				 else
					{
					 addr = inet_addr (c);
					 host = gethostbyaddr ((char *)&addr, sizeof (addr), PF_INET);
					}
				 if (host == NULL)
					{
					 MessageBox (hwnd, "Invalid Address", "Error", MB_OK | MB_ICONWARNING);
					 return (1);
					}
				}
			}
		else if (i == 1)
			{
			 idx = SendMessage (c1, CB_GETCURSEL, 0, 0L);
			 if (idx == 0)
				MessageBox (hwnd, "Cannot remove localhost from workgroup!",
					"Error", MB_TASKMODAL | MB_ICONWARNING);
			 if (idx == CB_ERR || idx == 0)
				{
				 DestroyWindow (c1);
				 DestroyWindow (c2);
				 DestroyWindow (c3);
				 DestroyWindow (hwnd);
				 return (0);
				}
			}
		}
	 else if ((DWORD) lParam == (DWORD) c3)
		{
		 DestroyWindow (c1);
		 DestroyWindow (c2);
		 DestroyWindow (c3);
		 DestroyWindow (hwnd);
		 return (0);
		}

	 if (!host || !status)
		{
		 DestroyWindow (hwnd);
		 return (0);
		}

	 if (i == 1)
		{
		 iplist = GlobalLock (hIPs);
		 --nIPs;
		 for (count=idx;count<nIPs;count++)
			{
			 iplist[count].port = iplist[count+1].port;
			 iplist[count].ip = iplist[count+1].ip;
			}
		 GlobalUnlock (hIPs);
MessageBox (hwnd, "About to ReAlloc(3)", "status", MB_SYSTEMMODAL);
		 hIPs = GlobalReAlloc (hIPs, nIPs * sizeof (struct dest), 0);
MessageBox (hwnd, "Finished ReAlloc(3)", "status", MB_SYSTEMMODAL);
		}
	 else if (i == 2)
		{
		 ++nIPs;
MessageBox (hwnd, "About to ReAlloc(4)", "status", MB_SYSTEMMODAL);
		 hIPs = GlobalReAlloc (hIPs, nIPs * sizeof (struct dest), 0);
MessageBox (hwnd, "Finished ReAlloc(4)", "status", MB_SYSTEMMODAL);
		 iplist = GlobalLock (hIPs);
		 _fmemcpy ((char *)&iplist[nIPs - 2].ip, host->h_addr_list[0], host->h_length);
		 iplist[nIPs - 2].port = port;
		 iplist[nIPs - 1].ip = 0;
		 iplist[nIPs - 1].port = 0;
		 GlobalUnlock (hIPs);
		}
	 DestroyWindow (hwnd);
	 return (0);
	}

 return (DefWindowProc (hwnd, message, wParam, lParam));
}

void Shake (HWND hwnd)
{
 RECT	rect;
 long	x, y, wd, ln, i, times;

 GetWindowRect (hwnd, &rect);
 x = rect.left;
 y = rect.top;
 wd = rect.right - x;
 ln = rect.bottom - y + 1;
 times = random (6) + 5;
 for (i = 0; i < times; i++)
	{
	 MoveWindow (hwnd, x - random (40) + 20, y - random (40) + 20, wd, ln, TRUE);
	 MoveWindow (hwnd, x - random (40) + 20, y - random (40) + 20, wd, ln, TRUE);
	}
 MoveWindow (hwnd, x, y, wd, ln - 1, TRUE);
 return;
}

BOOL CALLBACK AboutProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
 HDC			hdc;
 PAINTSTRUCT	ps;
 RECT		rect;
 HWND		hParent;

 switch (msg)
	{
	 case WM_CREATE:
	 case WM_INITDIALOG:
			SendMessage (hDlg, WM_PAINT, 0, 0L);
			return (FALSE);
	 case WM_COMMAND:
			EndDialog (hDlg, 0);
			hParent = GetParent (hDlg);
			SetFocus (hParent);
			return (TRUE);
	 case WM_PAINT:
			hdc = BeginPaint (hDlg, &ps);
			FillRect (hdc, &rect, Black);
			GetClientRect (hDlg, &rect);
			InvalidateRect (hDlg, &rect, TRUE);
			EndPaint (hDlg, &ps);
			return (TRUE);
	}
 return (DefWindowProc (hDlg, msg, wParam, lParam));
}

int GetFromSocket (int sock, char * retstring, int retlen)
{
 static	int		 lastsock = 0,
						 leftover = 0;
 static	fd_set	 stock_fds;
 static	HGLOBAL	 hStuff = NULL;
			int		 readsock,
						 count,
						 len,
						 i;
			char		*ctmp,
						 lang[16],
						 outmsg[128];
			fd_set	 fds;
			time_t	 now;
 struct	timeval	 notime = {0,0};
 struct	hostent	*host;
 HGLOBAL				 hOut;

 if (leftover)
	{
MessageBox (NULL, "Got leftovers", "status", MB_SYSTEMMODAL);
	 ctmp = GlobalLock (hStuff);
	 if (strlen (ctmp) < retlen)
		{
		 leftover = 0;
		 lstrcpy (retstring, ctmp);
		 GlobalUnlock (hStuff);
		 GlobalFree (hStuff);
		}
	 else
		{
		 _fmemset (retstring, '\000', retlen);
		 lstrcpyn (retstring, ctmp, retlen - 2);
		 lstrcpy (ctmp, &ctmp[retlen - 1]);
		 GlobalUnlock (hStuff);
		}
	 return (lstrlen (retstring));
	}
 if (retlen == -1)
	{
	 for (i=sock;i<FD_SETSIZE;i++)
		if (FD_ISSET (i, &stock_fds))
			closesocket (i);
	 return (0);
	}
 FD_ZERO (&fds);
 for (i=sock;i<FD_SETSIZE;i++)			// Copy the FD set
	if (FD_ISSET (i, &stock_fds))
		FD_SET (i, &fds);
 FD_SET (sock, &fds);						// Add the passed socket
 i = select (FD_SETSIZE, &fds, NULL, NULL, &notime);
 if (i == SOCKET_ERROR)
	{
	 GetFormattedError (outmsg, sizeof (outmsg), TRUE);
	 wsprintf (retstring, "Multithreading Problems [%s]", outmsg);
	 MessageBox (NULL, retstring, "Error", MB_OK | MB_ICONERROR);
	}
 if (i == 0)
	return (-1);
 if (FD_ISSET (sock, &fds))				// The "main" socket only requests
	{												// communication.  No data.
	 readsock = accept (sock, NULL, NULL);
	 if (readsock < 0)
		{
		 GetFormattedError (outmsg, sizeof (outmsg), TRUE);
		 wsprintf (retstring, "Failure on accept():  \"%s\"", outmsg);
		 MessageBox (NULL, retstring, "Error", MB_OK | MB_ICONERROR);
		}
	 else
		{
		 FD_ZERO (&fds);
		 FD_SET (readsock, &fds);
		 i = select (FD_SETSIZE, NULL, &fds, NULL, &notime);
		 if (i == SOCKET_ERROR)
			MessageBox (NULL, "Multithreading Problems", "Error", MB_OK | MB_ICONWARNING);
		 else if (FD_ISSET (readsock, &fds))	// If the socket is free...
			{
			 time (&now);
			 host = gethostbyname ("localhost");
			 host = gethostbyname (host->h_name);
			 wsprintf (outmsg, "220 %s on %s at %s\r", szIntroduction,
				host->h_name, ctime (&now));	// ...say "Hello"
			 send (readsock, outmsg, strlen (outmsg) + 1, 0);
			}
		 FD_SET (readsock, &stock_fds);	// Add to the list
		}
	 return (0);
	}
 _fmemset (retstring, 0, retlen);
 for (count=0;count<FD_SETSIZE;count++)		// Start from 1 to avoid repeating
	{														// a socket.
	 readsock = (count + lastsock) % FD_SETSIZE;	// Skip past the last socket
	 if (FD_ISSET (readsock, &fds))					// just to be fair
		{
		 lastsock = readsock + 1;
		 len = recv (readsock, retstring, retlen - 1, 0);
		 if (len > 0)
			send (readsock, retstring, len, 0);
		 else						// No data--kill the connection
			strcpy (retstring, "Q");
		 ctmp = strchr (retstring, 'Q');
		 if (ctmp)
			{
			 i = closesocket (readsock);
			 if (i == SOCKET_ERROR)
				MessageBox (NULL, "Cannot close socket", "Error", MB_OK | MB_ICONERROR);
			 *ctmp = '\000';
			 len = ctmp - retstring;
			 FD_CLR (readsock, &stock_fds);
			}
		 if (retstring[0] == '*')
			{
			 i = 0;
			 for (ctmp = &retstring[1]; *ctmp != '*'; ctmp++)
				lang[i++] = *ctmp;
			 lang[i] = '\000';
			 ++ctmp;
			 hOut = LibInterpret (lang, ctmp);
			 ctmp = GlobalLock (hOut);
			 len = lstrlen (ctmp) + 2;
			 if (len < retlen)
				{
				 lstrcpy (retstring, ctmp);
				 GlobalUnlock (hOut);
				 GlobalFree (hOut);
				}
			 else
				{
wsprintf (retstring, "length = %d, size = %d", len - 2, GlobalSize (hOut));
MessageBox (NULL, retstring, "status", MB_SYSTEMMODAL);
				 _fmemset (retstring, '\000', retlen);
				 lstrcpyn (retstring, ctmp, retlen - 2);
				 lstrcpy (ctmp, &ctmp[retlen - 1]);
				 GlobalUnlock (hOut);
				 hStuff = hOut;
				 leftover = 1;
				}
			 len = strlen (retstring);
			}
		 return (len);
		}
	}
 return (-1);
}

void ChangeTitle (HWND hwnd, char * NewTitle)
{
 char	TitleBar[256];

 if (NewTitle)
	wsprintf (TitleBar, "%s - [%s]", szTitleBase, NewTitle);
 else
	wsprintf (TitleBar, "%s", szTitleBase);
 SetWindowText (hwnd, TitleBar);
 flagModified = 0;
 return;
}

int ReadEtchFile (HANDLE hFile, LPSTR szFilename, HGLOBAL hMem, HWND hwnd)
{
 DWORD	i;
 char		temp[256],
			ebuf[256];

 history = GlobalLock (hMem);
 ReadFile (hFile, (void *)history, 24, &i, NULL);
 if (strncmp ((char *)history, "Etch-A-Sketch Save File\x1A", 24))
	{
	 MessageBox (NULL, "Invalid file format for Etch-A-Sketch file.",
		"File Open Error", MB_OK | MB_ICONERROR);
	 GlobalUnlock (hMem);
	 return (-1);
	}
 if (hMem != NULL)
	{
	 GlobalUnlock (hMem);
	 GlobalFree (hMem);
	}
 histmax = 128;
 hMem = GlobalAlloc (GMEM_FIXED, sizeof (POINT) * histmax);
 history = GlobalLock (hMem);
 i = ReadFile (hFile, (void *) history, histmax * sizeof (POINT),
	(LPDWORD) &histlen, NULL);
 do	{
	 GlobalUnlock (hMem);
MessageBox (hwnd, "About to ReAlloc(5)", "status", MB_SYSTEMMODAL);
	 hMem = GlobalReAlloc (hMem, 2 * sizeof (POINT) * histmax, 0);
MessageBox (hwnd, "Finished ReAlloc(5)", "status", MB_SYSTEMMODAL);
	 if (hMem == NULL)
		{
		 GetFormattedError (ebuf, sizeof (ebuf), FALSE);
		 wsprintf (temp, "Internal Allocation Error: \"%s\"", ebuf);
		 MessageBox (hwnd, temp, "Error", MB_OK | MB_ICONERROR);
		}
	 history = GlobalLock (hMem);
	 i = ReadFile (hFile, (void *) &history[histmax], histmax *
		sizeof (POINT), (LPDWORD) &histlen, NULL);
	 histmax *= 2;
	 i = histmax * sizeof (POINT) / 2;
	}
 while (histlen == i);
 histlen = history[0].y;
 history[0].y = 0;
 GlobalUnlock (hMem);
 CloseHandle (hFile);
 ChangeTitle (hwnd, szFilename);
 GlobalUnlock (hMem);
 return (0);
}

void WriteDXF (HANDLE hFile)
{
 DWORD	i;
 int		x1, x2, y1, y2, len, histidx;
 char	temp[64];

 x1 = y1 = 0;
 x2 = history[1].x - history[0].x;
 y2 = history[0].y - history[1].y;
 WriteFile (hFile, dxftitle[0], strlen (dxftitle[0]), (LPDWORD)&len, NULL);
 WriteFile (hFile, dxftitle[1], strlen (dxftitle[1]), (LPDWORD)&len, NULL);
 histidx = 1;
 while (histidx < histlen)
	{
	 WriteFile (hFile, dxftitle[2], strlen (dxftitle[2]), (LPDWORD)&len, NULL);
	 len = i = 0;
	 while (history[histidx].x > history[histidx-1].x)
		{
		 ++histidx;
		 ++x2;
		 len = 1;
		}
	 i = len;
	 while (!i && history[histidx].x < history[histidx-1].x)
		{
		 ++histidx;
		 --x2;
		 len = 1;
		}
	 i = len;
	 while (!i && history[histidx].y > history[histidx-1].y)
		{
		 ++histidx;
		 --y2;
		 len = 1;
		}
	 i = len;
	 while (!i && history[histidx].y < history[histidx-1].y)
		{
		 ++histidx;
		 ++y2;
		}
	 wsprintf (temp, dxftitle[3], x1, y1, x2, y2);
	 WriteFile (hFile, temp, strlen (temp), &i, NULL);
	 x1 = x2;
	 y1 = y2;
	}
 WriteFile (hFile, dxftitle[4], strlen (dxftitle[4]), &i, NULL);
 return;
}

DWORD ToRGB (char * commanumbers)
{
 int	 r, g, b;
 char	*c;

 r = atoi (commanumbers);
 c = strchr (commanumbers, ',') + 1;
 g = atoi (c);
 c = strchr (c, ',') + 1;
 b = atoi (c);
 return (RGB (r, g, b));
}

int GetFormattedError (LPTSTR dest, int size, BOOL socket)
{
 BYTE		 width = 0;
 DWORD	 flags,
			 lang,
			 dwLastError;
 char		*p;

 if (!socket)
	dwLastError = GetLastError ();
 else
	{
//	dwLastError = WSAGetLastError ();
	 wsprintf (dest, "Network Error #%lu", WSAGetLastError () - WSABASEERR);
	 return (0);
	}
 if (!dwLastError)
{
MessageBox (NULL, "No error found, thanks", "status", MB_SYSTEMMODAL);
	return (-1);
}
 flags = FORMAT_MESSAGE_MAX_WIDTH_MASK & width;
 flags |= FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
 lang = MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT);
 flags = FormatMessage (flags, NULL, dwLastError, lang, dest, size, NULL);
 p = strchr (dest, '\r');
 if (p == NULL)
 	wsprintf (p, "Error #%lu", dwLastError);
 wsprintf (p, " [%lu]", dwLastError);
 return (flags);
}

int GetRegValue (HKEY hKey, LPCSTR lpszSubKey, LPCSTR lpszQuery,
	LPBYTE lpszValue, DWORD iSize)
{
 HKEY		hOpen;
 DWORD	Type;

 _fmemset (lpszValue, 0, iSize);
 if (RegOpenKeyEx (hKey, lpszSubKey, 0, KEY_READ, &hOpen) == ERROR_SUCCESS)
	{
	 if (RegQueryValueEx (hOpen, lpszQuery, 0, &Type, lpszValue,
								&iSize) != ERROR_SUCCESS)
		{
		 RegCloseKey (hOpen);
		 return (-1);
		}
	 RegCloseKey (hOpen);
	}
 else
	return (-1);
 return (0);
}

BOOL AddMRU (HMENU hFileMenu, int iMRUSize, LPSTR szFile)
{
 char				 temp[256],
					*szFilename;
 int				 i,
					 j,
					 k;
 static	int	 iFileNum = 1;

 for (szFilename=&szFile[1];*szFilename;++szFilename)
	if (*szFilename == '\"')
		*szFilename = '\000';
 if (szFile[0] == '\"')
	szFilename = &szFile[1];
 else
	szFilename = szFile;

 j = GetMenuItemCount (hFileMenu);
 for (i=1;i<iFileNum;i++)
	{
	 GetMenuString (hFileMenu, j - i, temp, sizeof (temp), MF_BYPOSITION);
	 if (lstrcmpi (szFilename, &temp[3]) == 0)
		return (FALSE);
	}
 wsprintf (temp, "&%d %s", 1, szFilename);
 k = CM_A_FILE + iFileNum;
 if (iFileNum == 1)
	{
	 AppendMenu (hFileMenu, MF_SEPARATOR, 0, NULL);
	 AppendMenu (hFileMenu, MF_STRING, k, temp);
	}
 else
	{
	 j = GetMenuItemCount (hFileMenu) - iFileNum + 1;
	 InsertMenu (hFileMenu, j, MF_BYPOSITION, k, temp);
	 j = GetMenuItemCount (hFileMenu);
	 for (i=1;i<iFileNum;i++)
		{
		 GetMenuString (hFileMenu, j - i, temp, sizeof (temp), MF_BYPOSITION);
		 temp[1] = (char)('1' + iFileNum - i);
		 ModifyMenu (hFileMenu, j - i, MF_BYPOSITION, CM_A_FILE + i, temp);
		}
	}

 if (iFileNum > iMRUSize)
	{
	 j = GetMenuItemCount (hFileMenu) - 1;
	 DeleteMenu (hFileMenu, j, MF_BYPOSITION);
	 for (i=1;i<iFileNum;i++)
		{
		 GetMenuString (hFileMenu, j - i, temp, sizeof (temp), MF_BYPOSITION);
		 temp[1] = (char)('1' + iMRUSize - i);
		 ModifyMenu (hFileMenu, j - i, MF_BYPOSITION, CM_A_FILE + i, temp);
		}
	}
 else
	{
	 ++iFileNum;
	}
 return (TRUE);
}

void MRUtoFile (HMENU hFileMenu, int iMRUSize, LPSTR szOutfile)
{
 int		 i;
 char		 name[128],
			 kname[128],
			 allkeys[32768],
			 nul = '*',
			*c;

 memset (allkeys, 0, sizeof (allkeys));
 wsprintf (allkeys, "nfiles=%d", iMRUSize);
 for (i=CM_A_FILE + 1;i<=CM_A_FILE+iMRUSize;i++)
	{
	 GetMenuString (hFileMenu, i, name, sizeof (name), MF_BYCOMMAND);
	 if (name[0] != '&')
		continue;
	 for (c=name;!isspace(*c);++c)
		/* Nothing */ ;
	 while (isspace (*c))
		++c;
	 wsprintf (kname, "%cFile%d=\"%s\"", nul, i - CM_A_FILE, c);
	 lstrcat (allkeys, kname);
	 lstrcat (allkeys, "\000\000\000");
	}
 for (i=0;allkeys[i];i++)
	if (allkeys[i] == nul)
		allkeys[i] = '\000';
 WritePrivateProfileSection ("Files", allkeys, szOutfile);
 WritePrivateProfileSection (NULL, NULL, NULL);
 return;
}

void MRUfromFile (HMENU hFileMenu, int iMRUSize, LPSTR szInfile)
{
 int		 i;
 char		 names[32768],
 			*fname,
			 key[128];

 GetPrivateProfileSection ("Files", names, sizeof (names) - 1, szInfile);
 for (i=0;names[i] || names[i+1];i++)
	if (!names[i])
		{
		 lstrcpy (key, &names[i+1]);
		 if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE, key, 4,
				"file", -1) == 2)
			{
			 for (fname=key;*fname!='='&&*fname;fname++)
				/* Nothing */ ;
			 ++fname;
			 if (*fname)
				AddMRU (hFileMenu, iMRUSize, fname);
			}
		}
}

int GetStringFromMenuID (HMENU hMenu, LONG id, LPSTR retstring, int size)
{
 int	i, total, examine;

 wsprintf (retstring, "\000");
 total = GetMenuItemCount (hMenu);
 for (i=0;i<total;i++)
	{
	 examine = GetMenuItemID (hMenu, i);
	 if (examine == id)
		GetMenuString (hMenu, i, retstring, size, MF_BYPOSITION);
	}
 return (lstrlen (retstring));
}

HGLOBAL LibInterpret (LPSTR lang, LPSTR message)
{
 intfunc		*Interpret;
 char			 temp[256], notice[128], *ptr;
 HGLOBAL		 hIn, hOut;
 HINSTANCE	 hLib;

 wsprintf (temp, "%s.DLL", lang);
 hLib = LoadLibrary (temp);
 if (hLib == NULL)
	{
	 GetFormattedError (temp, sizeof (temp), FALSE);
	 wsprintf (notice, "Error from %s Library", lang);
	 MessageBox (NULL, temp, notice, MB_OK);
	}
 else
	{
	 Interpret = GetProcAddress (hLib, "Interpret");
	 if (Interpret == NULL)
		{
		 GetFormattedError (temp, sizeof (temp), FALSE);
		 wsprintf (notice, "The %s library doesn't have an Interpret() function", lang);
		 MessageBox (NULL, temp, "Function Error", MB_OK);
		}
	 else
		{
		 hIn = GlobalAlloc (GPTR, lstrlen (message));
		 hOut = GlobalAlloc (GPTR, lstrlen (message) * 128);
		 ptr = GlobalLock (hIn);
		 lstrcpy (ptr, message);
		 GlobalUnlock (hIn);
		 Interpret (hIn, hOut);
		 GlobalFree (hIn);
		}
	}
 return (hOut);
}

/*
 * NB-JNC:
 *	To Create a new Registry key, use:
 *		LONG RegCreateKeyEx (hKey, lpszSubKey, 0, lpszClass, REG_OPTION_NON_VOLATILE,
 *									KEY_ALL_ACCESS, NULL, &hOpen, &NewOrOld);
 *	This creates hKey\lpszSubKey, and returns the handle in hOpen.
 *
 *	To Change a Registry value, use:
 *		LONG RegSetValueEx (hOpen, lpszSubKey, 0, REG_SZ, lpszValue, strlen (lpszValue));
 *	This sets hOpen/lpszSubKey to lpszValue.  hOpen can be gotten from
 *	RegCreateKey() or RegOpenKey(), both shown above.
 *
 *	To Delete an existing Registry value, use:
 *		LONG RegDeleteKey (hOpen, lpszSubKey);
 *	The hOpen\lpszSubKey\* tree members are removed from the registry.
 */

/*
 * NB-JNC:
 * To use Mouse Wheels:
 * WM_MOUSEWHEEL
 *		fwKeys = LOWORD(wParam);			// key flags, combination of:
 *			MK_CONTROL -- Set if the CTRL key is down.
 *			MK_LBUTTON -- Set if the left mouse button is down.
 *			MK_MBUTTON -- Set if the middle mouse button is down.
 *			MK_RBUTTON -- Set if the right mouse button is down.
 *			MK_SHIFT -- Set if the SHIFT key is down.
 *		zDelta = (short) HIWORD(wParam);	// wheel rotation, mult or div of
 *			WHEEL_DELTA == 120
 *		xPos = (short) LOWORD(lParam);	// horizontal position of pointer
 *		yPos = (short) HIWORD(lParam);	// vertical position of pointer
 * #define WM_MOUSEWHEEL (WM_MOUSELAST + 1)
 */

