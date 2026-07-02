#pragma once

#include "TrustView.h"

namespace repaodv {
namespace trust {

/**
 * @brief Параметры байесовского обновления репутации и конечного автомата доверия.
 */
struct PolicyConfig {
    double alphaPrior = 2.0;
    double betaPrior = 1.0;
    int minObservations = 5;
    double suspectThreshold = 0.5;
    double quarantineThreshold = 0.2;
    double recoveryThreshold = 0.7;
    double recoveryWindow = 10.0;
    double quarantineWindow = 60.0;
    double evidenceHalfLife = 60.0;
    int maxQuarantineCycles = 2;
    int minDirectEvidence = 3;
};

/**
 * @brief Реализует байесовское обновление репутации и конечный автомат доверия.
 */
class TrustPolicy {
public:
    TrustPolicy(const PolicyConfig& config);

    /**
     * @brief Обновляет репутацию соседа по новому наблюдению.
     */
    void update(TrustView& view, bool success, double now, EvidenceType type = EvidenceType::FORWARDING);

    /**
     * @brief Применяет затухание со временем, возвращая репутацию к априорному значению.
     */
    void decay(TrustView& view, double now);

    /**
     * @brief Проверяет истечение карантина и применяет реабилитацию или окончательный бан.
     */
    void tick(TrustView& view, double now);

    double getAlphaPrior() const { return config.alphaPrior; }
    double getBetaPrior() const { return config.betaPrior; }

private:
    PolicyConfig config;
};

} // namespace trust
} // namespace repaodv
