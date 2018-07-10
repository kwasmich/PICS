#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#ifndef NO_OMX
#   include <bcm_host.h>
#   define OMX_SKIP64BIT
#   include <IL/OMX_Core.h>
#
#   include "omxHelper.h"
#endif

#include "httpClient.h"
#include "uvcCamera.h"



void error_die(const char *);
int startup(u_short *);




/**********************************************************************/
/* Print out an error message with perror() (for system errors; based
 * on value of errno, which indicates system call errors) and exit the
 * program indicating an error. */
/**********************************************************************/
void error_die(const char *sc)
{
    perror(sc);
    exit(1);
}



/**********************************************************************/
/* This function starts the process of listening for web connections
 * on a specified port.  If the port is 0, then dynamically allocate a
 * port and modify the original port variable to reflect the actual
 * port.
 * Parameters: pointer to variable containing the port to connect on
 * Returns: the socket */
/**********************************************************************/
int startup(in_port_t *port)
{
    int httpd = 0;
    struct sockaddr_in name;

    httpd = socket(PF_INET, SOCK_STREAM, 0);
    if (httpd == -1)
        error_die("socket");
    memset(&name, 0, sizeof name);
    name.sin_family = AF_INET;
    name.sin_port = htons(*port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(httpd, (struct sockaddr *)&name, sizeof name) < 0)
        error_die("bind");
    if (*port == 0)  /* if dynamically allocating a port */
    {
        socklen_t namelen = sizeof name;
        if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)
            error_die("getsockname");
        *port = ntohs(name.sin_port);
    }
    if (listen(httpd, 5) < 0)
        error_die("listen");
    return(httpd);
}





int s_server_sock = -1;



static void destroy() {
    fputs("destroy\n", stderr);

    for (int i = 0; i < 10; i++) {
        uvcDeinitWorker(i);
    }

    close(s_server_sock);

#ifndef NO_OMX
    OMX_Deinit();
    bcm_host_deinit();
#endif
}



static void terminated(const int in_SIG) {
    fprintf(stderr, "\nTERMINATING due to signal %i\n", in_SIG);
    exit(1);
}



int main(void) {
    atexit(destroy);
    signal(SIGINT, terminated);
    signal(SIGPIPE, SIG_IGN); // ignore broken pipe

#ifndef NO_OMX
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;

    bcm_host_init();
    omxErr = OMX_Init();
    omxAssert(omxErr);
#endif

    in_port_t port = 10000;
    struct sockaddr_in client_name;
    socklen_t client_name_len = sizeof client_name;
    pthread_t newthread;
    
    s_server_sock = startup(&port);
    printf("httpd running on port %d\n", port);

    for (int i = 0; i < 10; i++) {
        uvcInitWorker(i);
    }

    while (true) {
        httpClient_s *client = malloc(sizeof (httpClient_s));
        client->socket = accept(s_server_sock, (struct sockaddr *)&client_name, &client_name_len);
        client->keepAlive = true;

        if (client->socket == -1) {
            printf("accept error");
            continue;
        }

        int result = pthread_create(&newthread , NULL, httpClientThread, client);

        if (result != 0) {
            perror("pthread_create");
        }
        
        result = pthread_detach(newthread);
        
        if (result != 0) {
            perror("pthread_detach");
        }
    }

    return 0;
}
