int std_abs(int value) {
    if (value < 0) {
        return -value;
    }
    return value;
}

int std_max(int left, int right) {
    if (left > right) {
        return left;
    }
    return right;
}

int std_min(int left, int right) {
    if (left < right) {
        return left;
    }
    return right;
}

int std_clamp(int value, int min_value, int max_value) {
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

int std_pow2(int exponent) {
    int result = 1;
    int i = 0;

    if (exponent < 0) {
        return 0;
    }

    while (i < exponent) {
        result = result * 2;
        i++;
    }

    return result;
}

int std_is_even(int value) {
    return (value % 2) == 0;
}

int std_is_odd(int value) {
    return (value % 2) != 0;
}

int std_add(int left, int right) {
    return left + right;
}

int std_sub(int left, int right) {
    return left - right;
}

int std_mul(int left, int right) {
    return left * right;
}

int std_div(int left, int right) {
    if (right == 0) {
        return 0;
    }
    return left / right;
}

int std_mod(int left, int right) {
    if (right == 0) {
        return 0;
    }
    return left % right;
}
