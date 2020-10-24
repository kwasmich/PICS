//
//  uvcCamera.c
//  PICS
//
//  Created by Michael Kwasnicki on 29.10.17.
//  Copyright © 2017 Michael Kwasnicki. All rights reserved.
//

#include "uvcCamera.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#include <linux/videodev2.h>

#include "uvcCapture.h"
#include "cHelper.h"

#ifndef NO_OMX
#   include "omxJPEGEnc.h"
#   include "omxHelper.h"
#endif



typedef struct {
    pthread_cond_t frameReadyCond;
    pthread_cond_t frameConsumingDoneCond;
    pthread_cond_t clientsWaitingCond;
    pthread_cond_t clientConnectedCond;
    pthread_mutex_t frameMutex;
    pthread_t thread;

    uvcCamera_s *camera;

#ifndef NO_OMX
    OMXContext_s *omx;
#endif

    uint8_t *imageData;
    size_t imageSize;
    size_t imageFill;
    int numClients;
    int numWaitingClients;
    int numProcessingClients;
    bool keepAlive;
    bool isActive;
} uvcCameraWorker_s;


static uvcCameraWorker_s s_video[20];

static struct timeval s_timeout = { .tv_sec = 2, .tv_usec = 0 };

#ifndef NO_OMX
static pthread_mutex_t s_omxMutex = PTHREAD_MUTEX_INITIALIZER;
#endif


// https://gist.github.com/bellbind/6813905
#include <jpeglib.h>
#include <stdlib.h>

static void jpeg(uint8_t **out_buffer, size_t *out_bufferSize, uint8_t* rgb, uint32_t width, uint32_t height, int quality) {
    JSAMPARRAY image;
    image = calloc(height, sizeof (JSAMPROW));

    for (size_t i = 0; i < height; i++) {
        image[i] = calloc(width * 3, sizeof (JSAMPLE));

        for (size_t j = 0; j < width; j++) {
            image[i][j * 3 + 0] = rgb[(i * width + j) * 3 + 0];
            image[i][j * 3 + 1] = rgb[(i * width + j) * 3 + 1];
            image[i][j * 3 + 2] = rgb[(i * width + j) * 3 + 2];
        }
    }

    if (*out_buffer) {
        free(*out_buffer);
        *out_buffer = NULL;
    }

    struct jpeg_compress_struct compress;
    struct jpeg_error_mgr error;
    compress.err = jpeg_std_error(&error);
    jpeg_create_compress(&compress);
    jpeg_mem_dest(&compress, out_buffer, out_bufferSize);

    compress.image_width = width;
    compress.image_height = height;
    compress.input_components = 3;
    compress.in_color_space = JCS_RGB;
    jpeg_set_defaults(&compress);
    jpeg_set_quality(&compress, quality, TRUE);
    jpeg_start_compress(&compress, TRUE);
    jpeg_write_scanlines(&compress, image, height);
    jpeg_finish_compress(&compress);
    jpeg_destroy_compress(&compress);

    for (size_t i = 0; i < height; i++) {
        free(image[i]);
    }

    free(image);
}

static int minmax(int min, int v, int max) {
    return (v < min) ? min : (max < v) ? max : v;
}

static uint8_t* yuyv2rgb(uint8_t* yuyv, uint32_t width, uint32_t height) {
    uint8_t* rgb = calloc(width * height * 3, sizeof (uint8_t));
    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j < width; j += 2) {
            size_t index = i * width + j;
            int y0 = yuyv[index * 2 + 0] << 8;
            int u = yuyv[index * 2 + 1] - 128;
            int y1 = yuyv[index * 2 + 2] << 8;
            int v = yuyv[index * 2 + 3] - 128;
            rgb[index * 3 + 0] = minmax(0, (y0 + 359 * v) >> 8, 255);
            rgb[index * 3 + 1] = minmax(0, (y0 + 88 * v - 183 * u) >> 8, 255);
            rgb[index * 3 + 2] = minmax(0, (y0 + 454 * u) >> 8, 255);
            rgb[index * 3 + 3] = minmax(0, (y1 + 359 * v) >> 8, 255);
            rgb[index * 3 + 4] = minmax(0, (y1 + 88 * v - 183 * u) >> 8, 255);
            rgb[index * 3 + 5] = minmax(0, (y1 + 454 * u) >> 8, 255);
        }
    }
    return rgb;
}

