#pragma once

#include <map>
#include <functional>
#include <memory>
#include "Evidence.h"
#include "TrustPolicy.h"
#include "TrustView.h"

namespace repaodv {
namespace trust {

/**
 * @brief Фасад системы репутации.
 *
 * Управляет TrustView для всех соседей и координирует обновления через TrustPolicy.
 */
class TrustEngine {
public:
    using StateChangeCallback = std::function<void(NeighborAddr, TrustState, TrustState)>;

    /**
     * @brief Создаёт TrustEngine с заданной политикой.
     * @param policy Политика обновления репутации и переходов состояний.
     */
    explicit TrustEngine(std::unique_ptr<TrustPolicy> policy);

    /**
     * @brief Регистрирует новое наблюдение о соседе.
     * @param neighbor Адрес наблюдаемого соседа.
     * @param type Источник свидетельства.
     * @param success True, если узел повёл себя корректно.
     * @param now Текущее модельное время.
     */
    void updateEvidence(NeighborAddr neighbor, EvidenceType type, bool success, double now);

    /**
     * @brief Возвращает текущее состояние доверия к соседу.
     */
    TrustState getState(NeighborAddr neighbor) const;

    /**
     * @brief Возвращает текущую оценку репутации соседа (от 0.0 до 1.0).
     */
    double getReputation(NeighborAddr neighbor) const;

    /**
     * @brief Применяет затухание со временем ко всем известным соседям.
     * @param now Текущее модельное время.
     */
    void decayAll(double now);

    /**
     * @brief Задаёт callback, вызываемый при изменении состояния соседа.
     */
    void onStateChange(StateChangeCallback cb) { callback = cb; }

    /**
     * @brief Возвращает полное представление о соседе (для тестов/логирования).
     */
    const TrustView& getView(NeighborAddr neighbor) const;

    /**
     * @brief Возвращает все известные представления доверия.
     */
    const std::map<NeighborAddr, TrustView>& getAllViews() const { return views; }

private:
    /**
     * @brief Возвращает существующее представление или создаёт новое со значениями по умолчанию.
     */
    TrustView& getOrCreateView(NeighborAddr neighbor);

    std::unique_ptr<TrustPolicy> policy;
    std::map<NeighborAddr, TrustView> views;
    StateChangeCallback callback;
};

} // namespace trust
} // namespace repaodv
