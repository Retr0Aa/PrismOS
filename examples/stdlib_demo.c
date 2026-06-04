#include "std/prism_string.h"
#include "std/prism_math.h"
#include "std/prism_io.h"

int main() {
    string name;
    int a = 15;
    int b = 4;

    std_print_line("PrismCC stdlib demo");

    std_print_line("Math:");
    std_print_number(std_add(a, b));
    std_print_number(std_sub(a, b));
    std_print_number(std_mul(a, b));
    std_print_number(std_div(a, b));
    std_print_number(std_mod(a, b));
    std_print_number(std_abs(-42));
    std_print_number(std_pow2(8));

    std_print_line("String + IO:");
    name = std_read_line_prompt("Enter your name: ");
    std_print_line("Length:");
    std_print_number(std_str_len(name));

    if (std_str_not_empty(name)) {
        std_print_line("Hello:");
        std_print_line(name);
    }

    if (std_str_eq(name, "admin")) {
        std_print_color_line(10, "Welcome, admin");
    }

    return 0;
}
