#include "core/Algorithms.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <random>
#include <stdexcept>
#include <string>

using namespace osv;
int assertions = 0;
void check(bool condition, const std::string& message) {
    ++assertions;
    if (!condition) throw std::runtime_error(message);
}
void rejects(const std::function<void()>& action) {
    bool rejected = false;
    try { action(); } catch (const std::invalid_argument&) { rejected = true; }
    check(rejected, "invalid input must be rejected");
}
ScheduleResult cpu(std::vector<Process> p, Scheduling a = Scheduling::FCFS, int q = 2) { return StandardScheduler(a, q).run(p); }
PageResult page(std::vector<int> p, int frames, Replacement a) { return StandardReplacement(a).run(p, frames); }
void scheduleInvariants(const std::vector<Process>& input, const ScheduleResult& r) {
    std::map<int, int> executed, last;
    int cursor = 0;
    for (const auto& s : r.timeline) {
        check(s.start == cursor && s.end > s.start, "continuous positive timeline");
        cursor = s.end;
        if (s.pid < 0) {
            for (const auto& p : input) check(p.arrival >= s.end || executed[p.pid] == p.burst, "idle only without ready work");
        } else {
            auto p = std::find_if(input.begin(), input.end(), [&](const Process& v) { return v.pid == s.pid; });
            check(p != input.end() && s.start >= p->arrival, "never run before arrival");
            executed[s.pid] += s.end - s.start; last[s.pid] = s.end;
        }
    }
    double wait = 0, turnaround = 0; int total = 0;
    for (size_t i = 0; i < input.size(); ++i) {
        const auto& p = input[i]; const auto& m = r.processes[i];
        check(executed[p.pid] == p.burst, "burst conservation");
        check(m.pid == p.pid && m.completion == last[p.pid], "metrics match final slice");
        check(m.waiting >= 0 && m.turnaround == m.completion - p.arrival && m.waiting == m.turnaround - p.burst, "turnaround identities");
        check(std::abs(m.weightedTurnaround - double(m.turnaround) / p.burst) < 1e-9, "weighted turnaround");
        wait += m.waiting; turnaround += m.turnaround; total += p.burst;
    }
    check(std::abs(r.averageWaiting - wait / input.size()) < 1e-9, "average wait");
    check(std::abs(r.averageTurnaround - turnaround / input.size()) < 1e-9, "average turnaround");
    check(std::abs(r.utilization - double(total) / cursor) < 1e-9, "utilization includes initial idle");
}
int referenceFaults(const std::vector<int>& refs, int capacity, bool lru) {
    std::list<int> order; int faults = 0;
    for (int p : refs) {
        auto it = std::find(order.begin(), order.end(), p);
        if (it == order.end()) {
            ++faults; if (int(order.size()) == capacity) order.pop_front(); order.push_back(p);
        } else if (lru) { order.erase(it); order.push_back(p); }
    }
    return faults;
}
void testScheduling() {
    std::vector<Process> p = {{1, 0, 5, 2}, {2, 1, 3, 1}, {3, 2, 4, 3}};
    auto r = cpu(p); check(r.processes[0].completion == 5 && r.processes[2].completion == 12, "FCFS fixture");
    check(r.arrivals.size() == 3 && r.arrivals[1].pid == 2 && r.arrivals[1].arrival == 1, "arrival markers preserve original time inside a slice");
    check(std::abs(r.averageWaiting - 10.0 / 3) < 1e-9, "FCFS expected mean");
    r = cpu(p, Scheduling::RR); check(r.processes[0].completion == 12 && r.processes[1].completion == 9 && r.processes[2].completion == 11, "RR arrival-before-requeue fixture");
    r = cpu({{1, 0, 8, 0}, {2, 1, 4, 0}, {3, 2, 2, 0}}, Scheduling::SRTF);
    check(r.processes[0].completion == 14 && r.processes[1].completion == 7 && r.processes[2].completion == 4, "SRTF preemptions");
    r = cpu({{1, 0, 5, 1}, {2, 0, 2, 3}, {3, 0, 3, 2}}, Scheduling::SJF);
    check(r.timeline[0].pid == 2 && r.timeline[1].pid == 3, "SJF shortest ready first");
    r = cpu({{1, 0, 5, 3}, {2, 1, 2, 0}, {3, 0, 3, 2}}, Scheduling::Priority);
    check(r.timeline[0].pid == 3 && r.timeline[0].end == 3 && r.timeline[1].pid == 2, "priority lower value first, nonpreemptive");
    r = cpu({{9,0,2,1},{2,0,1,1}},Scheduling::Priority);
    check(r.timeline[0].pid==9,"priority stable equal-priority tie");
    r = cpu({{1,0,2,0},{2,1,5,0}},Scheduling::SRTF);
    check(r.timeline[0].pid==1 && r.timeline[0].end==2,"SRTF no preemption by longer arrival");
    r = cpu({{8,0,3,0},{2,1,2,0}},Scheduling::SRTF);
    check(r.timeline[0].pid==8 && r.timeline[0].end==3,"SRTF equal remaining favors earlier arrival");
    r = cpu({{9, 0, 2, 0}, {2, 0, 2, 0}}, Scheduling::SJF); check(r.timeline[0].pid == 9, "stable tie by input order");
    r = cpu({{1, 0, 2, 0}, {2, 1, 1, 0}}, Scheduling::RR, 100); check(r.processes[0].completion == 2, "large RR quantum");
    for (int a = 0; a < 5; ++a) {
        r = cpu({{1, 3, 2, 0}, {2, 9, 1, 0}}, static_cast<Scheduling>(a), 1);
        check(r.timeline[0].pid == -1 && r.timeline[0].end == 3 && r.processes[1].completion == 10, "initial and intermediate idle");
        scheduleInvariants({{1, 3, 2, 0}, {2, 9, 1, 0}}, r);
        r = cpu({{0, 4, 5, 0}}, static_cast<Scheduling>(a), 1); check(r.processes[0].waiting == 0, "single process");
    }
    rejects([] { cpu({}); }); rejects([] { cpu({{1, 0, 0, 0}}); });
    rejects([] { cpu({{1, -1, 2, 0}}); }); rejects([] { cpu({{1, 0, 2, 0}}, Scheduling::RR, 0); });
    rejects([] { cpu({{1, 0, 2, 0}, {1, 1, 3, 0}}); });
    rejects([] { cpu({{1, 0, 10000, 0}, {2, 0, 10000, 0}, {3, 0, 1, 0}}); });
}
void testPaging() {
    std::vector<int> refs = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2};
    check(page(refs, 3, Replacement::FIFO).faults == 10, "FIFO textbook fixture");
    check(page(refs, 3, Replacement::LRU).faults == 9, "LRU textbook fixture");
    check(page(refs, 3, Replacement::OPT).faults == 7, "OPT textbook fixture");
    auto opt = page({1,2,3,1},2,Replacement::OPT);
    check(opt.steps[2].replacedPage==2,"OPT chooses page never requested again");
    opt = page({1,2,3},2,Replacement::OPT);
    check(opt.steps[2].replacedPage==1,"OPT simultaneous never-again tie uses frame order");
    auto clock = page({1, 2, 3, 1, 4}, 3, Replacement::CLOCK);
    check(clock.faults == 4 && clock.steps.back().frames == std::vector<int>({4, 2, 3}), "CLOCK cyclic victim");
    check(clock.steps.back().clearedFrames == std::vector<int>({0, 1, 2}) && clock.steps.back().hand == 1, "CLOCK clears reference bits and advances");
    check(clock.steps[3].hand == 0 && clock.steps[3].referenceBits == std::vector<int>({1, 1, 1}), "CLOCK hit does not advance hand");
    auto lru = page({1, 2, 1, 3}, 2, Replacement::LRU);
    check(lru.steps[2].frames == std::vector<int>({1, 2}) && lru.steps[3].replacedPage == 2, "LRU keeps physical frame positions");
    check(lru.steps[0].nextVictim == -1 && lru.steps[0].lruOrder == std::vector<int>({0}), "free frames are not victims");
    check(lru.steps[1].nextVictim == 0 && lru.steps[1].lruOrder == std::vector<int>({0, 1}), "LRU full candidate");
    check(lru.steps[2].nextVictim == 1 && lru.steps[2].lruOrder == std::vector<int>({1, 0}), "LRU hit moves frame to MRU end");
    check(lru.steps[3].nextVictim == 0 && lru.steps[3].lruOrder == std::vector<int>({0, 1}), "LRU replacement updates order");
    check(page({7, 7}, 1, Replacement::LRU).steps.back().nextVictim == 0, "single frame remains candidate even on hit");
    for (int a = 0; a < 4; ++a) {
        auto strategy = static_cast<Replacement>(a);
        check(page({1, 1, 1}, 1, strategy).faults == 1, "all repeated");
        check(page({1, 2, 3, 4}, 1, strategy).faults == 4, "single frame");
        check(page({1, 2, 1}, 8, strategy).faults == 2, "more frames than pages");
        rejects([&] { page({}, 3, strategy); }); rejects([&] { page({1}, 0, strategy); });
        rejects([&] { page({-1}, 3, strategy); });
    }
    auto anomaly = std::vector<int>{1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
    check(page(anomaly, 3, Replacement::FIFO).faults == 9 && page(anomaly, 4, Replacement::FIFO).faults == 10, "Belady anomaly");
}
void randomizedTests() {
    std::mt19937 rng(408);
    for (int trial = 0; trial < 200; ++trial) {
        std::vector<Process> processes;
        for (int i = 0; i < 7; ++i) processes.push_back({i, int(rng() % 20), int(rng() % 8) + 1, int(rng() % 5)});
        for (int a = 0; a < 5; ++a) scheduleInvariants(processes, cpu(processes, static_cast<Scheduling>(a), int(rng() % 5) + 1));
        std::vector<int> refs; for (int i = 0; i < 40; ++i) refs.push_back(int(rng() % 9));
        int capacity = int(rng() % 6) + 1;
        int optimum = page(refs, capacity, Replacement::OPT).faults;
        check(page(refs, capacity, Replacement::FIFO).faults == referenceFaults(refs, capacity, false), "FIFO independent queue oracle");
        check(page(refs, capacity, Replacement::LRU).faults == referenceFaults(refs, capacity, true), "LRU independent list oracle");
        for (int a = 0; a < 4; ++a) {
            auto r = page(refs, capacity, static_cast<Replacement>(a));
            check(r.faults >= optimum, "OPT no worse than online strategies");
            int faults = 0; std::vector<int> previous(capacity, -1);
            for (const auto& s : r.steps) {
                bool hitBefore = std::find(previous.begin(), previous.end(), s.requestedPage) != previous.end();
                check(s.fault != hitBefore, "fault agrees with previous state");
                check(s.frames[s.touchedFrame] == s.requestedPage, "requested page is resident");
                for (int f = 0; f < capacity; ++f) if (f != s.touchedFrame) check(previous[f] == s.frames[f], "only victim slot changes");
                check(s.replacedPage == (s.fault ? previous[s.touchedFrame] : -1), "victim recorded correctly");
                previous = s.frames; faults += s.fault;
            }
            check(faults == r.faults, "fault count equals trace");
        }
    }
}
int main() {
    try { testScheduling(); testPaging(); randomizedTests(); std::cout << "PASS: " << assertions << " checks, 200 seeded randomized scenarios\n"; }
    catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
