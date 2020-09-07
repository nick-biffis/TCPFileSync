#include <arpa/inet.h>   // for sockaddr_in and inet_ntoa()
#include <netdb.h>
#include <stdlib.h>      // for atoi() */
#include <stdio.h>       // for printf() and fprintf()
#include <string.h>      // for memset()
#include <sys/socket.h>  // for socket(), bind(), and connect() 
#include <sys/time.h>    // FD_SET, FD_ISSET, FD_ZERO macros
#include <unistd.h>      // for unix stuff

#define SERVER_HOST "mathcs01"  /* default host */
#define SERVER_PORT "30450"

#define SA struct sockaddr

#define	BUFFSIZE	8192	/* buffer size for reads and writes */