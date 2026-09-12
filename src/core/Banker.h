#pragma once
#include <vector>

namespace osv {
struct BankerInput {
    std::vector<int> available;
    std::vector<std::vector<int>> allocation, max;
};
struct BankerStep {
    int processId;
    std::vector<int> workBefore, workAfter;
    bool canFinish;
    std::vector<int> insufficientResources;
};
struct BankerResult {
    bool safe = false;
    std::vector<std::vector<int>> need;
    std::vector<int> safeSequence, unfinished;
    std::vector<BankerStep> steps;
};
class BankerAlgorithm {
public:
    BankerResult run(const BankerInput& input) const;
};
}
