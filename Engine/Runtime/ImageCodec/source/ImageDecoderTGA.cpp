//
// Created by Zhou,Xuguang on 2018/10/27.
//

#include "ImageDecoderTGA.h"
#include "libtga/TGAlib.h"

NAMESPACE_IMAGECODEC_BEGIN

bool ImageDecoderTGA::onDecode(const void *buffer, size_t size, VImage *bitmap)
{
    tImageTGA *imageTGA = tgaLoadBuffer((unsigned char*)buffer, size);
    if (NULL == imageTGA)
    {
        return false;
    }
    
    //imageTGA->pixelDepth
    ImagePixelFormat format = FORMAT_UNKNOWN;
    if (imageTGA->pixelDepth == 24)
    {
        format = FORMAT_RGB8;
    }
    else if (imageTGA->pixelDepth == 32)
    {
        format = FORMAT_RGBA8;
    }
    
    bitmap->SetImageInfo(format, imageTGA->width, imageTGA->height, imageTGA->imageData, tgaDestroyImage);
    imageTGA->imageData = NULL;
    
    tgaDestroy(imageTGA);
    
    return true;
}

/**
 https://www.dca.fee.unicamp.br/~martino/disciplinas/ea978/tgaffs.pdf
 Byte 8-23 - Signature - Field 30
 This string is exactly 16 bytes long and is formatted exactly as shown below
 capital letters), with a hyphen between “TRUEVISION” and “XFILE.” If the
 signature is detected, the file is assumed to be of the New TGA format and MAY,
 therefore, contain the Developer Area and/or the Extension Area fields. If the
 signature is not found, then the file is assumed to be in the Original TGA format.
 TRUEVISION-XFILE
 Byte 24 - Reserved Character - Field 31
 Byte 24 is an ASCII character “.” (period). This character MUST BE a period or
 the file is not considered a proper TGA file.
 Byte 25 - Binary Zero String Terminator - Field 32
 Byte 25 is a binary zero which acts as a final terminator and allows the entire TGA
 File Footer to be read and utilized as a “C” string.
 */

bool ImageDecoderTGA::IsFormat(const void *buffer, size_t size)
{
    //按照标准，至少位36字节
    if (nullptr == buffer || size < 36)
    {
        return false;
    }
    
    char* terString = (char*)buffer + size - 18;
    
    char TRUEVISION[10] = {'T', 'R', 'U', 'E', 'V', 'I', 'S', 'I', 'O', 'N'};
    char XFILE[5] = {'X', 'F', 'I', 'L', 'E'};
    if (memcmp(TRUEVISION, terString, 10))
    {
        return false;
    }
    
    if (memcmp(XFILE, terString + 11, 5))
    {
        return false;
    }
    
    if (terString[16] != '.')
    {
        return false;
    }
    
    if (terString[17] != 0)
    {
        return false;
    }
    
    return true;
}

ImageStoreFormat ImageDecoderTGA::GetFormat() const
{
    return kTGA_Format;
}

ImageDecoderTGA* CreateTGADecoder()
{
    return new(std::nothrow) ImageDecoderTGA();
}

NAMESPACE_IMAGECODEC_END
