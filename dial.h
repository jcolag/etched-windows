/* DIAL.H
		Creation and support code for a "dial widget" to replace
		a scrollbar.

	John Colagioia
*/

#ifndef _DIAL_H
#define _DIAL_H
#include	<math.h>
#include	<stdlib.h>
//      Need this for trig functions to work (Well, it's not NEEDED, but it
//              DOES make things easier
#define PI                      (double)((double)355/(double)113)

// If Horizontal scrolling (or other control) is ever needed here, we'll
// use these flags to represent it.
#define VSCROLL 1
#define HSCROLL 2

// These make life easier (and more readable) when accessing Window Private Data
#define MINVAL  0
#define MAXVAL  (MINVAL + sizeof (LONG))
#define CURVAL  (MAXVAL + sizeof (LONG))
#define PARENT  (CURVAL + sizeof (LONG))
#define SCTYPE  (PARENT + sizeof (LONG))

#define DIALTIMERNUM    1701

#define R                               1
#define L                               2
#define M                               4

#define SZ                              4

#define MAX(X,Y)        ((X)>(Y))?(X):(Y)
#define MIN(X,Y)        ((X)<(Y))?(X):(Y)

#define	WM_DIAL_SETTIMERDELTA	(WM_USER + DIALTIMERNUM)

long FAR PASCAL _export _DialWndProc_ (HWND, UINT, UINT, LONG);

void FixArrow (long, long, long, RECT*);

static  char    szWidgetName[] = "DialWidget";
//static  HDC     hdcMem;
POINT           FAR     ptArrow[] =
					{
					 { 0.0,  0.0},
					 {-1.0, -1.0},
					 { 0.0,  1.0},
					 { 1.0, -1.0},
					},
				FAR     ptDraw[SZ];
long        fButton = 0,
				nTimeDef = 200,
				nDialTimerLen = 200,
				nTimerDiff = 15,
				nTimerMin = 1;
char			msg[80];
WNDCLASS    DialClass;

void RegisterDial (HANDLE hInstance, HANDLE hPrevInstance, HBRUSH hColor, int timer)
{
 if (!hPrevInstance)
	{
	 DialClass.style = CS_BYTEALIGNCLIENT;			// For BitBlt ()
	 DialClass.lpfnWndProc = _DialWndProc_;
	 DialClass.cbClsExtra = 0;
	 DialClass.cbWndExtra = 4 * sizeof (long) +		// nMin, nMax, nCurrent,
						sizeof (HANDLE);		// type, parent window
	 DialClass.hInstance = hInstance;
	 DialClass.hIcon = NULL;
	 DialClass.hCursor = NULL;
	 DialClass.hbrBackground = hColor;
	 DialClass.lpszMenuName = NULL;
	 DialClass.lpszClassName = szWidgetName;

	 RegisterClass (&DialClass);
	}
 if (timer)
 	nDialTimerLen = nTimeDef = timer;
}

HWND CreateDial (HWND hParent, HANDLE hInstance,
						long x, long y, long ht, long wd, LONG type)
{
 HWND                   hDial;

 hDial = CreateWindow (	szWidgetName,				// Window Class
					NULL,					// No caption
					WS_CHILD | WS_DLGFRAME,
					x, y,					// Position
					ht, wd,					// Size
					hParent,					// Obvious...
					NULL,					// No menu
					hInstance,				// Program
					NULL);
 SetWindowLong (hDial, MINVAL, (LONG)-1);
 SetWindowLong (hDial, PARENT, (LONG)hParent);
 SetWindowLong (hDial, SCTYPE, type);
 ShowWindow (hDial, SW_SHOWNORMAL);
 UpdateWindow (hDial);
 return hDial;
}

void SetDialRange (HWND hDial, long fnDial, LONG nMin, LONG nMax, BOOL fRedraw)
{
 RECT   rect;

 if (fnDial == SB_CTL)
	{
	 SetWindowLong (hDial, MINVAL, nMin);
	 SetWindowLong (hDial, MAXVAL, nMax);
	}
 if (fRedraw == TRUE)
	{
	 GetClientRect (hDial, &rect);
	 InvalidateRect (hDial, &rect, TRUE);
	 UpdateWindow (hDial);
	 }
}

LONG SetDialPos (HWND hDial, long fnDial, LONG nPos, BOOL fRepaint)
{
 LONG	temp;
 RECT	rect;

 temp = GetWindowLong (hDial, CURVAL);
 if (fnDial == SB_CTL) SetWindowLong (hDial, CURVAL, nPos);
 if (fRepaint == TRUE)
	{
	 GetClientRect (hDial, &rect);
	 InvalidateRect (hDial, &rect, TRUE);
	 UpdateWindow (hDial);
	 }
 return temp;                   // Old value of nPos
}

