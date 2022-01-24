#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/node.h"
#include "ns3/object-factory.h"
#include "ns3/pointer.h"
#include "ns3/callback.h"
#include "ns3/nstime.h"
#include "ns3/log.h"
#include "ns3/lora-helper.h"
#include "ns3/lora-phy.h"

#include "ns3/end-device-lora-phy.h"
#include "ns3/command-line.h"

#include "ns3/gateway-lora-phy.h"
#include "ns3/end-device-lorawan-mac.h"
#include "ns3/gateway-lorawan-mac.h"
#include "ns3/mobility-helper.h"
#include "ns3/node-container.h"
#include "ns3/position-allocator.h"
#include "ns3/one-shot-sender-helper.h"

#include <algorithm>
#include <ctime>
#include <iostream>
#include <cmath>
#include <random>

using namespace ns3;
using namespace lorawan;
NS_LOG_COMPONENT_DEFINE("LoRaD2D");

uint32_t nodes;                                  // Number of nodes
uint32_t area;                                   // Size of the field
uint32_t freq;                                   // Center frequency in Hz
uint32_t bw;                                     // Bandwidth in Hz
uint32_t sf;                                     // Spreading factor
uint32_t cr;                                     // Coding rate
uint32_t payloadSize;                            // Size of the payload in bytes
double totalSimulationTime;                      // Total simulation time in seconds
uint16_t msgPerNode;                             // How many packets a node should send
uint16_t seed;                                   // Seed for random number generator
Ptr<NormalRandomVariable> randomAreaDistributio; // Random number generator

void LoRaD2DLog(std::string event = "",
                std::string receiver = "",
                uint64_t packetID = 0,
                std::string sender = "")
{

    std::ostringstream msg;
    msg << ns3::Now() << ","
        << event << ","
        << receiver << ","
        << packetID << ","
        << sender << ","
        << RngSeedManager::GetRun();

    NS_LOG_INFO(msg.str());
}

void Configure(int argc, char **argv)
{
    LogComponentEnable("LoRaD2D", LOG_LEVEL_ALL);

    // Set default values for global variables
    nodes = 50;
    area = 10000;
    freq = 433;
    sf = 7;
    cr = 1;
    bw = 125000;
    payloadSize = 50;
    totalSimulationTime = 120.0;
    msgPerNode = 1;
    seed = 35039;

    CommandLine cmd;
    cmd.AddValue("nodes", "Number of nodes.", nodes);
    cmd.AddValue("area", "Size of the field", area);
    cmd.AddValue("freq", "Center frequency in Hz", freq);
    cmd.AddValue("cr", "Coding rate", cr);
    cmd.AddValue("sf", "Spreading factor", sf);
    cmd.AddValue("bw", "Bandwidth in Hz", bw);
    cmd.AddValue("payload_size", "Size of the payload in bytes", payloadSize);
    cmd.AddValue("sim_time", "Total simulation time", totalSimulationTime);
    cmd.AddValue("msg", "How many messages a node should send", msgPerNode);
    cmd.AddValue("seed", "The seed for initializing the RNG", seed);
    cmd.Parse(argc, argv);

    RngSeedManager::SetSeed(seed);

    randomAreaDistributio = CreateObject<NormalRandomVariable>();
    randomAreaDistributio->SetAttribute("Mean", DoubleValue(area / 2));
    randomAreaDistributio->SetAttribute("Variance", DoubleValue(200 * area));
}

void RxPacket(Ptr<Packet const> pkt, uint32_t devId)
{
    if (pkt->GetSize() < 50)
    {
        return;
    }
    uint64_t packetID = pkt->GetUid();

    LoRaD2DLog("RX",
               std::to_string(devId),
               packetID,
               "");
}

void WrongStatePacket(Ptr<Packet const> pkt, uint32_t devId)
{
    if (pkt->GetSize() < 50)
    {
        return;
    }
    uint64_t packetID = pkt->GetUid();

    LoRaD2DLog("FW",
               std::to_string(devId),
               packetID,
               "");
}

void InterferencePacket(Ptr<Packet const> pkt, uint32_t devId)
{
    if (pkt->GetSize() < 50)
    {
        return;
    }
    uint64_t packetID = pkt->GetUid();

    LoRaD2DLog("FI",
               std::to_string(devId),
               packetID,
               "");
}

void SensitivPacket(Ptr<Packet const> pkt, uint32_t devId)
{
    if (pkt->GetSize() < 50)
    {
        return;
    }

    uint64_t packetID = pkt->GetUid();

    LoRaD2DLog("FS",
               std::to_string(devId),
               packetID,
               "");
}

void LogTxPacket(Ptr<Packet const> pkt, uint32_t devId)
{
    if (pkt->GetSize() < 50)
    {
        return;
    }

    uint64_t packetID = pkt->GetUid();

    LoRaD2DLog("TX",
               "",
               packetID,
               std::to_string(devId));
}