static uint8_t* uyvy2rgb(uint8_t* uyvy, uint32_t width, uint32_t height) {
    uint8_t* rgb = calloc(width * height * 3, sizeof (uint8_t));
    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j < width; j += 2) {
            size_t index = i * width + j;
            int u = uyvy[index * 2 + 0] - 128;
            int y0 = uyvy[index * 2 + 1] << 8;
            int v = uyvy[index * 2 + 2] - 128;
            int y1 = uyvy[index * 2 + 3] << 8;
            rgb[index * 3 + 0] = minmax(0, (y0 + 359 * v) >> 8, 255);
            rgb[index * 3 + 1] = minmax(0, (y0 + 88 * v - 183 * u) >> 8, 255);
            rgb[index * 3 + 2] = minmax(0, (y0 + 454 * u) >> 8, 255);
            rgb[index * 3 + 3] = minmax(0, (y1 + 359 * v) >> 8, 255);
            rgb[index * 3 + 4] = minmax(0, (y1 + 88 * v - 183 * u) >> 8, 255);
            rgb[index * 3 + 5] = minmax(0, (y1 + 454 * u) >> 8, 255);
        }
    }
    return rgb;
}



static void convert_422_to_420(uint8_t *yuv, uint32_t const width, uint32_t const height, size_t const offset) {
    uint8_t *src = yuv + offset;
    uint8_t *dst = yuv + offset;

    for (int y = 0; y < height / 2; y++) {
        for (int x = 0; x < width; x++) {
            *dst = *src;
            dst += 2;
            src += 2;
        }

        src += width * 2;
    }
}



static void yuyv422_to_yuyv420(uint8_t *yuv, uint32_t width, uint32_t const height) {
    convert_422_to_420(yuv, width, height, 1);
}



static void uyvy422_to_uyvy420(uint8_t *yuv, uint32_t width, uint32_t const height) {
    convert_422_to_420(yuv, width, height, 0);
}






static void startCamera(uvcCameraWorker_s *cameraWorker) {
    puts("startCamera");
    cameraWorker->isActive = true;
    uvcStart(cameraWorker->camera);
}



static void captureImage(uvcCameraWorker_s *cameraWorker) {
    int result;
    //fputs("capturing Image...", stdout);
    //fflush(stdout);


//    fprintf(stderr, "Lock...");
    result = pthread_mutex_lock(&cameraWorker->frameMutex);
    assert(result == 0);
//    fprintf(stderr, "acquired\n");

    while (cameraWorker->numWaitingClients < cameraWorker->numClients) {
        result = pthread_cond_wait(&cameraWorker->clientsWaitingCond, &cameraWorker->frameMutex);
        assert(result == 0);
    }

    uvcCaptureFrame(cameraWorker->camera, s_timeout);

    if (cameraWorker->camera->pixelFormat == V4L2_PIX_FMT_JPEG) {
        cameraWorker->imageData = cameraWorker->camera->head->start;
        cameraWorker->imageFill = cameraWorker->camera->head->length;
    } else {
#ifndef NO_OMX
        switch (cameraWorker->camera->pixelFormat) {
            case V4L2_PIX_FMT_YUYV:
                yuyv422_to_yuyv420(cameraWorker->camera->head->start, cameraWorker->camera->width, cameraWorker->camera->height);
                break;

            case V4L2_PIX_FMT_UYVY:
                uyvy422_to_uyvy420(cameraWorker->camera->head->start, cameraWorker->camera->width, cameraWorker->camera->height);
                break;

            default:
                break;
        }

        result = pthread_mutex_lock(&s_omxMutex);
        assert(result == 0);

        omxJPEGEncProcess(cameraWorker->omx, cameraWorker->imageData, &cameraWorker->imageFill, cameraWorker->imageSize, cameraWorker->camera->head->start, cameraWorker->camera->head->length);

        result = pthread_mutex_unlock(&s_omxMutex);
        assert(result == 0);
#else
        uint8_t *rgb = NULL;

        switch (cameraWorker->camera->pixelFormat) {
            case V4L2_PIX_FMT_YUYV:
                rgb = yuyv2rgb(cameraWorker->camera->head->start, cameraWorker->camera->width, cameraWorker->camera->height);
                break;

            case V4L2_PIX_FMT_UYVY:
                rgb = uyvy2rgb(cameraWorker->camera->head->start, cameraWorker->camera->width, cameraWorker->camera->height);
                break;

            default:
                assert(false);
                break;
        }

        jpeg(&cameraWorker->imageData, &cameraWorker->imageFill, rgb, cameraWorker->camera->width, cameraWorker->camera->height, 25);
        free(rgb);
#endif
    }

    assert(cameraWorker->imageFill <= cameraWorker->imageSize);

//    printf("%zu\n", cameraWorker->imageFill);
//    puts("done");

    cameraWorker->numProcessingClients = cameraWorker->numWaitingClients;
    cameraWorker->numWaitingClients = 0;

    result = pthread_cond_broadcast(&cameraWorker->frameReadyCond);
    assert(result == 0);

    while (cameraWorker->numProcessingClients > 0) {
        result = pthread_cond_wait(&cameraWorker->frameConsumingDoneCond, &cameraWorker->frameMutex);
        assert(result == 0);
    }

    result = pthread_mutex_unlock(&cameraWorker->frameMutex);
    assert(result == 0);
}



