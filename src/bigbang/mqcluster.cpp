// Copyright (c) 2019-2020 The Bigbang developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "mqcluster.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread> // For sleep

#include "async_client.h"

using namespace std;

namespace bigbang
{

CMQCluster::CMQCluster(int catNodeIn)
  : thrMqttClient("mqttcli", boost::bind(&CMQCluster::MqttThreadFunc, this)),
    pCoreProtocol(nullptr),
    pBlockChain(nullptr),
    pService(nullptr),
    fAuth(false),
    fAbort(false),
    srvAddr("tcp://localhost:1883")
{
    switch (catNodeIn)
    {
    case 0:
        catNode = NODE_CATEGORY::BBCNODE;
        break;
    case 1:
        catNode = NODE_CATEGORY::FORKNODE;
        break;
    case 2:
        catNode = NODE_CATEGORY::DPOSNODE;
        break;
    }
}

bool CMQCluster::IsAuthenticated()
{
    return fAuth;
}

bool CMQCluster::HandleInitialize()
{
    if (NODE_CATEGORY::FORKNODE == catNode)
    {
        clientID = "FORKNODE-01";
    }
    else if (NODE_CATEGORY::DPOSNODE == catNode)
    {
        clientID = "DPOSNODE";
    }
    else if (NODE_CATEGORY::BBCNODE == catNode)
    {
        Log("CMQCluster::HandleInitialize(): bbc node so go passby");
        return true;
    }

    if (!GetObject("coreprotocol", pCoreProtocol))
    {
        Error("Failed to request coreprotocol");
        return false;
    }

    if (!GetObject("blockchain", pBlockChain))
    {
        Error("Failed to request blockchain");
        return false;
    }

    if (!GetObject("service", pService))
    {
        Error("Failed to request service");
        return false;
    }

    Log("CMQCluster::HandleInitialize() successfully");
    return true;
}

void CMQCluster::HandleDeinitialize()
{
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
    pService = nullptr;
}

bool CMQCluster::HandleInvoke()
{
    if (NODE_CATEGORY::BBCNODE == catNode)
    {
        Log("CMQCluster::HandleInvoke(): bbc node so go passby");
        return true;
    }

    if (!pBlockChain->ListForkNode(vForkNode))
    {
        Log("CMQCluster::HandleInvoke(): list fork node failed");
        return false;
    }

    if (!ThreadStart(thrMqttClient))
    {
        return false;
    }
    return IIOModule::HandleInvoke();
}

void CMQCluster::HandleHalt()
{
    if (NODE_CATEGORY::BBCNODE == catNode)
    {
        Log("CMQCluster::HandleHalt(): bbc node so go passby");
        return;
    }

    IIOModule::HandleHalt();

    condMQ.notify_all();
    if (thrMqttClient.IsRunning())
    {
        thrMqttClient.Interrupt();
    }
    ThreadExit(thrMqttClient);
}

bool CMQCluster::HandleEvent(CEventMQSyncBlock& eventMqSyncBlock)
{
    return true;
}

bool CMQCluster::HandleEvent(CEventMQUpdateBlock& eventMqUpdateBlock)
{
    return true;
}

bool CMQCluster::HandleEvent(CEventMQAgreement& eventMqAgreement)
{
    return true;
}

class callback : public virtual mqtt::callback
{
public:
    void connected(const string& cause) override
    {
        cout << "\nConnection connected" << endl;
        if (!cause.empty())
            cout << "\tcause: " << cause << endl;
    }

    void connection_lost(const string& cause) override
    {
        cout << "\nConnection lost" << endl;
        if (!cause.empty())
            cout << "\tcause: " << cause << endl;
    }

    void delivery_complete(mqtt::delivery_token_ptr tok) override
    {
        cout << "\tDelivery complete for token: "
             << (tok ? tok->get_message_id() : -1) << endl;
    }
};

class action_listener : public virtual mqtt::iaction_listener
{
protected:
    void on_failure(const mqtt::token& tok) override
    {
        cout << "\tListener failure for token: "
             << tok.get_message_id() << endl;
    }

    void on_success(const mqtt::token& tok) override
    {
        cout << "\tListener success for token: "
             << tok.get_message_id() << endl;
    }
};

class delivery_action_listener : public action_listener
{
    atomic<bool> done_;

    void on_failure(const mqtt::token& tok) override
    {
        action_listener::on_failure(tok);
        done_ = true;
    }

    void on_success(const mqtt::token& tok) override
    {
        action_listener::on_success(tok);
        done_ = true;
    }

public:
    delivery_action_listener()
      : done_(false) {}
    bool is_done() const
    {
        return done_;
    }
};

bool CMQCluster::ClientAgent(MQ_CLI_ACTION action)
{
    const string TOPIC{ "Cluster01/dpos/SyncBlockReq" };
    const char* PAYLOAD = "Request for secure main block chain for BBC.";

    static mqtt::async_client client(srvAddr, clientID);

    static callback cb;

    try
    {
        static mqtt::token_ptr conntok;
        switch (action)
        {
        case MQ_CLI_ACTION::CONN:
        {
            cout << "Initializing for server '" << srvAddr << "'..." << endl;
            client.set_callback(cb);
            cout << "  ...OK" << endl;

            cout << "\nConnecting..." << endl;
            conntok = client.connect();
            cout << "Waiting for the connection..." << endl;
            conntok->wait();
            cout << "  ...OK" << endl;
            break;
        }
        case MQ_CLI_ACTION::SUB:
        {
            break;
        }
        case MQ_CLI_ACTION::PUB:
        {
            for (int i = 0; i < 3; ++i)
            {
                cout << "\nSending message" << to_string(i) << "..." << endl;
                delivery_action_listener deliveryListener;
                mqtt::message_ptr pubmsg = mqtt::make_message(TOPIC, PAYLOAD);
                client.publish(pubmsg, nullptr, deliveryListener);

                while (!deliveryListener.is_done())
                {
                    this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                cout << "OK" << endl;
            }
            break;
        }
        case MQ_CLI_ACTION::DISCONN:
        {
            // Double check that there are no pending tokens

            auto toks = client.get_pending_delivery_tokens();
            if (!toks.empty())
                cout << "Error: There are pending delivery tokens!" << endl;

            // Disconnect
            cout << "\nDisconnecting..." << endl;
            conntok = client.disconnect();
            conntok->wait();
            cout << "  ...OK" << endl;
            break;
        }
        }
    }
    catch (const mqtt::exception& exc)
    {
        cerr << exc.what() << endl;
        return false;
    }

    return true;
}

void CMQCluster::MqttThreadFunc()
{
    Log("entering thread function of MQTT");
    //establish connection
    ClientAgent(MQ_CLI_ACTION::CONN);

    //subscribe topics
    ClientAgent(MQ_CLI_ACTION::SUB);

    //publish topics
    while (!fAbort)
    {
        boost::system_time const timeout = boost::get_system_time()
                                           + boost::posix_time::seconds(30);
        {
            boost::unique_lock<boost::mutex> lock(mutex);
            while (!fAbort)
            {
                if (!condMQ.timed_wait(lock, timeout))
                {
                    break;
                }
            }
        }
        ClientAgent(MQ_CLI_ACTION::PUB);
        Log("thread function of MQTT: go through an iteration");
    }

    //disconnect to broker
    ClientAgent(MQ_CLI_ACTION::DISCONN);

    Log("exiting thread function of MQTT");
}

} // namespace bigbang
