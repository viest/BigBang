// Copyright(c) 2019 - 2020 The Bigbang developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "mpvss.h"

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <map>
#include <vector>

#include "address.h"
#include "crypto.h"
#include "delegateverify.h"
#include "destination.h"
#include "mpinterpolation.h"
#include "stream/datastream.h"
#include "test_big.h"
#include "util.h"

using namespace xengine;
using namespace bigbang;
using namespace bigbang::crypto;
using namespace std;
using namespace boost::filesystem;

BOOST_FIXTURE_TEST_SUITE(mpvss_tests, BasicUtfSetup)

void RandGeneretor(uint8_t* p)
{
    for (int i = 0; i < 32; i++)
    {
        *p++ = rand();
    }
}

void KeyGenerator(uint256& priv, uint256& pub)
{
    uint8_t md32[32];

    RandGeneretor(md32);
    md32[31] &= 0x0F;
    memcpy(priv.begin(), md32, 32);

    CEdwards25519 P;
    P.Generate(priv);
    P.Pack(pub.begin());
}

BOOST_AUTO_TEST_CASE(fp25519)
{
    srand(time(0));
    uint8_t md32[32];

    // add, minus
    for (int i = 0; i < 10; i++)
    {
        RandGeneretor(md32);
        CFP25519 add1(md32);
        RandGeneretor(md32);
        CFP25519 add2(md32);
        CFP25519 fpSum = add1 + add2;
        BOOST_CHECK(add1 == fpSum - add2);
        BOOST_CHECK(add2 == fpSum - add1);
    }

    // multiply, divide
    for (int i = 0; i < 10; i++)
    {
        RandGeneretor(md32);
        CFP25519 mul1(md32);
        RandGeneretor(md32);
        CFP25519 mul2(md32);
        CFP25519 fpProduct = mul1 * mul2;
        BOOST_CHECK(mul1 == fpProduct / mul2);
        BOOST_CHECK(mul2 == fpProduct / mul1);
    }

    // inverse
    for (int i = 0; i < 10; i++)
    {
        RandGeneretor(md32);
        CFP25519 fp(md32);
        CFP25519 fpInverse = fp.Inverse();
        BOOST_CHECK(CFP25519(1) == fp * fpInverse);
    }

    // sqrt square
    for (int i = 0; i < 10; i++)
    {
        RandGeneretor(md32);
        CFP25519 fp1(md32);
        CFP25519 fp2(md32);
        fp2 = fp2.Square().Sqrt();
        BOOST_CHECK(fp2.Square() == fp1.Square());
    }
}

BOOST_AUTO_TEST_CASE(sc25519)
{
    srand(time(0));
    uint8_t md32[32];

    // add, minus
    for (int i = 0; i < 10; i++)
    {
        RandGeneretor(md32);
        CSC25519 add1(md32);
        RandGeneretor(md32);
        CSC25519 add2(md32);
        CSC25519 scSum = add1 + add2;
        BOOST_CHECK(add1 == scSum - add2);
        BOOST_CHECK(add2 == scSum - add1);
    }

    // multiply
    for (int i = 0; i < 10; i++)
    {
        RandGeneretor(md32);
        CSC25519 sc(md32);
        RandGeneretor(md32);
        uint32_t n = 10;
        CSC25519 scProduct = sc * n;
        for (int i = 0; i < n - 1; i++)
        {
            scProduct -= sc;
        }
        BOOST_CHECK(scProduct == sc);
    }

    // negative
    for (int i = 0; i < 10; i++)
    {
        RandGeneretor(md32);
        CSC25519 sc(md32);
        CSC25519 scNegative1 = -sc;
        CSC25519 scNegative2(sc);
        scNegative2.Negative();
        BOOST_CHECK(scNegative1 == scNegative2);
        BOOST_CHECK(CSC25519() == sc + scNegative1);
    }
}

BOOST_AUTO_TEST_CASE(ed25519)
{
    srand(time(0));
    uint64_t md32[4] = { 0x23753fb0451ab230, 0xa03e286ea4e97588, 0x513ae37812fb6579, 0x687709b0bc557364 };
    CSC25519 k((uint8*)md32);
    CEdwards25519 P;
    P.Generate(k);
    P.Pack((uint8*)md32);
    cout << uint256(md32).ToString() << endl;

    // sign, verify
    // for (int i = 0; i < 10; i++)
    // {
    //     uint256 priv1, pub1;
    //     uint256 priv2, pub2;
    //     KeyGenerator(priv1, pub1);
    //     KeyGenerator(priv2, pub2);

    //     RandGeneretor(md32);
    //     CSC25519 hash(md32);

    //     CSC25519 sign = CSC25519(priv1.begin()) + CSC25519(priv2.begin()) * hash;

    //     CEdwards25519 P,R,S;
    //     P.Unpack(pub1.begin());
    //     R.Unpack(pub2.begin());
    //     S.Generate(sign);

    //     BOOST_CHECK( S == (P + R.ScalarMult(hash)) );
    // }
}

