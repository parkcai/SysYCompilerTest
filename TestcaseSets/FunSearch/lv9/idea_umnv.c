/*
 * 本测试用例旨在验证SysY语言中的数组操作、函数定义与调用、逻辑运算短路特性以及变量作用域shadow行为。
 * 功能：实现一个简单的整数排序算法（冒泡排序），并通过用户输入的整数数组进行排序，最后输出排序后的数组。
 * 同时，该程序还包括了对一些逻辑运算短路特性的简单应用。
 */

// 冒泡排序
void bubble_sort(int arr[], int n) {
    int i = 0;
    while (i < n - 1) {
        int j = 0;
        while (j < n - 1 - i) {
            if (arr[j] > arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印整数数组
void print_ints(int arr[], int count) {
    int i = 0;
    while (i < count) {
        putint(arr[i]);
        if (i != count - 1) {
            putch(32); // 输出空格
        }
        i = i + 1;
    }
    putch(10); // 输出换行符
}

int main() {
    int input[10];
    int size = getarray(input); // 获取用户输入的整数数组
    
    // 测试变量shadow
    {
        int size = 5; // 影子变量，不影响外部size
        putint(size);  // 输出影子变量值
        putch(10);
    }
    
    // 排序数组
    bubble_sort(input, size);
    
    // 利用逻辑运算的短路特性避免不必要的计算或访问未初始化数据
    if (size > 0 && size <= 10) {
        print_ints(input, size);
    } else {
        putch(33);  // 如果输入无效，则输出'!'
    }

    return 0;
}