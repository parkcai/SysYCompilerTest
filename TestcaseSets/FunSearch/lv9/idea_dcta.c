/*
 * 测试数组操作、函数递归和逻辑运算
 * 功能：实现汉诺塔问题并验证输出
 */

// 汉诺塔递归解法
void hanoi(int n, int from, int to, int via) {
    if (n == 1) {
        putint(from);
        putch(45); // '-'
        putch(62); // '>'
        putint(to);
        putch(10); // 换行
        return;
    }
    hanoi(n - 1, from, via, to);
    hanoi(1, from, to, via);
    hanoi(n - 1, via, to, from);
}

// 验证数组操作
int verify_array(int arr[], int size) {
    int i = 0;
    int sum = 0;
    while (i < size) {
        sum = sum + arr[i];
        i = i + 1;
    }
    return sum;
}

int main() {
    const int DISKS = 3;
    int steps[7] = {0};
    int count = 0;
    
    // 测试shadow
    {
        int DISKS = 2;
        hanoi(DISKS, 1, 3, 2);
    }
    
    // 测试汉诺塔
    hanoi(DISKS, 1, 3, 2);
    
    // 测试数组操作
    steps[0] = 1;
    steps[1] = 2;
    steps[2] = 3;
    
    // 测试短路逻辑
    if (verify_array(steps, 3) == 6 || DISKS > 5) {
        putch(84); // 'T'
    } else {
        putch(70); // 'F'
    }
    putch(10);
    
    return 0;
}