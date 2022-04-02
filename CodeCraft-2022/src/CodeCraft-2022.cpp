/*
 * @Author: xv_rong
 * @Date: 2022-03-31 20:44:44
 * @LastEditors: xv_rong
 */
#include "config.h"
#include "data.h"
#include <iostream>
int main() {
    std::cout << INPUT << std::endl;
    Data data = read_file();

    // 实现功能
    FFD ffd(data);
    Distribution distribution = ffd.excute();
    output_distribution(data, distribution);
    debug << cal_cost(data, distribution) << endl;
    std::cout << "over" << std::endl;
    return 0;
}
