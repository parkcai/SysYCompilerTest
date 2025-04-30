/*
 * 测试常量数组初始化、函数递归、逻辑运算短路和变量shadow
 * 功能：实现二分查找并验证结果
 */

const int SORTED_ARRAY[10] = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19}; // 全局常量数组

// 递归实现二分查找
int binary_search(int left, int right, int target) {
    if (left > right) {
        return -1; // 未找到
    }
    
    int mid = left + (right - left) / 2;
    
    if (SORTED_ARRAY[mid] == target) {
        return mid;
    } else if (SORTED_ARRAY[mid] > target) {
        return binary_search(left, mid - 1, target);
    } else {
        return binary_search(mid + 1, right, target);
    }
}

int main() {
    int target = getint(); // 获取要查找的目标值
    
    // 测试shadow变量
    {
        const int SORTED_ARRAY = 42; // shadow全局常量
        putint(SORTED_ARRAY); // 应输出42
        putch(10); // 换行
    }
    
    // 测试短路特性
    if (target < SORTED_ARRAY[0] || target > SORTED_ARRAY[9]) {
        putch(78); // 'N'
        putch(111); // 'o'
        putch(116); // 't'
        putch(32); // ' '
        putch(102); // 'f'
        putch(111); // 'o'
        putch(117); // 'u'
        putch(110); // 'n'
        putch(100); // 'd'
    } else {
        int result = binary_search(0, 9, target);
        if (result != -1) {
            putint(result); // 输出找到的索引
        } else {
            putch(45); // '-'
            putch(49); // '1'
        }
    }
    
    return 0;
}