#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/bridge-helper.h"
#include "ns3/dash-helper.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-net-device.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/wifi-module.h"
#include "ns3/netanim-module.h"

#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");

struct ProgramConfigurations
{
  std::string apPos;
  std::string staMob;
  std::string simTag;
  std::string outputDir;
  bool logging;
  std::string animFile;
  std::string nodeTraceFile;
  double nodeTraceInterval;
  bool logPcap;
  bool lossModelLogNormal;
};

void
onStart (int *count)
{
  std::cout << "Starting at:" << Simulator::Now ().GetSeconds() << std::endl;
  (*count)++;
}
void
onStop (int *count)
{
  std::cout << "Stoping at:" << Simulator::Now ().GetSeconds() << std::endl;
  (*count)--;
  if (!(*count))
    {
      Simulator::Stop ();
    }
}

bool
isDir (std::string path)
{
  struct stat statbuf;
  if (stat (path.c_str (), &statbuf) != 0)
    return false;
  return S_ISDIR(statbuf.st_mode);
}


std::string
readNodeTrace (NodeContainer *apNodes, Ptr<Node> node, bool firstLine = false)
{
  const std::string rrcStates[] =
    {
        "IDLE_START",
        "IDLE_CELL_SEARCH",
        "IDLE_WAIT_MIB_SIB1",
        "IDLE_WAIT_MIB",
        "IDLE_WAIT_SIB1",
        "IDLE_CAMPED_NORMALLY",
        "IDLE_WAIT_SIB2",
        "IDLE_RANDOM_ACCESS",
        "IDLE_CONNECTING",
        "CONNECTED_NORMALLY",
        "CONNECTED_HANDOVER",
        "CONNECTED_PHY_PROBLEM",
        "CONNECTED_REESTABLISHING",
        "NUM_STATES"
    };

  if (firstLine)
    {
      std::string ret = "nodeId,velo_x,velo_y,pos_x,pos_y,chId,#dev,Freq,ChNo,rxGain,rxSensitivity";
#ifdef STA_WIFI_MAC_NEW_FUNC
      ret += ",rxSnr,rxPer,rxSignal,rxNoise,rxRate";
#endif
      ret += ",linkup,bssid,";
      return ret;
    }

  std::stringstream stream;
  stream << std::to_string (node->GetId ());
  auto mModel = node->GetObject<MobilityModel> ();
  stream << "," << mModel->GetVelocity ().x << "," << mModel->GetVelocity ().y;
  stream << "," << mModel->GetPosition ().x << "," << mModel->GetPosition ().y;

  Ptr<WifiNetDevice> netDevice;   // = node->GetObject<MmWaveUeNetDevice>();
  for (uint32_t i = 0; i < node->GetNDevices (); i++)
    {
      auto nd = node->GetDevice (i);
      if (nd->GetInstanceTypeId () == WifiNetDevice::GetTypeId ())
        {
          netDevice = DynamicCast<WifiNetDevice> (nd);
          break;
        }
      std::cout << "\tNode: " << node->GetId () << ", Device " << i << ": "
          << node->GetDevice (i)->GetInstanceTypeId () << std::endl;
    }

  Ptr<StaWifiMac> mac = DynamicCast<StaWifiMac>(netDevice->GetMac());

  stream << "," << std::to_string (netDevice->GetChannel ()->GetId());
  stream << "," << std::to_string (netDevice->GetChannel ()->GetNDevices());
  Ptr< WifiPhy > phy = netDevice->GetPhy();
  stream << "," << std::to_string(phy->GetFrequency ());
  stream << "," << std::to_string(phy->GetChannelNumber ());
  stream << "," << std::to_string(phy->GetRxGain ());
  stream << "," << std::to_string(phy->GetRxSensitivity ());
#ifdef STA_WIFI_MAC_NEW_FUNC
  auto recpStat = mac->GetLastReceptionStatus();
  stream << "," << recpStat.snr;
  stream << "," << recpStat.per;
  stream << "," << recpStat.signal;
  stream << "," << recpStat.noise;
  stream << "," << recpStat.rxRate;
#endif
  stream << "," << std::to_string(netDevice->IsLinkUp());

  Mac48Address remAddress = mac->GetBssid();
  stream << "," << remAddress;

  for(uint32_t i = 0; i < apNodes->GetN(); i++)
    {
      auto ap = apNodes->Get(i);
      auto apModel = ap->GetObject<MobilityModel> ();
      stream << "," << mModel->GetDistanceFrom(apModel);
    }


  return stream.str ();
}

