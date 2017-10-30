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
    pthread_t thread;
    int fd;
    bool isActive;
    pthread_cond_t imageCond;
    pthread_mutex_t imageMutex;
    pthread_cond_t clientCond;
    pthread_mutex_t clientMutex;
    pthread_rwlock_t lock;
    uint8_t *imageData;
    size_t imageSize;
    int numClients;
    bool keepAlive;
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
    fputs("capturing Image...", stdout);
    fflush(stdout);

    result = pthread_mutex_lock(&s_video.imageMutex);
    assert(result == 0);

    if (s_toggleImage) {
        camera->imageData = s_mapA.data;
        camera->imageSize = s_mapA.len;
    } else {
        camera->imageData = s_mapB.data;
        camera->imageSize = s_mapB.len;
    }

    s_toggleImage = !s_toggleImage;
    //sleep(1);
    usleep(200000);
    puts("done");

    result = pthread_mutex_unlock(&s_video.imageMutex);
    assert(result == 0);
    result = pthread_cond_broadcast(&s_video.imageCond);
    assert(result == 0);

    // give other threads enough time to acquire the lock and get the data.
    pthread_yield_np();
    usleep(5000);
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

    result = pthread_cond_init(&camera->imageCond, NULL);
    assert(result == 0);

    result = pthread_mutex_init(&camera->imageMutex, NULL);
    assert(result == 0);

    result = pthread_cond_init(&camera->clientCond, NULL);
    assert(result == 0);

    result = pthread_mutex_init(&camera->clientMutex, NULL);
    assert(result == 0);

    result = pthread_rwlock_init(&camera->lock, NULL);
    assert(result == 0);

    while (camera->keepAlive) {
        //usleep(5000);

        if ((camera->numClients > 0) && (!camera->isActive)) {
            startCamera(camera);
        } else if ((camera->numClients > 0) && (camera->isActive)) {
            captureImage(camera);
        } else if ((camera->numClients == 0) && (camera->isActive)) {
            stopCamera(camera);
        } else {
            fputs("waiting for client...", stdout);
            fflush(stdout);
            result = pthread_mutex_lock(&s_video.clientMutex);
            assert(result == 0);
            result = pthread_cond_wait(&s_video.clientCond, &s_video.clientMutex);
            assert(result == 0);
            result = pthread_mutex_unlock(&s_video.clientMutex);
            assert(result == 0);
            puts(" connected.");
        }
    }

    if (camera->isActive) {
        stopCamera(camera);
    }

    result = pthread_cond_destroy(&camera->imageCond);
    assert(result == 0);

    result = pthread_mutex_destroy(&camera->imageMutex);
    assert(result == 0);

    result = pthread_cond_destroy(&camera->clientCond);
    assert(result == 0);

    result = pthread_mutex_destroy(&camera->clientMutex);
    assert(result == 0);

    result = pthread_rwlock_destroy(&camera->lock);
    assert(result == 0);

    return NULL;
}



static void startCameraThread(uvcCamera_s *camera) {
    int result = pthread_create(&camera->thread , NULL, cameraThread, camera);

    if (result != 0) {
        perror("pthread_create");
    }
}



void uvcConnectClient() {
    int result;
    result = pthread_rwlock_wrlock(&s_video.lock);
    assert(result == 0);
    //    result = pthread_mutex_lock(&s_video.clientMutex);
    //    assert(result == 0);

    s_video.numClients++;

    //    result = pthread_mutex_unlock(&s_video.clientMutex);
    //    assert(result == 0);

    if (s_video.numClients == 1) {
        result = pthread_cond_broadcast(&s_video.clientCond);
        assert(result == 0);
    }

    result = pthread_rwlock_unlock(&s_video.lock);
    assert(result == 0);
}



void uvcDisconnectClient() {
    pthread_rwlock_wrlock(&s_video.lock);
    s_video.numClients--;
    pthread_rwlock_unlock(&s_video.lock);
}


void uvcGetImage(int videoIndex, uint8_t **data, size_t *len) {
    int result;
    result = pthread_mutex_lock(&s_video.imageMutex);
    assert(result == 0);
    result = pthread_cond_wait(&s_video.imageCond, &s_video.imageMutex);
    assert(result == 0);

    puts("got Image");
    *data = s_video.imageData;
    *len = s_video.imageSize;
}



void uvcGetImageDone(int videoIndex) {
    int result;
    result = pthread_mutex_unlock(&s_video.imageMutex);
    assert(result == 0);
}



void uvcInit() {
    s_video.fd = -1;
    s_video.numClients = 0;
    s_video.keepAlive = true;
    s_video.isActive = false;
    startCameraThread(&s_video);
}



//void cameraThread() {
//
//    sprintf(header2a, header2, mapA.len);
//    err = send(client, separator, strlen(separator), 0);
//    if (err == -1) break;
//    err = send(client, header2a, strlen(header2a), 0);
//    if (err == -1) break;
//    write(client, mapA.data, mapA.len);
//    if (err == -1) break;
//    usleep(500000);
//
//    sprintf(header2a, header2, mapB.len);
//    err = send(client, separator, strlen(separator), 0);
//    if (err == -1) break;
//    err = send(client, header2a, strlen(header2a), 0);
//    if (err == -1) break;
//    write(client, mapB.data, mapB.len);
//    if (err == -1) break;
//    usleep(500000);
//}
