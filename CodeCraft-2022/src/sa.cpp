/*
 * @Author: xv_rong
 */
#ifndef __SA__
#define __SA__
#include "config.h"
#include "data.h"
#include "weight_segment_tree.h"
#include <bits/stdc++.h>

using namespace std;
struct Stream {
    /* 三元组 */
    int customer_site;
    int flow;
    int stream_type;
    Stream(int customer, int flow, int type) : customer_site(customer), flow(flow), stream_type(type) {
    }
    Stream() {
    }
    bool operator<(const Stream rhs) const {
        return this->flow < rhs.flow;
    }
    bool operator>(const Stream rhs) const {
        return this->flow > rhs.flow;
    }
};

class SA {

    const Data data;

    // [m_time][...] <<stream>, <stream_type, customer_site>>
    vector<vector<Stream>> stream_per_time;

    //[mtime][customer][...] = <edge_site, stream_type>
    Distribution best_distribution;

    double best_cost = (double)LONG_LONG_MAX;

    double now_cost = (double)LONG_LONG_MAX;

    //[customer_site][...] = [qos_valid_edge_site]
    vector<vector<int>> customer_to_edge_qos_valid;

    //[edge_site] = flow_94
    vector<int> flow_94_per_edge_site;

    //[edge_site] = flow_95
    vector<int> flow_95_per_edge_site;

    //[edge_site] = flow_96
    vector<int> flow_96_per_edge_site;

    //[edge_site] = edge_site_tree
    vector<WeightSegmentTree> tree;

    // [edge_site][flow][...] = m_time
    vector<vector<set<int>>> flow_to_m_time_per_edge_site;

    //[m_time][edge_site] = total_flow_per_edge_site_per_time
    vector<vector<int>> edge_site_total_flow_per_time;

    // [edge_site] = total_flow_per_edge_site
    vector<long long> total_flow_per_edge_site;

    //[...] = edge_site_has_flow_in_95
    vector<int> edge_site_95_has_flow;

    // [m_time][edge_site][...] <stream, <stream_type, customer_site>>
    vector<vector<vector<Stream>>> ans;

    // [edge_site] = edge_site_cost
    vector<double> cost_per_edge_site;

    int index_94;
    int index_95;
    int index_96;

