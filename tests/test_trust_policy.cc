#include "test_harness.h"
#include "../src/repaodv/trust/TrustPolicy.h"
#include "../src/repaodv/trust/TrustView.h"

using namespace repaodv::trust;

TEST(test_basic_update) {
    TrustPolicy policy(PolicyConfig{});
    // Априорные значения по умолчанию: alpha=2, beta=1
    TrustView view;
    view.alpha = 2.0;
    view.beta = 1.0;

    policy.update(view, true, 1.0); // Успех при t=1.0
    ASSERT_EQ(view.alpha, 3.0);
    ASSERT_EQ(view.beta, 1.0);
    ASSERT_NEAR(view.getReputation(), 0.75, 0.001);

    policy.update(view, false, 2.0); // Отказ при t=2.0
    ASSERT_EQ(view.alpha, 3.0);
    ASSERT_EQ(view.beta, 2.0);
    ASSERT_NEAR(view.getReputation(), 0.6, 0.001);
    
    return true;
}

TEST(test_state_transitions) {
    TrustPolicy policy(PolicyConfig{}); // alphaPrior=2, betaPrior=1, suspectThreshold=0.5, quarantineThreshold=0.2
    TrustView view;
    view.alpha = 2.0;
    view.beta = 1.0;
    view.state = TrustState::TRUSTED;

    // С типом FORWARDING: после 1-го отказа directNegativeCount=1, effectiveMinObs падает до 2 (быстрый трекинг H1).
    // 1-й отказ: n=1 < effectiveMinObs=2 → всё ещё TRUSTED.
    policy.update(view, false, 1.0);
    ASSERT_EQ(view.state, TrustState::TRUSTED);

    // 2-й отказ: n=2 >= 2, reputation=2/5=0.4 < suspectThreshold=0.5 → SUSPECT.
    policy.update(view, false, 2.0);
    ASSERT_EQ(view.state, TrustState::SUSPECT);

    // Отказы 3-7: reputation 2/6..2/10 остаётся выше quarantineThreshold=0.2 → SUSPECT.
    for (int i = 3; i <= 7; ++i) {
        policy.update(view, false, (double)i);
        ASSERT_EQ(view.state, TrustState::SUSPECT);
    }

    // 8-й отказ: alpha=2, beta=10, reputation=2/12≈0.167 < 0.2, directNegativeCount=8 > 0 → QUARANTINE.
    policy.update(view, false, 8.0);
    ASSERT_EQ(view.state, TrustState::QUARANTINE);

    return true;
}

TEST(test_decay) {
    TrustPolicy policy(PolicyConfig{}); // alphaPrior=2, betaPrior=1
    TrustView view;
    view.alpha = 6.0; // 4 успеха + априор
    view.beta = 5.0;  // 4 отказа + априор
    view.lastDecayTime = 100.0;

    // Затухание при t = 160.0 (ровно через один период полураспада)
    policy.decay(view, 160.0);

    // alpha = (6 - 2) * 0.5 + 2 = 4
    // beta = (5 - 1) * 0.5 + 1 = 3
    ASSERT_NEAR(view.alpha, 4.0, 0.001);
    ASSERT_NEAR(view.beta, 3.0, 0.001);
    ASSERT_EQ(view.lastDecayTime, 160.0);

    // Затухание снова при t = 220.0 (ещё один период полураспада)
    policy.decay(view, 220.0);
    // alpha = (4 - 2) * 0.5 + 2 = 3
    // beta = (3 - 1) * 0.5 + 1 = 2
    ASSERT_NEAR(view.alpha, 3.0, 0.001);
    ASSERT_NEAR(view.beta, 2.0, 0.001);
    
    return true;
}

