/*
 * 测试多维数组初始化、函数递归调用、短路逻辑运算和变量shadow
 * 功能：计算并输出斐波那契数列的前n项，并验证某些值是否为质数
 */

// 计算斐波那契数列
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    } else {
        return fibonacci(n - 1) + fibonacci(n - 2);
    }
}

// 判断一个数是否为质数
int is_prime(int n) {
    if (n <= 1) {
        return 0; // 0表示不是质数，1表示是质数
    }
    int i = 2;
    while (i * i <= n) {
        if (n % i == 0) {
            return 0;
        }
        i = i + 1;
    }
    return 1;
}

// 打印斐波那契数列的前n项，并验证某些值是否为质数
void print_fibonacci_and_check_primes(int n) {
    int i = 0;
    while (i < n) {
        int fib = fibonacci(i);
        putint(fib);
        putch(32); // 空格
        if (is_prime(fib)) {
            putch(80); // 'P' 表示质数
        } else {
            putch(78); // 'N' 表示非质数
        }
        putch(32); // 空格
        i = i + 1;
    }
    putch(10); // 换行
}

int main() {
    // 测试变量shadow
    int n = getint();
    {
        int n = 10;
        putint(n); // 应输出10而非输入值
        putch(10);
    }

    // 测试短路逻辑运算
    if (n > 0 && n < 20) {
        putch(84); // 'T'
        putch(10); // 换行
    } else {
        putch(70); // 'F'
        putch(10); // 换行
    }

    // 测试多维数组初始化
    int fib_array[20];
    int i = 0;
    while (i < n) {
        fib_array[i] = fibonacci(i);
        i = i + 1;
    }

    print_fibonacci_and_check_primes(n);

    return 0;
}