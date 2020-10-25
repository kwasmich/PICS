//
//  uvcCapture.c
//  PICS
//
//  Created by Michael Kwasnicki on 02.11.17.
//  Copyright © 2017 Michael Kwasnicki. All rights reserved.
//

#include "uvcCapture.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/mman.h>


#include "cHelper.h"


#define uvcAssert(cond, message) \
if (!(cond)) { \
fprintf(stderr, "[%s:%d (%s)] %s - last error: %d - %s\n", __FILE__, __LINE__, __func__, "" message, errno, strerror(errno)); \
exit(EXIT_FAILURE); \
}

#define NUM_BUFFERS 4



enum v4l2_memory_extension {
    V4L2_MEMORY_READ             = 0
};



static int xioctl(int fd, int request, void *arg) {
    int r;

    do {
        r = ioctl(fd, request, arg);
    } while (r == -1 && errno == EINTR);

    return r;
}



void uvcListPixelFormats(const char *device) {
    int fd = open(device, O_RDWR | O_NONBLOCK, 0);
    uvcAssert(fd != -1, "open");

    struct v4l2_capability cap;
    int result = xioctl(fd, VIDIOC_QUERYCAP, &cap);
    uvcAssert(result != -1, "VIDIOC_QUERYCAP");
    uvcAssert(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE, "no capture");

    if (!(cap.device_caps & V4L2_CAP_VIDEO_CAPTURE)) {
        goto exit;
    }

    uvcAssert(cap.device_caps & V4L2_CAP_VIDEO_CAPTURE, "no capture");

    static const uint32_t pix_formats[] = {
        V4L2_PIX_FMT_ABGR32,
        V4L2_PIX_FMT_ARGB32,
        V4L2_PIX_FMT_ARGB444,
        V4L2_PIX_FMT_ARGB555,
        //V4L2_PIX_FMT_ARGB555X,
        V4L2_PIX_FMT_BGR24,
        V4L2_PIX_FMT_BGR32,
        V4L2_PIX_FMT_BGR666,
        V4L2_PIX_FMT_CIT_YYVYUY,
        V4L2_PIX_FMT_CPIA1,
        V4L2_PIX_FMT_DV,
        V4L2_PIX_FMT_ET61X251,
        V4L2_PIX_FMT_GREY,
        V4L2_PIX_FMT_H263,
        V4L2_PIX_FMT_H264,
        V4L2_PIX_FMT_H264_MVC,
        V4L2_PIX_FMT_H264_NO_SC,
        V4L2_PIX_FMT_HI240,
        V4L2_PIX_FMT_HM12,
        V4L2_PIX_FMT_JL2005BCD,
        V4L2_PIX_FMT_JPEG,
        V4L2_PIX_FMT_JPGL,
        V4L2_PIX_FMT_KONICA420,
        V4L2_PIX_FMT_M420,
        V4L2_PIX_FMT_MJPEG,
        V4L2_PIX_FMT_MPEG,
        V4L2_PIX_FMT_MPEG1,
        V4L2_PIX_FMT_MPEG2,
        V4L2_PIX_FMT_MPEG4,
        V4L2_PIX_FMT_MR97310A,
        V4L2_PIX_FMT_NV12,
        V4L2_PIX_FMT_NV12M,
        V4L2_PIX_FMT_NV12MT,
        V4L2_PIX_FMT_NV12MT_16X16,
        V4L2_PIX_FMT_NV16,
        V4L2_PIX_FMT_NV16M,
        V4L2_PIX_FMT_NV21,
        V4L2_PIX_FMT_NV21M,
        V4L2_PIX_FMT_NV24,
        V4L2_PIX_FMT_NV42,
        V4L2_PIX_FMT_NV61,
        V4L2_PIX_FMT_NV61M,
        V4L2_PIX_FMT_OV511,
        V4L2_PIX_FMT_OV518,
        V4L2_PIX_FMT_PAC207,
        V4L2_PIX_FMT_PAL8,
        V4L2_PIX_FMT_PJPG,
        //V4L2_PIX_FMT_PRIV_MAGIC,
        V4L2_PIX_FMT_PWC1,
        V4L2_PIX_FMT_PWC2,
        V4L2_PIX_FMT_RGB24,
        V4L2_PIX_FMT_RGB32,
        V4L2_PIX_FMT_RGB332,
        V4L2_PIX_FMT_RGB444,
        V4L2_PIX_FMT_RGB555,
        V4L2_PIX_FMT_RGB555X,
        V4L2_PIX_FMT_RGB565,
        V4L2_PIX_FMT_RGB565X,
        V4L2_PIX_FMT_S5C_UYVY_JPG,
        V4L2_PIX_FMT_SBGGR8,
        V4L2_PIX_FMT_SBGGR10,
        V4L2_PIX_FMT_SBGGR10ALAW8,
        V4L2_PIX_FMT_SBGGR10DPCM8,
        V4L2_PIX_FMT_SBGGR12,
        V4L2_PIX_FMT_SBGGR16,
        V4L2_PIX_FMT_SE401,
        V4L2_PIX_FMT_SGBRG8,
        V4L2_PIX_FMT_SGBRG10,
        V4L2_PIX_FMT_SGBRG10ALAW8,
        V4L2_PIX_FMT_SGBRG10DPCM8,
        V4L2_PIX_FMT_SGBRG12,
        V4L2_PIX_FMT_SGRBG8,
        V4L2_PIX_FMT_SGRBG10,
        V4L2_PIX_FMT_SGRBG10ALAW8,
        V4L2_PIX_FMT_SGRBG10DPCM8,
        V4L2_PIX_FMT_SGRBG12,
        V4L2_PIX_FMT_SN9C10X,
        V4L2_PIX_FMT_SN9C20X_I420,
        V4L2_PIX_FMT_SN9C2028,
        V4L2_PIX_FMT_SPCA501,
        V4L2_PIX_FMT_SPCA505,
        V4L2_PIX_FMT_SPCA508,
        V4L2_PIX_FMT_SPCA561,
        V4L2_PIX_FMT_SQ905C,
        V4L2_PIX_FMT_SRGGB8,
        V4L2_PIX_FMT_SRGGB10,
        V4L2_PIX_FMT_SRGGB10ALAW8,
        V4L2_PIX_FMT_SRGGB10DPCM8,
        V4L2_PIX_FMT_SRGGB12,
        V4L2_PIX_FMT_STV0680,
        V4L2_PIX_FMT_TM6000,
        V4L2_PIX_FMT_UV8,
        V4L2_PIX_FMT_UYVY,
        V4L2_PIX_FMT_VC1_ANNEX_G,
        V4L2_PIX_FMT_VC1_ANNEX_L,
        V4L2_PIX_FMT_VP8,
        V4L2_PIX_FMT_VYUY,
        V4L2_PIX_FMT_WNVA,
        V4L2_PIX_FMT_XBGR32,
        V4L2_PIX_FMT_XRGB32,
        V4L2_PIX_FMT_XRGB444,
        V4L2_PIX_FMT_XRGB555,
        //V4L2_PIX_FMT_XRGB555X,
        V4L2_PIX_FMT_XVID,
        V4L2_PIX_FMT_Y4,
        V4L2_PIX_FMT_Y6,
        V4L2_PIX_FMT_Y10,
        V4L2_PIX_FMT_Y10BPACK,
        V4L2_PIX_FMT_Y12,
        V4L2_PIX_FMT_Y16,
        V4L2_PIX_FMT_Y41P,
        V4L2_PIX_FMT_YUV32,
        V4L2_PIX_FMT_YUV410,
        V4L2_PIX_FMT_YUV411P,
        V4L2_PIX_FMT_YUV420,
        V4L2_PIX_FMT_YUV420M,
        V4L2_PIX_FMT_YUV422P,
        V4L2_PIX_FMT_YUV444,
        V4L2_PIX_FMT_YUV555,
        V4L2_PIX_FMT_YUV565,
        V4L2_PIX_FMT_YUYV,
        V4L2_PIX_FMT_YVU410,
        V4L2_PIX_FMT_YVU420,
        V4L2_PIX_FMT_YVU420M,
        V4L2_PIX_FMT_YVYU,
        V4L2_PIX_FMT_YYUV
    };

    const int count = sizeof pix_formats / sizeof pix_formats[0];
    struct v4l2_format format;

    for (int i = 0; i < count; i++) {
        memset(&format, 0, sizeof format);
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        result = xioctl(fd, VIDIOC_G_FMT, &format);
        uvcAssert(result != -1, "VIDIOC_G_FMT");
        format.fmt.pix.pixelformat = pix_formats[i];

        typedef union {
            uint32_t u32;
            uint8_t u8[4];
        } byte_t;

        byte_t x;
        x.u32 = pix_formats[i];

        printf("%c%c%c%c : ", x.u8[0], x.u8[1], x.u8[2], x.u8[3]);
        result = xioctl(fd, VIDIOC_S_FMT, &format);
        uvcAssert(result != -1, "VIDIOC_S_FMT");
        result = xioctl(fd, VIDIOC_G_FMT, &format);
        uvcAssert(result != -1, "VIDIOC_G_FMT");

        if (format.fmt.pix.pixelformat == pix_formats[i]) {
            printf("%03d OK\n", i);
        } else {
            printf("%03d -\n", i);
        }
    }

exit:
    result = close(fd);
    uvcAssert(result != -1, "close");
}



