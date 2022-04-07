/*
 * @Author: LinXuan
 * @Date: 2022-04-06 19:51:47
 * @Description:
 * @LastEditors: LinXuan
 * @LastEditTime: 2022-04-07 15:37:34
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
            // mtime >= 20 才需要计算96
            if (data.get_mtime_num() >= 20)
                this->specific_flow[edge_site]._96 = tree[edge_site].queryK(this->specific_index._96);
            // 计算95 cost
            int flow_95 = tree[edge_site].queryK(this->specific_index._95);
            this->specific_flow[edge_site]._95 = flow_95;

            // 计算单个节点的cost, 累加到total_cost中
            double cost = 0.0;
            if (this->total_stream_per_edge_site[edge_site] == 0)
                cost = 0.0;
            else
                cost = this->cal_95flow_cost(flow_95, edge_site);

            // 累计cost并维护
            this->cost_per_edge_site[edge_site] = cost;
            total_cost += cost;
        }
        this->best_cost = total_cost;

        cout << "init cost: " << total_cost << endl;
        cout << "init over" << endl;
        return;
    }


    /* 执行模拟退火的算法 */
    void excute_sa(double T, double dT, double end_T)
    {
        double now_cost = this->best_cost;
        while (T > end_T)
        {
            struct {
                int site;            // 边缘点
                int old_flow;        // 原来的流量
                int new_flow;        // 新的流量
                int new_flow_95;     // 新的95值
                double new_cost;     // 新的cost
            } site_from, site_to;    // 变化的两个点，流量从from移动到to

            // 随机选取合法的edge_site_from，最多执行edge_nume次。仍然找不到则降温
            site_from.site = rand() % data.get_edge_num();
            for (size_t i = 0; i < data.get_edge_num() and this->specific_flow[site_from.site]._95 == 0; ++i)
                site_from.site = rand() % data.get_edge_num();
            if (this->specific_flow[site_from.site]._95 == 0)
            {
                T = T * dT;
                continue;
            }

            // 选取edge_site_from的95值所在第一个时间点
            int mtime
                = *this->flow_to_m_time_per_eige_site[site_from.site][this->specific_flow[site_from.site]._95].begin();
            // 随机在该时刻选取一个stream Node: 非const引用，用于调用swap交换
            auto &stream = this->ans[mtime][site_from.site][rand() % ans[mtime][site_from.site].size()];

            // 随机选取一个合法的edge_site_to, 同时不为edge_site_from
            vector<int> valid_site;
            valid_site.reserve(this->customer_valid_edge_site[stream.customer_site].size());
            for (auto edge_site : this->customer_valid_edge_site[stream.customer_site])
            {
                if (this->edge_site_total_stream_per_time[mtime][edge_site] + stream.flow
                        <= this->data.site_bandwidth[edge_site]
                    and edge_site != site_from.site)
                {
                    valid_site.push_back(edge_site);
                }
            }
            if (valid_site.empty())
            {
                // 没有合法的edge_site_to
                T = T * dT;
                continue;
            }
            site_to.site = valid_site[rand() % valid_site.size()];    // 随机从合法范围选取


            /*计算拿出edge_sitge_from以后的95值和花费*/
            site_from.old_flow = this->edge_site_total_stream_per_time[mtime][site_from.site];
            site_from.new_flow = 0;
            // 确认edge_site_from新的95值，注意edge_site_from的流量是减少的
            if (data.get_customer_num() == 1)    // 只有一个时间点的特殊情况
                site_from.new_flow_95 = site_from.new_flow;
            else
            {
                if (site_from.new_flow >= this->specific_flow[site_from.site]._94)    // 新flow>=94值，则新95是新flow
                    site_from.new_flow_95 = site_from.new_flow;
                else    // 否则，新95是原94值
                    site_from.new_flow_95 = this->specific_flow[site_from.site]._94;
            }

            // 计算拿出edge_site_from以后的花费：拿完仍有流量使用公式计算，否则直为0
            if (this->total_stream_per_edge_site[site_from.site] - stream.flow > 0)
                site_from.new_cost = this->cal_95flow_cost(site_from.new_flow_95, site_from.site);
            else
                site_from.new_cost = 0.0;


            /*计算放入edge_site_to以后的95值和花费*/
            site_to.old_flow = this->edge_site_total_stream_per_time[mtime][site_to.site];
            site_to.new_flow = site_to.old_flow + stream.flow;
            // 确认edge_site_to新的95值。注意edge_site_to的流量是增加的
            if (data.get_mtime_num() < 20)    // mtime<20时，不需要考虑96值
            {
                /* 新流大于95值，则新95是新流
                 * 否则，95不变
                 */
                if (site_to.new_flow > this->specific_flow[site_to.site]._95)
                    site_to.new_flow_95 = site_to.new_flow;
                else
                    site_to.new_flow_95 = this->specific_flow[site_to.site]._95;
            }
            else    // 同时考虑94、95、96
            {
                // 旧流量>95，则新95不变
                if (site_to.old_flow > this->specific_flow[site_to.site]._95)
                    site_to.new_flow_95 = this->specific_flow[site_to.site]._95;
                else
                {
                    /* 旧流量<=95时：
                     * 新流大于96,则新95是原来的96
                     * 新流小于96大于95, 则新流就是新95
                     * 新流小于等于95,则95不变
                     */
                    if (site_to.new_flow > this->specific_flow[site_to.site]._96)
                        site_to.new_flow_95 = this->specific_flow[site_to.site]._96;
                    else if (site_to.new_flow > this->specific_flow[site_to.site]._95)
                        site_to.new_flow_95 = site_to.new_flow;
                    else
                        site_to.new_flow_95 = this->specific_flow[site_to.site]._95;
                }
            }
            // 计算放入edge_site_to以后的花费
            site_to.new_cost = this->cal_95flow_cost(site_to.new_flow_95, site_to.site);

            /* 开始考虑是否要进行更新 */
            bool is_accept = false;
            const double diff = (site_from.new_cost + site_to.new_cost)
                - (this->cost_per_edge_site[site_from.site] + this->cost_per_edge_site[site_to.site]);
            if (diff < 0.0)
                is_accept = true;
            else if (rand() / (double)RAND_MAX < exp(-diff / T))
                is_accept = true;


            /* 执行更新/回退: 由于维护的变量已经预更新了，需要重新同步线段树、维护量和ans*/
            if (is_accept == true)
            {
                /*先进行插入*/
                this->tree[site_to.site].update(site_to.old_flow, -1);
                this->tree[site_to.site].update(site_to.new_flow, 1);

                // 维护94、95、96的值
                if (data.get_mtime_num() < 20)
                {
                    /* 仅考虑94、95的值:
                     * 旧flow == _95;若旧flow<= _94; 两种情况
                     */
                    if (site_to.old_flow == this->specific_flow[site_to.site]._95)
                        this->specific_flow[site_to.site]._95 = site_to.new_flow;    // new_flow增加，必为95
                    else if (site_to.old_flow <= this->specific_flow[site_to.site]._94)
                    {
                        /* old_flow <= 原94:
                         * new_flow > 愿95, 则94、95右移
                         * new_flow <= 原95,且大于94，则95不变，94右移
                         * 剩余情况不用移动
                         */
                        if (site_to.new_flow > this->specific_flow[site_to.site]._95)
                        {
                            if (data.get_mtime_num() > 1ul)    // 有94
                                this->specific_flow[site_to.site]._94 = this->specific_flow[site_to.site]._95;
                            this->specific_flow[site_to.site]._95 = site_to.new_flow;
                        }
                        else if (site_to.new_flow > this->specific_flow[site_to.site]._94)
                        {
                            if (data.get_mtime_num() > 1ul)    // 有94
                                this->specific_flow[site_to.site]._94 = site_to.new_flow;
                        }
                    }
                }
                else
                {
                    /* 同时考虑94、95、96:
                     * 分为old_flow==96; old_flow==95; old_flow<=94三种情况
                     */
                    if (site_to.old_flow == this->specific_flow[site_to.site]._96)
                    {
                        // 旧流大于96, 新值增加人大于96,不影响。但是旧流==96时需重新查询96
                        this->specific_flow[site_to.site]._96
                            = this->tree[site_to.site].queryK(this->specific_index._96);
                    }
                    else if (site_to.old_flow == this->specific_flow[site_to.site]._95)
                    {
                        /* old_flow == 原95时：
                         * 新值>96, 95、96都左移
                         * 新值<=96 且 > 95, 96不变, 新流变95
                         */
                        if (site_to.new_flow > this->specific_flow[site_to.site]._96)
                        {
                            this->specific_flow[site_to.site]._95 = this->specific_flow[site_to.site]._96;
                            this->specific_flow[site_to.site]._96
                                = this->tree[site_to.site].queryK(this->specific_index._96);
                        }
                        else if (site_to.new_flow > this->specific_flow[site_to.site]._95)
                            this->specific_flow[site_to.site]._95 = site_to.new_flow;
                    }
                    else if (site_to.old_flow <= this->specific_flow[site_to.site]._94)
                    {
                        /* old_flow < 原95：
                         * 根据新值的范围进行不同程度的左移即可
                         */
                        if (site_to.new_flow > this->specific_flow[site_to.site]._96)
                        {
                            this->specific_flow[site_to.site]._94 = this->specific_flow[site_to.site]._95;
                            this->specific_flow[site_to.site]._95 = this->specific_flow[site_to.site]._96;
                            this->specific_flow[site_to.site]._96
                                = this->tree[site_to.site].queryK(this->specific_index._96);
                        }
                        else if (site_to.new_flow > this->specific_flow[site_to.site]._95)
                        {
                            this->specific_flow[site_to.site]._94 = this->specific_flow[site_to.site]._95;
                            this->specific_flow[site_to.site]._95 = site_to.new_flow;
                        }
                        else if (site_to.new_flow > this->specific_flow[site_to.site]._94)
                            this->specific_flow[site_to.site]._94 = site_to.new_flow;
                    }
                }

                // 维护flow的mtime
                this->flow_to_m_time_per_eige_site[site_to.site][site_to.old_flow].erase(mtime);
                this->flow_to_m_time_per_eige_site[site_to.site][site_to.new_flow].insert(mtime);
                // 维护95值(95值一定与预更新相同)
                this->specific_flow[site_to.site]._95 = site_to.new_flow_95;
                // 维护不同时刻节点的stream和
                this->edge_site_total_stream_per_time[mtime][site_to.site] = site_to.new_flow;
                // 维护节点的stream所有时间的和
                this->total_stream_per_edge_site[site_to.site] += stream.flow;
                // 维护cost
                this->cost_per_edge_site[site_to.site] = site_to.new_cost;
                // 维护ans
                ans[mtime][site_to.site].push_back(stream);


                /* 再删除 */
                // 维护94、95、96的值（site_from的old flow一定是95值）
                if (data.get_mtime_num() == 1)
                    this->specific_flow[site_from.site]._95 = site_from.new_flow;
                else
                {
                    if (site_from.new_flow >= this->specific_flow[site_from.site]._94)    // 仅更新95
                        this->specific_flow[site_from.site]._95 = site_from.new_flow;
                    else    // 减小后<94, 右移
                    {
                        this->specific_flow[site_from.site]._95 = this->specific_flow[site_from.site]._94;
                        this->specific_flow[site_from.site]._94
                            = this->tree[site_from.site].queryK(this->specific_index._94);
                    }
                }
                // 维护flow的mtime
                // mtime选取的就是begin，删除begin加速
                this->flow_to_m_time_per_eige_site[site_from.site][site_from.old_flow].erase(
                    this->flow_to_m_time_per_eige_site[site_from.site][site_from.old_flow].begin());
                this->flow_to_m_time_per_eige_site[site_from.site][site_from.new_flow].insert(mtime);
                // 维护95值
                this->specific_flow[site_from.site]._95 = site_from.new_flow_95;
                // 维护不同时刻节点的stream和
                this->edge_site_total_stream_per_time[mtime][site_from.site] = site_from.new_flow;
                // 维护节点的stream所有时间的和
                this->total_stream_per_edge_site[site_from.site] -= stream.flow;
                // 维护cost
                this->cost_per_edge_site[site_from.site] = site_from.new_cost;
                // 维护ans(与最后一个值进行交换，实现O(1)的删除)
                swap(stream, ans[mtime][site_from.site].back());
                ans[mtime][site_from.site].pop_back();


                // 累加cost
                now_cost += diff;
            }
        }


        // 执行完算法后的答案更新
        if (now_cost < this->best_cost)
        {
            this->best_cost = now_cost;
            // 将答案整合进distribution中
            //[mtime][customer][...] = <edge_site, stream_type>
            Distribution distribution(data.get_mtime_num(), vector<vector<pair<int, int>>>(data.get_customer_num()));
            for (size_t m_time = 0; m_time < data.get_mtime_num(); ++m_time)
            {
                for (size_t edge_site = 0; edge_site < data.get_edge_num(); ++edge_site)
                {
                    for (const auto &stream : ans[m_time][edge_site])
                    {
                        distribution[m_time][stream.customer_site].emplace_back(edge_site, stream.stream_type);
                    }
                }
            }
            best_distribution = distribution;
            cout << "best_cost: " << this->best_cost << endl;
        }
        return;
    }

public:
    SOLVE_SA(Data data) : data(data)
    {
        /* 初始化随机种子 */
        srand(time(NULL));

        /* 预分配成员变量的空间，并完成一定的初始化 */
        // 结果的初始化
        this->best_distribution.assign(data.get_mtime_num(), Distribution_t(data.get_customer_num()));
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

    Distribution excute()
    {
        this->simple_ffd();
        this->excute_sa(1e12, 0.9999994, 1e2);
        return this->best_distribution;
    }
}

;
#endif    // !_SA_
