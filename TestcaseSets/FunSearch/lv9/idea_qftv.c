/*
 * 测试多维数组、递归函数、逻辑运算和变量shadow
 * 功能：实现一个简单的斐波那契数列生成器，并计算前n个斐波那契数的和
 */

// 计算斐波那契数
int fibonacci(int n) {
    if (n == 0) {
        return 0;
    } else if (n == 1) {
        return 1;
    } else {
        return fibonacci(n - 1) + fibonacci(n - 2);
    }
}

// 计算前n个斐波那契数的和
int sum_fibonacci(int n) {
    int sum = 0;
    int i = 0;
    while (i < n) {
        sum = sum + fibonacci(i);
        i = i + 1;
    }
    return sum;
}

// 打印斐波那契数列
void print_fibonacci(int n) {
    int i = 0;
    while (i < n) {
        putint(fibonacci(i));
        if (i != n - 1) {
            putch(32); // 空格
        }
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    // 测试变量shadow
    int n = getint();
    {
        int n = 5;
        putint(n); // 应输出5
        putch(10);
    }
    
    // 测试短路特性
    if (n > 0 && (100 / n > 0)) {
        print_fibonacci(n);
        putint(sum_fibonacci(n));
    } else {
        putch(33); // 输出'!'表示无效输入
    }
    
    return 0;
}