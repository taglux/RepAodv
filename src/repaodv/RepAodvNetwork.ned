//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

package disserprotocol.repaodv;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.physicallayer.wireless.unitdisk.UnitDiskRadioMedium;
import inet.visualizer.common.IntegratedVisualizer;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.physicallayer.wireless.unitdisk.UnitDiskRadioMedium;
import inet.visualizer.common.IntegratedVisualizer;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.inet.WirelessHost;
//import inet.node.aodv.AodvRouter;
import disserprotocol.repaodv.RepAodvRouter;
import inet.common.scenario.ScenarioManager;
network RepAodvNetwork
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
        sink: RepAodvRouter {
            @display("p=269,206;is=l;i=device/antennatower");
        }
        // Sensor Nodes
        host[numHosts]: RepAodvRouter {
            @display("p=295,275;is=l;i=misc/sensor2");
        }
        scenarioManager: ScenarioManager {
            @display("p=506,200");
        }

    connections allowunconnected:
}
