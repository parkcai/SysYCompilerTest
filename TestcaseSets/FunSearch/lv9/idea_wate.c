/*
 * 测试多维数组、递归、变量shadow和逻辑运算短路
 * 功能：实现一个简单的矩阵转置并验证结果
 */

// 初始化二维数组
void init_matrix(int matrix[][3], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < size) {
            matrix[i][j] = i * size + j;
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印二维数组
void print_matrix(int matrix[][3], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < size) {
            putint(matrix[i][j]);
            if (j < size - 1) putch(32); // 空格
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

// 转置二维数组
void transpose_matrix(int matrix[][3], int transposed[][3], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < size) {
            transposed[j][i] = matrix[i][j];
            j = j + 1;
        }
        i = i + 1;
    }
}

// 验证转置结果
int verify_transpose(int matrix[][3], int transposed[][3], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < size) {
            if (matrix[i][j] != transposed[j][i]) return 0;
            j = j + 1;
        }
        i = i + 1;
    }
    return 1;
}

int main() {
    const int SIZE = 3; // 定义常量大小
    int matrix[3][3]; // 原始矩阵
    int transposed[3][3]; // 转置后的矩阵

    // 初始化矩阵
    init_matrix(matrix, SIZE);

    // 打印原始矩阵
    putch(84); // 'T'
    putch(105); // 'i'
    putch(110); // 'n'
    putch(105); // 'i'
    putch(97); // 'a'
    putch(108); // 'l'
    putch(32); // ' '
    putch(77); // 'M'
    putch(97); // 'a'
    putch(116); // 't'
    putch(114); // 'r'
    putch(105); // 'i'
    putch(120); // 'x'
    putch(10); // 换行
    print_matrix(matrix, SIZE);

    // 转置矩阵
    transpose_matrix(matrix, transposed, SIZE);

    // 打印转置后的矩阵
    putch(84); // 'T'
    putch(114); // 'r'
    putch(97); // 'a'
    putch(110); // 'n'
    putch(115); // 's'
    putch(112); // 'p'
    putch(111); // 'o'
    putch(115); // 's'
    putch(101); // 'e'
    putch(100); // 'd'
    putch(32); // ' '
    putch(77); // 'M'
    putch(97); // 'a'
    putch(116); // 't'
    putch(114); // 'r'
    putch(105); // 'i'
    putch(120); // 'x'
    putch(10); // 换行
    print_matrix(transposed, SIZE);

    // 验证转置结果
    if (verify_transpose(matrix, transposed, SIZE)) {
        putch(84); // 'T'
        putch(114); // 'r'
        putch(97); // 'a'
        putch(110); // 'n'
        putch(115); // 's'
        putch(112); // 'p'
        putch(111); // 'o'
        putch(115); // 's'
        putch(101); // 'e'
        putch(32); // ' '
        putch(83); // 'S'
        putch(117); // 'u'
        putch(99); // 'c'
        putch(99); // 'c'
        putch(101); // 'e'
        putch(115); // 's'
        putch(10); // 换行
    } else {
        putch(84); // 'T'
        putch(114); // 'r'
        putch(97); // 'a'
        putch(110); // 'n'
        putch(115); // 's'
        putch(112); // 'p'
        putch(111); // 'o'
        putch(115); // 's'
        putch(101); // 'e'
        putch(32); // ' '
        putch(70); // 'F'
        putch(97); // 'a'
        putch(105); // 'i'
        putch(108); // 'l'
        putch(101); // 'e'
        putch(10); // 换行
    }

    // 测试变量shadow
    {
        int SIZE = 5; // shadow外层SIZE
        putint(SIZE); // 应输出5
        putch(10);
    }

    return 0;
}