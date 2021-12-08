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

using namespace ns3;

/**
 * \brief Test script.
 *
 * This script 10 lora end-devices, one gateway. Lora end-devices send packets on three channels.
 *
 */
class LoraExample
{
public:
    LoraExample();
    /**
     * \brief Configure script parameters
     * \param argc is the command line argument count
     * \param argv is the command line arguments
     * \return true on successful configuration
     */
    bool Configure(int argc, char **argv);
    /// Run simulation
    void Run();
    /**
     * Report results
     * \param os the output stream
     */
    void Report(std::ostream &os);

private:
    // parameters
    /// Number of nodes
    uint32_t size;
    /// Number of channels
    double totalChannel;
    /// Simulation time, seconds
    double totalTime;

    ObjectFactory m_phyFac;
    uint32_t m_bytesRx;

private:
    /// Create the nodes
    Ptr<LoraNetDevice> CreateNode(Vector pos, Ptr<LoraChannel> chan);
    Ptr<LoraNetDevice> CreateGateway(Vector pos, Ptr<LoraChannel> chan);

    bool DoExamples();

    uint32_t DoOneExample(Ptr<LoraPropModel> prop);

    bool RxPacket(Ptr<NetDevice> dev, Ptr<const Packet> pkt, uint16_t mode, const Address &sender);
    void SendOnePacket(Ptr<LoraNetDevice> dev, uint32_t mode);
};

int main(int argc, char **argv)
{
    LoraExample test;

    test.Run();
    return 0;
}

//-----------------------------------------------------------------------------
LoraExample::LoraExample() : size(10),
                             totalChannel(3),
                             totalTime(100)
{
}

bool LoraExample::Configure(int argc, char **argv)
{
    CommandLine cmd;

    cmd.AddValue("size", "Number of nodes.", size);
    cmd.AddValue("size", "Number of nodes.", totalChannel);
    cmd.AddValue("time", "Simulation time, s.", totalTime);

    cmd.Parse(argc, argv);
    return true;
}

void LoraExample::Run()
{
    std::cout << "Starting simulation for " << totalTime << " s ...\n";
    std::cout << "Creating " << size << " nodes ...\n";
    std::cout << "Transmission on " << totalChannel << " channels ...\n";

    DoExamples();
}

bool LoraExample::RxPacket(Ptr<NetDevice> dev, Ptr<const Packet> pkt, uint16_t mode, const Address &sender)
{
    m_bytesRx += pkt->GetSize();
    std::cout << dev->GetAddress() << "; " << pkt->GetSize() << "; " << mode << ";" << sender << "\n";
    return true;
}

void LoraExample::SendOnePacket(Ptr<LoraNetDevice> dev, uint32_t mode)
{
    Ptr<Packet> pkt = Create<Packet>(13);
    dev->Send(pkt, dev->GetBroadcast(), mode);
}

Ptr<LoraNetDevice>
LoraExample::CreateNode(Vector pos, Ptr<LoraChannel> chan)
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

uint32_t
LoraExample::DoOneExample(Ptr<LoraPropModel> prop)
{
    Ptr<LoraChannel> channel = CreateObject<LoraChannel>();
    channel->SetAttribute("PropagationModel", PointerValue(prop));

    Ptr<LoraNetDevice> d1 = CreateNode(Vector(10, 10, 10), channel);
    Ptr<LoraNetDevice> d2 = CreateNode(Vector(20, 20, 20), channel);

    d2->SetReceiveCallback(MakeCallback(&LoraExample::RxPacket, this));

    d1->SetChannelMode(0);
    d1->SetTransmitStartTime(5);
    Simulator::Schedule(Seconds(5), &LoraExample::SendOnePacket, this, d1, 0);

    m_bytesRx = 0;
    Simulator::Stop(Seconds(15.0));
    Simulator::Run();
    Simulator::Destroy();

    return m_bytesRx;
}

bool LoraExample::DoExamples()
{

    LoraModesList mList;
    LoraTxMode mode = LoraTxModeFactory::CreateMode(LoraTxMode::LORA,
                                                    50000,  // Data rate in bps
                                                    80,     // PHY rate in SPS -> TODO
                                                    383000, // Center frequency 383 kHz
                                                    125000, // Bandwidth 125 kHz
                                                    2,      // Constellation -> TODO
                                                    "TestMode");
    mList.AppendMode(LoraTxMode(mode));

    Ptr<LoraPhyPerGenDefault> perDef = CreateObject<LoraPhyPerGenDefault>();
    Ptr<LoraPhyCalcSinrDefault> sinrDef = CreateObject<LoraPhyCalcSinrDefault>();
    m_phyFac.SetTypeId("ns3::LoraPhyGen");
    m_phyFac.Set("PerModel", PointerValue(perDef));
    m_phyFac.Set("SinrModel", PointerValue(sinrDef));
    m_phyFac.Set("SupportedModes", LoraModesListValue(mList));

    Ptr<LoraPropModelThorp> prop = CreateObject<LoraPropModelThorp>();

    uint32_t n_ReceivedPacket = DoOneExample(prop);

    std::cout << n_ReceivedPacket / 13 << " packets received, "
              << (1 - (n_ReceivedPacket / 13) / (size)) * 100 << "\% packets loss.\n";

    return false;
}
