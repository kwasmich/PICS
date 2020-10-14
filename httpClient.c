//
//  httpClient.c
//  PICS
//
//  Created by Michael Kwasnicki on 29.10.17.
//  Copyright © 2017 Michael Kwasnicki. All rights reserved.
//

#include "httpClient.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include "uvcCamera.h"
#include "mmapHelper.h"



#define SERVER_STRING "Server: PICS/0.0.1\r\n"



static int getLine(int socket, char *buf, int size) {
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
    (void)filePath;  /* could use filename to determine file type */
    char buf[] = ("HTTP/1.1 200 OK\r\n"
                  SERVER_STRING
                  "Content-Type: text/html\r\n"
                  "\r\n");
    send(client, buf, strlen(buf), 0);
}



static void notFound(int socket) {
    char buf[] = ("HTTP/1.1 404 NOT FOUND\r\n"
                  SERVER_STRING
                  "Content-Type: text/html\r\n"
                  "\r\n"
                  "<HTML><HEAD><TITLE>Not Found</TITLE></HEAD>\r\n"
                  "<BODY><H1>404 Not Found</H1></BODY></HTML>\r\n");
    send(socket, buf, strlen(buf), 0);
}



static void unimplemented(int socket) {
    char buf[] = ("HTTP/1.1 501 Not Implemented\r\n"
                  SERVER_STRING
                  "Content-Type: text/html\r\n"
                  "\r\n"
                  "<HTML><HEAD><TITLE>Not Implemented</TITLE></HEAD>\r\n"
                  "<BODY><H1>501 Not Implemented</H1></BODY></HTML>\r\n");
    send(socket, buf, strlen(buf), 0);
}



static void serveFile(int socket, const char *filePath) {
    discardHeaders(socket);

    MapFile_s file;
    initMapFile(&file, filePath, MAP_RO);
    headers(socket, filePath);
    send(socket, file.data, file.len, 0);
    freeMapFile(&file);
}



static void stream(httpClient_s *client, int device) {
    printf("connected client: %d\n", client->socket);
    uvcConnectClient(device);
    char header[] = ("HTTP/1.1 200 OK\r\n"
                     "Age: 0\r\n"
                     "Cache-Control: no-cache, private\r\n"
                     "Pragma: no-cache\r\n"
                     "Content-Type: multipart/x-mixed-replace; boundary=FRAME\r\n"
                     "\r\n");
    char separator[] = "--FRAME\r\n";
    char header2[] = ("Content-Type: image/jpeg\r\n"
                      "Content-Length: %d\r\n"
                      "\r\n");
    char header2a[1024];

    discardHeaders(client->socket);

    //    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    //    send(client, buf, strlen(buf), 0);
    send(client->socket, header, strlen(header), 0);
    ssize_t err = 0;
    uint8_t *imageData = NULL;
    size_t imageSize = 0;

    int cnt = 1000;

    while (client->keepAlive && cnt) {
        uvcGetImage(device, &imageData, &imageSize);
        sprintf(header2a, header2, imageSize);
        err = send(client->socket, separator, strlen(separator), 0);
        if (err == -1) break;
        err = send(client->socket, header2a, strlen(header2a), 0);
        if (err == -1) break;
        err = write(client->socket, imageData, imageSize);
        if (err == -1) break;
        uvcGetImageDone(device);
        cnt--;
    }

    printf("disconnected client: %d\n", client->socket);

    if (err == -1) {
        uvcGetImageDone(device);
    }
    
    uvcDisconnectClient(device);
}



static void still(httpClient_s *client, int device) {
    const int skipFrames = 10;
    printf("connected client: %d\n", client->socket);
    uvcConnectClient(device);
    char header[] = ("HTTP/1.1 200 OK\r\n"
                     "Age: 0\r\n"
                     "Cache-Control: no-cache, private\r\n"
                     "Pragma: no-cache\r\n"
                     "Content-Type: image/jpeg\r\n"
                     "Content-Length: %d\r\n"
                     "\r\n");

    char header_a[1024];

    discardHeaders(client->socket);

    ssize_t err = 0;
    uint8_t *imageData = NULL;
    size_t imageSize = 0;

    for (int i = 0; i < skipFrames; i++) {
        uvcGetImage(device, &imageData, &imageSize);
        uvcGetImageDone(device);
    }

    uvcGetImage(device, &imageData, &imageSize);
    sprintf(header_a, header, imageSize);
    err = send(client->socket, header_a, strlen(header_a), 0);
    if (err == -1) goto error;
    err = write(client->socket, imageData, imageSize);
    if (err == -1) goto error;

error:
    uvcGetImageDone(device);
    printf("disconnected client: %d\n", client->socket);
    uvcDisconnectClient(device);
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

    numChars = getLine(client->socket, buf, sizeof buf);
    assert(numChars > 0);

    buffer = buf;
    method = strsep(&buffer, " ");
    path = strsep(&buffer, " ");
    httpVersion = strsep(&buffer, " ");
    query = path;
    path = strsep(&query, "?");

    if (strcasecmp(method, "GET")) {
        unimplemented(client->socket);
    } else if (strncasecmp(path, "/dev/video", strlen("/dev/video")) == 0) {
        int device = -1;
        int scanned = sscanf(path, "/dev/video%d", &device);

        if ((scanned == 1) && (device >= 0) && (device < 10) && uvcDoesCameraExist(device)) {
            stream(client, device);
        } else {
            notFound(client->socket);
        }
    } else if (strncasecmp(path, "/dev/still", strlen("/dev/still")) == 0) {
        int device = -1;
        int scanned = sscanf(path, "/dev/still%d", &device);

        if ((scanned == 1) && (device >= 0) && (device < 10) && uvcDoesCameraExist(device)) {
            still(client, device);
        } else {
            notFound(client->socket);
        }
    } else {
        char filePath[1024];
        sprintf(filePath, "htdocs%s", path);
        size_t filePathLen = strlen(filePath);

        if (filePath[filePathLen - 1] == '/') {
            strncat(filePath, "index.html", (sizeof filePath) - strlen(filePath) - 1);
        }

        struct stat st;
        int err = stat(filePath, &st);

        if (err == -1) {
            notFound(client->socket);
        } else {
            if ((st.st_mode & S_IFMT) == S_IFDIR) {
                strncat(filePath, "/index.html", (sizeof filePath) - strlen(filePath) - 1);
            }

            serveFile(client->socket, filePath);
        }
    }

    shutdown(client->socket, SHUT_RDWR);
    //usleep(5000);
    close(client->socket);
    free(data);
    return NULL;
}