BOOST_AUTO_TEST_CASE(interpolation)
{
    srand(time(0));
    uint8_t md32[32];

    // lagrange, newton
    for (int i = 0; i < 1; i++)
    {
        vector<uint32_t> vX;
        for (int i = 1; i < 51; i++)
        {
            vX.push_back(i);
        }

        vector<pair<uint32_t, uint256>> vShare;
        for (int i = 0; i < 26; i++)
        {
            int index = rand() % vX.size();
            uint32_t x = vX[index];
            vX.erase(vX.begin() + index);

            RandGeneretor(md32);
            uint256 y((uint64_t*)md32);
            vShare.push_back(make_pair(x, y));
        }

        BOOST_CHECK(MPLagrange(vShare) == MPNewton(vShare));
    }
}

const uint256 GetHash(const map<uint256, vector<uint256>>& mapShare)
{
    vector<unsigned char> vch;
    xengine::CODataStream os(vch);
    os << mapShare;
    return CryptoHash(&vch[0], vch.size());
}

BOOST_AUTO_TEST_CASE(mpvss)
{
    srand(time(0));
    for (size_t count = 23; count <= 23; count++)
    {
        uint256 nInitValue;
        vector<uint256> vID;
        map<uint256, CMPSecretShare> mapSS;
        vector<CMPSealedBox> vSBox;
        vector<CMPCandidate> vCandidate;

        CMPSecretShare ssWitness;

        boost::posix_time::ptime t0;
        cout << "Test mpvss begin: count " << count << "\n{\n";
        vID.resize(count);
        vSBox.resize(count);
        vCandidate.resize(count);
        //Setup
        t0 = boost::posix_time::microsec_clock::universal_time();
        for (int i = 0; i < count; i++)
        {
            vID[i] = uint256(i + 1);
            mapSS[vID[i]] = CMPSecretShare(vID[i]);

            mapSS[vID[i]].Setup(count + 1, vSBox[i]);
            vCandidate[i] = CMPCandidate(vID[i], 1, vSBox[i]);

            nInitValue = nInitValue ^ mapSS[vID[i]].myBox.vCoeff[0];
        }
        cout << "\tSetup : " << ((boost::posix_time::microsec_clock::universal_time() - t0).ticks() / count) << "\n";
        cout << "\tInit value = " << nInitValue.GetHex() << "\n";
        {
            CMPSealedBox box;
            ssWitness.Setup(count + 1, box);
        }

        //Enroll
        t0 = boost::posix_time::microsec_clock::universal_time();
        for (int i = 0; i < count; i++)
        {
            mapSS[vID[i]].Enroll(vCandidate);
        }
        cout << "\tEnroll : " << ((boost::posix_time::microsec_clock::universal_time() - t0).ticks() / count) << "\n";
        ssWitness.Enroll(vCandidate);

        // Distribute
        t0 = boost::posix_time::microsec_clock::universal_time();
        for (int i = 0; i < count; i++)
        {
            map<uint256, vector<uint256>> mapShare;
            mapSS[vID[i]].Distribute(mapShare);

            uint256 hash, nR, nS;
            hash = GetHash(mapShare);
            mapSS[vID[i]].Signature(hash, nR, nS);

            for (int j = 0; j < count; j++)
            {
                if (i != j)
                {
                    // check sign
                    BOOST_CHECK(mapSS[vID[j]].VerifySignature(vID[i], hash, nR, nS));
                    // accept
                    BOOST_CHECK(mapSS[vID[j]].Accept(vID[i], mapShare[vID[j]]));
                }
            }
        }
        cout << "\tDistribute : " << ((boost::posix_time::microsec_clock::universal_time() - t0).ticks() / count) << "\n";

        // Publish
        t0 = boost::posix_time::microsec_clock::universal_time();
        vector<map<uint256, vector<uint256>>> vecMapShare;
        vecMapShare.resize(count);

        vector<tuple<uint256, uint256, uint256>> vecPublishSign;
        vecPublishSign.resize(count);
        for (int i = 0; i < count; i++)
        {
            mapSS[vID[i]].Publish(vecMapShare[i]);

            // sign
            uint256 hash, nR, nS;
            hash = GetHash(vecMapShare[i]);
            mapSS[vID[i]].Signature(hash, nR, nS);
            vecPublishSign[i] = make_tuple(hash, nR, nS);
        }

        for (int i = 0; i < count; i++)
        {
            for (int j = 0; j < count; j++)
            {
                int indexFrom = (i + j) % count;
                // check sign
                auto& sign = vecPublishSign[indexFrom];
                BOOST_CHECK(mapSS[vID[i]].VerifySignature(vID[indexFrom], get<0>(sign), get<1>(sign), get<2>(sign)));
                // collect
                BOOST_CHECK(mapSS[vID[i]].Collect(vID[indexFrom], vecMapShare[indexFrom]));
            }
            BOOST_CHECK(mapSS[vID[i]].IsCollectCompleted());
        }

        cout << "\tPublish : " << ((boost::posix_time::microsec_clock::universal_time() - t0).ticks() / count) << "\n";

        for (int i = 0; i < count && !ssWitness.IsCollectCompleted(); i++)
        {
            // check sign
            auto& sign = vecPublishSign[i];
            BOOST_CHECK(mapSS[vID[i]].VerifySignature(vID[i], get<0>(sign), get<1>(sign), get<2>(sign)));
            // collect
            BOOST_CHECK(ssWitness.Collect(vID[i], vecMapShare[i]));
        }
        BOOST_CHECK(ssWitness.IsCollectCompleted());

        // Reconstruct
        t0 = boost::posix_time::microsec_clock::universal_time();
        for (int i = 0; i < count; i++)
        {
            uint256 nRecValue;
            map<uint256, pair<uint256, size_t>> mapSecret;
            mapSS[vID[i]].Reconstruct(mapSecret);
            for (map<uint256, pair<uint256, size_t>>::iterator it = mapSecret.begin(); it != mapSecret.end(); ++it)
            {
                nRecValue = nRecValue ^ (*it).second.first;
            }
            BOOST_CHECK(nRecValue == nInitValue);
        }
        cout << "\tReconstruct : " << ((boost::posix_time::microsec_clock::universal_time() - t0).ticks() / count) << "\n";
        ;

        uint256 nRecValue;
        map<uint256, pair<uint256, size_t>> mapSecret;
        ssWitness.Reconstruct(mapSecret);
        for (map<uint256, pair<uint256, size_t>>::iterator it = mapSecret.begin(); it != mapSecret.end(); ++it)
        {
            nRecValue = nRecValue ^ (*it).second.first;
        }
        cout << "\tReconstruct witness : " << nRecValue.GetHex() << "\n";
        cout << "}\n";

        BOOST_CHECK(nRecValue == nInitValue);
    }
}

