#include <windows.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

int		 statements	(void);
void		 output (HGLOBAL, LPSTR);
int		 MemGetChar (HGLOBAL);
BOOL		 EndMem (HGLOBAL);

char		 mesg[2048];
HGLOBAL	 hi, ho;

int FAR PASCAL LibMain (HANDLE hInstance, WORD wDataSeg, WORD wHeapSize,
	LPSTR lpszCmdLine)
{
 if (wHeapSize > 0)
	return (wDataSeg != wHeapSize);
 return (1);
}

int FAR PASCAL _export WEP (int nParam)
{
 return (1);
}

BOOL FAR PASCAL _export Interpret (HGLOBAL hmIn, HGLOBAL hmOut)
{
 char		*ctmp;

 ctmp = GlobalLock (hmOut);
 strcpy (ctmp, "\000");
 GlobalUnlock (hmOut);

 MemGetChar (NULL);

 statements ();
 return (0);
}

int statements (void)
{
 int	 ret;

 do
	{
	}
 while (ret == VALID);
 return (0);
}

void output (HGLOBAL hOut, LPSTR message)
{
 char				*input;
 int				 memsize,
					 oldlen;

 memsize = GlobalSize (hOut);
 input = GlobalLock (hOut);
 oldlen = lstrlen (input);
 GlobalUnlock (hOut);
 if (memsize < lstrlen (message) + oldlen)
	GlobalReAlloc (hOut, lstrlen (message) + oldlen, GMEM_MOVEABLE);
 input = GlobalLock (hOut);
 lstrcat (input, message);
 GlobalUnlock (hOut);
 return;
}

int MemGetChar (HGLOBAL hMem)
{
 static	int	 index = 0;
 int				 ch;
 char				*input;

 if (hMem == NULL)
	{
	 index = 0;
	 return (NULL);
	}

 input = GlobalLock (hMem);
 ch = input[index];
 remaining = lstrlen (&input[index]);
 ++index;
 if (input[index] == '\000')
	remaining = 0;
 GlobalUnlock (hMem);
 if (ch == '\000')
 	return (-1);
 return (ch);
}

BOOL EndMem (HGLOBAL hMem)
{
 return (!remaining);
}

