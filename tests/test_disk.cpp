#include "core/Disk.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>
#include <sstream>
#include <functional>
using namespace osv;
void FCFS(const std::vector<int>&);
void SSTF(std::vector<int>, int);
void SCAN(std::vector<int>, int, int);
void CSCAN(std::vector<int>, int, int);
std::vector<int> legacyOrder(const std::function<void()>& action) {
    std::ostringstream output; auto* original=std::cout.rdbuf(output.rdbuf());
    action(); std::cout.rdbuf(original);
    auto text=output.str(); std::istringstream values(text.substr(text.find(':')+1));
    std::vector<int> order; int track; while(values>>track)order.push_back(track); return order;
}
void require(bool value, const char* text) { if (!value) throw std::runtime_error(text); }
DiskResult run(DiskInput input, DiskStrategy strategy) { return StandardDiskScheduler(strategy).run(input); }
void verify(const DiskInput& input, const DiskResult& r) {
    auto expected=input.requests, actual=r.sequence;
    std::sort(expected.begin(),expected.end()); std::sort(actual.begin(),actual.end());
    require(actual==expected,"request multiset preserved");
    int head=input.head, distance=0; std::vector<bool> seen(input.requests.size(),false);
    for(const auto& m:r.moves) {
        require(m.from==head && m.distance==std::abs(m.to-head),"continuous path");
        require(m.to>=0 && m.to<=input.maxTrack,"inside disk");
        if(m.kind==DiskMoveKind::Request) {
            require(m.requestIndex>=0 && m.requestIndex<int(seen.size()) && !seen[m.requestIndex],"serve request once");
            seen[m.requestIndex]=true; require(input.requests[m.requestIndex]==m.to,"request identity");
        }
        head=m.to; distance+=m.distance;
    }
    require(distance==r.totalMovement,"sum of movement");
    require(std::abs(r.averageMovement-(expected.empty()?0:double(distance)/expected.size()))<1e-9,"average denominator");
}
int main() {
    try {
        DiskInput input{53,199,{98,183,37,122,14,124,65,67},Direction::Right};
        require(run(input,DiskStrategy::FCFS).totalMovement==640,"FCFS fixture");
        require(run(input,DiskStrategy::SSTF).totalMovement==236,"SSTF fixture");
        require(run(input,DiskStrategy::SCAN).totalMovement==331,"SCAN right fixture");
        require(run(input,DiskStrategy::CSCAN).totalMovement==382,"CSCAN includes circular return");
        require(run(input,DiskStrategy::FCFS).sequence==legacyOrder([&]{FCFS(input.requests);}),"original exp7 FCFS");
        require(run(input,DiskStrategy::SSTF).sequence==legacyOrder([&]{SSTF(input.requests,input.head);}),"original exp7 SSTF");
        for(auto a:{DiskStrategy::SCAN,DiskStrategy::CSCAN}) {
            auto r=run(input,a); std::vector<int> path; for(const auto& m:r.moves)path.push_back(m.to);
            auto old=legacyOrder([&]{if(a==DiskStrategy::SCAN)SCAN(input.requests,input.head,200);else CSCAN(input.requests,input.head,200);});
            require(path==old,"original exp7 boundary path");
        }
        input.direction=Direction::Left;
        require(run(input,DiskStrategy::SCAN).totalMovement==236,"SCAN left fixture");
        require(run(input,DiskStrategy::CSCAN).totalMovement==386,"CSCAN left fixture");
        for(int a=0;a<4;++a) for(auto dir:{Direction::Left,Direction::Right}) {
            auto strategy=static_cast<DiskStrategy>(a);
            for(auto refs:std::vector<std::vector<int>>{{},{53},{53,53},{14},{65,80},{1,2,3},{0,199,53,0,199}}) {
                DiskInput v{53,199,refs,dir}; verify(v,run(v,strategy));
            }
        }
        require(run({50,99,{40,60},Direction::Right},DiskStrategy::SSTF).sequence[0]==40,"SSTF deterministic tie");
        for(auto v:std::vector<DiskInput>{{-1,199,{},Direction::Right},{1,0,{},Direction::Right},{53,199,{200},Direction::Right},{53,199,{-1},Direction::Right}}) {
            bool rejected=false; try{run(v,DiskStrategy::FCFS);}catch(const std::invalid_argument&){rejected=true;}
            require(rejected,"invalid input rejected");
        }
        std::mt19937 rng(408);
        for(int t=0;t<200;++t) {
            DiskInput v{int(rng()%200),199,{},Direction::Right};
            for(int i=0;i<20;++i)v.requests.push_back(int(rng()%200));
            for(int a=0;a<4;++a) {
                auto strategy=static_cast<DiskStrategy>(a); auto r=run(v,strategy); verify(v,r);
                if(a==1)require(r.sequence==legacyOrder([&]{SSTF(v.requests,v.head);}),"original SSTF randomized agreement");
                if(a>=2) {
                    auto mirror=v; mirror.head=199-v.head; mirror.direction=Direction::Left;
                    for(int& p:mirror.requests)p=199-p;
                    require(run(mirror,strategy).totalMovement==r.totalMovement,"left-right symmetry");
                }
            }
        }
        std::cout<<"PASS: disk fixtures, boundaries and 200 seeded scenarios\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
