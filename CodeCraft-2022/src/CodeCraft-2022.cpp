/*
 * @Author: xv_rong
 * @Date: 2022-03-31 20:44:44
 * @LastEditors: xv_rong
 */
#include "config.h"
#include "data.h"
#include "knapsack.cpp"
#include <iostream>
int main() {
    std::cout << INPUT << std::endl;
    Data data = read_file();
    // 实现功能
    Knapsack knapsack(data);
    Distribution distribution = knapsack.excute();

    output_distribution(data, distribution);

    std::cout << "over" << std::endl;
    return 0;
}
