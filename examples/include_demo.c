#include "lib/math_utils.h"
#include "lib/messages.h"

int main() {
    int x = 6;
    int y = 7;

    print(lib_banner());
    print("x + y =");
    print_int(lib_add(x, y));

    print("x * y =");
    print_int(lib_mul(x, y));

    print("square(x) =");
    print_int(lib_square(x));

    return 0;
}
