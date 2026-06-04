struct Person {
    int age;
    string name;
};

int main() {
    struct Person p;

    p.age = 42;
    p.name = "Trinity";

    print("Name:");
    print(p.name);

    print("Age:");
    print_int(p.age);

    p.age++;
    print("Age after ++:");
    print_int(p.age);

    return 0;
}
