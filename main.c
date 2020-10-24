#include <arpa/inet.h>
#include <assert.h>
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
#include "cHelper.h"



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
    int err = 0;
    int httpd = 0;
    struct sockaddr_in name;

    httpd = socket(PF_INET, SOCK_STREAM, 0);
    if (httpd == -1)
        error_die("socket");
    memset(&name, 0, sizeof name);
    name.sin_family = AF_INET;
    name.sin_port = htons(*port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);

    int val = 1;
    err = setsockopt(httpd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof val);
    perror("setsockopt");
    assert(err == 0);

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
static volatile bool s_keep_alive = true;



static void destroy() {
    int err = 0;
    fputs("destroy\n", stderr);

    for (int i = 0; i < 10; i++) {
        uvcDeinitWorker(i);
    }

    if (s_server_sock != -1) {
        err = shutdown(s_server_sock, SHUT_RDWR);
        assert(err == 0);
        err = close(s_server_sock);
        assert(err == 0);
    }

#ifndef NO_OMX
    OMX_Deinit();
    bcm_host_deinit();
#endif
}



static void terminated(const int in_SIG) {
    fprintf(stderr, "\nTERMINATING due to signal %i\n", in_SIG);
    s_keep_alive = false;
    exit(1);
}


static void help() {
    puts("    -p port             port to listen ");
}


int main(int argc, char **argv) {
    in_port_t port = 8080;
    int device = 0;
    int opt;

    while ((opt = getopt(argc, argv, "?p:d:")) > 0) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);

                if (port < 1024) {
                    fprintf(stderr, "Invalid port number %d\n", port);
                    help();
                    exit(EXIT_FAILURE);
                }

                break;

            case 'd':
                device = atoi(optarg);
                break;

            case '?':
            default:
                help();
                exit(EXIT_FAILURE);
        }
    }

    atexit(destroy);
    signal(SIGINT, terminated);
    signal(SIGPIPE, SIG_IGN); // ignore broken pipe

#ifndef NO_OMX
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;

    bcm_host_init();
    omxErr = OMX_Init();
    omxAssert(omxErr);
#endif

    struct sockaddr_in client_name;
    socklen_t client_name_len = sizeof client_name;
    
    s_server_sock = startup(&port);
    printf("httpd running on port %d\n", port);

    for (int i = 0; i < 10; i++) {
        uvcInitWorker(i);
    }

//    uvcInitWorker(device);

    while (s_keep_alive) {
        pthread_t newThread;
        httpClient_s *client = malloc(sizeof (httpClient_s));
        client->socket = accept(s_server_sock, (struct sockaddr *)&client_name, &client_name_len);
        client->keepAlive = true;

        if (client->socket == -1) {
            printf("accept error");
            continue;
        }

        int result = pthread_create(&newThread , NULL, httpClientThread, client);

        if (result != 0) {
            perror("pthread_create");
        }

//        printf(COLOR_YELLOW "httpClientThread: %08lx\n" COLOR_NC, newThread);
        
        result = pthread_detach(newThread);
        
        if (result != 0) {
            perror("pthread_detach");
        }
    }

    return 0;
}
