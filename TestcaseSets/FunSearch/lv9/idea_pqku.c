/*
 * 测试多维数组初始化、函数递归、逻辑运算短路和变量shadow
 * 功能：计算并输出阶乘数列前N项
 */

// 递归计算阶乘
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

// 初始化数组为阶乘数列
void init_fact_array(int arr[], int size) {
    int i = 0;
    while (i < size) {
        arr[i] = factorial(i);
        i = i + 1;
    }
}

int main() {
    int n = getint(); // 获取阶乘数列项数
    int fact[20];     // 存储阶乘数列
    
    // 测试输入合法性
    if (n <= 0 || n > 20) {
        putch(33); // 输出'!'表示错误
        return -1;
    }
    
    // 初始化数组
    init_fact_array(fact, n);
    
    // 输出结果
    putarray(n, fact);
    
    // 测试shadow变量和逻辑运算
    {
        int n = 5;
        if (n > 0 && (fact[n-1] > 0)) {
            putch(10); // 换行
            putint(fact[n-1]); // 应输出fact[4]=24
        }
    }
    
    return 0;
}