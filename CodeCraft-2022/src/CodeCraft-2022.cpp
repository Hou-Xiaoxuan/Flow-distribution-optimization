/*
 * @Author: xv_rong
 * @Date: 2022-03-31 20:44:44
 * @LastEditors: LinXuan
 */
#include "config.h"
#include "data.h"
#include "hungarian.h"
int main()
{
    std::cout << INPUT << std::endl;
    Data data = read_file();

    // 实现功能
    Hungarian solve(data);
    Distribution distribution = solve.excute();
    output_distribution(data, distribution);
    std::cout << "finally cost = " << cal_cost(data, distribution) << endl;

#ifdef _DEBUG
    check_distribution(data, distribution);
#endif
    std::cout << "over" << endl;
    return 0;
}
