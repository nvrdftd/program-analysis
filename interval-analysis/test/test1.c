int main() {
    int x = 3, a, b = -5;
    if (x + b - a <= 0) {
        x = 3 + b;
    } else if (a > 0) {
        x = b - 10;
    } else {
        b = 3;
        x = b + x;
    }
    return x;
}
