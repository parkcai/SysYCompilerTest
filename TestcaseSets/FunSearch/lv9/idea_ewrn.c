/*
 * 测试数组参数传递、嵌套循环、逻辑运算和变量shadow
 * 功能：实现矩阵乘法并验证结果
 */

// 矩阵乘法函数
void matrix_multiply(int a[][3], int b[][3], int result[][3], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < size) {
            result[i][j] = 0;
            int k = 0;
            while (k < size) {
                result[i][j] = result[i][j] + a[i][k] * b[k][j];
                k = k + 1;
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// 验证矩阵乘法结果
int verify_result(int expected[][3], int actual[][3], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < size) {
            if (expected[i][j] != actual[i][j]) {
                return 0; // 验证失败
            }
            j = j + 1;
        }
        i = i + 1;
    }
    return 1; // 验证成功
}

int main() {
    // 定义测试矩阵
    const int size = 3;
    int a[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    int b[3][3] = {{9, 8, 7}, {6, 5, 4}, {3, 2, 1}};
    int expected[3][3] = {{30, 24, 18}, {84, 69, 54}, {138, 114, 90}};
    int result[3][3];
    
    // 计算矩阵乘法
    matrix_multiply(a, b, result, size);
    
    // 验证结果
    if (verify_result(expected, result, size)) {
        putch(89); // 'Y'
        putch(10);  // 换行
    } else {
        putch(78); // 'N'
        putch(10);  // 换行
    }
    
    // 测试shadow和逻辑短路
    {
        int size = 2;
        if (size > 0 && size < 3) {
            putint(size); // 应输出2
            putch(10);    // 换行
        }
    }
    
    return 0;
}