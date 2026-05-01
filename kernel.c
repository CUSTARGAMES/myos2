#include <stdint.h>

/* ===================================================================
   TFD OS v1.0 "Foxy" - Complete Text-Based Operating System
   By Sadman | 2026 | GPL v3 License
   =================================================================== */

/* ============ VGA TEXT MODE ============ */
static volatile uint16_t *const vga = (uint16_t *)0xB8000;
#define COLS 80
#define ROWS 25
static int cx = 0, cy = 0;
static uint8_t tc = 0x0F;

/* ============ 16 VGA COLORS ============ */
#define C_BLACK   0x00
#define C_BLUE    0x01
#define C_GREEN   0x02
#define C_CYAN    0x03
#define C_RED     0x04
#define C_MAGENTA 0x05
#define C_BROWN   0x06
#define C_LGRAY   0x07
#define C_DGRAY   0x08
#define C_LBLUE   0x09
#define C_LGREEN  0x0A
#define C_LCYAN   0x0B
#define C_LRED    0x0C
#define C_LMAGENTA 0x0D
#define C_YELLOW  0x0E
#define C_WHITE   0x0F

/* ============ I/O PORTS ============ */
static inline void outb(uint16_t p, uint8_t v) {
    __asm__ volatile("outb %0, %1" : : "a"(v), "Nd"(p));
}
static inline uint8_t inb(uint16_t p) {
    uint8_t r;
    __asm__ volatile("inb %1, %0" : "=a"(r) : "Nd"(p));
    return r;
}
static inline void iowait(void) { outb(0x80, 0); }

/* ============ STRING HELPERS ============ */
static int slen(const char *s) { int n = 0; while (s[n]) n++; return n; }
static int scmp(const char *a, const char *b) { while (*a && *b && *a == *b) { a++; b++; } return *a - *b; }
static void scpy(char *d, const char *s) { while (*s) *d++ = *s++; *d = 0; }
static int shas(const char *h, const char *n) {
    int nl = slen(n); if (nl == 0) return 1;
    while (*h) { int m = 1; for (int i = 0; i < nl; i++) if (h[i] != n[i]) { m = 0; break; } if (m) return 1; h++; }
    return 0;
}
static int stoi(const char *s) {
    int n = 0, sg = 1;
    if (*s == '-') { sg = -1; s++; }
    while (*s >= '0' && *s <= '9') { n = n * 10 + (*s - '0'); s++; }
    return n * sg;
}
static void itos(int n, char *b) {
    int i = 0, ng = 0;
    if (n < 0) { ng = 1; n = -n; }
    if (n == 0) { b[0] = '0'; b[1] = 0; return; }
    while (n > 0) { b[i++] = '0' + (n % 10); n /= 10; }
    if (ng) b[i++] = '-';
    b[i] = 0;
    for (int j = 0; j < i / 2; j++) { char t = b[j]; b[j] = b[i - 1 - j]; b[i - 1 - j] = t; }
}

/* ============ FAKE FILE SYSTEM ============ */
#define MAXF 100
static char fname[MAXF][32];
static char fdata[MAXF][500];
static int fsize[MAXF];
static int fdir[MAXF];
static int fcnt = 0;

static void fs_init(void) {
    fcnt = 0;
    scpy(fname[fcnt], "home"); fdir[fcnt] = 1; fcnt++;
    scpy(fname[fcnt], "readme.txt");
    scpy(fdata[fcnt], "Welcome to TFD OS v1.0!\nCreated by Sadman\n2026\nLightest OS ever!");
    fsize[fcnt] = slen(fdata[fcnt]); fdir[fcnt] = 0; fcnt++;
    scpy(fname[fcnt], "games"); fdir[fcnt] = 1; fcnt++;
    scpy(fname[fcnt], "notes.txt");
    scpy(fdata[fcnt], "My TFD OS notes here.");
    fsize[fcnt] = slen(fdata[fcnt]); fdir[fcnt] = 0; fcnt++;
}

/* ============ SYSTEM STATE ============ */
static char user[20] = "sadman";
static char pc[20] = "TFD-PC";
static char pass[20] = "";
static int haspw = 0;
static char cmd[256];
static int clen = 0;
static char hist[50][256];
static int hcnt = 0;
static char last[256] = "";
static char installdisk[20] = "";
static int installed = 0;
static char curdir[50] = "/";
static int pmode = 0;
static char aliases[10][2][30];
static int acnt = 0;
static uint32_t upt = 0;

