//
// Created by Zhou,Xuguang on 2018/10/27.
//

#ifndef RENDERENGINE_TGALIB_H
#define RENDERENGINE_TGALIB_H

enum
{
    TGA_OK,
    TGA_ERROR_FILE_OPEN,
    TGA_ERROR_READING_FILE,
    TGA_ERROR_INDEXED_COLOR,
    TGA_ERROR_MEMORY,
    TGA_ERROR_COMPRESSED_FILE,
};

/** TGA format */
typedef struct sImageTGA
{
    int status;
    unsigned char type;
    unsigned char pixelDepth;  //位深度

    /** map width */
    signed short width;

    /** map height */
    signed short height;

    /** raw data */
    unsigned char *imageData;
    int flipped;
} tImageTGA;

/// load the image header fields. We only keep those that matter!
bool tgaLoadHeader(unsigned char *buffer, unsigned long bufSize, tImageTGA *info);

/// loads the image pixels. You shouldn't call this function directly
bool tgaLoadImageData(unsigned char *buffer, unsigned long bufSize, tImageTGA *info);

/// this is the function to call when we want to load an image buffer.
tImageTGA* tgaLoadBuffer(unsigned char* buffer, long size);

// /converts RGB to grayscale
void tgaRGBtogreyscale(tImageTGA *info);

/// releases the memory used for the image
void tgaDestroy(tImageTGA *info);

void tgaDestroyImage(void *pImage);

void tgaDestroyHeader(tImageTGA *info);


#endif //RENDERENGINE_TGALIB_H
