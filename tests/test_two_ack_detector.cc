#include "test_harness.h"
#include "trust/TrustEngine.h"
#include "trust/TrustPolicy.h"
#include "trust/detectors/TwoAckDetector.h"

using namespace repaodv::trust;

// Вспомогательная функция для создания engine с политикой по умолчанию
std::unique_ptr<TrustEngine> createTestEngine() {
    auto policy = std::make_unique<TrustPolicy>(PolicyConfig{}); // alphaPrior=2, betaPrior=1
    return std::make_unique<TrustEngine>(std::move(policy));
}

TEST(TwoAckDetector_Basic) {
    auto engine = createTestEngine();
    TwoAckDetector detector(*engine);
    NeighborAddr neighbor = 1001;
    PacketId pktId = 500;
    
    detector.onPacketSent(pktId, neighbor, 10.0);
    ASSERT_EQ(detector.getPendingCount(), 1u);
    
    // Проверяем успех при t=5.0
    detector.onAckReceived(pktId, neighbor, 5.0);
    ASSERT_EQ(detector.getPendingCount(), 0u);

    // Проверяем, что репутация выросла (alpha должна быть 3: 2 априор + 1 успех)
    auto& view = engine->getView(neighbor);
    ASSERT_EQ(view.alpha, 3.0);
    
    return true;
}

TEST(TwoAckDetector_Timeout) {
    auto engine = createTestEngine();
    TwoAckDetector detector(*engine);
    NeighborAddr neighbor = 2002;

    detector.onPacketSent(1, neighbor, 10.0);
    detector.onPacketSent(2, neighbor, 20.0);
    ASSERT_EQ(detector.getPendingCount(), 2u);

    // Обрабатываем при t=15 (первый должен истечь по таймауту)
    detector.processTimeouts(15.0);
    ASSERT_EQ(detector.getPendingCount(), 1u);

    // Проверяем, что репутация снизилась (beta должна быть 2: 1 априор + 1 отказ)
    auto& view = engine->getView(neighbor);
    ASSERT_EQ(view.beta, 2.0);

    return true;
}

TEST(TwoAckDetector_OnTimeoutCallback) {
    // callback onTimeout должен срабатывать с корректными (packetId, neighbor) при истечении дедлайна.
    // Это хук, используемый механизмом восстановления через ретрансляцию (J4).
    auto engine = createTestEngine();
    TwoAckDetector detector(*engine);
    NeighborAddr neighbor = 3003;
    PacketId pktId = 42;

    bool callbackFired = false;
    PacketId capturedId = 0;
    NeighborAddr capturedNeighbor = 0;

    detector.onTimeout = [&](PacketId id, NeighborAddr n) {
        callbackFired = true;
        capturedId = id;
        capturedNeighbor = n;
    };

    detector.onPacketSent(pktId, neighbor, 10.0);
    detector.processTimeouts(15.0);

    ASSERT_TRUE(callbackFired);
    ASSERT_EQ(capturedId, pktId);
    ASSERT_EQ(capturedNeighbor, neighbor);

    return true;
}

TEST(TwoAckDetector_MultipleNeighbors) {
    // Два соседа, смешанные ACK и таймауты — свидетельства должны атрибутироваться корректно.
    auto engine = createTestEngine();
    TwoAckDetector detector(*engine);
    NeighborAddr nA = 1001;
    NeighborAddr nB = 2002;

    // 2 пакета к A (дедлайны 5 и 10), 1 пакет к B (дедлайн 8)
    detector.onPacketSent(1, nA, 5.0);
    detector.onPacketSent(2, nA, 10.0);
    detector.onPacketSent(3, nB, 8.0);
    ASSERT_EQ(detector.getPendingCount(), 3u);

    // ACK для пакета 1 к A → успех для A
    detector.onAckReceived(1, nA, 4.0);
    ASSERT_EQ(detector.getPendingCount(), 2u);

    // t=9: пакет 3 к B истекает по таймауту (дедлайн 8), пакет 2 к A ещё ожидает (дедлайн 10)
    detector.processTimeouts(9.0);
    ASSERT_EQ(detector.getPendingCount(), 1u);

    auto& viewA = engine->getView(nA);
    auto& viewB = engine->getView(nB);
    ASSERT_EQ(viewA.alpha, 3.0); // априор 2 + 1 успех
    ASSERT_EQ(viewA.beta, 1.0);  // нет отказов
    ASSERT_EQ(viewB.alpha, 2.0); // нет успехов
    ASSERT_EQ(viewB.beta, 2.0);  // априор 1 + 1 отказ

    return true;
}
