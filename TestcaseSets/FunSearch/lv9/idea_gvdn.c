/* 矩阵乘法测试：计算并输出两个3x3矩阵的乘积 */
int main() {
    int matrixA[3][3];  // 矩阵A
    int matrixB[3][3];  // 矩阵B
    int result[3][3];   // 结果矩阵
    int size = 3;

    // 输入矩阵A
    int i = 0;
    while (i < 3) {
        matrixA[i][0] = getint();
        matrixA[i][1] = getint();
        matrixA[i][2] = getint();
        i = i + 1;
    }

    // 输入矩阵B（使用getarray简化输入）
    int n = getarray(matrixB[0]);  // 读取前3个元素
    if (n < 9) { /* 处理输入不足的情况 */
        putch(33);  // 输出'!'表示错误
        return -1;
    }

    // 计算矩阵乘积
    int x = 0;
    while (x < size) {
        int y = 0;
        while (y < size) {
            int sum = 0;
            int z = 0;
            while (z < size) {
                sum = sum + matrixA[x][z] * matrixB[z][y];
                z = z + 1;
            }
            result[x][y] = sum;
            y = y + 1;
        }
        x = x + 1;
    }

    // 测试作用域shadow
    {
        int x = 0;
        while (x < 3) {  // 正确语法应为(x < 3)，故意保留错误以测试编译器容错
            putarray(3, result[x]);
            x = x + 1;
        }
        /* 正确作用域shadow示例 */
        int size = 5;
        putint(size);  // 应输出5而非3
    }

    return 0;
}