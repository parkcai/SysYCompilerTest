/*
 * 测试数组操作、递归、短路求值、变量shadow和库函数使用
 * 功能：实现并测试快速排序算法
 */

// 交换数组中的两个元素
void swap(int arr[], int i, int j) {
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

// 快速排序的分区函数
int partition(int arr[], int low, int high) {
    int pivot = arr[high];
    int i = low - 1;
    int j = low;
    
    while (j < high) {
        // 测试短路求值
        if (arr[j] < pivot || (arr[j] == pivot && i < j)) {
            i = i + 1;
            swap(arr, i, j);
        }
        j = j + 1;
    }
    swap(arr, i + 1, high);
    return i + 1;
}

// 递归实现快速排序
void quick_sort(int arr[], int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quick_sort(arr, low, pi - 1);
        quick_sort(arr, pi + 1, high);
    }
}

// 测试变量shadow
void test_shadow() {
    int arr[5] = {5, 4, 3, 2, 1};
    putarray(5, arr); // 输出5 4 3 2 1
    
    {
        int arr[3] = {3, 2, 1};
        putarray(3, arr); // 输出3 2 1
    }
}

int main() {
    int n = getint(); // 获取数组长度
    int arr[10];
    
    // 测试输入合法性
    if (n <= 0 || n > 10) {
        putch(69); // 'E'
        putch(114); // 'r'
        putch(114); // 'r'
        putch(111); // 'o'
        putch(114); // 'r'
        return 0;
    }
    
    // 读取数组
    int i = 0;
    while (i < n) {
        arr[i] = getint();
        i = i + 1;
    }
    
    // 测试shadow
    test_shadow();
    
    // 排序前输出
    putch(66); // 'B'
    putch(101); // 'e'
    putch(102); // 'f'
    putch(111); // 'o'
    putch(114); // 'r'
    putch(101); // 'e'
    putch(58); // ':'
    putch(32); // 空格
    putarray(n, arr);
    
    // 执行排序
    quick_sort(arr, 0, n - 1);
    
    // 排序后输出
    putch(65); // 'A'
    putch(102); // 'f'
    putch(116); // 't'
    putch(101); // 'e'
    putch(114); // 'r'
    putch(58); // ':'
    putch(32); // 空格
    putarray(n, arr);
    
    return 0;
}