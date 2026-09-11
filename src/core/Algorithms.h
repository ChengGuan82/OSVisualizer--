#pragma once
#include <vector>

namespace osv {
// Integer simulation time; no context-switch overhead. PID -1 denotes idle.
struct Process { int pid, arrival, burst, priority; };
struct ProcessResult {
    int pid, completion, waiting, turnaround;
    double weightedTurnaround;
};
struct Slice { int pid, start, end; };
struct ScheduleResult {
    std::vector<ProcessResult> processes;
    std::vector<Slice> timeline;
    std::vector<Process> arrivals;
    double averageWaiting = 0, averageTurnaround = 0, utilization = 0;
};
enum class Scheduling { FCFS, SJF, SRTF, RR, Priority };
class Scheduler {
public:
    virtual ~Scheduler() = default;
    virtual ScheduleResult run(const std::vector<Process>& processes) const = 0;
};
class StandardScheduler final : public Scheduler {
public:
    explicit StandardScheduler(Scheduling strategy, int quantum = 2)
        : strategy_(strategy), quantum_(quantum) {}
    ScheduleResult run(const std::vector<Process>& processes) const override;
private:
    Scheduling strategy_;
    int quantum_;
};

enum class Replacement { FIFO, LRU, CLOCK, OPT };
struct PageStep {
    int requestedPage;
    std::vector<int> frames; // -1 means empty; physical positions stay fixed.
    std::vector<int> referenceBits;
    bool fault;
    int replacedPage, touchedFrame, hand;
    std::vector<int> clearedFrames; // CLOCK scan trace for this access.
    std::vector<int> lruOrder; // Occupied frame indices: least to most recently used.
    int nextVictim = -1; // LRU candidate after this access; -1 while a free frame exists.
};
struct PageResult {
    std::vector<PageStep> steps;
    int faults = 0;
    double faultRate = 0;
};
class PageReplacement {
public:
    virtual ~PageReplacement() = default;
    virtual PageResult run(const std::vector<int>& references, int frameCount) const = 0;
};
class StandardReplacement final : public PageReplacement {
public:
    explicit StandardReplacement(Replacement strategy) : strategy_(strategy) {}
    PageResult run(const std::vector<int>& references, int frameCount) const override;
private:
    Replacement strategy_;
};
}
