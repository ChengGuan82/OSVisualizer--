#include "core/Disk.h"
#include <algorithm>
#include <cstdlib>
#include <numeric>
#include <stdexcept>
namespace osv {
// Refactored from exp/OS-exp7/disk_schedule.cpp: add both directions, explicit
// disk bounds, repeated-request identity and movement accounting.
DiskResult StandardDiskScheduler::run(const DiskInput& input) const {
    if (input.maxTrack < 1 || input.maxTrack > 1000000 || input.head < 0 || input.head > input.maxTrack)
        throw std::invalid_argument("最大磁道号须为 1–1000000，磁头必须位于 0 至最大磁道号之间");
    if (input.requests.size() > 500) throw std::invalid_argument("最多输入 500 个磁盘请求");
    for (int track : input.requests) if (track < 0 || track > input.maxTrack)
        throw std::invalid_argument("所有请求必须位于 0 至最大磁道号之间");
    DiskResult result; int head = input.head;
    auto move = [&](int to, int index, DiskMoveKind kind) {
        int distance = std::abs(to - head);
        result.moves.push_back({head, to, distance, index, kind});
        result.totalMovement += distance; head = to;
        if (kind == DiskMoveKind::Request) result.sequence.push_back(to);
    };
    auto serve = [&](int index) { move(input.requests[index], index, DiskMoveKind::Request); };
    std::vector<int> indices(input.requests.size()); std::iota(indices.begin(), indices.end(), 0);
    if (strategy_ == DiskStrategy::FCFS) { for (int index : indices) serve(index); }
    else if (strategy_ == DiskStrategy::SSTF) {
        while (!indices.empty()) {
            auto best = std::min_element(indices.begin(), indices.end(), [&](int a, int b) {
                return std::abs(input.requests[a] - head) < std::abs(input.requests[b] - head);
            });
            serve(*best); indices.erase(best); // Equal distance: original input order.
        }
    } else {
        std::stable_sort(indices.begin(), indices.end(), [&](int a, int b) { return input.requests[a] < input.requests[b]; });
        std::vector<int> left, right;
        for (int index : indices) {
            if (input.requests[index] == input.head) serve(index);
            else if (input.requests[index] < input.head) left.push_back(index);
            else right.push_back(index);
        }
        auto descending = [&](std::vector<int>& v) {
            std::stable_sort(v.begin(), v.end(), [&](int a, int b) { return input.requests[a] > input.requests[b]; });
        };
        bool toRight = input.direction == Direction::Right;
        std::vector<int> forward = toRight ? right : left, other = toRight ? left : right;
        if (!toRight) descending(forward);
        if ((strategy_ == DiskStrategy::SCAN && toRight) || (strategy_ == DiskStrategy::CSCAN && !toRight)) descending(other);
        for (int index : forward) serve(index);
        // Stop at the final request; reach the physical boundary only when a
        // reversal/wrap is still needed to serve pending requests.
        if (!other.empty()) {
            int end = toRight ? input.maxTrack : 0;
            if (head != end) move(end, -1, DiskMoveKind::Boundary);
            if (strategy_ == DiskStrategy::CSCAN) move(toRight ? 0 : input.maxTrack, -1, DiskMoveKind::Wrap);
            for (int index : other) serve(index);
        }
    }
    if (!input.requests.empty()) result.averageMovement = double(result.totalMovement) / input.requests.size();
    return result;
}
}