void AssocDeassoc(Ptr<Node> node, bool assoc, Mac48Address val1) {
  std::cout << "time: " << Simulator::Now().GetSeconds() << " assoc: " << std::to_string(assoc) << " with: " << val1 << std::endl;
}

void TxDrop(Ptr<const Packet> pkt) {
  std::cout << "Packet dropped" << std::endl;
}

void RxDrop(Ptr<const Packet> pkt) {
//  Header header;
  Ipv4Header header;
  pkt->PeekHeader(header);
  std::cout << "Rx Packet dropped:" << header << std::endl;
}


int
main (int argc, char *argv[])
{
  ProgramConfigurations conf =
  {
   .apPos = "",
   .staMob = "",
   .simTag = "default",
   .outputDir = "default",
   .logging = false,
   .animFile = "",
   .nodeTraceFile = "trace",
   .nodeTraceInterval = 1,
   .logPcap = false,
   .lossModelLogNormal = false
  };


  CommandLine cmd;

  cmd.AddValue ("apPos", "Ap position list",
                conf.apPos);
  cmd.AddValue ("staMob", "Sta mobility trace", conf.staMob);
  cmd.AddValue ("nodeTraceFile", "Trace file path prefix", conf.nodeTraceFile);
  cmd.AddValue ("nodeTraceInterval", "Node trace interval",
                conf.nodeTraceInterval);
  cmd.AddValue (
      "simTag",
      "tag to be appended to output filenames to distinguish simulation campaigns",
      conf.simTag);
  cmd.AddValue ("outputDir", "directory where to store simulation results",
                conf.outputDir);
  cmd.AddValue ("logging", "Enable logging", conf.logging);
  cmd.AddValue ("logPcap", "Whether pcap files need to be store or not [0]",
                conf.logPcap);
  cmd.AddValue("lossModelLogNormal", "Whether to use lognormaldistant loss model", conf.lossModelLogNormal);

  cmd.Parse (argc, argv);

  if(conf.apPos.empty() || conf.staMob.empty()){
      std::cerr << "apPos and staMob is required" << std::endl;
      return 2;
  }

  double udpAppStartTime = 0.4; //seconds
  double apHeight = 4;

  Ptr<ListPositionAllocator> apPositionAlloc = CreateObject<ListPositionAllocator> ();
  uint16_t numAp = 0;
  if (1){
    std::ifstream myfile;
    myfile.open (conf.apPos);
    if(!myfile) {
        std::cerr << "apPos error" << std::endl;
        return 2;
    }
    double x, y;
    while(!myfile.eof()) {
        myfile >> x >> y;
        apPositionAlloc->Add (Vector (x, y, apHeight));
        numAp += 1;
    }
  }


  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1442));
  Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
  Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(10485760));
  Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(10485760));



  //===========================================================
  NodeContainer staNodes;
  NodeContainer apNodes;
  NodeContainer remoteHostContainer;
  staNodes.Create (1);
  remoteHostContainer.Create (1);
  apNodes.Create (numAp);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);




  for (uint32_t i = 0; i < numAp; i++)
    {
      std::cout << "nAp:" << i << " : " << apNodes.Get (i)->GetId () << std::endl;
    }
  for (uint32_t i = 0; i < 1; i++)
    {
      std::cout << "sta:" << i << " : " << staNodes.Get (i)->GetId () << std::endl;
    }

  MobilityHelper apMobility;

  Ns2MobilityHelper staMobility(conf.staMob);

  apMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  apMobility.SetPositionAllocator(apPositionAlloc);
  apMobility.Install (apNodes);

  apMobility.Install(remoteHostContainer);

