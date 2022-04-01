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
            demand_now_time.reserve(100 * ori_demand_now_time.size());
            // demand_now_time.push_back({-1, {-1, -1}}); // 偏移 1
            for (size_t customer_site = 0; customer_site < ori_demand_now_time.size(); ++customer_site) {
                for (size_t stream_index = 0; stream_index < ori_demand_now_time[customer_site].size();
                     ++stream_index) {
                    const auto &item = ori_demand_now_time[customer_site][stream_index];
                    demand[m_time].push_back({item.first, {item.second, customer_site}});
                }
            }
            demand_now_time.shrink_to_fit();
        }

        for (size_t m_time = 0; m_time < data.demand.size(); ++m_time) {
            sort(demand[m_time].begin(), demand[m_time].end(), greater<pair<int, pair<int, int>>>());

                vector<int>
                    edge_order; // 决定每轮遍历的顺序
            edge_order.reserve(data.edge_site.size());
            for (size_t edge_site = m_time % data.edge_site.size(); edge_site < data.edge_site.size(); ++edge_site) {
                edge_order.push_back(edge_site);
            }
            for (size_t edge_site = 0; edge_site < m_time % data.edge_site.size(); ++edge_site) {
                edge_order.push_back(edge_site);
            }
            vector<int> edge_cap = data.site_bandwidth;

            bool flag;
            for (size_t stream_index = 0; stream_index < demand[m_time].size(); ++stream_index) {
                flag = false;
                const auto &stream = demand[m_time][stream_index];
                for (const auto &edge_site : edge_order) {
                    if (edge_cap[edge_site] >= stream.first) {
                        if (data.qos[stream.second.second][edge_site] < data.qos_constraint) { // qos 限制
                            ans[m_time][edge_site].push_back(stream);
                            edge_cap[edge_site] -= stream.first;
                            flag = true;
                            break;
                        }
                    }
                }
                if (flag == false) { //找不到, 装不下
                    throw "error";
                }
            }
        }
        // [m_time][edge_site][...] <steam, <stream_type, customer_site>>
        // vector<vector<vector<pair<int, pair<int, int>>>>> ans;
        // 将答案整合进distribution中
        // [mtime][customer][...] = <edge_site, stream_type>
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
        return distribution;
    }
    FFD(Data data) : data(data) {
    }
};

#endif
