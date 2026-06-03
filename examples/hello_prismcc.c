int main() {
    string path = "/notes.txt";
    string msg = "hello from prismcc file io";
    string loaded;
    int exists_before;
    int exists_after;

    exists_before = file_exists(path);
    print("Exists before write:");
    print(exists_before);

    file_write(path, msg);
    file_append(path, " + append");
    loaded = file_read(path);

    exists_after = file_exists(path);
    print("Exists after write:");
    print(exists_after);

    print("Loaded file:");
    print(loaded);

    return 0;
}
