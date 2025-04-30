/*
 * 测试多维数组初始化、函数递归、短路求值和变量shadow
 * 功能：实现矩阵转置和行列式计算
 */

// 全局常量矩阵
const int IDENTITY[2][2] = {{1, 0}, {0, 1}};

// 递归计算行列式
int determinant(int mat[][2], int size) {
    if (size == 1) return mat[0][0];
    if (size == 2) return mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];
    return 0; // 仅支持2x2矩阵
}

// 矩阵转置
void transpose(int mat[][2], int size) {
    int i = 0;
    while (i < size) {
        int j = i + 1;
        while (j < size) {
            int temp = mat[i][j];
            mat[i][j] = mat[j][i];
            mat[j][i] = temp;
            j = j + 1;
        }
        i = i + 1;
    }
}

// 打印矩阵
void print_matrix(int mat[][2], int size) {
    int i = 0;
    while (i < size) {
        int j = 0;
        while (j < size) {
            putint(mat[i][j]);
            if (j < size - 1) putch(32);
            j = j + 1;
        }
        putch(10);
        i = i + 1;
    }
}

int main() {
    // 测试shadow
    {
        const int IDENTITY = 42;
        putint(IDENTITY); // 应输出42
        putch(10);
    }

    // 获取输入矩阵
    int mat[2][2];
    int i = 0;
    while (i < 2) {
        int j = 0;
        while (j < 2) {
            mat[i][j] = getint();
            j = j + 1;
        }
        i = i + 1;
    }

    // 测试短路求值
    if (determinant(mat, 2) != 0 || mat[0][0] == 0) {
        transpose(mat, 2);
        print_matrix(mat, 2);
    } else {
        putch(68); // 'D'
        putch(69); // 'E'
        putch(84); // 'T'
        putch(61); // '='
        putch(48); // '0'
        putch(10);
    }

    return 0;
}