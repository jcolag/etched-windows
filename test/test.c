#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <winsock.h>
#include <stdio.h>

int main (int argc, char *argv[])
{
 struct sockaddr_in sin;
 int sock, i;
 short port;
 char tempstr[256];
 WSADATA wsadata;

 port = htons ((short) atoi (argv[1]));

 WSAStartup (MAKEWORD (2,0), &wsadata);
 memset (&sin, 0, sizeof (sin));
 sin.sin_family = AF_INET;
 sin.sin_port = port;
 sin.sin_addr.s_addr = inet_addr ("127.0.0.1");
 sock = socket (PF_INET, SOCK_STREAM, 6);
 printf ("Socket %d on Port %d\n", sock, ntohs (port));
 i = connect (sock, (struct sockaddr *)&sin, sizeof (sin));
 if (i<0) i = WSAGetLastError ();
 printf ("Status of connect():  %d\nSending \"%s\"\n", i, argv[2]);
 i = send (sock, argv[2], strlen (argv[2]), 0);
 printf ("Status of send(): %d\n", i);
 while ((i = recv (sock, tempstr, sizeof (tempstr), 0)) > 0)
	{
	 printf ("Status of recv(): %d\n", i);
	 printf ("%s\n", tempstr);
	 memset (tempstr, 0, sizeof (tempstr));
	}
 closesocket (sock);
 WSACleanup ();
 return (argc);
}
