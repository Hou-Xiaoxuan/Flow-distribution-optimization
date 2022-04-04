/*
 * @Author: LinXuan
 * @Date: 2022-04-01 14:06:08
 * @Description:
 * @LastEditors: LinXuan
 * @LastEditTime: 2022-04-05 02:32:10
 * @FilePath: /FDO/CodeCraft-2022/src/hungarian.h
 */
#ifndef _HUNGARIAN_
 #define _HUNGARIAN_
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
    bool update_best_distribution(const Distribution &distribution);

    /*-----------------------特殊函数-------------------------*/
    /* 执行一遍基本的FFD，获取用到的edge_site节点 */
    vector<int> get_edge_order_by_simple_hungarian();

    /*-------------------------在给定的edge_order上运行exploit-----------------------------*/
    void do_exploit_on_edge_order(vector<int> edge_order);

public:
    Hungarian(const Data &_data) : data(_data), best_cost(INT32_MAX) { }

    Distribution excute()
    {
        /* 替代倍增的整体二分!
         * 执行一遍FFD&匈牙利的时间消耗大概为15/2s, 一边exploit大概为25/2秒
         * 因此，控制二分和exploit的次数都在10次以内。
         * 正常1e6的二分要执行20次，一方面优化执行速度，另一方面，仅在更新best_distribution后进行exploit。
         * * 如果超时，可以仅在二分结束后进行一次exploit
         */
        int max_bandwidth = *max_element(data.site_bandwidth.begin(), data.site_bandwidth.end());
        int min_limit = data.base_cost, max_limit = max_bandwidth;
        vector<vector<Stream>> stream_node_per_mtime(data.get_mtime_num());
        // 求出所有的stream_node
        for (size_t mtime = 0; mtime < data.get_mtime_num(); mtime++)
        {
            vector<Stream> &stream_node = stream_node_per_mtime[mtime];
            // 摊平图,并排序
            stream_node.reserve(data.get_customer_num() * 100);
            const auto &demant_t = data.demand[mtime];
            for (size_t i = 0; i < demant_t.size(); i++)
            {
                for (auto item : demant_t[i])
                {
                    if (item.first > 0)
                    {    // 筛掉flow为0的流量
                        stream_node.push_back({static_cast<int>(i), item.first, item.second});
                    }
                }
            }
            stream_node.shrink_to_fit();
            sort(stream_node.begin(), stream_node.end(), greater<Stream>());
        }
        while (min_limit <= max_limit)
        {
            int epoth_limit = (min_limit + max_limit) >> 1;
            debug << " -- epoth: " << epoth_limit << endl;
            vector<int> edge_order;     // 边缘节点的遍历顺序
            set<int> used_edge_site;    // 使用过的ede_site
            for (size_t i = 0; i < data.get_edge_num(); i++)
                edge_order.push_back(i);

            vector<vector<vector<int>>> edge_matched_per_mtime(
                data.get_mtime_num(), vector<vector<int>>(data.get_edge_num()));    // 每个时间edge节点的匹配数组

            vector<int> fixed_residual_capacity
                = data.site_bandwidth;    // 边缘节点剩余容量 135，每个mtime使用时复制一份
            for (auto &cap : fixed_residual_capacity)
                cap = min(cap, epoth_limit);
            bool flag = true;
            for (size_t mtime = 0; mtime < data.get_mtime_num(); mtime++)
            {
                vector<Stream> stream_node = stream_node_per_mtime[mtime];    // 客户流量，复制一份
                vector<int> residual_capacity = fixed_residual_capacity;      // 边缘节点剩余容量，复制一份
                vector<vector<int>> &edge_matched = edge_matched_per_mtime[mtime];    // edge节点的匹配数组，取引用

                flag = this->excute_match_per_mtime(stream_node, edge_matched, residual_capacity, edge_order);

                if (flag == false)
                    break;

                // 统计used_edge_site
                for (size_t edge_site : edge_order)
                {
                    if (residual_capacity[edge_site] != fixed_residual_capacity[edge_site])
                        used_edge_site.insert(edge_site);
                }
            }
            if (flag == true)
            {
                max_limit = epoth_limit - 1;
                // 进行更新
                Distribution distribution(data.get_mtime_num(), Distribution_t());
                for (size_t mtime = 0; mtime < data.get_mtime_num(); mtime++)
                {
                    //! 这里的stream_node严格来说与执行FFD后的是不一样。
                    //  但是由于更新distribution不需要状态量，所以传入一个纯净的stream_node就可以了
                    this->update_distribution_per_mtime(
                        mtime, distribution, edge_matched_per_mtime[mtime], stream_node_per_mtime[mtime]);
                }
                if (this->update_best_distribution(distribution))
                {
                    vector<int> exploit_edge_order;
                    exploit_edge_order.reserve(data.get_edge_num());
                    for (auto i : used_edge_site)
                        exploit_edge_order.push_back(i);
                    // 执行exploit
                    this->do_exploit_on_edge_order(edge_order);
                }
            } else
            {
                min_limit = epoth_limit + 1;
            }
        }

        // 返回最优结果
        return this->best_distribution;
    }
};
#endif
