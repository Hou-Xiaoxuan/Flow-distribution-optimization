/*
 * @Author: LinXuan
 * @Date: 2022-03-25 12:36:25
 * @Description:
 * @LastEditors: xv_rong
 * @LastEditTime: 2022-04-03 16:21:06
 * @FilePath: /FDO/CodeCraft-2022/src/weight_segment_tree.h
 */
#include <iostream>
#include <vector>
using namespace std;
class WeightSegmentTree {
private:
    vector<int> tree;

    inline void _update(int root, int left, int right, int num, int v) {
        int lc = root << 1, rc = root << 1 | 1, mid = (left + right) >> 1;

        if (left == right) {
            tree[root] += v;
            return;
        }

        if (num <= mid)
            _update(lc, left, mid, num, v);
        else
            _update(rc, mid + 1, right, num, v);

        tree[root] = tree[lc] + tree[rc];
    }
    inline int _queryK(int root, int left, int right, int num) {
        int lc = root << 1, rc = root << 1 | 1, mid = (left + right) >> 1;

        if (left == right)
            return left;

        if (tree[lc] >= num)
            return _queryK(lc, left, mid, num);
        else
            return _queryK(rc, mid + 1, right, num - tree[lc]);
    }

public:
    WeightSegmentTree(size_t capacity) : tree(((capacity + 5) << 2), 0) {
    }
    WeightSegmentTree() {
    }
    inline void update(int root, int left, int right, int num, int v) {
        _update(root + 1, left + 1, right + 1, num + 1, v);
    }
    inline int queryK(int root, int left, int right, int num) {
        return _queryK(root + 1, left + 1, right + 1, num);
    }
};
