/*
 * ============================================================================
 * COMMUNITY CLOUD WAN - Smart Traffic & Public Services
 * ============================================================================
 * 
 * Project: Cloud Services and WAN Technology for Public Services 
 *          and Traffic Management in Yaounde
 * 
 * Author: MAKUETE LEKOGNIA MARIE MICHELLE
 * Matricule: ICTU20234486
 * Institution: ICT University Yaounde
 * Date: 17-10-2025
 * 
 * NS-3 Version: 3.29
 * 
 * ============================================================================
 * ARCHITECTURE:
 * - Cloud Server (analytics and data storage)
 * - WAN Network (connects city infrastructure)
 * - Traffic Sensors (collect real-time traffic data)
 * - Government Offices (connected via WAN)
 * - Mobile Portal (citizen access)
 * ============================================================================
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CommunityCloudWAN");

int main(int argc, char* argv[])
{
    // ========================================================================
    // SIMULATION PARAMETERS
    // ========================================================================
    
    uint32_t nTrafficSensors = 8;      // Traffic sensors across Yaounde
    uint32_t nGovtOffices = 3;         // Government offices
    uint32_t nCitizens = 10;           // Citizens using mobile portal
    double simulationTime = 30.0;      // Simulation duration (seconds)
    bool verbose = true;

    CommandLine cmd;
    cmd.AddValue("sensors", "Number of traffic sensors", nTrafficSensors);
    cmd.AddValue("offices", "Number of government offices", nGovtOffices);
    cmd.AddValue("citizens", "Number of citizens", nCitizens);
    cmd.AddValue("time", "Simulation time", simulationTime);
    cmd.AddValue("verbose", "Enable logging", verbose);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("CommunityCloudWAN", LOG_LEVEL_INFO);
    }

    // Display simulation header
    std::cout << "\n";
    std::cout << "================================================================\n";
    std::cout << "     COMMUNITY CLOUD WAN - Smart City Simulation\n";
    std::cout << "================================================================\n";
    std::cout << "Project: Cloud Services & WAN for Public Services\n";
    std::cout << "Author: MAKUETE LEKOGNIA MARIE MICHELLE\n";
    std::cout << "ICT University Yaounde - NS-3.29\n";
    std::cout << "================================================================\n";
    std::cout << "\nConfiguration:\n";
    std::cout << "  Traffic Sensors:     " << nTrafficSensors << "\n";
    std::cout << "  Government Offices:  " << nGovtOffices << "\n";
    std::cout << "  Citizens (Mobile):   " << nCitizens << "\n";
    std::cout << "  Simulation Time:     " << simulationTime << " seconds\n";
    std::cout << "================================================================\n\n";

    // ========================================================================
    // CREATE NETWORK NODES
    // ========================================================================
    
    NS_LOG_INFO("Creating network nodes...");

    // Cloud Server (central analytics and storage)
    NodeContainer cloudServer;
    cloudServer.Create(1);

    // WAN Core Routers
    NodeContainer wanRouters;
    wanRouters.Create(3);

    // Traffic Management System nodes
    NodeContainer trafficSensors;
    trafficSensors.Create(nTrafficSensors);

    // Government Office nodes
    NodeContainer govtOffices;
    govtOffices.Create(nGovtOffices);

    // WiFi Access Point for citizen mobile portal
    NodeContainer mobileAccessPoint;
    mobileAccessPoint.Create(1);

    // Citizens with mobile devices
    NodeContainer citizens;
    citizens.Create(nCitizens);

    NS_LOG_INFO("Nodes created successfully");

    // ========================================================================
    // INSTALL INTERNET STACK
    // ========================================================================
    
    NS_LOG_INFO("Installing Internet stack...");

    InternetStackHelper stack;
    stack.Install(cloudServer);
    stack.Install(wanRouters);
    stack.Install(trafficSensors);
    stack.Install(govtOffices);
    stack.Install(mobileAccessPoint);
    stack.Install(citizens);

    // ========================================================================
    // CONFIGURE POINT-TO-POINT LINKS
    // ========================================================================
    
    NS_LOG_INFO("Configuring P2P links...");

    // High-capacity WAN backbone
    PointToPointHelper p2pWAN;
    p2pWAN.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    p2pWAN.SetChannelAttribute("Delay", StringValue("5ms"));

    // Medium-capacity links for sensors and offices
    PointToPointHelper p2pLocal;
    p2pLocal.SetDeviceAttribute("DataRate", StringValue("50Mbps"));
    p2pLocal.SetChannelAttribute("Delay", StringValue("2ms"));

    // Cloud Server to WAN Router 0
    NetDeviceContainer devCloudWAN0 = p2pWAN.Install(cloudServer.Get(0), wanRouters.Get(0));

    // WAN backbone mesh
    NetDeviceContainer devWAN01 = p2pWAN.Install(wanRouters.Get(0), wanRouters.Get(1));
    NetDeviceContainer devWAN12 = p2pWAN.Install(wanRouters.Get(1), wanRouters.Get(2));
    NetDeviceContainer devWAN20 = p2pWAN.Install(wanRouters.Get(2), wanRouters.Get(0));

    // Connect traffic sensors to WAN Router 1
    NetDeviceContainer* sensorDevices = new NetDeviceContainer[nTrafficSensors];
    for (uint32_t i = 0; i < nTrafficSensors; ++i)
    {
        sensorDevices[i] = p2pLocal.Install(trafficSensors.Get(i), wanRouters.Get(1));
    }

    // Connect government offices to WAN Router 2
    NetDeviceContainer* officeDevices = new NetDeviceContainer[nGovtOffices];
    for (uint32_t i = 0; i < nGovtOffices; ++i)
    {
        officeDevices[i] = p2pLocal.Install(govtOffices.Get(i), wanRouters.Get(2));
    }

    // Connect mobile access point to WAN Router 0
    NetDeviceContainer devAPWAN = p2pLocal.Install(mobileAccessPoint.Get(0), wanRouters.Get(0));

    // ========================================================================
    // CONFIGURE WIFI FOR MOBILE PORTAL
    // ========================================================================
    
    NS_LOG_INFO("Configuring WiFi for mobile portal...");

    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
    wifiPhy.SetChannel(wifiChannel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_PHY_STANDARD_80211b);
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    WifiMacHelper wifiMac;
    Ssid ssid = Ssid("Yaounde-Smart-City");

    // Configure citizen devices (stations)
    wifiMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer citizenDevices = wifi.Install(wifiPhy, wifiMac, citizens);

    // Configure access point
    wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevice = wifi.Install(wifiPhy, wifiMac, mobileAccessPoint);

    // ========================================================================
    // CONFIGURE MOBILITY
    // ========================================================================
    
    NS_LOG_INFO("Configuring mobility models...");

    MobilityHelper mobility;

    // Fixed infrastructure (cloud, WAN, sensors, offices, AP)
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(cloudServer);
    mobility.Install(wanRouters);
    mobility.Install(trafficSensors);
    mobility.Install(govtOffices);
    mobility.Install(mobileAccessPoint);

    // Mobile citizens
    mobility.SetPositionAllocator("ns3::RandomDiscPositionAllocator",
                                  "X", DoubleValue(200.0),
                                  "Y", DoubleValue(200.0),
                                  "Rho", StringValue("ns3::UniformRandomVariable[Min=0|Max=50]"));
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(100, 300, 100, 300)));
    mobility.Install(citizens);

    // ========================================================================
    // ASSIGN IP ADDRESSES
    // ========================================================================
    
    NS_LOG_INFO("Assigning IP addresses...");

    Ipv4AddressHelper address;

    // Cloud - WAN Router 0: 10.1.1.0/24
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifCloudWAN = address.Assign(devCloudWAN0);

    // WAN backbone
    address.SetBase("10.2.1.0", "255.255.255.0");
    address.Assign(devWAN01);
    address.SetBase("10.2.2.0", "255.255.255.0");
    address.Assign(devWAN12);
    address.SetBase("10.2.3.0", "255.255.255.0");
    address.Assign(devWAN20);

    // Traffic sensors: 172.16.0.0/16
    for (uint32_t i = 0; i < nTrafficSensors; ++i)
    {
        std::ostringstream subnet;
        subnet << "172.16." << (i + 1) << ".0";
        address.SetBase(subnet.str().c_str(), "255.255.255.0");
        address.Assign(sensorDevices[i]);
    }

    // Government offices: 172.17.0.0/16
    for (uint32_t i = 0; i < nGovtOffices; ++i)
    {
        std::ostringstream subnet;
        subnet << "172.17." << (i + 1) << ".0";
        address.SetBase(subnet.str().c_str(), "255.255.255.0");
        address.Assign(officeDevices[i]);
    }

    // Mobile portal access point: 192.168.1.0/24
    address.SetBase("192.168.1.0", "255.255.255.0");
    address.Assign(devAPWAN);

    // Mobile portal WiFi: 192.168.2.0/24
    address.SetBase("192.168.2.0", "255.255.255.0");
    address.Assign(citizenDevices);
    address.Assign(apDevice);

    // Enable global routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    NS_LOG_INFO("IP addresses assigned and routing configured");

    // ========================================================================
    // CONFIGURE APPLICATIONS
    // ========================================================================
    
    NS_LOG_INFO("Installing applications...");

    uint16_t port = 9;

    // Cloud Server UDP Echo Server (receives data from all sources)
    UdpEchoServerHelper echoServer(port);
    ApplicationContainer serverApps = echoServer.Install(cloudServer.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(simulationTime));

    Address cloudAddress(InetSocketAddress(ifCloudWAN.GetAddress(0), port));

    // Traffic sensors send data to cloud
    for (uint32_t i = 0; i < nTrafficSensors; ++i)
    {
        UdpEchoClientHelper sensorClient(ifCloudWAN.GetAddress(0), port);
        sensorClient.SetAttribute("MaxPackets", UintegerValue(100));
        sensorClient.SetAttribute("Interval", TimeValue(Seconds(0.5)));
        sensorClient.SetAttribute("PacketSize", UintegerValue(512));

        ApplicationContainer sensorApp = sensorClient.Install(trafficSensors.Get(i));
        sensorApp.Start(Seconds(2.0 + i * 0.2));
        sensorApp.Stop(Seconds(simulationTime));
    }

    // Government offices send/receive data to/from cloud
    for (uint32_t i = 0; i < nGovtOffices; ++i)
    {
        UdpEchoClientHelper officeClient(ifCloudWAN.GetAddress(0), port);
        officeClient.SetAttribute("MaxPackets", UintegerValue(50));
        officeClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
        officeClient.SetAttribute("PacketSize", UintegerValue(1024));

        ApplicationContainer officeApp = officeClient.Install(govtOffices.Get(i));
        officeApp.Start(Seconds(3.0 + i * 0.5));
        officeApp.Stop(Seconds(simulationTime));
    }

    // Citizens access services via mobile portal
    for (uint32_t i = 0; i < nCitizens; ++i)
    {
        UdpEchoClientHelper citizenClient(ifCloudWAN.GetAddress(0), port);
        citizenClient.SetAttribute("MaxPackets", UintegerValue(30));
        citizenClient.SetAttribute("Interval", TimeValue(Seconds(0.8)));
        citizenClient.SetAttribute("PacketSize", UintegerValue(256));

        ApplicationContainer citizenApp = citizenClient.Install(citizens.Get(i));
        citizenApp.Start(Seconds(4.0 + i * 0.3));
        citizenApp.Stop(Seconds(simulationTime));
    }

    NS_LOG_INFO("Applications configured");

    // ========================================================================
    // FLOW MONITOR
    // ========================================================================
    
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    // ========================================================================
    // NETANIM CONFIGURATION
    // ========================================================================
    
    NS_LOG_INFO("Configuring NetAnim visualization...");

    AnimationInterface anim("community-cloud-wan.xml");

    // Cloud Server
    anim.UpdateNodeDescription(cloudServer.Get(0), "Cloud-Server");
    anim.UpdateNodeColor(cloudServer.Get(0), 0, 0, 255);
    anim.UpdateNodeSize(cloudServer.Get(0)->GetId(), 15, 15);

    // WAN Routers
    for (uint32_t i = 0; i < wanRouters.GetN(); ++i)
    {
        std::ostringstream desc;
        desc << "WAN-Router-" << i;
        anim.UpdateNodeDescription(wanRouters.Get(i), desc.str());
        anim.UpdateNodeColor(wanRouters.Get(i), 0, 255, 0);
        anim.UpdateNodeSize(wanRouters.Get(i)->GetId(), 10, 10);
    }

    // Traffic Sensors
    for (uint32_t i = 0; i < trafficSensors.GetN(); ++i)
    {
        std::ostringstream desc;
        desc << "Sensor-" << (i + 1);
        anim.UpdateNodeDescription(trafficSensors.Get(i), desc.str());
        anim.UpdateNodeColor(trafficSensors.Get(i), 255, 165, 0);
    }

    // Government Offices
    for (uint32_t i = 0; i < govtOffices.GetN(); ++i)
    {
        std::ostringstream desc;
        desc << "Office-" << (i + 1);
        anim.UpdateNodeDescription(govtOffices.Get(i), desc.str());
        anim.UpdateNodeColor(govtOffices.Get(i), 255, 0, 255);
    }

    // Mobile Access Point
    anim.UpdateNodeDescription(mobileAccessPoint.Get(0), "Mobile-AP");
    anim.UpdateNodeColor(mobileAccessPoint.Get(0), 255, 215, 0);

    // Citizens
    for (uint32_t i = 0; i < citizens.GetN(); ++i)
    {
        std::ostringstream desc;
        desc << "Citizen-" << (i + 1);
        anim.UpdateNodeDescription(citizens.Get(i), desc.str());
        anim.UpdateNodeColor(citizens.Get(i), 173, 216, 230);
    }

    // ========================================================================
    // RUN SIMULATION
    // ========================================================================
    
    std::cout << "\nStarting simulation...\n\n";

    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    std::cout << "\n================================================================\n";
    std::cout << "                   SIMULATION RESULTS\n";
    std::cout << "================================================================\n\n";

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    uint64_t totalTx = 0, totalRx = 0;
    double totalThroughput = 0.0;
    double totalDelay = 0.0;
    uint32_t flowCount = 0;

    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
         i != stats.end(); ++i)
    {
        totalTx += i->second.txPackets;
        totalRx += i->second.rxPackets;
        totalThroughput += i->second.rxBytes * 8.0 / simulationTime / 1000.0;
        
        if (i->second.rxPackets > 0)
        {
            totalDelay += i->second.delaySum.GetSeconds() / i->second.rxPackets;
            flowCount++;
        }
    }

    std::cout << "Traffic Statistics:\n";
    std::cout << "  Packets Transmitted:  " << totalTx << "\n";
    std::cout << "  Packets Received:     " << totalRx << "\n";
    std::cout << "  Packets Lost:         " << (totalTx - totalRx) << "\n";
    
    if (totalTx > 0)
    {
        double lossRate = ((totalTx - totalRx) * 100.0 / totalTx);
        std::cout << "  Packet Loss Rate:     " << lossRate << " %\n";
    }
    
    std::cout << "  Total Throughput:     " << totalThroughput << " kbps\n";
    
    if (flowCount > 0)
    {
        double avgDelay = (totalDelay / flowCount) * 1000;
        std::cout << "  Average Delay:        " << avgDelay << " ms\n";
    }

    std::cout << "\n================================================================\n";
    std::cout << "Files Generated:\n";
    std::cout << "  NetAnim: community-cloud-wan.xml\n";
    std::cout << "================================================================\n";
    std::cout << "\nSimulation completed successfully!\n";
    std::cout << "Project: Community Cloud WAN\n";
    std::cout << "Author: MAKUETE LEKOGNIA MARIE MICHELLE\n";
    std::cout << "ICT University Yaounde\n";
    std::cout << "================================================================\n\n";

    Simulator::Destroy();
    
    delete[] sensorDevices;
    delete[] officeDevices;
    
    return 0;
}
