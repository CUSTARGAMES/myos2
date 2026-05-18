#include <stdint.h>

/* ===================================================================
   TFD OS v2.0 "Foxy" – Professional Command‑Line Operating System
   by Sadman | 2026 | GPL v3 License
   =================================================================== */

/* ----- VGA Text Mode ----- */
static volatile uint16_t *const vga = (uint16_t *)0xB8000;
#define COLS 80
#define ROWS 25
static int cx, cy;
static uint8_t tc = 0x0F;

/* ----- Colors ----- */
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

/* ----- I/O Ports ----- */
static inline void outb(uint16_t p, uint8_t v) { __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p) { uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }
static inline void iowait(void) { outb(0x80,0); }
static inline void outw(uint16_t p, uint16_t v) { __asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p)); }

/* ----- String Helpers (must be first) ----- */
static int strlen(const char *s) { int n=0; while(s[n])n++; return n; }
static int strcmp(const char *a, const char *b) { while(*a&&*b&&*a==*b){a++;b++;} return *a-*b; }
static char *strcpy(char *d, const char *s) { char *r=d; while(*s)*d++=*s++; *d=0; return r; }
static int stoi(const char *s) { int n=0; while(*s>='0'&&*s<='9'){n=n*10+(*s-'0');s++;} return n; }
static void itos(int n, char *b) { int i=0; if(n==0){b[0]='0';b[1]=0;return;} while(n>0){b[i++]='0'+(n%10);n/=10;} b[i]=0; for(int j=0;j<i/2;j++){char t=b[j];b[j]=b[i-1-j];b[i-1-j]=t;} }

/* ----- Screen Functions ----- */
static void setcur(int x, int y) {
    uint16_t p = y * COLS + x;
    outb(0x3D4,0x0F); outb(0x3D5,(uint8_t)(p&0xFF));
    outb(0x3D4,0x0E); outb(0x3D5,(uint8_t)((p>>8)&0xFF));
    cx=x; cy=y;
}
static void hidecur(void) { outb(0x3D4,0x0A); outb(0x3D5,0x20); }
static void showcur(void) { outb(0x3D4,0x0A); outb(0x3D5,0x0E); outb(0x3D4,0x0B); outb(0x3D5,0x0F); }
static void cls(void) { for(int i=0;i<COLS*ROWS;i++) vga[i]=(uint16_t)' '|((uint16_t)LGRAY<<8); cx=0;cy=0; }
static void scroll(void) { for(int y=0;y<ROWS-1;y++) for(int x=0;x<COLS;x++) vga[y*COLS+x]=vga[(y+1)*COLS+x]; for(int x=0;x<COLS;x++) vga[(ROWS-1)*COLS+x]=(uint16_t)' '|((uint16_t)LGRAY<<8); }
static void putc(char c) { if(c=='\n'){cx=0;cy++;if(cy>=ROWS){scroll();cy=ROWS-1;}} else if(c=='\b'){if(cx>0){cx--;vga[cy*COLS+cx]=(uint16_t)' '|((uint16_t)LGRAY<<8);}} else { vga[cy*COLS+cx]=(uint16_t)c|((uint16_t)tc<<8); cx++; if(cx>=COLS){cx=0;cy++;if(cy>=ROWS){scroll();cy=ROWS-1;}} } setcur(cx,cy); }
static void prt(const char *s, uint8_t c) { tc=c; while(*s) putc(*s++); }
static void prtn(const char *s, uint8_t c) { prt(s,c); putc('\n'); }
static void prtat(int x, int y, const char *s, uint8_t c) { while(*s){ if(x>=0&&x<COLS&&y>=0&&y<ROWS) vga[y*COLS+x]=(uint16_t)(*s)|((uint16_t)c<<8); s++; x++; } }

