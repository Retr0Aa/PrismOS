int main() {
    int x = 0;
    int y = 0;
    int color = 1;

    set_fullscreen(1);

    while (app_should_quit() == 0) {
        draw_pixel(x, y, color);

        x = x + 1;
        if (x >= 320) {
            x = 0;
            y = y + 1;
        }

        if (y >= 200) {
            y = 0;
            color = color + 1;
            if (color > 15) {
                color = 1;
            }
        }
    }

    set_fullscreen(0);
    return 0;
}
