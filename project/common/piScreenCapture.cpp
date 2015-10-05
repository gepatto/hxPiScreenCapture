#include <math.h>
#include <png.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include "bcm_host.h"

namespace piscreencapture {
    
//-----------------------------------------------------------------------

#ifndef ALIGN_TO_16
#define ALIGN_TO_16(x)  ((x + 15) & ~15)
#endif

//-----------------------------------------------------------------------
const char* program = NULL;
char pathName[255] = "/home/pi/";
//----------------------------------------


void setPath(const char * a)
{
    DIR *dir = NULL;
    dir = opendir(a);
    if (dir == NULL){
        printf("supplied path is not a directory %s\n", a);
    }else{
        const size_t len = strlen(a);
        char * tmp_filename = new char[len + 1];
        strncpy(tmp_filename, a, len);
        tmp_filename[len] = '\0';
        memset(&pathName[0], 0, sizeof(pathName));
        strcpy(pathName,tmp_filename);
        printf("ScreenCapture Path is now %s\n", pathName);
    }
}

int capture() {
        
    bool writeToStdout = false;
    char pngFileName[255];
    char pngName[255];
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    
    DIR *dir = NULL;
    dir = opendir(pathName);
    if (dir == NULL){
        printf("supplied path is not a directory %s\n", pathName);
        return 0;
    }

    strftime(pngFileName, sizeof(pngFileName), "%Y%m%d_%H%M%S_snapshot.png", tm);
    sprintf(pngName, "%s%s", pathName , pngFileName);

    int32_t requestedWidth = 0;
    int32_t requestedHeight = 0;
    uint32_t displayNumber = 0;

    VC_IMAGE_TYPE_T imageType = VC_IMAGE_RGBA32;
    int8_t dmxBytesPerPixel  = 4;

    int result = 0;

     bcm_host_init();

    //-------------------------------------------------------------------
    //
    // When the display is rotate (either 90 or 270 degrees) we need to
    // swap the width and height of the snapshot
    //

    char response[1024];
    int displayRotated = 0;

    if (vc_gencmd(response, sizeof(response), "get_config int") == 0)
    {
        vc_gencmd_number_property(response,
                                  "display_rotate",
                                  &displayRotated);
    }

    //-------------------------------------------------------------------

    DISPMANX_DISPLAY_HANDLE_T displayHandle
        = vc_dispmanx_display_open(displayNumber);

    DISPMANX_MODEINFO_T modeInfo;
    result = vc_dispmanx_display_get_info(displayHandle, &modeInfo);

    if (result != 0)
    {
        fprintf(stderr, "%s: unable to get display information\n", program);
        exit(EXIT_FAILURE);
    }

    int32_t pngWidth = modeInfo.width;
    int32_t pngHeight = modeInfo.height;

    if (requestedWidth > 0)
    {
        pngWidth = requestedWidth;

        if (requestedHeight == 0)
        {
            double numerator = modeInfo.height * requestedWidth;
            double denominator = modeInfo.width;

            pngHeight = (int32_t)ceil(numerator / denominator);
        }
    }

    if (requestedHeight > 0)
    {
        pngHeight = requestedHeight;

        if (requestedWidth == 0)
        {
            double numerator = modeInfo.width * requestedHeight;
            double denominator = modeInfo.height;

            pngWidth = (int32_t)ceil(numerator / denominator);
        }
    }

    //-------------------------------------------------------------------
    // only need to check low bit of displayRotated (value of 1 or 3).
    // If the display is rotated either 90 or 270 degrees (value 1 or 3)
    // the width and height need to be transposed.

    int32_t dmxWidth = pngWidth;
    int32_t dmxHeight = pngHeight;

    if (displayRotated & 1)
    {
        dmxWidth = pngHeight;
        dmxHeight = pngWidth;
    }

    int32_t dmxPitch = dmxBytesPerPixel * ALIGN_TO_16(dmxWidth);

    void *dmxImagePtr = malloc(dmxPitch * dmxHeight);

    if (dmxImagePtr == NULL)
    {
        fprintf(stderr, "%s: unable to allocated image buffer\n", program);
        exit(EXIT_FAILURE);
    }

    //-------------------------------------------------------------------

    uint32_t vcImagePtr = 0;
    DISPMANX_RESOURCE_HANDLE_T resourceHandle;
    resourceHandle = vc_dispmanx_resource_create(imageType,
                                                 dmxWidth,
                                                 dmxHeight,
                                                 &vcImagePtr);

    result = vc_dispmanx_snapshot(displayHandle,
                                  resourceHandle,
                                  DISPMANX_NO_ROTATE);

    if (result != 0)
    {
        vc_dispmanx_resource_delete(resourceHandle);
        vc_dispmanx_display_close(displayHandle);

        fprintf(stderr, "%s: vc_dispmanx_snapshot() failed\n", program);
        exit(EXIT_FAILURE);
    }

    VC_RECT_T rect;
    result = vc_dispmanx_rect_set(&rect, 0, 0, dmxWidth, dmxHeight);

    if (result != 0)
    {
        vc_dispmanx_resource_delete(resourceHandle);
        vc_dispmanx_display_close(displayHandle);

        fprintf(stderr, "%s: vc_dispmanx_rect_set() failed\n", program);
        exit(EXIT_FAILURE);
    }

    result = vc_dispmanx_resource_read_data(resourceHandle,
                                            &rect,
                                            dmxImagePtr,
                                            dmxPitch);


    if (result != 0)
    {
        vc_dispmanx_resource_delete(resourceHandle);
        vc_dispmanx_display_close(displayHandle);

        fprintf(stderr,
                "%s: vc_dispmanx_resource_read_data() failed\n",
                program);

        exit(EXIT_FAILURE);
    }

    vc_dispmanx_resource_delete(resourceHandle);
    vc_dispmanx_display_close(displayHandle);

    //-------------------------------------------------------------------
    // Convert from RGBA (32 bit) to RGB (24 bit)

    int32_t pngBytesPerPixel = 3;
    int32_t pngPitch = pngBytesPerPixel * pngWidth;
    void *pngImagePtr = malloc(pngPitch * pngHeight);

    int32_t j = 0;
    for (j = 0 ; j < pngHeight ; j++)
    {
        int32_t dmxXoffset = 0;
        int32_t dmxYoffset = 0;

        switch (displayRotated & 3)
        {
        case 0: // 0 degrees

            if (displayRotated & 0x20000) // flip vertical
            {
                dmxYoffset = (dmxHeight - j - 1) * dmxPitch;
            }
            else
            {
                dmxYoffset = j * dmxPitch;
            }

            break;

        case 1: // 90 degrees


            if (displayRotated & 0x20000) // flip vertical
            {
                dmxXoffset = j * dmxBytesPerPixel;
            }
            else
            {
                dmxXoffset = (dmxWidth - j - 1) * dmxBytesPerPixel;
            }

            break;

        case 2: // 180 degrees

            if (displayRotated & 0x20000) // flip vertical
            {
                dmxYoffset = j * dmxPitch;
            }
            else
            {
                dmxYoffset = (dmxHeight - j - 1) * dmxPitch;
            }

            break;

        case 3: // 270 degrees

            if (displayRotated & 0x20000) // flip vertical
            {
                dmxXoffset = (dmxWidth - j - 1) * dmxBytesPerPixel;
            }
            else
            {
                dmxXoffset = j * dmxBytesPerPixel;
            }

            break;
        }

        int32_t i = 0;
        for (i = 0 ; i < pngWidth ; i++)
        {
            uint8_t *pngPixelPtr = static_cast<uint8_t*>(pngImagePtr)
                                 + (i * pngBytesPerPixel)
                                 + (j * pngPitch);

            switch (displayRotated & 3)
            {
            case 0: // 0 degrees

                if (displayRotated & 0x10000) // flip horizontal
                {
                    dmxXoffset = (dmxWidth - i - 1) * dmxBytesPerPixel;
                }
                else
                {
                    dmxXoffset = i * dmxBytesPerPixel;
                }

                break;

            case 1: // 90 degrees

                if (displayRotated & 0x10000) // flip horizontal
                {
                    dmxYoffset = (dmxHeight - i - 1) * dmxPitch;
                }
                else
                {
                    dmxYoffset = i * dmxPitch;
                }

                break;

            case 2: // 180 degrees

                if (displayRotated & 0x10000) // flip horizontal
                {
                    dmxXoffset = i * dmxBytesPerPixel;
                }
                else
                {
                    dmxXoffset = (dmxWidth - i - 1) * dmxBytesPerPixel;
                }

                break;

            case 3: // 270 degrees

                if (displayRotated & 0x10000) // flip horizontal
                {
                    dmxYoffset = i * dmxPitch;
                }
                else
                {
                    dmxYoffset = (dmxHeight - i - 1) * dmxPitch;
                }

                break;
            }

            uint8_t *dmxPixelPtr = static_cast<uint8_t*>(dmxImagePtr) + dmxXoffset + dmxYoffset;

            memcpy(pngPixelPtr, dmxPixelPtr, 3);
        }
    }

    free(dmxImagePtr);
    dmxImagePtr = NULL;

    //-------------------------------------------------------------------

    png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                 NULL,
                                                 NULL,
                                                 NULL);

