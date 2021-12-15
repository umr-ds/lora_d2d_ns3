/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This is an example script for lora protocol.
 *
 * Authors: Pavel Boyko <boyko@iitp.ru>
 *          To Thanh Hai <tthhai@gmail.com>
 *
 */

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

class ISCRAMD2D
{
public:
    ISCRAMD2D();
    void Configure(int argc, char **argv);
    void Run();
    bool RxPacket(Ptr<NetDevice> dev, Ptr<const Packet> pkt, uint16_t mode, const Address &sender);
    void SendPacket(Ptr<LoraNetDevice> dev, uint32_t mode);
    Ptr<LoraNetDevice> CreateNode(Vector pos, Ptr<LoraChannel> chan);
    std::vector<std::tuple<int, int>> LocationDistribution();
    void Simulate();

private:
    // parameters
    /// Number of nodes
    uint32_t nodes;
    /// Size of the field
    uint32_t area;
    /// Center frequency in Hz
    uint32_t freq;
    /// Bits per second
    uint32_t bps;
    /// Symbols per second
    uint32_t sps;
    /// Bandwidth in Hz
    uint32_t bw;
    /// Size of the payload in bytes
    uint32_t p_size;
    /// Total simulation time in seconds
    double totalTime;
    /// How many packets a node should send
    uint16_t messagesPerNode;
    /// Iterations per configuration
    uint16_t iterations;
    /// Seed for random number generator
    uint16_t seed;

    /// Random number generator
    Ptr<NormalRandomVariable> randomAreaDistribution;

    ObjectFactory m_phyFac;
    uint32_t receivedBytes;
};

int main(int argc, char **argv)
{
    ISCRAMD2D test;

    test.Configure(argc, argv);

    test.Run();

    return 0;
}

ISCRAMD2D::ISCRAMD2D() : nodes(100),
                         area(5),
                         freq(433000),
                         bps(50000),
                         sps(61),
                         bw(125),
                         p_size(50),
                         totalTime(120),
                         messagesPerNode(3),
                         iterations(1),
                         seed(35039)
{
}

void ISCRAMD2D::Configure(int argc, char **argv)
{
    CommandLine cmd;

    cmd.AddValue("nodes", "Number of nodes.", nodes);
    cmd.AddValue("area", "Size of the field", area);
    cmd.AddValue("freq", "Center frequency in Hz", freq);
    cmd.AddValue("bps", "Bits per second", bps);
    cmd.AddValue("sps", "Symbols per second", sps);
    cmd.AddValue("bw", "Bandwidth in Hz", bw);
    cmd.AddValue("payload_size", "Size of the payload in bytes", p_size);
    cmd.AddValue("sim_time", "Total simulation time", totalTime);
    cmd.AddValue("msg", "How many messages a node should send", messagesPerNode);
    cmd.AddValue("iter", "How often the given configuration should be executed", iterations);

    cmd.Parse(argc, argv);

    RngSeedManager::SetSeed(seed);

    area = area * 1000;

    randomAreaDistribution = CreateObject<NormalRandomVariable>();
    randomAreaDistribution->SetAttribute("Mean", DoubleValue(area / 2));
    randomAreaDistribution->SetAttribute("Variance", DoubleValue(area / 3));
}

void ISCRAMD2D::Run()
{
    // log all commandline arguments
    std::cout << "nodes: " << nodes
              << "; area: " << area
              << "; freq: " << freq
              << "; sps: " << sps
              << "; bw: " << bw
              << "; payload_size: " << p_size
              << "; sim_time: " << totalTime
              << "; msg: " << messagesPerNode
              << "; iter: " << iterations
              << std::endl;

    Simulate();
}

bool ISCRAMD2D::RxPacket(Ptr<NetDevice> dev, Ptr<const Packet> pkt, uint16_t mode, const Address &sender)
{
    receivedBytes += pkt->GetSize();
    return true;
}

void ISCRAMD2D::SendPacket(Ptr<LoraNetDevice> dev, uint32_t mode)
{
    Ptr<Packet> pkt = Create<Packet>(this->p_size);
    dev->Send(pkt, dev->GetBroadcast(), mode);
}

Ptr<LoraNetDevice>
ISCRAMD2D::CreateNode(Vector pos, Ptr<LoraChannel> chan)
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

std::vector<std::tuple<int, int>> ISCRAMD2D::LocationDistribution()
{
    std::vector<std::tuple<int, int>> positions = std::vector<std::tuple<int, int>>();

    for (uint32_t i = 0; i < nodes; i++)
    {
        int x = (int)randomAreaDistribution->GetValue();
        int y = (int)randomAreaDistribution->GetValue();

        positions.push_back(std::make_tuple(x, y));
    }

    return positions;
}

void ISCRAMD2D::Simulate()
{

    LoraModesList mList;
    LoraTxMode mode = LoraTxModeFactory::CreateMode(LoraTxMode::LORA,
                                                    this->bps,  // this->bps,  // Data rate in bps
                                                    this->sps,  // PHY rate in SPS
                                                    this->freq, // Center frequency
                                                    this->bw,   // Bandwidth
                                                    0,          // Constellation -> Not used here
                                                    "TestMode");
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

        std::vector<std::tuple<int, int>> positions = LocationDistribution();

        std::vector<Ptr<LoraNetDevice>> devices = std::vector<Ptr<LoraNetDevice>>();

        for (std::tuple<int, int> position : positions)
        {
            Ptr<LoraNetDevice> dev = CreateNode(Vector(std::get<0>(position), std::get<1>(position), 0), channel);
            dev->SetReceiveCallback(MakeCallback(&ISCRAMD2D::RxPacket, this));
            devices.push_back(dev);
        }

        Ptr<UniformRandomVariable> simuTimeRandomRange = CreateObject<UniformRandomVariable>();
        simuTimeRandomRange->SetAttribute("Min", DoubleValue(0.0));
        simuTimeRandomRange->SetAttribute("Max", DoubleValue(totalTime));

        // Every devices sends a packet after a random delay
        for (Ptr<LoraNetDevice> dev : devices)
        {
            for (int i = 0; i < messagesPerNode; i++)
            {
                double scheduled_time = simuTimeRandomRange->GetValue();
                dev->SetChannelMode(0);
                dev->SetTransmitStartTime(scheduled_time);
                Simulator::Schedule(Seconds(scheduled_time), &ISCRAMD2D::SendPacket, this, dev, 0);
            }
        }

        Simulator::Stop(Seconds(totalTime + 10));
        Simulator::Run();
    }

    Simulator::Destroy();
}
