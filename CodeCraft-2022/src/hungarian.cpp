/*
 * @Author: LinXuan
 * @Date: 2022-04-03 21:52:23
 * @Description:
 * @LastEditors: LinXuan
 * @LastEditTime: 2022-04-03 22:48:40
 * @FilePath: /FDO/CodeCraft-2022/src/hungarian.cpp
 */
#include "hungarian.h"
/*-----------------------do something per mtime-----------------------*/
/* 在m_time上执行匹配 */
bool Hungarian::excute_match_per_mtime(vector<Stream> &stream_node, vector<vector<int>> &matched_per_edge,
    vector<int> &residual_capacity, const vector<int> &edge_order, int max_depth)
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

        if (depth <= 0)
            return false;
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
        if (stream_node[i].used == false and find_path(i, max_depth))
        {
            stream_node[i].used = true;
            continue;
        }
        else
        {
            flag = false;
            break;
        }
    }

    return flag;
}

void Hungarian::update_distribution_per_mtime(size_t mtime, Distribution &distribution,
    const vector<vector<int>> &matched_per_edge, const vector<Stream> &stream_node)
{
    // 写入distribution
    distribution[mtime].assign(data.get_customer_num(), vector<pair<int, int>>());
    Distribution_t &distribution_t = distribution[mtime];
    for (size_t edge_site = 0; edge_site < data.get_edge_num(); edge_site++)
    {
        for (int i : matched_per_edge[edge_site])
        {
            Stream stream = stream_node[i];
            distribution_t[stream.customer_site].push_back({edge_site, stream.stream_type});
        }
    }
}

void Hungarian::update_best_distribution(const Distribution &distribution)
{
    int cost = cal_cost(data, distribution);
    if (cost < best_cost)
    {
        debug << "from " << this->best_cost << " to " << cost << endl;
        this->best_cost = cost;
        this->best_distribution = distribution;
    }
}

