/*
 * 测试数组操作、函数递归和逻辑运算
 * 功能：实现汉诺塔问题并输出移动步骤
 */

// 汉诺塔移动步骤输出
void hanoi(int n, int from, int to, int via) {
    if (n == 1) {
        putint(from);
        putch(45);  // '-'
        putch(62);  // '>'
        putint(to);
        putch(10);  // 换行
        return;
    }
    hanoi(n - 1, from, via, to);
    hanoi(1, from, to, via);
    hanoi(n - 1, via, to, from);
}

int main() {
    // 测试shadow变量
    int n = getint();
    {
        int n = 3;
        putint(n);  // 应输出3
        putch(10);
    }
    
    // 测试短路特性
    if (n > 0 && n < 10) {
        hanoi(n, 1, 3, 2);  // 将n个盘子从柱1移动到柱3，柱2作为辅助
    } else {
        putch(33);  // 输出'!'表示无效输入
    }
    
    // 测试数组初始化
    int arr[3] = {1, 2, 3};
    putarray(3, arr);
    
    return 0;
}