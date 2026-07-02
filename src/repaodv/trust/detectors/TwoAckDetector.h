#pragma once

#include "../IDetector.h"
#include <functional>
#include <map>

namespace repaodv {
namespace trust {

class TrustEngine;

/**
 * @brief Реализация детектора 2ACK (подтверждение через два хопа).
 */
class TwoAckDetector : public IDetector {
public:
    explicit TwoAckDetector(TrustEngine& engine);

    /**
     * @brief Регистрирует отправленный пакет, для которого ожидается 2ACK.
     */
    void onPacketSent(PacketId packetId, NeighborAddr neighbor, double deadline);

    /**
     * @brief Подтверждает пересылку пакета по полученному 2ACK.
     */
    void onAckReceived(PacketId packetId, NeighborAddr fromNeighbor, double now);

    /**
     * @brief Проверяет истёкшие наблюдения.
     */
    void processTimeouts(double currentTime) override;

    /**
     * @brief Отменяет все ожидающие записи по соседу, не штрафуя его.
     * Вызывается при разрыве L2-канала, чтобы вызванные мобильностью таймауты не превращались в ложные срабатывания.
     */
    void cancelForNeighbor(NeighborAddr neighbor);

    /**
     * @brief Возвращает ближайший дедлайн в множестве ожидания или -1.0, если их нет.
     */
    double getNextTimeout() const;

    // Вызывается при истечении ожидающей записи 2ACK: (packetId, timedOutNeighbor).
    // Позволяет ретранслирующему слою отправить восстановительный ACK вверх по маршруту.
    std::function<void(PacketId, NeighborAddr)> onTimeout;

    // Для тестов
    size_t getPendingCount() const { return pending.size(); }

private:
    TrustEngine& engine;
    // Отображение (neighbor, packetId) -> дедлайн
    std::map<std::pair<NeighborAddr, PacketId>, double> pending;
    // Вспомогательное отображение дедлайн -> {(neighbor, packetId)} для быстрого getNextTimeout
    std::multimap<double, std::pair<NeighborAddr, PacketId>> timeouts;
};

} // namespace trust
} // namespace repaodv
