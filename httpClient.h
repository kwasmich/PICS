//
//  httpClient.h
//  PICS
//
//  Created by Michael Kwasnicki on 29.10.17.
//  Copyright © 2017 Michael Kwasnicki. All rights reserved.
//

#ifndef httpClient_h
#define httpClient_h

#include <stdbool.h>


typedef struct {
    int socket;
    bool keepAlive;
} httpClient_s;


void * httpClientThread(void *data);

#endif /* httpClient_h */