static void stopCamera(uvcCameraWorker_s *cameraWorker) {
    puts("stopCamera");
    uvcStop(cameraWorker->camera);
    cameraWorker->isActive = false;
}



static void * cameraThread(void *data) {
    uvcCameraWorker_s *cameraWorker = data;
    int result;

#ifndef NO_OMX
    switch (cameraWorker->camera->pixelFormat) {
        case V4L2_PIX_FMT_YUYV:
            cameraWorker->omx = omxJPEGEncInit(cameraWorker->camera->width, cameraWorker->camera->height, cameraWorker->camera->height, 7, OMX_COLOR_FormatYCbYCr);
            break;

        case V4L2_PIX_FMT_UYVY:
            cameraWorker->omx = omxJPEGEncInit(cameraWorker->camera->width, cameraWorker->camera->height, cameraWorker->camera->height, 7, OMX_COLOR_FormatCbYCrY);
            break;

        case V4L2_PIX_FMT_YUV420:
            cameraWorker->omx = omxJPEGEncInit(cameraWorker->camera->width, cameraWorker->camera->height, cameraWorker->camera->height, 7, OMX_COLOR_FormatYUV420PackedPlanar);
            break;

        case V4L2_PIX_FMT_JPEG:
            cameraWorker->omx = NULL;
            break;

        default:
            puts("unsupported color format");
            assert(false);
    }
#endif

    result = pthread_cond_init(&cameraWorker->frameReadyCond, NULL);
    assert(result == 0);

    result = pthread_cond_init(&cameraWorker->frameConsumingDoneCond, NULL);
    assert(result == 0);

    result = pthread_cond_init(&cameraWorker->clientConnectedCond, NULL);
    assert(result == 0);

    result = pthread_cond_init(&cameraWorker->clientsWaitingCond, NULL);
    assert(result == 0);

    result = pthread_mutex_init(&cameraWorker->frameMutex, NULL);
    assert(result == 0);

    while (cameraWorker->keepAlive) {
        if ((cameraWorker->numClients > 0) && (!cameraWorker->isActive)) {
            startCamera(cameraWorker);
        } else if ((cameraWorker->numClients > 0) && (cameraWorker->isActive)) {
            captureImage(cameraWorker);
        } else if ((cameraWorker->numClients == 0) && (cameraWorker->isActive)) {
            stopCamera(cameraWorker);
        } else {
            fputs("waiting for client...", stdout);
            fflush(stdout);
            result = pthread_mutex_lock(&cameraWorker->frameMutex);
            assert(result == 0);

            while (cameraWorker->numClients == 0) {
                result = pthread_cond_wait(&cameraWorker->clientConnectedCond, &cameraWorker->frameMutex);
                assert(result == 0);

                if (!cameraWorker->keepAlive) {
                    result = pthread_mutex_unlock(&cameraWorker->frameMutex);
                    assert(result == 0);
                    goto exit;
                }
            }

            result = pthread_mutex_unlock(&cameraWorker->frameMutex);
            assert(result == 0);
            puts(" connected.");
        }
    }

exit:

    uvcDeinit(cameraWorker->camera);
    cameraWorker->imageSize = 0;
    cameraWorker->imageFill = 0;
    free(cameraWorker->imageData);
    result = pthread_cond_signal(&cameraWorker->clientConnectedCond);
    assert(result == 0);

    result = pthread_mutex_destroy(&cameraWorker->frameMutex);
    assert(result == 0);

    result = pthread_cond_destroy(&cameraWorker->clientsWaitingCond);
    assert(result == 0);

    result = pthread_cond_destroy(&cameraWorker->clientConnectedCond);
    assert(result == 0);

    result = pthread_cond_destroy(&cameraWorker->frameConsumingDoneCond);
    assert(result == 0);

    result = pthread_cond_destroy(&cameraWorker->frameReadyCond);
    assert(result == 0);

    if (cameraWorker->isActive) {
        stopCamera(cameraWorker);
    }

#ifndef NO_OMX
    if (cameraWorker->omx) {
        omxJPEGEncDeinit(cameraWorker->omx);
    }
#endif

//    puts("cameraThread DONE");
    return NULL;
}



bool uvcDoesCameraExist(int device) {
//    pthread_t thread = pthread_self();
//    printf(COLOR_YELLOW "uvcDoesCameraExist: %08lx\n" COLOR_NC, thread);

    uvcCameraWorker_s *cameraWorker = &s_video[device];
    return cameraWorker->camera != NULL;
}



