import inet.routing.aodv.AodvControlPackets;

namespace inet::aodv;

class AodvForwardAck extends AodvControlPacket
{
    uint64_t packetId;
    L3Address originSender;
}

struct TrustShareEntry {
    L3Address nodeId;
    double reputationScore;
}

class AodvTrustShare extends AodvControlPacket {
    TrustShareEntry entries[];
}
