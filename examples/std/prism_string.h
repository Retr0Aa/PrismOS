int std_str_len(string value) {
    return string_len(value);
}

int std_str_eq(string left, string right) {
    return string_eq(left, right);
}

int std_str_is_empty(string value) {
    return string_len(value) == 0;
}

int std_str_not_empty(string value) {
    return string_len(value) != 0;
}

int std_str_len_eq(string value, int expected) {
    return string_len(value) == expected;
}

int std_str_len_gt(string value, int threshold) {
    return string_len(value) > threshold;
}

int std_str_len_lt(string value, int threshold) {
    return string_len(value) < threshold;
}

int std_str_len_between(string value, int min_len, int max_len) {
    int len = string_len(value);
    return len >= min_len && len <= max_len;
}
