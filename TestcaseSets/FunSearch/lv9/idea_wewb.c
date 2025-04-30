/*
 * 测试递归函数、多维数组、短路求值和变量shadow
 * 功能：实现汉诺塔问题的求解并验证移动步数
 */

// 全局变量记录移动步数
int steps = 0;

// 汉诺塔移动函数
void hanoi(int n, int from[], int to[], int aux[], int size) {
    if (n == 1) {
        // 测试短路求值
        if (size > 0 && from[size-1] != 0) {
            to[size-1] = from[size-1];
            from[size-1] = 0;
            steps = steps + 1;
        }
        return;
    }
    
    hanoi(n - 1, from, aux, to, size);
    
    // 移动最底下的盘子
    int i = 0;
    while (i < size && from[i] == 0) {
        i = i + 1;
    }
    if (i < size) {
        int j = 0;
        while (j < size && to[j] == 0) {
            j = j + 1;
        }
        if (j == 0 || (j > 0 && to[j-1] > from[i])) {
            to[j] = from[i];
            from[i] = 0;
            steps = steps + 1;
        }
    }
    
    // 测试变量shadow
    {
        int i = 100;
        if (i > 50 || n < 0) { // 测试短路求值
            putint(i); // 应输出100
            putch(32);
        }
    }
    
    hanoi(n - 1, aux, to, from, size);
}

// 验证汉诺塔步数是否正确
int verify_steps(int n) {
    if (n <= 0) {
        return 0;
    }
    int expected = 1;
    int i = 1;
    while (i < n) {
        expected = 2 * expected + 1;
        i = i + 1;
    }
    return expected;
}

int main() {
    int n = getint(); // 获取盘子数量
    const int size = 10; // 柱子大小
    
    // 初始化三根柱子
    int A[10] = {0};
    int B[10] = {0};
    int C[10] = {0};
    
    // 初始化A柱子
    int i = 0;
    while (i < n) {
        A[i] = n - i;
        i = i + 1;
    }
    
    // 解决汉诺塔问题
    hanoi(n, A, C, B, size);
    
    // 输出移动步数
    putint(steps);
    putch(10);
    
    // 验证步数是否正确
    if (steps == verify_steps(n)) {
        putch(84); // 'T'
    } else {
        putch(70); // 'F'
    }
    
    return 0;
}