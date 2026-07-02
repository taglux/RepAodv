#include "test_harness.h"
#include "trust/GossipSourceTracker.h"

using repaodv::trust::GossipSourceTracker;

// Single source below threshold k=2 — evidence must NOT be applied
TEST(gossip_single_source_below_threshold) {
    GossipSourceTracker tracker;
    tracker.addReport(/*reporter=*/1, /*target=*/42, /*now=*/0.0);
    ASSERT_TRUE(!tracker.hasSufficientSources(42, 2));
    return true;
}

// Two distinct sources reach threshold k=2 — evidence MUST be applied
TEST(gossip_two_sources_reach_threshold) {
    GossipSourceTracker tracker;
    tracker.addReport(1, 42, 0.0);
    tracker.addReport(2, 42, 0.0);
    ASSERT_TRUE(tracker.hasSufficientSources(42, 2));
    return true;
}

// Same reporter twice — must not double-count
TEST(gossip_same_reporter_no_double_count) {
    GossipSourceTracker tracker;
    tracker.addReport(1, 42, 0.0);
    tracker.addReport(1, 42, 1.0); // same reporter again
    ASSERT_TRUE(!tracker.hasSufficientSources(42, 2));
    return true;
}

// k=1 allows a single source
TEST(gossip_k1_allows_single_source) {
    GossipSourceTracker tracker;
    tracker.addReport(1, 42, 0.0);
    ASSERT_TRUE(tracker.hasSufficientSources(42, 1));
    return true;
}

// Unknown target always returns false
TEST(gossip_unknown_target_returns_false) {
    GossipSourceTracker tracker;
    ASSERT_TRUE(!tracker.hasSufficientSources(99, 2));
    return true;
}

// Reset clears all counts — single source remains insufficient after reset
TEST(gossip_reset_clears_counts) {
    GossipSourceTracker tracker;
    tracker.addReport(1, 42, 0.0);
    tracker.addReport(2, 42, 0.0);
    ASSERT_TRUE(tracker.hasSufficientSources(42, 2)); // sanity check before reset

    tracker.resetIfNeeded(/*now=*/100.0, /*interval=*/60.0); // 100 > 0+60 → resets
    ASSERT_TRUE(!tracker.hasSufficientSources(42, 2));
    return true;
}

// Reset does NOT fire if interval hasn't passed yet
TEST(gossip_reset_not_yet_due) {
    GossipSourceTracker tracker;
    tracker.addReport(1, 42, 0.0);
    tracker.addReport(2, 42, 0.0);

    tracker.resetIfNeeded(/*now=*/50.0, /*interval=*/60.0); // 50 < 0+60 → no reset
    ASSERT_TRUE(tracker.hasSufficientSources(42, 2));
    return true;
}

// Reports for different targets are independent
TEST(gossip_separate_targets_are_independent) {
    GossipSourceTracker tracker;
    tracker.addReport(1, 42, 0.0);
    tracker.addReport(2, 42, 0.0);
    // target 99 has zero reports
    ASSERT_TRUE(tracker.hasSufficientSources(42, 2));
    ASSERT_TRUE(!tracker.hasSufficientSources(99, 2));
    return true;
}
