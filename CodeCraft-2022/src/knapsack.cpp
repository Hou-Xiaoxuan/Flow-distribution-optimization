/*
 * @Author: xv_rong
 * @Date: 2022-03-31 19:42:58
 * @LastEditors: xv_rong
 */
;
#ifndef __KNAPSACK__
#define __KNAPSACK__
#include "config.h"
#include "data.h"
#include <bits/stdc++.h>

using namespace std;
class Knapsack {
    const Data data;

public:
    Distribution excute() {
        // [m_time][edge_site][...] <stream, <stream_type, customer_site>>
        vector<vector<vector<pair<int, pair<int, int>>>>> ans(
            data.demand.size(), vector<vector<pair<int, pair<int, int>>>>(data.site_bandwidth.size()));

        // 将同一个时刻的流量重整为一个一维数组
        //[m_time][...] <<stream>, <stream_type, customer_site>>
        vector<vector<pair<int, pair<int, int>>>> demand(data.demand.size());

        for (size_t m_time = 0; m_time < data.demand.size(); ++m_time) {
            const auto &ori_demand_now_time = data.demand[m_time];
            auto &demand_now_time = demand[m_time];
            demand_now_time.reserve(100 * ori_demand_now_time.size() + 1);
            demand_now_time.push_back({-1, {-1, -1}}); // 偏移 1
            for (size_t customer_site = 0; customer_site < ori_demand_now_time.size(); ++customer_site) {
                for (size_t stream_index = 0; stream_index < ori_demand_now_time[customer_site].size();
                     ++stream_index) {
                    const auto &item = ori_demand_now_time[customer_site][stream_index];
                    demand[m_time].push_back({item.first, {item.second, customer_site}});
                }
            }
        }

        for (size_t m_time = 0; m_time < data.demand.size(); ++m_time) {
            auto &ans_now_time = ans[m_time];
            for (size_t edge_site = 0; edge_site < data.site_bandwidth.size(); ++edge_site) {
                // TODO: 可能缺少一些停止条件

                auto demand_now_time = demand[m_time];
                if (demand_now_time.size() == 1) {
                    break;
                }

                int edge_bandwidth = data.site_bandwidth[edge_site];
                // 求所有流的最大公因数, 最大公因数 减小复杂度
                int d = edge_bandwidth;
                for (size_t stream_index = 0; stream_index < demand_now_time.size(); ++stream_index) {
                    d = __gcd(d, demand_now_time[stream_index].first);
                }
                for (size_t stream_index = 0; stream_index < demand_now_time.size(); ++stream_index) {
                    demand_now_time[stream_index].first /= d;
                }
                edge_bandwidth /= d;

                // 开始背包
                vector<vector<bool>> dp(demand_now_time.size() + 1, vector<bool>(edge_bandwidth + 1, false));
                // 对于每一个物品, 都可以从dp[i][0] 都应该是合法状态,要设置所有dp[i][0] 为true
                for (size_t i = 0 /*无偏移*/; i <= demand_now_time.size(); ++i) {
                    dp[i][0] = true;
                }
                for (size_t i = 1; i <= demand_now_time.size(); ++i) {
                    for (int j = 0; j <= edge_bandwidth; ++j) {
                        if (j < demand_now_time[i].first) {
                            dp[i][j] = dp[i - 1][j];
                        } else {
                            dp[i][j] = dp[i - 1][j] || dp[i - 1][j - demand_now_time[i].first];
                        }
                    }
                }

                // 找到最大的一个能填充的体积
                int max_bandwidth = -1;
                for (size_t i = demand_now_time.size(), j = edge_bandwidth; j >= 0; --j) {
                    if (dp[i][j] == true) {
                        max_bandwidth = j;
                        break;
                    }
                }

                // 获得这个背包内的答案
                for (size_t j = max_bandwidth, i = demand_now_time.size(); i >= 1 && j > 0; --i) {
                    if (dp[i - 1][j - demand_now_time[i].first] == true) {
                        ans_now_time[edge_site].push_back(demand_now_time[i]);
                        j -= demand_now_time[i].first;
                    }
                }

                // 将流放缩回原来的大小, 并且从demand中删除对应的流
                for (size_t i = 0; i < ans_now_time[edge_site].size(); ++i) {
                    auto &item = ans_now_time[edge_site][i];
                    item.first *= d;
                    demand[m_time].erase(find(demand[m_time].begin(), demand[m_time].end(), item));
                }
            }
        }
        // [m_time][edge_site][...] <steam, <stream_type, customer_site>>
        // vector<vector<vector<pair<int, pair<int, int>>>>> ans;
        // 将答案整合进distribution中
        // [mtime][customer][...] = <edge_site, stream_type>
        Distribution distribution(data.demand.size(), vector<vector<pair<int, int>>>(data.site_bandwidth.size()));
        for (size_t m_time = 0; m_time < data.demand.size(); ++m_time) {
            for (size_t edge_site = 0; edge_site < data.site_bandwidth.size(); ++edge_site) {
                for (int stream_index = 0; stream_index < ans[m_time][edge_site].size(); ++stream_index) {
                    const auto &stream = ans[m_time][edge_site][stream_index];
                    int customer_site = stream.second.second;
                    int stream_type = stream.second.first;
                    distribution[m_time][customer_site].push_back({edge_site, stream_type});
                }
            }
        }
        return distribution;
    }
    Knapsack(Data data) : data(data) {
    }
};

#endif
