/*
 * @Author: LinXuan
 * @Date: 2022-04-01 14:06:08
 * @Description: 
 * @LastEditors: LinXuan
 * @LastEditTime: 2022-04-01 19:46:38
 * @FilePath: /FDO/CodeCraft-2022/src/hungarian.h
 */
# ifndef _HUNGARIAN_
# define _HUNGARIAN
# include "config.h"
# include "data.h"
# include <bits/stdc++.h>
using namespace std;
/*  边缘节点作为可以多次匹配的右节点
    细分的流量作为单词匹配的左节点 进行二分图匹配
*/
class Hungarian
{
private:
    /* data */
    Data data;
    
    Distribution distribution;      // 最终分配结果

    // 三元组，存客户节点流量
    struct Stream
    {
        int customer_site;
        int flow;
        int stream_type;
        Stream(int cus, int flow, int type):customer_site(cus), flow(flow), stream_type(type){}
        bool operator <(const Stream rhs) const{
            return this->flow < rhs.flow;
        }
        bool operator >(const Stream rhs) const{
            return this->flow > rhs.flow;
        }
    };
    vector<Stream> stream_node;     // 客户流量 3500
    vector<bool> vis;                // 单词搜索时边缘节点的标记
    vector<vector<int>> matched_per_edge;      // edge节点的匹配数组
    vector<int> residual_capacity;  // 边缘节点剩余容量 135

    inline size_t get_stream_num() const{
        if(this->stream_node.empty()){
            return data.get_customer_num()*100;
        }
        return stream_node.size();
    }

    inline bool is_connect(size_t customer_site, size_t edge_site) const{
        return data.qos[customer_site][edge_site] < data.qos_constraint;
    }

    /* 在m_time上执行匹配 */
    void excute_match_per_mtime(size_t mtime)
    {
        // 摊平图,并排序
        this->stream_node.reserve(data.get_customer_num()*100);
        const auto &demant_t = data.demand[mtime];
        for(size_t i=0; i<demant_t.size(); i++)
        {
            for(auto item:demant_t[i])
            {   
                if(item.first > 0){ // 筛掉flow为0的流量
                    this->stream_node.push_back({static_cast<int>(i), item.first, item.second});
                }
            }
        }
        sort(this->stream_node.begin(), this->stream_node.end(), greater<Stream>());
        // 初始化
        this->residual_capacity = data.site_bandwidth; // 初值为最大流量
        this->vis.assign(data.get_edge_num(), false);
        this->matched_per_edge.assign(data.get_edge_num(), vector<int>());

        // 执行增广路过程
        int match_num = 0;
        for(size_t i=0; i<this->stream_node.size(); i++){
            if(find_path(i)){
                match_num++;
            } 
        #ifdef _DEBUG
            else {
                throw "分配方案不合法，没有全部分配";
            }
        #endif
        }

        
        // TODO: 更新匹配进入distrubution
    }

    /* 寻找增广路 */
    bool find_path(int u)
    {
        // 遍历所有的edge_site界定啊
        Stream stream = this->stream_node[u];
        // 有剩余容量: 直接加入匹配，返回
        for(size_t edge_site=0; edge_site<data.get_edge_num(); edge_site++)
        {
            if(this->is_connect(stream.customer_site, edge_site) /*and vis[edge_site]==false*/)
            {
                if(this->residual_capacity[edge_site] >= stream.flow){
                    this->matched_per_edge[edge_site].push_back(u);
                    this->residual_capacity[edge_site] -= stream.flow;
                    return true;
                }
            }
        }
        // 没有剩余容量: 以匹配的流量中，能腾出空间的，重新匹配
        // 再此过程中，需要标记不要重复访问边缘节点
        // for(size_t edge_site=0; edge_site<data.get_edge_num(); edge_site++)
        // {   
        //     if(this->is_connect(stream.customer_site, edge_site) and vis[edge_site]==false)
        //     {
        //         vis[edge_site] = true;
        //         auto &matcharray = this->matched_per_edge[edge_site];
        //         // 到着遍历
        //         for(auto ite=matcharray.begin(); ite!=matcharray.end(); ite++)
        //         {
        //             // 尝试回退一个可以腾出足够空间其他节点
        //             if(this->stream_node[*ite].flow + this->residual_capacity[edge_site] >= stream.flow)
        //             {
        //                 if(find_path(*ite) == true) {
        //                     // 回退成功
        //                     this->residual_capacity[edge_site] += this->stream_node[*ite].flow;
        //                     this->residual_capacity[edge_site] -= stream.flow;
        //                     matcharray.erase(ite);
        //                     matcharray.push_back(u);
        //                     vis[edge_site] = false; // 回退标记数组
        //                     return true;
        //                 }
        //             }
        //         }
        //         vis[edge_site] = false; // 回退标记
        //     }
        // }
        return false;
    }
public:
    Hungarian(const Data &_data): data(_data)
    {
        this->distribution.assign(data.get_mtime_num(), Distribution_t());
    }
    Distribution excute()
    {
        for(size_t i=0; i<data.get_mtime_num(); i++){
            this->excute_match_per_mtime(i);
            cout<<"excute match in mtime :"<<i<<endl;
        }

        return this->distribution;
    }
};
# endif