//  ObjectFactory pos;
//  pos.SetTypeId ("ns3::RandomBoxPositionAllocator");
//  pos.Set ("X", StringValue("ns3::UniformRandomVariable[Min=0|Max=" + std::to_string(conf.boxSide) + "]"));
//  pos.Set ("Y", StringValue("ns3::UniformRandomVariable[Min=0|Max=" + std::to_string(conf.boxSide) + "]"));
//  auto m_position = pos.Create ()->GetObject<PositionAllocator> ();

//  staMobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
//                               "Speed", StringValue("ns3::UniformRandomVariable[Min=3|Max=10]"),
//                               "Pause", StringValue("ns3::UniformRandomVariable[Min=3|Max=5]"),
//                               "PositionAllocator", PointerValue(m_position));
////  staMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
//
//  staMobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
//                                    "X", StringValue("ns3::UniformRandomVariable[Min=0|Max=" + std::to_string(conf.boxSide) + "]"),
//                                    "Y", StringValue("ns3::UniformRandomVariable[Min=0|Max=" + std::to_string(conf.boxSide) + "]"));
  staMobility.Install ();

  if (!isDir (conf.outputDir))
    {
      mkdir (conf.outputDir.c_str (), S_IRWXU);
    }

  //===========================================================

  YansWifiChannelHelper channel; // = YansWifiChannelHelper::Default ();
  channel.SetPropagationDelay ("ns3::RandomPropagationDelayModel", "Variable", StringValue ("ns3::UniformRandomVariable[Min=0.0000003|Max=0.000007]"));
  if(conf.lossModelLogNormal)
    channel.AddPropagationLoss("ns3::LogNormalDistancePropagationLossModel");
  else
    channel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
//  channel.AddPropagationLoss("ns3::LogNormalDistancePropagationLossModel");
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();

#ifndef OLD_NS3
  phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
#endif
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
//  wifi.SetStandard (WIFI_PHY_STANDARD_80211n_2_4GHZ);

#ifdef OLD_NS3
  NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();
#else
  WifiMacHelper mac;
