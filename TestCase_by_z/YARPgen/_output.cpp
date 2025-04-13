#include <stdio.h>

int seed = 0;

void hash(int *seed, int v) {
    *seed ^= v + 0x9e3779b9 + ((*seed) << 6) + ((*seed) >> 2);
}

int var_2 = 80;
int var_4 = 43;
int var_6 = -7;
int var_9 = 115;
int zero = 0;
int var_13 = 184;
int var_14 = 38204;
int tmp1 = 0;
int tmp2 = 0;
int ans = 0;

void test(signed char var_2, signed char var_4, signed char var_6, unsigned char var_9, int zero) {
    tmp1 = (((( 6958869 != 0 )) && (( var_4 != 0))));
    tmp2 = ((!(( var_2 != 0))));
    var_13 = ( ((( var_6)) > (((tmp1 > tmp2) * tmp1 + (tmp1 >= tmp2) * tmp2))));
    var_14 = ( var_9);
}

void init() {
}

void checksum() {
    hash(&seed, var_13);
    hash(&seed, var_14);
    ans = 0-seed;
}

int main() {
    init();
    test(var_2, var_4, var_6, var_9, zero);
    checksum();
    //printf("%d\n", ans);
    //1919631517
    return ans;
}