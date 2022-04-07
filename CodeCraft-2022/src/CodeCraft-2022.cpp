/*
 * @Author: xv_rong
 * @Date: 2022-03-31 20:44:44
 * @LastEditors: LinXuan
 */
#include "config.h"
#include "data.h"
#include "ffd.cpp"
#include "sa.h"
#include <iostream>
int main()
{
    std::cout << INPUT << std::endl;
    Data data = read_file();

    // 实现功能
        SOLVE_SA SA(data);
        Distribution distribution = SA.excute();
    #ifdef _DEBUG
        check_distribution(data, distribution);
        debug << cal_cost(data, distribution) << endl;
    #endif
        output_distribution(data, distribution);

    // FFD ffd(data);
    // Distribution distribution = ffd.excute();
    // output_distribution(data, distribution);
    return 0;
}
