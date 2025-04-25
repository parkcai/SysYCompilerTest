/*
 * 测试数组操作、函数递归和逻辑运算
 * 功能：实现二分查找算法并验证结果
 */

// 二分查找函数
int binary_search(int arr[], int size, int target) {
    int low = 0;
    int high = size - 1;
    
    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (arr[mid] == target) {
            return mid;
        }
        if (arr[mid] < target) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return -1;
}

// 测试数组排序检查
int is_sorted(int arr[], int size) {
    int i = 1;
    while (i < size) {
        if (arr[i-1] > arr[i]) {
            return 0;
        }
        i = i + 1;
    }
    return 1;
}

int main() {
    // 测试数组
    int arr[10];
    int n = 10;
    
    // 初始化数组
    int i = 0;
    while (i < n) {
        arr[i] = getint();
        i = i + 1;
    }
    
    // 检查数组是否有序
    if (!is_sorted(arr, n)) {
        putch(33); // '!'
        putch(10);
        return -1;
    }
    
    // 查找目标值
    int target = getint();
    int result = binary_search(arr, n, target);
    
    // 输出结果
    putint(result);
    putch(10);
    
    // 测试shadow变量
    {
        int arr[5] = {1, 2, 3, 4, 5};
        int target = 3;
        putint(binary_search(arr, 5, target)); // 应输出2
        putch(10);
    }
    
    return 0;
}