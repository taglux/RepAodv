#pragma once

#include <inet/routing/aodv/Aodv.h>
#include <inet/common/TagBase.h>
#include <memory>
#include <fstream>
#include <map>
#include <set>
#include "trust/TrustEngine.h"
#include "trust/detectors/TwoAckDetector.h"

namespace disserprotocol {
namespace aodv {

static constexpr int AODV_FORWARD_ACK_TYPE = 65;
static constexpr int AODV_TRUST_SHARE_TYPE = 66;

/**
 * @brief Тег, добавляемый к исходящим пакетам данных, когда запрашивается 2ACK.
 */
class TwoAckRequestTag : public inet::TagBase {
public:
    uint64_t requestId = 0;
    virtual TwoAckRequestTag *dup() const override { return new TwoAckRequestTag(*this); }
};

/**
 * @brief AODV с поддержкой репутации — протокол маршрутизации REP-AODV.
 */
class RepAodv : public inet::aodv::Aodv
{
  protected:
    std::unique_ptr<repaodv::trust::TrustEngine> trustEngine;
    std::unique_ptr<repaodv::trust::TwoAckDetector> twoAckDetector;

    bool isBlackhole = false;
    unsigned int blackholeSeqNumJump = 10000;
    uint64_t nextRequestId = 1;
    omnetpp::cMessage *periodicTickMsg = nullptr;
    omnetpp::cMessage *twoAckTimer = nullptr;
    omnetpp::cMessage *trustShareTimer = nullptr;
    std::multimap<std::pair<repaodv::trust::NeighborAddr, uint32_t>, inet::L3Address> ackRelayTable; // {(nextHop, packetId) -> prevHop}
    std::set<std::pair<uint32_t, uint32_t>> seenBhRreqs; // H8: дедупликация (rreqId, originator)
    double lastEmergencyBroadcastTime = -1e9; // Fix A: ограничение частоты экстренного gossip

  public:
    RepAodv() = default;
    virtual ~RepAodv() override;

  protected:
    static repaodv::trust::NeighborAddr toAddr(const inet::L3Address& a);
    static inet::L3Address fromAddr(repaodv::trust::NeighborAddr a);

    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(omnetpp::cMessage *msg) override;
    virtual void finish() override;

    virtual void socketDataArrived(inet::UdpSocket *socket, inet::Packet *packet) override;

    virtual void receiveSignal(omnetpp::cComponent *source, omnetpp::simsignal_t signalID, omnetpp::cObject *obj, omnetpp::cObject *details) override;
    virtual inet::INetfilter::IHook::Result datagramPreRoutingHook(inet::Packet *datagram) override;
    virtual inet::INetfilter::IHook::Result datagramForwardHook(inet::Packet *datagram) override;
    virtual inet::INetfilter::IHook::Result datagramLocalOutHook(inet::Packet *datagram) override;
    virtual inet::INetfilter::IHook::Result datagramLocalInHook(inet::Packet *datagram) override;

    virtual void buildTrustEngine();
    virtual void logEvent(const std::string& eventType, const std::string& details);
    virtual void onTrustStateChange(repaodv::trust::NeighborAddr neighbor,
                                    repaodv::trust::TrustState oldState,
                                    repaodv::trust::TrustState newState,
                                    double now);

    void sendTwoAck(inet::Packet *datagram);
    double adaptiveSampleRate(const repaodv::trust::NeighborAddr& neighbor) const;
    void sendTrustBroadcast();
    void handleTrustShare(inet::Packet *packet);
    uint32_t getPacketId(inet::Packet *datagram);
    inet::L3Address getPrevHop(inet::Packet *datagram);
    void scheduleNextTwoAckTimeout();

    static std::ofstream& logStream();
    static int& activeInstanceCount();
};

} // namespace aodv
} // namespace disserprotocol