BOOST_AUTO_TEST_CASE(bug)
{
    path datadir = absolute(boost::unit_test::framework::master_test_suite().argv[0]).parent_path() / ".bigbang";
    remove_all(datadir);
    InitLog(datadir, true, false, 1, 1);

    map<CDestination, size_t> mapWeight = {
        { CAddress("20m024kcanmkw49ef4v4tvy607d4md196eyg3tz1apagk0tr24n1jzcnn"), 1 },
        { CAddress("20m0d0p9gaynwq6k815kj4n8w8ttydcf80m67sjcp3zsjymjfb7162307"), 1 },
        { CAddress("20m0997062yn5vzk5ahjmfkyt2bjq8s6dt5xqynjq71k0rxcrbfc94w9x"), 1 },
    };
    map<CDestination, vector<unsigned char>> mapEnrollData = {
        { CAddress("20m024kcanmkw49ef4v4tvy607d4md196eyg3tz1apagk0tr24n1jzcnn"), ParseHexString("cdf450987a7b27dd6b2cad0c5b74b30a60c61853bd6460dd9f7b5399c1f401a617000000000000007ee0ef69bbe31778a8092035e05c1bc185dadf80c02acaf18931bdc2cedb9294bf03637125489c4438f8006081e3d2591f34d51b1f9dba53da6d59188ce66563c570ee30089840924ae670e325b3bf68ee1fe9ac6e8bd13059efdce074041cca785f05c4a5bc60a92e821475e6bd9061448981ed3936fd6885f9de76835663987fb7c6aecacce17b9db7c41eb56202c4d132c8d60d4b1883d3243111337dfb231ebd7414f312c229145803f3c65061dded7eca3d38a54c9fde3873ee3849d304926fe9125cdbba6e857c73a8403368232186ecb31d0322601c4d3781c3b7daac73a70e9317c679ea347c52fdfc8e0a1b8273bc593bc7f4b153f46129b630ee86a9e0159ff3375df9f2a6cebd4ecc8acd2b5e73ad446bcaf8a80e6c4e25c56e80c52547758d994be5992dafc180ae472cc8e3a640439f1273fc3e801a1fa0d2e9d83cb0bea48abbed4d8aa652896ce9c4ac53b961f145259b5fb463f01979b8912f13cccac4c58bb6a113f393492fc823e48ff5fb48c3106593ab8b9757ae0c0b17ad8d75d211103d3ad7ad480668c3dac4923dc3c5ed2c485230a5ea8ac99012e3a85cb1a7dbc6d3ad0c2da37a11614e3f5104983c4d2a34907b9710301c05434a2dbd42f0da05cb45582d02409dd09559ad11407a579981d93b17e0fcdf4295ebb0a2568606a5c352c60c943d30a61ccfebd0a01c0badbbe220fb67f422c408810f6b2ff4edf77fbe756a1171e03d5dcfde72ec1402545ae28f3b485eac63d9b5e03772c78b1a21529022ff5fd2771cadf7ce920a1540f0fe56dceb54c7ad4c73b0a801fc071774dad4fb753a7bf69d7b64ce37493b265bb23fdb8e5f823264d232a49e3aeb755554f489a6841597bfabf56d3c8baad9329143f69d5ff38738aed0cb6f2cb889965ce5e3c8d9ceeef381abd6e1f42f5b8c9aa119325ccc3304c565613c20190630ae27646390ee373674c4223ba769d6d25df7b4df9fe5fee757b1acf5cfc2dd38dbc827097188d03e2ea995d7044a2b81b74fcafd6f40b4547e0a487e1278c18e6f43ad55af4ab29afe39a4ec53819952ca92c273aa3f14dfeac3416a37109c257a795a0582fb5137d9b7a66f657852932a542fb37370a406") },
        { CAddress("20m0d0p9gaynwq6k815kj4n8w8ttydcf80m67sjcp3zsjymjfb7162307"), ParseHexString("08d222fec83bddc62414085b39591805a05c777ba13c2ca8a47d8a85b9b5a5e31700000000000000a0799d24b1915d6ef309752be0fc15ddcfe4f14e5186ad8056b4f55333a3906ab35e019a40a404e711fd0ca041eca11166b1f90fda01cd54b9a2c5487c397c44e51a43adfeb6b4cdb3312797c67a4afd33b2bfa156f14a9691e6b635bb4cdd7ba4e6c58e0259fbf40fdf612463c3a981d93c88ac1b6df7ac06bf1404fc3b4d95fe01d3be6edebe3a83780c75a0e55c6ce11b8b29abe7b775a8279fdab0f523334a87060aa0d3664c523d3cd126425fdfdfc5a2b6c80583982072c4fdf9cde99280f520356e19ad2892c05416f27d8df4248f050e72c8f63cc93c33dfe655e37c25b66eee1ab6c4f52733d83b909d2f60dc4a79416b5a06d9d0178b3bdd2e9691d6f6cc117540ecce0058e5d5a264600eb8f74c1d7d61e0d601d355b496ef739047f173f43baf5a2961ca9573fd7e68bfc03a065ca60db0ee561c526d2338a51b8d031b1391940c304dbadee3ae142bf7025a356cf1875c888125f10910f07c410e82c80c77bffa3389150776d5e0d0e3a76139d5d6cde5669e31875ac4136edb9223e5fb59348dc0b338159c5d699664855d67ed88fb977c9794b7f0ea38fa2fbb7eec6e50d705540e2163af23303d7279123c88df9f6cb41b879e5eccab5c05e16d9c535fc90300d026ca79755ec9f353fc9c0cd2fcc052021dfd56e30b93e6150bb5429d41c1e7721a91394d7abb71b2a39ec74f9b9c039fd527b0fe726bdfcaedce1f49b82e9b98d13c66077be51b595c9f1068d14153327aa313f04e9d115ed5872506263f3516d6fded9c1d57dc23a4cd8991d19111facee48dfe430beb9febb08f1a532fac53b4213073279aa2ecaeb653057681aee54b3566ba9d652504a294615534d910167873770e46031811e26745f7c6ad330b30d9240b52f680853f416ae715a2ab4da5055698f6c41d4f092908c373d833b3f644b0c8449f5a56d7e3be898b1a9ab541a37e0a80f5e938290bc274354fee3bbbe24f8bfc98799b76960037a81a10cbfce6d7bb508041c28478675589042eefe3aaa90789baf99d3158ab3ed40ed2a385db8c87117d7a0a2955576659d403ebfbc2badf1ac2d23ce0812dee4a9be815c245d0a3d68f0d120268bf3484d2fa9c304bcea70ed003") },
        { CAddress("20m0997062yn5vzk5ahjmfkyt2bjq8s6dt5xqynjq71k0rxcrbfc94w9x"), ParseHexString("79a07b5104d66089783bd8b4c6ac71ca7677fe21d4f0562fd85f3e09087b82591700000000000000a00c0fdd0b8212a25bb04cd88d96c86e77c5d8834e37f2df5c8fe97933810a2420618dc234fd9df638480c9780b6e7c352af5b9733cb4961b85c9294b0df8892e845d69857a97059ca02db52e5fa3f7322d4be4c0e24292c838538f17b960ce015c7835b3feb77a2b8279f7da039bb63a28ba57d6a26a3a8016cba0e37fe39f9b62049920e6883894e74101cf3e7a91fb8dc1796903edbf54c4b6a34c59cb9414de8cafbe031651514d9ea0591f050eeaf5b07886b52ae19d7c680639046723728ab1c15272a03df9b3fead42df76a61779072c7f3872f2658b71b107eb23376b28f6b4827753ff4778f5137346329185f84456f3bf072ce4b9ff6765ade27daa7baaca72d95fabe2977b03c74e23df449ce3d7a699bdf5ec5c88be6ccb1dd77eebb9b77fbb617f9ad707a1f586b769c293784e083eb00b768ff8e6c12cd66ce2d8531aeca166d8920720efa8d381f9a7c5b5037f71bb2262a31c9d09724c4b062479f484710be19a9d1ce4a835dfb9a6c9ee7d69f9cef068d9b4bb0fa59c2e833d0a94cac6db2dd9ab64f279c0c990ed7d02ef73322642e147b21612b1f6533dac15c4a9801c84240184347791cf2db68ba4cb1aa98199822fbe2dee84b6a5e757bb7f0b77076092197c4c8154599b21f994e034fc8b5588b77eac1dcc6bf8e9c753c6e393c06b6d8cb54b6aa9f2eded90ffde5e8502b9e130988aada3a4a67b24afd86dfbc222465456174ef2ed7fb2349ac6f3061c81c908e47cc21aa9e4f0ca7b0f74d9f0231ef055093f119408ab06b0a9333b54f1d07c9af038ac53e388ddd6fb6d836adb066c7ce0b7e29061f651f34c14322181f888207c72f33ba448bba21b8e76c5cc23f78ea669c9d7fbef1f4ff673136fb0b02bf25a21204a73ecdcbc401e55a86351b10ec0eb19f071bf8b58d8513231087c6089c93db6e615173ef7dd215912136134b6096746ac5a6549f380594808fedca0d78fdf424e1df0a0de625f4a37d57776b2e201e5d76de95c017a71b579d240aeff7e94ebb2af7ea3a238bfb2f18bbbf23935bb7b5b88e9c687e34d09be7fd4bc21a10407be52e9bbfad77b9d4a753bfcb7b4618e48fc2fe4a651f13902d4190c1c39a32cc6807") },
    };

    // 0000dc9ff404ab18bf15535c771c10b53bf63aa850e422d2088c72509b10f221
    {
        delegate::CDelegateVerify verifier(mapWeight, mapEnrollData);

        vector<unsigned char> vchProof = ParseHexString("03072a3a94037dc3c34425e8b234411da426c67d118caecdda95b78f6669b41cb102000000000000000500949c0617aa5dfe65546547cfda12e57464cdd17b7f565738660c75985bd803000000000000000500224d8aad27c225cf26c9adf8c03b4946852677a03d7c2ab2a1306b0225430100000000000000847a0c978f12fd09dcee076e193b9e358718e5760bdfaeb4043882d870a4f6000500d0593057abcb9a68096722551c46b5e6b1e8050c7cc9961ff32f524f59c20100000000000000f3cff43210c8a38253f80309e657af3fed37612e548a5adcce474fdd58229b0d0500949c0617aa5dfe65546547cfda12e57464cdd17b7f565738660c75985bd80100000000000000c8388efe494cc20eeace00f25ff144a1fdc2c7c17df6228b3a0e64b5703ecf053e3b7743374b695086e58b70ef44abc9bae7fe5d49df0099916d77e8f5268ebfe45efd6b4925b4826c34801844ec079ea0a735cc3ed53f44815ba0fe9f9faa010500d0593057abcb9a68096722551c46b5e6b1e8050c7cc9961ff32f524f59c203000000000000000500224d8aad27c225cf26c9adf8c03b4946852677a03d7c2ab2a1306b0225430100000000000000dca3deb96c6e8d9efd08c5fe10739f1b0be98776b508af7c19710c1424f445030500d0593057abcb9a68096722551c46b5e6b1e8050c7cc9961ff32f524f59c2010000000000000010268575c00d679673ac18f8f80f6b1d6cffc893137328911ccc1c16ed4fb5090500949c0617aa5dfe65546547cfda12e57464cdd17b7f565738660c75985bd8010000000000000045777862e9f63295202378aa1d22641c2bf29785cb5a6c47d6e6c3e70c95ba049eab17d75b617ac20c0a5e2398a4ddeba806ddc63f92643e270fdf030df05f5102cdb8d45aebdd7da35f74ea98d340a72188644ae00b69057586b70e3d49cb03");
        map<CDestination, size_t> mapBallot;
        uint256 nAgreement;
        size_t nWeight;
        BOOST_CHECK(verifier.VerifyProof(vchProof, nAgreement, nWeight, mapBallot));
        cout << "nAgreement: " << nAgreement.ToString() << endl;
    }

    // 0000dc9fa0e2cbe71f659a789a3a517174a8ce52f6fe4cd88ee7313e4cd6f294
    {
        delegate::CDelegateVerify verifier(mapWeight, mapEnrollData);

        vector<unsigned char> vchProof = ParseHexString("03e728ab402171e64041aa0b26d7eac028cb66010649037af021e1810086a6949202000000000000000500d0593057abcb9a68096722551c46b5e6b1e8050c7cc9961ff32f524f59c203000000000000000500224d8aad27c225cf26c9adf8c03b4946852677a03d7c2ab2a1306b0225430100000000000000dca3deb96c6e8d9efd08c5fe10739f1b0be98776b508af7c19710c1424f445030500d0593057abcb9a68096722551c46b5e6b1e8050c7cc9961ff32f524f59c2010000000000000010268575c00d679673ac18f8f80f6b1d6cffc893137328911ccc1c16ed4fb5090500949c0617aa5dfe65546547cfda12e57464cdd17b7f565738660c75985bd8010000000000000045777862e9f63295202378aa1d22641c2bf29785cb5a6c47d6e6c3e70c95ba049eab17d75b617ac20c0a5e2398a4ddeba806ddc63f92643e270fdf030df05f5102cdb8d45aebdd7da35f74ea98d340a72188644ae00b69057586b70e3d49cb030500d0593057abcb9a68096722551c46b5e6b1e8050c7cc9961ff32f524f59c203000000000000000500224d8aad27c225cf26c9adf8c03b4946852677a03d7c2ab2a1306b0225430100000000000000dca3deb96c6e8d9efd08c5fe10739f1b0be98776b508af7c19710c1424f445030500d0593057abcb9a68096722551c46b5e6b1e8050c7cc9961ff32f524f59c2010000000000000010268575c00d679673ac18f8f80f6b1d6cffc893137328911ccc1c16ed4fb5090500949c0617aa5dfe65546547cfda12e57464cdd17b7f565738660c75985bd8010000000000000045777862e9f63295202378aa1d22641c2bf29785cb5a6c47d6e6c3e70c95ba04308e8f1ae1b0afdce88278d0167f320a58b469b8f89cc8bb19930d108fa0e368cdb2e23c5f90a319d743cd1dc8f4622164f6bd625639560ac978eb6b7f8d1f0c");
        map<CDestination, size_t> mapBallot;
        uint256 nAgreement;
        size_t nWeight;
        BOOST_CHECK(!verifier.VerifyProof(vchProof, nAgreement, nWeight, mapBallot));
        cout << "nAgreement: " << nAgreement.ToString() << endl;
    }

    remove_all(datadir);
}

BOOST_AUTO_TEST_SUITE_END()
