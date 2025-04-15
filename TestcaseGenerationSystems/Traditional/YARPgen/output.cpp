#include <stdio.h>
#include <algorithm>
unsigned long long int seed = 0;


void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

signed char var_2 = (signed char)80;
signed char var_4 = (signed char)43;
signed char var_6 = (signed char)-7;
unsigned char var_9 = (unsigned char)115;
int zero = 0;
unsigned char var_13 = (unsigned char)184;
unsigned short var_14 = (unsigned short)38204;
void test(signed char var_2, signed char var_4, signed char var_6, unsigned char var_9, int zero) {
    var_13 = ((/* implicit */unsigned char) ((((/* implicit */int) var_6)) > (((/* implicit */int) std::max((((((/* implicit */bool) 6958869835664190384ULL)) && (((/* implicit */bool) var_4)))), ((!(((/* implicit */bool) var_2)))))))));
    var_14 = ((/* implicit */unsigned short) var_9);
}
void init() {
}

void checksum() {
    hash(&seed, var_13);
    hash(&seed, var_14);
}


int main() {
    init();
    test(var_2, var_4, var_6, var_9, zero);
    checksum();
    printf("%llu\n", seed);
}