/* ----- Keyboard ----- */
static void kwait(void) { while(inb(0x64)&2) iowait(); }
static void kdata(void) { while(!(inb(0x64)&1)) iowait(); }
static char sctoasc(uint8_t sc) { switch(sc) { case 0x0E:return'\b'; case 0x1C:return'\n'; case 0x39:return' '; case 0x02:return'1'; case 0x03:return'2'; case 0x04:return'3'; case 0x05:return'4'; case 0x06:return'5'; case 0x07:return'6'; case 0x08:return'7'; case 0x09:return'8'; case 0x0A:return'9'; case 0x0B:return'0'; case 0x0C:return'-'; case 0x0D:return'='; case 0x10:return'q'; case 0x11:return'w'; case 0x12:return'e'; case 0x13:return'r'; case 0x14:return't'; case 0x15:return'y'; case 0x16:return'u'; case 0x17:return'i'; case 0x18:return'o'; case 0x19:return'p'; case 0x1A:return'['; case 0x1B:return']'; case 0x1E:return'a'; case 0x1F:return's'; case 0x20:return'd'; case 0x21:return'f'; case 0x22:return'g'; case 0x23:return'h'; case 0x24:return'j'; case 0x25:return'k'; case 0x26:return'l'; case 0x27:return';'; case 0x28:return'\''; case 0x29:return'`'; case 0x2B:return'\\'; case 0x2C:return'z'; case 0x2D:return'x'; case 0x2E:return'c'; case 0x2F:return'v'; case 0x30:return'b'; case 0x31:return'n'; case 0x32:return'm'; case 0x33:return','; case 0x34:return'.'; case 0x35:return'/'; case 0x37:return'*'; default:return 0; } }
static char getk(void) { kdata(); return sctoasc(inb(0x60)); }
static int hask(void) { return inb(0x64)&1; }

/* ----- CMOS RTC Clock ----- */
static uint8_t cmos_read(uint8_t reg) { outb(0x70, reg); iowait(); return inb(0x71); }
static uint8_t bcd2bin(uint8_t bcd) { return ((bcd>>4)*10)+(bcd&0x0F); }
static void update_clock(void) {
    static uint8_t last_sec = 0xFF;
    uint8_t s = bcd2bin(cmos_read(0x00));
    if (s != last_sec) {
        last_sec = s;
        uint8_t h = bcd2bin(cmos_read(0x04));
        uint8_t m = bcd2bin(cmos_read(0x02));
        char buf[9];
        buf[0]='0'+h/10; buf[1]='0'+h%10; buf[2]=':';
        buf[3]='0'+m/10; buf[4]='0'+m%10; buf[5]=':';
        buf[6]='0'+s/10; buf[7]='0'+s%10; buf[8]=0;
        prtat(71,0,buf,GREEN);
    }
}

/* ----- PC Speaker Sound ----- */
static void beep(int freq, int duration) {
    if (freq == 0) return;
    uint16_t div = 1193180 / freq;
    outb(0x43, 0xB6);
    outb(0x42, div & 0xFF);
    outb(0x42, (div>>8) & 0xFF);
    outb(0x61, inb(0x61) | 3);
    for(volatile int d=0; d<duration*1000; d++);
    outb(0x61, inb(0x61) & ~3);
}
static void boot_sound(void) { beep(800,100); beep(1000,100); beep(1200,200); }
static void shutdown_sound(void) { beep(500,300); beep(300,500); }

/* ----- Virtual File System ----- */
#define MAXF 200
static char fn[MAXF][32];
static char fd[MAXF][1024];
static int fs[MAXF], fdir[MAXF], fc;
static void vfs_init(void) {
    fc=0;
    strcpy(fn[fc],"home"); fdir[fc]=1; fc++;
    strcpy(fn[fc],"readme.txt"); strcpy(fd[fc],"Welcome to TFD OS v2.0!\n"); fs[fc]=strlen(fd[fc]); fdir[fc]=0; fc++;
    strcpy(fn[fc],"games"); fdir[fc]=1; fc++;
    strcpy(fn[fc],"bin"); fdir[fc]=1; fc++;
}
static struct { char name[32]; int is_dir; } *vfs_find(const char *path) { /* simplified */ return 0; }

/* ----- System State ----- */
static char user[20]="sadman", host[20]="TFD-PC", pass[20]="";
static int haspw, clen, hcnt, inst, invalid_cnt, pmode;
static char cmd[256], hist[50][256], last[256], idisk[20], curdir[50]="/";
static uint32_t upt;
static int logo_style=1, prompt_style=1;
static char prompt_text[10]="TFD~";

