//
//  CryptoHash.cpp
//  BaseLib
//
//  Created by Zhou,Xuguang on 17/5/6.
//  Copyright © 2017年 Zhou,Xuguang. All rights reserved.
//

#include "CryptoHash.h"
#include "md5/md5.h"

NS_BASELIB_BEGIN

const char HEX[16] = {
    '0', '1', '2', '3',
    '4', '5', '6', '7',
    '8', '9', 'a', 'b',
    'c', 'd', 'e', 'f'
};

std::string BytesToHexString(const unsigned char* input, size_t length) {
    
    std::string str;
    str.reserve(length << 1);
    for (size_t i = 0; i < length; ++i)
    {
        int t = input[i];
        int a = t / 16;
        int b = t % 16;
        str.append(1, HEX[a]);
        str.append(1, HEX[b]);
    }
    return str;
}

std::string MD5String(const std::string& str)
{
    struct MD5Context context;
    MD5Init(&context);
    MD5Update(&context, (const unsigned char *)str.c_str(), (unsigned int)str.size());
    unsigned char szDigest[16];
    memset(szDigest, 0, 16);
    MD5Final(szDigest, &context);
    return BytesToHexString(szDigest, 16);
}

NS_BASELIB_END
