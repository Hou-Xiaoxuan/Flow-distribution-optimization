/*
 * @Author: xv_rong
 * @Date: 2022-03-31 19:42:58
 * @LastEditors: xv_rong
 */
#ifndef __FFD__
#define __FFD__
#include "config.h"
#include "data.h"
#include "weight_segment_tree.h"
#include <bits/stdc++.h>

using namespace std;
class FFD {
    const Data data;

    /*
    将同一个时刻的储存在同一维度中, 方便存取
    [m_time][...] <<stream>, <stream_type, customer_site>>
    */
    vector<vector<pair<int, pair<int, int>>>> stream_per_time;

    //[mtime][customer][...] = <edge_site, stream_type>
    Distribution best_distribution;

    double best_cost = (double)LONG_LONG_MAX;

    //[edge_site] = flow_94
    vector<int> edge_site_flow_94;

    //[edge_site] = flow_95
    vector<int> edge_site_flow_95;

    //[edge_site] = flow_96
    vector<int> edge_site_flow_96;

    //[edge_site] = edge_site_tree
    vector<WeightSegmentTree> tree;

    // [edge_site][flow][...] = m_time
    vector<vector<set<int>>> flow_to_m_time_per_eige_site;

    //[m_time][edge_site] = total_flow_per_edge_site_per_time
    vector<vector<int>> edge_site_total_stream_per_time;

    // [edge_site] = total_stream_per_edge_site
    vector<long long> total_stream_per_edge_site;

    // [edge_site] = edge_site_cost
    vector<double> cost_per_edge_site;

    // [m_time][edge_site][...] <stream, <stream_type, customer_site>>
    vector<vector<vector<pair<int, pair<int, int>>>>> ans;

    // 从1开始计数的95下标值
    int index_94;

    int index_95;

