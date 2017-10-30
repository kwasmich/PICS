//
//  uvcCamera.h
//  PICS
//
//  Created by Michael Kwasnicki on 29.10.17.
//  Copyright © 2017 Michael Kwasnicki. All rights reserved.
//

#ifndef uvcCamera_h
#define uvcCamera_h


#include <stddef.h>
#include <stdint.h>


void uvcInit(void);
void uvcDeinit(void);

void uvcConnectClient();
void uvcDisconnectClient();

void uvcGetImage(int videoIndex, uint8_t **data, size_t *len);
void uvcGetImageDone(int videoIndex);


#endif /* uvcCamera_h */
