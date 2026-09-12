#include "core/Banker.h"
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>
#include <algorithm>
using namespace osv;
void require(bool value, const char* text) { if (!value) throw std::runtime_error(text); }
void rejects(const BankerInput& v) {
    try { BankerAlgorithm().run(v); } catch (const std::invalid_argument&) { return; }
    throw std::runtime_error("invalid input accepted");
}
// Independent exhaustive sequence search for small randomized matrices.
bool oracle(const BankerInput& input, std::vector<int> work, unsigned done = 0) {
    if (done == (1u << input.allocation.size()) - 1) return true;
    for (size_t p = 0; p < input.allocation.size(); ++p) {
        if (done & (1u << p)) continue;
        bool can = true;
        for (size_t j = 0; j < work.size(); ++j) if (input.max[p][j] - input.allocation[p][j] > work[j]) can = false;
        if (can) {
            auto next = work;
            for (size_t j = 0; j < next.size(); ++j) next[j] += input.allocation[p][j];
            if (oracle(input, next, done | (1u << p))) return true;
        }
    }
    return false;
}
int main() {
    try {
        BankerInput sample{{3,3,2}, {{0,1,0},{2,0,0},{3,0,2},{2,1,1},{0,0,2}}, {{7,5,3},{3,2,2},{9,0,2},{2,2,2},{4,3,3}}};
        auto r = BankerAlgorithm().run(sample);
        require(r.safe && r.safeSequence == std::vector<int>({1,3,4,0,2}), "textbook safe sequence");
        require(r.need[0] == std::vector<int>({7,4,3}) && !r.steps[0].canFinish, "Need and failed check recorded");
        require(r.steps[1].workAfter == std::vector<int>({5,3,2}), "resource release");
        sample.available = {0,0,0}; require(!BankerAlgorithm().run(sample).safe, "unsafe example");
        require(BankerAlgorithm().run({{0},{{3}},{{3}}}).safe, "single process zero Need");
        require(!BankerAlgorithm().run({{0},{{1}},{{2}}}).safe, "single resource unsafe");
        require(BankerAlgorithm().run({{0},{{1},{2}},{{1},{2}}}).safe, "all Need zero");
        rejects({}); rejects({{1},{{2}},{{1}}}); rejects({{1,2},{{0}},{{1}}});
        rejects({{-1},{{0}},{{1}}}); rejects({{1},{{0}},{{-1}}}); rejects({{1},{{0},{0}},{{1}}});
        std::mt19937 rng(408);
        for (int t = 0; t < 200; ++t) {
            BankerInput v{{int(rng()%3), int(rng()%3)}, {}, {}};
            for (int i = 0; i < 5; ++i) {
                std::vector<int> a{int(rng()%3), int(rng()%3)}, m{a[0]+int(rng()%5), a[1]+int(rng()%5)};
                v.allocation.push_back(a); v.max.push_back(m);
            }
            r = BankerAlgorithm().run(v);
            require(r.safe == oracle(v,v.available), "exhaustive oracle agreement");
            auto work = v.available; std::vector<bool> done(5,false);
            for (const auto& s : r.steps) {
                require(s.workBefore == work && !done[s.processId], "trace continuity");
                if (s.canFinish) { done[s.processId] = true; for (int j=0;j<2;++j) work[j]+=v.allocation[s.processId][j]; }
                require(s.workAfter == work, "trace resource conservation");
            }
        }
        std::cout << "PASS: banker fixtures and 200 exhaustive-oracle scenarios\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