void TxPacket(Ptr<LoraNetDevice> dev)
{
    Ptr<Packet> pkt = Create<Packet>(payloadSize);

    LoraTxParameters txParams;
    txParams.sf = sf;
    txParams.codingRate = cr;
    txParams.bandwidthHz = bw;
    txParams.headerDisabled = 0;
    txParams.nPreamble = 8;
    txParams.crcEnabled = 0;
    txParams.lowDataRateOptimizationEnabled = false;

    dev->GetPhy()->Send(pkt, txParams, freq, 14);
}

std::vector<std::tuple<int, int>> LocationDistribution()
{
    std::vector<std::tuple<int, int>> positions = std::vector<std::tuple<int, int>>();

    for (uint32_t i = 0; i < nodes; i++)
    {
        int x = (int)randomAreaDistributio->GetValue();
        int y = (int)randomAreaDistributio->GetValue();

        positions.push_back(std::make_tuple(x, y));
    }

    return positions;
}

Ptr<LoraNetDevice> CreateNode(Vector pos, LoraPhyHelper phyHelper)
{

    Ptr<Node> node = CreateObject<Node>();

    Ptr<ConstantPositionMobilityModel> mobility = CreateObject<ConstantPositionMobilityModel>();
    mobility->SetPosition(pos);
    node->AggregateObject(mobility);

    Ptr<LoraNetDevice> device = CreateObject<LoraNetDevice>();

    Ptr<LoraPhy> phy = phyHelper.Create(node, device);
    device->SetPhy(phy);
    device->GetPhy()->GetObject<EndDeviceLoraPhy>()->SetSpreadingFactor(sf);
    device->GetPhy()->GetObject<EndDeviceLoraPhy>()->SetFrequency(freq);

    device->GetPhy()->GetObject<EndDeviceLoraPhy>()->SwitchToStandby();

    device->GetPhy()->TraceConnectWithoutContext("StartSending", MakeCallback(LogTxPacket));
    device->GetPhy()->TraceConnectWithoutContext("RXWrongState", MakeCallback(WrongStatePacket));
    device->GetPhy()->TraceConnectWithoutContext("ReceivedPacket", MakeCallback(RxPacket));
    device->GetPhy()->TraceConnectWithoutContext("LostPacketBecauseInterference", MakeCallback(InterferencePacket));
    device->GetPhy()->TraceConnectWithoutContext("LostPacketBecauseUnderSensitivity", MakeCallback(SensitivPacket));

    node->AddDevice(device);

    std::ostringstream msg;
    msg << "Position: X="
        << pos.x << ", Y="
        << pos.y << ", ID="
        << device->GetNode()->GetId();
    NS_LOG_INFO(msg.str());

    return device;
}

void Simulate()
{

    Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel>();
    loss->SetPathLossExponent(3.76);
    loss->SetReference(1, 7.7);
    Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel>();
    Ptr<LoraChannel> channel = CreateObject<LoraChannel>(loss, delay);

    LoraPhyHelper phyHelper = LoraPhyHelper();
    phyHelper.SetChannel(channel);
    phyHelper.SetDeviceType(LoraPhyHelper::ED);

    RngSeedManager::SetRun(seed);

    std::vector<std::tuple<int, int>> positions = LocationDistribution();

    std::vector<Ptr<LoraNetDevice>> devices = std::vector<Ptr<LoraNetDevice>>();

    for (std::tuple<int, int> position : positions)
    {
        Ptr<LoraNetDevice> dev = CreateNode(Vector(std::get<0>(position), std::get<1>(position), 0), phyHelper);
        devices.push_back(dev);
    }

    Ptr<UniformRandomVariable> simuTimeRandomRange = CreateObject<UniformRandomVariable>();
    simuTimeRandomRange->SetAttribute("Min", DoubleValue(0.0));
    simuTimeRandomRange->SetAttribute("Max", DoubleValue(totalSimulationTime));

    // Every devices sends a packet after a random delay
    for (Ptr<LoraNetDevice> dev : devices)
    {
        for (int i = 0; i < msgPerNode; i++)
        {
            double scheduledTime = simuTimeRandomRange->GetValue();
            Simulator::Schedule(Seconds(scheduledTime), &TxPacket, dev);
        }
    }

    Simulator::Stop(Seconds(totalSimulationTime + 10));
    Simulator::Run();
    Simulator::Destroy();
}

int main(int argc, char **argv)
{
    Configure(argc, argv);

    NS_LOG_INFO("Simulation Time,"
                << "Event,"
                << "Receiver ID,"
                << "Packet ID,"
                << "Sender ID,"
                << "Current Seed");

    Simulate();

    return 0;
}