    /*根据当前T确定 最大值1000, 最小值1*/
    int get_swap_times(double T) {
        return (1 - exp(-T)) * (rand() % 3500) + 1;
    }

public:
    void SA_shuffle(double T, double dT, double end_T) {
        vector<int> valid_edge_site;
        valid_edge_site.reserve(data.get_edge_num());

        // 变化记录, [...] = <<m_time, edge_site>, Stream>>
        vector<pair<pair<int, int>, Stream>> stream_record;
        stream_record.reserve(3500);

        // 变化记录 [...] = <edge_site, <ori_flow_95, ori_cost>>
        vector<pair<int, pair<int, double>>> record_95;
        stream_record.reserve(3500);

        // 变化记录 edge_site_95_has_flow 中增加的edge_site的数量
        int added_num_edge_site_95_has_flow = 0;

        while (T > end_T) {
            if (edge_site_95_has_flow.size() == 0) {
                break;
            }

            int edge_site_one_index = rand() % edge_site_95_has_flow.size();
            int edge_site_one = edge_site_95_has_flow[edge_site_one_index];

            int swap_times = get_swap_times(T);

            double delta_cost = 0.0;
            record_95.emplace_back(edge_site_one,
                                   make_pair(flow_95_per_edge_site[edge_site_one], cost_per_edge_site[edge_site_one]));

            while (swap_times-- > 0 && flow_95_per_edge_site[edge_site_one] != 0) {
                /*选择edge_site_one 95值的时点*/
                int m_time = *flow_to_m_time_per_edge_site[edge_site_one][flow_95_per_edge_site[edge_site_one]].begin();

                /*随机选择stream*/
                auto &stream = ans[m_time][edge_site_one][rand() % ans[m_time][edge_site_one].size()];

                /*随机选择edge_site_two*/
                // 挑选出所有满足qos约束 能放入flow 且不是edge_site_one的edge_site
                valid_edge_site.resize(0);
                for (auto edge_site : customer_to_edge_qos_valid[stream.customer_site]) {
                    if (edge_site_total_flow_per_time[m_time][edge_site] + stream.flow <=
                            data.site_bandwidth[edge_site] &&
                        edge_site != edge_site_one) {
                        valid_edge_site.emplace_back(edge_site);
                    }
                }

                if (valid_edge_site.empty() == true) {
                    --swap_times;
                    continue;
                }

                int edge_site_two = valid_edge_site[rand() % valid_edge_site.size()];

                /* 放入edge_site_two 并计算花费变化*/
                int old_flow_two = edge_site_total_flow_per_time[m_time][edge_site_two];
                int new_flow_two = old_flow_two + stream.flow;

                tree[edge_site_two].update(0, 0, data.site_bandwidth[edge_site_two], old_flow_two, -1);
                tree[edge_site_two].update(0, 0, data.site_bandwidth[edge_site_two], new_flow_two, 1);

                int old_flow_95_two = flow_95_per_edge_site[edge_site_two];
                int new_flow_95_two = old_flow_95_two;
                double old_cost_two = cost_per_edge_site[edge_site_two];
                double new_cost_two = old_cost_two;

                if (old_flow_two <= old_flow_95_two && new_flow_two > old_flow_95_two) {
                    new_flow_95_two = tree[edge_site_two].queryK(0, 0, data.site_bandwidth[edge_site_two], index_95);

                    if (new_flow_95_two <= data.base_cost) {
                        new_cost_two = data.base_cost;
                    } else {
                        new_cost_two = 1.0 * (new_flow_95_two - data.base_cost) * (new_flow_95_two - data.base_cost) /
                                           data.site_bandwidth[edge_site_two] +
                                       new_flow_95_two;
                    }

                    flow_95_per_edge_site[edge_site_two] = new_flow_95_two;
                    cost_per_edge_site[edge_site_two] = new_cost_two;
                    record_95.emplace_back(edge_site_two, make_pair(old_flow_95_two, old_cost_two));

                    if (old_flow_95_two == 0 && new_flow_95_two > 0) {
                        edge_site_95_has_flow.emplace_back(edge_site_two);
                        ++added_num_edge_site_95_has_flow;
                    }

                    delta_cost += new_cost_two - old_cost_two;
                }

                flow_to_m_time_per_edge_site[edge_site_two][old_flow_two].erase(m_time);
                flow_to_m_time_per_edge_site[edge_site_two][new_flow_two].emplace(m_time);
                edge_site_total_flow_per_time[m_time][edge_site_two] = new_flow_two;
                total_flow_per_edge_site[edge_site_two] += stream.flow;
                ans[m_time][edge_site_two].emplace_back(stream);
                stream_record.emplace_back(make_pair(m_time, edge_site_two), stream);

                /* 拿出edge_site_one*/
                int old_flow_one = flow_95_per_edge_site[edge_site_one];
                int new_flow_one = old_flow_one - stream.flow;

                tree[edge_site_one].update(0, 0, data.site_bandwidth[edge_site_one], old_flow_one, -1);
                tree[edge_site_one].update(0, 0, data.site_bandwidth[edge_site_one], new_flow_one, 1);

                int new_flow_95_one = tree[edge_site_one].queryK(0, 0, data.site_bandwidth[edge_site_one], index_95);

                flow_95_per_edge_site[edge_site_one] = new_flow_95_one;
                flow_to_m_time_per_edge_site[edge_site_one][old_flow_one].erase(m_time);
                flow_to_m_time_per_edge_site[edge_site_one][new_flow_one].emplace(m_time);
                edge_site_total_flow_per_time[m_time][edge_site_one] = new_flow_one;
                total_flow_per_edge_site[edge_site_one] -= stream.flow;
                swap(stream, ans[m_time][edge_site_one].back());
                ans[m_time][edge_site_one].pop_back();
            }

            /* 计算edge_site_one花费变化*/
            double old_cost_one = cost_per_edge_site[edge_site_one];
            double new_cost_one = 0.0;
            int new_flow_95_one = flow_95_per_edge_site[edge_site_one];
            if (total_flow_per_edge_site[edge_site_one] != 0) {
                if (new_flow_95_one <= data.base_cost) {
                    new_cost_one = data.base_cost;
                } else {
                    new_cost_one = 1.0 * (new_flow_95_one - data.base_cost) * (new_flow_95_one - data.base_cost) /
                                       data.site_bandwidth[edge_site_one] +
                                   new_flow_95_one;
                }
            }

            delta_cost += new_cost_one - old_cost_one;

            /*考虑是否接受更新*/
            int is_acc = false;
            if (delta_cost < 0.0) {
                is_acc = true;
            } else if (exp(-delta_cost / T) * RAND_MAX > rand()) {
                is_acc = true;
            }

            if (is_acc) {
                now_cost += delta_cost;
                cost_per_edge_site[edge_site_one] = new_cost_one;
                if (new_flow_95_one == 0) {
                    swap(edge_site_95_has_flow[edge_site_one_index], edge_site_95_has_flow.back());
                    edge_site_95_has_flow.pop_back();
                }
            } else { // 回退
                for (auto it = record_95.rbegin(); it < record_95.rend(); ++it) {
                    int edge_site = it->first;
                    int ori_flow_95 = it->second.first;
                    double ori_cost = it->second.second;
                    flow_95_per_edge_site[edge_site] = ori_flow_95;
                    cost_per_edge_site[edge_site] = ori_cost;
                }
                for (auto it = stream_record.rbegin(); it < stream_record.rend(); ++it) {
                    int m_time = it->first.first;
                    int edge_site_two = it->first.second;
                    auto &stream = it->second;

                    int new_flow_two = edge_site_total_flow_per_time[m_time][edge_site_two];
                    int old_flow_two = new_flow_two - stream.flow;
                    ans[m_time][edge_site_two].pop_back();
                    tree[edge_site_two].update(0, 0, data.site_bandwidth[edge_site_two], old_flow_two, 1);
                    tree[edge_site_two].update(0, 0, data.site_bandwidth[edge_site_two], new_flow_two, -1);
                    flow_to_m_time_per_edge_site[edge_site_two][new_flow_two].erase(m_time);
                    flow_to_m_time_per_edge_site[edge_site_two][old_flow_two].emplace(m_time);
                    edge_site_total_flow_per_time[m_time][edge_site_two] -= stream.flow;
                    total_flow_per_edge_site[edge_site_two] -= stream.flow;

                    int new_flow_one = edge_site_total_flow_per_time[m_time][edge_site_one];
                    int old_flow_one = new_flow_one + stream.flow;
                    ans[m_time][edge_site_one].emplace_back(stream);
                    tree[edge_site_one].update(0, 0, data.site_bandwidth[edge_site_one], old_flow_one, 1);
                    tree[edge_site_one].update(0, 0, data.site_bandwidth[edge_site_one], new_flow_one, -1);
                    flow_to_m_time_per_edge_site[edge_site_one][new_flow_one].erase(m_time);
                    flow_to_m_time_per_edge_site[edge_site_one][old_flow_one].emplace(m_time);
                    edge_site_total_flow_per_time[m_time][edge_site_one] += stream.flow;
                    total_flow_per_edge_site[edge_site_one] += stream.flow;
                }

                while (added_num_edge_site_95_has_flow--) {
                    edge_site_95_has_flow.pop_back();
                }
            }
            // 清空回退量
            stream_record.resize(0);
            record_95.resize(0);
            added_num_edge_site_95_has_flow = 0;

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
                        distribution[m_time][stream.customer_site].emplace_back(edge_site, stream.stream_type);
                    }
                }
            }
            best_distribution = distribution;
            cout << best_cost << endl;
        }

        for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site) {
            flow_94_per_edge_site[edge_site] = tree[edge_site].queryK(0, 0, data.site_bandwidth[edge_site], index_94);
            flow_96_per_edge_site[edge_site] = tree[edge_site].queryK(0, 0, data.site_bandwidth[edge_site], index_96);
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
        stream_per_time.assign(data.get_mtime_num(), vector<Stream>());

        best_distribution.assign(data.get_mtime_num(), vector<vector<pair<int, int>>>(data.get_customer_num()));

        customer_to_edge_qos_valid.assign(data.get_customer_num(), vector<int>());

        flow_94_per_edge_site.assign(data.get_edge_num(), 0);
        flow_95_per_edge_site.assign(data.get_edge_num(), 0);
        flow_96_per_edge_site.assign(data.get_edge_num(), 0);

        for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site) {
            tree.assign(data.get_edge_num(), WeightSegmentTree(data.site_bandwidth[edge_site]));
            flow_to_m_time_per_edge_site.assign(data.get_edge_num(),
                                                vector<set<int>>(data.site_bandwidth[edge_site] + 1));
        }

        edge_site_total_flow_per_time.assign(data.get_mtime_num(), vector<int>(data.get_edge_num()));

        cost_per_edge_site.assign(data.get_edge_num(), 0);

        ans.assign(data.get_mtime_num(), vector<vector<Stream>>(data.get_edge_num()));

        edge_site_95_has_flow.reserve(data.get_edge_num());

        total_flow_per_edge_site.assign(data.get_edge_num(), 0);

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
                    const auto &ori_stream = ori_demand_now_time[customer_site][stream_index];
                    int stream_flow = ori_stream.first;
                    int stream_type = ori_stream.second;
                    if (stream_flow != 0) {
                        stream_per_time[m_time].emplace_back(customer_site, stream_flow, stream_type);
                    }
                }
            }
            stream_now_time.shrink_to_fit();
            sort(stream_now_time.begin(), stream_now_time.end(), greater<Stream>());
            // 预分配ans 空间 //会炸空间
            // for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site) {
            //     ans[m_time][edge_site].reserve(stream_now_time.size() / 4);
            // }
        }

        for (size_t customer_site = 0; customer_site < data.get_customer_num(); ++customer_site) {
            customer_to_edge_qos_valid[customer_site].reserve(data.get_edge_num());
            for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site) {
                if (data.qos[customer_site][edge_site] < data.qos_constraint) {
                    customer_to_edge_qos_valid[customer_site].emplace_back(edge_site);
                }
            }
            customer_to_edge_qos_valid.shrink_to_fit();
        }

        /*使用ffd算法获得一组解*/
        list<int> edge_order;
        for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site) {
            edge_order.emplace_back(edge_site);
        }
        for (size_t m_time = 0; m_time < data.get_mtime_num(); ++m_time) {
            for (const auto &stream : stream_per_time[m_time]) {
                for (auto edge_site : edge_order) {
                    if (edge_site_total_flow_per_time[m_time][edge_site] + stream.flow <=
                        data.site_bandwidth[edge_site]) {
                        if (data.qos[stream.customer_site][edge_site] < data.qos_constraint) { // qos 限制
                            edge_site_total_flow_per_time[m_time][edge_site] += stream.flow;
                            total_flow_per_edge_site[edge_site] += stream.flow;
                            ans[m_time][edge_site].emplace_back(stream);
                            best_distribution[m_time][stream.customer_site].emplace_back(edge_site, stream.stream_type);
                            break;
                        }
                    }
                }
            }
            // 改变edge_site 的顺序
            edge_order.push_back(edge_order.front());
            edge_order.pop_front();
        }

        for (size_t m_time = 0; m_time < data.get_mtime_num(); ++m_time) {
            for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site) {
                flow_to_m_time_per_edge_site[edge_site][edge_site_total_flow_per_time[m_time][edge_site]].emplace(
                    m_time);
                tree[edge_site].update(0, 0, data.site_bandwidth[edge_site],
                                       edge_site_total_flow_per_time[m_time][edge_site], 1);
            }
        }

        double total_cost = 0.0;
        for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site) {

            if (data.get_mtime_num() > 1) {
                flow_94_per_edge_site[edge_site] =
                    tree[edge_site].queryK(0, 0, data.site_bandwidth[edge_site], index_94);
            }
            if (data.get_mtime_num() >= 20) {
                flow_96_per_edge_site[edge_site] =
                    tree[edge_site].queryK(0, 0, data.site_bandwidth[edge_site], index_96);
            }
            int flow_95 = tree[edge_site].queryK(0, 0, data.site_bandwidth[edge_site], index_95);
            flow_95_per_edge_site[edge_site] = flow_95;

            if (flow_95 > 0) {
                edge_site_95_has_flow.push_back(edge_site);
            }

            double cost = 0.0;
            if (total_flow_per_edge_site[edge_site] == 0) {
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
        now_cost = total_cost;

        cout << "init over" << endl;
    }

    void SA_decent(double T, double dT, double end_T) {
        vector<int> valid_edge_site;
        valid_edge_site.reserve(data.get_edge_num());
        // 第一层 温度下降
        while (T > end_T) {
            if (edge_site_95_has_flow.size() == 0) {
                break;
            }
            int edge_site_one_index = rand() % edge_site_95_has_flow.size();
            int edge_site_one = edge_site_95_has_flow[edge_site_one_index];

            /*选择edge_site_one 95值的时点*/
            int m_time = *flow_to_m_time_per_edge_site[edge_site_one][flow_95_per_edge_site[edge_site_one]].begin();

            /*随机选择stream*/
            auto &stream = ans[m_time][edge_site_one][rand() % ans[m_time][edge_site_one].size()];

            /*随机选择edge_site_two*/
            // 挑选出所有满足qos约束 能放入flow 且不是edge_site_one的edge_site
            valid_edge_site.resize(0);
            for (auto edge_site : customer_to_edge_qos_valid[stream.customer_site]) {
                if (edge_site_total_flow_per_time[m_time][edge_site] + stream.flow <= data.site_bandwidth[edge_site] &&
                    edge_site != edge_site_one) {
                    valid_edge_site.emplace_back(edge_site);
                }
            }
            if (valid_edge_site.empty() == true) {
                T = T * dT;
                continue;
            }
            int edge_site_two = valid_edge_site[rand() % valid_edge_site.size()];

            /* 拿出后 edge_site_one 花费是多少*/
            int old_flow_one = edge_site_total_flow_per_time[m_time][edge_site_one];
            int new_flow_one = old_flow_one - stream.flow;

            /* 确定new_flow_95_one值是多少*/
            int new_flow_95_one = 0;
            if (data.get_mtime_num() == 1) { // 特殊情况只有一个时点
                new_flow_95_one = new_flow_one;
            } else {
                if (new_flow_one >= flow_94_per_edge_site[edge_site_one]) {
                    new_flow_95_one = new_flow_one;
                } else {
                    new_flow_95_one = flow_94_per_edge_site[edge_site_one];
                }
            }

            double cost_one = 0.0;
            if (total_flow_per_edge_site[edge_site_one] - stream.flow != 0) {
                if (new_flow_95_one <= data.base_cost) {
                    cost_one = data.base_cost;
                } else {
                    cost_one = 1.0 * (new_flow_95_one - data.base_cost) * (new_flow_95_one - data.base_cost) /
                                   data.site_bandwidth[edge_site_one] +
                               new_flow_95_one;
                }
            }

            /* 放入后 edge_site_two 花费是多少*/
            const int old_flow_two = edge_site_total_flow_per_time[m_time][edge_site_two];
            const int new_flow_two = old_flow_two + stream.flow;

            /* 确定new_flow_95_two值是多少*/
            int old_flow_95_two = flow_95_per_edge_site[edge_site_two];
            int new_flow_95_two = 0;

            if (data.get_mtime_num() < 20) { // 特殊情况时点小于20
                if (new_flow_two > flow_95_per_edge_site[edge_site_two]) {
                    new_flow_95_two = new_flow_two;
                } else {
                    new_flow_95_two = flow_95_per_edge_site[edge_site_two];
                }
            } else {
                if (old_flow_two > flow_95_per_edge_site[edge_site_two]) {
                    new_flow_95_two = flow_95_per_edge_site[edge_site_two];
                } else {
                    if (new_flow_two > flow_96_per_edge_site[edge_site_two]) {
                        new_flow_95_two = flow_96_per_edge_site[edge_site_two];
                    } else if (new_flow_two > flow_95_per_edge_site[edge_site_two]) {
                        new_flow_95_two = new_flow_two;
                    } else {
                        new_flow_95_two = flow_95_per_edge_site[edge_site_two];
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
                    if (old_flow_two == flow_95_per_edge_site[edge_site_two]) {
                        flow_95_per_edge_site[edge_site_two] = new_flow_two;
                    } else if (old_flow_two <= flow_94_per_edge_site[edge_site_two]) {
                        if (new_flow_two > flow_95_per_edge_site[edge_site_two]) {
                            if (data.get_mtime_num() > 1) {
                                flow_94_per_edge_site[edge_site_two] = flow_95_per_edge_site[edge_site_two];
                            }
                            flow_95_per_edge_site[edge_site_two] = new_flow_two;
                        } else if (new_flow_two > flow_94_per_edge_site[edge_site_two]) {
                            if (data.get_mtime_num() > 1) {
                                flow_94_per_edge_site[edge_site_two] = new_flow_two;
                            }
                        }
                    }
                } else {
                    if (old_flow_two >= flow_96_per_edge_site[edge_site_two]) {
                        if (old_flow_two == flow_96_per_edge_site[edge_site_two]) {
                            flow_96_per_edge_site[edge_site_two] =
                                tree[edge_site_two].queryK(0, 0, data.site_bandwidth[edge_site_two], index_96);
                        }
                    } else if (old_flow_two == flow_95_per_edge_site[edge_site_two]) {
                        if (new_flow_two > flow_96_per_edge_site[edge_site_two]) {
                            flow_95_per_edge_site[edge_site_two] = flow_96_per_edge_site[edge_site_two];
                            flow_96_per_edge_site[edge_site_two] =
                                tree[edge_site_two].queryK(0, 0, data.site_bandwidth[edge_site_two], index_96);
                        } else if (new_flow_two > flow_95_per_edge_site[edge_site_two]) {
                            flow_95_per_edge_site[edge_site_two] = new_flow_two;
                        }
                    } else if (old_flow_two <= flow_94_per_edge_site[edge_site_two]) {
                        if (new_flow_two > flow_96_per_edge_site[edge_site_two]) {
                            flow_94_per_edge_site[edge_site_two] = flow_95_per_edge_site[edge_site_two];
                            flow_95_per_edge_site[edge_site_two] = flow_96_per_edge_site[edge_site_two];
                            flow_96_per_edge_site[edge_site_two] =
                                tree[edge_site_two].queryK(0, 0, data.site_bandwidth[edge_site_two], index_96);
                        } else if (new_flow_two > flow_95_per_edge_site[edge_site_two]) {
                            flow_94_per_edge_site[edge_site_two] = flow_95_per_edge_site[edge_site_two];
                            flow_95_per_edge_site[edge_site_two] = new_flow_two;
                        } else if (new_flow_two > flow_94_per_edge_site[edge_site_two]) {
                            flow_94_per_edge_site[edge_site_two] = new_flow_two;
                        }
                    }
                }

                flow_to_m_time_per_edge_site[edge_site_two][old_flow_two].erase(m_time);
                flow_to_m_time_per_edge_site[edge_site_two][new_flow_two].emplace(m_time);
                flow_95_per_edge_site[edge_site_two] = new_flow_95_two;
                edge_site_total_flow_per_time[m_time][edge_site_two] = new_flow_two;
                total_flow_per_edge_site[edge_site_two] += stream.flow;
                cost_per_edge_site[edge_site_two] = cost_two;
                ans[m_time][edge_site_two].emplace_back(stream);
                // 95值的前后状况 判断edge_site_two应在edge_site_95_has_flow中加入
                if (old_flow_95_two == 0 && new_flow_95_two > 0) {
                    edge_site_95_has_flow.push_back(edge_site_two);
                }

                // 再删除
                tree[edge_site_one].update(0, 0, data.site_bandwidth[edge_site_one], old_flow_one, -1);
                tree[edge_site_one].update(0, 0, data.site_bandwidth[edge_site_one], new_flow_one, 1);

                // 维护94 95 96 的值
                if (data.get_mtime_num() == 1) { // 特殊情况只有一个时点
                    flow_95_per_edge_site[edge_site_one] = new_flow_one;
                } else {
                    if (new_flow_one >= flow_94_per_edge_site[edge_site_one]) {
                        flow_95_per_edge_site[edge_site_one] = new_flow_one;
                    } else {
                        flow_95_per_edge_site[edge_site_one] = flow_94_per_edge_site[edge_site_one];
                        flow_94_per_edge_site[edge_site_one] =
                            tree[edge_site_one].queryK(0, 0, data.site_bandwidth[edge_site_one], index_94);
                    }
                }
                flow_to_m_time_per_edge_site[edge_site_one][old_flow_one].erase(
                    flow_to_m_time_per_edge_site[edge_site_one][old_flow_one].begin());
                flow_to_m_time_per_edge_site[edge_site_one][new_flow_one].emplace(m_time);
                flow_95_per_edge_site[edge_site_one] = new_flow_95_one;
                edge_site_total_flow_per_time[m_time][edge_site_one] = new_flow_one;
                total_flow_per_edge_site[edge_site_one] -= stream.flow;
                cost_per_edge_site[edge_site_one] = cost_one;
                swap(stream, ans[m_time][edge_site_one].back());
                ans[m_time][edge_site_one].pop_back();
                // 95值的前后状况 判断edge_site_one应在edge_site_95_has_flow中删除
                if (old_flow_one > 0 && new_flow_95_one == 0) {
                    swap(edge_site_95_has_flow[edge_site_one_index], edge_site_95_has_flow.back());
                    edge_site_95_has_flow.pop_back();
                }

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
                        distribution[m_time][stream.customer_site].emplace_back(edge_site, stream.stream_type);
                    }
                }
            }
            best_distribution = distribution;
            cout << best_cost << endl;
        }
    }

    Distribution excute() {
        simple_ffd();
        SA_decent(1e15, 0.999999, 1e11);
        SA_decent(1e5, 0.999999, 1e-3);
        SA_shuffle(1e15, 0.99, 1e13);
        SA_decent(1e5, 0.999999, 1e-3);
        // SA_excute(1e3, 0.9999, 1e-30);
        return best_distribution;
    }
    SA(Data data) : data(data) {
    }
};
#endif
