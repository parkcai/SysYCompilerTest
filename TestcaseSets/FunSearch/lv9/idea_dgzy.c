/*
 * 测试数组操作、函数递归和短路逻辑运算
 * 功能：实现二分查找并验证查找结果
 */

// 递归二分查找
int binary_search(int arr[], int left, int right, int target) {
    if (left > right) {
        return -1;
    }
    int mid = left + (right - left) / 2;
    if (arr[mid] == target) {
        return mid;
    } else if (arr[mid] < target) {
        return binary_search(arr, mid + 1, right, target);
    } else {
        return binary_search(arr, left, mid - 1, target);
    }
}

// 测试短路逻辑运算
int check_index(int arr[], int index) {
    // 利用短路特性防止数组越界
    return index >= 0 && index < 10 && arr[index] % 2 == 0;
}

int main() {
    int data[10];
    int size = getarray(data); // 获取输入数组
    
    // 先排序数组以便二分查找
    int i = 0;
    while (i < size - 1) {
        int j = 0;
        while (j < size - i - 1) {
            if (data[j] > data[j + 1]) {
                int temp = data[j];
                data[j] = data[j + 1];
                data[j + 1] = temp;
            }
            j = j + 1;
        }
        i = i + 1;
    }
    
    // 输出排序后数组
    putarray(size, data);
    putch(10); // 换行
    
    // 测试二分查找
    int target = getint();
    int result = binary_search(data, 0, size - 1, target);
    putint(result);
    putch(10);
    
    // 测试shadow变量和短路逻辑
    {
        int size = 5;
        int test_arr[5] = {2, 4, 6, 8, 10};
        if (check_index(test_arr, size - 1) && (100 / size > 0)) {
            putint(test_arr[size - 1]); // 应输出10
        }
    }
    
    return 0;
}