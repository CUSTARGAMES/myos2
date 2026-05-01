#include <stdint.h>

/* ============ VGA TEXT MODE ============ */
static volatile uint16_t *const vga = (uint16_t *)0xB8000;
#define COLS 80
#define ROWS 25
static int cx = 0, cy = 0;
static uint8_t tc = 0x0F;

/* ============ COLORS ============ */
#define BLACK   0x00
#define BLUE    0x01
#define GREEN   0x02
#define CYAN    0x03
#define RED     0x04
#define MAGENTA 0x05
#define BROWN   0x06
#define LGRAY   0x07
#define DGRAY   0x08
#define LBLUE   0x09
#define LGREEN  0x0A
#define LCYAN   0x0B
#define LRED    0x0C
#define LMAGENTA 0x0D
#define YELLOW  0x0E
#define WHITE   0x0F

/* ============ I/O ============ */
static inline void outb(uint16_t p, uint8_t v) {
    __asm__ volatile("outb %0, %1" : : "a"(v), "Nd"(p));
}
static inline uint8_t inb(uint16_t p) {
    uint8_t r;
    __asm__ volatile("inb %1, %0" : "=a"(r) : "Nd"(p));
    return r;
}
static inline void iowait(void) { outb(0x80, 0); }

/* ============ SYSTEM STATE ============ */
static char username[20] = "user";
static char pcname[20] = "TFD-PC";
static char password[20] = "";
static int has_password = 0;
static char cmd[256];
static int cmd_len = 0;
static char history[50][256];
static int hist_count = 0;
static char last_cmd[256] = "";
static char install_disk[20] = "";
static int is_installed = 0;
static uint32_t uptime_sec = 0;

/* ============ STRING HELPERS ============ */
static int str_len(const char *s) { int n = 0; while (s[n]) n++; return n; }
static int str_cmp(const char *a, const char *b) { while (*a && *b && *a == *b) { a++; b++; } return *a - *b; }
static void str_cpy(char *d, const char *s) { while (*s) *d++ = *s++; *d = 0; }
static int str_to_int(const char *s) {
    int n = 0;
    while (*s >= '0' && *s <= '9') { n = n * 10 + (*s - '0'); s++; }
    return n;
}
static void int_to_str(int n, char *b) {
    int i = 0;
    if (n == 0) { b[0] = '0'; b[1] = 0; return; }
    while (n > 0) { b[i++] = '0' + (n % 10); n /= 10; }
    b[i] = 0;
    for (int j = 0; j < i / 2; j++) { char t = b[j]; b[j] = b[i - 1 - j]; b[i - 1 - j] = t; }
}

