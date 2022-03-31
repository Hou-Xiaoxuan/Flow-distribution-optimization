/*
 * @Author: LinXuan
 * @Date: 2022-03-19 10:51:41
 * @Description: 
 * @LastEditors: LinXuan
 * @LastEditTime: 2022-03-21 17:29:16
 * @FilePath: /Flow-distribution-optimization/CodeCraft-2022/src/config.h
 */
#ifndef _CONFIG_
#define _CONFIG_

# include <iostream>
# include <fstream>
using namespace std;

extern const string INPUT;
extern const string OUTPUT;
# ifdef _DEBUG
extern ostream &debug;
# else
extern ofstream debug;
# endif
#endif
