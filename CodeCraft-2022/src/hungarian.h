/*
 * @Author: LinXuan
 * @Date: 2022-04-01 14:06:08
 * @Description:
 * @LastEditors: LinXuan
 * @LastEditTime: 2022-04-03 16:02:39
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
    Distribution distribution;    // 最终分配结果

    /*--------------------工具函数----------------------------------------*/
    inline bool is_connect(size_t customer_site, size_t edge_site) const
    {
        return data.qos[customer_site][edge_site] < data.qos_constraint;
    }
    /*-----------------------do something per mtime-----------------------*/
    /* 在m_time上执行匹配 */
    bool excute_match_per_mtime(vector<Stream> &stream_node, vector<vector<int>> &matched_per_edge,
        vector<int> &residual_capacity, const vector<int> &edge_order)
    {
        // 初始化
        vector<bool> vis(data.get_edge_num(), false);

        /*写在内部的递归搜索函数，省略传参*/
        function<bool(int, int)> find_path = [&](int u, int depth) -> bool {
            // 遍历所有的edge_site界定啊
            Stream stream = stream_node[u];
            // 有剩余容量: 直接加入匹配，返回
            for (size_t edge_site : edge_order)
            {
                if (this->is_connect(stream.customer_site, edge_site) and vis[edge_site] == false)
                {
                    if (residual_capacity[edge_site] >= stream.flow)
                    {
                        matched_per_edge[edge_site].push_back(u);
                        residual_capacity[edge_site] -= stream.flow;
                        return true;
                    }
                }
            }

            if (depth <= 0) return false;
            // 没有剩余容量: 以匹配的流量中，能腾出空间的，重新匹配
            // 再此过程中，需要标记不要重复访问边缘节点
            for (size_t edge_site : edge_order)
            {
                if (this->is_connect(stream.customer_site, edge_site) and vis[edge_site] == false)
                {
                    vis[edge_site] = true;
                    auto &matcharray = matched_per_edge[edge_site];
                    // 到着遍历
                    for (auto ite = matcharray.begin(); ite != matcharray.end(); ite++)
                    {
                        // 尝试回退一个可以腾出足够空间其他节点
                        if (stream_node[*ite].flow + residual_capacity[edge_site] >= stream.flow)
                        {
                            if (find_path(*ite, depth - 1) == true)
                            {
                                // 回退成功
                                residual_capacity[edge_site] += stream_node[*ite].flow;
                                residual_capacity[edge_site] -= stream.flow;
                                matcharray.erase(ite);
                                matcharray.push_back(u);
                                vis[edge_site] = false;    // 回退标记数组
                                return true;
                            }
                        }
                    }
                    vis[edge_site] = false;    // 回退标记
                }
            }

            return false;
        };

        // 执行增广路过程
        bool flag = true;
        for (size_t i = 0; i < stream_node.size(); i++)
        {
            if (stream_node[i].used == false and find_path(i, 0))
            {
                stream_node[i].used = true;
                continue;
            }
            else
            {
                flag = true;
                break;
            }
        }

        return flag;
    }

    void update_distribution_per_mtime(
        size_t mtime, const vector<vector<int>> &matched_per_edge, const vector<Stream> &stream_node)
    {
        // 写入distribution
        this->distribution[mtime].assign(data.get_customer_num(), vector<pair<int, int>>());
        Distribution_t &distribution_t = this->distribution[mtime];
        for (size_t edge_site = 0; edge_site < data.get_edge_num(); edge_site++)
        {
            for (int i : matched_per_edge[edge_site])
            {
                Stream stream = stream_node[i];
                distribution_t[stream.customer_site].push_back({edge_site, stream.stream_type});
            }
        }
    }

public:
    Hungarian(const Data &_data) : data(_data) { this->distribution.assign(data.get_mtime_num(), Distribution_t()); }
    Distribution excute()
    {
        vector<Stream> stream_node;              // 客户流量 3500
        vector<vector<int>> matched_per_edge;    // edge节点的匹配数组
        vector<int> residual_capacity;           // 边缘节点剩余容量 135
        vector<int> edge_order;                  // 边缘节点的遍历顺序

        for (size_t i = 0; i < data.get_edge_num(); i++)
            edge_order.push_back(i);

        for (size_t mtime = 0; mtime < data.get_mtime_num(); mtime++)
        {
            // 摊平图,并排序
            vector<Stream>().swap(stream_node);    // 清空
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

            matched_per_edge.assign(data.get_edge_num(), vector<int>());
            residual_capacity = data.site_bandwidth;

            this->excute_match_per_mtime(stream_node, matched_per_edge, residual_capacity, edge_order);

            this->update_distribution_per_mtime(mtime, matched_per_edge, stream_node);
            debug << "excute match in mtime :" << mtime << endl;
        }

        return this->distribution;
    }
};
#endif
