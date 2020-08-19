#include <windows.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#define	VALID			1
#define	NOT_VALID	-1
#define	DONE			0
#define  T_NM			256
#define	T_FD			257
#define	T_BK			258
#define	T_RT			259
#define	T_LT			260
#define	T_RP			261
#define	T_UN			262
#define	T_MAX			263

int		 statements	(void);
int		 statement	(int, int);
int		 lexan		(void);
void		 output (HGLOBAL, LPSTR);
int		 round		(double);
void		 cmdexecute (int *, int);
int		 MemGetChar (HGLOBAL);
BOOL		 EndMem (HGLOBAL);

char		 token_text[256];
int		 addto,
			 mlen = 0,
			 restart = 0,
			 remaining = 0;
double	 angle,
			 xcoord = 0.0,
			 ycoord = 0.0;
char		 mesg[2048];
HGLOBAL	 hi, ho;

int FAR PASCAL LibMain (HANDLE hInstance, WORD wDataSeg, WORD wHeapSize,
	LPSTR lpszCmdLine)
{
 if (wHeapSize > 0)
	return (wDataSeg != wHeapSize);
//	UnlockData (0);
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

 angle = M_PI_2;
 addto = 0;
 hi = hmIn;
 ho = hmOut;
 statements ();
 return (0);
}

int statements (void)
{
 int	 ret;

 do
	{
	 ret = statement (0, 0);
	 if (addto == 0)
		{
		 output (ho, mesg);
		 mlen = 0;
		 memset (mesg, 0, sizeof (mesg));
		}
	 if (ret == ']')
		return (']');
	}
 while (ret == VALID);
 return (0);
}

int statement (int cmd, int amt)
{
 int		 dir,
			 token,
			 i,
			 value,
			 sign,
			 tx1,
			 ty1,
			 tx,
			 ty,
			 cmdlist[2048],
			 index = 0;
 float	 xcoord1,
			 ycoord1;
 char		 stderr[256];

 tx = round (xcoord);
 ty = round (ycoord);

 if (cmd == 0)
	{
	 dir = lexan ();
	 if (dir == ']')
		return (dir);
	 if (dir <= DONE)
	 	return (DONE);

	 token = lexan ();
	 if (token != T_NM)
		{
		 wsprintf (stderr, "Invalid Token (%d) for direction (%d): \"%s\"\n", token, dir, token_text);
		 MessageBox (NULL, stderr, "Error", MB_SYSTEMMODAL);
		 return (NOT_VALID);
		}
	 value = atoi (token_text);
	}
 else
	{
	 dir = cmd;
	 value = amt;
	}

 switch (dir)
	{
	 case T_FD:
	 case T_BK:
		if (dir == T_FD)
			sign = 1;
		else
			sign = -1;
		for (i=0;i<value;i++)
			{
			 xcoord1 = xcoord + sign * cos (angle);
			 ycoord1 = ycoord + sign * sin (angle);
			 tx1 = round (xcoord1);
			 ty1 = round (ycoord1);
			 if ((mlen + 4) > sizeof (mesg))
				{
				 output (ho, mesg);
				 memset (mesg, 0, sizeof (mesg));
				 mlen = 0;
				}
			 if (tx1 == tx + 1) { mesg[mlen++] = 'R'; }
			 if (tx1 + 1 == tx) { mesg[mlen++] = 'L'; }
			 if (ty1 == ty + 1) { mesg[mlen++] = 'U'; }
			 if (ty1 + 1 == ty) { mesg[mlen++] = 'D'; }
			 xcoord = xcoord1;
			 ycoord = ycoord1;
			 tx = round (xcoord);
			 ty = round (ycoord);
			}
		break;
	 case T_LT:
		angle += value * M_PI / 180;
		break;
	 case T_RT:
		angle -= value * M_PI / 180;
		break;
	 case T_UN:
		for (i=0;i<value;i++)
			{
			 if ((mlen + 4) > sizeof (mesg))
				{
				 output (ho, mesg);
				 mlen = 0;
				 memset (mesg, 0, sizeof (mesg));
				}
			 mesg[mlen++] = 'Z';
			}
		break;
	 case T_RP:
		token = lexan ();
		if (token == '[')
			{
			 ++addto;
			 do{
				 token = lexan ();
				 if ((token <= T_NM || token >= T_MAX) && token != ']')
					{
					 wsprintf (stderr, "Invalid command: \"%s\"\n", token_text);
					 MessageBox (NULL, stderr, "Error", MB_SYSTEMMODAL);
					 return (NOT_VALID);
					}
				 cmdlist[index++] = token;
				 if (token != ']')
					{
					 i = lexan ();
					 if (i != T_NM)
						{
						 wsprintf (stderr, "Invalid number: \"%s\"\n", token_text);
						 MessageBox (NULL, stderr, "Error", MB_SYSTEMMODAL);
						 return (NOT_VALID);
						}
					 cmdlist[index++] = atoi (token_text);
					}
				 else
					{
					 cmdlist[index++] = 0;
					 --addto;
					}
				 if (token == T_RP)
					if ((token = lexan()) != '[')
						{
						 wsprintf (stderr, "Malformed loop: \"%s\"\n", token_text);
						 MessageBox (NULL, stderr, "Error", MB_SYSTEMMODAL);
						 return (NOT_VALID);
						}
					else
						++addto;
				}
			 while (token != ']' || addto);
			 for (i=0;i<value;i++)
				cmdexecute (cmdlist, index-2);
			}
		else
			{
			 wsprintf (stderr, "Malformed loop\n");
			 MessageBox (NULL, stderr, "Error", MB_SYSTEMMODAL);
			 return (NOT_VALID);
			}
		break;
	 default:
		return (NOT_VALID);
	}
 return (VALID);
}

