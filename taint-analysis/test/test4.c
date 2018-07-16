#include <stdio.h>

int main() {

    int a, b, sink, source = 2000;

    /* read a from input*/
    a = source;
    b = a;
    if (a > b){
      b = b + 1;
    } else {
      b = 1;
    }

    sink = b;
}
