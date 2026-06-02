#include "command.h"

#include "comport/comport.h"
#include "debug/log.h"
#include "display/console.h"
#include "platform/io.h"
#include "platform/system.h"
#include "filesystem/vfs.h"
#include "apps/app_manager.h"
#include "apps/prismcc_runtime.h"

#define COMMAND_PATH_CAPACITY 128
#define COMMAND_TOKEN_CAPACITY 64
#define COMMAND_TEXT_CAPACITY 512

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

static unsigned int string_length(const char* text) {
    unsigned int len = 0;

    while (text[len] != '\0') {
        len++;
    }

    return len;
}

static void command_help(const char* arguments);
static void command_clear(const char* arguments);
static void command_echo(const char* arguments);
static void command_about(const char* arguments);
static void command_reboot(const char* arguments);
static void command_shutdown(const char* arguments);
static void command_comport(const char* arguments);
static void command_ls(const char* arguments);
static void command_cd(const char* arguments);
static void command_touch(const char* arguments);
static void command_mkdir(const char* arguments);
static void command_rm(const char* arguments);
static void command_rmdir(const char* arguments);
static void command_delete(const char* arguments);
static void command_cat(const char* arguments);
static void command_write(const char* arguments);
static void command_append(const char* arguments);
static void command_edit(const char* arguments);
static void command_app_run(const char* arguments);
static void command_cc(const char* arguments);

static char command_cwd[COMMAND_PATH_CAPACITY] = "/";

typedef struct {
    int count;
} LsCommandContext;

static int command_ls_visit(const vfs_entry_t* entry, void* context) {
    LsCommandContext* ls_context = (LsCommandContext*)context;

    console_write(entry->is_directory ? "[D] " : "[F] ");
    console_write(entry->name);
    if (!entry->is_directory) {
        console_write(" (");
        console_write_uint(entry->size);
        console_write(" bytes)");
    }
    console_write_char('\n');

    ls_context->count++;
    return 0;
}

static void copy_string_limited(char* destination, const char* source, unsigned int capacity) {
    unsigned int index = 0;

    if (capacity == 0U) {
        return;
    }

    while (source[index] != '\0' && index < (capacity - 1U)) {
        destination[index] = source[index];
        index++;
    }

    destination[index] = '\0';
}

static int parse_token(const char* input, char* token, unsigned int token_capacity, const char** remainder) {
    const char* cursor = skip_spaces(input);
    unsigned int length = 0;

    if (*cursor == '\0') {
        return -1;
    }

    while (*cursor != '\0' && *cursor != ' ') {
        if (length >= (token_capacity - 1U)) {
            return -1;
        }

        token[length++] = *cursor;
        cursor++;
    }

    token[length] = '\0';
    *remainder = skip_spaces(cursor);
    return 0;
}

static int resolve_to_absolute_path(const char* input_path, char* output_path, unsigned int output_capacity) {
    return vfs_normalize_path(command_cwd, input_path, output_path, output_capacity);
}

// One registry powers execution and help output, so adding commands stays local.
static const Command commands[] = {
    {"help", "show this message", command_help},
    {"clear", "clear the screen", command_clear},
    {"echo", "print text after the command", command_echo},
    {"about", "show system information and version", command_about},
    {"reboot", "reboot the machine", command_reboot},
    {"shutdown", "shut down the machine", command_shutdown},
    {"comport", "send text to COM1 serial port", command_comport},
    {"ls", "list directory contents", command_ls},
    {"cd", "change current directory", command_cd},
    {"touch", "create an empty file", command_touch},
    {"mkdir", "create a directory", command_mkdir},
    {"rm", "remove a file", command_rm},
    {"rmdir", "remove an empty directory", command_rmdir},
    {"delete", "remove file or empty directory", command_delete},
    {"cat", "print file contents", command_cat},
    {"write", "overwrite file with text", command_write},
    {"append", "append text to file", command_append},
    {"edit", "open text editor application", command_edit},
    {"app-run", "run app package path [args]", command_app_run},
    {"cc", "compile subset C source to app", command_cc},
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
    DEBUG_LOG("help command executed");
    command_print_help();
}

static void command_clear(const char* arguments) {
    (void)arguments;
    DEBUG_LOG("clear command executed");
    console_clear();
}

static void command_echo(const char* arguments) {
    DEBUG_LOG("echo command executed");
    console_writeln(arguments);
}

static void command_about(const char* arguments) {
    (void)arguments;
    DEBUG_LOG("about command executed");
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
    WARNINIG_LOG("reboot command executed");
    console_writeln("Rebooting...");
    reboot_system();
}

static void command_shutdown(const char* arguments) {
    (void)arguments;
    WARNINIG_LOG("shutdown command executed");
    console_writeln("Shutting down...");
    shutdown_system();
}

