//
//  omxJPEGEnc.h
//  OpenMAX
//
//  Created by Michael Kwasnicki on 29.08.17.
//  Copyright © 2017 Michael Kwasnicki. All rights reserved.
//

#ifndef omxJPEGEnc_h
#define omxJPEGEnc_h


#include <stddef.h>
#include <stdint.h>

#define OMX_SKIP64BIT
#include <IL/OMX_Component.h>


// forward declaration of a typedef struct
struct OMXContext_s;
typedef struct OMXContext_s OMXContext_s;


OMXContext_s * omxJPEGEncInit(uint32_t rawImageWidth, uint32_t rawImageHeight, uint32_t sliceHeight, uint8_t outputQuality, OMX_COLOR_FORMATTYPE colorFormat);
void omxJPEGEncDeinit(OMXContext_s *ctx);
void omxJPEGEncProcess(OMXContext_s *ctx, uint8_t *output, size_t *outputFill, size_t outputSize, uint8_t *rawImage, size_t rawImageSize);

void omxJPEGEnc(void);


#endif /* omxJPEGEnc_h */
