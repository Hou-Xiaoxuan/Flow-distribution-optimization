/*
 * @Author: xv_rong
 * @Date: 2022-03-31 19:42:58
 * @LastEditors: xv_rong
 */
;
#ifndef __FFD__
#define __FFD__
#include "config.h"
#include "data.h"
#include "weight_segment_tree.h"
#include <bits/stdc++.h>

using namespace std;
class FFD {
    const Data data;
    //[mtime][customer][...] = <edge_site, stream_type>
    Distribution best_distribution;
    long long best_cost = LONG_LONG_MAX;

public:
    void init() {
        // 将同一个时刻的流量重整为一个一维数组
        //[m_time][...] <<stream>, <stream_type, customer_site>>
        vector<vector<pair<int, pair<int, int>>>> stream_per_time(data.demand.size());
        for (size_t m_time = 0; m_time < data.demand.size(); ++m_time) {
            const auto &ori_demand_now_time = data.demand[m_time];
            auto &demand_now_time = stream_per_time[m_time];
            demand_now_time.reserve(100 * ori_demand_now_time.size());
            // demand_now_time.push_back({-1, {-1, -1}}); // 偏移 1
            for (size_t customer_site = 0; customer_site < ori_demand_now_time.size(); ++customer_site) {
                for (size_t stream_index = 0; stream_index < ori_demand_now_time[customer_site].size();
                     ++stream_index) {
                    const auto &item = ori_demand_now_time[customer_site][stream_index];
                    stream_per_time[m_time].push_back({item.first, {item.second, customer_site}});
                }
            }
            demand_now_time.shrink_to_fit();
            sort(demand_now_time.begin(), demand_now_time.end(), greater<pair<int, pair<int, int>>>());
        }

        vector<int> edge_order; // 决定每轮遍历的顺序
        edge_order.reserve(data.edge_site.size());
        for (size_t edge_site = 0; edge_site < data.edge_site.size(); ++edge_site) {
            edge_order.push_back(edge_site);
        }

        int v = max(1024, data.base_cost);
        int max_bandwidth = *max_element(data.site_bandwidth.begin(), data.site_bandwidth.end());
        // TODO :  加上退火优化
        while (true) {
            vector<vector<vector<pair<int, pair<int, int>>>>> ans(
                data.demand.size(), vector<vector<pair<int, pair<int, int>>>>(data.site_bandwidth.size()));
            bool is_v_cover = true;
            for (size_t m_time = 0; m_time < data.demand.size(); ++m_time) {
                vector<int> edge_cap = data.site_bandwidth;
                for (size_t edge_site = 0; edge_site < data.edge_site.size(); ++edge_site) {
                    edge_cap[edge_site] = min(v, edge_cap[edge_site]);
                }
                for (size_t stream_index = 0; stream_index < stream_per_time[m_time].size(); ++stream_index) {
                    bool flag = false;
                    const auto &stream = stream_per_time[m_time][stream_index];
                    int stream_flow = stream.first;
                    int customer_site = stream.second.second;
                    for (const auto &edge_site : edge_order) {
                        if (edge_cap[edge_site] >= stream_flow) {

                            if (data.qos[customer_site][edge_site] < data.qos_constraint) { // qos 限制
                                ans[m_time][edge_site].push_back(stream);
                                edge_cap[edge_site] -= stream_flow;
                                flag = true;
                                break;
                            }
                        }
                    }
                    if (flag == false) { //找不到, 装不下
                        is_v_cover = false;
                        break;
                    }
                }
                if (is_v_cover == false) {
                    break;
                }
            }
            // [m_time][edge_site][...] <steam, <stream_type, customer_site>>
            // vector<vector<vector<pair<int, pair<int, int>>>>> ans;
            // 将答案整合进distribution中
            // [mtime][customer][...] = <edge_site, stream_type>
            if (is_v_cover == true) {
                Distribution distribution(data.demand.size(),
                                          vector<vector<pair<int, int>>>(data.customer_site.size()));
                for (size_t m_time = 0; m_time < data.demand.size(); ++m_time) {
                    for (size_t edge_site = 0; edge_site < data.site_bandwidth.size(); ++edge_site) {
                        for (size_t stream_index = 0; stream_index < ans[m_time][edge_site].size(); ++stream_index) {
                            const auto &stream = ans[m_time][edge_site][stream_index];
                            int customer_site = stream.second.second;
                            int stream_type = stream.second.first;
                            distribution[m_time][customer_site].push_back({edge_site, stream_type});
                        }
                    }
                }
                int cost = cal_cost(data, distribution);
                if (cost < best_cost) {
                    best_cost = cost;
                    best_distribution = distribution;
                    cout << best_cost << endl;
                }
            }
            if (v >= max_bandwidth) {
                break;
            }
            v *= 1.4;
        }
    }

