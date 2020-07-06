// Copyright (c) 2019-2020 The Bigbang developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "delegateverify.h"

using namespace std;
using namespace xengine;

namespace bigbang
{
namespace delegate
{

//////////////////////////////
// CDelegateVerify

CDelegateVerify::CDelegateVerify(const map<CDestination, size_t>& mapWeight,
                                 const map<CDestination, vector<unsigned char>>& mapEnrollData)
{
    Enroll(mapWeight, mapEnrollData);
}

bool CDelegateVerify::VerifyProof(const vector<unsigned char>& vchProof, uint256& nAgreement,
                                  size_t& nWeight, map<CDestination, size_t>& mapBallot)
{
    uint256 nAgreementParse;
    try
    {
        unsigned char nWeightParse;
        vector<CDelegateData> vPublish;
        CIDataStream is(vchProof);
        is >> nWeightParse >> nAgreementParse;
        if (nWeightParse == 0 && nAgreementParse == 0)
        {
            return true;
        }
        is >> vPublish;
        StdLog("CCH", "VerifyProof: nWeightParse: %lu, nAgreementParse: %s, vPublish: %lu",
               nWeightParse, nAgreementParse.ToString().c_str(), vPublish.size());
        for (int i = 0; i < vPublish.size(); i++)
        {
            const CDelegateData& delegateData = vPublish[i];
            vector<unsigned char> vOutData;
            {
                CODataStream os(vOutData);
                os << delegateData;
            }
            //StdLog("CCH", "VerifyProof delegateData[%d]: %s", i, delegateData.ToString().c_str());
            StdLog("CCH", "VerifyProof delegateData[%d]: %s", i, ToHexString(vOutData).c_str());

            {
                StdLog("CCH", "nIdentFrom: %s", delegateData.nIdentFrom.GetHex().c_str());
                StdLog("CCH", "nR: %s", delegateData.nR.GetHex().c_str());
                StdLog("CCH", "nS: %s", delegateData.nS.GetHex().c_str());
                StdLog("CCH", "mapShare.size(): %lu", delegateData.mapShare.size());
                for (std::map<uint256, std::vector<uint256>>::const_iterator it = delegateData.mapShare.begin();
                     it != delegateData.mapShare.end(); ++it)
                {
                    StdLog("CCH", "mapShare: %s, size: %lu", (*it).first.GetHex().c_str(), (*it).second.size());
                    for (int i = 0; i < (*it).second.size(); i++)
                    {
                        StdLog("CCH", "mapShare uint256: %s", (*it).second[i].GetHex().c_str());
                    }
                }
            }

            if (!witness.IsCollectCompleted()
                && (!VerifySignature(delegateData) || !witness.Collect(delegateData.nIdentFrom, delegateData.mapShare)))
            {
                return false;
            }
        }
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }

    GetAgreement(nAgreement, nWeight, mapBallot);

    return (nAgreement == nAgreementParse);
}

} // namespace delegate
} // namespace bigbang
