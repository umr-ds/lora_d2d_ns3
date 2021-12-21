#include "ns3/lora-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"

#include "ns3/lora-net-device.h"
#include "ns3/lora-channel.h"
#include "ns3/lora-phy-gen.h"
#include "ns3/lora-transducer-hd.h"
#include "ns3/lora-prop-model-ideal.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/node.h"
#include "ns3/object-factory.h"
#include "ns3/pointer.h"
#include "ns3/callback.h"
#include "ns3/nstime.h"
#include "ns3/log.h"
#include "ns3/mac-lora-gw.h"

#include <iostream>
#include <cmath>
#include <random>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("LoRaD2D");

uint32_t nodes;                                  // Number of nodes
uint32_t area;                                   // Size of the field
uint32_t freq;                                   // Center frequency in Hz
uint32_t bps;                                    // Bits per second
uint32_t sps;                                    // Symbols per second
uint32_t bw;                                     // Bandwidth in Hz
uint32_t payloadSize;                            // Size of the payload in bytes
double totalSimulationTime;                      // Total simulation time in seconds
uint16_t msgPerNode;                             // How many packets a node should send
uint16_t iterations;                             // Iterations per configuration
uint16_t seed;                                   // Seed for random number generator
Ptr<NormalRandomVariable> randomAreaDistributio; // Random number generator

void LoRaD2DLog(std::string event = "",
                Address receiver = Address(),
                uint32_t packetSize = 0,
                uint64_t packetID = 0,
                Address sender = Address(),
                bool sendSuccess = false)
{

    std::ostringstream msg;
    msg << ns3::Now() << ","
        << event << ","

        // RX/TX Event
        << receiver << ","
        << packetSize << ","
        << packetID << ","
        << sender << ","
        << sendSuccess << ","

        << RngSeedManager::GetRun();

    NS_LOG_INFO(msg.str());
}

void Configure(int argc, char **argv)
{
    LogComponentEnable("LoRaD2D", LOG_LEVEL_ALL);

    // Set default values for global variables
    nodes = 50;
    area = 5000;
    freq = 433000;
    bps = 5468;
    sps = 61;
    bw = 125;
    payloadSize = 50;
    totalSimulationTime = 120.0;
    msgPerNode = 3;
    iterations = 1;
    seed = 35039;

    CommandLine cmd;
    cmd.AddValue("nodes", "Number of nodes.", nodes);
    cmd.AddValue("area", "Size of the field", area);
    cmd.AddValue("freq", "Center frequency in Hz", freq);
    cmd.AddValue("bps", "Bits per second", bps);
    cmd.AddValue("sps", "Symbols per second", sps);
    cmd.AddValue("bw", "Bandwidth in Hz", bw);
    cmd.AddValue("payload_size", "Size of the payload in bytes", payloadSize);
    cmd.AddValue("sim_time", "Total simulation time", totalSimulationTime);
    cmd.AddValue("msg", "How many messages a node should send", msgPerNode);
    cmd.AddValue("iter", "How often the given configuration should be executed", iterations);
    cmd.Parse(argc, argv);

    RngSeedManager::SetSeed(seed);

    randomAreaDistributio = CreateObject<NormalRandomVariable>();
    randomAreaDistributio->SetAttribute("Mean", DoubleValue(area / 2));
    randomAreaDistributio->SetAttribute("Variance", DoubleValue(area / 3));
}

bool RxPacket(Ptr<NetDevice> dev, Ptr<const Packet> pkt, uint16_t mode, const Address &sender)
{
    uint32_t packetSize = pkt->GetSize();
    uint64_t packetID = pkt->GetUid();

    LoRaD2DLog("RX",
               dev->GetAddress(),
               packetSize,
               packetID,
               sender,
               false);

    return true;
}

