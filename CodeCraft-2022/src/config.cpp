/*
 * @Author: LinXuan
 * @Date: 2022-03-19 12:53:02
 * @Description:
 * @LastEditors: xv_rong
 * @LastEditTime: 2022-03-23 17:41:03
 * @FilePath: /Flow-distribution-optimization/CodeCraft-2022/src/config.cpp
 */
#include "config.h"

#ifdef _DEBUG
const string INPUT = "./data/";
const string OUTPUT = "./data/";
ostream &debug = cout;
#else
ofstream debug = ofstream("/dev/null");
const string INPUT = "/data/";
const string OUTPUT = "/output/";
#endif
