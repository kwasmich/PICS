//
//  uvcCapture.h
//  PICS
//
//  Created by Michael Kwasnicki on 02.11.17.
//  Copyright © 2017 Michael Kwasnicki. All rights reserved.
//

#ifndef uvcCapture_h
#define uvcCapture_h

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <sys/time.h>

typedef struct {
    uint8_t *start;
    size_t length;
} buffer_s;


typedef struct {
    int fd;
    int io;
    int lastFrameIndex;
    uint32_t width;
    uint32_t height;
    uint32_t pixelFormat;
    ssize_t bufferCount;
    uint32_t bufferSize;
    buffer_s *buffers;
    buffer_s *head;
    struct v4l2_buffer buf;
} uvcCamera_s;


uvcCamera_s * uvcInit(const char *device, uint32_t width, uint32_t height, uint32_t pixelFormat);
void uvcDeinit(uvcCamera_s *camera);
void uvcStart(uvcCamera_s *camera);
void uvcStop(uvcCamera_s *camera);
bool uvcCaptureFrame(uvcCamera_s *camera, struct timeval timeout);

void uvcListPixelFormats(const char *device);


#endif /* uvcCapture_h */