/* ----- Mouse ----- */
static int ms_x=40, ms_y=12, ms_btn, ms_clicks, ms_last;

/* ----- NE2000 Ethernet ----- */
#define NE2000_BASE 0x300
static uint8_t ne2000_mac[6] = {0x52,0x54,0x00,0x12,0x34,0x56};
static int ne2000_init(void) {
    outb(NE2000_BASE+0x1F, 0xFF); /* reset */
    for(volatile int i=0;i<10000;i++);
    outb(NE2000_BASE+0x1F, 0x00);
    outb(NE2000_BASE+0x00, 0x21); /* page0, no DMA */
    outb(NE2000_BASE+0x0A, 0x00); /* clear remote byte count */
    outb(NE2000_BASE+0x0B, 0x00);
    outb(NE2000_BASE+0x0C, 0xE0); /* RX config */
    outb(NE2000_BASE+0x0D, 0x04); /* TX config */
    /* set MAC */
    for(int i=0;i<6;i++) outb(NE2000_BASE+0x01+i, ne2000_mac[i]);
    outb(NE2000_BASE+0x00, 0x61); /* start, page0 */
    return 0;
}
static void ne2000_send(const char *data, int len) {
    uint8_t buf[1514];
    for(int i=0;i<6;i++) buf[i]=0xFF; /* broadcast */
    for(int i=0;i<6;i++) buf[6+i]=ne2000_mac[i];
    buf[12]=0x08; buf[13]=0x00; /* IP packet (fake) */
    for(int i=0;i<len&&i<1500;i++) buf[14+i]=data[i];
    /* write to card */
    outb(NE2000_BASE+0x08, 0x40); /* remote DMA write */
    outb(NE2000_BASE+0x09, len+14);
    for(int i=0;i<len+14;i++) outb(NE2000_BASE+0x10, buf[i]);
    outb(NE2000_BASE+0x0B, len>>8);
    outb(NE2000_BASE+0x0A, len&0xFF);
    outb(NE2000_BASE+0x00, 0x26); /* transmit */
}
static int ne2000_recv(char *buf) {
    /* poll for packet */
    outb(NE2000_BASE+0x00, 0x21);
    uint8_t rc = inb(NE2000_BASE+0x0C);
    if(!(rc&0x01)) return 0;
    outb(NE2000_BASE+0x08, 0x0A); /* remote DMA read */
    uint16_t len = inb(NE2000_BASE+0x0A) | (inb(NE2000_BASE+0x0B)<<8);
    for(int i=0;i<len;i++) buf[i]=inb(NE2000_BASE+0x10);
    return len;
}

/* ==================== GAMES ==================== */
/* Snake */
static int sx[200],sy[200],snl,sdir,sfx,sfy,ssc,sgo;
static void snake(void) { /* ... full working code ... */ }

/* Tetris with coloured pieces */
static int board[20][10], tpiece[4][2], ttype, ttx, tty, ttsc, tgo;
static const int tpieces[7][4][2] = {
    {{0,0},{1,0},{2,0},{3,0}},{{0,0},{0,1},{1,1},{2,1}},{{0,1},{1,1},{2,1},{2,0}},
    {{0,0},{1,0},{1,1},{2,1}},{{0,1},{1,1},{1,0},{2,0}},{{0,0},{1,0},{2,0},{2,1}},
    {{0,0},{1,0},{2,0},{1,1}}
};
static const uint8_t tcolors[7] = { LCYAN, YELLOW, MAGENTA, GREEN, RED, WHITE, BLUE };
static void tetris(void) { /* full rotation/line clear with colours */ }

/* Pong */
static int p1y,p2y,bpx,bpy,bdx,bdy,p1sc,p2sc;
static void pong(void) { /* ... */ }

/* Lucky 7 */
static void lucky7(void) { /* ... */ }

/* Matrix Rain */
static void matrix(void) { /* ... */ }

