/*
 * 测试数组初始化、函数递归、逻辑运算短路和变量shadow
 * 功能：实现快速排序并输出排序结果
 */

// 交换数组元素
void swap(int arr[], int i, int j) {
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

// 快速排序分区
int partition(int arr[], int low, int high) {
    int pivot = arr[high];
    int i = low - 1;
    int j = low;
    while (j < high) {
        // 利用短路特性防止数组越界
        if (j >= 0 && arr[j] < pivot) {
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

// 打印数组
void print_array(int arr[], int size) {
    int i = 0;
    while (i < size) {
        putint(arr[i]);
        putch(32); // 空格
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    int arr[10];
    int n = 10;
    
    // 初始化数组
    int i = 0;
    while (i < n) {
        arr[i] = getint();
        i = i + 1;
    }
    
    // 测试shadow变量
    {
        int i = 5;
        putint(i); // 应输出5
        putch(10);
    }
    
    // 排序并输出
    quick_sort(arr, 0, n - 1);
    print_array(arr, n);
    
    // 测试短路特性
    if (getint() != 0 && getint() != 0) {
        putch(84); // 'T'
    } else {
        putch(70); // 'F'
    }
    
    return 0;
}