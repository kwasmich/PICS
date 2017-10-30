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
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

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


static uvcCamera_s s_video;
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



void uvcConnectClient() {
    int result;
    result = pthread_mutex_lock(&s_video.frameMutex);
    assert(result == 0);

    s_video.numClients++;

    if (s_video.numClients == 1) {
        result = pthread_cond_signal(&s_video.clientConnectedCond);
        assert(result == 0);
    }

    result = pthread_mutex_unlock(&s_video.frameMutex);
    assert(result == 0);
}



void uvcDisconnectClient() {
    int result;
    result = pthread_mutex_lock(&s_video.frameMutex);
    assert(result == 0);

    s_video.numClients--;

    if (s_video.numWaitingClients >= s_video.numClients) {
        result = pthread_cond_signal(&s_video.clientsWaitingCond);
        assert(result == 0);
    }

    result = pthread_mutex_unlock(&s_video.frameMutex);
    assert(result == 0);
}



void uvcGetImage(int videoIndex, uint8_t **data, size_t *len) {
    int result;
    result = pthread_mutex_lock(&s_video.frameMutex);
    assert(result == 0);

    s_video.numWaitingClients++;

    if (s_video.numWaitingClients == s_video.numClients) {
        result = pthread_cond_signal(&s_video.clientsWaitingCond);
        assert(result == 0);
    }

    result = pthread_cond_wait(&s_video.frameReadyCond, &s_video.frameMutex);
    assert(result == 0);

    //puts("got Image");
    *data = s_video.imageData;
    *len = s_video.imageSize;
}



void uvcGetImageDone(int videoIndex) {
    int result;
    s_video.numProcessingClients--;

    if (s_video.numProcessingClients == 0) {
        result = pthread_cond_signal(&s_video.frameConsumingDoneCond);
        assert(result == 0);
    }

    result = pthread_mutex_unlock(&s_video.frameMutex);
    assert(result == 0);
}



void uvcInit() {
    printf("pthread_t: %zd\n", sizeof(pthread_t));
    printf("pthread_cond_t: %zd\n", sizeof(pthread_cond_t));
    printf("pthread_mutex_t: %zd\n", sizeof(pthread_mutex_t));

    s_video.imageData = NULL;
    s_video.imageSize = 0;
    s_video.fd = -1;
    s_video.numClients = 0;
    s_video.numWaitingClients = 0;
    s_video.numProcessingClients = 0;
    s_video.keepAlive = true;
    s_video.isActive = false;

    int result = pthread_create(&s_video.thread , NULL, cameraThread, &s_video);
    assert(result == 0);
}



void uvcDeinit() {
    s_video.keepAlive = false;
    int result = pthread_cond_signal(&s_video.clientConnectedCond);
    assert(result == 0);
    pthread_join(s_video.thread, NULL);
}