    void greedy() {
        bool is_success = true;

        // [m_time][edge_site][...] <steam, <stream_type, customer_site>>
        vector<vector<vector<pair<int, pair<int, int>>>>> ans(
            data.demand.size(), vector<vector<pair<int, pair<int, int>>>>(data.site_bandwidth.size()));

        // 将同一个时刻的流量重整为一个一维数组
        //[m_time][...] <<stream>, <stream_type, customer_site>>
        vector<vector<pair<int, pair<int, int>>>> stream_per_time(data.demand.size());
        for (size_t m_time = 0; m_time < data.demand.size(); ++m_time) {
            const auto &ori_demand_now_time = data.demand[m_time];
            auto &demand_now_time = stream_per_time[m_time];
            demand_now_time.reserve(100 * ori_demand_now_time.size());
            // demand_now_time.push_back({-1, {-1, -1}}); // 偏移 1
            for (size_t customer_site = 0; customer_site < ori_demand_now_time.size(); ++customer_site) {
                for (size_t stream_index = 0; stream_index < ori_demand_now_time[customer_site].size();
                     ++stream_index) {
                    const auto &item = ori_demand_now_time[customer_site][stream_index];
                    stream_per_time[m_time].push_back({item.first, {item.second, customer_site}});
                }
            }
            demand_now_time.shrink_to_fit();
            sort(demand_now_time.begin(), demand_now_time.end(), greater<pair<int, pair<int, int>>>());
        }

        vector<WeightSegmentTree> tree(data.get_edge_num());
        for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site) {
            tree[edge_site] = WeightSegmentTree(data.site_bandwidth[edge_site]);
            tree[edge_site].update(0, 0, data.site_bandwidth[edge_site], 0, data.get_mtime_num());
        }

        // 从1开始计数的下标值
        int index_95 = (data.get_mtime_num() * 19 - 1) / 20 + 1;

        vector<int> edge_stream_95(data.get_edge_num(), 0);
        vector<int> edge_stream_96(data.get_edge_num(), 0);
        vector<vector<int>> edge_site_total_stream_per_time(data.get_mtime_num(), vector<int>(data.get_edge_num(), 0));
        vector<double> pre_edge_site_cost(data.get_mtime_num(), 0.0);

