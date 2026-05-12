#include "shell.h"

#include "commands/command.h"
#include "display/console.h"
#include "input/keyboard.h"

#define SHELL_PROMPT "PrismOS> "
#define SHELL_HISTORY_SIZE 16
#define SHELL_MAX_INPUT 96

typedef struct {
    char line[SHELL_MAX_INPUT + 1];
    int length;
    int cursor;
    char draft[SHELL_MAX_INPUT + 1];
    int draft_length;
    char history[SHELL_HISTORY_SIZE][SHELL_MAX_INPUT + 1];
    int history_count;
    int history_index;
    int prompt_row;
} ShellState;

static ShellState shell = {{0}, 0, 0, {0}, 0, {{0}}, 0, -1, 0};

static int string_length(const char* text) {
    int length = 0;

    while (text[length] != '\0') {
        length++;
    }

    return length;
}

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

static void shell_copy_line(char* destination, const char* source, int max_length) {
    int index = 0;

    while (source[index] != '\0' && index < max_length) {
        destination[index] = source[index];
        index++;
    }

    destination[index] = '\0';
}

// Preserve the current command line so history navigation can come back to it.
static void shell_reset_history_navigation(void) {
    shell.history_index = -1;
    shell.draft_length = 0;
    shell.draft[0] = '\0';
}

static void shell_set_line(const char* text) {
    shell_copy_line(shell.line, text, SHELL_MAX_INPUT);
    shell.length = string_length(shell.line);
    shell.cursor = shell.length;
}

static void shell_render_input(void) {
    console_set_cursor(0, shell.prompt_row);
    console_clear_row(shell.prompt_row);
    console_set_cursor(0, shell.prompt_row);
    console_write(SHELL_PROMPT);
    console_write(shell.line);
    console_set_cursor((int)string_length(SHELL_PROMPT) + shell.cursor, shell.prompt_row);
}

static void shell_show_prompt(void) {
    shell.prompt_row = console_get_y();
    shell.line[0] = '\0';
    shell.length = 0;
    shell.cursor = 0;
    shell_reset_history_navigation();
    console_clear_row(shell.prompt_row);
    console_set_cursor(0, shell.prompt_row);
    console_write(SHELL_PROMPT);
    console_set_cursor((int)string_length(SHELL_PROMPT), shell.prompt_row);
}

static void shell_refresh_line(void) {
    shell_render_input();
}

static void shell_insert_char(char c) {
    if (shell.length >= SHELL_MAX_INPUT) {
        return;
    }

    for (int index = shell.length; index > shell.cursor; index--) {
        shell.line[index] = shell.line[index - 1];
    }

    shell.line[shell.cursor] = c;
    shell.length++;
    shell.cursor++;
    shell.line[shell.length] = '\0';
    shell_refresh_line();
}

static void shell_backspace(void) {
    if (shell.cursor <= 0) {
        return;
    }

    for (int index = shell.cursor - 1; index < shell.length - 1; index++) {
        shell.line[index] = shell.line[index + 1];
    }

    shell.length--;
    shell.cursor--;
    shell.line[shell.length] = '\0';
    shell_refresh_line();
}

static void shell_delete_at_cursor(void) {
    if (shell.cursor >= shell.length) {
        return;
    }

    for (int index = shell.cursor; index < shell.length - 1; index++) {
        shell.line[index] = shell.line[index + 1];
    }

    shell.length--;
    shell.line[shell.length] = '\0';
    shell_refresh_line();
}

static void shell_move_cursor_left(void) {
    if (shell.cursor > 0) {
        shell.cursor--;
        shell_refresh_line();
    }
}

static void shell_move_cursor_right(void) {
    if (shell.cursor < shell.length) {
        shell.cursor++;
        shell_refresh_line();
    }
}

static void shell_move_cursor_home(void) {
    shell.cursor = 0;
    shell_refresh_line();
}

static void shell_move_cursor_end(void) {
    shell.cursor = shell.length;
    shell_refresh_line();
}

static void shell_store_history(const char* line) {
    if (line[0] == '\0') {
        return;
    }

    if (shell.history_count > 0 && string_equals(shell.history[shell.history_count - 1], line)) {
        return;
    }

    if (shell.history_count < SHELL_HISTORY_SIZE) {
        shell_copy_line(shell.history[shell.history_count], line, SHELL_MAX_INPUT);
        shell.history_count++;
        return;
    }

    for (int index = 1; index < SHELL_HISTORY_SIZE; index++) {
        shell_copy_line(shell.history[index - 1], shell.history[index], SHELL_MAX_INPUT);
    }

    shell_copy_line(shell.history[SHELL_HISTORY_SIZE - 1], line, SHELL_MAX_INPUT);
}

static void shell_history_up(void) {
    if (shell.history_count == 0) {
        return;
    }

    if (shell.history_index == -1) {
        shell_copy_line(shell.draft, shell.line, SHELL_MAX_INPUT);
        shell.draft_length = shell.length;
        shell.history_index = shell.history_count - 1;
    } else if (shell.history_index > 0) {
        shell.history_index--;
    }

    shell_set_line(shell.history[shell.history_index]);
    shell_refresh_line();
}

static void shell_history_down(void) {
    if (shell.history_index == -1) {
        return;
    }

    if (shell.history_index < shell.history_count - 1) {
        shell.history_index++;
        shell_set_line(shell.history[shell.history_index]);
    } else {
        shell_set_line(shell.draft);
        shell.length = shell.draft_length;
        shell.cursor = shell.length;
        shell.history_index = -1;
    }

    shell_refresh_line();
}

static void shell_handle_event(KeyEvent event) {
    if (event.type == KEY_EVENT_CHARACTER) {
        if (event.character == '\t') {
            shell_insert_char(' ');
            shell_insert_char(' ');
            shell_insert_char(' ');
            shell_insert_char(' ');
            return;
        }

        shell_insert_char(event.character);
        return;
    }

    switch (event.type) {
        case KEY_EVENT_ENTER:
            console_write_char('\n');
            shell_store_history(shell.line);
            command_execute(shell.line);
            if (console_get_x() != 0) {
                console_write_char('\n');
            }
            shell_show_prompt();
            break;
        case KEY_EVENT_BACKSPACE:
            shell_backspace();
            break;
        case KEY_EVENT_DELETE:
            shell_delete_at_cursor();
            break;
        case KEY_EVENT_LEFT:
            shell_move_cursor_left();
            break;
        case KEY_EVENT_RIGHT:
            shell_move_cursor_right();
            break;
        case KEY_EVENT_UP:
            shell_history_up();
            break;
        case KEY_EVENT_DOWN:
            shell_history_down();
            break;
        case KEY_EVENT_HOME:
            shell_move_cursor_home();
            break;
        case KEY_EVENT_END:
            shell_move_cursor_end();
            break;
        case KEY_EVENT_NONE:
        default:
            break;
    }
}

void shell_run(void) {
    console_clear();
    console_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    console_writeln("PrismOS Loaded!");
    console_set_color(COLOR_WHITE, COLOR_BLACK);
    console_writeln("Type 'help' for commands.");
    console_writeln("");
    shell_show_prompt();

    while (1) {
        shell_handle_event(keyboard_read_event());
    }
}