#endif
  Ssid ssid = Ssid ("ns-3-ssid");

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));
  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, apNodes);

  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));
  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, staNodes);
  //===========================================================
  //  InternetStackHelper stack;
  // stack.Install (csmaNodes);

  InternetStackHelper stack;
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("500Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  stack.Install(remoteHostContainer);
//  NetDeviceContainer remoteHostDevice = csma.Install(remoteHostContainer);

  stack.Install (apNodes);
  NetDeviceContainer backboneDevices;
  NetDeviceContainer bridgeDevices;
  backboneDevices = csma.Install (NodeContainer(apNodes, remoteHost));
  for(uint32_t i=0; i < numAp; i++) {
      BridgeHelper bridge;
      bridge.SetDeviceAttribute("ExpirationTime", TimeValue(Seconds(1)));
      bridge.SetDeviceAttribute("EnableLearning", BooleanValue(false));
      NetDeviceContainer bridgeDev;
      bridgeDev = bridge.Install (apNodes.Get (i), NetDeviceContainer (apDevices.Get(i), backboneDevices.Get (i)));
      bridgeDevices.Add(bridgeDev);
  }

  stack.Install (staNodes);

  Ipv4AddressHelper address;


  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer remotehostIpIfaces = address.Assign(backboneDevices.Get(numAp));
  Ipv4InterfaceContainer wifiInterface = address.Assign (bridgeDevices);
  address.Assign (staDevices);
  //===========================================================
  std::cout << remotehostIpIfaces.GetAddress(0) << " Num b:" << bridgeDevices.GetN() << " sta:" << staDevices.GetN() << " nAp:" << numAp << std::endl;
  std::cout << "BbN:" << apNodes.GetN() << " bbDev:" << backboneDevices.GetN() << " apdev:" << apDevices.GetN() << std::endl;
  //===========================================================

  uint16_t dlPort = 1234;
  ApplicationContainer clientApps, serverApps;

  auto trackNode = staNodes.Get(0);
  std::string trcSrc = "/NodeList/" + std::to_string(trackNode->GetId ()) + "/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::StaWifiMac/";
  Config::ConnectWithoutContext(trcSrc + "Assoc", MakeBoundCallback(AssocDeassoc, trackNode, true));
  Config::ConnectWithoutContext(trcSrc + "DeAssoc", MakeBoundCallback(AssocDeassoc, trackNode, false));

  Config::ConnectWithoutContext("/NodeList/2/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::ApWifiMac/MacTxDrop", MakeCallback(TxDrop));
  Config::ConnectWithoutContext("/NodeList/2/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::ApWifiMac/MacRxDrop", MakeCallback(RxDrop));

  DashServerHelper dashSrHelper (dlPort);
  serverApps.Add (dashSrHelper.Install (remoteHost));

  int counter = 0;
  DashClientHelper dlClient (remotehostIpIfaces.GetAddress (0), dlPort); //Remotehost is the second node, pgw is first
  dlClient.SetAttribute ("OnStartCB",
                         CallbackValue (MakeBoundCallback (onStart, &counter)));
  dlClient.SetAttribute ("OnStopCB",
                         CallbackValue (MakeBoundCallback (onStop, &counter)));
  if (!conf.outputDir.empty ())
    {
      dlClient.SetAttribute ("NodeTracePath", StringValue (conf.outputDir + "/" + conf.nodeTraceFile));
    }
  dlClient.SetAttribute ("NodeTraceInterval",
                         TimeValue (Seconds (conf.nodeTraceInterval)));
  dlClient.SetAttribute ("NodeTraceHelperCallBack", CallbackValue (MakeCallback (readNodeTrace)));
  dlClient.SetAttribute ("NodeTraceHelperCallBack",
                         CallbackValue (MakeBoundCallback (readNodeTrace, &apNodes)));
  dlClient.SetAttribute ("Timeout", TimeValue(Seconds(-1)));

  if (!conf.outputDir.empty ())
    {
      dlClient.SetAttribute ("NodeTracePath", StringValue (conf.outputDir + "/" + conf.nodeTraceFile));
      dlClient.SetAttribute ("TracePath", StringValue (conf.outputDir + "/TraceData"));
    }

  clientApps = dlClient.Install (staNodes);

  serverApps.Start (Seconds (udpAppStartTime));
  clientApps.Start (Seconds (udpAppStartTime + 1));
  //===========================================================
  AnimationInterface *anim;
  if (!conf.outputDir.empty () && conf.logPcap)
    {
      csma.EnablePcapAll (conf.outputDir + "/csma");
      phy.EnablePcapAll(conf.outputDir + "/wifi");

      anim = new AnimationInterface(conf.outputDir + "/anim.xml");
      anim->SetConstantPosition(remoteHost, 0, 0, 0);
//      anim->EnablePacketMetadata ();

      for (uint32_t i = 0; i < staNodes.GetN (); ++i)
        {
          anim->UpdateNodeDescription (staNodes.Get (i), "STA"); // Optional
          anim->UpdateNodeColor (staNodes.Get (i), 255, 0, 0); // Optional
        }
      for (uint32_t i = 0; i < apNodes.GetN (); ++i)
        {
          anim->UpdateNodeDescription (apNodes.Get (i), "AP"); // Optional
          anim->UpdateNodeColor (apNodes.Get (i), 0, 255, 0); // Optional
        }
      for (uint32_t i = 0; i < remoteHostContainer.GetN (); ++i)
        {
          anim->UpdateNodeDescription (remoteHostContainer.Get (i), "CSMA"); // Optional
          anim->UpdateNodeColor (remoteHostContainer.Get (i), 0, 0, 255); // Optional
        }
    }


  Simulator::Run ();

  Simulator::Destroy ();
  delete anim;
  return 0;
}
