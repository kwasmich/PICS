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
#include <strings.h>
#include <unistd.h>
#include <sys/socket.h>

#include "uvcCamera.h"



#define ISspace(x) isspace((int)(x))
#define SERVER_STRING "Server: kwasihttpd/0.0.1\r\n"



static int get_line(int sock, char *buf, int size);
static void headers(int client, const char *filename);
static void not_found(int client);
static void unimplemented(int client);
static void serve_file(int client, const char *filename);
static void cat(int client, FILE *resource);
static void stream(httpClient_s *client);



/**********************************************************************/
/* A request has caused a call to accept() on the server port to
 * return.  Process the request appropriately.
 * Parameters: the socket connected to the client */
/**********************************************************************/
void * httpClientThread(void *data)
{
    httpClient_s *client = data;

    char buf[1024];
    int numchars;
    char method[255];
    char url[255];
    char path[512];
    size_t i, j;
    //struct stat st;
    int cgi = 0;      /* becomes true if server decides this is a CGI
                       * program */
    char *query_string = NULL;

    numchars = get_line(client->socket, buf, sizeof(buf));
    i = 0; j = 0;
    while (!ISspace(buf[j]) && (i < sizeof(method) - 1))
    {
        method[i] = buf[j];
        i++; j++;
    }
    method[i] = '\0';

    if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
    {
        unimplemented(client->socket);
        return NULL;
    }

    if (strcasecmp(method, "POST") == 0)
        cgi = 1;

    i = 0;
    while (ISspace(buf[j]) && (j < sizeof(buf)))
        j++;
    while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
    {
        url[i] = buf[j];
        i++; j++;
    }
    url[i] = '\0';

    if (strcasecmp(method, "GET") == 0)
    {
        query_string = url;
        while ((*query_string != '?') && (*query_string != '\0'))
            query_string++;
        if (*query_string == '?')
        {
            cgi = 1;
            *query_string = '\0';
            query_string++;
        }
    }

    sprintf(path, "htdocs%s", url);
    if (path[strlen(path) - 1] == '/')
        strcat(path, "index.html");
    //    if (stat(path, &st) == -1) {
    //        while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
    //            numchars = get_line(client, buf, sizeof(buf));
    //        not_found(client);
    //    }
    //    else
    {
        //        if ((st.st_mode & S_IFMT) == S_IFDIR)
        //            strcat(path, "/index.html");
        //        if ((st.st_mode & S_IXUSR) ||
        //            (st.st_mode & S_IXGRP) ||
        //            (st.st_mode & S_IXOTH)    )
        //            cgi = 1;
        if (!cgi)
            serve_file(client->socket, path);
        else
            //not_found(client->socket);
            stream(client);
        //execute_cgi(client, path, method, query_string);
    }


    // discard everything
//    char c;
//    int i = 0;
//    ssize_t n;
//
//    do {
//        putchar('.');
//        fflush(stdout);
//        n = recv(client->socket, &c, 1, 0);
//
//        switch (c) {
//            case '\r':
//            case '\n':
//                i++;
//                break;
//
//            default:
//                i = 0;
//        }
//
//        putchar('*');
//        fflush(stdout);
//        putchar(c);
//        fflush(stdout);
//    } while (n > 0 && i < 4);
//
//    not_found(client->socket);
    shutdown(client->socket, SHUT_RDWR);
    close(client->socket);
    return NULL;
}



/**********************************************************************/
/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */
/**********************************************************************/
static int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    ssize_t n;

    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);
        /* DEBUG printf("%02X\n", c); */
        if (n > 0)
        {
            if (c == '\r')
            {
                n = recv(sock, &c, 1, MSG_PEEK);
                /* DEBUG printf("%02X\n", c); */
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        }
        else
            c = '\n';
    }
    buf[i] = '\0';

    return(i);
}



/**********************************************************************/
/* Return the informational HTTP headers about a file. */
/* Parameters: the socket to print the headers on
 *             the name of the file */
/**********************************************************************/
static void headers(int client, const char *filename)
{
    char buf[1024];
    (void)filename;  /* could use filename to determine file type */

    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}



/**********************************************************************/
/* Give a client a 404 not found status message. */
/**********************************************************************/
static void not_found(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}



/**********************************************************************/
/* Inform the client that the requested web method has not been
 * implemented.
 * Parameter: the client socket */
/**********************************************************************/
static void unimplemented(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}



/**********************************************************************/
/* Send a regular file to the client.  Use headers, and report
 * errors to client if they occur.
 * Parameters: a pointer to a file structure produced from the socket
 *              file descriptor
 *             the name of the file to serve */
/**********************************************************************/
static void serve_file(int client, const char *filename)
{
    FILE *resource = NULL;
    int numchars = 1;
    char buf[1024];

    buf[0] = 'A'; buf[1] = '\0';
    while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
        numchars = get_line(client, buf, sizeof(buf));

    resource = fopen(filename, "r");
    if (resource == NULL)
        not_found(client);
    else
    {
        headers(client, filename);
        cat(client, resource);
    }
    fclose(resource);
}



/**********************************************************************/
/* Put the entire contents of a file out on a socket.  This function
 * is named after the UNIX "cat" command, because it might have been
 * easier just to do something like pipe, fork, and exec("cat").
 * Parameters: the client socket descriptor
 *             FILE pointer for the file to cat */
/**********************************************************************/
static void cat(int client, FILE *resource)
{
    char buf[1024];

    fgets(buf, sizeof(buf), resource);
    while (!feof(resource))
    {
        send(client, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), resource);
    }
}



static void stream(httpClient_s *client) {
    printf("connected client: %d\n", client->socket);
    uvcConnectClient();
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

    uvcDisconnectClient();
}
