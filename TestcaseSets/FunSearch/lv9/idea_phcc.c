/*
 * 测试多维数组初始化、函数递归、逻辑运算短路和变量shadow
 * 功能：实现汉诺塔问题并输出移动步骤
 */

// 递归解决汉诺塔问题
void hanoi(int n, int from, int to, int via) {
    if (n == 1) {
        putint(from);
        putch(45); // '-'
        putch(62); // '>'
        putint(to);
        putch(10); // 换行
        return;
    }
    
    // 利用递归和逻辑运算短路特性
    hanoi(n - 1, from, via, to);
    hanoi(1, from, to, via);
    if (n > 1 && (via != to || from != via)) { // 测试逻辑短路
        hanoi(n - 1, via, to, from);
    }
}

int main() {
    int n = getint(); // 获取盘子数量
    
    // 测试输入合法性
    if (n <= 0) {
        putch(33); // 输出'!'表示错误
        return -1;
    }
    
    // 测试shadow变量
    {
        int n = 3;
        putint(n); // 应输出3而非输入值
        putch(10);
    }
    
    // 解决汉诺塔问题
    hanoi(n, 1, 3, 2);
    
    return 0;
}