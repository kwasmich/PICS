//
//  mmapHelper.c
//  PICS
//
//  Created by Michael Kwasnicki on 20.09.17.
//  Copyright © 2017 Michael Kwasnicki. All rights reserved.
//

#include "mmapHelper.h"

#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>



void initMapFile(MapFile_s * const out_map, const char * const in_PATH, const MapFileFlags in_FLAGS) {
    int fd = -1;

    if (in_FLAGS == MAP_RO) {
        fd = open(in_PATH, O_RDONLY);
    } else {
        fd = open(in_PATH, O_RDWR);
    }

    assert(fd >= 0);

    struct stat st;
    int ret = fstat(fd, &st);
    assert(ret == 0);

    off_t len = st.st_size;
    assert(len > 0);

    uint8_t *map;

    if (in_FLAGS == MAP_RO) {
        map = mmap(NULL, len, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0);
    } else {
        map = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fd, 0);
    }

    assert(map != MAP_FAILED);

    out_map->fd = fd;
    out_map->len = len;
    out_map->data = map;
}



void freeMapFile(MapFile_s * const in_out_map) {
    int ret = 0;

    ret = munmap(in_out_map->data, in_out_map->len);
    assert(ret == 0);

    ret = close(in_out_map->fd);
    assert(ret == 0);

    in_out_map->fd = -1;
    in_out_map->len = 0;
    in_out_map->data = NULL;
}
