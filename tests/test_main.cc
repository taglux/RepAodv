#include "test_harness.h"

std::vector<Test>& _allTests() {
    static std::vector<Test> tests;
    return tests;
}

int main() {
    auto& tests = _allTests();
    int failed = 0;
    for (auto& t : tests) {
        std::cout << "[ RUN  ] " << t.name << std::endl;
        bool ok = t.fn();
        std::cout << (ok ? "[ PASS ] " : "[ FAIL ] ") << t.name << std::endl;
        if (!ok) failed++;
    }
    std::cout << "\nTotal: " << tests.size()
              << ", Failed: " << failed << std::endl;
    return failed == 0 ? 0 : 1;
}
