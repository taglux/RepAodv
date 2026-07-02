CXX = g++
CXXFLAGS = -std=c++17 -O0 -g -Wall -Wextra -I ../src/repaodv

TRUST_SRCS = $(wildcard ../src/repaodv/trust/*.cc) \
             $(wildcard ../src/repaodv/trust/detectors/*.cc)

TEST_SRCS = $(wildcard test_*.cc)

test_runner: $(TEST_SRCS) $(TRUST_SRCS)
	$(CXX) $(CXXFLAGS) -o test_runner $(TEST_SRCS) $(TRUST_SRCS)

run: test_runner
	./test_runner

clean:
	rm -f test_runner

.PHONY: run clean
