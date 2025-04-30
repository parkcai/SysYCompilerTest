/*
 * 测试数组操作、逻辑运算短路特性、递归函数和变量shadow
 * 功能：实现选择排序算法并验证结果
 */

// 交换数组元素
void swap(int arr[], int i, int j) {
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

// 寻找数组中最小值的索引
int find_min_index(int arr[], int start, int end) {
    int min_index = start;
    int i = start + 1;
    while (i <= end) {
        if (arr[i] < arr[min_index]) {
            min_index = i;
        }
        i = i + 1;
    }
    return min_index;
}

// 选择排序主函数
void selection_sort(int arr[], int size) {
    int i = 0;
    while (i < size) {
        int min_index = find_min_index(arr, i, size - 1);
        swap(arr, i, min_index);
        i = i + 1;
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
    // 测试数组排序
    int arr[10] = {5, 8, 1, 3, 9, 6, 2, 7, 4, 0};
    const int size = 10;
    
    // 测试短路求值
    if (size > 0 && arr[0] == 5) {
        putch(84); // 'T'
    } else {
        putch(70); // 'F'
    }
    putch(10);
    
    // 排序前
    print_array(arr, size);
    
    // 排序操作
    selection_sort(arr, size);
    
    // 排序后
    print_array(arr, size);
    
    // 测试shadow
    {
        int size = 5;
        int arr[5] = {9, 7, 5, 3, 1};
        selection_sort(arr, size);
        print_array(arr, size);
    }
    
    return 0;
}