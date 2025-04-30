/*
 * 测试多维数组、递归函数、逻辑运算短路特性以及变量shadow
 * 功能：实现并验证汉诺塔问题的解法
 */

// 递归解决汉诺塔问题
void hanoi(int n, int from, int to, int aux) {
    if (n == 1) {
        putint(from);
        putch(32); // 空格
        putint(to);
        putch(10); // 换行
    } else {
        hanoi(n - 1, from, aux, to);
        putint(from);
        putch(32); // 空格
        putint(to);
        putch(10); // 换行
        hanoi(n - 1, aux, to, from);
    }
}

// 验证汉诺塔步骤是否正确
int check_hanoi_steps(int n, int steps[][2]) {
    int i = 0;
    while (i < n) {
        if (steps[i][0] == 0 || steps[i][1] == 0) {
            return 0; // 无效步骤
        }
        i = i + 1;
    }
    return 1; // 所有步骤有效
}

int main() {
    const int N_DISKS = 3; // 汉诺塔盘子数量
    int steps[7][2]; // 存储步骤的数组
    
    // 测试shadow
    {
        int N_DISKS = 4; // shadow全局常量
        putint(N_DISKS); // 应输出4
        putch(10); // 换行
    }
    
    // 解决汉诺塔问题
    hanoi(N_DISKS, 1, 3, 2);
    
    // 读取用户输入的步骤
    int i = 0;
    while (i < 7) {
        steps[i][0] = getint();
        steps[i][1] = getint();
        i = i + 1;
    }
    
    // 验证步骤
    if (check_hanoi_steps(7, steps)) {
        putch(84); // 'T'
    } else {
        putch(70); // 'F'
    }
    putch(10);
    
    return 0;
}