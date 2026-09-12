#pragma once
#include <vector>
namespace osv {
enum class DiskStrategy { FCFS, SSTF, SCAN, CSCAN };
enum class Direction { Left, Right };
enum class DiskMoveKind { Request, Boundary, Wrap };
struct DiskInput {
    int head = 53, maxTrack = 199;
    std::vector<int> requests;
    Direction direction = Direction::Right;
};
struct DiskMove { int from, to, distance, requestIndex; DiskMoveKind kind; };
struct DiskResult {
    std::vector<int> sequence; // Serviced requests only, including duplicates.
    std::vector<DiskMove> moves; // Also includes boundary travel and circular return.
    int totalMovement = 0;
    double averageMovement = 0;
};
class DiskScheduler {
public:
    virtual ~DiskScheduler() = default;
    virtual DiskResult run(const DiskInput& input) const = 0;
};
class StandardDiskScheduler final : public DiskScheduler {
public:
    explicit StandardDiskScheduler(DiskStrategy strategy) : strategy_(strategy) {}
    DiskResult run(const DiskInput& input) const override;
private:
    DiskStrategy strategy_;
};
}
