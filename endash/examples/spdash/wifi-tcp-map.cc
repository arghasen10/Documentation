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
  uint16_t nAp;
  uint16_t nSta;
//  uint16_t interApGap;
  uint16_t boxSize;
  std::string simTag;
  std::string outputDir;
  bool logging;
  std::string animFile;
  uint32_t downloadSize;
  uint16_t numDownload;
  std::string nodeTraceFile;
  double nodeTraceInterval;
  bool logPcap;
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

int
main (int argc, char *argv[])
{
  ProgramConfigurations conf =
  {
   .nAp = 3,
   .nSta = 3,
   .boxSize = 1000, // 1 KM
   .simTag = "default",
   .outputDir = "default",
   .logging = false,
   .animFile = "",
   .downloadSize = 0xFFFFFF,
   .numDownload = 1,
   .nodeTraceFile = "trace",
   .nodeTraceInterval = 1,
   .logPcap = false
  };


  CommandLine cmd;

  cmd.AddValue ("nAp", "The number of Ap in multiple-ue topology",
                conf.nAp);
  cmd.AddValue ("nSta", "The number of sta multiple-sta topology", conf.nSta);
  cmd.AddValue("boxSize", "The length of one size of the square", conf.boxSize);
  cmd.AddValue ("downloadSize", "Data to be downloaded in a session (in byte)",
                conf.downloadSize);
  cmd.AddValue ("numDownload", "Number of download session per ue",
                conf.numDownload);
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

  cmd.Parse (argc, argv);

  double udpAppStartTime = 0.4; //seconds


  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1442));
  Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
  Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(10485760));
  Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(10485760));


  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);


//===========================================================
  NodeContainer staNodes;
  NodeContainer apNodes;

  apNodes.Create (conf.nAp);
  staNodes.Create (conf.nSta);

  double apHeight = 10;

  for (uint32_t i = 0; i < conf.nAp; i++)
    {
      std::cout << "nAp:" << i << " : " << apNodes.Get (i)->GetId () << std::endl;
    }
  for (uint32_t i = 0; i < conf.nSta; i++)
    {
      std::cout << "sta:" << i << " : " << staNodes.Get (i)->GetId () << std::endl;
    }


  MobilityHelper apMobility, staMobility;

  apMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  apMobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
                                   "X", StringValue("ns3::UniformRandomVariable[Min=0|Max=" + std::to_string(conf.boxSize) + "]"),
                                   "Y", StringValue("ns3::UniformRandomVariable[Min=0|Max=" + std::to_string(conf.boxSize) + "]"),
                                   "Z", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(apHeight) + "]"));;
  apMobility.Install (apNodes);

  apMobility.Install(remoteHostContainer);

  ObjectFactory pos;
  pos.SetTypeId ("ns3::RandomBoxPositionAllocator");
  pos.Set ("X", StringValue("ns3::UniformRandomVariable[Min=0|Max=" + std::to_string(conf.boxSize) + "]"));
  pos.Set ("Y", StringValue("ns3::UniformRandomVariable[Min=0|Max=" + std::to_string(conf.boxSize) + "]"));
  auto m_position = pos.Create ()->GetObject<PositionAllocator> ();

//  staMobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
//                               "Speed", StringValue("ns3::UniformRandomVariable[Min=3|Max=10]"),
//                               "Pause", StringValue("ns3::UniformRandomVariable[Min=3|Max=5]"),
//                               "PositionAllocator", PointerValue(m_position));
  staMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  staMobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
                                    "X", StringValue("ns3::UniformRandomVariable[Min=0|Max=" + std::to_string(conf.boxSize) + "]"),
                                    "Y", StringValue("ns3::UniformRandomVariable[Min=0|Max=" + std::to_string(conf.boxSize) + "]"));;
  staMobility.Install (staNodes);

  if (!isDir (conf.outputDir))
    {
      mkdir (conf.outputDir.c_str (), S_IRWXU);
    }

  //===========================================================

  YansWifiChannelHelper channel; // = YansWifiChannelHelper::Default ();
  channel.SetPropagationDelay ("ns3::RandomPropagationDelayModel", "Variable", StringValue ("ns3::UniformRandomVariable[Min=0.0000003|Max=0.000007]"));
