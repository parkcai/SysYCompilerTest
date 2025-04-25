/*
 * 测试数组操作、函数递归、逻辑运算短路和变量shadow
 * 功能：计算并输出汉诺塔问题的移动步骤
 */

// 递归解决汉诺塔问题
void hanoi(int n, int from, int to, int via) {
    if (n == 1) {
        putint(from);
        putch(45);  // '-'
        putch(62);  // '>'
        putint(to);
        putch(10);  // 换行
        return;
    }
    
    // 利用短路特性优化递归
    if (n > 1 && n < 10) {
        hanoi(n - 1, from, via, to);
        hanoi(1, from, to, via);
        hanoi(n - 1, via, to, from);
    }
}

int main() {
    int n = getint();
    
    // 测试shadow
    {
        int n = 3;  // shadow输入n
        putint(n);  // 输出3
        putch(10);
    }
    
    // 测试逻辑短路
    if (n <= 0 || n > 8) {
        putint(-1);  // 输入不合法
        return 0;
    }
    
    // 测试数组和库函数
    int steps[100];
    int count = 0;
    
    // 开始计算汉诺塔
    hanoi(n, 1, 3, 2);
    
    return 0;
}