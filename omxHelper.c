//
//  omxHelper.c
//  OpenMAX
//
//  Created by Michael Kwasnicki on 29.08.17.
//  Copyright © 2017 Michael Kwasnicki. All rights reserved.
//


#include "omxHelper.h"

#include <assert.h>
#include <stdio.h>

#include "cHelper.h"



const char *omxBoolEnum[] = {"OMX_FALSE", "OMX_TRUE"};
const char *omxDirTypeEnum[] = {"OMX_DirInput", "OMX_DirOutput"};
const char *omxPortDomainTypeEnum[] = {
    "OMX_PortDomainAudio",
    "OMX_PortDomainVideo",
    "OMX_PortDomainImage",
    "OMX_PortDomainOther"
};



const char *omxImageCodingTypeEnum(OMX_IMAGE_CODINGTYPE eCompressionFormat) {
    static char unknown[32];

    switch (eCompressionFormat) {
        case CASE_STRING(OMX_IMAGE_CodingUnused);
        case CASE_STRING(OMX_IMAGE_CodingAutoDetect);
        case CASE_STRING(OMX_IMAGE_CodingJPEG);
        case CASE_STRING(OMX_IMAGE_CodingJPEG2K);
        case CASE_STRING(OMX_IMAGE_CodingEXIF);
        case CASE_STRING(OMX_IMAGE_CodingTIFF);
        case CASE_STRING(OMX_IMAGE_CodingGIF);
        case CASE_STRING(OMX_IMAGE_CodingPNG);
        case CASE_STRING(OMX_IMAGE_CodingLZW);
        case CASE_STRING(OMX_IMAGE_CodingBMP);

        case CASE_STRING(OMX_IMAGE_CodingTGA);
        case CASE_STRING(OMX_IMAGE_CodingPPM);

        default: {
            snprintf(unknown, sizeof(unknown), "OMX_IMAGE_CODINGTYPE 0x%08x", eCompressionFormat);
            return unknown;
        }
    }
}



