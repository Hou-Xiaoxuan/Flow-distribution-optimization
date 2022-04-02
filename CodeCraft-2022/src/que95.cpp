/*
 * @Author: xv_rong
 * @Date: 2022-04-02 10:11:09
 * @LastEditors: xv_rong
 */
#ifndef __QUE95__
#define __QUE95__
#include "data.h"
#include <bits/stdc++.h>
using namespace std;
struct Que95 {
    //大堆顶
    priority_queue<pair<int, int>> small_95;
    // 小堆顶, 堆顶是计费的那个时刻
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> big_5;

    int get_cost() {
        return small_95.top().first;
    }
    int get_cost_m_time() {
        return small_95.top().second;
    }
    void init(const vector<int> &edge_stream_per_m_time) {
        // 花费从小到大排序后, 下标0开始的开始, 计费的时刻的下标
        vector<pair<int, int>> tmp(edge_stream_per_m_time.size());
        for (size_t m_time = 0; m_time < edge_stream_per_m_time.size(); ++m_time) {
            tmp[m_time] = {edge_stream_per_m_time[m_time], m_time};
        }
        small_95 = priority_queue<pair<int, int>>(tmp.begin(), tmp.end());

        int free_size = edge_stream_per_m_time.size() - ((edge_stream_per_m_time.size() * 19 - 1) / 20 + 1);
        tmp.resize(free_size);

        for (auto &item : tmp) {
            item = small_95.top();
            small_95.pop();
        }
        big_5 = priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>>(tmp.begin(), tmp.end());
    }

    void update(const pair<int, int> &new_edge_stream) {
        small_95.pop(); // 清除原来的95值
        big_5.push(new_edge_stream);
        small_95.push(big_5.top());
        big_5.pop();
    }
};
#endif