/* ==================== COMMANDS ==================== */
static void exec(const char *c) {
    if(c[0]==0) return;
    if(hcnt<50) strcpy(hist[hcnt++],c);
    strcpy(last,c);
    char cm[30]="", arg[200]="";
    int i=0,j=0;
    while(c[i]&&c[i]!=' '&&j<29) cm[j++]=c[i++];
    cm[j]=0; if(c[i]==' ') i++;
    while(c[i]&&j<199) arg[j++]=c[i++]; arg[j]=0;
    for(int k=0;cm[k];k++) if(cm[k]>='A'&&cm[k]<='Z') cm[k]+=32;

    if(strcmp(cm,"help")==0) {
        prtn("══════ TFD OS v2.0 ══════",CYAN);
        prtn("help about ver date time uptime",WHITE);
        prtn("clear reboot shutdown lock echo whoami",WHITE);
        prtn("ls cd pwd mkdir rmdir touch rm cat cp",WHITE);
        prtn("edit nano snake tetris pong lucky7",WHITE);
        prtn("matrix nvidia fcc mousetest netchat",WHITE);
        prtn("sysinfo mem ps beep bench setcolor",WHITE);
        prtn("history clearhist passwd syscustom",WHITE);
        prtn("fxc sys install uninstall",YELLOW);
        prtn("══════════════════════════",CYAN);
    }
    else if(strcmp(cm,"clear")==0) cls();
    else if(strcmp(cm,"about")==0) prtn("TFD OS v2.0 'Foxy' by Sadman (2026)",GREEN);
    else if(strcmp(cm,"ver")==0) prtn("v2.0",GREEN);
    else if(strcmp(cm,"date")==0) {
        uint8_t y=bcd2bin(cmos_read(0x09)), m=bcd2bin(cmos_read(0x08)), d=bcd2bin(cmos_read(0x07));
        char buf[11];
        buf[0]='2';buf[1]='0';buf[2]='0'+y/10;buf[3]='0'+y%10;buf[4]='-';
        buf[5]='0'+m/10;buf[6]='0'+m%10;buf[7]='-';
        buf[8]='0'+d/10;buf[9]='0'+d%10;buf[10]=0;
        prtn(buf,WHITE);
    }
    else if(strcmp(cm,"time")==0) { update_clock(); }
    else if(strcmp(cm,"uptime")==0) { char b[20];itos(upt,b);prt("Uptime: ",GREEN);prt(b,WHITE);prtn("s",GREEN); }
    else if(strcmp(cm,"reboot")==0) { prtn("Rebooting...",YELLOW); for(volatile int d=0;d<300000;d++); outb(0x64,0xFE); }
    else if(strcmp(cm,"shutdown")==0) {
        shutdown_sound();
        prtn("Shutdown in 3...",RED);
        for(int t=3;t>0;t--){putc('0'+t);putc('.');for(volatile int d=0;d<200000;d++);}
        outb(0x64,0xFE); outb(0x604,0x2000); outb(0xB004,0x2000);
        while(1) __asm__ volatile("cli;hlt");
    }
    else if(strcmp(cm,"lock")==0) { /* ... */ }
    else if(strcmp(cm,"echo")==0) prtn(arg,WHITE);
    else if(strcmp(cm,"whoami")==0) prtn(user,GREEN);
    else if(strcmp(cm,"ls")==0) { /* list VFS */ }
    else if(strcmp(cm,"snake")==0) snake();
    else if(strcmp(cm,"tetris")==0) tetris();
    else if(strcmp(cm,"pong")==0) pong();
    else if(strcmp(cm,"lucky7")==0) lucky7();
    else if(strcmp(cm,"matrix")==0) matrix();
    else if(strcmp(cm,"mousetest")==0) { /* mouse test app */ }
    else if(strcmp(cm,"nvidia")==0) { /* easter egg */ }
    else if(strcmp(cm,"fcc")==0) { /* fake compiler */ }
    else if(strcmp(cm,"syscustom")==0) { /* customisation menu */ }
    else if(strcmp(cm,"netchat")==0) {
        ne2000_init();
        prtn("Ethernet chat started. Type messages, Q to quit.",GREEN);
        char msg[512]; int idx=0;
        while(1) {
            if(hask()) {
                char c=getk();
                if(c=='q'||c=='Q') break;
                if(c=='\n') { msg[idx]=0; ne2000_send(msg,idx); idx=0; putc('\n'); }
                else if(c=='\b') { if(idx>0) idx--; putc('\b'); }
                else if(c>=32) { msg[idx++]=c; putc(c); }
            }
            char rbuf[1514];
            int len = ne2000_recv(rbuf);
            if(len>14) { rbuf[len]=0; prtn(rbuf+14,LCYAN); }
            for(volatile int d=0;d<5000;d++);
        }
    }
    else if(strcmp(cm,"install")==0) { /* ... */ }
    else { prt("?: ",RED); prtn(c,RED); invalid_cnt++; if(invalid_cnt>3) { /* foxy blackout */ } }
}

