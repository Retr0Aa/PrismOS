int std_print_line(string value) {
    print(value);
    return 0;
}

int std_print_number(int value) {
    print_int(value);
    return 0;
}

int std_print_char(char value) {
    print_char(value);
    return 0;
}

int std_print_color_line(int color, string value) {
    print_color(color, value);
    return 0;
}

string std_read_line() {
    return read_text();
}

string std_read_line_prompt(string prompt) {
    return read_text(prompt);
}

int std_read_int() {
    return input_int();
}

int std_input_length() {
    return input_len();
}

int std_input_is(string literal) {
    return input_eq(literal);
}

int std_print_input_line() {
    print_input();
    return 0;
}

int std_file_exists(string path) {
    return file_exists(path);
}

string std_file_read(string path) {
    return file_read(path);
}

int std_file_write(string path, string text) {
    file_write(path, text);
    return 0;
}

int std_file_append(string path, string text) {
    file_append(path, text);
    return 0;
}