int lexan (void)
{
 static	int	c = 0;
			int	index = 0;
			char	word[16],
					stderr[256];

 if (c == 0)
	c = MemGetChar (hi);
 memset (word, 0, sizeof (word));
 while (!EndMem (hi) && (c != -1))
	{
	 while (isspace (c))
		c = MemGetChar (hi);

	 if (isdigit (c))
		{
		 while (isdigit (c))
			{
			 word[index++] = (char)(c & 0xFF);
			 if (index == 16)
				{
				 wsprintf (stderr, "Integer Overflow: (\"%s\")\n", word);
				 MessageBox (NULL, stderr, "Error", MB_SYSTEMMODAL);
				}
			 c = MemGetChar (hi);
			}
		 word[index] = '\000';
		 strcpy (token_text, word);
		 return (T_NM);
		}

	 else if (isalpha (c))
		{
		 while (isalpha (c))
			{
			 word[index++] = (char)(c & 0xFF);
			 if (index == 16)
				{
				 wsprintf (stderr, "Lexical Overflow: (\"%s\")\n", word);
				 MessageBox (NULL, stderr, "Error", MB_SYSTEMMODAL);
				}
			 c = MemGetChar (hi);
			}
		 word[index] = '\000';
		 strcpy (token_text, word);
		 if (!strcmp (word, "fd") || !strcmp (word, "forward"))
			return (T_FD);
		 if (!strcmp (word, "bk") || !strcmp (word, "back"))
			return (T_BK);
		 if (!strcmp (word, "rt") || !strcmp (word, "right"))
			return (T_RT);
		 if (!strcmp (word, "lt") || !strcmp (word, "left"))
			return (T_LT);
		 if (!strcmp (word, "undo"))
			return (T_UN);
		 if (!strcmp (word, "repeat"))
			return (T_RP);
		}
	 else
		{
		 token_text[0] = (char)(c & 0xFF);
		 token_text[1] = '\000';
		 index = c;
		 c = 0;
		 return (index);
		}
	}
 return (DONE);
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

int round (double num)
{
 int		trunc;

 trunc = floor (num);
 if ((num - trunc) >= 0.5)
	++trunc;
 return (trunc);
}

void cmdexecute (int * commands, int length)
{
 int	i, j, k;

 for (i=0;i<length;i+=2)
{
	if (commands[i] != T_RP)
		statement (commands[i], commands[i+1]);
	else
		{
		 for (j=length-2;commands[j]!=']'&&j;j-=2)
			/* Nothing */ ;
		 for (k=0;k<commands[i+1];k++)
			 cmdexecute (&commands[i+2], j-i-2);
		 i = j;
		}
}
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

