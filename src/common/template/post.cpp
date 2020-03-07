// Copyright (c) 2019-2020 The Bigbang developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "post.h"

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "../../bigbang/address.h"
#include "rpc/auto_protocol.h"
#include "stream/datastream.h"
#include "template.h"
#include "transaction.h"
#include "util.h"
#include "filecoin.h"

using namespace std;
using namespace xengine;
using namespace bigbang::crypto;

//////////////////////////////
// CTemplatePost

CTemplatePost::CTemplatePost()
  : CTemplate(TEMPLATE_POST)
{
    m_business = CDestination();
    m_customer = CDestination();
    m_height_begin = 0;
    m_height_cycle = 0;
    m_total = 0;
    m_price = 0;
    memset(m_post_base,0,sizeof(m_post_base));
    std::vector<uint8> v_post_data;
    v_post_data.assign(m_post_base,m_post_base + sizeof(m_post_base));
    vchData.clear();
    CODataStream os(vchData);
    os << m_business.prefix << m_business.data << m_customer.prefix << m_customer.data << m_height_begin << m_height_cycle << m_total << m_price << v_post_data;
    nId = CTemplateId(nType, bigbang::crypto::CryptoHash(vchData.data(), vchData.size()));

}

CTemplatePost::CTemplatePost(const std::vector<unsigned char>& vchDataIn)
  : CTemplate(TEMPLATE_POST)
{
    xengine::CIDataStream is(vchDataIn);
    std::vector<uint8> v_post_data;
    is >> m_business.prefix >> m_business.data >> m_customer.prefix >> m_customer.data >> m_height_begin >> m_height_cycle >> m_total >> m_price >> v_post_data;
    memcpy(m_post_base,v_post_data.data(),sizeof(m_post_base));
    vchData.assign(vchDataIn.begin(), vchDataIn.begin() + DataLen);
    nId = CTemplateId(nType, bigbang::crypto::CryptoHash(vchData.data(), vchData.size()));
}

CTemplatePost* CTemplatePost::clone() const
{
    return new CTemplatePost(*this);
}

void CTemplatePost::GetTemplateData(bigbang::rpc::CTemplateResponse& obj, CDestination&& destInstance) const
{
    obj.post.strBusiness = m_business.ToString();
    obj.post.strCustomer = m_customer.ToString();
    obj.post.nHeight_Begin = m_height_begin;
    obj.post.nHeight_Cycle = m_height_cycle;
    obj.post.nTotal = m_total;
    obj.post.nPrice = m_price;
    obj.post.strPost_Base = ToHexString( m_post_base,sizeof(m_post_base));
}

const CTemplatePostPtr CTemplatePost::CreateTemplatePtr(CTemplatePost* ptr)
{
    return boost::dynamic_pointer_cast<CTemplatePost>(CTemplate::CreateTemplatePtr(ptr));
}

bool CTemplatePost::ValidateParam() const
{
    if (m_height_cycle <= SafeHeight)
    {
        return false;
    }
    if (m_total % m_price != 0)
    {
        return false;
    }
    if (m_total == 0 || m_price == 0)
    {
        return false;
    }
    return true;
}