/*-----------------------特殊函数-------------------------*/
/* 执行一遍基本的FFD，获取用到的edge_site节点 */
vector<int> Hungarian::get_edge_order_by_simple_hungarian()
{
    Distribution distribution(data.get_mtime_num(), Distribution_t());

    vector<int> edge_order;     // 边缘节点的遍历顺序
    set<int> used_edge_site;    // 使用过的ede_site
    /*-----------------------------------执行一次FFD的匹配逻辑， 获取使用到的edge_site----------------------------*/
    for (size_t i = 0; i < data.get_edge_num(); i++)
    {
        edge_order.push_back(i);
    }

    for (size_t mtime = 0; mtime < data.get_mtime_num(); mtime++)
    {
        vector<Stream> stream_node;    // 客户流量 3500
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

        vector<vector<int>> matched_per_edge(data.get_edge_num());    // edge节点的匹配数组
        vector<int> residual_capacity = data.site_bandwidth;          // 边缘节点剩余容量 135

        this->excute_match_per_mtime(stream_node, matched_per_edge, residual_capacity, edge_order);

        // 更新used_edge_site
        for (size_t edge_site : edge_order)
        {
            if (residual_capacity[edge_site] != data.site_bandwidth[edge_site])
            {
                used_edge_site.insert(edge_site);
            }
        }
        this->update_distribution_per_mtime(mtime, distribution, matched_per_edge, stream_node);
        // debug << "excute match in mtime :" << mtime << endl;
    }
    this->update_best_distribution(distribution);


    /*获取edge_order*/
    vector<int>().swap(edge_order);    // 清空edge_order
    edge_order.reserve(data.get_edge_num());
    for (auto ite : used_edge_site)
    {
        edge_order.push_back(ite);
    }

    return edge_order;
}
///////////////////////////////完全独立的倍增/////////////////////
/* 执行倍增逻辑，获取最优状态下的edge_site节点 */
void Hungarian::init()
{
    // 将同一个时刻的流量重整为一个一维数组
    //[m_time][...] <<stream>, <stream_type, customer_site>>
    vector<vector<pair<int, pair<int, int>>>> stream_per_time(data.demand.size());
    for (size_t m_time = 0; m_time < data.demand.size(); ++m_time)
    {
        const auto &ori_demand_now_time = data.demand[m_time];
        auto &demand_now_time = stream_per_time[m_time];
        demand_now_time.reserve(100 * ori_demand_now_time.size());
        // demand_now_time.push_back({-1, {-1, -1}}); // 偏移 1
        for (size_t customer_site = 0; customer_site < ori_demand_now_time.size(); ++customer_site)
        {
            for (size_t stream_index = 0; stream_index < ori_demand_now_time[customer_site].size(); ++stream_index)
            {
                const auto &item = ori_demand_now_time[customer_site][stream_index];
                if (item.first > 0)
                    stream_per_time[m_time].push_back({
                        item.first, {item.second, customer_site}
                    });
            }
        }
        demand_now_time.shrink_to_fit();
        sort(demand_now_time.begin(), demand_now_time.end(), greater<pair<int, pair<int, int>>>());
    }

    vector<int> edge_order;    // 决定每轮遍历的顺序
    edge_order.reserve(data.edge_site.size());
    for (size_t edge_site = 0; edge_site < data.edge_site.size(); ++edge_site)
    {
        edge_order.push_back(edge_site);
    }

    int v = max(1024, data.base_cost);
    int max_bandwidth = *max_element(data.site_bandwidth.begin(), data.site_bandwidth.end());
    while (true)
    {
        vector<vector<vector<pair<int, pair<int, int>>>>> ans(
            data.demand.size(), vector<vector<pair<int, pair<int, int>>>>(data.site_bandwidth.size()));
        bool is_v_cover = true;
        for (size_t m_time = 0; m_time < data.demand.size(); ++m_time)
        {
            vector<int> edge_cap = data.site_bandwidth;
            for (size_t edge_site = 0; edge_site < data.edge_site.size(); ++edge_site)
            {
                edge_cap[edge_site] = min(v, edge_cap[edge_site]);
            }
            for (size_t stream_index = 0; stream_index < stream_per_time[m_time].size(); ++stream_index)
            {
                bool flag = false;
                const auto &stream = stream_per_time[m_time][stream_index];
                int stream_flow = stream.first;
                int customer_site = stream.second.second;
                for (const auto &edge_site : edge_order)
                {
                    if (edge_cap[edge_site] >= stream_flow)
                    {

                        if (data.qos[customer_site][edge_site] < data.qos_constraint)
                        {    // qos 限制
                            ans[m_time][edge_site].push_back(stream);
                            edge_cap[edge_site] -= stream_flow;
                            flag = true;
                            break;
                        }
                    }
                }
                if (flag == false)
                {    //找不到, 装不下
                    is_v_cover = false;
                    break;
                }
            }
            if (is_v_cover == false)
            {
                break;
            }
        }
        // [m_time][edge_site][...] <steam, <stream_type, customer_site>>
        // vector<vector<vector<pair<int, pair<int, int>>>>> ans;
        // 将答案整合进distribution中
        // [mtime][customer][...] = <edge_site, stream_type>
        if (is_v_cover == true)
        {
            Distribution distribution(data.demand.size(), vector<vector<pair<int, int>>>(data.customer_site.size()));
            for (size_t m_time = 0; m_time < data.demand.size(); ++m_time)
            {
                for (size_t edge_site = 0; edge_site < data.site_bandwidth.size(); ++edge_site)
                {
                    for (size_t stream_index = 0; stream_index < ans[m_time][edge_site].size(); ++stream_index)
                    {
                        const auto &stream = ans[m_time][edge_site][stream_index];
                        int customer_site = stream.second.second;
                        int stream_type = stream.second.first;
                        distribution[m_time][customer_site].push_back({edge_site, stream_type});
                    }
                }
            }
            int cost = cal_cost(data, distribution);
            if (cost < best_cost)
            {
                best_cost = cost;
                this->best_distribution = distribution;
                cout << best_cost << endl;
            }
        }
        if (v >= max_bandwidth)
        {
            break;
        }
        v *= 1.4;
    }
}
vector<int> Hungarian::get_edge_order_by_beizeng()
{
    this->init();
    // 从当前的best_ans中读出最优状态时的edge_order;
    set<int> edge_used;
    for (const Distribution_t &distribution_t : this->best_distribution)
    {
        // <edge_site, stream_type>
        for (size_t customer_site = 0; customer_site < data.get_customer_num(); customer_site++)
        {
            for (auto item : distribution_t[customer_site])
            {
                edge_used.insert(item.first);
            }
        }
    }
    vector<int> edge_order;
    edge_order.reserve(data.get_edge_num());
    for (auto i : edge_used)
    {
        edge_order.push_back(i);
    }
    return edge_order;
}
//////////////////////////////倍增结束///////////////////////////



