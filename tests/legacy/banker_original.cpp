#include <iostream>
#include <vector>
using namespace std;

// 判断是否安全的函数
bool isSafe(vector<int> &available, vector<vector<int>> &max, vector<vector<int>> &allocation, vector<vector<int>> &need)
{
    int numProcesses = allocation.size();
    int numResources = available.size();
    vector<bool> finish(numProcesses, false);
    vector<int> work = available;

    for (int i = 0; i < numProcesses; ++i) 
    {
        for (int j = 0; j < numProcesses; ++j) 
        {
            if (!finish[j]) 
            {
                bool canAllocate = true;
                for (int k = 0; k < numResources; ++k) 
                {
                    if (need[j][k] > work[k]) 
                    {
                        canAllocate = false;
                        break;
                    }
                }
                if (canAllocate) {
                    for (int k = 0; k < numResources; ++k)
                    {
                        work[k] += allocation[j][k];
                    }
                    finish[j] = true;
                    i = -1; // Restart the process check
                }
            }
        }
    }

    for (bool f : finish) 
    {
        if (!f) return false;
    }
    return true;
}

int main() 
{
    int numProcesses = 5, numResources = 3;

    // 可用资源向量
    vector<int> available = {3, 3, 2};

    // 最大需求矩阵
    vector<vector<int>> max = {
        {7, 5, 3},
        {3, 2, 2},
        {9, 0, 2},
        {2, 2, 2},
        {4, 3, 3}
    };

    // 分配矩阵
    vector<vector<int>> allocation = {
        {0, 1, 0},
        {2, 0, 0},
        {3, 0, 2},
        {2, 1, 1},
        {0, 0, 2}
    };

    // 计算需求矩阵
    vector<vector<int>> need(numProcesses, vector<int>(numResources));
    for (int i = 0; i < numProcesses; ++i) 
    {
        for (int j = 0; j < numResources; ++j) 
        {
            need[i][j] = max[i][j] - allocation[i][j];
        }
    }

    while (true) 
    {
        int process;
        vector<int> request(numResources);

        cout << "输入需要申请资源的进程编号 (-1退出): ";
        cin >> process;
        if (process == -1) break;

        cout << "输入申请的资源数量: ";
        for (int i = 0; i < numResources; ++i) 
        {
            cin >> request[i];
        }

        // 检查申请是否超出声明
        bool validRequest = true;
        for (int i = 0; i < numResources; ++i) 
        {
            if (request[i] > need[process][i]) 
            {
                validRequest = false;
                break;
            }
        }

        if (!validRequest) 
        {
            cout << "错误: 申请的资源数量超出声明的最大需求!" << endl;
            continue;
        }

        // 检查系统是否有足够资源
        for (int i = 0; i < numResources; ++i) 
        {
            if (request[i] > available[i]) 
            {
                validRequest = false;
                break;
            }
        }

        if (!validRequest)
        {
            cout << "错误: 系统没有足够的资源满足申请!" << endl;
            continue;
        }

        // 尝试分配资源
        for (int i = 0; i < numResources; ++i) 
        {
            available[i] -= request[i];
            allocation[process][i] += request[i];
            need[process][i] -= request[i];
        }

        // 检查是否安全
        if (isSafe(available, max, allocation, need)) 
        {
            cout << "资源分配成功，系统处于安全状态!" << endl;
        } else {
            // 回滚分配
            for (int i = 0; i < numResources; ++i) 
            {
                available[i] += request[i];
                allocation[process][i] -= request[i];
                need[process][i] += request[i];
            }
            cout << "资源分配失败，系统处于不安全状态!" << endl;
        }
    }

    return 0;
}