#pragma once

#include <map>
#include <set>
#include "Evidence.h"

namespace repaodv {
namespace trust {

/**
 * Отслеживает, какие соседи сообщили о целевом узле как о подозрительном.
 * Свидетельство считается подтверждённым только когда K или более независимых
 * источников подали сообщение в течение текущего временного окна.
 */
class GossipSourceTracker {
public:
    GossipSourceTracker() = default;

    void addReport(NeighborAddr reporter, NeighborAddr target, double now);

    bool hasSufficientSources(NeighborAddr target, int k) const;

    // Очищает все сообщения, если (now - lastReset) >= interval.
    void resetIfNeeded(double now, double interval);

private:
    std::map<NeighborAddr, std::set<NeighborAddr>> reporters; // цель → {отправители}
    double lastReset = 0.0;
};

} // namespace trust
} // namespace repaodv
