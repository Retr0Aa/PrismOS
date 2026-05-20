#include "command.h"

#include "display/console.h"
#include "platform/io.h"
#include "platform/system.h"

typedef void (*CommandHandler)(const char* arguments);

typedef struct {
    const char* name;
    const char* description;
    CommandHandler handler;
} Command;

static int string_equals(const char* left, const char* right) {
    while (*left != '\0' && *right != '\0') {
        if (*left != *right) {
            return 0;
        }

        left++;
        right++;
    }

    return *left == '\0' && *right == '\0';
}

static const char* skip_spaces(const char* text) {
    while (*text == ' ') {
        text++;
    }

    return text;
}

static void command_help(const char* arguments);
static void command_clear(const char* arguments);
static void command_echo(const char* arguments);
static void command_about(const char* arguments);
static void command_reboot(const char* arguments);
static void command_shutdown(const char* arguments);

// One registry powers execution and help output, so adding commands stays local.
static const Command commands[] = {
    {"help", "show this message", command_help},
    {"clear", "clear the screen", command_clear},
    {"echo", "print text after the command", command_echo},
    {"about", "show system information and version", command_about},
    {"reboot", "reboot the machine", command_reboot},
    {"shutdown", "shut down the machine", command_shutdown},
};

static const int command_count = (int)(sizeof(commands) / sizeof(commands[0]));

static const Command* command_find(const char* name) {
    for (int index = 0; index < command_count; index++) {
        if (string_equals(commands[index].name, name)) {
            return &commands[index];
        }
    }

    return 0;
}

static void command_help(const char* arguments) {
    (void)arguments;
    command_print_help();
}

static void command_clear(const char* arguments) {
    (void)arguments;
    console_clear();
}

static void command_echo(const char* arguments) {
    console_writeln(arguments);
}

static void command_about(const char* arguments) {
    (void)arguments;
    const uint32_t total_memory_kb = PRISMOS_TOTAL_MEMORY_KB;

    // Keep the version with the general system summary so users only need one command.
    console_writeln("PrismOS version 0.1 beta - early development stage");
    console_writeln("PrismOS is running in a 32-bit shell.");

    if (total_memory_kb == 0) {
        console_writeln("RAM: unknown");
        return;
    }

    // Print the largest readable unit without needing decimals.
    if (total_memory_kb >= 1024U * 1024U) {
        console_write("RAM: ");
        console_write_uint(total_memory_kb / (1024U * 1024U));
        console_writeln(" GB");
        return;
    }

    console_write("RAM: ");
    console_write_uint(total_memory_kb / 1024U);
    console_writeln(" MB");
}

static void command_reboot(const char* arguments) {
    (void)arguments;
    console_writeln("Rebooting...");
    reboot_system();
}

static void command_shutdown(const char* arguments) {
    (void)arguments;
    console_writeln("Shutting down...");
    shutdown_system();
}

void command_print_help(void) {
    console_writeln("Commands:");

    for (int index = 0; index < command_count; index++) {
        console_write("  ");
        console_write(commands[index].name);
        console_write(" - ");
        console_writeln(commands[index].description);
    }
}

void command_execute(const char* line) {
    char command_name[16];
    const char* arguments;
    int index = 0;

    line = skip_spaces(line);
    if (*line == '\0') {
        return;
    }

    while (line[index] != '\0' && line[index] != ' ' && index < (int)(sizeof(command_name) - 1)) {
        command_name[index] = line[index];
        index++;
    }

    command_name[index] = '\0';
    arguments = skip_spaces(line + index);

    const Command* command = command_find(command_name);
    if (command != 0) {
        command->handler(arguments);
        return;
    }

    console_write("Unknown command: ");
    console_writeln(command_name);
}