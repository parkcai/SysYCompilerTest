/*
 * 测试数组操作、递归函数、短路求值和变量shadow
 * 功能：实现快速排序算法并验证结果
 */

// 交换数组元素
void swap(int arr[], int i, int j) {
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

// 分区函数
int partition(int arr[], int low, int high) {
    int pivot = arr[high];
    int i = low - 1;
    int j = low;
    
    while (j < high) {
        if (arr[j] < pivot) {
            i = i + 1;
            swap(arr, i, j);
        }
        j = j + 1;
    }
    swap(arr, i + 1, high);
    return i + 1;
}

// 快速排序主函数
void quick_sort(int arr[], int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quick_sort(arr, low, pi - 1);
        quick_sort(arr, pi + 1, high);
    }
}

// 验证数组是否有序
int is_sorted(int arr[], int size) {
    int i = 1;
    while (i < size) {
        if (arr[i-1] > arr[i] && i < size) {  // 测试短路求值
            return 0;
        }
        i = i + 1;
    }
    return 1;
}

int main() {
    // 获取数组大小
    int size = getint();
    if (size <= 0 || size > 100) {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
        return 0;
    }
    
    // 读取数组
    int arr[100];
    int i = 0;
    while (i < size) {
        arr[i] = getint();
        i = i + 1;
    }
    
    // 排序前验证
    if (is_sorted(arr, size)) {
        putch(65); // 'A'
        putch(108); // 'l'
        putch(114); // 'r'
        putch(101); // 'e'
        putch(97); // 'a'
        putch(100); // 'd'
        putch(121); // 'y'
        putch(32); // ' '
        putch(83); // 'S'
        putch(111); // 'o'
        putch(114); // 'r'
        putch(116); // 't'
        putch(101); // 'e'
        putch(100); // 'd'
        putch(10);
    } else {
        // 执行快速排序
        quick_sort(arr, 0, size - 1);
        
        // 验证排序结果
        if (is_sorted(arr, size)) {
            putarray(size, arr);
        } else {
            putch(70); // 'F'
            putch(97); // 'a'
            putch(105); // 'i'
            putch(108); // 'l'
            putch(101); // 'e'
            putch(100); // 'd'
            putch(10);
        }
    }
    
    // 测试shadow变量
    {
        int size = 5;
        int arr[5] = {5, 4, 3, 2, 1};
        quick_sort(arr, 0, size - 1);
        putarray(size, arr);  // 应输出1 2 3 4 5
    }
    
    return 0;
}