/*
 * 测试数组操作、函数递归和逻辑运算
 * 功能：实现二分查找算法并验证结果
 */

// 二分查找函数
int binary_search(int arr[], int left, int right, int target) {
    if (left > right) {
        return -1;
    }
    
    int mid = left + (right - left) / 2;
    
    if (arr[mid] == target) {
        return mid;
    } else if (arr[mid] > target && left <= mid - 1) { // 测试逻辑短路
        return binary_search(arr, left, mid - 1, target);
    } else {
        return binary_search(arr, mid + 1, right, target);
    }
}

// 测试shadow变量
void test_shadow() {
    int arr[5] = {1, 2, 3, 4, 5};
    {
        int arr[3] = {10, 20, 30};
        putarray(3, arr); // 应输出10 20 30
    }
    putarray(5, arr); // 应输出1 2 3 4 5
}

int main() {
    // 初始化有序数组
    int arr[10];
    int i = 0;
    while (i < 10) {
        arr[i] = getint();
        i = i + 1;
    }
    
    // 输出原始数组
    putarray(10, arr);
    
    // 测试shadow变量
    test_shadow();
    
    // 查找目标值
    int target = getint();
    int result = binary_search(arr, 0, 9, target);
    
    // 输出查找结果
    putint(result);
    putch(10);
    
    return 0;
}