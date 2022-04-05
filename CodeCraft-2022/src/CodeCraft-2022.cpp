/*
 * @Author: xv_rong
 * @Date: 2022-03-31 20:44:44
 * @LastEditors: xv_rong
 */
#include "config.h"
#include "data.h"
#include "ffd.cpp"
#include <iostream>
int main() {
    std::cout << INPUT << std::endl;
    Data data = read_file();

    // 实现功能
    FFD ffd(data);
    Distribution distribution = ffd.excute();

    // #ifdef _DEBUG
    //     check_distribution(data, distribution);
    //     debug << cal_cost(data, distribution) << endl;
    // #endif

    output_distribution(data, distribution);

    std::cout << "over" << std::endl;
    return 0;
}