uvcCamera_s * uvcInit(const char *device, uint32_t width, uint32_t height, uint32_t pixelFormat) {
//    pthread_t thread = pthread_self();
//    printf(COLOR_CYAN "uvcInit: %08lx\n" COLOR_NC, thread);

    int fd = open(device, O_RDWR | O_NONBLOCK, 0);
    uvcAssert(fd != -1, "open");
    uvcCamera_s *camera = malloc(sizeof(uvcCamera_s));
    camera->fd = fd;
    camera->lastFrameIndex = -1;
    camera->width = 0;
    camera->height = 0;
    camera->bufferCount = 0;
    camera->buffers = NULL;
    camera->head = NULL;

    struct v4l2_capability cap;
    int result = xioctl(camera->fd, VIDIOC_QUERYCAP, &cap);
    uvcAssert(result != -1, "VIDIOC_QUERYCAP");

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
//        printf(COLOR_RED "%s: no capture\n" COLOR_NC, device);
        goto error;
    }

    uvcAssert(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE, "no capture");
    uvcAssert(cap.capabilities & V4L2_CAP_STREAMING, "no streaming"); // required for mmap

    if (!(cap.device_caps & V4L2_CAP_VIDEO_CAPTURE)) {
//        printf(COLOR_RED "%s: no device capture\n" COLOR_NC, device);
        goto error;
    }

    uvcAssert(cap.device_caps & V4L2_CAP_VIDEO_CAPTURE, "no capture");

    // reset cropping and scaling
    struct v4l2_cropcap cropcap;
    memset(&cropcap, 0, sizeof cropcap);
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    result = xioctl(camera->fd, VIDIOC_CROPCAP, &cropcap);

    if (result == 0) {
        struct v4l2_crop crop;
        memset(&crop, 0, sizeof crop);
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect;
        result = xioctl(camera->fd, VIDIOC_S_CROP, &crop);
        //uvcAssert(result != -1 || errno == EINVAL, "VIDIOC_S_CROP"); // fire and forget as some do not support this
    }


    // attempt to set frame size and pixelformat
    struct v4l2_format format;
    memset(&format, 0, sizeof format);
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.pixelformat = pixelFormat;
    format.fmt.pix.field = V4L2_FIELD_NONE;
    result = xioctl(camera->fd, VIDIOC_S_FMT, &format);
    uvcAssert(result != -1, "VIDIOC_S_FMT");
    result = xioctl(camera->fd, VIDIOC_G_FMT, &format);
    uvcAssert(result != -1, "VIDIOC_G_FMT");
    camera->width = format.fmt.pix.width;
    camera->height = format.fmt.pix.height;
    camera->bufferSize = format.fmt.pix.sizeimage;
    camera->pixelFormat = format.fmt.pix.pixelformat;


    // attempt to set FPS
    struct v4l2_streamparm streamparam;
    memset(&streamparam, 0, sizeof streamparam);
    streamparam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    result = xioctl(camera->fd, VIDIOC_G_PARM, &streamparam);

    if (result == -1) {
//        printf(COLOR_RED "%s: no VIDIOC_G_PARM\n" COLOR_NC, device);
        goto error;
    }

    uvcAssert(result != -1, "VIDIOC_G_PARM");

    if (streamparam.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) {
        streamparam.parm.capture.timeperframe.numerator = 1;
        streamparam.parm.capture.timeperframe.denominator = 5;
        printf("FPS: %d\n", streamparam.parm.capture.timeperframe.denominator);
        result = xioctl(camera->fd, VIDIOC_S_PARM, &streamparam);
        uvcAssert(result != -1, "VIDIOC_S_PARM");
        result = xioctl(camera->fd, VIDIOC_G_PARM, &streamparam);
        uvcAssert(result != -1, "VIDIOC_G_PARM");
    }

    printf("FPS: %d\n", streamparam.parm.capture.timeperframe.denominator);


