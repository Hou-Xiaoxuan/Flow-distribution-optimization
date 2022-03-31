/*
 * @Author: LinXuan
 * @Date: 2022-03-31 19:24:16
 * @Description: 
 * @LastEditors: LinXuan
 * @LastEditTime: 2022-03-31 19:46:47
 * @FilePath: /FDO/CodeCraft-2022/src/data.h
 */
#ifndef _DATA_
#define _DATA_
#include<bits/stdc++.h>
#include "config.h"
using namespace std;
struct Data
{
    /* Data */
    vector<string> customer_site;   // 客户节点
    vector<string> edge_site;       // 边缘节点
    vector<string> stream_type;     // 流量类型
    unordered_map<string, int> re_customer_site;
    unordered_map<string, int> re_edge_site;
    unordered_map<string, int> re_stream_type;

    int qos_constraint;
    int base_cost;
    vector<vector<vector<pair<int, int>>>> demand;
    vector<int> site_bandwith;
    vector<vector<int>> qos;
};
Data read_file();

vector<string> get_split_line(ifstream &file, char delim);
#endif
