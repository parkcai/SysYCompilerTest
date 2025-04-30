/*
 * 测试数组操作、函数递归、短路求值和变量shadow
 * 功能：实现快速排序并输出排序结果
 */

// 交换两个元素
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
        if (a[j] < pivot || (a[j] == pivot && (i < 0 || a[i] < pivot))) {
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

int main() {
    int size = getint();
    int arr[100];
    
    // 读取数组
    int i = 0;
    while (i < size) {
        arr[i] = getint();
        i = i + 1;
    }
    
    // 测试shadow变量
    {
        int size = 5;
        int arr[5] = {5, 4, 3, 2, 1};
        quick_sort(arr, 0, size - 1);
        print_array(arr, size); // 输出1 2 3 4 5
    }
    
    // 排序并输出结果
    quick_sort(arr, 0, size - 1);
    print_array(arr, size);
    
    // 测试短路特性
    if (size > 0 && arr[0] < arr[size - 1]) {
        putch(84); // 'T'
    } else {
        putch(70); // 'F'
    }
    putch(10);
    
    return 0;
}