const char *omxColorFormatTypeEnum(OMX_COLOR_FORMATTYPE eColorFormat) {
    static char unknown[32];

    switch (eColorFormat) {
        case CASE_STRING(OMX_COLOR_FormatUnused);
        case CASE_STRING(OMX_COLOR_FormatMonochrome);
        case CASE_STRING(OMX_COLOR_Format8bitRGB332);
        case CASE_STRING(OMX_COLOR_Format12bitRGB444);
        case CASE_STRING(OMX_COLOR_Format16bitARGB4444);
        case CASE_STRING(OMX_COLOR_Format16bitARGB1555);
        case CASE_STRING(OMX_COLOR_Format16bitRGB565);
        case CASE_STRING(OMX_COLOR_Format16bitBGR565);
        case CASE_STRING(OMX_COLOR_Format18bitRGB666);
        case CASE_STRING(OMX_COLOR_Format18bitARGB1665);
        case CASE_STRING(OMX_COLOR_Format19bitARGB1666);
        case CASE_STRING(OMX_COLOR_Format24bitRGB888);
        case CASE_STRING(OMX_COLOR_Format24bitBGR888);
        case CASE_STRING(OMX_COLOR_Format24bitARGB1887);
        case CASE_STRING(OMX_COLOR_Format25bitARGB1888);
        case CASE_STRING(OMX_COLOR_Format32bitBGRA8888);
        case CASE_STRING(OMX_COLOR_Format32bitARGB8888);
        case CASE_STRING(OMX_COLOR_FormatYUV411Planar);
        case CASE_STRING(OMX_COLOR_FormatYUV411PackedPlanar);
        case CASE_STRING(OMX_COLOR_FormatYUV420Planar);
        case CASE_STRING(OMX_COLOR_FormatYUV420PackedPlanar);
        case CASE_STRING(OMX_COLOR_FormatYUV420SemiPlanar);
        case CASE_STRING(OMX_COLOR_FormatYUV422Planar);
        case CASE_STRING(OMX_COLOR_FormatYUV422PackedPlanar);
        case CASE_STRING(OMX_COLOR_FormatYUV422SemiPlanar);
        case CASE_STRING(OMX_COLOR_FormatYCbYCr);
        case CASE_STRING(OMX_COLOR_FormatYCrYCb);
        case CASE_STRING(OMX_COLOR_FormatCbYCrY);
        case CASE_STRING(OMX_COLOR_FormatCrYCbY);
        case CASE_STRING(OMX_COLOR_FormatYUV444Interleaved);
        case CASE_STRING(OMX_COLOR_FormatRawBayer8bit);
        case CASE_STRING(OMX_COLOR_FormatRawBayer10bit);
        case CASE_STRING(OMX_COLOR_FormatRawBayer8bitcompressed);
        case CASE_STRING(OMX_COLOR_FormatL2);
        case CASE_STRING(OMX_COLOR_FormatL4);
        case CASE_STRING(OMX_COLOR_FormatL8);
        case CASE_STRING(OMX_COLOR_FormatL16);
        case CASE_STRING(OMX_COLOR_FormatL24);
        case CASE_STRING(OMX_COLOR_FormatL32);
        case CASE_STRING(OMX_COLOR_FormatYUV420PackedSemiPlanar);
        case CASE_STRING(OMX_COLOR_FormatYUV422PackedSemiPlanar);
        case CASE_STRING(OMX_COLOR_Format18BitBGR666);
        case CASE_STRING(OMX_COLOR_Format24BitARGB6666);
        case CASE_STRING(OMX_COLOR_Format24BitABGR6666);

        case CASE_STRING(OMX_COLOR_Format32bitABGR8888);
        case CASE_STRING(OMX_COLOR_Format8bitPalette);
        case CASE_STRING(OMX_COLOR_FormatYUVUV128);
        case CASE_STRING(OMX_COLOR_FormatRawBayer12bit);
        case CASE_STRING(OMX_COLOR_FormatBRCMEGL);
        case CASE_STRING(OMX_COLOR_FormatBRCMOpaque); 
        case CASE_STRING(OMX_COLOR_FormatYVU420PackedPlanar); 
        case CASE_STRING(OMX_COLOR_FormatYVU420PackedSemiPlanar);
        case CASE_STRING(OMX_COLOR_FormatRawBayer16bit); 
        case CASE_STRING(OMX_COLOR_FormatYUV420_16PackedPlanar); 
        case CASE_STRING(OMX_COLOR_FormatYUVUV64_16);
        default: {
            snprintf(unknown, sizeof(unknown), "OMX_COLOR_FORMATTYPE 0x%08x", eColorFormat);
            return unknown;
        }
    }
}



const char *omxEventTypeEnum(OMX_EVENTTYPE eEvent) {
    static char unknown[32];

    switch (eEvent) {
        case CASE_STRING(OMX_EventCmdComplete);
        case CASE_STRING(OMX_EventError);
        case CASE_STRING(OMX_EventMark);
        case CASE_STRING(OMX_EventPortSettingsChanged);
        case CASE_STRING(OMX_EventBufferFlag);
        case CASE_STRING(OMX_EventResourcesAcquired);
        case CASE_STRING(OMX_EventComponentResumed);
        case CASE_STRING(OMX_EventDynamicResourcesAvailable);
        case CASE_STRING(OMX_EventPortFormatDetected);
            
        case CASE_STRING(OMX_EventParamOrConfigChanged);

        default: {
            snprintf(unknown, sizeof(unknown), "OMX_EVENTTYPE 0x%08x", eEvent);
            return unknown;
        }
    }
}



const char *omxStateTypeEnum(OMX_STATETYPE eState) {
    static char unknown[32];

    switch (eState) {
        case CASE_STRING(OMX_StateInvalid);
        case CASE_STRING(OMX_StateLoaded);
        case CASE_STRING(OMX_StateIdle);
        case CASE_STRING(OMX_StateExecuting);
        case CASE_STRING(OMX_StatePause);
        case CASE_STRING(OMX_StateWaitForResources);

        default: {
            snprintf(unknown, sizeof(unknown), "OMX_STATETYPE 0x%08x", eState);
            return unknown;
        }
    }
}