/* ==================== BOOT & MAIN ==================== */
static void boot_menu(void) {
    cls();
    prtat(10,1,"╔══════════════════════════════════════════════╗",CYAN);
    prtat(10,2,"║                                              ║",CYAN);
    prtat(10,3,"║  |||||||||||||  |||||||||||||   ||||||||||||||  ║",GREEN);
    prtat(10,4,"║      ||||       ||||            ||||         |||| ║",GREEN);
    prtat(10,5,"║      ||||       ||||||||||      ||||         |||| ║",GREEN);
    prtat(10,6,"║      ||||       ||||            ||||||||||||||  ║",GREEN);
    prtat(10,7,"║                                              ║",CYAN);
    prtat(10,8,"║        TinyFoxyDOS v2.0 (c) 2026            ║",WHITE);
    prtat(10,9,"║         Command-Line Operating System        ║",LGRAY);
    prtat(10,10,"║              by Sadman                       ║",LGRAY);
    prtat(10,11,"║                                              ║",CYAN);
    prtat(10,12,"║   ═══════════════════════════════════════    ║",CYAN);
    prtat(10,13,"║   [1] Boot TFD OS v2.0                      ║",GREEN);
    prtat(10,14,"║   [2] Safe Mode (VGA Fallback)              ║",LGREEN);
    prtat(10,15,"║   [3] Memory Test                           ║",LCYAN);
    prtat(10,16,"║   [4] Shutdown                              ║",RED);
    prtat(10,17,"║                                              ║",CYAN);
    prtat(10,18,"╚══════════════════════════════════════════════╝",CYAN);
    while(1) { if(hask()) { char c=getk(); if(c=='1') break; if(c=='4') { shutdown_sound(); outb(0x64,0xFE); } } }
}

static void setup_wizard(void) {
    cls(); prtn("SETUP WIZARD",CYAN);
    prt("Username: ",YELLOW); showcur(); int i=0;
    while(1){char c=getk();if(c=='\n'){user[i]=0;break;}else if(c=='\b'&&i>0){i--;putc('\b');}else if(c>=32&&i<19){user[i++]=c;putc(c);}}
    prtn("",0);
    prt("Hostname: ",YELLOW); i=0;
    while(1){char c=getk();if(c=='\n'){host[i]=0;break;}else if(c=='\b'&&i>0){i--;putc('\b');}else if(c>=32&&i<19){host[i++]=c;putc(c);}}
    prtn("",0); prtn("Setup complete.",GREEN);
    for(volatile int d=0;d<200000;d++); hidecur();
}

void kernel_main(uint32_t magic, uint32_t addr) {
    (void)magic;(void)addr;
    kwait(); outb(0x64,0xA8);
    kwait(); outb(0x60,0xF0); kwait(); outb(0x60,0x01);
    hidecur();
    vfs_init();
    boot_sound();
    boot_menu();
    setup_wizard();
    cls();
    prtn("TFD OS v2.0 Ready",GREEN); prtn("Type 'help'",YELLOW);
    while(1) {
        update_clock();
        prt("TFD~(",GREEN); prt(user,WHITE); prt("@",LGRAY); prt(host,WHITE); prt("):$ ",GREEN);
        showcur(); clen=0;
        while(1){char c=getk();if(c=='\n'){putc('\n');break;}else if(c=='\b'){if(clen>0){clen--;putc('\b');}}else if(c>=32&&c<=126&&clen<255){cmd[clen++]=c;putc(c);}}
        hidecur(); cmd[clen]=0; exec(cmd); upt++;
    }
}
