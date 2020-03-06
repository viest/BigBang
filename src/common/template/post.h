// Copyright (c) 2019-2020 The Bigbang developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_TEMPLATE_POST_H
#define COMMON_TEMPLATE_POST_H

#include "key.h"
#include "template.h"

class CTemplatePost;
typedef boost::shared_ptr<CTemplatePost> CTemplatePostPtr;

class CTemplatePost : virtual public CTemplate
{
public:
    static const CTemplatePostPtr CreateTemplatePtr(CTemplatePost* ptr);

public:
    CTemplatePost();

    CTemplatePost(const std::vector<unsigned char>& vchDataIn);

    virtual CTemplatePost* clone() const;
    virtual bool GetSignDestination(const CTransaction& tx, const std::vector<uint8>& vchSig,
                                    std::set<CDestination>& setSubDest, std::vector<uint8>& vchSubSig) const;
    virtual void GetTemplateData(bigbang::rpc::CTemplateResponse& obj, CDestination&& destInstance) const;

    virtual bool BuildTxSignature(const uint256& hash,
                                  const uint256& hashAnchor,
                                  const CDestination& destTo,
                                  const std::vector<uint8>& vchPreSig,
                                  std::vector<uint8>& vchSig) const;

    bool VerifySignature(const uint256& hash, const std::vector<uint8>& vchSig, int height, const uint256& fork);

    bool VerifyTransaction(const CTransaction& tx,
                            uint256 &block_hash,
                            uint32 height,
                            uint64 nValueIn);

protected:
    virtual bool ValidateParam() const;
    virtual bool SetTemplateData(const std::vector<uint8>& vchDataIn);
    virtual bool SetTemplateData(const bigbang::rpc::CTemplateRequest& obj, CDestination&& destInstance);
    virtual void BuildTemplateData();

    virtual bool VerifyTxSignature(const uint256& hash, const uint256& hashAnchor, const CDestination& destTo,
                                   const std::vector<uint8>& vchSig, const int32 nForkHeight, bool& fCompleted) const;

public:
    CDestination m_business;
    CDestination m_customer;
    uint32 m_height_begin;
    uint32 m_height_cycle;
    uint64 m_total;
    uint64 m_price;
    uint8 m_post_base[40];
    static const int DataLen = sizeof(m_business) + sizeof(m_customer) + sizeof(m_height_begin) + sizeof(m_height_cycle) + sizeof(m_total) + sizeof(m_price) + sizeof(m_post_base);
};

#endif