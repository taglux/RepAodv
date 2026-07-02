package disserprotocol;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.aodv.AodvRouter;
//import inet.physicallayer.wireless.ieee802154.packetlevel.Ieee802154NarrowbandScalarRadioMedium;
import inet.physicallayer.wireless.unitdisk.UnitDiskRadioMedium;
import inet.visualizer.common.IntegratedVisualizer;
import inet.networklayer.ipv4.RoutingTableRecorder;
//
// WSN Network Topology for Reputation Model Thesis
// Standard: IEEE 802.15.4
//
network WSNNetwork_AODV
{
    parameters:
        int numHosts = default(4);
        double playgroundSizeX @unit(m) = 100m;
        double playgroundSizeY @unit(m) = 100m;
        @display("bgb=617.4,552.2");

    submodules:
        visualizer: IntegratedVisualizer {
            @display("p=506,48");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=506,92");
            config = xml("<config><interface hosts='**' address='10.0.0.x' netmask='255.255.255.0'/></config>");
        }
        radioMedium: UnitDiskRadioMedium {
            parameters:
                @display("p=505,131;is=s");
        }
        routingTableRecorder: RoutingTableRecorder {
            parameters:
                @display("p=497,153;is=s");
        }
        // The Sink Node (Gateway) - Fixed in Center
        sink: AodvRouter {
            @display("p=269,206;is=l;i=device/antennatower");
        }
        // Sensor Nodes
        host[numHosts]: AodvRouter {
            @display("p=295,275;is=s;i=misc/sensor2");
        }
    connections allowunconnected:
}
