/*
 * 测试数组操作、递归函数、短路求值和变量shadow
 * 功能：实现快速排序并验证结果
 */

// 交换数组元素
void swap(int arr[], int i, int j) {
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

// 快速排序分区函数
int partition(int arr[], int low, int high) {
    int pivot = arr[high];
    int i = low - 1;
    int j = low;
    while (j < high) {
        if (arr[j] < pivot || (arr[j] == pivot && j % 2 == 0)) { // 测试短路求值
            i = i + 1;
            swap(arr, i, j);
        }
        j = j + 1;
    }
    swap(arr, i + 1, high);
    return i + 1;
}

// 快速排序递归实现
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
        if (i < size - 1) {
            putch(32); // 空格
        }
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    // 获取数组
    int size = getint();
    int arr[10];
    int i = 0;
    while (i < size) {
        arr[i] = getint();
        i = i + 1;
    }

    // 测试shadow变量
    {
        int size = 3;
        int arr[3] = {5, 2, 7};
        quick_sort(arr, 0, size - 1);
        print_array(arr, size); // 应输出排序后的数组
    }

    // 排序并输出结果
    quick_sort(arr, 0, size - 1);
    print_array(arr, size);

    return 0;
}