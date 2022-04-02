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
#include <bits/stdc++.h>

using namespace std;
class FFD {
    const Data data;
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
    void optimize() {
        // [m_time][edge_site][...] <steam, <stream_type, customer_site>>
        vector<vector<vector<pair<int, pair<int, int>>>>> ans(
            data.demand.size(), vector<vector<pair<int, pair<int, int>>>>(data.site_bandwidth.size()));

        //[m_time][...] <<stream>, <stream_type, customer_site>>
        vector<vector<pair<int, pair<int, int>>>> stream_per_time(data.demand.size());
        vector<vector<bool>> vis_stream_per_time(data.get_mtime_num());
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
            vis_stream_per_time[m_time] = vector<bool>(demand_now_time.size(), false);
        }

        vector<bool> vis_edge_site(data.get_edge_num(), false);
        for (size_t m_time = 0; m_time < data.get_mtime_num(); ++m_time) {
            for (size_t customer_site = 0; customer_site < data.get_customer_num(); ++customer_site) {
                for (const auto &stream : best_distribution[m_time][customer_site]) {
                    int edge_site = stream.first;
                    vis_edge_site[edge_site] = true;
                }
            }
        }
        vector<int> valid_edge_site;
        valid_edge_site.reserve(data.get_edge_num());
        for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site) {
            if (vis_edge_site[edge_site] == true) {
                valid_edge_site.push_back(edge_site);
            }
        }
        valid_edge_site.shrink_to_fit();

        vector<pair<int, int>> total_stream_per_time(data.get_mtime_num());
        long long total_stream = 0;
        for (size_t m_time = 0; m_time < data.get_mtime_num(); ++m_time) {
            total_stream_per_time[m_time].second = m_time;
            for (size_t customer_site = 0; customer_site < data.get_customer_num(); ++customer_site) {
                for (const auto &stream : data.demand[m_time][customer_site]) {
                    total_stream_per_time[m_time].first += stream.first;
                    total_stream += stream.first;
                }
            }
        }

        int max_exploit_count_per_edge_site = data.get_mtime_num() - ((data.get_mtime_num() * 19 - 1) / 20 + 1);
        long long total = max_exploit_count_per_edge_site * valid_edge_site.size();
        vector<int> exploit_num_per_time(data.get_mtime_num());
        sort(total_stream_per_time.begin(), total_stream_per_time.end());
        for (size_t i = 0; i < total_stream_per_time.size(); ++i) {
            int m_time = total_stream_per_time[i].second;
            int stream = total_stream_per_time[i].first;
            exploit_num_per_time[m_time] = (total * stream / total_stream);
            total -= exploit_num_per_time[m_time];
            total_stream -= stream;
        }
        vector<vector<int>> edge_site_cap_per_time(data.get_mtime_num());
        for (size_t m_time = 0; m_time < data.get_mtime_num(); ++m_time) {
            edge_site_cap_per_time[m_time] = data.site_bandwidth;
        }
        vector<int> exploit_count(data.get_edge_num(), 0);
        vector<set<int>> exploit_edge_site_per_time(data.get_mtime_num());
        // 一个一个取exploit点
        for (size_t m_time = 0; m_time < data.get_mtime_num(); ++m_time) {
            for (int cnt = 0; cnt < exploit_count[m_time]; ++cnt) {
                // 计算此时刻 每个edge_site 连接的需求总量
                vector<pair<int, int>> total_stream_per_edge_site(data.get_edge_num(), {0, 0});
                for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site) {
                    total_stream_per_edge_site[edge_site].second = edge_site;
                }

                for (size_t stream_index = 0; stream_index < stream_per_time[m_time].size(); ++stream_index) {
                    if (vis_stream_per_time[m_time][stream_index] == true) {
                        continue;
                    }
                    const auto &stream = stream_per_time[m_time][stream_index];
                    int stream_flow = stream.first;
                    int customer_site = stream.second.second;
                    for (const auto &edge_site : valid_edge_site) {
                        if (data.qos[customer_site][edge_site] < data.qos_constraint) {
                            total_stream_per_edge_site[edge_site].first += stream_flow;
                        }
                    }
                }

                sort(total_stream_per_edge_site.begin(), total_stream_per_edge_site.end(), greater<pair<int, int>>());
                // 找到第一个使用次数没到限制,并且没有被本轮用过的点
                int exploit_edge_site = -1;
                for (const auto &tmp : total_stream_per_edge_site) {
                    int edge_site = tmp.second;
                    if (exploit_count[edge_site] < max_exploit_count_per_edge_site &&
                        exploit_edge_site_per_time[m_time].find(edge_site) ==
                            exploit_edge_site_per_time[m_time].end()) {
                        if (tmp.first == 0) {
                            break;
                        } else {
                            exploit_edge_site = edge_site;
                        }
                    }
                }
                if (exploit_edge_site == -1) {
                    break;
                }

                for (size_t stream_index = 0; stream_index < stream_per_time[m_time].size(); ++stream_index) {
                    if (vis_stream_per_time[m_time][stream_index] == true) {
                        continue;
                    }
                    const auto &stream = stream_per_time[m_time][stream_index];
                    int customer_site = stream.second.second;
                    int stream_flow = stream.first;
                    if (edge_site_cap_per_time[m_time][exploit_edge_site] >= stream_flow) {
                        if (data.qos[customer_site][exploit_edge_site] < data.qos_constraint) { // qos 限制
                            ans[m_time][exploit_edge_site].push_back(stream);
                            edge_site_cap_per_time[m_time][exploit_edge_site] -= stream_flow;
                            vis_stream_per_time[m_time][stream_index] = true;
                        }
                    }
                }
                ++exploit_count[exploit_edge_site];
                exploit_edge_site_per_time[m_time].insert(exploit_edge_site);
            }
        }

        for (size_t m_time = 0; m_time < data.get_edge_num(); ++m_time) {
            // 调整cap
            auto &edge_cap = edge_site_cap_per_time[m_time];
            for (const auto &edge_site : valid_edge_site) {
                if (exploit_edge_site_per_time[m_time].count(edge_site) == 0) {
                    edge_cap[edge_site] = min(data.base_cost, edge_cap[edge_site]);
                }
            }

            for (size_t stream_index = 0; stream_index < stream_per_time[m_time].size(); ++stream_index) {
                if (vis_stream_per_time[m_time][stream_index] == true) {
                    continue;
                }
                const auto &stream = stream_per_time[m_time][stream_index];
                int stream_flow = stream.first;
                int customer_site = stream.second.second;
                for (const auto &edge_site : valid_edge_site) {
                    if (edge_cap[edge_site] >= stream_flow) {
                        if (data.qos[customer_site][edge_site] < data.qos_constraint) { // qos 限制
                            ans[m_time][edge_site].push_back(stream);
                            edge_cap[edge_site] -= stream_flow;
                            vis_stream_per_time[m_time][stream_index] = true;
                            break;
                        }
                    }
                }
            }
        }

        bool is_success = true;
        for (size_t m_time = 0; m_time < data.get_edge_num() && is_success; ++m_time) {
            // 调整cap
            auto &edge_cap = edge_site_cap_per_time[m_time];
            for (const auto &edge_site : valid_edge_site) {
                if (exploit_edge_site_per_time[m_time].count(edge_site) == 0) {
                    if (data.base_cost < data.site_bandwidth[edge_site]) {
                        edge_cap[edge_site] = data.site_bandwidth[edge_site] - (data.base_cost - edge_cap[edge_site]);
                    }
                }
            }

            for (size_t stream_index = 0; stream_index < stream_per_time[m_time].size(); ++stream_index) {
                if (vis_stream_per_time[m_time][stream_index] == true) {
                    continue;
                }
                const auto &stream = stream_per_time[m_time][stream_index];
                int stream_flow = stream.first;
                int customer_site = stream.second.second;
                for (const auto &edge_site : valid_edge_site) {
                    if (edge_cap[edge_site] >= stream_flow) {
                        if (data.qos[customer_site][edge_site] < data.qos_constraint) { // qos 限制
                            ans[m_time][edge_site].push_back(stream);
                            edge_cap[edge_site] -= stream_flow;
                            vis_stream_per_time[m_time][stream_index] = true;
                            break;
                        }
                    }
                }
                if (vis_stream_per_time[m_time][stream_index] == false) {
                    is_success = false;
                    break;
                }
            }
        }

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
    }

    Distribution excute() {
        init();
        optimize();
        return best_distribution;
    }
    FFD(Data data) : data(data) {
    }
};

#endif
