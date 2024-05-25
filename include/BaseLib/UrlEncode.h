//
//  UrlEncode.h
//  BaseLib
//
//  Created by Zhou,Xuguang on 17/4/22.
//  Copyright © 2017年 Zhou,Xuguang. All rights reserved.
//

#include "PreCompile.h"

class BASELIB_API UrlEncode
{
public:
    static std::string Encode(const std::string &src);
    static std::string Decode(const std::string &src);
    
};
