//
//  httpClient.c
//  PICS
//
//  Created by Michael Kwasnicki on 29.10.17.
//  Copyright © 2017 Michael Kwasnicki. All rights reserved.
//

#include "httpClient.h"

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include "uvcCamera.h"
#include "mmapHelper.h"



#define ISspace(x) isspace((int)(x))
#define SERVER_STRING "Server: PICS/0.0.1\r\n"



static int get_line(int socket, char *buf, int size) {
    int i = 0;
    char c = '\0';
    ssize_t n;

    while ((i < size - 1) && (c != '\n')) {
        n = recv(socket, &c, 1, 0);

        if (n > 0) {
            if (c == '\r') {
                n = recv(socket, &c, 1, MSG_PEEK);

                if ((n > 0) && (c == '\n')) {
                    recv(socket, &c, 1, 0);
                } else {
                    c = '\n';
                }
            }

            buf[i] = c;
            i++;
        } else {
            c = '\n';
        }
    }
    
    buf[i] = '\0';
    return i;
}



static void discardHeaders(int socket) {
    //    char buf[1024];
    //    int numChars;
    //
    //    do {
    //        numChars = get_line(socket, buf, sizeof buf);
    //    } while ((numChars > 0) && strncmp("\n", buf, 2));

    char c;
    int i = 0;
    ssize_t n;

    do {
        n = recv(socket, &c, 1, 0);

        switch (c) {
            case '\r':
            case '\n':
                i++;
                break;

            default:
                i = 0;
        }
    } while (n > 0 && i < 4);
}



static void headers(int client, const char *filePath) {
    char buf[1024];
    (void)filePath;  /* could use filename to determine file type */

    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}



static void not_found(int socket) {
    char buf[1024];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(socket, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(socket, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(socket, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(socket, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(socket, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(socket, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(socket, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(socket, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(socket, buf, strlen(buf), 0);

    shutdown(socket, SHUT_RDWR);
    close(socket);
}



static void unimplemented(int socket) {
    char buf[1024];

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(socket, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(socket, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(socket, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(socket, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(socket, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(socket, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(socket, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(socket, buf, strlen(buf), 0);

    shutdown(socket, SHUT_RDWR);
    close(socket);
}



static void serveFile(int socket, const char *filePath) {
    discardHeaders(socket);

    MapFile_s file;
    initMapFile(&file, filePath, MAP_RO);
    headers(socket, filePath);
    send(socket, file.data, file.len, 0);
    freeMapFile(&file);
}



static void stream(httpClient_s *client) {
    printf("connected client: %d\n", client->socket);
    uvcConnectClient(0);
    char header[] = ""
    "HTTP/1.0 200 OK\n"
    "Age: 0\n"
    "Cache-Control: no-cache, private\n"
    "Pragma: no-cache\n"
    "Content-Type: multipart/x-mixed-replace; boundary=FRAME\n\n";
    char separator[] = "--FRAME\n";
    char header2[] = ""
    "Content-Type: image/jpeg\n"
    "Content-Length: %d\n\n";
    char header2a[1024];

    char buf[1024];
    int numchars = 0;

    do { /* read & discard headers */
        numchars = get_line(client->socket, buf, sizeof(buf));
    } while ((numchars > 0) && strcmp("\n", buf));

    //    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    //    send(client, buf, strlen(buf), 0);
    send(client->socket, header, strlen(header), 0);
    ssize_t err = 0;
    uint8_t *imageData = NULL;
    size_t imageSize = 0;

    int cnt = 300;


    while (true && cnt) {
        uvcGetImage(0, &imageData, &imageSize);
        sprintf(header2a, header2, imageSize);
        err = send(client->socket, separator, strlen(separator), 0);
        if (err == -1) break;
        err = send(client->socket, header2a, strlen(header2a), 0);
        if (err == -1) break;
        err = write(client->socket, imageData, imageSize);
        if (err == -1) break;
        uvcGetImageDone(0);
        cnt--;
    }

    printf("disconnected client: %d\n", client->socket);

    if (err == -1) {
        uvcGetImageDone(0);
    }
    
    uvcDisconnectClient(0);
}



/**********************************************************************/
/* A request has caused a call to accept() on the server port to
 * return.  Process the request appropriately.
 * Parameters: the socket connected to the client */
/**********************************************************************/
void * httpClientThread(void *data) {
    httpClient_s *client = data;

    char buf[1024];
    int numChars;
    char *buffer;
    char *method;
    char *path;
    char *query;
    char *httpVersion;

//    int cameraIndex = -1;

    buffer = buf;
    numChars = get_line(client->socket, buf, sizeof(buf));
    method = strsep(&buffer, " ");
    path = strsep(&buffer, " ");
    httpVersion = strsep(&buffer, " ");
    query = path;
    path = strsep(&query, "?");

    if (strcasecmp(method, "GET")) {
        unimplemented(client->socket);
        return NULL;
    }

    if (strncasecmp(path, "/dev/video", strlen("/dev/video")) == 0) {
        stream(client);
    } else {
        char filePath[1024];
        sprintf(filePath, "htdocs%s", path);
        size_t filePathLen = strlen(filePath);

        if (filePath[filePathLen - 1] == '/') {
            strlcat(filePath, "index.html", sizeof filePath);
        }

        printf("%s\n", filePath);

        struct stat st;
        int err = stat(filePath, &st);

        if (err == -1) {
            not_found(client->socket);
            return NULL;
        } else {
            if ((st.st_mode & S_IFMT) == S_IFDIR) {
                strlcat(filePath, "/index.html", sizeof filePath);
            }

            serveFile(client->socket, filePath);
        }
    }

    shutdown(client->socket, SHUT_RDWR);
    close(client->socket);
    return NULL;
}
