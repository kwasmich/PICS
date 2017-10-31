//
//  uvcCamera.c
//  PICS
//
//  Created by Michael Kwasnicki on 29.10.17.
//  Copyright © 2017 Michael Kwasnicki. All rights reserved.
//

#include "uvcCamera.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#include "mmapHelper.h"



typedef struct {
    pthread_cond_t frameReadyCond;
    pthread_cond_t frameConsumingDoneCond;
    pthread_cond_t clientsWaitingCond;
    pthread_cond_t clientConnectedCond;
    pthread_mutex_t frameMutex;
    pthread_t thread;

    volatile uint8_t *imageData;
    volatile size_t imageSize;
    int fd;
    volatile int numClients;
    volatile int numWaitingClients;
    volatile int numProcessingClients;
    volatile bool keepAlive;
    bool isActive;
} uvcCamera_s;


static uvcCamera_s s_video[10];
MapFile_s s_mapA;
MapFile_s s_mapB;
bool s_toggleImage;



static void startCamera(uvcCamera_s *camera) {
    puts("startCamera");
    camera->isActive = true;
}



static void captureImage(uvcCamera_s *camera) {
    int result;
    //fputs("capturing Image...", stdout);
    fflush(stdout);

    result = pthread_mutex_lock(&camera->frameMutex);
    assert(result == 0);

    while (camera->numWaitingClients < camera->numClients) {
        result = pthread_cond_wait(&camera->clientsWaitingCond, &camera->frameMutex);
        assert(result == 0);
    }

    if (s_toggleImage) {
        camera->imageData = s_mapA.data;
        camera->imageSize = s_mapA.len;
    } else {
        camera->imageData = s_mapB.data;
        camera->imageSize = s_mapB.len;
    }

    s_toggleImage = !s_toggleImage;
    usleep(200000);
    //puts("done");

    camera->numProcessingClients = camera->numWaitingClients;
    camera->numWaitingClients = 0;

    result = pthread_cond_broadcast(&camera->frameReadyCond);
    assert(result == 0);

    while (camera->numProcessingClients > 0) {
        result = pthread_cond_wait(&camera->frameConsumingDoneCond, &camera->frameMutex);
        assert(result == 0);
    }

    result = pthread_mutex_unlock(&camera->frameMutex);
    assert(result == 0);
}



static void stopCamera(uvcCamera_s *camera) {
    puts("stopCamera");
    camera->isActive = false;
}



static void * cameraThread(void *data) {
    uvcCamera_s *camera = data;
    int result;
    
    initMapFile(&s_mapA, "a.jpg", MAP_RO);
    initMapFile(&s_mapB, "b.jpg", MAP_RO);
    assert(s_mapA.len > 0);
    assert(s_mapB.len > 0);
    printf("jpegDataSize: %zu\n", s_mapA.len);
    printf("jpegDataSize: %zu\n", s_mapB.len);

    result = pthread_cond_init(&camera->frameReadyCond, NULL);
    assert(result == 0);

    result = pthread_cond_init(&camera->frameConsumingDoneCond, NULL);
    assert(result == 0);

    result = pthread_cond_init(&camera->clientConnectedCond, NULL);
    assert(result == 0);

    result = pthread_cond_init(&camera->clientsWaitingCond, NULL);
    assert(result == 0);

    result = pthread_mutex_init(&camera->frameMutex, NULL);
    assert(result == 0);

    while (camera->keepAlive) {
        if ((camera->numClients > 0) && (!camera->isActive)) {
            startCamera(camera);
        } else if ((camera->numClients > 0) && (camera->isActive)) {
            captureImage(camera);
        } else if ((camera->numClients == 0) && (camera->isActive)) {
            stopCamera(camera);
        } else {
            fputs("waiting for client...", stdout);
            fflush(stdout);
            result = pthread_mutex_lock(&camera->frameMutex);
            assert(result == 0);

            while (camera->numClients == 0) {
                result = pthread_cond_wait(&camera->clientConnectedCond, &camera->frameMutex);
                assert(result == 0);

                if (!camera->keepAlive) {
                    result = pthread_mutex_unlock(&camera->frameMutex);
                    assert(result == 0);
                    goto exit;
                }
            }

            result = pthread_mutex_unlock(&camera->frameMutex);
            assert(result == 0);
            puts(" connected.");
        }
    }

exit:

    result = pthread_mutex_destroy(&camera->frameMutex);
    assert(result == 0);

    result = pthread_cond_destroy(&camera->clientsWaitingCond);
    assert(result == 0);

    result = pthread_cond_destroy(&camera->clientConnectedCond);
    assert(result == 0);

    result = pthread_cond_destroy(&camera->frameConsumingDoneCond);
    assert(result == 0);

    result = pthread_cond_destroy(&camera->frameReadyCond);
    assert(result == 0);

    if (camera->isActive) {
        stopCamera(camera);
    }

    return NULL;
}



