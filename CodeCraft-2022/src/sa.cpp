/*
 * @Author: LinXuan
 * @Date: 2022-04-06 19:51:47
 * @Description:
 * @LastEditors: LinXuan
 * @LastEditTime: 2022-04-07 11:06:00
 * @FilePath: /FDO/CodeCraft-2022/src/sa.cpp
 */
#ifndef _SA_
#define _SA_
#include "config.h"
#include "data.h"
#include "weight_segment_tree.h"
#include <bits/stdc++.h>
using namespace std;
class SOLVE_SA
{
private:
    struct Stream {
        /* 三元组 */
        int customer_site;
        int flow;
        int stream_type;
        Stream(int customer, int flow, int type) : customer_site(customer), flow(flow), stream_type(type) { }
        Stream() { }
        bool operator<(const Stream rhs) const { return this->flow < rhs.flow; }
        bool operator>(const Stream rhs) const { return this->flow > rhs.flow; }
    };

    const Data data;
    //[mtime][customer][...] = <edge_site, stream_type>
    Distribution best_distribution;
    double best_cost;    // 最好情况的答案

    //[m_time][...] <<stream>, <stream_type, customer_site>>
    vector<vector<Stream>> stream_per_mtime;    // 摊平的stream

    // [custome][...] = edge_site
    vector<vector<int>> customer_valid_edge_site;    // 提前预处理，每个customer的可用边缘点

    // 权值线段树快速计算cost值
    vector<WeightSegmentTree> tree;

    // 手动维护demand_sequence的第95%个及其前后值等特殊位置的数值来避免访问线段树，加快访问
    struct SiteFlow {
        int _94 = 0;
        int _95 = 0;
        int _96 = 0;
    };
    vector<SiteFlow> specific_flow;
    // 94、95、96个点的下标
    SiteFlow specific_index;


    /* 在模拟退火中快速选择下一个领域解所需要维护的变量 */
    // [edge_site][flow][...] = m_time
    vector<vector<set<int>>> flow_to_m_time_per_eige_site;
    //[m_time][edge_site] = total_flow_per_edge_site_per_time
    vector<vector<int>> edge_site_total_stream_per_time;
    // [edge_site] = total_stream_per_edge_site
    vector<long long> total_stream_per_edge_site;
    // [edge_site] = edge_site_cost
    vector<double> cost_per_edge_site;

    /* ans */
    /* 三个量的index互相同步 */
    // [m_time][edge_site][...(index)]  = stream
    vector<vector<vector<Stream>>> ans;    // 最终的分配答案

    /* 一些工具函数 */
    inline bool is_connect(int customer, int edge) const { return data.qos[customer][edge] < data.qos_constraint; }

public:
    SOLVE_SA(Data data) : data(data)
    {
        /* 初始化随机种子 */
        srand(time(NULL));

        /* 预分配成员变量的空间，并完成一定的初始化 */
        // 结果的初始化
        this->best_distribution.assign(data.get_mtime_num(), Distribution_t(data.get_edge_num()));
        this->best_cost = 1e10;

        // 初始化stream_per_time
        this->stream_per_mtime.assign(data.get_mtime_num(), vector<Stream>());
        for (size_t m_time = 0; m_time < data.get_mtime_num(); ++m_time)
        {
            const auto &ori_demand_now_time = data.demand[m_time];
            auto &stream_now_time = this->stream_per_mtime[m_time];
            stream_now_time.reserve(100 * data.get_customer_num());
            for (size_t customer_site = 0; customer_site < data.get_customer_num(); ++customer_site)
            {
                for (size_t stream_index = 0; stream_index < ori_demand_now_time[customer_site].size(); ++stream_index)
                {
                    const auto &stream = ori_demand_now_time[customer_site][stream_index];
                    if (stream.first > 0)
                    {
                        this->stream_per_mtime[m_time].push_back(
                            {static_cast<int>(customer_site), stream.first, stream.second});
                    }
                }
            }
            stream_now_time.shrink_to_fit();
            sort(stream_now_time.begin(), stream_now_time.end(), greater<Stream>());
        }

        // 初始化customer_valid_edge_site
        this->customer_valid_edge_site.assign(data.get_customer_num(), vector<int>());
        for (size_t customer_site = 0; customer_site < data.get_customer_num(); ++customer_site)
        {
            for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site)
            {
                if (is_connect(customer_site, edge_site))
                    this->customer_valid_edge_site[customer_site].push_back(edge_site);
            }
        }

