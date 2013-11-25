#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include "Request.h"
#include <stropts.h>
#include <sys/ioctl.h>
#include <linux/netdevice.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>

#define ECHOMAX 255
using namespace std;

void DieWithError(const char *);
char CharToSend();
void ValidateInputArguments(int, const char*);
int GetIncarnationNumber(bool);
string GetAddresses(const int);
int main(int argc, const char* argv[]) {
	int socketDescriptor;
    struct sockaddr_in echoServAddr; /* Echo server address */
    struct sockaddr_in fromAddr;     /* Source address of echo */
    unsigned short echoServerPort;   /* Echo server port */
    unsigned int fromSize;           /* In-out of address size for recvfrom() */
    const char *serverIp;            /* IP address of server */
    char echoBuffer[ECHOMAX + 1];    /* Buffer for receiving echoed string */
    int respStringLen;               /* Length of received response */
    int successfulRequests = 0;

	//GetIncarnationNumber();
    ValidateInputArguments(argc, argv[0]);
    serverIp = argv[1];
	
    if (argc == 4)
        echoServerPort = atoi(argv[3]);
    else
        echoServerPort = 7;

    if ((socketDescriptor = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Construct the server address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));    /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                 /* Internet addr family */
    echoServAddr.sin_addr.s_addr = inet_addr(serverIp);  /* Server IP address */
    echoServAddr.sin_port   = htons(echoServerPort);     /* Server port */
	
	srand((unsigned)time(NULL));
	int beginFailure = (rand() % 20);
	printf("beginFailure: %d", beginFailure)
	while (successfulRequests < 20) 
	{
		struct requestf r;
		r.inc = GetIncarnationNumber(false);
		r.client = atoi(argv[2]);
		r.req = successfulRequests;
		string s = GetAddresses(AF_INET);
		int sizeOfString=s.size();
	
		for (int i=0;i<=sizeOfString;i++)
        {
            r.client_ip[i]=s[i];
        }	
		r.c = CharToSend();
		cout << "Sending : " << r.c << endl;
		//PrintRequest(&r); 
		const unsigned char * const px = (unsigned char*)&r;
		/*
		unsigned int i;
		cout << "HEX DUMPING" << endl;
		for (i = 0; i < sizeof(r); ++i) printf("%02X ", px[i]);
		cout << "DONE" << endl;
		*/
		
		if(r.req >= beginFailure)
		{
			srand((unsigned)time(NULL));
			bool clientFailure = (rand() % 100) < 50; //may need seed by time
			printf("clientFailure: %d", clientFailure)
			if(!clientFailure)
			{
				int sentdata = sendto(socketDescriptor, px, sizeof(r), 0, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr));
				if(sentdata == -1)
				{ 
					printf("%d \n", errno);
					DieWithError("sendto() sent a different number of bytes than expected");
				}

				fromSize = sizeof(fromAddr);
				respStringLen = recvfrom(socketDescriptor, echoBuffer, ECHOMAX, 0, (struct sockaddr *) &fromAddr, &fromSize);
				if (respStringLen < (int)sizeof(char)*5)
					DieWithError("recvfrom() failed");

				if (echoServAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr)
				{
					fprintf(stderr,"Error: received a packet from unknown source.\n");
					exit(1);
				}       
		
				/* null-terminate the received data */
				echoBuffer[respStringLen] = '\0';
				printf("Received: %s\n\n", echoBuffer);    /* Print the echoed arg */	
				successfulRequests++;
			}
			else
			{
				GetIncarnationNumber(true);
				printf("Client failed to send request.");
				//increment incarnation number
			}
		}
		else
		{
			int sentdata = sendto(socketDescriptor, px, sizeof(r), 0, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr));
			if(sentdata == -1)
			{ 
				printf("%d \n", errno);
				DieWithError("sendto() sent a different number of bytes than expected");
			}

			fromSize = sizeof(fromAddr);
			respStringLen = recvfrom(socketDescriptor, echoBuffer, ECHOMAX, 0, (struct sockaddr *) &fromAddr, &fromSize);
			if (respStringLen < (int)sizeof(char)*5)
				DieWithError("recvfrom() failed");

			if (echoServAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr)
			{
				fprintf(stderr,"Error: received a packet from unknown source.\n");
				exit(1);
			}       
		
			/* null-terminate the received data */
			echoBuffer[respStringLen] = '\0';
			printf("Received: %s\n\n", echoBuffer);    /* Print the echoed arg */	
			successfulRequests++;
		}
		
	}
	close(socketDescriptor);
	exit(0);
	return 0;
}