/*-------------------------在给定的edge_order上运行exploit---------*/
void Hungarian::do_exploit_on_edge_order(vector<int> edge_order)
{
    Distribution distribution(data.get_mtime_num(), Distribution_t());

    /*-------------------------------执行exploit逻辑--------------------------------------------*/
    /* 初始化per_time变量 */
    vector<vector<vector<int>>> edge_matched_per_mtime(
        data.get_mtime_num(), vector<vector<int>>(data.get_edge_num()));                 // edge节点的匹配数组
    vector<vector<Stream>> stream_ndoe_per_mtime(data.get_mtime_num());                  // stream_node
    vector<vector<int>> residual_capacity(data.get_mtime_num(), data.site_bandwidth);    // 边缘节点剩余容量 135
    for (size_t mtime = 0; mtime < data.get_mtime_num(); mtime++)
    {
        const auto &demant_t = data.demand[mtime];
        auto &stream_node = stream_ndoe_per_mtime[mtime];
        stream_node.reserve(data.get_customer_num() * 100);
        for (size_t i = 0; i < demant_t.size(); i++)
        {
            for (auto item : demant_t[i])
            {
                if (item.first > 0)
                {
                    stream_node.push_back({static_cast<int>(i), item.first, item.second});
                }
            }
        }
        stream_node.shrink_to_fit();
        sort(stream_node.begin(), stream_node.end(), greater<Stream>());
    }

    /* 函数，获取指定时刻的edge需求并从大到小排序*/
    auto get_demand_sequence = [&](const vector<Stream> &stream_ndoe) -> vector<pair<int, int>> {
        vector<pair<int, int>> demand_sequence(data.get_edge_num());
        for (auto edge_site : edge_order)
        {
            demand_sequence[edge_site].first = edge_site;
            for (const auto &stream : stream_ndoe)
            {
                if (this->is_connect(stream.customer_site, edge_site))
                {
                    demand_sequence[edge_site].first += stream.flow;
                }
            }
        }
        sort(demand_sequence.begin(), demand_sequence.end());
        return demand_sequence;
    };
    vector<vector<pair<int, int>>> demand_sequence_per_mtime(data.get_mtime_num());
    for (size_t mtime = 0; mtime < data.get_mtime_num(); mtime++)
    {
        demand_sequence_per_mtime[mtime] = get_demand_sequence(stream_ndoe_per_mtime[mtime]);
    }

    /* 选取点进行exploit */
    vector<int> exploit_count(data.get_edge_num(), 0);
    int max_exploit_count = data.get_mtime_num() - ((data.get_mtime_num() * 95 - 1) / 100 + 1);
    int debug_coutn = 0;
    while (true and ++debug_coutn)
    {
        debug << "exploit times: " << debug_coutn << endl;
        struct {
            int flow = -1;
            int mtime = -1;
            int edge_site = -1;
        } site;
        for (size_t mtime = 0; mtime < data.get_mtime_num(); mtime++)
        {
            auto &demand_sequence = demand_sequence_per_mtime[mtime];
            while (
                demand_sequence.empty() == false and exploit_count[demand_sequence.back().second] > max_exploit_count)
            {
                demand_sequence.pop_back();
            }
            if (demand_sequence.empty() == false and demand_sequence.back().first > site.flow)
            {
                site.flow = demand_sequence.back().first;
                site.edge_site = demand_sequence.back().second;
                site.mtime = mtime;
            }
        }

        if (site.mtime == -1)
            break;
        // 执行exploit
        auto &matched = edge_matched_per_mtime[site.mtime][site.edge_site];
        auto &capacity = residual_capacity[site.mtime][site.edge_site];
        auto &stream_node = stream_ndoe_per_mtime[site.mtime];
        for (size_t i = 0; i < stream_node.size(); i++)
        {
            Stream &stream = stream_node[i];
            // clang-format off
                if (stream.used == false 
                and this->is_connect(stream.customer_site, site.edge_site
                and capacity >= site.flow)){
                    stream.used = true;         // 标记为用过
                    capacity -= stream.flow;    // 减去流量
                    matched.push_back(i);       // 加入匹配
                }
            // clang-format on
        }
        // 更新状态
        exploit_count[site.edge_site]++;
        demand_sequence_per_mtime[site.mtime] = get_demand_sequence(stream_ndoe_per_mtime[site.mtime]);
    }


    /*重新进行FFD&&匈牙利*/
    bool success = true;
    for (size_t mtime = 0; mtime < data.get_mtime_num(); mtime++)
    {
        success = this->excute_match_per_mtime(
            stream_ndoe_per_mtime[mtime], edge_matched_per_mtime[mtime], residual_capacity[mtime], edge_order, 1);
        if (success == true)
            this->update_distribution_per_mtime(
                mtime, distribution, edge_matched_per_mtime[mtime], stream_ndoe_per_mtime[mtime]);
        else
            break;
    }

    if (success)
    {
        debug << "valid after exploit" << endl;
        this->update_best_distribution(distribution);
    }
}
