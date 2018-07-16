int main() {
    int N, a, b, x, y, z = 0;

    int i = 0;

    while (i < N) {
      x = -((x + 2 * y * 3 * z) % 3);
      y = (3 * z + 2 * y) % x;
      z++;
    }
    return 0;
}
