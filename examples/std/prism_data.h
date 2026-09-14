string std_data_root() {
    return "/DATA";
}

int std_data_exists(string abs_path) {
    return file_exists(abs_path);
}

string std_data_read(string abs_path) {
    return file_read(abs_path);
}

int std_data_write(string abs_path, string text) {
    file_write(abs_path, text);
    return 0;
}

int std_data_append(string abs_path, string text) {
    file_append(abs_path, text);
    return 0;
}

/* Optional fixed data files for small apps that do not need dynamic paths. */
string std_data_slot_path(int slot) {
    if (slot == 1) {
        return "/DATA/APPDATA1.TXT";
    }
    if (slot == 2) {
        return "/DATA/APPDATA2.TXT";
    }
    if (slot == 3) {
        return "/DATA/APPDATA3.TXT";
    }
    if (slot == 4) {
        return "/DATA/APPDATA4.TXT";
    }
    return "/DATA/APPDATA5.TXT";
}