bool uvcDoesCameraExist(int device) {
    uvcCamera_s *camera = &s_video[device];
    return camera->thread;
}



void uvcConnectClient(int device) {
    uvcCamera_s *camera = &s_video[device];
    int result;
    result = pthread_mutex_lock(&camera->frameMutex);
    assert(result == 0);

    camera->numClients++;

    if (camera->numClients == 1) {
        result = pthread_cond_signal(&camera->clientConnectedCond);
        assert(result == 0);
    }

    result = pthread_mutex_unlock(&camera->frameMutex);
    assert(result == 0);
}



void uvcDisconnectClient(int device) {
    uvcCamera_s *camera = &s_video[device];
    int result;
    result = pthread_mutex_lock(&camera->frameMutex);
    assert(result == 0);

    camera->numClients--;

    if (camera->numWaitingClients >= camera->numClients) {
        result = pthread_cond_signal(&camera->clientsWaitingCond);
        assert(result == 0);
    }

    result = pthread_mutex_unlock(&camera->frameMutex);
    assert(result == 0);
}



void uvcGetImage(int device, uint8_t **data, size_t *len) {
    uvcCamera_s *camera = &s_video[device];
    int result;
    result = pthread_mutex_lock(&camera->frameMutex);
    assert(result == 0);

    camera->numWaitingClients++;

    if (camera->numWaitingClients == camera->numClients) {
        result = pthread_cond_signal(&camera->clientsWaitingCond);
        assert(result == 0);
    }

    result = pthread_cond_wait(&camera->frameReadyCond, &camera->frameMutex);
    assert(result == 0);

    //puts("got Image");
    *data = camera->imageData;
    *len = camera->imageSize;
}



void uvcGetImageDone(int device) {
    uvcCamera_s *camera = &s_video[device];
    int result;
    camera->numProcessingClients--;

    if (camera->numProcessingClients == 0) {
        result = pthread_cond_signal(&camera->frameConsumingDoneCond);
        assert(result == 0);
    }

    result = pthread_mutex_unlock(&camera->frameMutex);
    assert(result == 0);
}



void uvcInit(int device) {
    uvcCamera_s *camera = &s_video[device];
    camera->imageData = NULL;
    camera->imageSize = 0;
    camera->fd = -1;
    camera->numClients = 0;
    camera->numWaitingClients = 0;
    camera->numProcessingClients = 0;
    camera->keepAlive = true;
    camera->isActive = false;

    int err;
    struct stat st;
    char path[12];
    sprintf(path, "/dev/video%d", device);
    err = stat(path, &st);
    
    if ((err == 0) || (device == 0)) {
        err = pthread_create(&camera->thread , NULL, cameraThread, &s_video);
        assert(err == 0);
    }
}



void uvcDeinit(int device) {
    uvcCamera_s *camera = &s_video[device];

    if (camera->thread) {
        camera->keepAlive = false;
        int result = pthread_cond_signal(&camera->clientConnectedCond);
        assert(result == 0);
        pthread_join(camera->thread, NULL);
    }
}
