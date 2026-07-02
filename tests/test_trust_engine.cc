#include "test_harness.h"
#include "../src/repaodv/trust/TrustEngine.h"
#include <memory>

using namespace repaodv::trust;

TEST(test_engine_basic) {
    auto policy = std::make_unique<TrustPolicy>(PolicyConfig{});
    TrustEngine engine(std::move(policy));
    
    NeighborAddr n1 = 100;

    // Начальное состояние
    ASSERT_EQ(engine.getState(n1), TrustState::TRUSTED);

    // Обновляем свидетельство
    engine.updateEvidence(n1, EvidenceType::FORWARDING, true, 1.0);

    // alpha=3, beta=1. Rep = 3/4 = 0.75
    ASSERT_NEAR(engine.getReputation(n1), 0.75, 0.01);

    return true;
}

TEST(test_engine_callback) {
    auto policy = std::make_unique<TrustPolicy>(PolicyConfig{.alphaPrior=1.0, .betaPrior=1.0}); // minObservations по умолчанию равен 5
    TrustEngine engine(std::move(policy));
    
    NeighborAddr n1 = 100;
    bool callbackCalled = false;
    TrustState capturedOld, capturedNew;
    
    engine.onStateChange([&](NeighborAddr addr, TrustState oldS, TrustState newS) {
        if (addr == n1 && oldS != newS) {  // фиксируем только реальные смены состояния
            callbackCalled = true;
            capturedOld = oldS;
            capturedNew = newS;
        }
    });

    // alphaPrior=betaPrior=1, effectiveMinObs=2 после первого отказа FORWARDING.
    // После 2-го отказа: n=2>=2, rep=1/4=0.25 < 0.5 → TRUSTED→SUSPECT.
    // После 4-го отказа: rep=1/6≈0.167 < 0.2 → SUSPECT→QUARANTINE.
    for (int i = 0; i < 5; ++i) {
        engine.updateEvidence(n1, EvidenceType::FORWARDING, false, (double)i);
    }

    ASSERT_TRUE(callbackCalled);
    ASSERT_EQ(capturedOld, TrustState::SUSPECT);
    ASSERT_EQ(capturedNew, TrustState::QUARANTINE);
    
    return true;
}