    if (pngPtr == NULL)
    {
        fprintf(stderr,
                "%s: unable to allocated PNG write structure\n",
                program);

        exit(EXIT_FAILURE);
    }

    png_infop infoPtr = png_create_info_struct(pngPtr);

    if (infoPtr == NULL)
    {
        fprintf(stderr,
                "%s: unable to allocated PNG info structure\n",
                program);

        exit(EXIT_FAILURE);
    }

    if (setjmp(png_jmpbuf(pngPtr)))
    {
        fprintf(stderr, "%s: unable to create PNG\n", program);
        exit(EXIT_FAILURE);
    }

    FILE *pngfp = NULL;

    if (writeToStdout)
    {
        pngfp = stdout;
    }
    else
    {
        pngfp = fopen(pngName, "wb");

        if (pngfp == NULL)
        {
            fprintf(stderr,
                    "%s: unable to create %s - %s\n",
                    program,
                    pngName,
                    strerror(errno));

            exit(EXIT_FAILURE);
        }
    }

    png_init_io(pngPtr, pngfp);

    int png_color_type = PNG_COLOR_TYPE_RGB;

    png_set_IHDR(
        pngPtr,
        infoPtr,
        pngWidth,
        pngHeight,
        8,
        png_color_type,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE);

    png_write_info(pngPtr, infoPtr);

    int y = 0;
    for (y = 0; y < pngHeight; y++)
    {
        png_write_row(pngPtr, static_cast<uint8_t*>(pngImagePtr) + (pngPitch * y));
    }

    png_write_end(pngPtr, NULL);
    png_destroy_write_struct(&pngPtr, &infoPtr);

    if (pngfp != stdout)
    {
        fclose(pngfp);
    }

    //-------------------------------------------------------------------

    free(pngImagePtr);
    pngImagePtr = NULL;

    return 0;
        
    }
    
    
}