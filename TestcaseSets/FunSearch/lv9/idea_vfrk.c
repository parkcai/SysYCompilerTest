/*
 * 测试多维数组初始化、逻辑运算短路求值、变量shadow和递归函数
 * 功能：计算并输出杨辉三角的前n行
 */

// 计算组合数C(n,k)的递归实现
int combination(int n, int k) {
    if (k == 0 || k == n) {
        return 1;
    }
    return combination(n - 1, k - 1) + combination(n - 1, k);
}

int main() {
    int n = getint(); // 获取要输出的行数
    int i = 0;
    
    // 测试短路求值
    if (n <= 0 || n > 10) {
        putint(-1); // 输入不合法
        return 0;
    }
    
    // 定义二维数组存储杨辉三角
    int triangle[10][10];
    
    while (i < n) {
        int j = 0;
        while (j <= i) {
            // 计算并存储组合数
            triangle[i][j] = combination(i, j);
            
            // 输出当前元素
            putint(triangle[i][j]);
            putch(32); // 空格
            
            j = j + 1;
        }
        putch(10); // 换行
        
        // 测试shadow
        {
            int i = 99;
            putint(i); // 应输出99
            putch(10);
        }
        
        i = i + 1;
    }
    
    return 0;
}