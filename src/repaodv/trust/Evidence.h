#pragma once

#include <cstdint>

namespace repaodv {
namespace trust {

/**
 * @brief Типы свидетельств, которые можно собрать о соседе.
 */
enum class EvidenceType {
    FORWARDING,   ///< Прямое наблюдение пересылки пакета (Watchdog)
    SIGNED_HINT   ///< Полученная косвенная репутация или подписанный отзыв
};

using NeighborAddr = uint32_t;

/**
 * @brief Одно свидетельство, собранное системой.
 */
struct Evidence {
    NeighborAddr neighbor;
    EvidenceType type;
    double weight;
    double timestamp;
    bool positive; // true — успех, false — отказ
};

} // namespace trust
} // namespace repaodv
