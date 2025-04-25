/*
 * 测试复杂表达式、数组操作和短路求值
 * 功能：实现简单的计算器功能，支持加减乘除和模运算
 */

// 计算两个数的运算结果
int calculate(int a, int b, int op) {
    if (op == 0) return a + b;
    if (op == 1) return a - b;
    if (op == 2) return a * b;
    if (op == 3) {
        if (b != 0) return a / b;  // 测试短路求值
        else return -1;
    }
    if (op == 4) {
        if (b != 0) return a % b;   // 测试短路求值
        else return -1;
    }
    return 0;
}

// 打印运算结果
void print_result(int result) {
    if (result == -1) {
        putch(69); // 'E'
        putch(114); // 'r'
        putch(114); // 'r'
        putch(111); // 'o'
        putch(114); // 'r'
    } else {
        putint(result);
    }
    putch(10); // 换行
}

int main() {
    // 测试数组初始化和访问
    int nums[2];
    nums[0] = getint();
    nums[1] = getint();
    int op = getint();
    
    // 测试复杂表达式和短路求值
    int result = calculate(nums[0], nums[1], op);
    print_result(result);
    
    // 测试shadow变量
    {
        int nums[3] = {10, 5, 2};
        int i = 0;
        while (i < 3) {
            int result = calculate(nums[i], nums[(i+1)%3], i);
            print_result(result);
            i = i + 1;
        }
    }
    
    return 0;
}