//  channel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
  channel.AddPropagationLoss("ns3::LogNormalDistancePropagationLossModel");
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
  csma.SetChannelAttribute ("DataRate", StringValue ("5Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  stack.Install(remoteHostContainer);
//  NetDeviceContainer remoteHostDevice = csma.Install(remoteHostContainer);

  stack.Install (apNodes);
  NetDeviceContainer backboneDevices;
  NetDeviceContainer bridgeDevices;
  backboneDevices = csma.Install (NodeContainer(apNodes, remoteHost));
  for(uint32_t i=0; i < conf.nAp; i++) {
      BridgeHelper bridge;
      NetDeviceContainer bridgeDev;
      bridgeDev = bridge.Install (apNodes.Get (i), NetDeviceContainer (apDevices.Get(i), backboneDevices.Get (i)));
      bridgeDevices.Add(bridgeDev);
  }

  stack.Install (staNodes);

  Ipv4AddressHelper address;


  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer remotehostIpIfaces = address.Assign(backboneDevices.Get(conf.nAp));
  Ipv4InterfaceContainer wifiInterface = address.Assign (bridgeDevices);
  address.Assign (staDevices);
  //===========================================================
  std::cout << remotehostIpIfaces.GetAddress(0) << " Num b:" << bridgeDevices.GetN() << " sta:" << staDevices.GetN() << " nAp:" << conf.nAp << std::endl;
  std::cout << "BbN:" << apNodes.GetN() << " bbDev:" << backboneDevices.GetN() << " apdev:" << apDevices.GetN() << std::endl;
  //===========================================================

  uint16_t dlPort = 1234;
  ApplicationContainer clientApps, serverApps;

  auto trackNode = staNodes.Get(0);
  std::string trcSrc = "/NodeList/" + std::to_string(trackNode->GetId ()) + "/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::StaWifiMac/";
  Config::ConnectWithoutContext(trcSrc + "Assoc", MakeBoundCallback(AssocDeassoc, trackNode, true));
  Config::ConnectWithoutContext(trcSrc + "DeAssoc", MakeBoundCallback(AssocDeassoc, trackNode, false));

  DashServerHelper dashSrHelper (dlPort);
  serverApps.Add (dashSrHelper.Install (remoteHost));

  int counter = 0;
  DashHttpDownloadHelper dlClient (remotehostIpIfaces.GetAddress (0), dlPort); //Remotehost is the second node, pgw is first
  dlClient.SetAttribute ("Size", UintegerValue (conf.downloadSize));
  dlClient.SetAttribute ("NumberOfDownload", UintegerValue (conf.numDownload));
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

  if (!conf.outputDir.empty () && conf.logPcap) {
    csma.EnablePcapAll (conf.outputDir + "/nr-csma");
    phy.EnablePcapAll(conf.outputDir + "/nr-wifi");

    AnimationInterface anim(conf.outputDir + "/anim.xml");
    anim.EnablePacketMetadata ();

    for (uint32_t i = 0; i < staNodes.GetN (); ++i)
        {
          anim.UpdateNodeDescription (staNodes.Get (i), "STA"); // Optional
          anim.UpdateNodeColor (staNodes.Get (i), 255, 0, 0); // Optional
        }
      for (uint32_t i = 0; i < apNodes.GetN (); ++i)
        {
          anim.UpdateNodeDescription (apNodes.Get (i), "AP"); // Optional
          anim.UpdateNodeColor (apNodes.Get (i), 0, 255, 0); // Optional
        }
      for (uint32_t i = 0; i < remoteHostContainer.GetN (); ++i)
        {
          anim.UpdateNodeDescription (remoteHostContainer.Get (i), "CSMA"); // Optional
          anim.UpdateNodeColor (remoteHostContainer.Get (i), 0, 0, 255); // Optional
        }
  }


  Simulator::Run ();

  Simulator::Destroy ();
  return 0;
}
