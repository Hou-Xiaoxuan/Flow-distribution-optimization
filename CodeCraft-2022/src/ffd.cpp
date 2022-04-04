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
    //[mtime][customer][...] = <edge_site, stream_type>
    Distribution best_distribution;
    int best_cost = INT_MAX;

    vector<vector<pair<int, pair<int, int>>>> stream_per_time;
    vector<WeightSegmentTree> tree;
    vector<vector<int>> edge_site_total_stream_per_time;
    vector<double> pre_edge_site_cost;

public:
    void SA() {
        /*从best_distribution 中获取一个由edge_site为索引的ans*/
        // [m_time][edge_site][...] <stream, <stream_type, customer_site>>
        vector<vector<unordered_map<int, pair<int, pair<int, int>>>>> ans(
            data.get_mtime_num(), vector<unordered_map<int, pair<int, pair<int, int>>>>(data.get_edge_num()));
        for (size_t m_time = 0; m_time < data.get_mtime_num(); ++m_time) {
            for (size_t customer_site = 0; customer_site < data.get_customer_num(); ++customer_site) {
                for (const auto &stream : best_distribution[m_time][customer_site]) {
                    const int edge_site = stream.first;
                    const int stream_type = stream.second;
                    const int stream_flow = data.stream_type_to_flow[m_time][customer_site].at(stream_type);
                    ans[m_time][edge_site][ans[m_time][edge_site].size()] = {stream_flow, {stream_type, customer_site}};
                }
            }
        }

        double now_cost = accumulate(pre_edge_site_cost.begin(), pre_edge_site_cost.end(), 0.0);

        vector<vector<unordered_set<int>>> remaining_ans_index(data.get_mtime_num(),
                                                               vector<unordered_set<int>>(data.get_edge_num()));

        vector<vector<int>> max_ans_index(data.get_mtime_num(), vector<int>(data.get_edge_num()));

        for (size_t m_time = 0; m_time < data.get_mtime_num(); ++m_time) {
            for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site) {
                max_ans_index[m_time][edge_site] = ans[m_time][edge_site].size();
            }
        }

        double T = 2000;  //代表开始的温度
        double dT = 0.99; //代表系数delta T
        double eps = 1e-5;
        int index_95 = get_k95_order();
        while (T > eps) {
            // 注意%0错误

            /*随机选择m_time*/
            int m_time = rand() % data.get_mtime_num();

            /*随机选则edge_site_one*/
            int edge_site_one;
            do {
                edge_site_one = rand() % data.get_edge_num();
            } while (ans[m_time][edge_site_one].size() == 0); // 不能为空

            /*随机选择stream*/
            int stream_index;
            do {
                stream_index = rand() % max_ans_index[m_time][edge_site_one];
            } while (remaining_ans_index[m_time][edge_site_one].count(stream_index) !=
                     0); // 下标在remaining_ans_index中就不会在ans中

            const auto &stream = ans[m_time][edge_site_one][stream_index];
            const int flow = stream.first;
            const int customer_site = stream.second.second;

            /*随机选择edge_site_two*/

            // 挑选出所有满足qos约束 能放入flow 且不是edge_site_one的edge_site
            vector<int> valid_edge_site;
            valid_edge_site.reserve(data.get_edge_num());
            for (int edge_site = 0; edge_site < (int)data.get_edge_num(); ++edge_site) {
                if (data.qos[customer_site][edge_site] < data.qos_constraint &&
                    edge_site_total_stream_per_time[m_time][edge_site] + flow <= data.site_bandwidth[edge_site] &&
                    edge_site != edge_site_one) {
                    valid_edge_site.push_back(edge_site);
                }
            }
            if (valid_edge_site.empty() == true) {
                break;
            }
            int edge_site_two = valid_edge_site[rand() % valid_edge_site.size()];

            /* 拿出后当前edge_site_one 花费是多少*/
            const int old_flow_one = edge_site_total_stream_per_time[m_time][edge_site_one];
            const int new_flow_one = old_flow_one - flow;
            tree[edge_site_one].update(0, 0, data.site_bandwidth[edge_site_one], old_flow_one, -1);
            tree[edge_site_one].update(0, 0, data.site_bandwidth[edge_site_one], new_flow_one, 1);
            const int new_flow_95_one = tree[edge_site_one].queryK(0, 0, data.site_bandwidth[edge_site_one], index_95);
            double cost_one = 0.0;
            if (ans[m_time][edge_site_one].size() != 1) {
                if (new_flow_95_one <= data.base_cost) {
                    cost_one = data.base_cost;
                } else {
                    cost_one = 1.0 * (new_flow_95_one - data.base_cost) * (new_flow_95_one - data.base_cost) /
                                   data.site_bandwidth[edge_site_one] +
                               new_flow_95_one;
                }
            }

            /* 放入后后当前edge_site_two 花费是多少*/
            const int old_flow_two = edge_site_total_stream_per_time[m_time][edge_site_two];
            const int new_flow_two = old_flow_two + flow;
            tree[edge_site_two].update(0, 0, data.site_bandwidth[edge_site_two], old_flow_two, -1);
            tree[edge_site_two].update(0, 0, data.site_bandwidth[edge_site_two], new_flow_two, 1);
            const int new_flow_95_two = tree[edge_site_two].queryK(0, 0, data.site_bandwidth[edge_site_two], index_95);
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
                (cost_one + cost_two) - (pre_edge_site_cost[edge_site_one] + pre_edge_site_cost[edge_site_two]);
            if (df < 0.0) {
                is_acc = true;
            } else if (exp(-df / T) * RAND_MAX > rand()) {
                is_acc = true;
            }

            if (is_acc == true) { // 必须先删除 再插入
                edge_site_total_stream_per_time[m_time][edge_site_two] = new_flow_two;
                pre_edge_site_cost[edge_site_two] = cost_two;
                if (remaining_ans_index[m_time][edge_site_two].empty()) {
                    ans[m_time][edge_site_two][max_ans_index[m_time][edge_site_two]] = stream;
                    ++max_ans_index[m_time][edge_site_two];
                } else {
                    int new_stream_index = *remaining_ans_index[m_time][edge_site_two].begin();
                    ans[m_time][edge_site_two][new_stream_index] = stream;
                    remaining_ans_index[m_time][edge_site_two].erase(
                        remaining_ans_index[m_time][edge_site_two].begin());
                }

                edge_site_total_stream_per_time[m_time][edge_site_one] = new_flow_one;
                pre_edge_site_cost[edge_site_one] = cost_one;
                ans[m_time][edge_site_one].erase(stream_index);
                if (stream_index == max_ans_index[m_time][edge_site_one] - 1) {
                    --max_ans_index[m_time][edge_site_one];
                    remaining_ans_index[m_time][edge_site_one].erase(
                        max_ans_index[m_time][edge_site_one]); // 如果存在就应该删除
                } else {
                    remaining_ans_index[m_time][edge_site_one].insert(stream_index);
                }
                now_cost += df;

                // 将答案整合进distribution中
                //[mtime][customer][...] = <edge_site, stream_type>
                Distribution distribution(data.get_mtime_num(),
                                          vector<vector<pair<int, int>>>(data.get_customer_num()));
                for (size_t m_time = 0; m_time < data.get_mtime_num(); ++m_time) {
                    for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site) {
                        for (const auto &stream_with_key : ans[m_time][edge_site]) {
                            const auto &stream = stream_with_key.second;
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
                assert(cost == int(now_cost + 0.5));
                // assert(now_cost)
            } else {
                // 回退
                tree[edge_site_one].update(0, 0, data.site_bandwidth[edge_site_one], old_flow_one, 1);
                tree[edge_site_one].update(0, 0, data.site_bandwidth[edge_site_one], new_flow_one, -1);
                tree[edge_site_two].update(0, 0, data.site_bandwidth[edge_site_two], old_flow_two, 1);
                tree[edge_site_two].update(0, 0, data.site_bandwidth[edge_site_two], new_flow_two, -1);
            }
#ifdef _DEBUG
            debug << "T: " << T << " cost: " << now_cost << endl;
#endif

            T = T * dT; //降温
        }

        // 将答案整合进distribution中
        //[mtime][customer][...] = <edge_site, stream_type>
        Distribution distribution(data.get_mtime_num(), vector<vector<pair<int, int>>>(data.get_customer_num()));
        for (size_t m_time = 0; m_time < data.get_mtime_num(); ++m_time) {
            for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site) {
                for (const auto &stream_with_key : ans[m_time][edge_site]) {
                    const auto &stream = stream_with_key.second;
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
    void init() {
        // 将同一个时刻的流量重整为一个一维数组
        //[m_time][...] <<stream>, <stream_type, customer_site>>
        srand((unsigned int)time(NULL));
        stream_per_time.resize(data.demand.size());
        for (size_t m_time = 0; m_time < data.demand.size(); ++m_time) {
            const auto &ori_demand_now_time = data.demand[m_time];
            auto &demand_now_time = stream_per_time[m_time];
            demand_now_time.reserve(100 * ori_demand_now_time.size());
            // demand_now_time.push_back({-1, {-1, -1}}); // 偏移 1
            for (size_t customer_site = 0; customer_site < ori_demand_now_time.size(); ++customer_site) {
                for (size_t stream_index = 0; stream_index < ori_demand_now_time[customer_site].size();
                     ++stream_index) {
                    const auto &item = ori_demand_now_time[customer_site][stream_index];
                    if (item.first != 0) {
                        stream_per_time[m_time].push_back({item.first, {item.second, customer_site}});
                    }
                }
            }
            demand_now_time.shrink_to_fit();
            sort(demand_now_time.begin(), demand_now_time.end(), greater<pair<int, pair<int, int>>>());
        }
    }
    inline int get_k95_order() {
        return (data.get_mtime_num() * 19 - 1) / 20 + 1;
    }
    void greedy() {
        bool is_success = true;

        // [mtime][customer][...] = <edge_site, stream_type>
        Distribution distribution(data.demand.size(), vector<vector<pair<int, int>>>(data.customer_site.size()));

        tree.resize(data.get_edge_num());
        for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site) {
            tree[edge_site] = WeightSegmentTree(data.site_bandwidth[edge_site]);
            tree[edge_site].update(0, 0, data.site_bandwidth[edge_site], 0, data.get_mtime_num());
        }

        edge_site_total_stream_per_time.assign(data.get_mtime_num(), vector<int>(data.get_edge_num(), 0));

        // 从1开始计数的下标值
        int index_95 = get_k95_order();
        vector<int> edge_stream_95(data.get_edge_num(), 0);
        vector<int> edge_stream_96(data.get_edge_num(), 0);
        pre_edge_site_cost.assign(data.get_edge_num(), 0.0);

        // 获得一个随机的edge_site顺序
        vector<int> edge_order(data.get_edge_num());
        for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site) {
            edge_order[edge_site] = edge_site;
        }

        // unsigned seed = chrono::system_clock::now().time_since_epoch().count();
        // shuffle(edge_order.begin(), edge_order.end(), default_random_engine(seed));

        for (size_t m_time = 0; m_time < data.get_mtime_num() && is_success; ++m_time) {
            for (size_t stream_index = 0; stream_index < stream_per_time[m_time].size(); ++stream_index) {
                const auto &stream = stream_per_time[m_time][stream_index];
                const int stream_type = stream.second.first;
                const int flow = stream.first;
                const int customer_site = stream.second.second;
                vector<pair<double, int>> cost_per_edge_site;
                cost_per_edge_site.reserve(data.get_customer_num());
                for (const auto &edge_site : edge_order) {
                    /* 加不进去, 跳过*/
                    if (data.qos[customer_site][edge_site] >= data.qos_constraint ||
                        edge_site_total_stream_per_time[m_time][edge_site] + flow > data.site_bandwidth[edge_site]) {
                        continue;
                    }

                    /* 确定当前95值是多少*/
                    int old_flow = edge_site_total_stream_per_time[m_time][edge_site];
                    int new_flow = old_flow + flow;
                    int new_95_flow = edge_stream_95[edge_site];

                    if (data.get_mtime_num() < 20) {
                        if (new_flow > edge_stream_95[edge_site]) {
                            new_95_flow = new_flow;
                        }
                    } else {
                        if (new_flow > edge_stream_95[edge_site]) {
                            if (old_flow <= edge_stream_95[edge_site]) {
                                if (new_flow > edge_stream_95[edge_site] && new_flow <= edge_stream_96[edge_site]) {
                                    new_95_flow = new_flow;
                                } else {
                                    new_95_flow = edge_stream_96[edge_site];
                                }
                            }
                        }
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

                const auto &tmp = *min_element(cost_per_edge_site.begin(), cost_per_edge_site.end()); // 直接取最小

                double add_cost = tmp.first;
                int edge_site = tmp.second;

                int old_flow = edge_site_total_stream_per_time[m_time][edge_site];
                tree[edge_site].update(0, 0, data.site_bandwidth[edge_site], old_flow, -1);
                int new_flow = old_flow + flow;
                tree[edge_site].update(0, 0, data.site_bandwidth[edge_site], new_flow, 1);

                if (data.get_mtime_num() < 20) {
                    if (new_flow > edge_stream_95[edge_site]) {
                        edge_stream_95[edge_site] = new_flow;
                    }
                } else {
                    if (new_flow > edge_stream_95[edge_site]) {
                        if (old_flow <= edge_stream_95[edge_site]) {
                            if (new_flow > edge_stream_95[edge_site] && new_flow <= edge_stream_96[edge_site]) {
                                edge_stream_95[edge_site] = new_flow;
                            } else {
                                edge_stream_95[edge_site] = edge_stream_96[edge_site];
                            }
                        }
                    }
                }
                if (data.get_mtime_num() >= 20) { // 只在此情况下更新96值才有意义
                    edge_stream_96[edge_site] =
                        tree[edge_site].queryK(0, 0, data.site_bandwidth[edge_site], index_95 + 1);
                }
                edge_site_total_stream_per_time[m_time][edge_site] = new_flow;
                pre_edge_site_cost[edge_site] += add_cost;
                distribution[m_time][customer_site].push_back({edge_site, stream_type});
            }
        }

        if (is_success == true) {
            int cost = cal_cost(data, distribution);
            if (cost < best_cost) {
                best_cost = cost;
                best_distribution = distribution;
                cout << best_cost << endl;
            }
        }
    }

    Distribution excute() {
        init();
        greedy();
        SA();
        return best_distribution;
    }
    FFD(Data data) : data(data) {
    }
};

#endif
