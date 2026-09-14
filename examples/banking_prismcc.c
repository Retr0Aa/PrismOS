#include "std/prism_data.h"

string username_path(int slot) {
    if (slot == 1) {
        return "/DATA/BANK_U1.TXT";
    }
    if (slot == 2) {
        return "/DATA/BANK_U2.TXT";
    }
    if (slot == 3) {
        return "/DATA/BANK_U3.TXT";
    }
    if (slot == 4) {
        return "/DATA/BANK_U4.TXT";
    }
    return "/DATA/BANK_U5.TXT";
}

string password_path(int slot) {
    if (slot == 1) {
        return "/DATA/BANK_P1.TXT";
    }
    if (slot == 2) {
        return "/DATA/BANK_P2.TXT";
    }
    if (slot == 3) {
        return "/DATA/BANK_P3.TXT";
    }
    if (slot == 4) {
        return "/DATA/BANK_P4.TXT";
    }
    return "/DATA/BANK_P5.TXT";
}

string deposit_log_path(int slot) {
    if (slot == 1) {
        return "/DATA/BANK_D1.LOG";
    }
    if (slot == 2) {
        return "/DATA/BANK_D2.LOG";
    }
    if (slot == 3) {
        return "/DATA/BANK_D3.LOG";
    }
    if (slot == 4) {
        return "/DATA/BANK_D4.LOG";
    }
    return "/DATA/BANK_D5.LOG";
}

string withdraw_log_path(int slot) {
    if (slot == 1) {
        return "/DATA/BANK_W1.LOG";
    }
    if (slot == 2) {
        return "/DATA/BANK_W2.LOG";
    }
    if (slot == 3) {
        return "/DATA/BANK_W3.LOG";
    }
    if (slot == 4) {
        return "/DATA/BANK_W4.LOG";
    }
    return "/DATA/BANK_W5.LOG";
}

int choose_slot() {
    int slot;

    print("Choose account slot (1-5):");
    slot = input_int();

    if (slot < 1) {
        print("Invalid slot");
        return 0;
    }

    if (slot > 5) {
        print("Invalid slot");
        return 0;
    }

    return slot;
}

int account_exists(int slot) {
    return std_data_exists(username_path(slot));
}

int current_balance(int slot) {
    string deposits;
    string withdrawals;

    deposits = std_data_read(deposit_log_path(slot));
    withdrawals = std_data_read(withdraw_log_path(slot));

    return string_len(deposits) - string_len(withdrawals);
}

void append_units(string path, int amount) {
    int i = 0;

    while (i < amount) {
        std_data_append(path, ".");
        i++;
    }
}

void create_account() {
    int slot;
    string user;
    string pass;

    slot = choose_slot();
    if (slot == 0) {
        return;
    }

    if (account_exists(slot) == 1) {
        print("Slot already has an account");
        return;
    }

    user = read_text("Username: ");
    pass = read_text("Password: ");

    if (string_len(user) == 0) {
        print("Username is required");
        return;
    }

    if (string_len(pass) == 0) {
        print("Password is required");
        return;
    }

    std_data_write(username_path(slot), user);
    std_data_write(password_path(slot), pass);
    std_data_write(deposit_log_path(slot), "");
    std_data_write(withdraw_log_path(slot), "");

    print("Account created");
}

int login() {
    int slot;
    string user;
    string pass;
    string saved_user;
    string saved_pass;

    slot = choose_slot();
    if (slot == 0) {
        return 0;
    }

    if (account_exists(slot) == 0) {
        print("No account in that slot");
        return 0;
    }

    user = read_text("Username: ");
    pass = read_text("Password: ");

    saved_user = std_data_read(username_path(slot));
    saved_pass = std_data_read(password_path(slot));

    if (string_eq(user, saved_user) == 0) {
        print("Invalid credentials");
        return 0;
    }

    if (string_eq(pass, saved_pass) == 0) {
        print("Invalid credentials");
        return 0;
    }

    return slot;
}

void account_menu(int slot) {
    int running = 1;

    while (running == 1) {
        int action;
        int amount;
        int balance;

        print("--- Account Menu ---");
        print("1) Show balance");
        print("2) Deposit");
        print("3) Withdraw");
        print("4) Logout");

        action = input_int();

        if (action == 1) {
            balance = current_balance(slot);
            print("Balance:");
            print(balance);
        } else if (action == 2) {
            print("Deposit amount:");
            amount = input_int();

            if (amount <= 0) {
                print("Amount must be positive");
            } else {
                append_units(deposit_log_path(slot), amount);
                print("Deposit successful");
            }
        } else if (action == 3) {
            print("Withdraw amount:");
            amount = input_int();

            if (amount <= 0) {
                print("Amount must be positive");
            } else {
                balance = current_balance(slot);
                if (amount > balance) {
                    print("Insufficient funds");
                } else {
                    append_units(withdraw_log_path(slot), amount);
                    print("Withdrawal successful");
                }
            }
        } else if (action == 4) {
            running = 0;
        } else {
            print("Unknown option");
        }
    }
}

int main() {
    int running = 1;

    while (running == 1) {
        int action;
        int slot;

        print("=== Prism Banking App ===");
        print("1) Create account");
        print("2) Login");
        print("3) Exit");

        action = input_int();

        if (action == 1) {
            create_account();
        } else if (action == 2) {
            slot = login();
            if (slot == 0) {
                print("Login failed");
            } else {
                account_menu(slot);
            }
        } else if (action == 3) {
            running = 0;
        } else {
            print("Unknown option");
        }
    }

    return 0;
}