/* ============ SCREEN ============ */
static void set_cursor(int x, int y) {
    uint16_t p = y * COLS + x;
    outb(0x3D4, 0x0F); outb(0x3D5, (uint8_t)(p & 0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (uint8_t)((p >> 8) & 0xFF));
    cx = x; cy = y;
}
static void hide_cursor(void) { outb(0x3D4, 0x0A); outb(0x3D5, 0x20); }
static void show_cursor(void) { outb(0x3D4, 0x0A); outb(0x3D5, 0x0E); outb(0x3D4, 0x0B); outb(0x3D5, 0x0F); }
static void clear_screen(void) {
    for (int i = 0; i < COLS * ROWS; i++) vga[i] = (uint16_t)' ' | ((uint16_t)LGRAY << 8);
    cx = 0; cy = 0;
}
static void scroll_up(void) {
    for (int y = 0; y < ROWS - 1; y++)
        for (int x = 0; x < COLS; x++)
            vga[y * COLS + x] = vga[(y + 1) * COLS + x];
    for (int x = 0; x < COLS; x++)
        vga[(ROWS - 1) * COLS + x] = (uint16_t)' ' | ((uint16_t)LGRAY << 8);
}
static void putc(char c) {
    if (c == '\n') { cx = 0; cy++; if (cy >= ROWS) { scroll_up(); cy = ROWS - 1; } }
    else if (c == '\b') { if (cx > 0) { cx--; vga[cy * COLS + cx] = (uint16_t)' ' | ((uint16_t)LGRAY << 8); } }
    else { vga[cy * COLS + cx] = (uint16_t)c | ((uint16_t)tc << 8); cx++; if (cx >= COLS) { cx = 0; cy++; if (cy >= ROWS) { scroll_up(); cy = ROWS - 1; } } }
    set_cursor(cx, cy);
}
static void print(const char *s, uint8_t c) { tc = c; while (*s) putc(*s++); }
static void println(const char *s, uint8_t c) { print(s, c); putc('\n'); }
static void putc_at(int x, int y, char c, uint8_t col) {
    if (x >= 0 && x < COLS && y >= 0 && y < ROWS)
        vga[y * COLS + x] = (uint16_t)c | ((uint16_t)col << 8);
}
static void print_at(int x, int y, const char *s, uint8_t c) { while (*s) putc_at(x++, y, *s++, c); }
static void print_prompt(void) {
    print("TFD~(", GREEN); print(username, WHITE); print("@", LGRAY);
    print(pcname, WHITE); print("):$ ", GREEN);
}

/* ============ KEYBOARD ============ */
static void ps2_wait_write(void) { while (inb(0x64) & 2) iowait(); }
static void ps2_wait_data(void) { while (!(inb(0x64) & 1)) iowait(); }
static char scancode_to_ascii(uint8_t sc) {
    switch (sc) {
        case 0x0E: return '\b'; case 0x1C: return '\n'; case 0x39: return ' ';
        case 0x02: return '1'; case 0x03: return '2'; case 0x04: return '3'; case 0x05: return '4';
        case 0x06: return '5'; case 0x07: return '6'; case 0x08: return '7'; case 0x09: return '8';
        case 0x0A: return '9'; case 0x0B: return '0'; case 0x0C: return '-'; case 0x0D: return '=';
        case 0x10: return 'q'; case 0x11: return 'w'; case 0x12: return 'e'; case 0x13: return 'r';
        case 0x14: return 't'; case 0x15: return 'y'; case 0x16: return 'u'; case 0x17: return 'i';
        case 0x18: return 'o'; case 0x19: return 'p'; case 0x1A: return '['; case 0x1B: return ']';
        case 0x1E: return 'a'; case 0x1F: return 's'; case 0x20: return 'd'; case 0x21: return 'f';
        case 0x22: return 'g'; case 0x23: return 'h'; case 0x24: return 'j'; case 0x25: return 'k';
        case 0x26: return 'l'; case 0x27: return ';'; case 0x28: return '\''; case 0x29: return '`';
        case 0x2B: return '\\'; case 0x2C: return 'z'; case 0x2D: return 'x'; case 0x2E: return 'c';
        case 0x2F: return 'v'; case 0x30: return 'b'; case 0x31: return 'n'; case 0x32: return 'm';
        case 0x33: return ','; case 0x34: return '.'; case 0x35: return '/'; case 0x37: return '*';
        default: return 0;
    }
}
static char read_key(void) { ps2_wait_data(); return scancode_to_ascii(inb(0x60)); }
static int key_available(void) { return inb(0x64) & 1; }

/* ============ SHUTDOWN (Real Power Off) ============ */
static void power_off(void) {
    /* Try APM shutdown first */
    outb(0x64, 0xFE);  /* System reset (works on some) */
    
    /* Try ACPI shutdown */
    outw(0x604, 0x2000);  /* QEMU/VirtualBox ACPI */
    
    /* Try Bochs/QEMU shutdown */
    outw(0xB004, 0x2000);
    
    /* If all fails, halt CPU */
    __asm__ volatile("cli; hlt");
}

/* ============ COMMAND EXECUTION ============ */
static void exec_command(const char *c) {
    if (c[0] == 0) return;
    
    /* Save history */
    if (hist_count < 50) str_cpy(history[hist_count++], c);
    str_cpy(last_cmd, c);
    
    /* Parse command */
    char cm[30] = "", arg[200] = "";
    int i = 0, j = 0;
    while (c[i] && c[i] != ' ' && j < 29) cm[j++] = c[i++];
    cm[j] = 0;
    if (c[i] == ' ') i++;
    while (c[i] && j < 199) arg[j++] = c[i++];
    arg[j] = 0;
    for (int k = 0; cm[k]; k++) if (cm[k] >= 'A' && cm[k] <= 'Z') cm[k] += 32;

    /* === DISPATCH === */
    if (str_cmp(cm, "help") == 0) {
        println("", 0);
        println("═════ TFD OS v1.0 ═════", CYAN);
        println("help       Show commands", WHITE);
        println("clear      Clear screen", WHITE);
        println("about      OS info", WHITE);
        println("ver        Version", WHITE);
        println("date       Show date", WHITE);
        println("time       Show time", WHITE);
        println("uptime     System uptime", WHITE);
        println("reboot     Restart system", WHITE);
        println("shutdown   Power off", RED);
        println("lock       Lock screen", WHITE);
        println("echo       Print text", WHITE);
        println("whoami     Show username", WHITE);
        println("history    Command history", WHITE);
        println("clearhist  Clear history", WHITE);
        println("passwd     Set password", WHITE);
        println("fxc sys    System status", WHITE);
        println("install    Install to disk", YELLOW);
        println("uninstall  Remove installation", RED);
        println("snake      Play snake", GREEN);
        println("tetris     Play tetris", GREEN);
        println("pong       Play pong", GREEN);
        println("dice       Roll dice", GREEN);
        println("coin       Flip coin", GREEN);
        println("beep       PC speaker beep", WHITE);
        println("colors     Show colors", WHITE);
        println("rainbow    Rainbow text", WHITE);
        println("═══════════════════════", CYAN);
    }
    else if (str_cmp(cm, "clear") == 0) clear_screen();
    else if (str_cmp(cm, "about") == 0) {
        println("TFD OS v1.0 - Text-Based OS by Sadman (2026)", GREEN);
    }
    else if (str_cmp(cm, "ver") == 0) println("TFD OS v1.0", GREEN);
    else if (str_cmp(cm, "date") == 0) println("2026-05-01", WHITE);
    else if (str_cmp(cm, "time") == 0) println("12:00:00", WHITE);
    else if (str_cmp(cm, "uptime") == 0) {
        char b[20]; int_to_str(uptime_sec, b);
        print("Uptime: ", GREEN); print(b, WHITE); println(" seconds", GREEN);
    }
    else if (str_cmp(cm, "reboot") == 0) {
        println("Rebooting...", YELLOW);
        for (volatile int d = 0; d < 300000; d++);
        outb(0x64, 0xFE);  /* Pulse reset pin */
    }
    else if (str_cmp(cm, "shutdown") == 0) {
        println("Shutting down...", RED);
        for (int t = 3; t > 0; t--) {
            char b[3]; b[0] = '0' + t; b[1] = 0;
            print(b, RED); print("...", RED);
            for (volatile int d = 0; d < 200000; d++);
        }
        println("", 0);
        println("Power off.", GREEN);
        for (volatile int d = 0; d < 100000; d++);
        power_off();
        while (1) __asm__ volatile("hlt");
    }
    else if (str_cmp(cm, "lock") == 0) {
        if (has_password) {
            println("Screen locked. Enter password:", YELLOW);
            char pw[20]; int i = 0;
            while (1) {
                char c = read_key();
                if (c == '\n') { pw[i] = 0; break; }
                else if (c >= 32 && i < 19) { pw[i++] = c; putc('*'); }
            }
            if (str_cmp(pw, password) == 0) { println("", 0); println("Unlocked!", GREEN); }
            else { println("", 0); println("Wrong password!", RED); }
        } else {
            println("No password set. Use 'passwd'.", YELLOW);
        }
    }
    else if (str_cmp(cm, "echo") == 0) println(arg, WHITE);
    else if (str_cmp(cm, "whoami") == 0) println(username, GREEN);
    else if (str_cmp(cm, "history") == 0) {
        for (int i = 0; i < hist_count; i++) {
            char b[5]; b[0] = ' '; if (i < 9) b[1] = '1' + i;
            else { b[0] = '1'; b[1] = '0' + i - 9; } b[2] = ' '; b[3] = 0;
            print(b, WHITE); println(history[i], LGRAY);
        }
    }
    else if (str_cmp(cm, "clearhist") == 0) { hist_count = 0; println("Cleared.", GREEN); }
    else if (str_cmp(cm, "passwd") == 0) {
        print("New password: ", YELLOW);
        int i = 0; password[0] = 0;
        while (1) {
            char c = read_key();
            if (c == '\n') { password[i] = 0; break; }
            else if (c >= 32 && i < 19) { password[i++] = c; putc('*'); }
        }
        has_password = 1;
        println("", 0); println("Password set!", GREEN);
    }
    else if (str_cmp(cm, "fxc") == 0) {
        println("", 0);
        if (is_installed) {
            print("Mode: PERMANENT (", GREEN);
            print(install_disk, WHITE);
            println(")", GREEN);
        } else {
            println("Mode: LIVE (USB Boot)", YELLOW);
        }
    }
    else if (str_cmp(cm, "install") == 0) {
        println("TFD OS Installation Wizard", YELLOW);
        print("Type 'y' to install: ", WHITE);
        char c = read_key(); putc(c); println("", 0);
        if (c == 'y') {
            println("Select: 1=HDD 2=SSD 3=USB", WHITE);
            char d = read_key();
            if (d == '1') str_cpy(install_disk, "HDD");
            else if (d == '2') str_cpy(install_disk, "SSD");
            else if (d == '3') str_cpy(install_disk, "USB");
            else { println("Cancelled.", YELLOW); return; }
            
            for (int p = 0; p <= 100; p += 25) {
                print("[", GREEN);
                for (int j = 0; j < 20; j++) print(j < p/5 ? "=" : " ", GREEN);
                print("] ", GREEN);
                char b[5]; int_to_str(p, b); println(b, WHITE);
                for (volatile int d = 0; d < 200000; d++);
            }
            is_installed = 1;
            println("Installed! Use 'fxc sys' to check.", GREEN);
        }
    }
    else if (str_cmp(cm, "uninstall") == 0) {
        if (is_installed) {
            print("Remove TFD OS from ", RED); print(install_disk, RED);
            println("? Type 'y':", RED);
            char c = read_key();
            if (c == 'y') {
                is_installed = 0;
                install_disk[0] = 0;
                println("System removed. Reboot to finish.", GREEN);
            } else println("Cancelled.", YELLOW);
        } else {
            println("Not installed. Use 'fxc sys' to check.", YELLOW);
        }
    }
    else if (str_cmp(cm, "snake") == 0) {
        /* Simple snake placeholder */
        println("Snake game - Coming soon!", GREEN);
    }
    else if (str_cmp(cm, "tetris") == 0) {
        println("Tetris game - Coming soon!", GREEN);
    }
    else if (str_cmp(cm, "pong") == 0) {
        println("Pong game - Coming soon!", GREEN);
    }
    else if (str_cmp(cm, "dice") == 0) {
        char b[3]; int_to_str((inb(0x40) % 6) + 1, b);
        println(b, YELLOW);
    }
    else if (str_cmp(cm, "coin") == 0) {
        println((inb(0x40) % 2) ? "Heads!" : "Tails!", YELLOW);
    }
    else if (str_cmp(cm, "beep") == 0) {
        outb(0x43, 0xB6);
        outb(0x42, 0x50);
        outb(0x42, 0x09);
        outb(0x61, inb(0x61) | 3);
        for (volatile int d = 0; d < 80000; d++);
        outb(0x61, inb(0x61) & ~3);
    }
    else if (str_cmp(cm, "colors") == 0) {
        for (int i = 0; i < 16; i++) { tc = i; print("███ ", i); }
        println("", WHITE);
    }
    else if (str_cmp(cm, "rainbow") == 0) {
        print("R", RED); print("A", YELLOW); print("I", GREEN);
        print("N", CYAN); print("B", BLUE); print("O", MAGENTA);
        println("W", RED);
    }
    else {
        print("?: ", RED); println(c, RED);
        println("Type 'help' for commands", YELLOW);
    }
}

/* ============ BOOT MENU ============ */
static void boot_menu(void) {
    clear_screen();
    print_at(25, 2, "╔══════════════════════════════╗", CYAN);
    print_at(25, 3, "║                              ║", CYAN);
    print_at(25, 4, "║   ████████╗███████╗██████╗   ║", RED);
    print_at(25, 5, "║   ╚══██╔══╝██╔════╝██╔══██╗  ║", RED);
    print_at(25, 6, "║      ██║   █████╗  ██║  ██║  ║", GREEN);
    print_at(25, 7, "║      ██║   ██╔══╝  ██║  ██║  ║", GREEN);
    print_at(25, 8, "║      ██║   ██║     ██████╔╝  ║", BLUE);
    print_at(25, 9, "║      ╚═╝   ╚═╝     ╚═════╝   ║", BLUE);
    print_at(25,10, "║                              ║", CYAN);
    print_at(25,11, "║     TFD OS v1.0              ║", CYAN);
    print_at(25,12, "║     by Sadman                ║", CYAN);
    print_at(25,13, "║     Text-Based OS            ║", CYAN);
    print_at(25,14, "╚══════════════════════════════╝", CYAN);
    print_at(28, 16, "[ PRESS ENTER TO BOOT ]", YELLOW);
    while (1) { if (key_available() && read_key() == '\n') break; }
}

/* ============ SETUP ============ */
static void setup_wizard(void) {
    clear_screen();
    println("SETUP WIZARD", CYAN);
    println("", 0);
    print("Username: ", YELLOW); show_cursor();
    int i = 0; username[0] = 0;
    while (1) {
        char c = read_key();
        if (c == '\n') { username[i] = 0; break; }
        else if (c == '\b' && i > 0) { i--; putc('\b'); }
        else if (c >= 32 && i < 19) { username[i++] = c; putc(c); }
    }
    println("", 0);
    print("PC name: ", YELLOW);
    i = 0; pcname[0] = 0;
    while (1) {
        char c = read_key();
        if (c == '\n') { pcname[i] = 0; break; }
        else if (c == '\b' && i > 0) { i--; putc('\b'); }
        else if (c >= 32 && i < 19) { pcname[i++] = c; putc(c); }
    }
    println("", 0);
    print("Welcome, ", GREEN); println(username, WHITE);
    for (volatile int d = 0; d < 200000; d++);
    hide_cursor();
}

/* ============ MAIN ============ */
void kernel_main(uint32_t magic, uint32_t addr) {
    (void)magic; (void)addr;
    
    /* Init keyboard */
    ps2_wait_write(); outb(0x64, 0xA8);
    ps2_wait_write(); outb(0x60, 0xF0);
    ps2_wait_write(); outb(0x60, 0x01);
    
    hide_cursor();
    boot_menu();
    setup_wizard();
    clear_screen();
    println("TFD OS v1.0 - Ready", GREEN);
    println("Type 'help' for commands", YELLOW);
    println("", 0);
    cmd_len = 0;

    while (1) {
        print_prompt();
        show_cursor();
        cmd_len = 0;
        
        while (1) {
            char c = read_key();
            if (c == '\n') { putc('\n'); break; }
            else if (c == '\b') { if (cmd_len > 0) { cmd_len--; putc('\b'); } }
            else if (c >= 32 && c <= 126 && cmd_len < 255) { cmd[cmd_len++] = c; putc(c); }
        }
        
        hide_cursor();
        cmd[cmd_len] = 0;
        exec_command(cmd);
        uptime_sec++;
        println("", 0);
    }
}
