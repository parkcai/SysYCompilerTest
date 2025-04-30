/*
 * 测试多维数组、函数递归、逻辑运算短路特性、变量shadow
 * 功能：实现矩阵旋转90度并验证结果
 */

// 递归计算阶乘
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

// 矩阵旋转90度
void rotate_matrix(int mat[][3], int result[][3]) {
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            result[j][2 - i] = mat[i][j];
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印矩阵
void print_matrix(int mat[][3]) {
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            putint(mat[i][j]);
            putch(32); // 空格
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

// 验证旋转是否正确
int verify_rotate(int mat[][3], int rotated[][3]) {
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            // 测试短路求值
            if (mat[i][j] != rotated[2 - j][i] && (i != j || j != i)) {
                return 0;
            }
            j = j + 1;
        }
        i = i + 1;
    }
    return 1;
}

int main() {
    int matrix[3][3];
    int rotated[3][3];
    
    // 输入矩阵
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            matrix[i][j] = getint();
            j = j + 1;
        }
        i = i + 1;
    }
    
    // 测试shadow
    {
        int i = 99;
        putint(i); // 应输出99
        putch(10);
    }
    
    rotate_matrix(matrix, rotated);
    
    // 输出结果
    putch(79); // 'O'
    putch(114); // 'r'
    putch(105); // 'i'
    putch(103); // 'g'
    putch(105); // 'i'
    putch(110); // 'n'
    putch(97); // 'a'
    putch(108); // 'l'
    putch(10);
    print_matrix(matrix);
    
    putch(82); // 'R'
    putch(111); // 'o'
    putch(116); // 't'
    putch(97); // 'a'
    putch(116); // 't'
    putch(101); // 'e'
    putch(100); // 'd'
    putch(10);
    print_matrix(rotated);
    
    // 验证结果
    if (verify_rotate(matrix, rotated)) {
        putch(86); // 'V'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
    } else {
        putch(73); // 'I'
        putch(110); // 'n'
        putch(118); // 'v'
        putch(97); // 'a'
        putch(108); // 'l'
        putch(105); // 'i'
        putch(100); // 'd'
    }
    
    return 0;
}