LONG GetDialPos (HWND hDial, long fnDial)
{
 if (fnDial == SB_CTL) return (GetWindowLong (hDial, CURVAL));
 else return 0;
}

void GetDialRange (HWND hDial, long fnDial, long FAR *lpnMin, long FAR *lpnMax)
{
 if (fnDial == SB_CTL)
	{
	 *lpnMin = GetWindowLong (hDial, MINVAL);
	 *lpnMax = GetWindowLong (hDial, MAXVAL);
	}
}

long FAR PASCAL _export _DialWndProc_ (HWND hDial, UINT message, UINT wParam,
													LONG lParam)
{
 HDC				hdc;
 static HDC		hdcMem;
 PAINTSTRUCT		ps;
 RECT			rect, drect;
 long			ht, wd, dia, type, nMin, nMax, nPos;
 HPEN			dialpen, oldpen;
 HBRUSH			dialbk, oldbk;
 HWND			hParent;
 static HBITMAP	hBitmap, hOldBit;

 switch (message)
	{
	 case WM_TIMER:
			if (nDialTimerLen > 5) nDialTimerLen -= (long) nTimerDiff;
			if (nDialTimerLen < 1) nDialTimerLen = nTimerMin;
			if (fButton == L) SendMessage (hDial, WM_LBUTTONDOWN, 0, 0L);
			if (fButton == R) SendMessage (hDial, WM_RBUTTONDOWN, 0, 0L);
			if (fButton == (L | R)) // It's silly, but I like it...
				{
				 if (random (2)) SendMessage (hDial, WM_LBUTTONDOWN, 0, 0L);
				 else SendMessage (hDial, WM_RBUTTONDOWN, 0, 0L);
				}
			break;

	 case WM_CREATE:
			return 0;

	 case WM_SIZE:                   // Creates the dial's circle when resized
			hdc = BeginPaint (hDial, &ps);
			GetClientRect (hDial, &rect);
			hdcMem = CreateCompatibleDC (hdc);
			hBitmap = CreateCompatibleBitmap (hdc, rect.right-rect.left, rect.bottom-rect.top);
			hOldBit = SelectObject (hdcMem, hBitmap);
			drect.top = rect.top;
			ht = rect.bottom - rect.top;
			wd = rect.right - rect.left;
			dia = 0.9 * (ht<wd)?ht:wd;              // Get the smaller of the two and shrink
			drect.left = rect.left + (wd - dia) / 2;
			drect.top = rect.top + (ht - dia) / 2;
			drect.right = rect.right - (wd - dia) / 2;
			drect.bottom = rect.bottom - (wd - dia) / 2;
									// Center the dial in the box

			dialpen = CreatePen (PS_DOT, (ht / 40), RGB (128,64,64));
			dialbk  = CreateSolidBrush (RGB (64,64,64));
			oldpen = SelectObject (hdcMem, dialpen);
			oldbk = SelectObject (hdcMem, dialbk);

			FillRect (hdcMem, &rect, DialClass.hbrBackground);
			Chord (hdcMem, drect.left, drect.top, drect.right, drect.bottom,
						(drect.right - drect.left)/2, drect.top, (drect.right - drect.left)/2,
						drect.top);     // Draw the circle

			SelectObject (hdcMem, oldpen);
			SelectObject (hdcMem, oldbk);
			DeleteObject (dialpen);
			DeleteObject (dialbk);
			EndPaint (hDial, &ps);

			break;

	 case WM_PAINT:
			hdc = BeginPaint (hDial, &ps);
			GetClientRect (hDial, &rect);
			BitBlt (hdc, 0, 0, rect.right-rect.left, rect.bottom-rect.top,
														hdcMem, 0, 0, SRCCOPY);
										// Return circle w/o arrow

			GetDialRange (hDial, SB_CTL, &nMin, &nMax);
			nPos = GetDialPos (hDial, SB_CTL);
			FixArrow (nMin, nMax, nPos, &rect);

			dialpen = GetStockObject (BLACK_PEN);
			oldpen = SelectObject (hdc, dialpen);
			dialbk  = GetStockObject (GRAY_BRUSH);
			oldbk = SelectObject (hdc, dialbk);
			Polygon (hdc, ptDraw, SZ);

			SelectObject (hdc, oldpen);
			SelectObject (hdc, oldbk);
			EndPaint (hDial, &ps);
			break;

	 case WM_RBUTTONDOWN:
			fButton |= R;
			type = GetWindowLong (hDial, SCTYPE);   // Type of control for dial
			hParent = (HWND) GetWindowLong (hDial, PARENT);
			switch (type)
				{
				 case VSCROLL:
                         PostMessage (hParent, WM_VSCROLL, SB_LINEDOWN, 1L);
					SetDialPos (hDial, SB_CTL,
                         	(LONG)(GetDialPos (hDial, SB_CTL) + 1), TRUE);
					break;
				 case HSCROLL:
                         PostMessage (hParent, /*WM_HSCROLL*/25178, SB_LINEDOWN, 1L);
					SetDialPos (hDial, SB_CTL,
                         	(LONG)(GetDialPos (hDial, SB_CTL) + 1), TRUE);
					break;
				}
			SetTimer (hDial, DIALTIMERNUM, nDialTimerLen, NULL);
			break;

	 case WM_LBUTTONDOWN:
			fButton |= L;
			type = GetWindowLong (hDial, SCTYPE);   // Type of control for dial
			hParent = (HWND) GetWindowLong (hDial, PARENT);
			SetWindowLong (hDial, CURVAL, (LONG)(GetWindowLong (hDial, CURVAL) - 1));
			if (type == VSCROLL)
               	{
                     PostMessage (hParent, WM_VSCROLL, SB_LINEUP, 1L);
				 SetDialPos (hDial, SB_CTL,
					(LONG)(GetDialPos (hDial, SB_CTL) - 1), TRUE);
				}
			else
				{
				 PostMessage (hParent, /*WM_HSCROLL*/25178, SB_LINEUP, 1L);
				 SetDialPos (hDial, SB_CTL,
					(LONG)(GetDialPos (hDial, SB_CTL) - 1), TRUE);
				}
			SetTimer (hDial, DIALTIMERNUM, nDialTimerLen, NULL);
			break;

	 case WM_MBUTTONDOWN:
			fButton ^= M;
			break;

	 case WM_LBUTTONUP:
			fButton &= (!L);
			KillTimer (hDial, DIALTIMERNUM);
			nDialTimerLen = nTimeDef;
			break;

	 case WM_RBUTTONUP:
			fButton &= (!R);
			KillTimer (hDial, DIALTIMERNUM);
			nDialTimerLen = nTimeDef;
			break;

	 case WM_DIAL_SETTIMERDELTA:
			type = nTimerDiff;
			nTimerDiff = wParam;
			if (lParam != 0)
				nTimerMin = lParam;
			return (type);

	 case WM_DESTROY:
			DeleteObject (DialClass.hbrBackground);
			break;
	}

 return DefWindowProc (hDial, message, wParam, lParam);
}

