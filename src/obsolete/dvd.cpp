#ifdef WIN32
#include <windows.h>
#include <strmif.h>
#include <control.h>
#include <uuids.h>
#endif

#ifdef LINUX
#include <string.h>	// for bzero
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>	// for inet_addr
#include <unistd.h>	// for read/write
#endif

#include "dvd.h"
#include "daphne.h"
#include "conout.h"
#include <stdio.h>

int g_sockfd = -1;

#ifdef WIN32

int dvd_init()
{

	WSADATA wsaData;
	struct sockaddr_in saRemote;
	int iRetCode;

	WSAStartup(MAKEWORD(1,1), &wsaData);

	ZeroMemory(&saRemote, sizeof(saRemote));
	saRemote.sin_family = AF_INET;
	saRemote.sin_addr.s_addr = inet_addr(get_vldp_ip());
	saRemote.sin_port = htons(get_vldp_port());

	g_sockfd = socket(AF_INET, SOCK_STREAM, 0);

	iRetCode = connect(g_sockfd, (struct sockaddr *) &saRemote, sizeof(saRemote));

	return ((iRetCode + 1) && (g_sockfd));	// iRetCode is -1 if failure, 0 if success

}

void dvd_shutdown()
{
}

int dvd_search(unsigned int frame)
{
	char szSeek[80];

	sprintf(szSeek, "seek %d\n", frame);
	send(g_sockfd, szSeek, strlen(szSeek), 0);
	Sleep(0);
	recv(g_sockfd, szSeek, 80, 0);

	return 1;
}


void dvd_play()
{	

	char szSeek[80];

	send(g_sockfd, "play\n", 5, 0);
	Sleep(0);
	recv(g_sockfd, szSeek, 80, 0);
}

#endif

#ifdef LINUX

int dvd_init()
{

	int result = 0;
	struct sockaddr_in saRemote;

	g_sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (g_sockfd != -1)
	{

		bzero(&saRemote, sizeof(saRemote));
		saRemote.sin_family = AF_INET;
		saRemote.sin_addr.s_addr = inet_addr(get_vldp_ip());
		saRemote.sin_port = htons(get_vldp_port());

		result = connect(g_sockfd, (struct sockaddr *) &saRemote, sizeof(saRemote));
		if (result == -1)
		{
			printline("Critical error, could not connect to VLDP's port");
		}
	}
	else
	{
		printline("Could not open TCP socket! =(");
	}

	return ((result + 1) && (g_sockfd));	// result is -1 on failure, 0 on success

}

void dvd_shutdown()
{
	close(g_sockfd);
}

// uses unix sockets code
// returns 1 on success, 0 on error
int dvd_search(unsigned int frame)
{

	char s[81] = { 0 };
	int result = 0;

	sprintf(s, "seek %d\n", frame);

	// if our info was successfully sent
	if (write (g_sockfd, s, strlen(s)) > 0)
	{
		// if we successfully read the response
		if (read (g_sockfd, s, sizeof(s)) > 0)
		{
			result = 1;
		}
		else
		{
			printline("Error reading from VLDP");
		}
	}
	else
	{
		printline("Error writing to VLDP");
	}
	
	return (result);

}

void dvd_play()
{

	char s[81] = { 0 };

	if(g_sockfd != -1)
	{
		sprintf(s, "play\n");
		write(g_sockfd, s, strlen(s));
		read(g_sockfd, s, sizeof(s));
	}

}

#endif