int GetIncarnationNumber(bool setIncarnationNumber)
{
	cout << "Updating Incarnation Number" << endl;
	bool failedToUpdate = true;
	while(failedToUpdate)
	{
		failedToUpdate = false;
		struct flock fl;
		int fd;
			
		fl.l_type   = F_WRLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
		fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
		fl.l_start  = 0;        /* Offset from l_whence         */
		fl.l_len    = 0;        /* length, 0 = to EOF           */
		fl.l_pid    = getpid(); /* our PID                      */

		fd = open("inc.txt", O_WRONLY);

		fcntl(fd, F_SETLKW, &fl);  /* F_GETLK, F_SETLK, F_SETLKW */
		
		if ((fd = open("inc.txt", O_RDWR)) == -1)
		{
			failedToUpdate = true;
			continue;
			//DieWithError("GetLockedFile: open");
		}
		printf("Trying to get lock...");

		if (fcntl(fd, F_SETLKW, &fl) == -1)
		{
			failedToUpdate = true;
			continue;
			//DieWithError("GetLockedFile: fcntl");
		}
			
		printf("got lock\n");
		
		FILE * readIncarnation = fopen ("inc.txt", "r");
		int currentIncarnationNumber = 0;
		fscanf (readIncarnation, "%d", &currentIncarnationNumber);
		fclose(readIncarnation);
		
		if(setIncarnationNumber)
			currentIncarnationNumber++;
			
		FILE * writeIncarnation = fopen ("inc.txt", "w");
		fprintf(writeIncarnation, "%d", currentIncarnationNumber);
		fclose(writeIncarnation);
	
		fl.l_type = F_UNLCK;  /* set to unlock same region */

		if (fcntl(fd, F_SETLK, &fl) == -1) 
		{
			perror("fcntl failed to unlock file");
			exit(1);
		}

		printf("Unlocked.\n");

		close(fd);
		return currentIncarnationNumber;
	}
}

void ValidateInputArguments(int argc, const char* argv)
{
	if ((argc < 3) || (argc > 4))
    {
        fprintf(stderr,"Usage: %s <Server IP> <Client Number> [<Echo Port>]\n", argv);
        exit(1);
    }
}

char CharToSend() 
{
	char randChar = ' ';
	int randNum = 0;
	randNum = 26 * (rand() / (RAND_MAX + 1.0));
	randNum += 97;
	randChar = (char)randNum;
	
	return randChar;
}

void DieWithError(const char * errorMessage)
{
    perror(errorMessage);
    exit(1);
}

string GetAddresses(const int domain)
{
	int s;
	struct ifconf ifconf;
	struct ifreq ifr[50];
	int ifs;
	int i;

	s = socket(domain, SOCK_STREAM, 0);
	if (s < 0) 
	{
		perror("socket");
		return 0;
	}

	ifconf.ifc_buf = (char *) ifr;
	ifconf.ifc_len = sizeof ifr;

	if (ioctl(s, SIOCGIFCONF, &ifconf) == -1) 
	{
		perror("ioctl");
		return 0;
	}

	ifs = ifconf.ifc_len / sizeof(ifr[0]);
	for (i = 0; i < ifs; i++) 
	{
		char ip[INET_ADDRSTRLEN];
		struct sockaddr_in *s_in = (struct sockaddr_in *) &ifr[i].ifr_addr;

		if (!inet_ntop(domain, &s_in->sin_addr, ip, sizeof(ip))) 
		{
			perror("inet_ntop");
			return 0;
		}
		if(!strcmp("wlan0", ifr[i].ifr_name))
		{
			return ip;
		}
	}

	close(s);
	return "";
}
