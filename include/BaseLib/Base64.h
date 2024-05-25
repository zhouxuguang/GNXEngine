/**
* @file              Base64.h
* @brief             Base64编码和解码
* @details           
* @author            zxg (zhouxuguang.com)
* @date              2015年月日
* @version           1.0.0.1
* @par               Copyright (c):2015
* @par               History:
*/
#ifndef __BASE64_H_BDD84F75_E636_45B7_BED2_1E027851A831__
#define __BASE64_H_BDD84F75_E636_45B7_BED2_1E027851A831__

#include "PreCompile.h"

NS_BASELIB_BEGIN

	/**
	* @brief base64_encode
	* @param[in] const char *
	* @param[in] unsigned int len
	* @return std::string
	* @author zxg
	* @date 2015年月日
	* @note 
	*/
	BASELIB_API std::string Base64_Encode(const  char* , unsigned int len);

	/**
	* @brief base64_decode
	* @param[in] const std::string & s
	* @return std::string 
	* @author zxg
	* @date 2015年月日
	* @note 
	*/
	BASELIB_API std::string Base64_Decode(const std::string& s);


NS_BASELIB_END

#endif
