#include "GossipSourceTracker.h"

namespace repaodv {
namespace trust {

void GossipSourceTracker::addReport(NeighborAddr reporter, NeighborAddr target, double now)
{
    if (lastReset == 0.0)
        lastReset = now;
    reporters[target].insert(reporter);
}

bool GossipSourceTracker::hasSufficientSources(NeighborAddr target, int k) const
{
    auto it = reporters.find(target);
    if (it == reporters.end())
        return false;
    return static_cast<int>(it->second.size()) >= k;
}

void GossipSourceTracker::resetIfNeeded(double now, double interval)
{
    if (now - lastReset >= interval) {
        reporters.clear();
        lastReset = now;
    }
}

} // namespace trust
} // namespace repaodv
