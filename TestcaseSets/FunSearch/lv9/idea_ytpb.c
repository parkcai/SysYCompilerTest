/*
 * 测试数组操作、逻辑运算短路和递归函数
 * 功能：计算汉诺塔移动步骤并验证数组操作
 */

// 递归解决汉诺塔问题
void hanoi(int n, int from, int to, int via) {
    if (n == 1) {
        putint(from);
        putch(45); // '-'
        putint(to);
        putch(32); // 空格
        return;
    }
    hanoi(n - 1, from, via, to);
    hanoi(1, from, to, via);
    hanoi(n - 1, via, to, from);
}

// 验证数组是否按升序排列
int is_sorted(int arr[], int size) {
    int i = 1;
    while (i < size) {
        // 短路测试：当arr[i-1]>arr[i]时立即返回0
        if (arr[i - 1] > arr[i]) {
            return 0;
        }
        i = i + 1;
    }
    return 1;
}

int main() {
    const int DISK_NUM = 3; // 汉诺塔盘数
    int towers[3][3] = {{1, 2, 3}, {0, 0, 0}, {0, 0, 0}}; // 初始化汉诺塔
    
    // 输出初始状态
    putarray(3, towers[0]);
    putarray(3, towers[1]);
    putarray(3, towers[2]);
    
    // 测试shadow
    {
        int DISK_NUM = 5;
        putint(DISK_NUM); // 应输出5
        putch(10); // 换行
    }
    
    // 解决汉诺塔问题
    hanoi(DISK_NUM, 1, 3, 2);
    putch(10); // 换行
    
    // 测试排序检查
    int test_arr[5];
    int size = getarray(test_arr);
    if (is_sorted(test_arr, size)) {
        putch(89); // 'Y'
    } else {
        putch(78); // 'N'
    }
    
    return 0;
}