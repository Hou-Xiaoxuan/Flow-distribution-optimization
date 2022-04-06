/*
 * @Author: LinXuan
 * @Date: 2022-03-25 12:36:25
 * @Description:
 * @LastEditors: LinXuan
 * @LastEditTime: 2022-04-06 22:43:37
 * @FilePath: /FDO/CodeCraft-2022/src/weight_segment_tree.h
 */
#ifndef _WEIGHT_SEGMENT_TREE_
#define _WEIGHT_SEGMENT_TREE_
#include <iostream>
#include <vector>
using namespace std;
class WeightSegmentTree
{
private:
    vector<int> tree;
    int capacity;
    inline void _update(int root, int left, int right, int num, int v)
    {
        int lc = root << 1, rc = root << 1 | 1, mid = (left + right) >> 1;

        if (left == right)
        {
            tree[root] += v;
            return;
        }

        if (num <= mid)
            _update(lc, left, mid, num, v);
        else
            _update(rc, mid + 1, right, num, v);

        tree[root] = tree[lc] + tree[rc];
    }
    inline int _queryK(int root, int left, int right, int num)
    {
        int lc = root << 1, rc = root << 1 | 1, mid = (left + right) >> 1;

        if (left == right)
            return left;

        if (tree[lc] >= num)
            return _queryK(lc, left, mid, num);
        else
            return _queryK(rc, mid + 1, right, num - tree[lc]);
    }

public:
    WeightSegmentTree(size_t capacity) : tree(((capacity + 5) << 2), 0), capacity(static_cast<int>(capacity)) { }
    WeightSegmentTree() { }
    inline void update(int root, int left, int right, int num, int v)
    {
        _update(root + 1, left + 1, right + 1, num + 1, v);
    }
    inline int queryK(int root, int left, int right, int num)
    {
        return _queryK(root + 1, left + 1, right + 1, num) - 1;
    }
    /*增加重载，去掉多余参数，同时进行+1的偏移*/
    inline void update(int num, int v) { _update(1, 1, this->capacity + 1, num + 1, v); }
    inline int queryK(int num) { return _queryK(1, 1, this->capacity + 1, num) - 1; }
};
#endif    // _WEIGHT_SEGMENT_TREE_
