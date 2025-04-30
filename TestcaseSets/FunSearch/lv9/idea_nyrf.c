/*
 * 测试递归函数、多维数组、短路求值和变量shadow
 * 功能：实现快速排序并验证结果
 */

// 交换两个整数
void swap(int a[], int i, int j) {
    int temp = a[i];
    a[i] = a[j];
    a[j] = temp;
}

// 快速排序分区函数
int partition(int a[], int low, int high) {
    int pivot = a[high];
    int i = low - 1;
    int j = low;
    while (j < high) {
        if (a[j] < pivot) {
            i = i + 1;
            swap(a, i, j);
        }
        j = j + 1;
    }
    swap(a, i + 1, high);
    return i + 1;
}

// 快速排序递归实现
void quick_sort(int a[], int low, int high) {
    if (low < high) {
        int pi = partition(a, low, high);
        quick_sort(a, low, pi - 1);
        quick_sort(a, pi + 1, high);
    }
}

// 打印数组
void print_array(int a[], int size) {
    int i = 0;
    while (i < size) {
        putint(a[i]);
        if (i != size - 1) {
            putch(32); // 空格
        }
        i = i + 1;
    }
    putch(10); // 换行
}

// 验证数组是否有序
int is_sorted(int a[], int size) {
    int i = 0;
    while (i < size - 1) {
        if (a[i] > a[i + 1]) {
            return 0; // 未排序
        }
        i = i + 1;
    }
    return 1; // 已排序
}

int main() {
    int size = getint();
    int arr[100];
    
    // 获取数组输入
    int i = 0;
    while (i < size) {
        arr[i] = getint();
        i = i + 1;
    }
    
    // 测试shadow变量
    {
        int size = 3;
        int arr[3] = {3, 1, 2};
        quick_sort(arr, 0, size - 1);
        print_array(arr, size); // 应输出1 2 3
    }
    
    // 排序主数组
    quick_sort(arr, 0, size - 1);
    
    // 输出排序结果
    print_array(arr, size);
    
    // 验证排序结果
    if (is_sorted(arr, size) == 1) {
        putch(84); // 'T' 表示正确
    } else {
        putch(70); // 'F' 表示错误
    }
    putch(10);
    
    return 0;
}