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

uint32_t nodes;                                     // Number of nodes
uint32_t area;                                      // Size of the field
uint32_t freq;                                      // Center frequency in Hz
uint32_t bps;                                       // Bits per second
uint32_t sps;                                       // Symbols per second
uint32_t bw;                                        // Bandwidth in Hz
uint32_t p_size;                                    // Size of the payload in bytes
double total_simulation_time;                       // Total simulation time in seconds
uint16_t msg_per_node;                              // How many packets a node should send
uint16_t iterations;                                // Iterations per configuration
uint16_t seed;                                      // Seed for random number generator
Ptr<NormalRandomVariable> random_area_distribution; // Random number generator

ObjectFactory m_phyFac;

void configure(int argc, char **argv)
{
    LogComponentEnable("LoRaD2D", LOG_LEVEL_ALL);

    // Set default values for global variables
    nodes = 50;
    area = 5000;
    freq = 433000;
    bps = 5468;
    sps = 61;
    bw = 125;
    p_size = 50;
    total_simulation_time = 120.0;
    msg_per_node = 3;
    iterations = 1;
    seed = 35039;

    CommandLine cmd;
    cmd.AddValue("nodes", "Number of nodes.", nodes);
    cmd.AddValue("area", "Size of the field", area);
    cmd.AddValue("freq", "Center frequency in Hz", freq);
    cmd.AddValue("bps", "Bits per second", bps);
    cmd.AddValue("sps", "Symbols per second", sps);
    cmd.AddValue("bw", "Bandwidth in Hz", bw);
    cmd.AddValue("payload_size", "Size of the payload in bytes", p_size);
    cmd.AddValue("sim_time", "Total simulation time", total_simulation_time);
    cmd.AddValue("msg", "How many messages a node should send", msg_per_node);
    cmd.AddValue("iter", "How often the given configuration should be executed", iterations);
    cmd.Parse(argc, argv);

    RngSeedManager::SetSeed(seed);

    random_area_distribution = CreateObject<NormalRandomVariable>();
    random_area_distribution->SetAttribute("Mean", DoubleValue(area / 2));
    random_area_distribution->SetAttribute("Variance", DoubleValue(area / 3));
}

bool rx_packet(Ptr<NetDevice> dev, Ptr<const Packet> pkt, uint16_t mode, const Address &sender)
{
    uint32_t receivedBytes = pkt->GetSize();
    std::cout << dev->GetAddress() << " received " << receivedBytes << " bytes from " << sender << std::endl;
    return true;
}

void tx_packet(Ptr<LoraNetDevice> dev, uint32_t mode)
{
    Ptr<Packet> pkt = Create<Packet>(p_size);
    dev->Send(pkt, dev->GetBroadcast(), mode);
}

std::vector<std::tuple<int, int>> location_distribution()
{
    std::vector<std::tuple<int, int>> positions = std::vector<std::tuple<int, int>>();

    for (uint32_t i = 0; i < nodes; i++)
    {
        int x = (int)random_area_distribution->GetValue();
        int y = (int)random_area_distribution->GetValue();

        positions.push_back(std::make_tuple(x, y));
    }

    return positions;
}

Ptr<LoraNetDevice> create_node(Vector pos, Ptr<LoraChannel> chan)
{
    Ptr<LoraPhy> phy = m_phyFac.Create<LoraPhy>();
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

    return dev;
}

void simulate()
{

    LoraModesList mList;
    LoraTxMode mode = LoraTxModeFactory::CreateMode(LoraTxMode::LORA,
                                                    bps,  // Data rate in bps
                                                    sps,  // PHY rate in SPS
                                                    freq, // Center frequency
                                                    bw,   // Bandwidth
                                                    0,    // Constellation -> Not used here
                                                    "default_mode");
    mList.AppendMode(LoraTxMode(mode));

    Ptr<LoraPhyPerGenDefault> perDef = CreateObject<LoraPhyPerGenDefault>();
    Ptr<LoraPhyCalcSinrDefault> sinrDef = CreateObject<LoraPhyCalcSinrDefault>();
    m_phyFac.SetTypeId("ns3::LoraPhyGen");
    m_phyFac.Set("PerModel", PointerValue(perDef));
    m_phyFac.Set("SinrModel", PointerValue(sinrDef));
    m_phyFac.Set("SupportedModes", LoraModesListValue(mList));

    Ptr<LoraPropModelThorp> prop = CreateObject<LoraPropModelThorp>();

    Ptr<LoraChannel> channel = CreateObject<LoraChannel>();
    channel->SetAttribute("PropagationModel", PointerValue(prop));

    for (uint16_t iteration = 0; iteration < iterations; iteration++)
    {

        uint16_t iterationSeed = seed + iteration;
        RngSeedManager::SetRun(iterationSeed);

        std::vector<std::tuple<int, int>> positions = location_distribution();

        std::vector<Ptr<LoraNetDevice>> devices = std::vector<Ptr<LoraNetDevice>>();

        for (std::tuple<int, int> position : positions)
        {
            Ptr<LoraNetDevice> dev = create_node(Vector(std::get<0>(position), std::get<1>(position), 0), channel);
            dev->SetReceiveCallback(MakeCallback(&rx_packet));
            devices.push_back(dev);
        }

        Ptr<UniformRandomVariable> simuTimeRandomRange = CreateObject<UniformRandomVariable>();
        simuTimeRandomRange->SetAttribute("Min", DoubleValue(0.0));
        simuTimeRandomRange->SetAttribute("Max", DoubleValue(total_simulation_time));

        // Every devices sends a packet after a random delay
        for (Ptr<LoraNetDevice> dev : devices)
        {
            for (int i = 0; i < msg_per_node; i++)
            {
                double scheduled_time = simuTimeRandomRange->GetValue();
                dev->SetChannelMode(0);
                dev->SetTransmitStartTime(scheduled_time);
                Simulator::Schedule(Seconds(scheduled_time), &tx_packet, dev, 0);
            }
        }

        Simulator::Stop(Seconds(total_simulation_time + 10));
        Simulator::Run();
    }

    Simulator::Destroy();
}

int main(int argc, char **argv)
{
    configure(argc, argv);

    // log all commandline arguments
    std::cout << "nodes: " << nodes
              << "; area: " << area
              << "; freq: " << freq
              << "; sps: " << sps
              << "; bw: " << bw
              << "; payload_size: " << p_size
              << "; sim_time: " << total_simulation_time
              << "; msg: " << msg_per_node
              << "; iter: " << iterations
              << std::endl;

    simulate();

    return 0;
}