void TxPacket(Ptr<LoraNetDevice> dev, uint32_t mode)
{
    Ptr<Packet> pkt = Create<Packet>(payloadSize);
    bool success = dev->Send(pkt, dev->GetBroadcast(), mode);

    uint32_t packetSize = pkt->GetSize();
    uint64_t packetID = pkt->GetUid();

    LoRaD2DLog("TX",
               Address(),
               packetSize,
               packetID,
               dev->GetAddress(),
               success);
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

Ptr<LoraNetDevice> CreateNode(Vector pos, Ptr<LoraChannel> chan, ObjectFactory phyFac)
{
    Ptr<LoraPhy> phy = phyFac.Create<LoraPhy>();
    Ptr<Node> node = CreateObject<Node>();
    Ptr<LoraNetDevice> dev = CreateObject<LoraNetDevice>();
    Ptr<MacLoraAca> mac = CreateObject<MacLoraAca>();
    Ptr<ConstantPositionMobilityModel> mobility = CreateObject<ConstantPositionMobilityModel>();

    Ptr<LoraTransducerHd> trans = CreateObject<LoraTransducerHd>();

    mobility->SetPosition(pos);
    node->AggregateObject(mobility);
    mac->SetAddress(LoraAddress::Allocate());

    dev->SetPhy(phy);
    dev->SetMac(mac);
    dev->SetChannel(chan);
    dev->SetTransducer(trans);
    node->AddDevice(dev);

    std::ostringstream msg;
    msg << "Position: X="
        << pos.x << ", Y="
        << pos.y << ", Address="
        << dev->GetAddress();

    NS_LOG_INFO(msg.str());

    return dev;
}

void Simulate()
{

    LoraModesList modeList;
    LoraTxMode mode = LoraTxModeFactory::CreateMode(LoraTxMode::LORA,
                                                    bps,  // Data rate in bps
                                                    sps,  // PHY rate in SPS
                                                    freq, // Center frequency
                                                    bw,   // Bandwidth
                                                    0,    // Constellation -> Not used here
                                                    "default_mode");
    modeList.AppendMode(LoraTxMode(mode));

    Ptr<LoraPhyPerGenDefault> defaultPer = CreateObject<LoraPhyPerGenDefault>();
    Ptr<LoraPhyCalcSinrDefault> defaultSinr = CreateObject<LoraPhyCalcSinrDefault>();

    ObjectFactory phyFac;
    phyFac.SetTypeId("ns3::LoraPhyGen");
    phyFac.Set("PerModel", PointerValue(defaultPer));
    phyFac.Set("SinrModel", PointerValue(defaultSinr));
    phyFac.Set("SupportedModes", LoraModesListValue(modeList));

    Ptr<LoraPropModelThorp> prop = CreateObject<LoraPropModelThorp>();

    Ptr<LoraChannel> channel = CreateObject<LoraChannel>();
    channel->SetAttribute("PropagationModel", PointerValue(prop));

    for (uint16_t iteration = 0; iteration < iterations; iteration++)
    {

        uint16_t iterationSeed = seed + iteration;
        RngSeedManager::SetRun(iterationSeed);

        std::vector<std::tuple<int, int>> positions = LocationDistribution();

        std::vector<Ptr<LoraNetDevice>> devices = std::vector<Ptr<LoraNetDevice>>();

        for (std::tuple<int, int> position : positions)
        {
            Ptr<LoraNetDevice> dev = CreateNode(Vector(std::get<0>(position), std::get<1>(position), 0), channel, phyFac);
            dev->SetReceiveCallback(MakeCallback(&RxPacket));
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
                dev->SetChannelMode(0);
                dev->SetTransmitStartTime(scheduledTime);
                Simulator::Schedule(Seconds(scheduledTime), &TxPacket, dev, 0);
            }
        }

        Simulator::Stop(Seconds(totalSimulationTime + 10));
        Simulator::Run();
    }

    Simulator::Destroy();
}

int main(int argc, char **argv)
{
    Configure(argc, argv);

    NS_LOG_INFO("Simulation Time,"
                << "Event,"
                << "Receiver Address,"
                << "Packet Size,"
                << "Packet ID,"
                << "Sender Address,"
                << "Send Success State,"
                << "Current Seed");

    Simulate();

    return 0;
}
