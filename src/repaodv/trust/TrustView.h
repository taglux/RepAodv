#pragma once

#include <ostream>
#include "Evidence.h"

namespace repaodv {
namespace trust {

/**
 * @brief Категории доверия к узлу.
 */
enum class TrustState {
    TRUSTED,    ///< Высокая репутация, штатная работа
    SUSPECT,    ///< Падающая репутация, усиленный мониторинг
    QUARANTINE, ///< Репутация ниже порога, изоляция на некоторый период
    PERMANENT   ///< Окончательно занесён в чёрный список
};

/**
 * @brief Параметры бета-распределения и метаданные для конкретного соседа.
 */
struct TrustView {
    NeighborAddr neighbor;

    // Параметры бета-распределения
    double alpha = 1.0; // Положительные наблюдения + 1
    double beta = 1.0;  // Отрицательные наблюдения + 1

    // Метаданные для принятия решений
    double lastObservation = 0.0;
    double lastDecayTime = 0.0;
    double stateEnteredAt = 0.0;
    int quarantineCycles = 0;
    int directNegativeCount = 0;
    TrustState state = TrustState::TRUSTED;

    // Производная величина (мат. ожидание бета-распределения: alpha / (alpha + beta))
    double getReputation() const {
        return alpha / (alpha + beta);
    }
};

/**
 * @brief Вспомогательный вывод TrustState в поток.
 */
inline std::ostream& operator<<(std::ostream& os, const TrustState& state) {
    switch (state) {
        case TrustState::TRUSTED:    return os << "TRUSTED";
        case TrustState::SUSPECT:    return os << "SUSPECT";
        case TrustState::QUARANTINE: return os << "QUARANTINE";
        case TrustState::PERMANENT:  return os << "PERMANENT";
        default:                     return os << "UNKNOWN";
    }
}

} // namespace trust
} // namespace repaodv