bool CTemplatePost::SetTemplateData(const vector<uint8>& vchDataIn)
{
    CIDataStream is(vchDataIn);
    try
    {
        std::vector<uint8> v_post_data;
        is >> m_business.prefix >> m_business.data >> m_customer.prefix >> m_customer.data >> m_height_begin >> m_height_cycle >> m_total >> m_price >> v_post_data;
        memcpy(m_post_base,v_post_data.data(),sizeof(m_post_base));
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CTemplatePost::SetTemplateData(const bigbang::rpc::CTemplateRequest& obj, CDestination&& destInstance)
{
    if (obj.strType != GetTypeName(TEMPLATE_POST))
    {
        return false;
    }
    const string& business = obj.post.strBusiness;
    const string& customer = obj.post.strCustomer;
    bigbang::CAddress addr_business(business);
    bigbang::CAddress addr_customer(customer);
    if (addr_business.IsNull() || addr_customer.IsNull())
    {
        return false;
    }
    m_business = addr_business;
    m_customer = addr_customer;
    m_height_begin = obj.post.nHeight_Begin;
    m_height_cycle = obj.post.nHeight_Cycle;
    m_price = obj.post.nPrice;
    m_total = obj.post.nTotal;
    std::vector<unsigned char> v_post_data = ParseHexString(obj.post.strPost_Base);
    if (v_post_data.size() != sizeof(m_post_base))
    {
        return false;
    }
    memcpy(m_post_base,v_post_data.data(),sizeof(m_post_base));
    return true;
}

void CTemplatePost::BuildTemplateData()
{
    vchData.clear();
    std::vector<uint8> v_post_data;
    v_post_data.assign(m_post_base,m_post_base + sizeof(m_post_base));
    CODataStream os(vchData);
    os << m_business.prefix << m_business.data << m_customer.prefix << m_customer.data << m_height_begin << m_height_cycle << m_total << m_price << v_post_data;
    
}

bool CTemplatePost::VerifyTxSignature(const uint256& hash, const uint256& hashAnchor, const CDestination& destTo,
                                          const vector<uint8>& vchSig, const int32 nForkHeight, bool& fCompleted) const
{
    uint32 height_max = SafeHeight + m_height_begin + ((m_total / m_price) + 1) * m_height_cycle;
    if (nForkHeight >= height_max)
    {
        if (m_customer.GetPubKey().Verify(hash, vchSig))
        {
            fCompleted = true;
            return true;
        }
    }
    else if (nForkHeight >= m_height_begin)
    {
        if (m_business.GetPubKey().Verify(hash, vchSig))
        {
            fCompleted = true;
            return true;
        }
    }
    else
    {
        return false;
    }
    return false;
}

bool CTemplatePost::GetSignDestination(const CTransaction& tx, const std::vector<uint8>& vchSig,
                                           std::set<CDestination>& setSubDest, std::vector<uint8>& vchSubSig) const
{
    int height;
    xengine::CIDataStream ds(tx.vchSig);
    try
    {
        ds >> height;
    }
    catch (const std::exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    setSubDest.clear();
    uint32 height_max = SafeHeight + m_height_begin + ((m_total / m_price) + 1) * m_height_cycle;
    if (height >= height_max)
    {
        setSubDest.insert(m_customer);
    }
    else if (height >= m_height_begin)
    {
        setSubDest.insert(m_business);
    }
    return true;
}

bool CTemplatePost::BuildTxSignature(const uint256& hash,
                                         const uint256& hashAnchor,
                                         const CDestination& destTo,
                                         const vector<uint8>& vchPreSig,
                                         vector<uint8>& vchSig) const
{
    vchSig.insert(vchSig.end(), vchPreSig.begin(), vchPreSig.end());
    return true;
}

bool CTemplatePost::VerifyTransaction(const CTransaction& tx,uint256 &block_hash,uint32 height,uint64 nValueIn)
{
    uint32 height_max = SafeHeight + m_height_begin + ((m_total / m_price) + 1) * m_height_cycle;
    if (height >= height_max)
    {
        return true;
    }
    if ((tx.nAmount + tx.nTxFee) != m_price)
    {
        return false;
    }
    if (tx.vInput.size() != 1)
    {
        return false;
    }
    if (tx.vchData.size() != 545)
    {
        return false;
    }
    //uint8_t bufrandomness[] = {9,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uint8_t *bufrandomness = (uint8_t*)(&(block_hash[0]));
    std::vector<uint8> result = tx.vchData;
    result.erase(result.begin() + 20);
    
    uint8 *bufproof = result.data();  
    uint8 *bufwinners = result.data() + 384; 
    
    const uint8_t *sectorIds = &m_post_base[0];
    const uint8_t *flattenedCommRs = &m_post_base[8];
    const uint8_t proverID[32] = {0};

	VerifyPoStResponse *resp = verify_post(
                    1024,
	                (const uint8_t (*)[32])bufrandomness,
	                2,
	                (const uint64_t *)sectorIds,
	                1,
	                (const uint8_t *)flattenedCommRs,
	                32,
	                (const uint8_t *)bufproof,
	                384,
	                (const FFICandidate *)bufwinners,
	                2,
	                (const uint8_t (*)[32])proverID);
    bool res = resp->is_valid;
    destroy_verify_post_response(resp);
    return res;
}