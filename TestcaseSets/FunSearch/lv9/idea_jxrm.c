/*
 * 测试多维数组、函数递归、短路求值、变量shadow和库函数综合应用
 * 功能：实现矩阵转置并验证结果
 */

// 全局常量测试
const int SIZE = 3;

// 矩阵转置函数
void transpose(int mat[][3], int result[][3]) {
    int i = 0;
    while (i < SIZE) {
        int j = 0;
        while (j < SIZE) {
            result[j][i] = mat[i][j];
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印矩阵
void print_matrix(int mat[][3]) {
    int i = 0;
    while (i < SIZE) {
        int j = 0;
        while (j < SIZE) {
            putint(mat[i][j]);
            putch(32); // 空格
            j = j + 1;
        }
        putch(10); // 换行
        i = i + 1;
    }
}

// 验证转置是否正确
int verify_transpose(int mat[][3], int trans[][3]) {
    int i = 0;
    while (i < SIZE) {
        int j = 0;
        while (j < SIZE) {
            // 测试短路求值
            if (mat[i][j] != trans[j][i] && (i != j || j != i)) {
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
    int transposed[3][3];
    
    // 输入矩阵
    int i = 0;
    while (i < SIZE) {
        int j = 0;
        while (j < SIZE) {
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
    
    transpose(matrix, transposed);
    
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
    putch(10);
    print_matrix(transposed);
    
    // 验证结果
    if (verify_transpose(matrix, transposed)) {
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