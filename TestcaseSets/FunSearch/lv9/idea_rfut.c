/*
 * 测试多维数组、递归函数、逻辑运算和变量shadow
 * 功能：实现一个简单的二分查找算法，并验证其正确性
 */

// 二分查找函数
int binary_search(int arr[], int left, int right, int target) {
    if (left > right) {
        return -1; // 未找到目标值
    }
    int mid = (left + right) / 2;
    if (arr[mid] == target) {
        return mid; // 找到目标值
    } else if (arr[mid] < target) {
        return binary_search(arr, mid + 1, right, target); // 在右半部分查找
    } else {
        return binary_search(arr, left, mid - 1, target); // 在左半部分查找
    }
}

// 打印数组
void print_array(int arr[], int size) {
    int i = 0;
    while (i < size) {
        putint(arr[i]);
        if (i != size - 1) {
            putch(32); // 空格
        }
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    // 初始化一个有序数组
    int arr[10] = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19};
    
    // 测试变量shadow
    {
        int arr[5] = {1, 2, 3, 4, 5};
        putarray(5, arr); // 输出1 2 3 4 5
    }
    
    // 测试二分查找
    int target = getint(); // 从标准输入读取目标值
    int result = binary_search(arr, 0, 9, target);
    
    // 输出结果
    if (result != -1) {
        putch(84); // 'T'
        putch(105); // 'i'
        putch(110); // 'n'
        putch(116); // 't'
        putch(101); // 'e'
        putch(114); // 'r'
        putch(32); // 空格
        putint(result); // 输出目标值的索引
    } else {
        putch(78); // 'N'
        putch(111); // 'o'
        putch(116); // 't'
        putch(32); // 空格
        putch(102); // 'f'
        putch(111); // 'o'
        putch(117); // 'u'
        putch(110); // 'n'
        putch(100); // 'd'
    }
    putch(10); // 换行
    
    return 0;
}