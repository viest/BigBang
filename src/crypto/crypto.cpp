// Copyright (c) 2019-2020 The Bigbang developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <base32.h>

std::string strDataBase32;

extern "C" const char * GetAddr(const char*str_hex)
{
    std::string base32;
    bigbang::crypto::Base32Encode((const unsigned char*)(&str_hex[1]), base32);
    strDataBase32 =  std::string(1, '0' + str_hex[0]) + base32;
    return strDataBase32.c_str();
}