const char *omxCommandTypeEnum(OMX_COMMANDTYPE eCommand) {
    static char unknown[32];

    switch (eCommand) {
        case CASE_STRING(OMX_CommandStateSet);
        case CASE_STRING(OMX_CommandFlush);
        case CASE_STRING(OMX_CommandPortDisable);
        case CASE_STRING(OMX_CommandPortEnable);
        case CASE_STRING(OMX_CommandMarkBuffer);

        default: {
            snprintf(unknown, sizeof(unknown), "OMX_COMMANDTYPE 0x%08x", eCommand);
            return unknown;
        }
    }
}



const char *omxErrorTypeEnum(OMX_ERRORTYPE eError) {
    static char unknown[32];

    switch (eError) {
        case CASE_STRING(OMX_ErrorNone);
        case CASE_STRING(OMX_ErrorInsufficientResources);
        case CASE_STRING(OMX_ErrorUndefined);
        case CASE_STRING(OMX_ErrorInvalidComponentName);
        case CASE_STRING(OMX_ErrorComponentNotFound);
        case CASE_STRING(OMX_ErrorInvalidComponent);
        case CASE_STRING(OMX_ErrorBadParameter);
        case CASE_STRING(OMX_ErrorNotImplemented);
        case CASE_STRING(OMX_ErrorUnderflow);
        case CASE_STRING(OMX_ErrorOverflow);
        case CASE_STRING(OMX_ErrorHardware);
        case CASE_STRING(OMX_ErrorInvalidState);
        case CASE_STRING(OMX_ErrorStreamCorrupt);
        case CASE_STRING(OMX_ErrorPortsNotCompatible);
        case CASE_STRING(OMX_ErrorResourcesLost);
        case CASE_STRING(OMX_ErrorNoMore);
        case CASE_STRING(OMX_ErrorVersionMismatch);
        case CASE_STRING(OMX_ErrorNotReady);
        case CASE_STRING(OMX_ErrorTimeout);
        case CASE_STRING(OMX_ErrorSameState);
        case CASE_STRING(OMX_ErrorResourcesPreempted);
        case CASE_STRING(OMX_ErrorPortUnresponsiveDuringAllocation);
        case CASE_STRING(OMX_ErrorPortUnresponsiveDuringDeallocation);
        case CASE_STRING(OMX_ErrorPortUnresponsiveDuringStop);
        case CASE_STRING(OMX_ErrorIncorrectStateTransition);
        case CASE_STRING(OMX_ErrorIncorrectStateOperation);
        case CASE_STRING(OMX_ErrorUnsupportedSetting);
        case CASE_STRING(OMX_ErrorUnsupportedIndex);
        case CASE_STRING(OMX_ErrorBadPortIndex);
        case CASE_STRING(OMX_ErrorPortUnpopulated);
        case CASE_STRING(OMX_ErrorComponentSuspended);
        case CASE_STRING(OMX_ErrorDynamicResourcesUnavailable);
        case CASE_STRING(OMX_ErrorMbErrorsInFrame);
        case CASE_STRING(OMX_ErrorFormatNotDetected);
        case CASE_STRING(OMX_ErrorContentPipeOpenFailed);
        case CASE_STRING(OMX_ErrorContentPipeCreationFailed);
        case CASE_STRING(OMX_ErrorSeperateTablesUsed);
        case CASE_STRING(OMX_ErrorTunnelingUnsupported);

        case CASE_STRING(OMX_ErrorDiskFull);
        case CASE_STRING(OMX_ErrorMaxFileSize);
        case CASE_STRING(OMX_ErrorDrmUnauthorised);
        case CASE_STRING(OMX_ErrorDrmExpired);
        case CASE_STRING(OMX_ErrorDrmGeneral);

        default: {
            snprintf(unknown, sizeof(unknown), "OMX_ERRORTYPE 0x%08x", eError);
            return unknown;
        }
    }
}