void FixArrow (long nMin, long nMax, long nPos, RECT *rect)
{
 long		nRange, nRad, i, xctr, yctr;
 float		percent, angle, thcos, thsin;

 nRad = MIN ((rect->bottom - rect->top), (rect->right - rect->left)) / 1.9;

 nRange = nMax - nMin;
 if (nRange == 0) percent = 0;
 else percent = (float)(nPos - nMin) / (float)nRange;
 angle = (135 * PI / 180) - (percent * 1.5 * PI);
 thcos = cos (angle);
 thsin = sin (angle);

 xctr = (rect->right - rect->left) / 2 + rect->left;
 yctr = (rect->bottom - rect->top) / 2 + rect->top;

 // Modified the transformation to look more spectacular.  Don't look too
 //             close or you'll hurt your eyes!
 for (i=0;i<SZ;i++)
	{
//                      Here's the normal transformation...
//       ptDraw[i].x = (ptArrow[i].x * thcos - ptArrow[i].y * thsin) * nRad + xctr;
//       ptDraw[i].y = yctr - (ptArrow[i].x * thsin + ptArrow[i].y * thcos) * nRad;
//                      But this one's more interesting...
	 ptDraw[i].x = (ptArrow[i].x * thcos - ptArrow[i].y * thsin) * nRad + xctr;
	 ptDraw[i].y = (ptArrow[i].x * thsin - ptArrow[i].y * thcos) * nRad + yctr;
	}
/*
 wsprintf (msg, "(%d,%d),(%d,%d),(%d,%d),(%d,%d)", ptDraw[0].x, ptDraw[0].y,
																	ptDraw[1].x, ptDraw[1].y,
																	ptDraw[2].x, ptDraw[2].y,
																	ptDraw[3].x, ptDraw[3].y);
 MessageBox (NULL, NULL, msg, MB_SYSTEMMODAL);
*/
}

#endif
