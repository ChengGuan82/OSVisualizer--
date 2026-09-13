#include "core/Algorithms.h"
#include <algorithm>
#include <numeric>
#include <queue>
#include <set>
#include <stdexcept>
#include <tuple>

namespace osv {
// Refactored from exp/OS-exp5/GUI/process.cpp: preserve ready-queue semantics,
// remove Qt data types, add deterministic ties and a complete execution trace.
ScheduleResult StandardScheduler::run(const std::vector<Process>& input) const {
    if (input.empty() || input.size() > 100) throw std::invalid_argument("请输入 1–100 个进程");
    if (quantum_ < 1 || quantum_ > 10000) throw std::invalid_argument("时间片必须为 1–10000");
    std::set<int> ids;
    int totalBurst = 0;
    for (const auto& p : input) {
        if (p.pid < 0 || !ids.insert(p.pid).second) throw std::invalid_argument("PID 必须为不重复的非负整数");
        if (p.arrival < 0 || p.arrival > 10000 || p.burst < 1 || p.burst > 10000)
            throw std::invalid_argument("到达时间范围 0–10000；运行时间范围 1–10000");
        totalBurst += p.burst;
    }
    if (totalBurst > 20000) throw std::invalid_argument("总运行时间不能超过 20000，请缩小演示规模");
    auto p = input;
    std::stable_sort(p.begin(), p.end(), [](const Process& a, const Process& b) {
        return a.arrival < b.arrival;
    });
    const int n = static_cast<int>(p.size());
    std::vector<int> remaining(n), completion(n);
    for (int i = 0; i < n; ++i) remaining[i] = p[i].burst;
    ScheduleResult result;
    result.arrivals = p;
    int time = 0, finished = 0, nextArrival = 0;
    std::queue<int> ready;
    auto append = [&](int pid, int duration) {
        // Keep RR quantum boundaries visible, even if the same PID runs again.
        if (strategy_ != Scheduling::RR && !result.timeline.empty() &&
            result.timeline.back().pid == pid && result.timeline.back().end == time)
            result.timeline.back().end += duration;
        else result.timeline.push_back({pid, time, time + duration});
        time += duration;
    };
    auto enqueue = [&] {
        while (nextArrival < n && p[nextArrival].arrival <= time) ready.push(nextArrival++);
    };
    while (finished < n) {
        int chosen = -1;
        if (strategy_ == Scheduling::RR) {
            enqueue();
            if (!ready.empty()) { chosen = ready.front(); ready.pop(); }
        } else {
            for (int i = 0; i < n; ++i) {
                if (!remaining[i] || p[i].arrival > time) continue;
                if (chosen == -1) { chosen = i; continue; }
                if ((strategy_ == Scheduling::SJF && p[i].burst < p[chosen].burst) ||
                    (strategy_ == Scheduling::SRTF && remaining[i] < remaining[chosen]) ||
                    (strategy_ == Scheduling::Priority && p[i].priority < p[chosen].priority)) chosen = i;
            }
        }
        if (chosen < 0) {
            int next = 2000000;
            for (int i = 0; i < n; ++i)
                if (remaining[i] && p[i].arrival > time) next = std::min(next, p[i].arrival);
            append(-1, next - time);
            continue;
        }
        int duration = remaining[chosen];
        if (strategy_ == Scheduling::RR) duration = std::min(duration, quantum_);
        if (strategy_ == Scheduling::SRTF) {
            for (int i = 0; i < n; ++i)
                if (remaining[i] && p[i].arrival > time) duration = std::min(duration, p[i].arrival - time);
        }
        append(p[chosen].pid, duration);
        remaining[chosen] -= duration;
        if (!remaining[chosen]) { completion[chosen] = time; ++finished; }
        if (strategy_ == Scheduling::RR) {
            enqueue(); // Arrivals at a quantum boundary precede the requeued process.
            if (remaining[chosen]) ready.push(chosen);
        }
    }
    int busy = 0;
    // Return metrics in the original editable table order.
    for (const auto& original : input) {
        auto it = std::find_if(p.begin(), p.end(), [&](const Process& item) { return item.pid == original.pid; });
        int c = completion[std::distance(p.begin(), it)];
        int turnaround = c - original.arrival, waiting = turnaround - original.burst;
        result.processes.push_back({original.pid, c, waiting, turnaround, double(turnaround) / original.burst});
        result.averageWaiting += waiting;
        result.averageTurnaround += turnaround;
        busy += original.burst;
    }
    result.averageWaiting /= n;
    result.averageTurnaround /= n;
    result.utilization = double(busy) / time;
    return result;
}

// Based on exp/OS-exp6/GUI/page_replacement_alg.cpp. Frames are physical slots,
// separate from replacement order; CLOCK is deterministic and advances on insert.
PageResult StandardReplacement::run(const std::vector<int>& pages, int count) const {
    if (count < 1 || count > 16) throw std::invalid_argument("物理块数量必须为 1–16");
    if (pages.empty() || pages.size() > 500) throw std::invalid_argument("请输入 1–500 个页面编号");
    for (int page : pages) if (page < 0) throw std::invalid_argument("页面编号必须为非负整数");
    std::vector<int> frames(count, -1), bits(count, 0), lastUsed(count, -1);
    int hand = 0;
    PageResult result;
    for (int i = 0; i < static_cast<int>(pages.size()); ++i) {
        auto found = std::find(frames.begin(), frames.end(), pages[i]);
        bool fault = found == frames.end();
        int slot = static_cast<int>(std::distance(frames.begin(), found)), evicted = -1;
        std::vector<int> cleared;
        if (fault) {
            ++result.faults;
            if (strategy_ == Replacement::CLOCK) {
                while (bits[hand]) { bits[hand] = 0; cleared.push_back(hand); hand = (hand + 1) % count; }
                slot = hand;
                hand = (hand + 1) % count;
            } else if (strategy_ == Replacement::FIFO) {
                slot = hand;
                hand = (hand + 1) % count;
            } else {
                slot = static_cast<int>(std::distance(frames.begin(), std::find(frames.begin(), frames.end(), -1)));
                if (slot == count) {
                    if (strategy_ == Replacement::LRU)
                        slot = static_cast<int>(std::distance(lastUsed.begin(), std::min_element(lastUsed.begin(), lastUsed.end())));
                    else {
                        int farthest = -1;
                        for (int f = 0; f < count; ++f) {
                            int next = static_cast<int>(std::distance(pages.begin(), std::find(pages.begin() + i + 1, pages.end(), frames[f])));
                            if (next > farthest) { farthest = next; slot = f; }
                        }
                    }
                }
            }
            evicted = frames[slot];
            frames[slot] = pages[i];
        }
        lastUsed[slot] = i;
        bits[slot] = 1;
        PageStep snapshot{pages[i], frames, bits, fault, evicted, slot, hand, cleared, {}, -1};
        if (strategy_ == Replacement::LRU) {
            for (int f = 0; f < count; ++f) if (frames[f] >= 0) snapshot.lruOrder.push_back(f);
            std::sort(snapshot.lruOrder.begin(), snapshot.lruOrder.end(), [&](int a, int b) { return lastUsed[a] < lastUsed[b]; });
            if (int(snapshot.lruOrder.size()) == count) snapshot.nextVictim = snapshot.lruOrder.front();
        }
        result.steps.push_back(std::move(snapshot));
    }
    result.faultRate = double(result.faults) / pages.size();
    return result;
}
}