//    // allocate buffers
//    struct v4l2_requestbuffers req;
//    memset(&req, 0, sizeof req);
//    req.count = NUM_BUFFERS;
//    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//    req.memory = V4L2_MEMORY_USERPTR;
//    result = xioctl(camera->fd, VIDIOC_REQBUFS, &req);
//
//    if (result == 0) {
//        camera->io = V4L2_MEMORY_USERPTR;
//        camera->bufferCount = req.count;
//        camera->buffers = calloc(req.count, sizeof(buffer_s));
//        uvcAssert(camera->buffers, "allocating buffers");
//
//        printf("%dx%d (%d x %d bytes)\n", camera->width, camera->height, camera->bufferCount, camera->bufferSize);
//
//        for (int i = 0; i < camera->bufferCount; i++) {
//            camera->buffers[i].length = camera->bufferSize;
//            camera->buffers[i].start = malloc(camera->bufferSize);
//            uvcAssert(camera->buffers[i].start, "allocating buffers");
//        }
//
//        puts("Using USERPTR");
//        return camera;
//    }


    // attempt to use mmap instead of user_ptr
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof req);
    req.count = NUM_BUFFERS;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    result = xioctl(camera->fd, VIDIOC_REQBUFS, &req);

    if (result == 0) {
        uvcAssert(req.count >= 2, "insufficient memory on device");
        camera->io = V4L2_MEMORY_MMAP;
        camera->bufferCount = req.count;
        camera->buffers = calloc(req.count, sizeof(buffer_s));
        camera->head = calloc(1, sizeof(buffer_s));
        camera->head->length = camera->bufferSize;
        camera->head->start = malloc(camera->bufferSize);
        uvcAssert(camera->buffers, "allocating buffers");

        printf("%dx%d (%d x %d bytes)\n", camera->width, camera->height, camera->bufferCount, camera->bufferSize);

        for (int i = 0; i < req.count; i++) {
            struct v4l2_buffer buf;
            memset(&buf, 0, sizeof buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            result = xioctl(fd, VIDIOC_QUERYBUF, &buf);
            uvcAssert(result != -1, "VIDIOC_QUERYBUF");
            camera->buffers[i].length = buf.length;
            camera->buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
            uvcAssert(camera->buffers[i].start != MAP_FAILED, "mmap");
        }

        puts("Using MMAP");
        return camera;
    }

    uvcAssert(result != -1, "VIDIOC_REQBUFS");
    return NULL;

error:
    free(camera);
    int err = close(fd);
    uvcAssert(err != -1, "close");
    return NULL;
}



