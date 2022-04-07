/*
 * @Author: LinXuan
 * @Date: 2022-04-06 19:51:47
 * @Description:
 * @LastEditors: LinXuan
 * @LastEditTime: 2022-04-07 21:44:51
 * @FilePath: /FDO/CodeCraft-2022/src/sa.h
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

    // 最终的分配答案 [m_time][edge_site][...(index)]  = stream
    vector<vector<vector<Stream>>> ans;

    /* 一些工具函数 */
    // 判断是否联通
    inline bool is_connect(int customer, int edge) const { return data.qos[customer][edge] < data.qos_constraint; }
    // 从合法的95流量计算cost开销
    inline double cal_95flow_cost(int flow_95, int edge_site) const
    {
        double cost = 0.0;
        if (flow_95 <= data.base_cost)
            cost = data.base_cost;
        else
            cost = 1.0 * (flow_95 - data.base_cost) * (flow_95 - data.base_cost) / data.site_bandwidth[edge_site]
                + flow_95;
        return cost;
    }

    /* 执行一遍简单的FFD来获取一个初始状态 */
    void simple_ffd();


    /* 执行模拟退火的算法 */
    void excute_sa(double T, double dT, double end_T);

public:
    SOLVE_SA(Data data);

    Distribution excute();
}

;
#endif    // !_SA_
