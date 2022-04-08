/*
 * @Author: xv_rong
 * @Date: 2022-03-31 20:44:44
 * @LastEditors: Please set LastEditors
 */
#include "config.h"
#include "data.h"
#include "sa.cpp"
#include <iostream>
int main() {
    std::cout << INPUT << std::endl;
    Data data = read_file();

    // 实现功能
    SA sa(data);
    Distribution distribution = sa.excute();

#ifdef DEBUG
    check_distribution(data, distribution);
    // cout << cal_cost(data, distribution) << endl;
#endif

    output_distribution(data, distribution);

    std::cout << "over" << std::endl;
    return 0;
}
