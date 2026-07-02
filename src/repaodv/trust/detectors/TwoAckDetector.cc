#include "TwoAckDetector.h"
#include "../TrustEngine.h"

namespace repaodv {
namespace trust {

TwoAckDetector::TwoAckDetector(TrustEngine& engine) : engine(engine) {}

void TwoAckDetector::onPacketSent(PacketId packetId, NeighborAddr neighbor, double deadline) {
    auto key = std::make_pair(neighbor, packetId);
    pending[key] = deadline;
    timeouts.emplace(deadline, key);
}

void TwoAckDetector::onAckReceived(PacketId packetId, NeighborAddr fromNeighbor, double now) {
    auto it = pending.find({fromNeighbor, packetId});
    if (it != pending.end()) {
        engine.updateEvidence(fromNeighbor, EvidenceType::FORWARDING, true, now);
        pending.erase(it);
    }
}

void TwoAckDetector::processTimeouts(double currentTime) {
    while (!timeouts.empty()) {
        auto it = timeouts.begin();
        // Добавляем эпсилон 1e-8, чтобы учесть потерю точности double против simtime_t
        if (it->first > currentTime + 1e-8) {
            break; // Истёкших таймаутов больше нет
        }

        double deadline = it->first;
        auto key = it->second;
        timeouts.erase(it);

        auto pIt = pending.find(key);
        // Обрабатываем только если запись не была удалена и дедлайн совпадает
        // (на случай, когда тот же ключ был добавлен заново с новым дедлайном)
        if (pIt != pending.end() && pIt->second == deadline) {
            // Примечание: RepAodv залогирует это через callback updateEvidence
            engine.updateEvidence(key.first, EvidenceType::FORWARDING, false, currentTime);
            if (onTimeout) onTimeout(key.second, key.first);
            pending.erase(pIt);
        }
    }
}

void TwoAckDetector::cancelForNeighbor(NeighborAddr neighbor)
{
    auto it = pending.lower_bound({neighbor, 0});
    while (it != pending.end() && it->first.first == neighbor) {
        it = pending.erase(it);
    }
    // Записи в multimap timeouts для этого соседа становятся устаревшими — processTimeouts()
    // и так пропускает записи, отсутствующие в pending, поэтому дополнительная очистка не нужна.
}

double TwoAckDetector::getNextTimeout() const {
    // Нужно отсеять устаревшие таймауты, чтобы найти настоящий ближайший
    for (auto it = timeouts.begin(); it != timeouts.end(); ++it) {
        auto pIt = pending.find(it->second);
        if (pIt != pending.end() && pIt->second == it->first) {
            return it->first;
        }
    }
    return -1.0;
}

} // namespace trust
} // namespace repaodv
