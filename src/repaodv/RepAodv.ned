//
// Copyright (C) 2014 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

package disserprotocol.repaodv;

import inet.applications.contract.IApp;

//
// AODV с поддержкой репутации (REP-AODV) и движком доверия (Trust Engine).
//
// Этот модуль реализует переработанную подсистему доверия, используя байесовское
// бета-распределение и детекторы 2ACK для изоляции вредоносных узлов.
//
simple RepAodv like IApp
{
    parameters:
        @class("disserprotocol::aodv::RepAodv");
        @display("i=block/network2");
        string routingTableModule = default("^.ipv4.routingTable");
        string interfaceTableModule = default("^.interfaceTable");
        string networkProtocolModule = default("^.ipv4.ip");
        bool askGratuitousRREP = default(false);
        bool useHelloMessages = default(false);
        bool useLocalRepair = default(false);
        bool destinationOnlyFlag = default(false);
        int udpPort = default(654);
        string interface = default("wlan0");

        double maxPeriodicJitter @unit(s) = default(helloInterval / 4);
        volatile double periodicJitter @unit(s) = default(uniform(0s, maxPeriodicJitter));
        double maxJitter @unit(s) = default(5ms);
        volatile double jitter @unit(s) = default(uniform(0ms, maxJitter));

        double helloInterval @unit(s) = default(1s);
        int allowedHelloLoss = default(2);
        double activeRouteTimeout @unit(s) = default(3s);
        int netDiameter = default(35);
        double nodeTraversalTime @unit(s) = default(0.04s);
        int rerrRatelimit = default(10);
        int rreqRetries = default(2);
        int rreqRatelimit = default(10);
        int timeoutBuffer = default(2);
        int ttlStart = default(2);
        int ttlIncrement = default(2);
        int ttlThreshold = default(7);
        int localAddTTL = default(2);

        double myRouteTimeout @unit(s) = default(2 * activeRouteTimeout);
        double deletePeriod @unit(s) = default(5 * max(activeRouteTimeout, helloInterval));
        double blacklistTimeout @unit(s) = default(rreqRetries * netTraversalTime);
        double netTraversalTime @unit(s) = default(2 * nodeTraversalTime * netDiameter);
        double nextHopWait @unit(s) = default(nodeTraversalTime + 0.01s);
        double pathDiscoveryTime @unit(s) = default(2 * netTraversalTime);

        // ----------------------------------------------------
        // [REP-AODV Redesign] Trust Engine Parameters
        // ----------------------------------------------------

        // -- Trust Policy --
        double alphaPrior = default(2.0);
        double betaPrior = default(1.0);
        int minObservations = default(5);
        double suspectThreshold = default(0.4);
        double quarantineThreshold = default(0.2);
        double recoveryThreshold = default(0.7);
        double recoveryWindow @unit(s) = default(10s);
        double quarantineWindow @unit(s) = default(60s);
        double evidenceHalfLife @unit(s) = default(60s);
        int maxQuarantineCycles = default(2);
        int minDirectEvidence = default(3);

        double trustShareInterval @unit(s) = default(10s);
        double gossipThreshold = default(0.35);

        // -- 2ACK Detector --
        double ackTimeout @unit(s) = default(0.5s);
        double ackSampleRate = default(0.35);
        double trustedSampleFactor = default(0.1);  // scale factor applied to ackSampleRate for rep > 0.8 neighbors
        bool enableTwoAck = default(true);

        // -- Anomaly Detection --
        bool enableAnomalyDetection = default(true);

        // -- Future Extension Flags --
        bool enablePathHealth = default(false);
        bool enableSeqNumAnomaly = default(false);

        // -- Attack Simulation (unchanged semantics) --
        bool isBlackhole = default(false);
        int blackholeSeqNumJump = default(10000);
        // ----------------------------------------------------

      gates:
        input socketIn;
        output socketOut;
}