void uvcDeinit(uvcCamera_s *camera) {
//    pthread_t thread = pthread_self();
//    printf(COLOR_CYAN "uvcDeinit: %08lx\n" COLOR_NC, thread);

    switch (camera->io) {
        case V4L2_MEMORY_MMAP:
            for (int i = 0; i < camera->bufferCount; i++) {
                int result = munmap(camera->buffers[i].start, camera->buffers[i].length);
                uvcAssert(result != -1, "munmap");
                camera->buffers[i].start = NULL;
                camera->buffers[i].length = 0;
            }

            break;

        case V4L2_MEMORY_READ:
        case V4L2_MEMORY_USERPTR:
            for (int i = 0; i < camera->bufferCount; i++) {
                free(camera->buffers[i].start);
                camera->buffers[i].start = NULL;
                camera->buffers[i].length = 0;
            }

            break;
    }

    free(camera->buffers);
    camera->bufferCount = 0;
    camera->buffers = NULL;
    free(camera->head->start);
    free(camera->head);
    camera->head = NULL;

    int err = close(camera->fd);
    uvcAssert(err != -1, "close");
    free(camera);
}



void uvcStart(uvcCamera_s *camera) {
//    pthread_t thread = pthread_self();
//    printf(COLOR_CYAN "uvcStart: %08lx\n" COLOR_NC, thread);

    int result;
    enum v4l2_buf_type type;

    switch (camera->io) {
        case V4L2_MEMORY_READ:
            // do nothing
            break;

        case V4L2_MEMORY_MMAP:
        case V4L2_MEMORY_USERPTR:
            for (int i = 0; i < camera->bufferCount; i++) {
                struct v4l2_buffer buf;
                memset(&buf, 0, sizeof buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = camera->io;
                buf.index = i;

                if (camera->io == V4L2_MEMORY_USERPTR) {
                    buf.m.userptr = (unsigned long)camera->buffers[i].start;
                    buf.length = camera->buffers[i].length;
                }

                result = xioctl(camera->fd, VIDIOC_QBUF, &buf);
                uvcAssert(result != -1, "VIDIOC_QBUF");
            }

            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            result = xioctl(camera->fd, VIDIOC_STREAMON, &type);
            uvcAssert(result != -1, "VIDIOC_STREAMON");
            break;
    }
}



void uvcStop(uvcCamera_s *camera) {
//    pthread_t thread = pthread_self();
//    printf(COLOR_CYAN "uvcStop: %08lx\n" COLOR_NC, thread);

    int result;
    enum v4l2_buf_type type;

    switch (camera->io) {
        case V4L2_MEMORY_READ:
            // do nothing
            break;

        case V4L2_MEMORY_MMAP:
        case V4L2_MEMORY_USERPTR:
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            result = xioctl(camera->fd, VIDIOC_STREAMOFF, &type);
            uvcAssert(result != -1, "VIDIOC_STREAMOFF");
            break;
    }
}



static bool captureFrame(uvcCamera_s *camera) {
//    pthread_t thread = pthread_self();
//    printf(COLOR_CYAN "captureFrame: %08lx\n" COLOR_NC, thread);
    int result;

    switch (camera->io) {
        case V4L2_MEMORY_READ:
            uvcAssert(false, "not implemented");
            break;

        case V4L2_MEMORY_MMAP:
        case V4L2_MEMORY_USERPTR:
            memset(&camera->buf, 0, sizeof camera->buf);
            camera->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            camera->buf.memory = camera->io;
            result = xioctl(camera->fd, VIDIOC_DQBUF, &camera->buf);

            if (result == -1) {
                if (errno == EAGAIN) {
                    return false;
                }

                uvcAssert(result != -1, "VIDIOC_DQBUF");
            }

//            if ((camera->buf.index - camera->lastFrameIndex + NUM_BUFFERS) % NUM_BUFFERS != 1) {
//                puts(COLOR_RED "Frames out of sequence!" COLOR_NC);
//            }

            camera->lastFrameIndex = camera->buf.index;

//            printf("%d (%d bytes)\n", camera->buf.index, camera->buf.bytesused);
//            printf("%08lx %p\n", camera->buf.m.userptr, camera->buffers[camera->buf.index].start);
//            camera->head = &camera->buffers[(camera->buf.index + NUM_BUFFERS - 1) % NUM_BUFFERS];
            memcpy(camera->head->start, camera->buffers[camera->buf.index].start, camera->head->length);
            result = xioctl(camera->fd, VIDIOC_QBUF, &camera->buf);
            uvcAssert(result != -1, "VIDIOC_QBUF");
            break;
    }

//    printf(COLOR_CYAN "captureFrame: %08lx DONE\n" COLOR_NC, thread);
    return true;
}



bool uvcCaptureFrame(uvcCamera_s *camera, struct timeval timeout) {
//    pthread_t thread = pthread_self();
//    printf(COLOR_CYAN "uvcCaptureFrame: %08lx\n" COLOR_NC, thread);

    fd_set fds_r;
    FD_ZERO(&fds_r);
    FD_SET(camera->fd, &fds_r);
    fd_set fds_w;
    FD_ZERO(&fds_w);
    FD_SET(camera->fd, &fds_w);
    fd_set fds_e;
    FD_ZERO(&fds_e);
    FD_SET(camera->fd, &fds_e);
    int r = select(camera->fd + 1, &fds_r, &fds_w, &fds_e, &timeout);
    uvcAssert(r != -1, "select");

//    printf(COLOR_CYAN "uvcCaptureFrame: %08lx SELECT DONE\n" COLOR_NC, thread);
    
    if (r == 0) {
//        printf(COLOR_CYAN "uvcCaptureFrame: %08lx FAILED\n" COLOR_NC, thread);
        return 0;
    } else {
        bool x = captureFrame(camera);
//        printf(COLOR_CYAN "uvcCaptureFrame: %08lx DONE\n" COLOR_NC, thread);
        return x;
    }
}
