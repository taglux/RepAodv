#include "TrustEngine.h"
#include <stdexcept>

namespace repaodv {
namespace trust {

TrustEngine::TrustEngine(std::unique_ptr<TrustPolicy> policy)
    : policy(std::move(policy))
{
}

void TrustEngine::updateEvidence(NeighborAddr neighbor, EvidenceType type, bool success, double now)
{
    TrustView& view = getOrCreateView(neighbor);
    TrustState oldState = view.state;

    policy->update(view, success, now, type);

    if (callback) {
        callback(neighbor, oldState, view.state);
    }
}

TrustState TrustEngine::getState(NeighborAddr neighbor) const
{
    auto it = views.find(neighbor);
    if (it != views.end()) {
        return it->second.state;
    }
    return TrustState::TRUSTED;
}

double TrustEngine::getReputation(NeighborAddr neighbor) const
{
    auto it = views.find(neighbor);
    if (it != views.end()) {
        return it->second.getReputation();
    }
    return policy->getAlphaPrior() / (policy->getAlphaPrior() + policy->getBetaPrior());
}

void TrustEngine::decayAll(double now)
{
    for (auto& pair : views) {
        TrustState oldState = pair.second.state;
        policy->decay(pair.second, now);
        policy->tick(pair.second, now);
        if (callback && pair.second.state != oldState) {
            callback(pair.first, oldState, pair.second.state);
        }
    }
}

const TrustView& TrustEngine::getView(NeighborAddr neighbor) const
{
    auto it = views.find(neighbor);
    if (it != views.end()) {
        return it->second;
    }
    throw std::runtime_error("TrustEngine::getView: unknown neighbor");
}

TrustView& TrustEngine::getOrCreateView(NeighborAddr neighbor)
{
    auto it = views.find(neighbor);
    if (it == views.end()) {
        TrustView newView;
        newView.neighbor = neighbor;
        newView.alpha = policy->getAlphaPrior();
        newView.beta = policy->getBetaPrior();
        return views.emplace(neighbor, newView).first->second;
    }
    return it->second;
}

} // namespace trust
} // namespace repaodv
