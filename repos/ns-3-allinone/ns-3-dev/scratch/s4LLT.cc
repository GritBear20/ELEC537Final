
#include "ns3/core-module.h"
#include "ns3/propagation-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;
 int waitingWindow = 16;
 int priorityWindow = 8;
/// Run single 10 seconds experiment with enabled or disabled RTS/CTS mechanism
void experiment (bool enableCtsRts,int simulationTime, int packetSize)
{
  // 0. Enable or disable CTS/RTS
  UintegerValue ctsThr = (enableCtsRts ? UintegerValue (100) : UintegerValue (2200));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", ctsThr);

  // 1. Create 3 nodes
  NodeContainer nodes;
  nodes.Create (4);

  // 2. Place nodes somehow, this is required by every wireless simulation
  for (size_t i = 0; i < 4; ++i)
    {
      nodes.Get (i)->AggregateObject (CreateObject<ConstantPositionMobilityModel> ());

    }
    nodes.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(0.0,0.0,0.0));
    nodes.Get(1)->GetObject<MobilityModel>()->SetPosition(Vector(100.0,0.0,0.0));
    nodes.Get(2)->GetObject<MobilityModel>()->SetPosition(Vector(150.0,0.0,0.0));
    nodes.Get(3)->GetObject<MobilityModel>()->SetPosition(Vector(250.0,0.0,0.0));

// 3. Create propagation loss matrix
  Ptr<MatrixPropagationLossModel> lossModel = CreateObject<MatrixPropagationLossModel> ();
  lossModel->SetDefaultLoss (200); // set default loss to 200 dB (no link)
  lossModel->SetLoss (nodes.Get (0)->GetObject<MobilityModel>(), nodes.Get (1)->GetObject<MobilityModel>(), 0); 
  lossModel->SetLoss (nodes.Get (0)->GetObject<MobilityModel>(), nodes.Get (2)->GetObject<MobilityModel>(), 0); 
  lossModel->SetLoss (nodes.Get (0)->GetObject<MobilityModel>(), nodes.Get (3)->GetObject<MobilityModel>(), 0); 
  lossModel->SetLoss (nodes.Get (1)->GetObject<MobilityModel>(), nodes.Get (2)->GetObject<MobilityModel>(), 0); 
 
  lossModel->SetLoss (nodes.Get (2)->GetObject<MobilityModel>(), nodes.Get (3)->GetObject<MobilityModel>(), 0); // set symmetric loss 2 <-> 1 to 50 dB
  
  // 4. Create & setup wifi channel
  Ptr<YansWifiChannel> wifiChannel = CreateObject <YansWifiChannel> ();
  wifiChannel->SetPropagationLossModel (lossModel);
  wifiChannel->SetPropagationDelayModel (CreateObject <ConstantSpeedPropagationDelayModel> ());



  // 5. Install wireless devices
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue ("DsssRate2Mbps"),
                                "ControlMode",StringValue ("DsssRate1Mbps"));
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  wifiPhy.SetChannel (wifiChannel);
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifiMac.SetType ("ns3::AdhocWifiMac"); // use ad-hoc MAC
  NetDeviceContainer devices = wifi.InstallLLT (wifiPhy, wifiMac, nodes, waitingWindow, priorityWindow);

  // uncomment the following to have athstats output
  // AthstatsHelper athstats;
  // athstats.EnableAthstats(enableCtsRts ? "basic-athstats-node" : "rtscts-athstats-node", nodes);

  // uncomment the following to have pcap output
  //wifiPhy.EnablePcap (enableCtsRts ? "basic-pcap-node" : "rtscts-pcap-node", nodes);


  // 6. Install TCP/IP stack & assign IP addresses
  InternetStackHelper internet;
  internet.Install (nodes);
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.0.0.0");
  ipv4.Assign (devices);

  // 7. Install applications: three CBR streams each saturating the channel
  ApplicationContainer cbrApps;
  uint16_t cbrPort = 12345;
  OnOffHelper onOffHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address ("10.0.0.1"), cbrPort));
  onOffHelper.SetAttribute ("PacketSize", UintegerValue (packetSize));
  onOffHelper.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));


  // flow 1:  node 1 -> node 0
  onOffHelper.SetAttribute ("DataRate", StringValue ("3000000bps"));
  onOffHelper.SetAttribute ("StartTime", TimeValue (Seconds (1.000000)));
  cbrApps.Add (onOffHelper.Install (nodes.Get (1)));

  uint16_t  echoPort = 9;
  UdpEchoClientHelper echoClientHelper (Ipv4Address ("10.0.0.2"), echoPort);
  echoClientHelper.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClientHelper.SetAttribute ("Interval", TimeValue (Seconds (0.1)));
  echoClientHelper.SetAttribute ("PacketSize", UintegerValue (10));
  ApplicationContainer pingApps;
  // again using different start times to workaround Bug 388 and Bug 912
  echoClientHelper.SetAttribute ("StartTime", TimeValue (Seconds (0.001)));
  pingApps.Add (echoClientHelper.Install (nodes.Get (1)));
//  echoClientHelper.SetAttribute ("StartTime", TimeValue (Seconds (0.006)));
//  pingApps.Add (echoClientHelper.Install (nodes.Get (3)));

  ApplicationContainer cbrApps1;
  uint16_t cbrPort1 = 12345;
  OnOffHelper onOffHelper1 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address ("10.0.0.3"), cbrPort1));
  onOffHelper1.SetAttribute ("PacketSize", UintegerValue (packetSize));
  onOffHelper1.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  onOffHelper1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));



  // flow 2:  node 3 -> node 2
  onOffHelper1.SetAttribute ("DataRate", StringValue ("3000000bps"));
  onOffHelper1.SetAttribute ("StartTime", TimeValue (Seconds (1.0001)));
  cbrApps1.Add (onOffHelper1.Install (nodes.Get (3)));

  uint16_t  echoPort1 = 10;
  UdpEchoClientHelper echoClientHelper1 (Ipv4Address ("10.0.0.3"), echoPort1);
  echoClientHelper1.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClientHelper1.SetAttribute ("Interval", TimeValue (Seconds (0.1)));
  echoClientHelper1.SetAttribute ("PacketSize", UintegerValue (10));
  ApplicationContainer pingApps1;
  echoClientHelper1.SetAttribute ("StartTime", TimeValue (Seconds (0.006)));
  pingApps1.Add (echoClientHelper.Install (nodes.Get (3)));




  // 8. Install FlowMonitor on all nodes
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  // 9. Run simulation for 10 seconds
  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();

  // 10. Print per flow statistics
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
double normalThroughtput;
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {

      // first 2 FlowIds are for ECHO apps, we don't want to display them
      if (i->first > 2)
        {
          Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
          std::cout << "Flow " << i->first - 2 << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
          std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
          std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
	  normalThroughtput = (double)(i->second.rxBytes * 8.0 / (double)simulationTime / 1000.0 / 1000.0)/2.0;
          std::cout << "  Throughput: " <<  normalThroughtput << " Mbps\n";
	          
	 
          
        }
	
    }
  
  // 11. Cleanup
  Simulator::Destroy ();
  
}


int main (int argc, char **argv)
{
  
  
  SeedManager::SetRun (3);  // Changes run number from default of 1 to 7
	   experiment (true,20,1000);
	
  return 0;
}