TEST(test_quarantine_rehabilitation_memory) {
    PolicyConfig config;
    config.quarantineWindow = 60.0;
    config.maxQuarantineCycles = 2;
    config.minDirectEvidence = 100; // Принудительная реабилитация для этого теста

    TrustPolicy policy(config);
    TrustView view;
    view.alpha = config.alphaPrior;
    view.beta = config.betaPrior;

    // 1. Переводим в QUARANTINE
    // Нужно достаточно наблюдений, чтобы вызвать SUSPECT, затем QUARANTINE
    for (int i = 0; i < 20; ++i) {
        policy.update(view, false, (double)i, EvidenceType::FORWARDING);
    }
    ASSERT_EQ(view.state, TrustState::QUARANTINE);
    ASSERT_TRUE(view.directNegativeCount > 0);
    int directNegativeInQuarantine = view.directNegativeCount;

    // 2. Ждём quarantineWindow и вызываем tick
    double rehabilitationTime = view.stateEnteredAt + config.quarantineWindow + 1.0;
    policy.tick(view, rehabilitationTime);

    // 3. Проверяем реабилитацию с памятью (H1)
    ASSERT_EQ(view.state, TrustState::TRUSTED);

    // НОВОЕ ПОВЕДЕНИЕ: beta не должна быть просто config.betaPrior (1.0)
    // Она должна быть betaPrior + directNegativeCount * 0.5
    ASSERT_TRUE(view.beta > config.betaPrior);
    ASSERT_EQ(view.beta, config.betaPrior + directNegativeInQuarantine * 0.5);

    // НОВОЕ ПОВЕДЕНИЕ: directNegativeCount НЕ должен сбрасываться
    ASSERT_EQ(view.directNegativeCount, directNegativeInQuarantine);

    return true;
}

TEST(test_permanent_via_direct_evidence) {
    // Когда directNegativeCount >= minDirectEvidence, tick() должен перейти в PERMANENT
    // независимо от quarantineCycles.
    PolicyConfig config;
    config.quarantineWindow = 60.0;
    config.maxQuarantineCycles = 100; // велико — путь по циклам не должен срабатывать
    config.minDirectEvidence = 3;

    TrustPolicy policy(config);
    TrustView view;
    view.alpha = config.alphaPrior;
    view.beta = config.betaPrior;

    // Доводим до QUARANTINE отказами FORWARDING (увеличивает directNegativeCount)
    for (int i = 0; i < 20; ++i)
        policy.update(view, false, (double)i, EvidenceType::FORWARDING);

    ASSERT_EQ(view.state, TrustState::QUARANTINE);
    ASSERT_TRUE(view.directNegativeCount >= config.minDirectEvidence);

    // tick() после quarantineWindow: directNegativeCount >= 3 → PERMANENT
    policy.tick(view, view.stateEnteredAt + config.quarantineWindow + 1.0);
    ASSERT_EQ(view.state, TrustState::PERMANENT);

    return true;
}

TEST(test_permanent_via_cycles) {
    // После maxQuarantineCycles окон карантина tick() должен перейти в PERMANENT.
    PolicyConfig config;
    config.quarantineWindow = 60.0;
    config.maxQuarantineCycles = 2;
    config.minDirectEvidence = 100; // велико — путь по прямым свидетельствам не должен срабатывать

    TrustPolicy policy(config);
    TrustView view;
    view.alpha = config.alphaPrior;
    view.beta = config.betaPrior;

    // Цикл 1: доводим до QUARANTINE, затем реабилитируем
    for (int i = 0; i < 20; ++i)
        policy.update(view, false, (double)i, EvidenceType::FORWARDING);
    ASSERT_EQ(view.state, TrustState::QUARANTINE);

    double t1 = view.stateEnteredAt + config.quarantineWindow + 1.0;
    policy.tick(view, t1);
    ASSERT_EQ(view.state, TrustState::TRUSTED);
    ASSERT_EQ(view.quarantineCycles, 1);

    // Цикл 2: снова доводим до QUARANTINE, затем tick → PERMANENT
    for (int i = 0; i < 20; ++i)
        policy.update(view, false, t1 + 100.0 + i, EvidenceType::FORWARDING);
    ASSERT_EQ(view.state, TrustState::QUARANTINE);

    double t2 = view.stateEnteredAt + config.quarantineWindow + 1.0;
    policy.tick(view, t2);
    ASSERT_EQ(view.state, TrustState::PERMANENT);

    return true;
}

