// Copyright (c) 2019-2020 The Bigbang developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <base32.h>
#include "util.h"
#include <chrono>
#include "transaction.h"
#include <boost/algorithm/string.hpp>
//#include "address.h"
#include "stream/stream.h"

std::string strDataBase32;

extern "C" const char * GetAddr(const char*str_hex)
{
    std::string base32;
    bigbang::crypto::Base32Encode((const unsigned char*)(&str_hex[1]), base32);
    strDataBase32 =  std::string(1, '0' + str_hex[0]) + base32;
    return strDataBase32.c_str();
}

class CAddress : public CDestination
{
public:
    enum
    {
        ADDRESS_LEN = 57
    };

    CAddress(const CDestination& destIn = CDestination());
    CAddress(const std::string& strAddress);
    virtual ~CAddress() = default;

    virtual bool ParseString(const std::string& strAddress);
    virtual std::string ToString() const;
};

CAddress::CAddress(const CDestination& destIn)
  : CDestination(destIn)
{
}

CAddress::CAddress(const std::string& strAddress)
{
    ParseString(strAddress);
}

bool CAddress::ParseString(const std::string& strAddress)
{
    if (strAddress.size() != ADDRESS_LEN || strAddress[0] < '0' || strAddress[0] >= '0' + PREFIX_MAX)
    {
        return false;
    }
    if (!bigbang::crypto::Base32Decode(strAddress.substr(1), data.begin()))
    {
        return false;
    }
    prefix = strAddress[0] - '0';
    return true;
}

std::string CAddress::ToString() const
{
    std::string strDataBase32;
    bigbang::crypto::Base32Encode(data.begin(), strDataBase32);
    return (std::string(1, '0' + prefix) + strDataBase32);
}

//std::string str_inputs = "5e995d11322ec239debb8a3fc641c3c707338685ea72335b6cba9b762cd88cdb,1|5e995d1161d9d843d6b67c87a5eb558c4aeb0af8ecdc87b0538b6e5cf7d0a2fe,0";
extern "C" const char * create_tx(  const char *str_fork,
                                    const char *str_to,
                                    const char *str_vchdata,
                                    const char *str_inputs,
                                    uint64_t amount,uint64_t free,
                                    uint32_t ts)
{
    CTransaction txNew;
    txNew.SetNull();
    txNew.hashAnchor = uint256(str_fork);
    //uint256("5e995d11322ec239debb8a3fc641c3c707338685ea72335b6cba9b762cd88cdb");
    txNew.nType = CTransaction::TX_TOKEN;
    if (ts == 0)
    {
        txNew.nTimeStamp = std::chrono::system_clock::now().time_since_epoch().count();
    }
    else
    {
        txNew.nTimeStamp = ts;
    }
    txNew.nLockUntil = 0;
    txNew.sendTo = CAddress(str_to);
    //CAddress("1sg25akg16fn6x2dmrg5x6gny3wdswcgnz73y8c2x70pgzs45wecvqdjg");
    txNew.nAmount = amount;
    txNew.nTxFee = free;
    if (strlen(str_vchdata) > 0)
    {
        txNew.vchData = xengine::ParseHexString(str_vchdata);
    }
    std::vector<std::string> vecInputs;
    boost::split(vecInputs, str_inputs,boost::is_any_of("|"));
    for (int i = 0; i < vecInputs.size(); i++)
    {
        CTxIn obj;
        obj.prevout.hash = uint256(vecInputs[i].substr(0, 64));
        std::cout << obj.prevout.hash.GetHex() << std::endl;
        obj.prevout.n = 0;
        if (vecInputs[i][65] == '1')
        {
            obj.prevout.n = 1;
        }
        txNew.vInput.push_back(obj);
    }
    xengine::CBufStream ss;
    ss << txNew;
    static std::string ret_str;
    ret_str = xengine::ToHexString((const unsigned char*)ss.GetData(), ss.GetSize());
    return ret_str.c_str();
}