/*
 * 测试数组操作、逻辑运算和函数调用
 * 实现冒泡排序算法并验证
 */

// 全局常量数组
const int INIT_DATA[5] = {5, 3, 4, 1, 2};

// 交换数组元素
void swap(int arr[], int i, int j) {
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

// 冒泡排序实现
void bubble_sort(int arr[], int len) {
    int i = 0;
    while (i < len - 1) {
        int j = 0;
        while (j < len - i - 1) {
            if (arr[j] > arr[j + 1]) {
                swap(arr, j, j + 1);  // 调用交换函数
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印数组
void print_array(int arr[], int len) {
    int i = 0;
    while (i < len) {
        putint(arr[i]);
        if (i != len - 1) {
            putch(32);  // 输出空格分隔
        }
        i = i + 1;
    }
    putch(10);  // 换行
}

int main() {
    int arr[5];
    int i = 0;
    
    // 初始化数组
    while (i < 5) {
        arr[i] = INIT_DATA[i];  // 使用全局常量初始化
        i = i + 1;
    }
    
    putch(83);  // 'S'
    putch(111); // 'o'
    putch(114); // 'r'
    putch(116); // 't'
    putch(105); // 'i'
    putch(110); // 'n'
    putch(103); // 'g'
    putch(58);  // ':'
    putch(10);  // 换行
    
    print_array(arr, 5);  // 排序前输出
    bubble_sort(arr, 5);  // 调用排序函数
    print_array(arr, 5);  // 排序后输出
    
    // 测试逻辑运算
    int a = getint();
    int b = getint();
    if (a > 0 && b > 0 || a == b) {
        putint(1);
    } else {
        putint(0);
    }
    
    return 0;
}