/*
 * 统计输入字符串中各字母出现次数（不区分大小写）
 * 测试点：作用域shadow、多维数组访问、逻辑运算短路特性
 */

const int ALPHABET_SIZE = 26;  // 全局常量初始化

// 初始化字母统计数组
void init_counter(int cnt[]) {
    int i = 0;
    while (i < ALPHABET_SIZE) {
        cnt[i] = 0;
        i = i + 1;
    }
}

// 递归处理字符并统计
int process_char(int c, int cnt[]) {
    // 递归终止条件：非字母字符
    if (!((c >= 65 && c <= 90) || (c >= 97 && c <= 122))) {
        return 0;
    }
    
    // 转换为小写并统计
    int idx = (c - 65) % 32;  // 利用ASCII码特性
    cnt[idx] = cnt[idx] + 1;
    
    // 递归处理下一个字符
    return process_char(getch(), cnt) + 1;
}

int main() {
    int counter[26];  // 字母统计数组
    int total = 0;
    init_counter(counter);
    
    putch(62);  // 输出'>'提示符
    putch(32);  // 输出空格
    
    // 处理输入并统计
    total = process_char(getch(), counter);
    
    /* 测试作用域shadow */
    {
        int total = 999;  // shadow外层total
        putint(total);     // 应输出999
        putch(10);         // 换行
    }
    
    // 输出统计结果
    int i = 0;
    while (i < ALPHABET_SIZE) {
        if (counter[i] > 0 && (i < 5 || i > 20)) {  // 测试逻辑运算符短路
            putint(97 + i);   // 输出字母
            putch(58);        // 输出':'
            putint(counter[i]);
            putch(32);
        }
        i = i + 1;
    }
    
    return total % 256;  // 返回处理字符数取模
}