void omxDumpParamPortDefinition(OMX_PARAM_PORTDEFINITIONTYPE portDefinition) {
    printf(LEVEL_2 "nPortIndex:         %u\n", portDefinition.nPortIndex);
    printf(LEVEL_2 "eDir:               %s\n", omxDirTypeEnum[portDefinition.eDir]);
    printf(LEVEL_2 "nBufferCountActual: %u\n", portDefinition.nBufferCountActual);
    printf(LEVEL_2 "nBufferCountMin:    %u\n", portDefinition.nBufferCountMin);
    printf(LEVEL_2 "nBufferSize:        %u\n", portDefinition.nBufferSize);
    printf(LEVEL_2 "bEnabled:           %s\n", omxBoolEnum[portDefinition.bEnabled]);
    printf(LEVEL_2 "bPopulated:         %s\n", omxBoolEnum[portDefinition.bPopulated]);
    printf(LEVEL_2 "bBuffersContiguous: %s\n", omxBoolEnum[portDefinition.bBuffersContiguous]);
    printf(LEVEL_2 "nBufferAlignment:   %u\n", portDefinition.nBufferAlignment);
    printf(LEVEL_2 "eDomain:            %s\n", omxPortDomainTypeEnum[portDefinition.eDomain]);
}



void omxDumpImagePortDefinition(OMX_IMAGE_PORTDEFINITIONTYPE image) {
    printf(LEVEL_3 "cMIMEType:             %s\n", image.cMIMEType);
    printf(LEVEL_3 "nFrameWidth:           %u\n", image.nFrameWidth);
    printf(LEVEL_3 "nFrameHeight:          %u\n", image.nFrameHeight);
    printf(LEVEL_3 "nStride:               %d\n", image.nStride);
    printf(LEVEL_3 "nSliceHeight:          %u\n", image.nSliceHeight);
    printf(LEVEL_3 "bFlagErrorConcealment: %s\n", omxBoolEnum[image.bFlagErrorConcealment]);
    printf(LEVEL_3 "eCompressionFormat:    %s\n", omxImageCodingTypeEnum(image.eCompressionFormat));
    printf(LEVEL_3 "eColorFormat:          %s\n", omxColorFormatTypeEnum(image.eColorFormat));
}



void omxAssertState(OMX_HANDLETYPE handle, OMX_STATETYPE state) {
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;
    OMX_STATETYPE omxState = OMX_StateInvalid;
    omxErr = OMX_GetState(handle, &omxState);
    omxAssert(omxErr);
    assert(omxState == state);
}



bool omxAssertImagePortFormatSupported(OMX_HANDLETYPE omxHandle, OMX_U32 nPortIndex, OMX_COLOR_FORMATTYPE eColorFormat) {
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;
    OMX_IMAGE_PARAM_PORTFORMATTYPE portformat;
    OMX_INIT_STRUCTURE(portformat);
    portformat.nPortIndex = nPortIndex;
    int i = 0;

    do {
        portformat.nIndex = i;
        omxErr = OMX_GetParameter(omxHandle, OMX_IndexParamImagePortFormat, &portformat);

        if (omxErr == OMX_ErrorNone) {
            if (portformat.eColorFormat == eColorFormat) {
                return true;
            }

            i++;
        }
    } while (omxErr == OMX_ErrorNone);

    return false;
}



void omxEnablePort(OMX_HANDLETYPE omxHandle, OMX_U32 portIndex, OMX_BOOL enabled) {
    static const OMX_COMMANDTYPE command[2] = {OMX_CommandPortDisable, OMX_CommandPortEnable};
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE portDefinition;
    OMX_INIT_STRUCTURE(portDefinition);
    portDefinition.nPortIndex = portIndex;

    omxErr = OMX_SendCommand(omxHandle, command[enabled], portIndex, NULL);
    omxAssert(omxErr);

    do {
        omxErr = OMX_GetParameter(omxHandle, OMX_IndexParamPortDefinition, &portDefinition);
        omxAssert(omxErr);
    } while (portDefinition.bEnabled != enabled);
}



void omxSwitchToState(OMX_HANDLETYPE omxHandle, OMX_STATETYPE state) {
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;
    OMX_STATETYPE omxState = OMX_StateInvalid;
    omxErr = OMX_GetState(omxHandle, &omxState);
    omxAssert(omxErr);

    if (omxState != state) {
        omxErr = OMX_SendCommand(omxHandle, OMX_CommandStateSet, state, NULL);
        printf("%x -> %x : %x\n", omxState, state, omxErr);
        omxAssert(omxErr);

        int c = 10;

        do {
            c--;
            omxErr = OMX_GetState(omxHandle, &omxState);
            printf("test : %x -> %x : %x\n", omxState, state, omxErr);
            omxAssert(omxErr);
            //sleep(1);
        } while (omxState != state && c);
    }
}

