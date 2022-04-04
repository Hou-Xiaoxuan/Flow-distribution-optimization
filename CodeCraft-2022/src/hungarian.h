/*
 * @Author: LinXuan
 * @Date: 2022-04-01 14:06:08
 * @Description:
 * @LastEditors: LinXuan
 * @LastEditTime: 2022-04-04 22:28:40
 * @FilePath: /FDO/CodeCraft-2022/src/hungarian.h
 */
#ifndef _HUNGARIAN_
 #define _HUNGARIAN
 #include "config.h"
 #include "data.h"
 #include <bits/stdc++.h>
using namespace std;
/*  边缘节点作为可以多次匹配的右节点
    细分的流量作为单词匹配的左节点 进行二分图匹配
*/
/*储存每一粉流量的信息*/
struct Stream {
    int customer_site;    // 客户节点
    int flow;             // 流量
    int stream_type;      // 类型
    int used = false;     // 是否使用过
    Stream(int cus, int flow, int type) : customer_site(cus), flow(flow), stream_type(type) { }
    bool operator<(const Stream rhs) const { return this->flow < rhs.flow; }
    bool operator>(const Stream rhs) const { return this->flow > rhs.flow; }
};
class Hungarian
{
private:
    /* data */
    Data data;
    Distribution best_distribution;    // 最终分配结果
    int best_cost;

    /*--------------------工具函数----------------------------------------*/
    inline bool is_connect(size_t customer_site, size_t edge_site) const
    {
        return data.qos[customer_site][edge_site] < data.qos_constraint;
    }
    /*-----------------------do something per mtime-----------------------*/
    /* 在m_time上执行匹配 */
    bool excute_match_per_mtime(vector<Stream> &stream_node, vector<vector<int>> &matched_per_edge,
        vector<int> &residual_capacity, const vector<int> &edge_order, int max_depth = 0);

    /*更新pertime的distribution*/
    void update_distribution_per_mtime(size_t mtime, Distribution &distribution,
        const vector<vector<int>> &matched_per_edge, const vector<Stream> &stream_node);

    /*比较、更新最优的distribution*/
    void update_best_distribution(const Distribution &distribution);

    /*-----------------------特殊函数-------------------------*/
    /* 执行一遍基本的FFD，获取用到的edge_site节点 */
    vector<int> get_edge_order_by_simple_hungarian();

    /*-------------------------在给定的edge_order上运行exploit-----------------------------*/
    void do_exploit_on_edge_order(vector<int> edge_order);

public:
    Hungarian(const Data &_data) : data(_data), best_cost(INT32_MAX) { }

    Distribution excute() { return this->best_distribution; }
};
#endif