TEST(test_permanent_state_irreversible) {
    // Успехи и tick() не должны восстанавливать узел в состоянии PERMANENT.
    PolicyConfig config;
    config.quarantineWindow = 60.0;
    config.maxQuarantineCycles = 1;
    config.minDirectEvidence = 100;

    TrustPolicy policy(config);
    TrustView view;
    view.alpha = config.alphaPrior;
    view.beta = config.betaPrior;

    for (int i = 0; i < 20; ++i)
        policy.update(view, false, (double)i, EvidenceType::FORWARDING);
    ASSERT_EQ(view.state, TrustState::QUARANTINE);

    policy.tick(view, view.stateEnteredAt + config.quarantineWindow + 1.0);
    ASSERT_EQ(view.state, TrustState::PERMANENT);

    // Множество успехов — состояние не должно меняться
    for (int i = 0; i < 50; ++i)
        policy.update(view, true, 200.0 + i, EvidenceType::FORWARDING);
    ASSERT_EQ(view.state, TrustState::PERMANENT);

    // tick() далеко в будущем — не должен реабилитировать
    policy.tick(view, 100000.0);
    ASSERT_EQ(view.state, TrustState::PERMANENT);

    return true;
}

TEST(test_gossip_quarantine_no_rehabilitation) {
    // Карантин только по gossip (directNegativeCount=0) должен продлевать окно карантина,
    // а не реабилитировать узел в TRUSTED по истечении quarantineWindow.
    PolicyConfig cfg;
    cfg.quarantineWindow = 60.0;
    cfg.minObservations = 2;
    TrustPolicy policy(cfg);
    TrustView view;
    view.alpha = cfg.alphaPrior;
    view.beta = cfg.betaPrior;

    // Доводим до QUARANTINE только через gossip (SIGNED_HINT)
    for (int i = 0; i < 15; ++i)
        policy.update(view, false, (double)i, EvidenceType::SIGNED_HINT);
    ASSERT_EQ(view.state, TrustState::QUARANTINE);
    ASSERT_EQ(view.directNegativeCount, 0);

    double enteredAt = view.stateEnteredAt;

    // tick до окончания окна — без изменений
    policy.tick(view, enteredAt + 30.0);
    ASSERT_EQ(view.state, TrustState::QUARANTINE);

    // tick после окна — НЕ должен реабилитировать; вместо этого должен продлить окно
    policy.tick(view, enteredAt + 61.0);
    ASSERT_EQ(view.state, TrustState::QUARANTINE);
    ASSERT_TRUE(view.stateEnteredAt > enteredAt); // окно было продлено

    return true;
}

TEST(test_gossip_quarantine_escape_via_direct_positive) {
    // Узел, помещённый в карантин по gossip (directNegativeCount=0), должен выйти в SUSPECT,
    // когда поступает достаточно прямых положительных свидетельств FORWARDING.
    PolicyConfig cfg;
    cfg.quarantineWindow = 60.0;
    cfg.minObservations = 2;
    cfg.recoveryThreshold = 0.7;
    TrustPolicy policy(cfg);
    TrustView view;
    view.alpha = cfg.alphaPrior;
    view.beta = cfg.betaPrior;

    // Доводим до QUARANTINE через gossip
    for (int i = 0; i < 15; ++i)
        policy.update(view, false, (double)i, EvidenceType::SIGNED_HINT);
    ASSERT_EQ(view.state, TrustState::QUARANTINE);
    ASSERT_EQ(view.directNegativeCount, 0);

    // Накапливаем положительные свидетельства FORWARDING, пока reputation > recoveryThreshold
    for (int i = 0; i < 30; ++i)
        policy.update(view, true, 100.0 + i, EvidenceType::FORWARDING);

    // Узел должен был выйти из карантина в SUSPECT
    ASSERT_TRUE(view.state == TrustState::SUSPECT || view.state == TrustState::TRUSTED);

    return true;
}

TEST(test_signed_hint_no_direct_count) {
    // SIGNED_HINT (gossip) НЕ должен увеличивать directNegativeCount.
    // На счётчик прямых свидетельств влияет только свидетельство FORWARDING.
    TrustPolicy policy(PolicyConfig{});
    TrustView view;
    view.alpha = policy.getAlphaPrior();
    view.beta = policy.getBetaPrior();

    for (int i = 0; i < 20; ++i)
        policy.update(view, false, (double)i, EvidenceType::SIGNED_HINT);

    ASSERT_EQ(view.directNegativeCount, 0);

    // Один отказ FORWARDING всё же увеличивает его
    policy.update(view, false, 30.0, EvidenceType::FORWARDING);
    ASSERT_EQ(view.directNegativeCount, 1);

    return true;
}
