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

NS_LOG_COMPONENT_DEFINE("ISCRAM");

class ISCRAMD2D
{
public:
    ISCRAMD2D();
    bool Configure(int argc, char **argv);
    void Run();
    bool RxPacket(Ptr<NetDevice> dev, Ptr<const Packet> pkt, uint16_t mode, const Address &sender);
    void SendPacket(Ptr<LoraNetDevice> dev, uint32_t mode);
    Ptr<LoraNetDevice> CreateNode(Vector pos, Ptr<LoraChannel> chan);
    std::vector<std::tuple<int, int>> LocationDistribution();
    bool Simulate();

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
                         messagesPerNode(3)
{
}

bool ISCRAMD2D::Configure(int argc, char **argv)
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

    cmd.Parse(argc, argv);
    return true;
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
              << std::endl;

    NS_LOG_DEBUG("Run");

    Simulate();
}

bool ISCRAMD2D::RxPacket(Ptr<NetDevice> dev, Ptr<const Packet> pkt, uint16_t mode, const Address &sender)
{
    receivedBytes += pkt->GetSize();
    std::cout << dev->GetAddress() << "; " << pkt->GetSize() << "; " << mode << ";" << sender << "\n";
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

    std::default_random_engine generator;
    generator.seed(35039);
    std::normal_distribution<double> distribution(area / 2, area / 3);

    for (uint32_t i = 0; i < nodes; i++)
    {
        int x = (int)distribution(generator);
        int y = (int)distribution(generator);
        positions.push_back(std::make_tuple(x, y));
    }

    return positions;
}

bool ISCRAMD2D::Simulate()
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

    std::vector<std::tuple<int, int>> positions = LocationDistribution();

    std::vector<Ptr<LoraNetDevice>> devices = std::vector<Ptr<LoraNetDevice>>();

    for (std::tuple<int, int> position : positions)
    {
        Ptr<LoraNetDevice> dev = CreateNode(Vector(std::get<0>(position), std::get<1>(position), 0), channel);
        dev->SetReceiveCallback(MakeCallback(&ISCRAMD2D::RxPacket, this));
        devices.push_back(dev);
    }

    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> uni(0, totalTime * 1000);

    // Every devices sends a packet after a random delay
    for (Ptr<LoraNetDevice> dev : devices)
    {
        for (int i = 0; i < messagesPerNode; i++)
        {
            int scheduled_time = uni(rng);
            float scheduled_time_s = (float)scheduled_time / 1000.0;
            dev->SetChannelMode(0);
            dev->SetTransmitStartTime(scheduled_time_s);
            Simulator::Schedule(Seconds(scheduled_time_s), &ISCRAMD2D::SendPacket, this, dev, 0);
        }
    }

    Simulator::Stop(Seconds(totalTime + 10));
    Simulator::Run();
    Simulator::Destroy();

    std::cout << nodes * messagesPerNode << " packets transmitted, "
              << receivedBytes / p_size << " packets received, "
              << (1 - (receivedBytes / p_size) / (nodes * messagesPerNode)) * 100 << "\% packets loss.\n";

    return false;
}
