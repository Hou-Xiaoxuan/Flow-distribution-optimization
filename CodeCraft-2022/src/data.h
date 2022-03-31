/*
 * @Author: LinXuan
 * @Date: 2022-03-31 19:24:16
 * @Description: 
 * @LastEditors: LinXuan
 * @LastEditTime: 2022-03-31 21:22:31
 * @FilePath: /FDO/CodeCraft-2022/src/data.h
 */
#ifndef _DATA_
#define _DATA_
#include<bits/stdc++.h>
#include "config.h"
using namespace std;

/*Distribution类型 [mtime][customer][...] = <edge_site, stream_type>*/
using Distribution = vector<vector<vector<pair<int, int>>>>;
struct Data
{
    /* Data */
    vector<string> customer_site;   // 客户节点
    vector<string> edge_site;       // 边缘节点
    vector<string> stream_type;     // 流量类型
    unordered_map<string, int> re_customer_site;
    unordered_map<string, int> re_edge_site;
    unordered_map<string, int> re_stream_type;

    int qos_constraint;             // qos阀值
    int base_cost;                  // base_cost(暂时无用)
    // [m_time][customer_site][index] = <bandwidth, stream_type>
    vector<vector<vector<pair<int, int>>>> demand;
    vector<int> site_bandwidth;
    // [customer_site][edge_site] = qos
    vector<vector<int>> qos;
};
Data read_file();
void output_distribution(const Data &data, const Distribution &distribution);
vector<string> get_split_line(ifstream &file, char delim);
#endif
