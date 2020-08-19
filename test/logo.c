#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <winsock.h>

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
void		 output		(char *);
int		 round		(double);
void		 cmdexecute (int *, int);

char		 token_text[256];
int		 addto,
			 mlen = 0;
double	 angle,
			 xcoord = 0.0,
			 ycoord = 0.0;
char		 mesg[2048];
FILE		*infile;

int main (int argc, char *argv[])
{
 WSADATA wsadata;

 WSAStartup (MAKEWORD (2, 0), &wsadata);
 angle = M_PI_2;
 addto = 0;
 if (argc < 2)
	infile = stdin;
 else
	{
	 infile = fopen (argv[1], "r");
	 if (infile == NULL)
		{
		 fprintf (stderr, "\n%s is unable to open file:  \"%s\"\n\n",
					argv[0], argv[1]);
		 exit (1);
		}
	}
 statements ();
 WSACleanup ();
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
		 output (mesg);
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

 tx = round (xcoord);
 ty = round (ycoord);

 if (cmd == 0)
	{
	 dir = lexan ();
	 if (dir == ']')
		return (dir);

	 token = lexan ();
	 if (token != T_NM)
		{
		 fprintf (stderr, "Invalid Token (%d): \"%s\"\n", token, token_text);
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
				 output (mesg);
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
				 output (mesg);
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
					 fprintf (stderr, "Invalid command: \"%s\"\n", token_text);
					 return (NOT_VALID);
					}
				 cmdlist[index++] = token;
				 if (token != ']')
					{
					 i = lexan ();
					 if (i != T_NM)
						{
						 fprintf (stderr, "Invalid number: \"%s\"\n", token_text);
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
						 fprintf (stderr, "Malformed loop: \"%s\"\n", token_text);
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
			 fprintf (stderr, "Malformed loop\n");
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
			char	word[16];

 if (c == 0)
	c = fgetc (infile);
 memset (word, 0, sizeof (word));
 while (!feof (infile) && (c != -1))
	{
	 while (isspace (c))
		c = fgetc (infile);

	 if (isdigit (c))
		{
		 while (isdigit (c))
			{
			 word[index++] = (char)(c & 0xFF);
			 if (index == 16)
				fprintf (stderr, "Integer Overflow: (\"%s\")\n", word);
			 c = fgetc (infile);
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
				fprintf (stderr, "Lexical Overflow: (\"%s\")\n", word);
			 c = fgetc (infile);
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

void output (char * message)
{
 struct	sockaddr_in	 sin;
 int						 sock,
							 i;
 short					 port = 2342;
 char						 tempstr[128];

 if (strlen (message) == 0)
	return;
 strcat (message, "Q");
 sock = socket (PF_INET, SOCK_STREAM, 6);
 if (sock < 0)
	{
	 fprintf (stderr, "Error %d--Cannot allocate socket.\n", WSAGetLastError ());
	 return;
	}
 memset (&sin, 0, sizeof (sin));
 sin.sin_family = AF_INET;
 sin.sin_port = htons (port);
 sin.sin_addr.s_addr = inet_addr ("127.0.0.1");
 i = connect (sock, (struct sockaddr *)&sin, sizeof (sin));
 if (i<0)
	{
	 i = - WSAGetLastError ();
	 fprintf (stderr, "Error %d--Cannot connect to remote Etch-A-Sketch.\n", i);
	}
 i = send (sock, message, strlen (message), 0);
 if (i<0)
	{
	 i = - WSAGetLastError ();
	 fprintf (stderr, "Error %d--Cannot communicate with Etch-A-Sketch.\n", i);
	}
 while ((i = recv (sock, tempstr, sizeof (tempstr), 0)) > 0)
	memset (tempstr, 0, sizeof (tempstr));
 closesocket (sock);
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

