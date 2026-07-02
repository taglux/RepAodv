#pragma once

#include "Evidence.h"
#include <cstdint>

namespace repaodv {
namespace trust {

using PacketId = uint64_t;

/**
 * @brief Событие, обнаруженное поведенческим детектором.
 */
struct DetectorEvent {
    PacketId packetId;
    NeighborAddr neighbor;
    double timestamp;
    bool success;
};

/**
 * @brief Базовый класс для поведенческих детекторов.
 */
class IDetector {
public:
    virtual ~IDetector() = default;

    /**
     * @brief Периодическая обработка таймаутов и иных событий, привязанных ко времени.
     * @param currentTime Текущее модельное время.
     */
    virtual void processTimeouts(double currentTime) = 0;
};

} // namespace trust
} // namespace repaodv
