/*
 * 测试数组操作、函数递归、短路求值和变量shadow
 * 功能：实现二分查找并验证结果
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
        // 测试短路求值
        if (arr[mid] < target && mid + 1 <= high) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return -1;
}

// 生成排序数组
void generate_sorted_array(int arr[], int size) {
    int i = 0;
    while (i < size) {
        arr[i] = i * 2 + 1; // 生成奇数序列
        i = i + 1;
    }
}

int main() {
    // 测试shadow变量
    int size = 10;
    {
        int size = 20; // shadow外层size
        int arr[20];
        generate_sorted_array(arr, size);
        
        // 测试查找
        int target = 15;
        int result = binary_search(arr, size, target);
        putint(result); // 应输出7
        putch(10);
    }
    
    // 处理输入数组
    int arr[100];
    int actual_size = getarray(arr);
    int target = getint();
    
    // 测试边界条件
    if (actual_size > 0 && binary_search(arr, actual_size, target) != -1) {
        putch(89); // 'Y'
    } else {
        putch(78); // 'N'
    }
    putch(10);
    
    return 0;
}