static void command_comport(const char* arguments) {
    if (*arguments == '\0') {
        WARNINIG_LOG("comport command missing arguments");
        console_writeln("Usage: comport <text>");
        return;
    }

    if (comport_init() != 0) {
        ERROR_LOG("COM1 initialization failed in comport command");
        console_writeln("COM1 init failed");
        return;
    }

    // Forward user-provided payload directly to COM1 for live diagnostics.
    DEBUG_LOG("comport command writing data to COM1");
    comport_write_string(arguments);
    comport_write_char('\n');

    console_write("Sent ");
    console_write_uint(string_length(arguments));
    console_writeln(" bytes to COM1");
}

static void command_ls(const char* arguments) {
    char target[COMMAND_PATH_CAPACITY];
    const char* path = skip_spaces(arguments);
    LsCommandContext context = {0};

    if (*path == '\0') {
        path = command_cwd;
    }

    if (resolve_to_absolute_path(path, target, sizeof(target)) != 0) {
        console_writeln("Invalid path");
        return;
    }

    if (vfs_list(target, command_ls_visit, &context) != 0) {
        ERROR_LOG("ls failed to read directory");
        console_writeln("ls failed");
        return;
    }

    if (context.count == 0) {
        console_writeln("(empty)");
    }
}

static void command_cd(const char* arguments) {
    char token[COMMAND_TOKEN_CAPACITY];
    char absolute[COMMAND_PATH_CAPACITY];
    const char* remainder = 0;
    int is_dir = 0;

    if (parse_token(arguments, token, sizeof(token), &remainder) != 0 || *remainder != '\0') {
        console_writeln("Usage: cd <path>");
        return;
    }

    if (resolve_to_absolute_path(token, absolute, sizeof(absolute)) != 0) {
        console_writeln("Invalid path");
        return;
    }

    if (vfs_path_is_dir(absolute, &is_dir) != 0 || !is_dir) {
        console_writeln("Directory not found");
        return;
    }

    copy_string_limited(command_cwd, absolute, sizeof(command_cwd));
}

static void command_touch(const char* arguments) {
    char token[COMMAND_TOKEN_CAPACITY];
    char absolute[COMMAND_PATH_CAPACITY];
    const char* remainder = 0;

    if (parse_token(arguments, token, sizeof(token), &remainder) != 0 || *remainder != '\0') {
        console_writeln("Usage: touch <path>");
        return;
    }

    if (resolve_to_absolute_path(token, absolute, sizeof(absolute)) != 0 || vfs_touch(absolute) != 0) {
        console_writeln("touch failed");
        return;
    }
}

static void command_mkdir(const char* arguments) {
    char token[COMMAND_TOKEN_CAPACITY];
    char absolute[COMMAND_PATH_CAPACITY];
    const char* remainder = 0;

    if (parse_token(arguments, token, sizeof(token), &remainder) != 0 || *remainder != '\0') {
        console_writeln("Usage: mkdir <path>");
        return;
    }

    if (resolve_to_absolute_path(token, absolute, sizeof(absolute)) != 0 || vfs_mkdir(absolute) != 0) {
        console_writeln("mkdir failed");
        return;
    }
}

static void command_rm(const char* arguments) {
    char token[COMMAND_TOKEN_CAPACITY];
    char absolute[COMMAND_PATH_CAPACITY];
    const char* remainder = 0;

    if (parse_token(arguments, token, sizeof(token), &remainder) != 0 || *remainder != '\0') {
        console_writeln("Usage: rm <path>");
        return;
    }

    if (resolve_to_absolute_path(token, absolute, sizeof(absolute)) != 0 || vfs_rm(absolute) != 0) {
        console_writeln("rm failed");
    }
}

static void command_rmdir(const char* arguments) {
    char token[COMMAND_TOKEN_CAPACITY];
    char absolute[COMMAND_PATH_CAPACITY];
    const char* remainder = 0;

    if (parse_token(arguments, token, sizeof(token), &remainder) != 0 || *remainder != '\0') {
        console_writeln("Usage: rmdir <path>");
        return;
    }

    if (resolve_to_absolute_path(token, absolute, sizeof(absolute)) != 0 || vfs_rmdir(absolute) != 0) {
        console_writeln("rmdir failed (directory must be empty)");
    }
}

static void command_delete(const char* arguments) {
    char token[COMMAND_TOKEN_CAPACITY];
    char absolute[COMMAND_PATH_CAPACITY];
    const char* remainder = 0;
    int is_dir = 0;

    if (parse_token(arguments, token, sizeof(token), &remainder) != 0 || *remainder != '\0') {
        console_writeln("Usage: delete <path>");
        return;
    }

    if (resolve_to_absolute_path(token, absolute, sizeof(absolute)) != 0) {
        console_writeln("Invalid path");
        return;
    }

    if (vfs_path_is_dir(absolute, &is_dir) != 0) {
        console_writeln("Path not found");
        return;
    }

    if (is_dir) {
        if (vfs_rmdir(absolute) != 0) {
            console_writeln("delete failed (directory must be empty)");
        }
    } else if (vfs_rm(absolute) != 0) {
        console_writeln("delete failed");
    }
}

