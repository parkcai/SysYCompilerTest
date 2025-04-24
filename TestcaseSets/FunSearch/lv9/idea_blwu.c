/*
 * 统计数组元素奇偶性并验证作用域嵌套
 */

const int INIT_DATA[5] = {2, 3, 5, 7, 11}; // 全局常量数组初始化

// 奇偶统计函数
int analyze(int arr[], int len) {
    int even_cnt = 0; // 外层作用域计数器
    int i = 0;
    
    while (i < len) {
        {   // 内层作用域开始
            int even_cnt = 0; // shadow外层计数器
            if (arr[i] % 2 == 0) {
                even_cnt = 1; // 修改内层变量
            }
            putint(even_cnt); // 输出临时结果
        }   // 内层作用域结束
        
        if (arr[i] % 2 != 0) {
            even_cnt = even_cnt + 1; // 修改外层变量
        }
        i = i + 1;
    }
    
    // 多级条件语句测试
    if (even_cnt > 2) {
        return 1;
    } else if (even_cnt > 0) {
        return 0;
    } else {
        return -1;
    }
}

int main() {
    int a = getint();
    int b = getint();
    int data[5] = {INIT_DATA[0], a, b, 0}; // 混合初始化
    int result = analyze(data, 4);
    
    /* 验证逻辑运算短路特性 */
    int flag = 0;
    while (flag < 5) {
        if (flag == 3 && (result != 0 || data[1] < 10)) {
            putch(33); // 输出'!'的ASCII码
            break;
        }
        flag = flag + 1;
    }
    
    return result;
}