        for (size_t m_time = 0; m_time < data.get_mtime_num() && is_success; ++m_time) {
            vector<int> edge_cap = data.site_bandwidth;
            for (size_t stream_index = 0; stream_index < stream_per_time[m_time].size(); ++stream_index) {
                const auto &stream = stream_per_time[m_time][stream_index];
                const int flow = stream.first;
                const int customer_site = stream.second.second;
                vector<pair<double, int>> cost_per_edge_site;
                cost_per_edge_site.reserve(data.get_customer_num());
                for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site) {
                    /* 加不进去, 跳过*/
                    if (data.qos[customer_site][edge_site] >= data.qos_constraint || edge_cap[edge_site] < flow) {
                        continue;
                    }

                    /* 确定当前95值是多少*/
                    int new_flow = edge_site_total_stream_per_time[m_time][edge_site] + flow;
                    int new_95_flow = 0;
                    if (new_flow <= edge_stream_95[edge_site]) {
                        new_95_flow = edge_stream_95[edge_site];
                    } else if (new_flow > edge_stream_95[edge_site] && new_flow <= edge_stream_96[edge_site]) {
                        new_95_flow = new_flow;
                    } else {
                        new_95_flow = edge_stream_96[edge_site];
                    }

                    /* 计算当前cost是多少*/
                    double cost = 0.0;
                    if (new_95_flow <= data.base_cost) {
                        cost = data.base_cost;
                    } else {
                        cost = 1.0 * (new_95_flow - data.base_cost) * (new_95_flow - data.base_cost) /
                                   data.site_bandwidth[edge_site] +
                               new_95_flow;
                    }

                    /* 计算本轮增加的cost是多少*/
                    cost -= pre_edge_site_cost[edge_site];
                    cost_per_edge_site.push_back({cost, edge_site});

                    /*本轮增加0, 取这个结果不需要进一步遍历*/
                    if (cost < 1e-6) {
                        break;
                    }
                }

                if (cost_per_edge_site.empty() == true) {
                    is_success = false;
                    break;
                }

                sort(cost_per_edge_site.begin(), cost_per_edge_site.end());
                // TODO :如果是空判一下失败
                int add_cost = cost_per_edge_site.front().first;
                int edge_site = cost_per_edge_site.front().second;
                int old_flow = edge_site_total_stream_per_time[m_time][edge_site];
                tree[edge_site].update(0, 0, data.site_bandwidth[edge_site], old_flow, -1);
                int new_flow = old_flow + flow;
                edge_cap[edge_site] -= flow;
                tree[edge_site].update(0, 0, data.site_bandwidth[edge_site], new_flow, 1);

                if (new_flow > edge_stream_95[edge_site] && new_flow <= edge_stream_96[edge_site]) {
                    edge_stream_95[edge_site] = new_flow;
                } else if (new_flow > edge_stream_96[edge_site]) {
                    edge_stream_95[edge_site] = edge_stream_96[edge_site];
                    edge_stream_96[edge_site] =
                        tree[edge_site].queryK(0, 0, data.site_bandwidth[edge_site], index_95 + 1);
                }

                edge_site_total_stream_per_time[m_time][edge_site] = new_flow;
                pre_edge_site_cost[edge_site] += add_cost;
                ans[m_time][edge_site].push_back(stream);
            }
        }
        // [m_time][edge_site][...] <steam, <stream_type, customer_site>>
        // vector<vector<vector<pair<int, pair<int, int>>>>> ans;
        // 将答案整合进distribution中
        // [mtime][customer][...] = <edge_site, stream_type>
        if (is_success == true) {
            Distribution distribution(data.demand.size(), vector<vector<pair<int, int>>>(data.customer_site.size()));
            for (size_t m_time = 0; m_time < data.demand.size(); ++m_time) {
                for (size_t edge_site = 0; edge_site < data.site_bandwidth.size(); ++edge_site) {
                    for (size_t stream_index = 0; stream_index < ans[m_time][edge_site].size(); ++stream_index) {
                        const auto &stream = ans[m_time][edge_site][stream_index];
                        int customer_site = stream.second.second;
                        int stream_type = stream.second.first;
                        distribution[m_time][customer_site].push_back({edge_site, stream_type});
                    }
                }
            }
            int cost = cal_cost(data, distribution);
            if (cost < best_cost) {
                best_cost = cost;
                best_distribution = distribution;
                cout << best_cost << endl;
            }
        }
        double cost = 0;
        for (size_t edge_site = 0; edge_site < pre_edge_site_cost.size(); ++edge_site) {
            cost += pre_edge_site_cost[edge_site];
        }
        debug << (int)(cost + 0.5) << endl;
    }

    Distribution excute() {
        // init();
        greedy();
        return best_distribution;
    }
    FFD(Data data) : data(data) {
    }
};

#endif
