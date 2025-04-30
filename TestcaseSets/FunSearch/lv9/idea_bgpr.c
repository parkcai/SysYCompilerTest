/*
 * 测试数组操作、函数调用、逻辑运算和变量shadow
 * 功能：实现一个简单的学生成绩管理系统，支持录入成绩、计算平均分、查询最低分和输出所有成绩
 */

// 计算平均分
int average(int scores[], int size) {
    int total = 0;
    int i = 0;
    while (i < size) {
        total = total + scores[i];
        i = i + 1;
    }
    return total / size;
}

// 查询最低分
int min_score(int scores[], int size) {
    int min = scores[0];
    int i = 1;
    while (i < size) {
        if (scores[i] < min) {
            min = scores[i];
        }
        i = i + 1;
    }
    return min;
}

// 输出所有成绩
void print_scores(int scores[], int size) {
    putarray(size, scores);
}

int main() {
    // 初始化成绩数组
    int scores[5];
    int i = 0;
    while (i < 5) {
        scores[i] = getint();
        i = i + 1;
    }

    // 输出所有成绩
    print_scores(scores, 5);

    // 测试shadow变量
    {
        int i = 42;  // shadow外层循环中的i
        putint(i);   // 应输出42
        putch(10);   // 换行
    }

    // 计算并输出平均分
    int avg = average(scores, 5);
    putint(avg);
    putch(10);   // 换行

    // 查询并输出最低分
    int lowest = min_score(scores, 5);
    putint(lowest);
    putch(10);   // 换行

    return 0;
}