#include <iostream>
#include "config.h"
#include "data.h"
#include "hungarian.h"
int main() {
    std::cout << INPUT <<std::endl;
    Data data = read_file();
    // 实现功能
    Hungarian solve(data);
    Distribution distribution = solve.excute();
    output_distribution(data, distribution);

    std::cout<<"over"<<std::endl;
    return 0;
}
