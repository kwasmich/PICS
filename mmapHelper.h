//
//  mmapHelper.h
//  PICS
//
//  Created by Michael Kwasnicki on 20.09.17.
//  Copyright © 2017 Michael Kwasnicki. All rights reserved.
//

#ifndef mmapHelper_h
#define mmapHelper_h


#include <stdlib.h>


typedef struct MapFile_s {
    void *data;
    size_t len;
    int fd;
} MapFile_s;


typedef enum {
    MAP_RO = 0x01,
    MAP_WO = 0x02,
    MAP_RW = 0x03
} MapFileFlags;


void initMapFile(MapFile_s * const out_map, const char * const in_PATH, const MapFileFlags in_FLAGS);
void freeMapFile(MapFile_s * const in_out_map);


#endif /* mmapHelper_h */
