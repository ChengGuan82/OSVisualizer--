#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>

using namespace std;

// 排序函数
void sortAscending(vector<int>& requests) {
    sort(requests.begin(), requests.end());
}

// FCFS调度算法
void FCFS(const vector<int>& requests) {
    cout << "FCFS调度顺序: ";
    for (int request : requests) {
        cout << request << " ";
    }
    cout << endl;
}

// SSTF调度算法
void SSTF(vector<int> requests, int start) {
    cout << "SSTF调度顺序: ";
    while (!requests.empty()) {
        auto closest = min_element(requests.begin(), requests.end(),
                                   [start](int a, int b) { return abs(a - start) < abs(b - start); });
        cout << *closest << " ";
        start = *closest;
        requests.erase(closest);
    }
    cout << endl;
}

// SCAN调度算法
void SCAN(vector<int> requests, int start, int range) {
    sortAscending(requests);
    cout << "SCAN调度顺序: ";
    vector<int> left, right;
    for (int request : requests) {
        if (request < start) left.push_back(request);
        else right.push_back(request);
    }
    reverse(left.begin(), left.end());
    for (int request : right) cout << request << " ";
    cout << range - 1 << " "; // 磁头移动到底
    for (int request : left) cout << request << " ";
    cout << endl;
}

// LOOK调度算法
void LOOK(vector<int> requests, int start) {
    sortAscending(requests);
    cout << "LOOK调度顺序: ";
    vector<int> left, right;
    for (int request : requests) {
        if (request < start) left.push_back(request);
        else right.push_back(request);
    }
    reverse(left.begin(), left.end());
    if (!right.empty()) {
        for (int request : right) cout << request << " ";
    }
    if (!left.empty()) {
        for (int request : left) cout << request << " ";
    }
    cout << endl;
}

// CSCAN调度算法
void CSCAN(vector<int> requests, int start, int range) {
    sortAscending(requests);
    cout << "CSCAN调度顺序: ";
    vector<int> left, right;
    for (int request : requests) {
        if (request < start) left.push_back(request);
        else right.push_back(request);
    }
    for (int request : right) cout << request << " ";
    cout << range - 1 << " "; // 磁头移动到最大值
    cout << 0 << " ";         // 磁头回到最小值
    for (int request : left) cout << request << " ";
    cout << endl;
}

// CLOOK调度算法
void CLOOK(vector<int> requests, int start) {
    sortAscending(requests);
    cout << "CLOOK调度顺序: ";
    vector<int> left, right;
    for (int request : requests) {
        if (request < start) left.push_back(request);
        else right.push_back(request);
    }
    for (int request : right) cout << request << " ";
    for (int request : left) cout << request << " "; // 磁头直接跳到最小值处理剩余请求
    cout << endl;
}

// NStepSCAN调度算法
void NStepSCAN(vector<int> requests, int start, int range, int N) {
    cout << "NStepSCAN调度顺序: ";
    sortAscending(requests); // 先对请求队列进行排序
    vector<vector<int>> subQueues;

    // 将请求队列分成长度为N的子队列
    for (size_t i = 0; i < requests.size(); i += N) {
        vector<int> subQueue(requests.begin() + i, requests.begin() + min(requests.size(), i + N));
        subQueues.push_back(subQueue);
    }

    // 按照FCFS顺序处理每个子队列，子队列内部使用SCAN算法
    for (auto& subQueue : subQueues) {
        vector<int> left, right;
        for (int request : subQueue) {
            if (request < start) left.push_back(request);
            else right.push_back(request);
        }
        reverse(left.begin(), left.end());
        for (int request : right) cout << request << " ";
        cout << range - 1 << " "; // 磁头移动到底
        for (int request : left) cout << request << " ";
    }
    cout << endl;
}

// FSCAN调度算法
void FSCAN(vector<int> requests, int start, int range) {
    cout << "FSCAN调度顺序: ";
    vector<int> currentQueue = requests; // 当前请求队列
    vector<int> newQueue;                // 新出现的请求队列

    // 模拟扫描期间新出现的请求
    srand(time(0));
    for (int i = 0; i < rand() % 5 + 1; ++i) { // 随机生成一些新请求
        newQueue.push_back(rand() % range);
    }

    // 处理当前队列（按照SCAN算法）
    sortAscending(currentQueue);
    vector<int> left, right;
    for (int request : currentQueue) {
        if (request < start) left.push_back(request);
        else right.push_back(request);
    }
    reverse(left.begin(), left.end());
    for (int request : right) cout << request << " ";
    cout << range - 1 << " "; // 磁头移动到底
    for (int request : left) cout << request << " ";

    // 处理新队列（按照SCAN算法）
    cout << "\n新出现的请求队列: ";
    for (int request : newQueue) cout << request << " ";
    cout << "\n处理新队列: ";
    sortAscending(newQueue);
    left.clear();
    right.clear();
    for (int request : newQueue) {
        if (request < start) left.push_back(request);
        else right.push_back(request);
    }
    reverse(left.begin(), left.end());
    for (int request : right) cout << request << " ";
    cout << range - 1 << " "; // 磁头移动到底
    for (int request : left) cout << request << " ";
    cout << endl;
}

int main() {
    int totalRequests, range, start, N;
    cout << "请输入需要访问的磁盘总数: ";
    cin >> totalRequests;
    cout << "请输入磁块范围: ";
    cin >> range;
    cout << "请输入磁头起始位置: ";
    cin >> start;

    vector<int> requests(totalRequests);
    srand(time(0));
    cout << "随机生成的磁块号: ";
    for (int i = 0; i < totalRequests; ++i) {
        requests[i] = rand() % range;
        cout << requests[i] << " ";
    }
    cout << endl;

    int choice;
    while (true) {
        cout << "选择磁盘调度算法 (0: 退出, 1: FCFS, 2: SSTF, 3: SCAN, 4: LOOK, 5: CSCAN, 6: CLOOK, 7: NStepSCAN, 8: FSCAN): ";
        cin >> choice;
        if (choice == 0) {
            cout << "程序已退出。" << endl;
            break;
        }
        switch (choice) {
            case 1:
                FCFS(requests);
                break;
            case 2:
                SSTF(requests, start);
                break;
            case 3:
                SCAN(requests, start, range);
                break;
            case 4:
                LOOK(requests, start);
                break;
            case 5:
                CSCAN(requests, start, range);
                break;
            case 6:
                CLOOK(requests, start);
                break;
            case 7:
                cout << "请输入子队列长度N: ";
                cin >> N;
                NStepSCAN(requests, start, range, N);
                break;
            case 8:
                FSCAN(requests, start, range);
                break;
            default:
                cout << "无效选择!" << endl;
        }
    }

    return 0;
}