#include "Aodv.h"

#include <inet/common/ModuleAccess.h>
#include <inet/common/packet/Packet.h>
#include <inet/networklayer/common/L3AddressTag_m.h>
#include <inet/networklayer/common/HopLimitTag_m.h>
#include <inet/networklayer/common/NextHopAddressTag_m.h>
#include <inet/networklayer/common/L3Tools.h>
#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include <inet/routing/aodv/AodvControlPackets_m.h>
#include <inet/linklayer/common/MacAddressTag_m.h>
#include <inet/networklayer/contract/IArp.h>

#include "repAodv_m.h"

namespace disserprotocol {
namespace aodv {

using namespace inet;
using namespace inet::aodv;
using namespace repaodv::trust;

Define_Module(RepAodv);

RepAodv::~RepAodv()
{
    cancelAndDelete(periodicTickMsg);
    cancelAndDelete(twoAckTimer);
    cancelAndDelete(trustShareTimer);
}

repaodv::trust::NeighborAddr RepAodv::toAddr(const L3Address& a)
{
    if (a.getType() == L3Address::IPv4) return a.toIpv4().getInt();
    return 0;
}

L3Address RepAodv::fromAddr(repaodv::trust::NeighborAddr a)
{
    return L3Address(Ipv4Address(a));
}

void RepAodv::scheduleNextTwoAckTimeout()
{
    double nextTimeout = twoAckDetector->getNextTimeout();
    if (nextTimeout >= 0.0) {
        // Не планируем в прошлом из-за погрешностей вычислений с плавающей точкой
        if (nextTimeout < simTime().dbl()) {
            nextTimeout = simTime().dbl();
        }
        
        if (twoAckTimer->isScheduled()) {
            if (twoAckTimer->getArrivalTime().dbl() > nextTimeout) {
                cancelEvent(twoAckTimer);
                scheduleAt(nextTimeout, twoAckTimer);
            }
        } else {
            scheduleAt(nextTimeout, twoAckTimer);
        }
    }
}

void RepAodv::initialize(int stage)
{
    Aodv::initialize(stage);

    if (stage == INITSTAGE_ROUTING_PROTOCOLS) {
        isBlackhole = par("isBlackhole");
        buildTrustEngine();
        
        periodicTickMsg = new cMessage("trustTick");
        scheduleAt(simTime() + 0.25, periodicTickMsg);

        twoAckTimer = new cMessage("twoAckTimer");

        trustShareTimer = new cMessage("trustShareTimer");
        scheduleAt(simTime() + uniform(0, par("trustShareInterval").doubleValue()), trustShareTimer);
        
        activeInstanceCount()++;
        logEvent("NODE_START", isBlackhole ? "type=blackhole" : "type=honest");
    }
}

void RepAodv::handleMessageWhenUp(cMessage *msg)
{
    if (msg == periodicTickMsg) {
        double now = simTime().dbl();
        trustEngine->decayAll(now);
        scheduleAt(simTime() + 0.25, periodicTickMsg);
    } else if (msg == twoAckTimer) {
        double now = simTime().dbl();
        twoAckDetector->processTimeouts(now);
        scheduleNextTwoAckTimeout();
    } else if (msg == trustShareTimer) {
        sendTrustBroadcast();
        scheduleAt(simTime() + par("trustShareInterval").doubleValue(), trustShareTimer);
    } else {
        Aodv::handleMessageWhenUp(msg);
    }
}

void RepAodv::finish()
{
    logEvent("NODE_STOP", "");
    activeInstanceCount()--;
    if (activeInstanceCount() == 0) {
        logStream().close();
    }
    omnetpp::cSimpleModule::finish();
}

void RepAodv::sendTrustBroadcast()
{
    auto sharePkt = makeShared<AodvTrustShare>();
    sharePkt->setPacketType((AodvControlPacketType)AODV_TRUST_SHARE_TYPE);
    
    int count = 0;
    for (const auto& pair : trustEngine->getAllViews()) {
        if (pair.second.state >= TrustState::SUSPECT) {
            TrustShareEntry entry;
            entry.nodeId = fromAddr(pair.first);
            entry.reputationScore = pair.second.getReputation();
            sharePkt->insertEntries(count++, entry);
        }
    }

    if (count > 0) {
        sharePkt->setChunkLength(B(4 + count * 8)); // Оценка длины
        sendAODVPacket(sharePkt, L3Address(Ipv4Address::ALLONES_ADDRESS), 1, 0);
        logEvent("TRUST_SHARE_SEND", "entries=" + std::to_string(count));
    }
}

void RepAodv::handleTrustShare(Packet *packet)
{
    auto sourceAddrTag = packet->findTag<L3AddressInd>();
    L3Address srcAddr = sourceAddrTag ? sourceAddrTag->getSrcAddress() : L3Address();
    
    // H5: принимаем gossip только от TRUSTED-источников — SUSPECT-источники могут распространять ложные негативы
    if (trustEngine->getState(toAddr(srcAddr)) != TrustState::TRUSTED) return;

    auto share = packet->peekAtFront<AodvTrustShare>();
    int n = share->getEntriesArraySize();
    double now = simTime().dbl();
    double gossipThresh = par("gossipThreshold").doubleValue();

    for (int i = 0; i < n; i++) {
        const auto& entry = share->getEntries(i);
        // Пропускаем gossip о себе и проверяем порог
        if (entry.nodeId != getSelfIPAddress() && entry.reputationScore < gossipThresh) {
            trustEngine->updateEvidence(toAddr(entry.nodeId), EvidenceType::SIGNED_HINT, false, now);
        }
    }
    
    logEvent("TRUST_SHARE_RECV", "src=" + srcAddr.str() + ",entries=" + std::to_string(n));
}

void RepAodv::socketDataArrived(UdpSocket *socket, Packet *packet)
{
    const auto& controlPacket = packet->peekAtFront<AodvControlPacket>(b(-1), Chunk::PF_ALLOW_NULLPTR);
    if (!controlPacket) {
        Aodv::socketDataArrived(socket, packet);
        return;
    }

    auto pktType = controlPacket->getPacketType();
    auto sourceAddrTag = packet->findTag<L3AddressInd>();
    L3Address sourceAddr = sourceAddrTag ? sourceAddrTag->getSrcAddress() : L3Address();

    // Подтверждение пересылки 2ACK
    if (par("enableTwoAck").boolValue() && pktType == (AodvControlPacketType)AODV_FORWARD_ACK_TYPE) {
        auto ack = packet->peekAtFront<AodvForwardAck>();
        uint32_t packetId = ack->getPacketId();
        logEvent("2ACK_RECV", "from=" + sourceAddr.str() + ",id=" + std::to_string(packetId));
        twoAckDetector->onAckReceived(packetId, toAddr(sourceAddr), simTime().dbl());
        
        auto relayKey = std::make_pair(toAddr(sourceAddr), packetId);
        auto relayIt = ackRelayTable.find(relayKey);
        if (relayIt != ackRelayTable.end()) {
            auto relayAck = makeShared<AodvForwardAck>();
            relayAck->setChunkLength(B(20));
            relayAck->setPacketType((AodvControlPacketType)AODV_FORWARD_ACK_TYPE);
            relayAck->setPacketId(packetId);
            relayAck->setOriginSender(getSelfIPAddress());
            sendAODVPacket(relayAck, relayIt->second, 255, 0);
            logEvent("2ACK_RELAY", "to=" + relayIt->second.str() + ",id=" + std::to_string(packetId));
            ackRelayTable.erase(relayIt);
        }
        delete packet;
        return;
    }

    // Обмен репутацией (Trust Share)
    if (pktType == (AodvControlPacketType)AODV_TRUST_SHARE_TYPE) {
        handleTrustShare(packet);
        delete packet;
        return;
    }

    // Blackhole: перехват RREQ — handleRREQ в INET не виртуальный, перехватываем здесь
    if (isBlackhole && (pktType == RREQ || pktType == RREQ_IPv6)) {
        const auto& rreq = packet->peekAtFront<Rreq>();
        if (rreq && !sourceAddr.isUnspecified()) {
            // H8: отвечаем только на первый уникальный RREQ (originator,rreqId), чтобы избежать потока RREP
            auto key = std::make_pair(rreq->getRreqId(),
                                      rreq->getOriginatorAddr().toIpv4().getInt());
            if (seenBhRreqs.count(key) == 0) {
                seenBhRreqs.insert(key);
                logEvent("FAKE_RREP_SENT", "dest=" + rreq->getDestAddr().str());
                auto fakeRrep = makeShared<Rrep>();
                fakeRrep->setChunkLength(B(20));
                fakeRrep->setPacketType(RREP);
                fakeRrep->setDestAddr(rreq->getDestAddr());
                fakeRrep->setDestSeqNum(rreq->getDestSeqNum() + blackholeSeqNumJump);
                fakeRrep->setOriginatorAddr(rreq->getOriginatorAddr());
                fakeRrep->setHopCount(0);
                fakeRrep->setLifeTime(myRouteTimeout);
                sendAODVPacket(fakeRrep, sourceAddr, 1, 0);
            }
        }
        delete packet;
        return;
    }

    // B1/B2: перехват RREP — handleRREP в INET не виртуальный, перехватываем здесь
    if (pktType == RREP || pktType == RREP_IPv6) {
        const auto& rrep = packet->peekAtFront<Rrep>();
        if (rrep) {
            // B1: блокируем маршруты от next-hop, находящегося в карантине
            if (trustEngine->getState(toAddr(sourceAddr)) >= TrustState::QUARANTINE) {
                logEvent("RREP_DROP", "reason=quarantined,src=" + sourceAddr.str());
                delete packet;
                return;
            }
            // F4: B2 удалён — проверял rrep->getDestAddr() (легитимный получатель, а не отправитель-blackhole),
            // поэтому никогда не срабатывал для фальшивых RREP и вызывал ложные отбрасывания RREP.
        }
    }

    // RERR: сбрасываем «отравленные» seqnum после обработки базовым классом.
    // Маршрут разорван → мы больше не знаем реальный seqnum получателя.
    // Установка hasValidDestNum=false заставляет следующий RREQ использовать unknownSeqNumFlag=true,
    // позволяя честным узлам отвечать независимо от значения seqnum.
    if (pktType == RERR || pktType == RERR_IPv6) {
        const auto& rerr = packet->peekAtFront<Rerr>(b(-1), Chunk::PF_ALLOW_NULLPTR);
        std::vector<L3Address> unreachableDests;
        if (rerr) {
            for (unsigned int i = 0; i < rerr->getUnreachableNodesArraySize(); i++)
                unreachableDests.push_back(rerr->getUnreachableNodes(i).addr);
        }
        Aodv::socketDataArrived(socket, packet);
        for (const auto& dest : unreachableDests) {
            IRoute *route = routingTable->findBestMatchingRoute(dest);
            if (route && route->getSource() == this) {
                auto *rd = dynamic_cast<AodvRouteData *>(route->getProtocolData());
                if (rd && !rd->isActive())
                    rd->setHasValidDestNum(false);
            }
        }
        return;
    }

    Aodv::socketDataArrived(socket, packet);
}

INetfilter::IHook::Result RepAodv::datagramPreRoutingHook(Packet *datagram)
{
    if (!par("enableTwoAck").boolValue()) return Aodv::datagramPreRoutingHook(datagram);
    
    auto l3Tag = datagram->findTag<L3AddressInd>();
    if (l3Tag) {
        L3Address src = l3Tag->getSrcAddress();
        if (trustEngine->getState(toAddr(src)) >= TrustState::QUARANTINE) {
            logEvent("DATAGRAM_DROP", "reason=quarantined_src,src=" + src.str());
            return INetfilter::IHook::DROP;
        }
    }
    return Aodv::datagramPreRoutingHook(datagram);
}

L3Address RepAodv::getPrevHop(Packet *datagram)
{
    auto macInd = datagram->findTag<MacAddressInd>();
    if (macInd) {
        MacAddress prevMac = macInd->getSrcAddress();
        cModule *host = getContainingNode(this);
        if (host) {
            IArp *arp = dynamic_cast<IArp *>(host->getSubmodule("ipv4")->getSubmodule("arp"));
            if (arp) {
                return arp->getL3AddressFor(prevMac);
            }
        }
    }
    return L3Address();
}

uint32_t RepAodv::getPacketId(Packet *datagram)
{
    auto ipv4Hdr = dynamic_pointer_cast<const Ipv4Header>(getNetworkProtocolHeader(datagram));
    if (ipv4Hdr) return ipv4Hdr->getIdentification();

    try {
        const auto& hdr = datagram->peekAtFront<Ipv4Header>();
        if (hdr) return hdr->getIdentification();
    } catch (...) {}

    return 0;
}

double RepAodv::adaptiveSampleRate(const NeighborAddr& neighbor) const
{
    double base = par("ackSampleRate").doubleValue();
    double rep  = trustEngine->getReputation(neighbor);
    if (rep > 0.8)
        return base * par("trustedSampleFactor").doubleValue();
    if (rep < par("suspectThreshold").doubleValue()) {
        double increased = base * 2.5;
        return increased > 1.0 ? 1.0 : increased;
    }
    return base;
}

void RepAodv::sendTwoAck(Packet *datagram)
{
    Enter_Method_Silent();
    if (!par("enableTwoAck").boolValue()) return;

    uint32_t packetId = getPacketId(datagram);
    if (packetId == 0) return;

    L3Address immediatePrevHop;
    auto macInd = datagram->findTag<MacAddressInd>();
    if (macInd) {
        MacAddress prevMac = macInd->getSrcAddress();
        cModule *host = getContainingNode(this);
        if (host) {
            IArp *arp = dynamic_cast<IArp *>(host->getSubmodule("ipv4")->getSubmodule("arp"));
            if (arp) {
                immediatePrevHop = arp->getL3AddressFor(prevMac);
            }
        }
    }

    const auto& networkHeader = getNetworkProtocolHeader(datagram);
    L3Address srcAddr = networkHeader ? networkHeader->getSourceAddress() : L3Address();

    if (immediatePrevHop.isUnspecified()) {
        IRoute *reverseRoute = routingTable->findBestMatchingRoute(srcAddr);
        if (reverseRoute && reverseRoute->getSource() == this) {
            immediatePrevHop = reverseRoute->getNextHopAsGeneric();
        }
    }

    if (!immediatePrevHop.isUnspecified()) {
        auto ack = makeShared<AodvForwardAck>();
        ack->setChunkLength(B(20));
        ack->setPacketType((AodvControlPacketType)AODV_FORWARD_ACK_TYPE);
        ack->setPacketId(packetId);
        ack->setOriginSender(getSelfIPAddress());
        
        sendAODVPacket(ack, immediatePrevHop, 255, 0);
        logEvent("2ACK_SENT", "to=" + immediatePrevHop.str() + ",id=" + std::to_string(packetId));
    }
}

INetfilter::IHook::Result RepAodv::datagramForwardHook(Packet *datagram)
{
    Enter_Method_Silent();
    const auto& networkHeader = getNetworkProtocolHeader(datagram);
    L3Address srcAddr = networkHeader ? networkHeader->getSourceAddress() : L3Address();
    L3Address destAddr = networkHeader ? networkHeader->getDestinationAddress() : L3Address();

    if (isBlackhole) {
        logEvent("DATAGRAM_BLACKHOLE_DROP", "src=" + srcAddr.str() + ",dest=" + destAddr.str());
        return INetfilter::IHook::DROP;
    }

    // Сначала вызываем базовый класс, чтобы AODV разрешил/установил NextHopAddressReq
    auto result = Aodv::datagramForwardHook(datagram);
    if (result != INetfilter::IHook::ACCEPT) return result;

    if (datagram->getByteLength() > 128) {
        auto nextHopTag = datagram->findTag<NextHopAddressReq>();
        L3Address nextHop = nextHopTag ? nextHopTag->getNextHopAddress() : L3Address();
        
        // Запасной вариант — таблица маршрутизации, если тег отсутствует
        if (nextHop.isUnspecified()) {
            IRoute *route = routingTable->findBestMatchingRoute(destAddr);
            if (route) nextHop = route->getNextHopAsGeneric();
        }

        logEvent("DATA_FORWARD", "src=" + srcAddr.str() + ",dest=" + destAddr.str() + ",nextHop=" + nextHop.str());

        if (par("enableTwoAck").boolValue() && !nextHop.isUnspecified()) {
            if (trustEngine->getState(toAddr(nextHop)) >= TrustState::QUARANTINE) {
                logEvent("DATA_DROP", "reason=quarantined_next,next=" + nextHop.str());
                handleLinkBreakSendRERR(destAddr);
                return INetfilter::IHook::DROP;
            }

            auto ipv4Hdr = datagram->peekAtFront<Ipv4Header>();
            if (ipv4Hdr) {
                bool ackRequested = (ipv4Hdr->getTypeOfService() & 0x80) != 0;
                uint32_t packetId = ipv4Hdr->getIdentification();

                double ttl = ipv4Hdr->getTimeToLive();
                double timeout = par("ackTimeout").doubleValue() + (ttl * 0.01);

                bool requestAck = (uniform(0, 1) < adaptiveSampleRate(toAddr(nextHop)));
                if (ackRequested) {
                    // Немедленно отправляем ACK вверх по маршруту (доказывает, что мы пересылаем)
                    sendTwoAck(datagram);
                    if (requestAck) {
                        // ретранслируем ACK от нижестоящего узла обратно вверх, когда он придёт
                        L3Address prevHop = getPrevHop(datagram);
                        if (!prevHop.isUnspecified() && packetId != 0) {
                            ackRelayTable.emplace(std::make_pair(toAddr(nextHop), packetId), prevHop);
                        }
                    } else {
                        // прерываем каскад: сбрасываем бит ACK, чтобы следующий узел не запускал его повторно
                        auto mutableIpv4Hdr = dynamic_pointer_cast<Ipv4Header>(datagram->removeAtFront<Ipv4Header>()->dupShared());
                        mutableIpv4Hdr->setTypeOfService(mutableIpv4Hdr->getTypeOfService() & ~0x80);
                        datagram->insertAtFront(mutableIpv4Hdr);
                    }
                }

                if (requestAck) {
                    auto mutableIpv4Hdr = dynamic_pointer_cast<Ipv4Header>(datagram->removeAtFront<Ipv4Header>()->dupShared());
                    mutableIpv4Hdr->setTypeOfService(mutableIpv4Hdr->getTypeOfService() | 0x80);
                    twoAckDetector->onPacketSent(packetId, toAddr(nextHop), simTime().dbl() + timeout);
                    scheduleNextTwoAckTimeout();
                    datagram->insertAtFront(mutableIpv4Hdr);
                }
            }
        }
    }

    return result;
}

INetfilter::IHook::Result RepAodv::datagramLocalOutHook(Packet *datagram)
{
    Enter_Method_Silent();
    const auto& networkHeader = getNetworkProtocolHeader(datagram);
    L3Address destAddr = networkHeader ? networkHeader->getDestinationAddress() : L3Address();

    uint16_t packetId = 0;
    if (datagram->getByteLength() > 128) {
        auto ipv4Hdr = datagram->peekAtFront<Ipv4Header>();
        if (ipv4Hdr) {
            auto mutableHdr = dynamic_pointer_cast<Ipv4Header>(datagram->removeAtFront<Ipv4Header>()->dupShared());
            if (mutableHdr->getIdentification() == 0)
                mutableHdr->setIdentification(nextRequestId++);
            packetId = mutableHdr->getIdentification();
            datagram->insertAtFront(mutableHdr);
        }
    }

    // Вызываем базовый класс
    auto result = Aodv::datagramLocalOutHook(datagram);
    
    if (result != INetfilter::IHook::ACCEPT) return result;

    if (datagram->getByteLength() > 128) {
        auto nextHopTag = datagram->findTag<NextHopAddressReq>();
        L3Address nextHop = nextHopTag ? nextHopTag->getNextHopAddress() : L3Address();

        // Запасной вариант — таблица маршрутизации
        if (nextHop.isUnspecified()) {
            IRoute *route = routingTable->findBestMatchingRoute(destAddr);
            if (route) nextHop = route->getNextHopAsGeneric();
        }

        logEvent("DATA_SEND", "src=" + getSelfIPAddress().str() + ",dest=" + destAddr.str() + ",nextHop=" + nextHop.str() + ",id=" + std::to_string(packetId));

        if (par("enableTwoAck").boolValue() && !nextHop.isUnspecified()) {
            if (trustEngine->getState(toAddr(nextHop)) >= TrustState::QUARANTINE) {
                logEvent("DATA_DROP", "reason=quarantined_next,next=" + nextHop.str());
                handleLinkBreakSendRERR(destAddr);
                return INetfilter::IHook::DROP;
            }

            auto ipv4Hdr = datagram->peekAtFront<Ipv4Header>();
            double ttl = ipv4Hdr ? ipv4Hdr->getTimeToLive() : 64;
            double timeout = par("ackTimeout").doubleValue() + (ttl * 0.01);

            bool requestAck = (uniform(0, 1) < adaptiveSampleRate(toAddr(nextHop)));
            if (requestAck) {
                auto mutableHdr = dynamic_pointer_cast<Ipv4Header>(datagram->removeAtFront<Ipv4Header>()->dupShared());
                mutableHdr->setTypeOfService(mutableHdr->getTypeOfService() | 0x80);
                twoAckDetector->onPacketSent(packetId, toAddr(nextHop), simTime().dbl() + timeout);
                scheduleNextTwoAckTimeout();
                datagram->insertAtFront(mutableHdr);
            }
        }
    }

    return result;
}

INetfilter::IHook::Result RepAodv::datagramLocalInHook(Packet *datagram)
{
    Enter_Method_Silent();
    const auto& networkHeader = getNetworkProtocolHeader(datagram);
    L3Address srcAddr = networkHeader ? networkHeader->getSourceAddress() : L3Address();
    L3Address destAddr = networkHeader ? networkHeader->getDestinationAddress() : L3Address();

    if (datagram->getByteLength() > 128) {
        auto ipv4Hdr = datagram->peekAtFront<Ipv4Header>();
        uint16_t packetId = ipv4Hdr ? ipv4Hdr->getIdentification() : 0;
        logEvent("DATA_RECV", "src=" + srcAddr.str() + ",dest=" + destAddr.str() + ",id=" + std::to_string(packetId));
        if (par("enableTwoAck").boolValue()) {
            if (ipv4Hdr && (ipv4Hdr->getTypeOfService() & 0x80) != 0) {
                sendTwoAck(datagram);
            }
        }
    }

    return Aodv::datagramLocalInHook(datagram);
}

void RepAodv::buildTrustEngine()
{
    repaodv::trust::PolicyConfig config;
    config.alphaPrior = par("alphaPrior");
    config.betaPrior = par("betaPrior");
    config.minObservations = par("minObservations");
    config.suspectThreshold = par("suspectThreshold");
    config.quarantineThreshold = par("quarantineThreshold");
    config.recoveryThreshold = par("recoveryThreshold");
    config.recoveryWindow = par("recoveryWindow");
    config.quarantineWindow = par("quarantineWindow");
    config.evidenceHalfLife = par("evidenceHalfLife");
    config.maxQuarantineCycles = par("maxQuarantineCycles");
    config.minDirectEvidence = par("minDirectEvidence");

    auto policy = std::make_unique<TrustPolicy>(config);
    
    trustEngine = std::make_unique<TrustEngine>(std::move(policy));
    twoAckDetector = std::make_unique<TwoAckDetector>(*trustEngine);

    // Перенаправление обвинения: когда C (nextHop) не подтверждает ACK для B (нас), отправляем
    // ретранслированный ACK узлу A (prevHop), чтобы A зафиксировал положительное свидетельство о B,
    // а не ошибочно обвинил B в отбрасывании, совершённом C.
    twoAckDetector->onTimeout = [this](repaodv::trust::PacketId packetId, repaodv::trust::NeighborAddr timedOutNeighbor) {
        auto relayKey = std::make_pair(timedOutNeighbor, (uint32_t)packetId);
        auto range = ackRelayTable.equal_range(relayKey);
        for (auto it = range.first; it != range.second; ) {
            L3Address prevHop = it->second;
            auto relayAck = makeShared<AodvForwardAck>();
            relayAck->setChunkLength(B(20));
            relayAck->setPacketType((AodvControlPacketType)AODV_FORWARD_ACK_TYPE);
            relayAck->setPacketId((uint32_t)packetId);
            relayAck->setOriginSender(getSelfIPAddress());
            sendAODVPacket(relayAck, prevHop, 255, 0);
            logEvent("2ACK_RELAY_RECOVER", "to=" + prevHop.str() + ",failedNext=" + fromAddr(timedOutNeighbor).str());
            it = ackRelayTable.erase(it);
        }
    };

    trustEngine->onStateChange([this](NeighborAddr addr, TrustState oldS, TrustState newS) {
        if (newS != oldS) {
            this->onTrustStateChange(addr, oldS, newS, simTime().dbl());
        }
        
        // Логируем обновление репутации каждый раз при её пересчёте
        const auto& view = trustEngine->getView(addr);
        logEvent("REPUTATION_UPDATE", "neighbor=" + fromAddr(addr).str() + 
                 ",rep=" + std::to_string(view.getReputation()) + 
                 ",a=" + std::to_string(view.alpha) + 
                 ",b=" + std::to_string(view.beta));
    });
}

void RepAodv::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    // Отменяем ожидающие записи 2ACK для разорванного канала ДО того, как базовый класс
    // аннулирует маршруты — чтобы мы ещё могли определить следующий узел.
    if (signalID == linkBrokenSignal && twoAckDetector) {
        auto *datagram = dynamic_cast<Packet *>(obj);
        if (datagram) {
            const auto& netHdr = findNetworkProtocolHeader(datagram);
            if (netHdr) {
                L3Address dest = netHdr->getDestinationAddress();
                IRoute *route = routingTable->findBestMatchingRoute(dest);
                if (route && route->getSource() == this) {
                    L3Address nh = route->getNextHopAsGeneric();
                    twoAckDetector->cancelForNeighbor(toAddr(nh));
                    logEvent("LINK_BREAK_CANCEL", "neighbor=" + nh.str());
                }
            }
        }
    }
    Aodv::receiveSignal(source, signalID, obj, details);
}

void RepAodv::onTrustStateChange(NeighborAddr neighbor, TrustState /*oldState*/, TrustState newState, double /*now*/)
{
    L3Address nh = fromAddr(neighbor);
    logEvent("TRUST_STATE_CHANGE", "neighbor=" + nh.str() + ",state=" + std::to_string((int)newState));

    if (newState >= TrustState::QUARANTINE) {
        double now = simTime().dbl();
        double interval = par("trustShareInterval").doubleValue();
        if (now - lastEmergencyBroadcastTime >= interval) {
            lastEmergencyBroadcastTime = now;
            sendTrustBroadcast();
        }
        
        std::vector<L3Address> affectedDestinations;
        for (int i = routingTable->getNumRoutes() - 1; i >= 0; i--) {
            IRoute *r = routingTable->getRoute(i);
            if (r->getNextHopAsGeneric() == nh) {
                affectedDestinations.push_back(r->getDestinationAsGeneric());
            }
        }

        // Генерируем RERR для каждого аннулированного получателя, чтобы уведомить вышестоящие узлы.
        // Делаем это ДО удаления из routingTable, чтобы базовый класс Aodv мог найти предшественников
        for (const auto& dest : affectedDestinations) {
            logEvent("ROUTE_INVALIDATED", "dest=" + dest.str() + ",via=" + nh.str());
            handleLinkBreakSendRERR(dest);
        }

        // Теперь удалять безопасно
        for (int i = routingTable->getNumRoutes() - 1; i >= 0; i--) {
            IRoute *r = routingTable->getRoute(i);
            if (r->getNextHopAsGeneric() == nh) {
                routingTable->deleteRoute(r);
            }
        }

    }
}

void RepAodv::logEvent(const std::string& eventType, const std::string& details)
{
    std::string nodeId = getParentModule() ? getParentModule()->getFullName() : getFullName();
    logStream() << simTime().dbl() << ";" << nodeId << ";" 
                << (isBlackhole ? "Blackhole" : "Honest") << ";" 
                << eventType << ";" << details << "\n";
}

std::ofstream& RepAodv::logStream()
{
    static std::ofstream stream("result/simulation_log.csv");
    return stream;
}

int& RepAodv::activeInstanceCount()
{
    static int count = 0;
    return count;
}

} // namespace aodv
} // namespace disserprotocol
