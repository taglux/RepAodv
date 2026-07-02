#include "TrustPolicy.h"
#include <cmath>

namespace repaodv {
namespace trust {

TrustPolicy::TrustPolicy(const PolicyConfig& config)
    : config(config)
{
}

void TrustPolicy::update(TrustView& view, bool success, double now, EvidenceType type)
{
    double weight = (type == EvidenceType::SIGNED_HINT) ? 0.5 : 1.0;
    if (success) {
        view.alpha += weight;
    } else {
        view.beta += weight;
        if (type == EvidenceType::FORWARDING) {
            view.directNegativeCount++;
        }
    }
    view.lastObservation = now;

    // Эффективное число наблюдений (без учёта априорных значений)
    double n = (view.alpha + view.beta) - (config.alphaPrior + config.betaPrior);
    int effectiveMinObs = (view.directNegativeCount > 0) ? 2 : config.minObservations;
    if (n < (double)effectiveMinObs) {
        return;
    }

    double reputation = view.getReputation();

    // Переходы состояний на основе репутации
    if (view.state == TrustState::TRUSTED) {
        if (reputation < config.suspectThreshold) {
            view.state = TrustState::SUSPECT;
        }
    } else if (view.state == TrustState::SUSPECT) {
        if (reputation < config.quarantineThreshold) {
            view.state = TrustState::QUARANTINE;
            view.stateEnteredAt = now;
        } else if (reputation > config.recoveryThreshold) {
            view.state = TrustState::TRUSTED;
        }
    } else if (view.state == TrustState::QUARANTINE) {
        // Предохранительный клапан: прямое положительное свидетельство пересылки может вернуть
        // вызванный gossip-ом карантин обратно в SUSPECT (для реальной BH не срабатывает, так как она всё отбрасывает).
        if (success && type == EvidenceType::FORWARDING && view.directNegativeCount == 0
                && reputation > config.recoveryThreshold) {
            view.state = TrustState::SUSPECT;
            view.stateEnteredAt = 0.0;
        }
    }
}

void TrustPolicy::tick(TrustView& view, double now)
{
    if (view.state != TrustState::QUARANTINE) return;
    if (now - view.stateEnteredAt < config.quarantineWindow) return;

    // Карантин только по gossip (нет прямых отрицательных свидетельств): продлеваем окно вместо
    // реабилитации — это не даёт вернуть BH в таблицу маршрутизации. Выход — через
    // положительное прямое свидетельство в update().
    if (view.directNegativeCount == 0) {
        view.stateEnteredAt = now;
        return;
    }

    view.quarantineCycles++;
    bool hasSufficientDirectEvidence = (view.directNegativeCount >= config.minDirectEvidence);
    if (view.quarantineCycles >= config.maxQuarantineCycles || hasSufficientDirectEvidence) {
        view.state = TrustState::PERMANENT;
    } else {
        view.state = TrustState::TRUSTED;
        view.alpha = config.alphaPrior;
        view.beta = config.betaPrior + view.directNegativeCount * 0.5;
        view.stateEnteredAt = 0.0;
        // H1: directNegativeCount здесь НЕ сбрасывается, чтобы накапливать его в сторону PERMANENT
    }
}

void TrustPolicy::decay(TrustView& view, double now)
{
    double dt = now - view.lastDecayTime;
    if (dt <= 0) return;

    double factor = std::pow(0.5, dt / config.evidenceHalfLife);

    view.alpha = (view.alpha - config.alphaPrior) * factor + config.alphaPrior;
    view.beta = (view.beta - config.betaPrior) * factor + config.betaPrior;
    view.lastDecayTime = now;
}

} // namespace trust
} // namespace repaodv