    int index_96;

public:
    void SA(double T, double dT, double end_T) {
        double now_cost = best_cost;

        vector<int> valid_edge_site;
        valid_edge_site.reserve(data.get_edge_num());
        while (T > end_T) {
            // 注意%0错误

            /*随机选择 edge_site_one*/
            int edge_site_one = rand() % data.get_edge_num();

            if (edge_site_flow_95[edge_site_one] == 0) {
                T = T * dT; //降温
                continue;   // 说明95值时没有flow
            }

            /*选择edge_site_one 95值的时点*/
            int m_time = *flow_to_m_time_per_eige_site[edge_site_one][edge_site_flow_95[edge_site_one]].begin();

            /*随机选择stream*/
            auto &stream = ans[m_time][edge_site_one][rand() % ans[m_time][edge_site_one].size()];
            const int flow = stream.first;
            const int customer_site = stream.second.second;

            /*随机选择edge_site_two*/
            // 挑选出所有满足qos约束 能放入flow 且不是edge_site_one的edge_site
            valid_edge_site.resize(0);
            for (int edge_site = 0; edge_site < (int)data.get_edge_num(); ++edge_site) {
                if (data.qos[customer_site][edge_site] < data.qos_constraint &&
                    edge_site_total_stream_per_time[m_time][edge_site] + flow <= data.site_bandwidth[edge_site] &&
                    edge_site != edge_site_one) {
                    valid_edge_site.push_back(edge_site);
                }
            }
            if (valid_edge_site.empty() == true) {
                T = T * dT; //降温
                continue;
            }
            int edge_site_two = valid_edge_site[rand() % valid_edge_site.size()];

            /* 拿出后 edge_site_one 花费是多少*/
            int old_flow_one = edge_site_total_stream_per_time[m_time][edge_site_one];
            int new_flow_one = old_flow_one - flow;

            /* 确定new_flow_95_one值是多少*/

            int new_flow_95_one = 0;
            if (data.get_mtime_num() == 1) { // 特殊情况只有一个时点
                new_flow_95_one = new_flow_one;
            } else {
                if (new_flow_one >= edge_site_flow_94[edge_site_one]) {
                    new_flow_95_one = new_flow_one;
                } else {
                    new_flow_95_one = edge_site_flow_94[edge_site_one];
                }
            }

            double cost_one = 0.0;
            if (total_stream_per_edge_site[edge_site_one] - flow != 0) {
                if (new_flow_95_one <= data.base_cost) {
                    cost_one = data.base_cost;
                } else {
                    cost_one = 1.0 * (new_flow_95_one - data.base_cost) * (new_flow_95_one - data.base_cost) /
                                   data.site_bandwidth[edge_site_one] +
                               new_flow_95_one;
                }
            }

            /* 放入后 edge_site_two 花费是多少*/
            const int old_flow_two = edge_site_total_stream_per_time[m_time][edge_site_two];
            const int new_flow_two = old_flow_two + flow;

            /* 确定new_flow_95_two值是多少*/
            int new_flow_95_two = 0;

            if (data.get_mtime_num() < 20) { // 特殊情况时点小于20
                if (new_flow_two > edge_site_flow_95[edge_site_two]) {
                    new_flow_95_two = new_flow_two;
                } else {
                    new_flow_95_two = edge_site_flow_95[edge_site_two];
                }
            } else {
                if (old_flow_two > edge_site_flow_95[edge_site_two]) {
                    new_flow_95_two = edge_site_flow_95[edge_site_two];
                } else {
                    if (new_flow_two > edge_site_flow_96[edge_site_two]) {
                        new_flow_95_two = edge_site_flow_96[edge_site_two];
                    } else if (new_flow_two > edge_site_flow_95[edge_site_two]) {
                        new_flow_95_two = new_flow_two;
                    } else {
                        new_flow_95_two = edge_site_flow_95[edge_site_two];
                    }
                }
            }

            double cost_two = 0.0;
            if (new_flow_95_two <= data.base_cost) {
                cost_two = data.base_cost;
            } else {
                cost_two = 1.0 * (new_flow_95_two - data.base_cost) * (new_flow_95_two - data.base_cost) /
                               data.site_bandwidth[edge_site_two] +
                           new_flow_95_two;
            }

            /*考虑是否接受更新*/
            int is_acc = false;
            const double df =
                (cost_one + cost_two) - (cost_per_edge_site[edge_site_one] + cost_per_edge_site[edge_site_two]);
            if (df < 0.0) {
                is_acc = true;
            } else if (exp(-df / T) * RAND_MAX > rand()) {
                is_acc = true;
            }

            if (is_acc == true) {
                // 先插入
                tree[edge_site_two].update(0, 0, data.site_bandwidth[edge_site_two], old_flow_two, -1);
                tree[edge_site_two].update(0, 0, data.site_bandwidth[edge_site_two], new_flow_two, 1);

                // 维护94 95 96 的值
                if (data.get_mtime_num() < 20) { // 特殊情况时点小于20 不需要维护96值
                    if (old_flow_two == edge_site_flow_95[edge_site_two]) {
                        edge_site_flow_95[edge_site_two] = new_flow_two;
                    } else if (old_flow_two <= edge_site_flow_94[edge_site_two]) {
                        if (new_flow_two > edge_site_flow_95[edge_site_two]) {
                            if (data.get_mtime_num() > 1) {
                                edge_site_flow_94[edge_site_two] = edge_site_flow_95[edge_site_two];
                            }
                            edge_site_flow_95[edge_site_two] = new_flow_two;
                        } else if (new_flow_two > edge_site_flow_94[edge_site_two]) {
                            if (data.get_mtime_num() > 1) {
                                edge_site_flow_94[edge_site_two] = new_flow_two;
                            }
                        }
                    }
                } else {
                    if (old_flow_two >= edge_site_flow_96[edge_site_two]) {
                        if (old_flow_two == edge_site_flow_96[edge_site_two]) {
                            edge_site_flow_96[edge_site_two] =
                                tree[edge_site_two].queryK(0, 0, data.site_bandwidth[edge_site_two], index_96);
                        }
                    } else if (old_flow_two == edge_site_flow_95[edge_site_two]) {
                        if (new_flow_two > edge_site_flow_96[edge_site_two]) {
                            edge_site_flow_95[edge_site_two] = edge_site_flow_96[edge_site_two];
                            edge_site_flow_96[edge_site_two] =
                                tree[edge_site_two].queryK(0, 0, data.site_bandwidth[edge_site_two], index_96);
                        } else if (new_flow_two > edge_site_flow_95[edge_site_two]) {
                            edge_site_flow_95[edge_site_two] = new_flow_two;
                        }
                    } else if (old_flow_two <= edge_site_flow_94[edge_site_two]) {
                        if (new_flow_two > edge_site_flow_96[edge_site_two]) {
                            edge_site_flow_94[edge_site_two] = edge_site_flow_95[edge_site_two];
                            edge_site_flow_95[edge_site_two] = edge_site_flow_96[edge_site_two];
                            edge_site_flow_96[edge_site_two] =
                                tree[edge_site_two].queryK(0, 0, data.site_bandwidth[edge_site_two], index_96);
                        } else if (new_flow_two > edge_site_flow_95[edge_site_two]) {
                            edge_site_flow_94[edge_site_two] = edge_site_flow_95[edge_site_two];
                            edge_site_flow_95[edge_site_two] = new_flow_two;
                        } else if (new_flow_two > edge_site_flow_94[edge_site_two]) {
                            edge_site_flow_94[edge_site_two] = new_flow_two;
                        }
                    }
                }

                flow_to_m_time_per_eige_site[edge_site_two][old_flow_two].erase(m_time);
                flow_to_m_time_per_eige_site[edge_site_two][new_flow_two].insert(m_time);
                edge_site_flow_95[edge_site_two] = new_flow_95_two;
                edge_site_total_stream_per_time[m_time][edge_site_two] = new_flow_two;
                total_stream_per_edge_site[edge_site_two] += flow;
                cost_per_edge_site[edge_site_two] = cost_two;
                ans[m_time][edge_site_two].push_back(stream);

                // 再删除
                tree[edge_site_one].update(0, 0, data.site_bandwidth[edge_site_one], old_flow_one, -1);
                tree[edge_site_one].update(0, 0, data.site_bandwidth[edge_site_one], new_flow_one, 1);

                // 维护94 95 96 的值
                if (data.get_mtime_num() == 1) { // 特殊情况只有一个时点
                    edge_site_flow_95[edge_site_one] = new_flow_one;
                } else {
                    if (new_flow_one >= edge_site_flow_94[edge_site_one]) {
                        edge_site_flow_95[edge_site_one] = new_flow_one;
                    } else {
                        edge_site_flow_95[edge_site_one] = edge_site_flow_94[edge_site_one];
                        edge_site_flow_94[edge_site_one] =
                            tree[edge_site_one].queryK(0, 0, data.site_bandwidth[edge_site_one], index_94);
                    }
                }
                flow_to_m_time_per_eige_site[edge_site_one][old_flow_one].erase(
                    flow_to_m_time_per_eige_site[edge_site_one][old_flow_one].begin());
                flow_to_m_time_per_eige_site[edge_site_one][new_flow_one].insert(m_time);
                edge_site_flow_95[edge_site_one] = new_flow_95_one;
                edge_site_total_stream_per_time[m_time][edge_site_one] = new_flow_one;
                total_stream_per_edge_site[edge_site_one] -= flow;
                cost_per_edge_site[edge_site_one] = cost_one;
                swap(stream, ans[m_time][edge_site_one].back());
                ans[m_time][edge_site_one].pop_back();

                now_cost += df;
            }
#ifdef _DEBUG
            debug << "T: " << T << " cost: " << now_cost << endl;
#endif

            T = T * dT; //降温
        }

        if (now_cost < best_cost) {
            best_cost = now_cost;
            // 将答案整合进distribution中
            //[mtime][customer][...] = <edge_site, stream_type>
            Distribution distribution(data.get_mtime_num(), vector<vector<pair<int, int>>>(data.get_customer_num()));
            for (size_t m_time = 0; m_time < data.get_mtime_num(); ++m_time) {
                for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site) {
                    for (const auto &stream : ans[m_time][edge_site]) {
                        int customer_site = stream.second.second;
                        int stream_type = stream.second.first;
                        distribution[m_time][customer_site].push_back({edge_site, stream_type});
                    }
                }
            }
            best_distribution = distribution;
            cout << best_cost << endl;
        }
    }

    inline int get_k94_order() {
        return max((data.get_mtime_num() * 19 - 1) / 20 + 1 - 1, 1ul);
    }

    inline int get_k95_order() {
        return (data.get_mtime_num() * 19 - 1) / 20 + 1;
    }

    inline int get_k96_order() {
        return min((data.get_mtime_num() * 19 - 1) / 20 + 1 + 1, data.get_mtime_num());
    }

    void simple_ffd() {
        /* 获得随机种子*/
        srand((unsigned int)time(NULL));

        /*预分配空间*/
        stream_per_time.assign(data.get_mtime_num(), vector<pair<int, pair<int, int>>>());

        best_distribution.assign(data.get_mtime_num(), vector<vector<pair<int, int>>>(data.get_customer_num()));

        edge_site_flow_94.assign(data.get_edge_num(), 0);
        edge_site_flow_95.assign(data.get_edge_num(), 0);
        edge_site_flow_96.assign(data.get_edge_num(), 0);

        for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site) {
            tree.assign(data.get_edge_num(), WeightSegmentTree(data.site_bandwidth[edge_site]));
            flow_to_m_time_per_eige_site.assign(data.get_edge_num(),
                                                vector<set<int>>(data.site_bandwidth[edge_site] + 1));
        }

        edge_site_total_stream_per_time.assign(data.get_mtime_num(), vector<int>(data.get_edge_num()));

        cost_per_edge_site.assign(data.get_edge_num(), 0);

        ans.assign(data.get_mtime_num(), vector<vector<pair<int, pair<int, int>>>>(data.get_edge_num()));

        total_stream_per_edge_site.assign(data.get_edge_num(), 0);

        index_94 = get_k94_order();
        index_95 = get_k95_order();
        index_96 = get_k96_order();

        /*更新stream_per_time*/
        for (size_t m_time = 0; m_time < data.get_mtime_num(); ++m_time) {
            const auto &ori_demand_now_time = data.demand[m_time];
            auto &stream_now_time = stream_per_time[m_time];
            stream_now_time.reserve(100 * data.get_customer_num());
            for (size_t customer_site = 0; customer_site < data.get_customer_num(); ++customer_site) {
                for (size_t stream_index = 0; stream_index < ori_demand_now_time[customer_site].size();
                     ++stream_index) {
                    const auto &stream = ori_demand_now_time[customer_site][stream_index];
                    if (stream.first != 0) {
                        stream_per_time[m_time].push_back({stream.first, {stream.second, customer_site}});
                    }
                }
            }
            stream_now_time.shrink_to_fit();
            sort(stream_now_time.begin(), stream_now_time.end(), greater<pair<int, pair<int, int>>>());
            // 预分配ans 空间
            // for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site) {
            //     ans[m_time][edge_site].reserve(stream_now_time.size() / 4);
            // }
        }

        /*使用ffd算法获得一组解*/
        vector<int> edge_order(data.get_edge_num());
        for (size_t m_time = 0; m_time < data.get_mtime_num(); ++m_time) {
            // 决定每轮遍历的顺序
            edge_order.resize(0);
            for (size_t edge_site = m_time % data.get_edge_num(); edge_site < data.get_edge_num(); ++edge_site) {
                edge_order.push_back(edge_site);
            }
            for (size_t edge_site = 0; edge_site < m_time % data.get_edge_num(); ++edge_site) {
                edge_order.push_back(edge_site);
            }
            for (const auto &stream : stream_per_time[m_time]) {
                const int flow = stream.first;
                const int stream_type = stream.second.first;
                const int customer_site = stream.second.second;
                for (auto edge_site : edge_order) {
                    if (edge_site_total_stream_per_time[m_time][edge_site] + flow <= data.site_bandwidth[edge_site]) {
                        if (data.qos[customer_site][edge_site] < data.qos_constraint) { // qos 限制
                            edge_site_total_stream_per_time[m_time][edge_site] += flow;
                            total_stream_per_edge_site[edge_site] += flow;
                            ans[m_time][edge_site].push_back({flow, {stream_type, customer_site}});
                            best_distribution[m_time][customer_site].push_back({edge_site, stream_type});
                            break;
                        }
                    }
                }
            }
        }

        for (size_t m_time = 0; m_time < data.get_mtime_num(); ++m_time) {
            for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site) {
                flow_to_m_time_per_eige_site[edge_site][edge_site_total_stream_per_time[m_time][edge_site]].insert(
                    m_time);
                tree[edge_site].update(0, 0, data.site_bandwidth[edge_site],
                                       edge_site_total_stream_per_time[m_time][edge_site], 1);
            }
        }

        double total_cost = 0.0;
        for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site) {
            if (data.get_mtime_num() > 1) {
                edge_site_flow_94[edge_site] = tree[edge_site].queryK(0, 0, data.site_bandwidth[edge_site], index_94);
            }
            if (data.get_mtime_num() >= 20) {
                edge_site_flow_96[edge_site] = tree[edge_site].queryK(0, 0, data.site_bandwidth[edge_site], index_96);
            }

            int flow_95 = tree[edge_site].queryK(0, 0, data.site_bandwidth[edge_site], index_95);
            edge_site_flow_95[edge_site] = flow_95;

            double cost = 0.0;
            if (total_stream_per_edge_site[edge_site] == 0) {
                cost = 0.0;
            } else {
                if (flow_95 <= data.base_cost) {
                    cost = data.base_cost;
                } else {
                    cost =
                        1.0 * (flow_95 - data.base_cost) * (flow_95 - data.base_cost) / data.site_bandwidth[edge_site] +
                        flow_95;
                }
            }
            cost_per_edge_site[edge_site] = cost;
            total_cost += cost;
        }
        best_cost = total_cost;

        cout << "init over" << endl;
    }

    Distribution excute() {
        simple_ffd();
        // SA(1e6, 0.99999, 1e-3);
        SA(1e12, 0.9999994, 1e2);
        SA(1e5, 0.99999, 1e-14);
        return best_distribution;
    }
    FFD(Data data) : data(data) {
    }
};
#endif