static void command_cat(const char* arguments) {
    char token[COMMAND_TOKEN_CAPACITY];
    char absolute[COMMAND_PATH_CAPACITY];
    char content[COMMAND_TEXT_CAPACITY + 1];
    uint32_t size = 0;
    const char* remainder = 0;

    if (parse_token(arguments, token, sizeof(token), &remainder) != 0 || *remainder != '\0') {
        console_writeln("Usage: cat <path>");
        return;
    }

    if (resolve_to_absolute_path(token, absolute, sizeof(absolute)) != 0) {
        console_writeln("Invalid path");
        return;
    }

    if (vfs_read_file(absolute, content, sizeof(content), &size) != 0) {
        console_writeln("cat failed");
        return;
    }

    if (size == 0U) {
        console_writeln("(empty)");
        return;
    }

    console_write(content);
    if (content[size - 1U] != '\n') {
        console_write_char('\n');
    }
}

static void command_write(const char* arguments) {
    char token[COMMAND_TOKEN_CAPACITY];
    char absolute[COMMAND_PATH_CAPACITY];
    const char* text = 0;

    if (parse_token(arguments, token, sizeof(token), &text) != 0 || *text == '\0') {
        console_writeln("Usage: write <path> <text>");
        return;
    }

    if (resolve_to_absolute_path(token, absolute, sizeof(absolute)) != 0) {
        console_writeln("Invalid path");
        return;
    }

    if (vfs_write_file(absolute, text, (uint32_t)string_length(text), 0) != 0) {
        console_writeln("write failed");
    }
}

static void command_append(const char* arguments) {
    char token[COMMAND_TOKEN_CAPACITY];
    char absolute[COMMAND_PATH_CAPACITY];
    const char* text = 0;

    if (parse_token(arguments, token, sizeof(token), &text) != 0 || *text == '\0') {
        console_writeln("Usage: append <path> <text>");
        return;
    }

    if (resolve_to_absolute_path(token, absolute, sizeof(absolute)) != 0) {
        console_writeln("Invalid path");
        return;
    }

    if (vfs_write_file(absolute, text, (uint32_t)string_length(text), 1) != 0) {
        console_writeln("append failed");
    }
}

static void command_edit(const char* arguments) {
    char token[COMMAND_TOKEN_CAPACITY];
    char absolute[COMMAND_PATH_CAPACITY];
    const char* remainder = 0;

    if (parse_token(arguments, token, sizeof(token), &remainder) != 0 || *remainder != '\0') {
        console_writeln("Usage: edit <path>");
        return;
    }

    if (resolve_to_absolute_path(token, absolute, sizeof(absolute)) != 0) {
        console_writeln("Invalid path");
        return;
    }

    if (app_manager_run_editor(absolute) != 0) {
        console_writeln("editor failed");
    }
}

static void command_app_run(const char* arguments) {
    char token[COMMAND_TOKEN_CAPACITY];
    char absolute[COMMAND_PATH_CAPACITY];
    const char* remainder = 0;

    if (parse_token(arguments, token, sizeof(token), &remainder) != 0) {
        console_writeln("Usage: app-run <path> [args]");
        return;
    }

    if (resolve_to_absolute_path(token, absolute, sizeof(absolute)) != 0) {
        console_writeln("Invalid path");
        return;
    }

    if (app_manager_run_path(absolute, remainder) != 0) {
        console_writeln("app execution failed");
    }
}

static void command_cc(const char* arguments) {
    char input_token[COMMAND_TOKEN_CAPACITY];
    char output_token[COMMAND_TOKEN_CAPACITY];
    char input_absolute[COMMAND_PATH_CAPACITY];
    char output_absolute[COMMAND_PATH_CAPACITY];
    char error[96];
    const char* remainder = 0;

    if (parse_token(arguments, input_token, sizeof(input_token), &remainder) != 0) {
        console_writeln("Usage: cc <input.c> <output.app>");
        return;
    }

    if (parse_token(remainder, output_token, sizeof(output_token), &remainder) != 0 || *remainder != '\0') {
        console_writeln("Usage: cc <input.c> <output.app>");
        return;
    }

    if (resolve_to_absolute_path(input_token, input_absolute, sizeof(input_absolute)) != 0) {
        console_writeln("Invalid input path");
        return;
    }

    if (resolve_to_absolute_path(output_token, output_absolute, sizeof(output_absolute)) != 0) {
        console_writeln("Invalid output path");
        return;
    }

    if (prismcc_compile_file(input_absolute, output_absolute, error, sizeof(error)) != 0) {
        console_write("cc failed: ");
        console_writeln(error[0] == '\0' ? "compile error" : error);
        return;
    }

    console_write("Compiled app: ");
    console_writeln(output_absolute);
}

const char* command_get_cwd(void) {
    return command_cwd;
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
        DEBUG_LOG("empty command ignored");
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
        // Emit one trace per successful command before handing over control.
        DEBUG_LOG("command resolved and dispatched");
        command->handler(arguments);
        return;
    }

    WARNINIG_LOG("unknown command entered");
    console_write("Unknown command: ");
    console_writeln(command_name);
}