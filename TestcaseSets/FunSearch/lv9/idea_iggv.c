/*
 * 测试多维数组、函数递归、短路逻辑和变量shadow
 * 功能：实现快速排序算法并验证排序结果
 */

// 交换两个元素的值
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
        if (arr[j] < pivot) {
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

// 测试短路逻辑和变量shadow
void test_logic_shadow(int size) {
    int arr[5] = {1, 3, 5, 7, 9};
    // 测试短路逻辑
    if (size > 0 && size <= 5 && arr[size-1] % 2 == 1) {
        putint(arr[size-1]); // 应输出对应奇数
    }
    
    // 测试变量shadow
    {
        int size = 3;
        int arr[3] = {2, 4, 6};
        putarray(size, arr); // 应输出2 4 6
    }
}

int main() {
    // 获取输入数组
    int data[10];
    int size = getarray(data);
    
    // 输出原始数组
    putarray(size, data);
    
    // 测试逻辑和shadow
    test_logic_shadow(size);
    
    // 快速排序
    quick_sort(data, 0, size-1);
    
    // 输出排序后数组
    putarray(size, data);
    
    return 0;
}