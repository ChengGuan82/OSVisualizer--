#include "core/Banker.h"
#include <stdexcept>

namespace osv {
// Adapted from exp/OS-exp3/banker_alg.cpp. Input is immutable; every check is
// recorded, including failed comparisons, so the safety proof can be replayed.
BankerResult BankerAlgorithm::run(const BankerInput& input) const {
    const size_t n = input.allocation.size(), m = input.available.size();
    if (n == 0 || n > 30 || m == 0 || m > 8 || input.max.size() != n)
        throw std::invalid_argument("进程数须为 1–30，资源种类须为 1–8，矩阵行数必须一致");
    auto valid = [](int v) { return v >= 0 && v <= 1000000; };
    for (int v : input.available) if (!valid(v)) throw std::invalid_argument("资源数量须为 0–1000000 的整数");
    BankerResult result;
    result.need.resize(n, std::vector<int>(m));
    for (size_t i = 0; i < n; ++i) {
        if (input.allocation[i].size() != m || input.max[i].size() != m)
            throw std::invalid_argument("Allocation、Max 列数必须与 Available 一致");
        for (size_t j = 0; j < m; ++j) {
            if (!valid(input.allocation[i][j]) || !valid(input.max[i][j]))
                throw std::invalid_argument("资源数量须为 0–1000000 的整数");
            if (input.max[i][j] < input.allocation[i][j]) throw std::invalid_argument("Max 不能小于 Allocation");
            result.need[i][j] = input.max[i][j] - input.allocation[i][j];
        }
    }
    auto work = input.available;
    std::vector<bool> finished(n, false);
    while (result.safeSequence.size() < n) {
        bool progress = false;
        for (size_t i = 0; i < n; ++i) {
            if (finished[i]) continue;
            BankerStep step{int(i), work, work, true, {}};
            for (size_t j = 0; j < m; ++j) if (result.need[i][j] > work[j]) {
                step.canFinish = false; step.insufficientResources.push_back(int(j));
            }
            if (step.canFinish) {
                for (size_t j = 0; j < m; ++j) work[j] += input.allocation[i][j];
                finished[i] = true; progress = true;
                result.safeSequence.push_back(int(i)); step.workAfter = work;
            }
            result.steps.push_back(step);
        }
        if (!progress) break;
    }
    for (size_t i = 0; i < n; ++i) if (!finished[i]) result.unfinished.push_back(int(i));
    result.safe = result.unfinished.empty();
    return result;
}
}