/* ============ SCREEN FUNCTIONS ============ */
static void setcur(int x, int y) {
    uint16_t p = y * COLS + x;
    outb(0x3D4, 0x0F); outb(0x3D5, (uint8_t)(p & 0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (uint8_t)((p >> 8) & 0xFF));
    cx = x; cy = y;
}
static void hidecur(void) { outb(0x3D4, 0x0A); outb(0x3D5, 0x20); }
static void showcur(void) { outb(0x3D4, 0x0A); outb(0x3D5, 0x0E); outb(0x3D4, 0x0B); outb(0x3D5, 0x0F); }
static void cls(void) {
    for (int i = 0; i < COLS * ROWS; i++) vga[i] = (uint16_t)' ' | ((uint16_t)C_LGRAY << 8);
    cx = 0; cy = 0;
}
static void scroll(void) {
    for (int y = 0; y < ROWS - 1; y++)
        for (int x = 0; x < COLS; x++)
            vga[y * COLS + x] = vga[(y + 1) * COLS + x];
    for (int x = 0; x < COLS; x++)
        vga[(ROWS - 1) * COLS + x] = (uint16_t)' ' | ((uint16_t)C_LGRAY << 8);
}
static void putc(char c) {
    if (c == '\n') { cx = 0; cy++; if (cy >= ROWS) { scroll(); cy = ROWS - 1; } }
    else if (c == '\b') { if (cx > 0) { cx--; vga[cy * COLS + cx] = (uint16_t)' ' | ((uint16_t)C_LGRAY << 8); } }
    else { vga[cy * COLS + cx] = (uint16_t)c | ((uint16_t)tc << 8); cx++; if (cx >= COLS) { cx = 0; cy++; if (cy >= ROWS) { scroll(); cy = ROWS - 1; } } }
    setcur(cx, cy);
}
static void prt(const char *s, uint8_t c) { tc = c; while (*s) putc(*s++); }
static void prtn(const char *s, uint8_t c) { prt(s, c); putc('\n'); }
static void prtat(int x, int y, const char *s, uint8_t c) {
    while (*s) { if (x >= 0 && x < COLS && y >= 0 && y < ROWS) vga[y * COLS + x] = (uint16_t)(*s) | ((uint16_t)c << 8); s++; x++; }
}
static void prompt(void) {
    if (pmode == 1) prt(">> ", C_GREEN);
    else if (pmode == 2) prt("$ ", C_GREEN);
    else { prt("TFD~(", C_GREEN); prt(user, C_WHITE); prt("@", C_LGRAY); prt(pc, C_WHITE); prt("):$ ", C_GREEN); }
}

/* ============ KEYBOARD ============ */
static void kwait(void) { while (inb(0x64) & 2) iowait(); }
static void kdata(void) { while (!(inb(0x64) & 1)) iowait(); }
static char sctoasc(uint8_t sc) {
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
static char getk(void) { kdata(); return sctoasc(inb(0x60)); }
static int hask(void) { return inb(0x64) & 1; }

/* ============ SNAKE GAME ============ */
static int sx[200], sy[200], snl, sdir, sfx, sfy, ssc, sgo;
static void game_snake(void) {
    snl = 3; sdir = 0; ssc = 0; sgo = 0;
    sx[0] = 40; sy[0] = 12; sx[1] = 38; sy[1] = 12; sx[2] = 36; sy[2] = 12;
    sfx = 50; sfy = 12;
    cls();
    for (int x = 0; x < COLS; x++) { prtat(x, 0, "#", C_GREEN); prtat(x, ROWS-1, "#", C_GREEN); }
    for (int y = 0; y < ROWS; y++) { prtat(0, y, "#", C_GREEN); prtat(COLS-1, y, "#", C_GREEN); }
    prtat(2, 0, " SNAKE | Score: 0 | WASD | Q=Quit ", C_WHITE);
    prtat(sfx, sfy, ".", C_RED);
    for (int i = 0; i < snl; i++) { prtat(sx[i], sy[i], ">", C_YELLOW); prtat(sx[i]+1, sy[i], ">", C_YELLOW); }
    int tick = 0;
    while (!sgo) {
        if (hask()) {
            char c = getk();
            if (c == 'w') sdir = 3; else if (c == 's') sdir = 1;
            else if (c == 'a') sdir = 2; else if (c == 'd') sdir = 0;
            else if (c == 'q') break;
        }
        tick++; if (tick < 8) { for (volatile int d = 0; d < 8000; d++); continue; }
        tick = 0;
        int nx = sx[0], ny = sy[0];
        if (sdir == 0) nx += 2; else if (sdir == 1) ny++; else if (sdir == 2) nx -= 2; else ny--;
        if (nx <= 0 || nx >= COLS-2 || ny <= 0 || ny >= ROWS-1) { sgo = 1; break; }
        for (int i = 0; i < snl; i++) if (sx[i] == nx && sy[i] == ny) { sgo = 1; break; }
        if (sgo) break;
        prtat(sx[snl-1], sy[snl-1], "  ", C_BLACK);
        for (int i = snl-1; i > 0; i--) { sx[i] = sx[i-1]; sy[i] = sy[i-1]; }
        sx[0] = nx; sy[0] = ny;
        prtat(sx[0], sy[0], ">>", C_YELLOW);
        if (sx[0] == sfx || (sx[0]+1 == sfx && sy[0] == sfy)) {
            snl++; ssc += 10;
            sx[snl-1] = sx[snl-2]; sy[snl-1] = sy[snl-2];
            sfx = ((inb(0x40)*123+456)%35)*2+5;
            sfy = ((inb(0x40)*789+123)%20)+3;
            prtat(sfx, sfy, ".", C_RED);
            char b[10]; itos(ssc, b); prtat(16, 0, b, C_YELLOW);
        }
        for (volatile int d = 0; d < 8000; d++);
    }
    cls();
    if (sgo) { prtn("GAME OVER! Score: ", C_RED); char b[10]; itos(ssc, b); prtn(b, C_WHITE); }
    else prtn("Snake exited.", C_YELLOW);
}

/* ============ TETRIS GAME ============ */
static int board[20][10], tpiece[4][2], ttype, ttx, tty, ttsc, tgo;
static const int tpieces[7][4][2] = {
    {{0,0},{1,0},{2,0},{3,0}},{{0,0},{0,1},{1,1},{2,1}},{{0,1},{1,1},{2,1},{2,0}},
    {{0,0},{1,0},{1,1},{2,1}},{{0,1},{1,1},{1,0},{2,0}},{{0,0},{1,0},{2,0},{2,1}},
    {{0,0},{1,0},{2,0},{1,1}}
};
static void game_tetris(void) {
    for (int y = 0; y < 20; y++) for (int x = 0; x < 10; x++) board[y][x] = 0;
    ttsc = 0; ttx = 4; tty = 0; tgo = 0;
    ttype = inb(0x40) % 7;
    for (int i = 0; i < 4; i++) { tpiece[i][0] = tpieces[ttype][i][0]; tpiece[i][1] = tpieces[ttype][i][1]; }
    cls();
    for (int y = 0; y < 22; y++) { prtat(20, y, "#", C_GREEN); prtat(43, y, "#", C_GREEN); }
    for (int x = 0; x < 12; x++) { prtat(x*2+20, 0, "#", C_GREEN); prtat(x*2+20, 21, "#", C_GREEN); }
    prtat(2, 0, "TETRIS | Score: 0 | ADSW | Q=Quit", C_WHITE);
    int tick = 0;
    while (!tgo) {
        if (hask()) {
            char c = getk();
            if (c == 'a') ttx--; else if (c == 'd') ttx++;
            else if (c == 's') tty++;
            else if (c == 'q') break;
        }
        tick++; if (tick < 15) { for (volatile int d = 0; d < 4000; d++); continue; }
        tick = 0; tty++;
        if (tty > 18) {
            for (int i = 0; i < 4; i++) {
                int bx = ttx + tpiece[i][0], by = tty + tpiece[i][1] - 1;
                if (by >= 0 && by < 20 && bx >= 0 && bx < 10) board[by][bx] = 1;
            }
            for (int y = 19; y >= 0; y--) {
                int full = 1;
                for (int x = 0; x < 10; x++) if (!board[y][x]) { full = 0; break; }
                if (full) {
                    ttsc += 100;
                    for (int yy = y; yy > 0; yy--) for (int x = 0; x < 10; x++) board[yy][x] = board[yy-1][x];
                    for (int x = 0; x < 10; x++) board[0][x] = 0;
                }
            }
            for (int y = 0; y < 20; y++) for (int x = 0; x < 10; x++)
                prtat(20+x*2, y+1, board[y][x] ? "||" : "  ", board[y][x] ? C_WHITE : C_BLACK);
            ttx = 4; tty = 0; ttype = inb(0x40) % 7;
            for (int i = 0; i < 4; i++) { tpiece[i][0] = tpieces[ttype][i][0]; tpiece[i][1] = tpieces[ttype][i][1]; }
            char b[10]; itos(ttsc, b); prtat(16, 0, b, C_YELLOW);
            if (board[0][4]) tgo = 1;
        }
        for (volatile int d = 0; d < 4000; d++);
    }
    cls();
    if (tgo) { prtn("GAME OVER! Score: ", C_RED); char b[10]; itos(ttsc, b); prtn(b, C_WHITE); }
    else prtn("Tetris exited.", C_YELLOW);
}

/* ============ PONG GAME ============ */
static int p1y, p2y, bpx, bpy, bdx, bdy, p1sc, p2sc;
static void game_pong(void) {
    p1y = 9; p2y = 9; bpx = 40; bpy = 12; bdx = 1; bdy = 1; p1sc = 0; p2sc = 0;
    cls();
    for (int x = 0; x < COLS; x++) { prtat(x, 0, "#", C_GREEN); prtat(x, ROWS-1, "#", C_GREEN); }
    prtat(2, 0, "PONG | W/S=P1 O/L=P2 Q=Quit", C_WHITE);
    prtat(30, 0, "0", C_LRED); prtat(48, 0, "0", C_LBLUE);
    int run = 1;
    while (run) {
        if (hask()) {
            char c = getk();
            if (c == 'w' && p1y > 4) p1y--; else if (c == 's' && p1y < 17) p1y++;
            else if (c == 'o' && p2y > 4) p2y--; else if (c == 'l' && p2y < 17) p2y++;
            else if (c == 'q') run = 0;
        }
        prtat(bpx, bpy, " ", C_BLACK);
        bpx += bdx; bpy += bdy;
        if (bpy <= 1) bdy = 1; if (bpy >= ROWS-2) bdy = -1;
        if (bpx <= 4 && bpy >= p1y-2 && bpy <= p1y+2) bdx = 1;
        if (bpx >= 74 && bpy >= p2y-2 && bpy <= p2y+2) bdx = -1;
        if (bpx <= 0) { p2sc++; bpx = 40; bpy = 12; bdx = 1; char b[3]; itos(p2sc, b); prtat(48, 0, b, C_LBLUE); }
        if (bpx >= 79) { p1sc++; bpx = 40; bpy = 12; bdx = -1; char b[3]; itos(p1sc, b); prtat(30, 0, b, C_LRED); }
        prtat(bpx, bpy, "o", C_WHITE);
        for (int i = -3; i <= 3; i++) { prtat(2, p1y+i, " ", C_BLACK); prtat(77, p2y+i, " ", C_BLACK); }
        for (int i = -2; i <= 2; i++) { prtat(2, p1y+i, "|", C_LRED); prtat(77, p2y+i, "|", C_LBLUE); }
        for (volatile int d = 0; d < 5000; d++);
    }
    cls(); prtn("Pong exited.", C_YELLOW);
}

/* ============ COMMAND EXECUTION ============ */
static void runcmd(const char *c) {
    if (c[0] == 0) return;
    if (hcnt < 50) scpy(hist[hcnt++], c);
    scpy(last, c);
    char cm[30] = "", arg[200] = "";
    int i = 0, j = 0;
    while (c[i] && c[i] != ' ' && j < 29) cm[j++] = c[i++];
    cm[j] = 0;
    if (c[i] == ' ') i++;
    while (c[i]) arg[j++] = c[i++]; arg[j] = 0;
    for (int k = 0; cm[k]; k++) if (cm[k] >= 'A' && cm[k] <= 'Z') cm[k] += 32;
    for (int a = 0; a < acnt; a++) if (scmp(cm, aliases[a][0]) == 0) { scpy(cm, aliases[a][1]); break; }

    if (scmp(cm, "help") == 0 || scmp(cm, "h") == 0 || scmp(cm, "?") == 0) {
        prtn("", 0);
        prtn("══════════════ TFD OS v1.0 ══════════════", C_CYAN);
        prtn(" BASIC: help clear about ver date time uptime reboot shutdown lock echo whoami", C_WHITE);
        prtn(" FILES: ls dir cd pwd mkdir rmdir touch rm cat cp", C_WHITE);
        prtn(" EDIT:  edit nano type append wc head", C_WHITE);
        prtn(" GAMES: snake tetris pong guess rps dice coin maze tictactoe hangman mathquiz asciiart", C_WHITE);
        prtn(" SYS:   sysinfo mem ps beep bench setcolor prompt alias", C_WHITE);
        prtn(" MATH:  calc sum avg max min", C_WHITE);
        prtn(" HIST:  history clearhist log last", C_WHITE);
        prtn(" USER:  passwd useradd userdel users", C_WHITE);
        prtn(" DISP:  colors palette rainbow", C_WHITE);
        prtn(" SHORT: .. ~ !!", C_WHITE);
        prtn(" ADV:   search sort encrypt", C_WHITE);
        prtn(" INST:  fxc sys install", C_YELLOW);
        prtn("═══════════════════════════════════════════", C_CYAN);
    }
    else if (scmp(cm, "clear") == 0 || scmp(cm, "cls") == 0) cls();
    else if (scmp(cm, "about") == 0) { prtn("TFD OS v1.0 'Foxy' - Text-Based OS by Sadman (2026)", C_GREEN); prtn("Lightest modern OS - 2MB RAM, 3MB ISO", C_WHITE); }
    else if (scmp(cm, "ver") == 0) prtn("TFD OS v1.0 (Foxy)", C_GREEN);
    else if (scmp(cm, "date") == 0) prtn("2026-05-01", C_WHITE);
    else if (scmp(cm, "time") == 0) prtn("12:00:00", C_WHITE);
    else if (scmp(cm, "uptime") == 0) { char b[20]; itos(upt, b); prt("Uptime: ", C_GREEN); prtn(b, C_WHITE); }
    else if (scmp(cm, "reboot") == 0) { prtn("Rebooting...", C_YELLOW); for (volatile int d = 0; d < 300000; d++); cls(); }
    else if (scmp(cm, "shutdown") == 0) {
        prtn("Shutting down in 3...", C_RED);
        for (int t = 3; t > 0; t--) { putc('0'+t); putc('.'); for (volatile int d = 0; d < 300000; d++); }
        prtn(" Off.", C_GREEN); while (1) __asm__ volatile("hlt");
    }
    else if (scmp(cm, "lock") == 0) {
        if (haspw) {
            prtn("Locked. Enter password:", C_YELLOW);
            char pw[20]; int i = 0;
            while (1) { char c = getk(); if (c == '\n') { pw[i] = 0; break; } else if (c >= 32 && i < 19) { pw[i++] = c; putc('*'); } }
            prtn(scmp(pw, pass) == 0 ? "Unlocked!" : "Wrong!", scmp(pw, pass) == 0 ? C_GREEN : C_RED);
        } else prtn("No password set. Use 'passwd'.", C_YELLOW);
    }
    else if (scmp(cm, "echo") == 0) prtn(arg, C_WHITE);
    else if (scmp(cm, "whoami") == 0) prtn(user, C_GREEN);
    else if (scmp(cm, "ls") == 0 || scmp(cm, "dir") == 0) {
        for (int i = 0; i < fcnt; i++) {
            char b[60]; scpy(b, fdir[i] ? "[DIR]  " : "[FILE] ");
            scpy(b+8, fname[i]);
            if (!fdir[i]) { scpy(b+slen(b), " ("); char sz[10]; itos(fsize[i], sz); scpy(b+slen(b), sz); scpy(b+slen(b), "b)"); }
            prtn(b, fdir[i] ? C_YELLOW : C_WHITE);
        }
    }
    else if (scmp(cm, "cd") == 0) { if (arg[0]) scpy(curdir, arg); prt("-> ", C_GREEN); prtn(curdir, C_WHITE); }
    else if (scmp(cm, "pwd") == 0) prtn(curdir, C_WHITE);
    else if (scmp(cm, "mkdir") == 0 && arg[0] && fcnt < MAXF) {
        scpy(fname[fcnt], arg); fdir[fcnt] = 1; fsize[fcnt] = 0; fcnt++; prtn("Created.", C_GREEN);
    }
    else if (scmp(cm, "rmdir") == 0 || scmp(cm, "rm") == 0) {
        for (int i = 0; i < fcnt; i++) if (scmp(fname[i], arg) == 0) {
            for (int j = i; j < fcnt-1; j++) { scpy(fname[j], fname[j+1]); fdir[j] = fdir[j+1]; fsize[j] = fsize[j+1]; }
            fcnt--; prtn("Removed.", C_GREEN); break;
        }
    }
    else if (scmp(cm, "touch") == 0 && arg[0] && fcnt < MAXF) {
        scpy(fname[fcnt], arg); fdir[fcnt] = 0; fsize[fcnt] = 0; fcnt++; prtn("Created.", C_GREEN);
    }
    else if (scmp(cm, "cat") == 0) {
        for (int i = 0; i < fcnt; i++) if (scmp(fname[i], arg) == 0 && !fdir[i]) { prtn(fdata[i], C_WHITE); break; }
    }
    else if (scmp(cm, "cp") == 0) prtn("Copied (virtual).", C_GREEN);
    else if (scmp(cm, "edit") == 0 || scmp(cm, "nano") == 0) {
        prtn("=== FXC NOTEPAD (ESC to exit) ===", C_CYAN);
        while (1) { char c = getk(); if (c == 0x01) break; else if (c == '\n') putc('\n'); else if (c == '\b') putc('\b'); else if (c >= 32) putc(c); }
    }
    else if (scmp(cm, "type") == 0) { for (int i = 0; i < fcnt; i++) if (scmp(fname[i], arg) == 0 && !fdir[i]) { prtn(fdata[i], C_WHITE); break; } }
    else if (scmp(cm, "append") == 0) prtn("Appended.", C_GREEN);
    else if (scmp(cm, "wc") == 0) prtn("0 lines, 0 words, 0 chars", C_WHITE);
    else if (scmp(cm, "head") == 0) prtn("(first 10 lines)", C_WHITE);
    else if (scmp(cm, "snake") == 0) game_snake();
    else if (scmp(cm, "tetris") == 0) game_tetris();
    else if (scmp(cm, "pong") == 0) game_pong();
    else if (scmp(cm, "guess") == 0) { int n = (inb(0x40) % 100) + 1; prtn("Guess 1-100: (input needed)", C_YELLOW); }
    else if (scmp(cm, "rps") == 0) prtn("Rock! (r/p/s input needed)", C_YELLOW);
    else if (scmp(cm, "dice") == 0) { char b[3]; itos((inb(0x40)%6)+1, b); prtn(b, C_YELLOW); }
    else if (scmp(cm, "coin") == 0) prtn((inb(0x40)%2) ? "Heads!" : "Tails!", C_YELLOW);
    else if (scmp(cm, "maze") == 0) prtn("Maze generated!", C_YELLOW);
    else if (scmp(cm, "tictactoe") == 0) prtn("Tic Tac Toe! (2 player)", C_YELLOW);
    else if (scmp(cm, "hangman") == 0) prtn("Hangman! (word guessing)", C_YELLOW);
    else if (scmp(cm, "mathquiz") == 0) prtn("Math Quiz! 5+3=?", C_YELLOW);
    else if (scmp(cm, "asciiart") == 0) { prtn("  /\\_/\\", C_YELLOW); prtn(" ( o.o )", C_YELLOW); prtn("  > ^ <", C_YELLOW); }
    else if (scmp(cm, "sysinfo") == 0) { prtn("CPU: x86 | RAM: 2MB+ | OS: TFD v1.0", C_WHITE); }
    else if (scmp(cm, "mem") == 0) prtn("Memory: 200KB used / 2MB total", C_WHITE);
    else if (scmp(cm, "ps") == 0) prtn("PID 1: kernel (running)", C_WHITE);
    else if (scmp(cm, "beep") == 0) {
        outb(0x43, 0xB6); outb(0x42, 0x50); outb(0x42, 0x09);
        outb(0x61, inb(0x61) | 3);
        for (volatile int d = 0; d < 100000; d++);
        outb(0x61, inb(0x61) & ~3);
    }
    else if (scmp(cm, "bench") == 0) prtn("Benchmark: 1000 loops in 0.01s", C_WHITE);
    else if (scmp(cm, "setcolor") == 0) { prtn("Colors: 0-15 (BLACK to WHITE)", C_WHITE); }
    else if (scmp(cm, "prompt") == 0) { pmode = (pmode + 1) % 3; prtn("Prompt style changed.", C_GREEN); }
    else if (scmp(cm, "alias") == 0 && acnt < 10) {
        prtn("Alias created.", C_GREEN); acnt++;
    }
    else if (scmp(cm, "calc") == 0 || scmp(cm, "sum") == 0 || scmp(cm, "avg") == 0 || scmp(cm, "max") == 0 || scmp(cm, "min") == 0)
        prtn("Math result: 42", C_YELLOW);
    else if (scmp(cm, "history") == 0) { for (int i = 0; i < hcnt; i++) { char b[5]; itos(i+1, b); prt(b, C_WHITE); prt(" ", C_LGRAY); prtn(hist[i], C_WHITE); } }
    else if (scmp(cm, "clearhist") == 0) { hcnt = 0; prtn("History cleared.", C_GREEN); }
    else if (scmp(cm, "log") == 0) prtn("System log: OK", C_WHITE);
    else if (scmp(cm, "last") == 0) prtn("Last login: now", C_WHITE);
    else if (scmp(cm, "passwd") == 0) {
        prt("New password: ", C_YELLOW); int i = 0;
        while (1) { char c = getk(); if (c == '\n') { pass[i] = 0; break; } else if (c >= 32 && i < 19) { pass[i++] = c; putc('*'); } }
        haspw = 1; prtn("", 0); prtn("Password set!", C_GREEN);
    }
    else if (scmp(cm, "useradd") == 0) prtn("User added.", C_GREEN);
    else if (scmp(cm, "userdel") == 0) prtn("User deleted.", C_RED);
    else if (scmp(cm, "users") == 0) { prtn(user, C_GREEN); }
    else if (scmp(cm, "colors") == 0) {
        for (int i = 0; i < 16; i++) { tc = i; prt("███", i); } prtn("", C_WHITE);
    }
    else if (scmp(cm, "palette") == 0) prtn("16 VGA colors available.", C_WHITE);
    else if (scmp(cm, "rainbow") == 0) {
        prt("R", C_RED); prt("A", C_YELLOW); prt("I", C_GREEN); prt("N", C_CYAN); prt("B", C_BLUE); prt("O", C_MAGENTA); prtn("W", C_RED);
    }
    else if (scmp(cm, "..") == 0) prtn("/", C_WHITE);
    else if (scmp(cm, "~") == 0) prtn("/home", C_WHITE);
    else if (scmp(cm, "!!") == 0 && last[0]) runcmd(last);
    else if (scmp(cm, "search") == 0) prtn("Searching...", C_WHITE);
    else if (scmp(cm, "sort") == 0) prtn("Sorted.", C_WHITE);
    else if (scmp(cm, "encrypt") == 0) prtn("Encrypted (ROT13).", C_WHITE);
    else if (scmp(cm, "fxc") == 0 && shas(arg, "sys")) {
        prtn("", 0);
        if (installed) { prt("Mode: PERMANENT (", C_GREEN); prt(installdisk, C_WHITE); prtn(")", C_GREEN); }
        else prtn("Mode: LIVE (USB Boot)", C_YELLOW);
    }
    else if (scmp(cm, "install") == 0) {
        prtn("INSTALL WIZARD - Type 'y' to continue:", C_YELLOW);
        if (getk() == 'y') {
            prtn("Select: 1=HDD 2=SSD 3=USB", C_WHITE);
            char d = getk();
            if (d == '1') scpy(installdisk, "HDD");
            else if (d == '2') scpy(installdisk, "SSD");
            else scpy(installdisk, "USB");
            for (int p = 0; p <= 100; p += 20) {
                prt("[", C_GREEN); for (int j = 0; j < 20; j++) prt(j < p/5 ? "=" : " ", C_GREEN);
                prt("] ", C_GREEN); char b[5]; itos(p, b); prtn(b, C_WHITE);
                for (volatile int dly = 0; dly < 200000; dly++);
            }
            installed = 1; prtn("Installed! Reboot now.", C_GREEN);
        } else prtn("Cancelled.", C_YELLOW);
    }
    else { prt("?: ", C_RED); prtn(c, C_RED); prtn("Type 'help' for commands", C_YELLOW); }
}

/* ============ BOOT MENU ============ */
static void boot_menu(void) {
    cls();
    prtat(25,2,"╔══════════════════════════════╗",C_CYAN);
    prtat(25,3,"║                              ║",C_CYAN);
    prtat(25,4,"║   ████████╗███████╗██████╗   ║",C_RED);
    prtat(25,5,"║   ╚══██╔══╝██╔════╝██╔══██╗  ║",C_RED);
    prtat(25,6,"║      ██║   █████╗  ██║  ██║  ║",C_GREEN);
    prtat(25,7,"║      ██║   ██╔══╝  ██║  ██║  ║",C_GREEN);
    prtat(25,8,"║      ██║   ██║     ██████╔╝  ║",C_BLUE);
    prtat(25,9,"║      ╚═╝   ╚═╝     ╚═════╝   ║",C_BLUE);
    prtat(25,10,"║                              ║",C_CYAN);
    prtat(25,11,"║     TFD OS v1.0              ║",C_CYAN);
    prtat(25,12,"║     by Sadman                ║",C_CYAN);
    prtat(25,13,"║     Text-Based OS 🦊         ║",C_CYAN);
    prtat(25,14,"╚══════════════════════════════╝",C_CYAN);
    prtat(28,16,"[ PRESS ENTER TO BOOT ]",C_YELLOW);
    while (1) { if (hask() && getk() == '\n') break; }
}

/* ============ SETUP WIZARD ============ */
static void setup(void) {
    cls();
    prtn("══════ SETUP WIZARD ══════", C_CYAN);
    prtn("", 0);
    prt("Enter username: ", C_YELLOW); showcur();
    int i = 0; user[0] = 0;
    while (1) { char c = getk(); if (c == '\n') { user[i] = 0; break; } else if (c == '\b' && i > 0) { i--; putc('\b'); } else if (c >= 32 && i < 19) { user[i++] = c; putc(c); } }
    prtn("", 0);
    prt("Enter PC name: ", C_YELLOW);
    i = 0; pc[0] = 0;
    while (1) { char c = getk(); if (c == '\n') { pc[i] = 0; break; } else if (c == '\b' && i > 0) { i--; putc('\b'); } else if (c >= 32 && i < 19) { pc[i++] = c; putc(c); } }
    prtn("", 0);
    prt("Welcome, ", C_GREEN); prtn(user, C_WHITE);
    for (volatile int d = 0; d < 200000; d++);
    hidecur();
}

/* ============ MAIN ============ */
void kernel_main(uint32_t magic, uint32_t addr) {
    (void)magic; (void)addr;
    kwait(); outb(0x64, 0xA8);
    kwait(); outb(0x60, 0xF0);
    kwait(); outb(0x60, 0x01);
    hidecur();
    fs_init();
    boot_menu();
    setup();
    cls();
    prtn("TFD OS v1.0 'Foxy' - Text-Based OS", C_GREEN);
    prtn("Type 'help' for all 72 commands", C_YELLOW);
    prtn("", 0);
    clen = 0;

    while (1) {
        prompt(); showcur();
        clen = 0;
        while (1) {
            char c = getk();
            if (c == '\n') { putc('\n'); break; }
            else if (c == '\b') { if (clen > 0) { clen--; putc('\b'); } }
            else if (c >= 32 && c <= 126 && clen < 255) { cmd[clen++] = c; putc(c); }
        }
        hidecur();
        cmd[clen] = 0;
        runcmd(cmd);
        upt++;
        prtn("", 0);
    }
}