void uvcConnectClient(int device) {
//    pthread_t thread = pthread_self();
//    printf(COLOR_YELLOW "uvcConnectClient: %08lx\n" COLOR_NC, thread);

    uvcCameraWorker_s *cameraWorker = &s_video[device];
    int result;
    result = pthread_mutex_lock(&cameraWorker->frameMutex);
    assert(result == 0);

    cameraWorker->numClients++;

    if (cameraWorker->numClients == 1) {
        result = pthread_cond_signal(&cameraWorker->clientConnectedCond);
        assert(result == 0);
    }

    result = pthread_mutex_unlock(&cameraWorker->frameMutex);
    assert(result == 0);
}



void uvcDisconnectClient(int device) {
//    pthread_t thread = pthread_self();
//    printf(COLOR_YELLOW "uvcDisconnectClient: %08lx\n" COLOR_NC, thread);

    uvcCameraWorker_s *cameraWorker = &s_video[device];
    int result;
    result = pthread_mutex_lock(&cameraWorker->frameMutex);
    assert(result == 0);

    cameraWorker->numClients--;

    if (cameraWorker->numWaitingClients >= cameraWorker->numClients) {
        result = pthread_cond_signal(&cameraWorker->clientsWaitingCond);
        assert(result == 0);
    }

    result = pthread_mutex_unlock(&cameraWorker->frameMutex);
    assert(result == 0);
}



void uvcGetImage(int device, uint8_t **data, size_t *len) {
//    pthread_t thread = pthread_self();
//    printf(COLOR_YELLOW "uvcGetImage: %08lx\n" COLOR_NC, thread);

    uvcCameraWorker_s *cameraWorker = &s_video[device];
    int result;
    result = pthread_mutex_lock(&cameraWorker->frameMutex);
    assert(result == 0);

    cameraWorker->numWaitingClients++;

    if (cameraWorker->numWaitingClients == cameraWorker->numClients) {
        result = pthread_cond_signal(&cameraWorker->clientsWaitingCond);
        assert(result == 0);
    }

    result = pthread_cond_wait(&cameraWorker->frameReadyCond, &cameraWorker->frameMutex);
    assert(result == 0);

    //puts("got Image");
    *data = cameraWorker->imageData;
    *len = cameraWorker->imageFill;
}



void uvcGetImageDone(int device) {
//    pthread_t thread = pthread_self();
//    printf(COLOR_YELLOW "uvcGetImageDone: %08lx\n" COLOR_NC, thread);

    uvcCameraWorker_s *cameraWorker = &s_video[device];
    int result;
    cameraWorker->numProcessingClients--;

    if (cameraWorker->numProcessingClients == 0) {
        result = pthread_cond_signal(&cameraWorker->frameConsumingDoneCond);
        assert(result == 0);
    }

    result = pthread_mutex_unlock(&cameraWorker->frameMutex);
    assert(result == 0);
}



void uvcInitWorker(int device) {
    uvcCameraWorker_s *cameraWorker = &s_video[device];
    cameraWorker->imageData = NULL;
    cameraWorker->imageSize = 0;
    cameraWorker->imageFill = 0;
    cameraWorker->numClients = 0;
    cameraWorker->numWaitingClients = 0;
    cameraWorker->numProcessingClients = 0;
    cameraWorker->keepAlive = true;
    cameraWorker->isActive = false;
    cameraWorker->camera = NULL;

    int err;
    struct stat st;
    char path[16];
    sprintf(path, "/dev/video%d", device);
    err = stat(path, &st);

    if (err == 0) {
        cameraWorker->camera = uvcInit(path, 640, 480, V4L2_PIX_FMT_YUYV);

        if (cameraWorker->camera) {
            cameraWorker->imageSize = cameraWorker->camera->width * cameraWorker->camera->height * sizeof(uint8_t);
            cameraWorker->imageData = malloc(cameraWorker->imageSize);
            err = pthread_create(&cameraWorker->thread, NULL, cameraThread, &s_video[device]);
            assert(err == 0);
            err = pthread_detach(&cameraWorker->thread);
            assert(err == 0);
            printf(COLOR_YELLOW "uvcInitWorker %d: %08lx\n" COLOR_NC, device, cameraWorker->thread);
        } else {
            printf(COLOR_RED "\nfailed to open %s" COLOR_NC "\n", path);
        }
    } else {
        printf(COLOR_RED "\ncould not find %s" COLOR_NC "\n", path);
    }
}



void uvcDeinitWorker(int device) {
//    pthread_t thread = pthread_self();
//    printf(COLOR_YELLOW "uvcDeinitWorker: %08lx\n" COLOR_NC, thread);

    uvcCameraWorker_s *cameraWorker = &s_video[device];
    cameraWorker->keepAlive = false;
    puts("uvcDeinitWorker DONE");
}
