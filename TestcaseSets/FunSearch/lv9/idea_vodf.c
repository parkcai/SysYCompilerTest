/*
 * 测试数组操作、函数递归和逻辑运算
 * 功能：实现汉诺塔问题并验证结果
 */

const int DISK_NUM = 3;  // 定义常量盘子数

// 移动盘子
void move_disk(int n, int from, int to, int via) {
    if (n == 1) {
        putint(from);
        putch(45);  // '-'
        putch(62);  // '>'
        putint(to);
        putch(10);  // 换行
        return;
    }
    move_disk(n - 1, from, via, to);
    move_disk(1, from, to, via);
    move_disk(n - 1, via, to, from);
}

// 验证移动步数是否正确
int verify_steps(int n) {
    if (n == 1) return 1;
    return 2 * verify_steps(n - 1) + 1;
}

int main() {
    int steps = 0;
    
    // shadow测试
    {
        const int DISK_NUM = 2;  // shadow全局常量
        steps = verify_steps(DISK_NUM);
        putint(steps);  // 输出3
        putch(10);      // 换行
    }
    
    // 实际移动盘子
    move_disk(DISK_NUM, 1, 3, 2);
    
    // 验证总步数
    steps = verify_steps(DISK_NUM);
    putint(steps);  // 输出7
    putch(10);      // 换行
    
    return 0;
}