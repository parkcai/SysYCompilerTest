/*
 * 测试一维数组、逻辑运算短路、递归函数和变量shadow
 * 功能：实现一个简单的计算器，支持加减乘除操作
 */

// 递归计算阶乘
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

// 计算两数之和
int add(int a, int b) {
    return a + b;
}

// 计算两数之差
int subtract(int a, int b) {
    return a - b;
}

// 计算两数之积
int multiply(int a, int b) {
    return a * b;
}

// 计算两数之商
int divide(int a, int b) {
    if (b == 0) {
        putint(-1); // 除数为0
        return 0;
    }
    return a / b;
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
    // 获取输入的数组
    int arr[5];
    int n = getarray(arr);
    
    // 测试输入合法性
    if (n <= 0 || n > 5) {
        putint(-1); // 非法输入
        return 0;
    }
    
    // 计算数组元素的阶乘
    int fact_arr[5] = {0};
    int i = 0;
    while (i < n) {
        fact_arr[i] = factorial(arr[i]);
        i = i + 1;
    }
    
    // 打印阶乘数组
    print_array(fact_arr, n);
    
    // 测试逻辑短路
    if (n > 2 && arr[0] > 0 && arr[1] > 0) {
        putint(add(arr[0], arr[1])); // 输出两数之和
    } else {
        putint(0);
    }
    
    // 测试变量shadow
    {
        int i = 999;
        putint(i); // 应输出999
        putch(10);
    }
    
    return 0;
}