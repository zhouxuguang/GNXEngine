//
//  md5.hpp
//  BaseLib
//
//  Created by Zhou,Xuguang on 17/5/6.
//  Copyright © 2017年 Zhou,Xuguang. All rights reserved.
//

#ifndef md5_hpp
#define md5_hpp

#include "PreCompile.h"

NS_BASELIB_BEGIN

typedef unsigned int u32;
typedef unsigned char u8;

struct MD5Context {
    u32 buf[4];
    u32 bits[2];
    u8 in[64];
};

void MD5Init(struct MD5Context *ctx);

void MD5Update(struct MD5Context *ctx, unsigned char const *buf, unsigned len);

void MD5Final(unsigned char digest[16], struct MD5Context *ctx);

NS_BASELIB_END


#endif /* md5_hpp */
