/*
 * 测试多维数组操作、函数递归调用、短路逻辑运算和变量shadow
 * 功能：实现并测试一个简单的冒泡排序算法，并验证数组的逆序输出
 */

// 冒泡排序函数
void bubble_sort(int arr[], int n) {
    int i = 0;
    while (i < n - 1) {
        int j = 0;
        while (j < n - i - 1) {
            if (arr[j] > arr[j + 1]) {
                // 交换元素
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// 逆序输出数组
void print_reverse(int arr[], int n) {
    int i = n - 1;
    while (i >= 0) {
        putint(arr[i]);
        putch(32); // 空格
        i = i - 1;
    }
    putch(10); // 换行
}

// 递归求和函数
int sum_recursive(int arr[], int n) {
    if (n == 0) {
        return 0;
    } else {
        return arr[n - 1] + sum_recursive(arr, n - 1);
    }
}

int main() {
    // 初始化数组
    int arr[10] = {5, 3, 8, 1, 9, 2, 7, 4, 6, 0};
    
    // 测试冒泡排序
    bubble_sort(arr, 10);
    
    // 输出排序后的数组
    putarray(10, arr);
    putch(10); // 换行
    
    // 测试递归求和
    int total = sum_recursive(arr, 10);
    putint(total);
    putch(10); // 换行
    
    // 测试逆序输出
    print_reverse(arr, 10);
    
    // 测试变量shadow
    {
        int arr = 42;
        putint(arr); // 应输出42
        putch(10); // 换行
    }
    
    // 测试短路逻辑运算
    if (arr[0] <= 0 || arr[9] >= 10) {
        putch(69); // 'E'
        putch(114); // 'r'
        putch(114); // 'r'
        putch(111); // 'o'
        putch(114); // 'r'
        putch(10);
    }
    
    return 0;
}