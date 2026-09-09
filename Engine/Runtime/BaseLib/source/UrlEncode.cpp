//
//  UrlEncode.cpp
//  BaseLib
//
//  Created by Zhou,Xuguang on 17/4/22.
//  Copyright © 2017年 Zhou,Xuguang. All rights reserved.
//

#include "UrlEncode.h"

static std::string HttpUrlDecode(const std::string& srcUrl);
static std::string HttpUrlEncode(const std::string& srcUrl);
static int hexchar2int(char c);
static std::string UrlEncodeFormat(const unsigned char cValue);
static std::string DecimalToHexString(unsigned int nValue);

std::string UrlEncode::Encode(const std::string &src)
{
    return HttpUrlEncode(src);
}

std::string UrlEncode::Decode(const std::string &src)
{
    return HttpUrlDecode(src);
}

//
std::string HttpUrlDecode(const std::string& srcUrl)
{
    std::string   desStr;
    int length = 0;
    int flag      =1;
    int firstNum  = 0;
    int secondNum = 0;
    const char * pchar = srcUrl.c_str();
    while(length < srcUrl.length())
    {
        if(pchar[length]=='%')
        {
            //最后一位,need break;
            if (srcUrl.length() - length < 3)
            {
                return srcUrl;
            }
            //正常移位
            length++;	firstNum = hexchar2int(pchar[length]);
            length++;	secondNum = hexchar2int(pchar[length]);
            if (firstNum == -1 || secondNum == -1)//判断字符转换成的整数是否有效
            {
                flag = 0;
                break;
            }
            desStr += static_cast<char>((firstNum << 4) | secondNum);
        }
        else if(pchar[length]=='+')
        {
            desStr +=' ';//.append(' ');//spaceb
        }
        //不是特殊字符的urlcode了，即英文字符，直接append到string即可
        else
        {
            desStr +=pchar[length];// .append(pchar[length]);
        }
        length ++;
    }
    if(flag == 0)
    {
        return srcUrl;
    }
    return desStr;
}

std::string HttpUrlEncode(const std::string& srcUrl)
{
    std::string encodedUrl="";
    int length = srcUrl.length();
    if (length == 0)
    {
        return encodedUrl;
    }
    
    std::string sDontEncode = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_~.";//
    
    // Parse a the chars in the url
    for (int i=0; i<length; i++)
    {
        
        char cToFind = srcUrl.at(i);
        
        if (-1 == sDontEncode.find(cToFind,0))
        {
            // Char not found encode it.
            std::string tmp = UrlEncodeFormat(cToFind);
            encodedUrl.append(tmp);
        }
        else if(cToFind == ' ')
        {
            encodedUrl += '+';//.append('+');
        }
        else
        {
            encodedUrl += cToFind;//.append(&cToFind,1);
        }
    }
    return encodedUrl;
}

std::string UrlEncodeFormat(const unsigned char cValue)
{
    std::string tmp;
    tmp.append("%");
    unsigned int nDiv = cValue/16;
    unsigned int nMod = cValue%16;
    tmp.append(DecimalToHexString(nDiv));
    tmp.append(DecimalToHexString(nMod));
    return tmp;
}

std::string DecimalToHexString(unsigned int nValue)
{
    std::string tmp;
    if(nValue<10)
        tmp+= ((char)nValue +48);//append((int)nValue);
    else
    {
        switch(nValue)
        {
            case 10:tmp.append("A");break;
            case 11:tmp.append("B");break;
            case 12:tmp.append("C");break;
            case 13:tmp.append("D");break;
            case 14:tmp.append("E");break;
            case 15:tmp.append("F");break;
            default:break;
        }
    }
    return tmp;
}

/**
 *将hex字符转换成对应的整数
 *return 0~15：转换成功，-1:表示c 不是 hexchar
 */
int hexchar2int(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    else
        return -1;
}