        // 初始化求cost的线段树 以及 4个加速优化量
        for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site)
        {
            this->tree.assign(data.get_edge_num(), WeightSegmentTree(data.site_bandwidth[edge_site]));
            this->flow_to_m_time_per_eige_site.assign(
                data.get_edge_num(), vector<set<int>>(data.site_bandwidth[edge_site] + 1));
        }
        this->edge_site_total_stream_per_time.assign(data.get_mtime_num(), vector<int>(data.get_edge_num()));
        this->cost_per_edge_site.assign(data.get_edge_num(), 0);
        this->total_stream_per_edge_site.assign(data.get_edge_num(), 0);

        // 初始化线段树三个特殊位置的变量和下标
        this->specific_flow.assign(data.get_edge_num(), SiteFlow());
        this->specific_index._94 = max((data.get_mtime_num() * 19 - 1) / 20 + 1 - 1, 1ul);
        this->specific_index._95 = (data.get_mtime_num() * 19 - 1) / 20 + 1;
        this->specific_index._96 = min((data.get_mtime_num() * 19 - 1) / 20 + 1 + 1, data.get_mtime_num());

        // 初始化ans
        this->ans.assign(data.get_mtime_num(), vector<vector<Stream>>(data.get_edge_num()));
    };

    /* 执行一遍简单的FFD来获取一个初始状态 */
    void simple_ffd()
    {
        // 使用轮询算法尽量平均地初始化
        deque<int> edge_order;
        for (size_t i = 0; i < data.get_edge_num(); ++i)
            edge_order.push_back(i);

        for (size_t mtime = 0; mtime < data.get_mtime_num(); ++mtime)
        {
            for (const auto &stream : this->stream_per_mtime[mtime])
            {
                for (auto edge_site : edge_order)
                {
                    if (this->is_connect(stream.customer_site, edge_site)
                        and edge_site_total_stream_per_time[mtime][edge_site] + stream.flow
                            <= data.site_bandwidth[edge_site])
                    {
                        edge_site_total_stream_per_time[mtime][edge_site] += stream.flow;
                        total_stream_per_edge_site[edge_site] += stream.flow;
                        ans[mtime][edge_site].push_back(stream);
                        best_distribution[mtime][stream.customer_site].emplace_back(edge_site, stream.stream_type);
                        break;
                    }
                }
            }
            // 进行轮询
            edge_order.push_back(edge_order.front());
            edge_order.pop_front();
        }

        /* 维护大量用户优化速度的成员变量 */
        // 线段树与ans同步
        for (size_t mtime = 0; mtime < data.get_mtime_num(); ++mtime)
        {
            for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site)
            {
                this->flow_to_m_time_per_eige_site[edge_site][edge_site_total_stream_per_time[mtime][edge_site]].insert(
                    mtime);
                this->tree[edge_site].update(this->edge_site_total_stream_per_time[mtime][edge_site], 1);
            }
        }

        // cost等值同步
        double total_cost = 0.0;
        for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site)
        {
            // mtime > 1 才需要计算94
            if (data.get_mtime_num() > 1)
                this->specific_flow[edge_site]._94 = tree[edge_site].queryK(this->specific_index._94);
            // mtime > 20 才需要计算96
            if (data.get_mtime_num() > 20)
                this->specific_flow[edge_site]._96 = tree[edge_site].queryK(this->specific_index._96);
            // 计算95 cost
            int flow_95 = tree[edge_site].queryK(this->specific_index._95);
            this->specific_flow[edge_site]._95 = flow_95;

            // 计算单个节点的cost, 累加到total_cost中
            double cost = 0.0;
            if (this->total_stream_per_edge_site[edge_site] == 0)
                cost = 0.0;
            else
            {
                if (flow_95 <= data.base_cost)
                    cost = data.base_cost;
                else
                    cost
                        = 1.0 * (flow_95 - data.base_cost) * (flow_95 - data.base_cost) / data.site_bandwidth[edge_site]
                        + flow_95;
            }
            this->cost_per_edge_site[edge_site] = cost;
            total_cost += cost;
            // debug << "edge_site: " << edge_site << "flow" << flow_95 << " cost: " << cost << endl;
        }
        this->best_cost = total_cost;

        cout << "init cost: " << total_cost << endl;
        cout << "init over" << endl;
        return;
    }
}

;
#endif    // !_SA_
