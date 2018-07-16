int main() {
    int N, a, b, x = 10, y = 2, z = 0;

    int i = 0;

    if (i < N) {
      x = -((x + 2 * y * 3 * z) % 3);
      y = (3 * x + 2 * y + z) % 11;
      z++;
    }
    return 0;
}
