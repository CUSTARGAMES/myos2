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

/* ----- CMOS RTC ----- */
static uint8_t cmos_read(uint8_t reg) { outb(0x70, reg); iowait(); return inb(0x71); }
static uint8_t bcd2bin(uint8_t bcd) { return ((bcd>>4)*10)+(bcd&0x0F); }

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
static void boot_melody(void) { beep(800,100); beep(1000,100); beep(1200,200); }
static void shutdown_melody(void) { beep(500,300); beep(300,500); }

/* ----- Virtual File System (simple) ----- */
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

/* ----- System State ----- */
static char user[20]="sadman", host[20]="TFD-PC", pass[20]="";
static int haspw, clen, hcnt, inst, invalid_cnt, pmode;
static char cmd[256], hist[50][256], last[256], idisk[20], curdir[50]="/";
static uint32_t upt;
static int logo_style=1;
static char prompt_text[10]="TFD~";

/* ----- Mouse Driver ----- */
static int ms_x=40, ms_y=12, ms_btn, ms_clicks, ms_last;
static void mouse_poll(void) {
    static int cycle;
    static uint8_t bytes[3];
    if(!(inb(0x64)&0x20)) return;
    uint8_t d = inb(0x60);
    if(cycle==0) { if(d&0x08) { bytes[0]=d; cycle=1; } }
    else { bytes[cycle++]=d; if(cycle==3) { cycle=0;
        int dx=bytes[1], dy=bytes[2];
        if(bytes[0]&0x10) dx|=~0xFF;
        if(bytes[0]&0x20) dy|=~0xFF;
        dy=-dy; ms_x+=dx/2; ms_y+=dy/2;
        if(ms_x<1)ms_x=1; if(ms_x>78)ms_x=78;
        if(ms_y<4)ms_y=4; if(ms_y>20)ms_y=20;
        int nb=bytes[0]&0x07;
        if(nb!=ms_btn) { ms_btn=nb; if(nb&1){ms_last=1;ms_clicks++;} else if(nb&2){ms_last=2;ms_clicks++;} }
    }}}
}

/* ----- ATA PIO Driver ----- */
#define ATA_DATA       0x1F0
#define ATA_ERROR      0x1F1
#define ATA_SECTORS    0x1F2
#define ATA_LBA_LO     0x1F3
#define ATA_LBA_MID    0x1F4
#define ATA_LBA_HI     0x1F5
#define ATA_DRIVE_SEL  0x1F6
#define ATA_COMMAND    0x1F7
#define ATA_STATUS     0x1F7

static int ata_read_sector(uint32_t lba, uint8_t *buf) {
    outb(ATA_DRIVE_SEL, 0xE0 | ((lba>>24)&0x0F));
    outb(ATA_SECTORS, 1);
    outb(ATA_LBA_LO, lba & 0xFF);
    outb(ATA_LBA_MID, (lba>>8)&0xFF);
    outb(ATA_LBA_HI, (lba>>16)&0xFF);
    outb(ATA_COMMAND, 0x20);
    while((inb(ATA_STATUS)&0x80)!=0) iowait();
    for(int i=0;i<256;i++) {
        uint16_t w = (uint16_t)inb(ATA_DATA) | ((uint16_t)inb(ATA_DATA)<<8);
        buf[i*2] = w & 0xFF;
        buf[i*2+1] = (w>>8) & 0xFF;
    }
    return 0;
}
static int ata_write_sector(uint32_t lba, const uint8_t *buf) {
    outb(ATA_DRIVE_SEL, 0xE0 | ((lba>>24)&0x0F));
    outb(ATA_SECTORS, 1);
    outb(ATA_LBA_LO, lba & 0xFF);
    outb(ATA_LBA_MID, (lba>>8)&0xFF);
    outb(ATA_LBA_HI, (lba>>16)&0xFF);
    outb(ATA_COMMAND, 0x30);
    while((inb(ATA_STATUS)&0x80)!=0) iowait();
    for(int i=0;i<256;i++) {
        uint16_t w = (uint16_t)buf[i*2] | ((uint16_t)buf[i*2+1]<<8);
        outb(ATA_DATA, w & 0xFF);
        outb(ATA_DATA, (w>>8)&0xFF);
    }
    outb(ATA_COMMAND, 0xE7);
    while((inb(ATA_STATUS)&0x80)!=0) iowait();
    return 0;
}

/* ----- NE2000 Ethernet ----- */
#define NE2000_BASE 0x300
static uint8_t ne2000_mac[6] = {0x52,0x54,0x00,0x12,0x34,0x56};
static void ne2000_init(void) {
    outb(NE2000_BASE+0x1F, 0xFF);
    for(volatile int i=0;i<10000;i++);
    outb(NE2000_BASE+0x1F, 0x00);
    outb(NE2000_BASE+0x00, 0x21);
    outb(NE2000_BASE+0x0A, 0x00);
    outb(NE2000_BASE+0x0B, 0x00);
    outb(NE2000_BASE+0x0C, 0xE0);
    outb(NE2000_BASE+0x0D, 0x04);
    for(int i=0;i<6;i++) outb(NE2000_BASE+0x01+i, ne2000_mac[i]);
    outb(NE2000_BASE+0x00, 0x61);
}
static void ne2000_send(const char *data, int len) {
    uint8_t buf[1514];
    for(int i=0;i<6;i++) buf[i]=0xFF;
    for(int i=0;i<6;i++) buf[6+i]=ne2000_mac[i];
    buf[12]=0x08; buf[13]=0x00;
    for(int i=0;i<len;i++) buf[14+i]=data[i];
    outb(NE2000_BASE+0x08, 0x40);
    outb(NE2000_BASE+0x09, len+14);
    for(int i=0;i<len+14;i++) outb(NE2000_BASE+0x10, buf[i]);
    outb(NE2000_BASE+0x0B, len>>8);
    outb(NE2000_BASE+0x0A, len&0xFF);
    outb(NE2000_BASE+0x00, 0x26);
}
static int ne2000_recv(char *buf) {
    outb(NE2000_BASE+0x00, 0x21);
    if(!(inb(NE2000_BASE+0x0C)&0x01)) return 0;
    outb(NE2000_BASE+0x08, 0x0A);
    uint16_t len = inb(NE2000_BASE+0x0A) | (inb(NE2000_BASE+0x0B)<<8);
    for(int i=0;i<len;i++) buf[i]=inb(NE2000_BASE+0x10);
    return len;
}

/* ==================== GAMES ==================== */

/* Snake (full) */
static int sx[200],sy[200],snl,sdir,sfx,sfy,ssc,sgo;
static void snake(void) {
    snl=3; sdir=0; ssc=0; sgo=0;
    sx[0]=40;sy[0]=12; sx[1]=38;sy[1]=12; sx[2]=36;sy[2]=12;
    sfx=50;sfy=12;
    cls();
    for(int x=0;x<COLS;x++){prtat(x,0,"#",GREEN);prtat(x,ROWS-1,"#",GREEN);}
    for(int y=0;y<ROWS;y++){prtat(0,y,"#",GREEN);prtat(COLS-1,y,"#",GREEN);}
    prtat(2,0," SNAKE | Score: 0 | WASD | Q=Quit ",WHITE);
    prtat(sfx,sfy,".",RED);
    for(int i=0;i<snl;i++){prtat(sx[i],sy[i],">",YELLOW);prtat(sx[i]+1,sy[i],">",YELLOW);}
    int tick=0;
    while(!sgo){
        if(hask()){char c=getk();if(c=='w'||c=='W')sdir=3;else if(c=='s'||c=='S')sdir=1;else if(c=='a'||c=='A')sdir=2;else if(c=='d'||c=='D')sdir=0;else if(c=='q'||c=='Q')break;}
        tick++;if(tick<15){for(volatile int d=0;d<5000;d++);continue;}
        tick=0;
        int nx=sx[0],ny=sy[0];
        if(sdir==0)nx+=2;else if(sdir==1)ny++;else if(sdir==2)nx-=2;else ny--;
        if(nx<=0||nx>=COLS-2||ny<=0||ny>=ROWS-1){sgo=1;break;}
        for(int i=0;i<snl;i++) if(sx[i]==nx&&sy[i]==ny){sgo=1;break;}
        if(sgo)break;
        prtat(sx[snl-1],sy[snl-1],"  ",BLACK);
        for(int i=snl-1;i>0;i--){sx[i]=sx[i-1];sy[i]=sy[i-1];}
        sx[0]=nx;sy[0]=ny;
        prtat(sx[0],sy[0],">>",YELLOW);
        if(sx[0]==sfx||(sx[0]+1==sfx&&sy[0]==sfy)){
            snl++;ssc+=10;
            sx[snl-1]=sx[snl-2];sy[snl-1]=sy[snl-2];
            sfx=((inb(0x40)*123+456)%35)*2+5;
            sfy=((inb(0x40)*789+123)%20)+3;
            prtat(sfx,sfy,".",RED);
            char b[10];itos(ssc,b);prtat(16,0,b,YELLOW);
        }
        for(volatile int d=0;d<5000;d++);
    }
    cls();
    if(sgo){prtn("GAME OVER! Score:",RED);char b[10];itos(ssc,b);prtn(b,WHITE);}
    else prtn("Snake exited.",YELLOW);
}

/* Tetris (with colours) */
static int board[20][10], tpiece[4][2], ttype, ttx, tty, ttsc, tgo;
static const int tpieces[7][4][2] = {
    {{0,0},{1,0},{2,0},{3,0}},{{0,0},{0,1},{1,1},{2,1}},{{0,1},{1,1},{2,1},{2,0}},
    {{0,0},{1,0},{1,1},{2,1}},{{0,1},{1,1},{1,0},{2,0}},{{0,0},{1,0},{2,0},{2,1}},
    {{0,0},{1,0},{2,0},{1,1}}
};
static const uint8_t tcolors[7] = { LCYAN, YELLOW, MAGENTA, GREEN, RED, WHITE, BLUE };
static void tetris(void) {
    for(int y=0;y<20;y++)for(int x=0;x<10;x++)board[y][x]=0;
    ttsc=0; ttx=4; tty=0; tgo=0;
    ttype = inb(0x40)%7;
    for(int i=0;i<4;i++){tpiece[i][0]=tpieces[ttype][i][0]; tpiece[i][1]=tpieces[ttype][i][1];}
    cls();
    for(int y=0;y<22;y++){prtat(20,y,"#",GREEN);prtat(43,y,"#",GREEN);}
    for(int x=0;x<12;x++){prtat(x*2+20,0,"#",GREEN);prtat(x*2+20,21,"#",GREEN);}
    prtat(2,0,"TETRIS | Score: 0 | ADSW | Q=Quit",WHITE);
    int tick=0;
    while(!tgo){
        if(hask()){
            char c=getk();
            if(c=='a'||c=='A') ttx--;
            else if(c=='d'||c=='D') ttx++;
            else if(c=='s'||c=='S') tty++;
            else if(c=='w'||c=='W'){
                int np[4][2];
                for(int i=0;i<4;i++){np[i][0]=-tpiece[i][1]; np[i][1]=tpiece[i][0];}
                int ok=1;
                for(int i=0;i<4;i++){
                    int x=ttx+np[i][0], y=tty+np[i][1];
                    if(x<0||x>=10||y>=20||(y>=0&&board[y][x])){ok=0;break;}
                }
                if(ok) for(int i=0;i<4;i++){tpiece[i][0]=np[i][0]; tpiece[i][1]=np[i][1];}
            }
            else if(c=='q'||c=='Q') break;
        }
        tick++; if(tick<15){for(volatile int d=0;d<4000;d++);continue;}
        tick=0; tty++;
        int hit=0;
        for(int i=0;i<4;i++){
            int x=ttx+tpiece[i][0], y=tty+tpiece[i][1];
            if(y>=20||(y>=0&&board[y][x])){hit=1;break;}
        }
        if(hit){
            for(int i=0;i<4;i++){
                int x=ttx+tpiece[i][0], y=tty+tpiece[i][1]-1;
                if(y>=0&&y<20&&x>=0&&x<10) board[y][x]=1;
            }
            for(int y=19;y>=0;y--){
                int full=1;
                for(int x=0;x<10;x++) if(!board[y][x]){full=0;break;}
                if(full){
                    ttsc+=100;
                    for(int yy=y;yy>0;yy--) for(int x=0;x<10;x++) board[yy][x]=board[yy-1][x];
                    for(int x=0;x<10;x++) board[0][x]=0;
                    y++;
                }
            }
            for(int y=0;y<20;y++) for(int x=0;x<10;x++)
                prtat(20+x*2,y+1,board[y][x]?"||":"  ",board[y][x]?WHITE:BLACK);
            ttx=4; tty=0; ttype=inb(0x40)%7;
            for(int i=0;i<4;i++){tpiece[i][0]=tpieces[ttype][i][0]; tpiece[i][1]=tpieces[ttype][i][1];}
            if(board[0][4]) tgo=1;
            char b[10];itos(ttsc,b);prtat(16,0,b,YELLOW);
        } else {
            for(int i=0;i<4;i++){
                int px=20+(ttx+tpiece[i][0])*2, py=tty+tpiece[i][1]+1;
                if(py>0) prtat(px,py,"||",tcolors[ttype]);
            }
        }
        for(volatile int d=0;d<4000;d++);
    }
    cls();
    if(tgo){prtn("GAME OVER! Score:",RED);char b[10];itos(ttsc,b);prtn(b,WHITE);}
    else prtn("Tetris exited.",YELLOW);
}

/* Pong */
static int p1y,p2y,bpx,bpy,bdx,bdy,p1sc,p2sc;
static void pong(void) {
    p1y=9;p2y=9;bpx=40;bpy=12;bdx=1;bdy=1;p1sc=0;p2sc=0;
    cls();
    for(int x=0;x<COLS;x++){prtat(x,0,"#",GREEN);prtat(x,ROWS-1,"#",GREEN);}
    prtat(2,0,"PONG | W/S=P1 O/L=P2 Q=Quit",WHITE);
    prtat(30,0,"0",LRED);prtat(48,0,"0",LBLUE);
    for(int i=-2;i<=2;i++){prtat(2,p1y+i,"|",LRED);prtat(77,p2y+i,"|",LBLUE);}
    prtat(bpx,bpy,"o",WHITE);
    while(1){
        if(hask()){
            char c=getk();
            if(c=='w'||c=='W'){if(p1y>4)p1y--;}
            else if(c=='s'||c=='S'){if(p1y<17)p1y++;}
            else if(c=='o'||c=='O'){if(p2y>4)p2y--;}
            else if(c=='l'||c=='L'){if(p2y<17)p2y++;}
            else if(c=='q'||c=='Q') break;
        }
        prtat(bpx,bpy," ",BLACK);
        bpx+=bdx; bpy+=bdy;
        if(bpy<=1)bdy=1; if(bpy>=ROWS-2)bdy=-1;
        if(bpx<=4&&bpy>=p1y-2&&bpy<=p1y+2)bdx=1;
        if(bpx>=74&&bpy>=p2y-2&&bpy<=p2y+2)bdx=-1;
        if(bpx<=0){p2sc++;bpx=40;bpy=12;bdx=1; char b[4];itos(p2sc,b);prtat(48,0,b,LBLUE);}
        if(bpx>=79){p1sc++;bpx=40;bpy=12;bdx=-1; char b[4];itos(p1sc,b);prtat(30,0,b,LRED);}
        prtat(bpx,bpy,"o",WHITE);
        for(int i=-3;i<=3;i++){prtat(2,p1y+i," ",BLACK);prtat(77,p2y+i," ",BLACK);}
        for(int i=-2;i<=2;i++){prtat(2,p1y+i,"|",LRED);prtat(77,p2y+i,"|",LBLUE);}
        for(volatile int d=0;d<8000;d++);
    }
    cls(); prtn("Pong exited.",YELLOW);
}

/* Lucky 7 */
static void lucky7(void) {
    int credits=100, bet=10;
    const char symbols[]={'7','*','@','$','!','X'};
    const int payouts[]={100,50,25,10,5,0};
    while(credits>0){
        cls();
        prtat(25,2,"╔══════════════════════════════╗",YELLOW);
        prtat(25,3,"║       🎰 LUCKY 7 🎰        ║",YELLOW);
        prtat(25,4,"║      TFD OS Casino          ║",YELLOW);
        prtat(25,5,"╚══════════════════════════════╝",YELLOW);
        prtat(26,7,"[?] [?] [?]",WHITE);
        char status[40];
        strcpy(status,"Credits: "); itos(credits,status+9);
        strcpy(status+strlen(status),"  Bet: "); itos(bet,status+strlen(status));
        prtat(22,10,status,WHITE);
        prtat(18,12,"ENTER=Spin +/-=Bet Q=Quit",LGRAY);
        char c=getk();
        if(c=='q'||c=='Q') break;
        if(c=='+'){if(bet<credits)bet+=10;}
        else if(c=='-'){if(bet>10)bet-=10;}
        else if(c=='\n'){
            int s1=inb(0x40)%6, s2=inb(0x40)%6, s3=inb(0x40)%6;
            prtat(26,7,"   ",WHITE);
            char line[20];
            line[0]='[';line[1]=symbols[s1];line[2]=']';line[3]=' ';line[4]='[';line[5]=symbols[s2];line[6]=']';line[7]=' ';line[8]='[';line[9]=symbols[s3];line[10]=']';line[11]=0;
            prtat(26,7,line,WHITE);
            if(s1==s2&&s2==s3){ int win=bet*payouts[s1]; credits+=win; prtat(22,9,s1==0?"JACKPOT!!!":"WINNER!",s1==0?YELLOW:GREEN); }
            else if(s1==s2||s2==s3||s1==s3){ credits+=bet; prtat(24,9,"Two match! +1x",CYAN); }
            else { credits-=bet; prtat(24,9,"No match. Lost bet.",RED); }
            for(volatile int d=0;d<100000;d++);
        }
    }
    cls(); prtn("Thanks for playing Lucky 7!",YELLOW);
}

/* Matrix Rain */
static void matrix(void) {
    cls();
    prtat(25,12,"Press ENTER to start, ESC to quit",YELLOW);
    while(1){if(hask()&&getk()=='\n')break;}
    cls();
    int drops[COLS];
    for(int x=0;x<COLS;x++) drops[x] = (inb(0x40)+x) % ROWS;
    int running=1;
    while(running){
        if(hask()){ if(getk()==0x01) running=0; }
        for(int x=0;x<COLS;x+=2){
            int y=drops[x];
            if(y>=0&&y<ROWS){
                char ch=(inb(0x40)%94)+33;
                prtat(x,y,(char[]){ch,0},LGREEN);
            }
            if(y-1>=0) prtat(x,y-1," ",BLACK);
            if(y-2>=0){
                char ch2=(inb(0x40)%94)+33;
                prtat(x,y-2,(char[]){ch2,0},GREEN);
            }
            if(y+1<ROWS) prtat(x,y+1," ",BLACK);
            drops[x]++;
            if(drops[x]>ROWS+5) drops[x]=-5;
        }
        for(volatile int d=0;d<3000;d++);
    }
    cls();
}

/* NVIDIA Easter Egg */
static void nvidia_egg(void) {
    cls();
    prtat(25,4,"╔══════════════════════════════╗",LRED);
    prtat(25,5,"║     Linus says:              ║",WHITE);
    prtat(25,6,"║       ,,,                    ║",WHITE);
    prtat(25,7,"║      (o o)                   ║",WHITE);
    prtat(25,8,"║  ---ooO-(_)-Ooo---           ║",WHITE);
    prtat(25,9,"║                              ║",LRED);
    prtat(25,10,"║     F**K YOU NVIDIA!         ║",GREEN);
    prtat(25,11,"╚══════════════════════════════╝",LRED);
    prtat(20,14,"Press any key...",LGRAY);
    while(!hask()) iowait(); getk();
    cls();
}

/* FCC Fake Compiler */
static void fcc(void) {
    cls();
    prtn("══════ FCC - Foxy C Compiler v1.0 ══════",CYAN);
    prtn("Compiling: hello.c",WHITE);
    for(int i=0;i<=100;i+=20){
        prt("[",GREEN);
        for(int j=0;j<20;j++) prt(j<i/5?"=":" ",GREEN);
        prt("] ",GREEN);
        char b[5]; itos(i,b); prtn(b,WHITE);
        for(volatile int d=0;d<200000;d++);
    }
    prtn("Output: hello.oap",GREEN);
    prtn("Status: Compilation successful!",GREEN);
    prtn("Note: FCC is a showcase compiler.",YELLOW);
    prtn("══════════════════════════════════════",CYAN);
}

/* Mousetest */
static void mousetest(void) {
    ms_x=40; ms_y=12; ms_clicks=0; ms_last=0;
    kwait(); outb(0x64,0xA8);
    outb(0x64,0xD4); outb(0x60,0xFF);
    for(volatile int i=0;i<10000;i++);
    while(inb(0x64)&1) inb(0x60);
    outb(0x64,0xD4); outb(0x60,0xF4);
    cls();
    int running=1;
    while(running){
        mouse_poll();
        if(hask()){ if(getk()=='q') running=0; }
        prtat(0,0,"╔══════════════════════════════════════════════╗",CYAN);
        prtat(0,1,"║  MOUSE TEST v1.0            [_] [□] [X]     ║",CYAN);
        prtat(0,2,"╠══════════════════════════════════════════════╣",CYAN);
        char pos[30]; strcpy(pos,"Position: ("); itos(ms_x,pos+10); pos[12]=',';pos[13]=' '; itos(ms_y,pos+14); pos[16]=')'; pos[17]=0;
        prtat(2,4,pos,WHITE);
        char clk[30]; strcpy(clk,"Last click: "); if(ms_last==1) strcpy(clk+12,"LEFT"); else if(ms_last==2) strcpy(clk+12,"RIGHT"); else strcpy(clk+12,"NONE");
        prtat(2,5,clk,WHITE);
        char cnt[30]; strcpy(cnt,"Clicks: "); itos(ms_clicks,cnt+8);
        prtat(2,6,cnt,WHITE);
        prtat(2,8,"┌─────────────────────────────────────────────┐",LGRAY);
        prtat(2,9,"│ Move mouse / click to test                  │",LGREEN);
        prtat(2,10,"└─────────────────────────────────────────────┘",LGRAY);
        prtat(ms_x,ms_y,"+",YELLOW);
        for(int i=0;i<COLS;i++){ if(i==0||i==COLS-1) prtat(i,21,"║",CYAN); }
        prtat(0,22,"║  Press Q to quit                              ║",CYAN);
        prtat(0,23,"╚══════════════════════════════════════════════╝",CYAN);
        for(volatile int d=0;d<5000;d++);
    }
    outb(0x64,0xD4); outb(0x60,0xF5);
    cls(); prtn("Mouse test exited.",YELLOW);
}

/* Foxy Blackout (Red Screen of Death) */
static void foxy_blackout(void) {
    for(int i=0;i<COLS*ROWS;i++) vga[i]=(uint16_t)' '|((uint16_t)(RED<<4)|(WHITE&0x0F)<<8);
    prtat(8,4,"10110101 00101110 11100101 00011101",LRED);
    prtat(8,6,"01001110 10110011 00101101 11001010",LRED);
    prtat(8,8,"11100101 00011101 10101100 01001110",LRED);
    prtat(8,10,"00101101 11001010 01011100 11100101",LRED);
    prtat(22,17,"SYSTEM HALTED",WHITE);
    prtat(16,19,">> Press ENTER to reboot <<",YELLOW);
    /* annoying beep loop */
    while(1){
        beep(800,200);
        if(hask()){ if(getk()=='\n') break; }
        for(volatile int d=0;d<30000;d++);
    }
    outb(0x64,0xFE);
}

/* syscustom menu */
static void syscustom(void) {
    while(1){
        cls();
        prtn("═══════ SYSTEM CUSTOMIZATION ═══════",CYAN);
        prt("1. Logo style (",WHITE); prt("1-3",YELLOW); prtn(")",WHITE);
        prt("2. Prompt text (current: ",WHITE); prt(prompt_text,WHITE); prtn(")",WHITE);
        prtn("3. Reset to defaults",WHITE);
        prtn("4. Exit",RED);
        char c = getk();
        if(c=='1') { logo_style = (logo_style%3)+1; }
        else if(c=='2') {
            prt("New prompt: ",YELLOW); showcur();
            int i=0; prompt_text[0]=0;
            while(1){ char k=getk(); if(k=='\n'){prompt_text[i]=0;break;} else if(k=='\b'&&i>0){i--;putc('\b');} else if(k>=32&&i<9){prompt_text[i++]=k;putc(k);} }
            hidecur();
        }
        else if(c=='3') { logo_style=1; strcpy(prompt_text,"TFD~"); }
        else if(c=='4') break;
    }
}

/* ==================== COMMAND EXECUTION ==================== */
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
    else if(strcmp(cm,"time")==0) {
        uint8_t h=bcd2bin(cmos_read(0x04)), m=bcd2bin(cmos_read(0x02)), s=bcd2bin(cmos_read(0x00));
        char buf[9];
        buf[0]='0'+h/10;buf[1]='0'+h%10;buf[2]=':';
        buf[3]='0'+m/10;buf[4]='0'+m%10;buf[5]=':';
        buf[6]='0'+s/10;buf[7]='0'+s%10;buf[8]=0;
        prtn(buf,WHITE);
    }
    else if(strcmp(cm,"uptime")==0) { char b[20];itos(upt,b);prt("Uptime: ",GREEN);prt(b,WHITE);prtn("s",GREEN); }
    else if(strcmp(cm,"reboot")==0) { prtn("Rebooting...",YELLOW); for(volatile int d=0;d<300000;d++); outb(0x64,0xFE); }
    else if(strcmp(cm,"shutdown")==0) {
        shutdown_melody();
        prtn("Shutting down...",RED);
        for(volatile int d=0;d<200000;d++);
        outw(0x604,0x2000);
        outw(0xB004,0x2000);
        outb(0x64,0xFE);
        __asm__ volatile("cli;hlt");
    }
    else if(strcmp(cm,"lock")==0) {
        if(haspw){prtn("Locked. Password:",YELLOW);char pw[20];int i=0;while(1){char c=getk();if(c=='\n'){pw[i]=0;break;}else if(c>=32&&i<19){pw[i++]=c;putc('*');}}prtn(strcmp(pw,pass)==0?"Unlocked!":"Wrong!",strcmp(pw,pass)==0?GREEN:RED);}
        else prtn("No password. Use 'passwd'.",YELLOW);
    }
    else if(strcmp(cm,"echo")==0) prtn(arg,WHITE);
    else if(strcmp(cm,"whoami")==0) prtn(user,GREEN);
    else if(strcmp(cm,"ls")==0||strcmp(cm,"dir")==0) {
        for(int i=0;i<fc;i++) { prt(fdir[i]?"[DIR]  ":"[FILE] ",fdir[i]?YELLOW:WHITE); prtn(fn[i],WHITE); }
    }
    else if(strcmp(cm,"cd")==0) { /* simplified */ prtn("-> /",GREEN); }
    else if(strcmp(cm,"pwd")==0) prtn(curdir,WHITE);
    else if(strcmp(cm,"mkdir")==0&&arg[0]&&fc<MAXF) {
        strcpy(fn[fc],arg); fdir[fc]=1; fs[fc]=0; fc++; prtn("Created.",GREEN);
    }
    else if(strcmp(cm,"touch")==0&&arg[0]&&fc<MAXF) {
        strcpy(fn[fc],arg); fdir[fc]=0; fs[fc]=0; fc++; prtn("Created.",GREEN);
    }
    else if(strcmp(cm,"cat")==0) { for(int i=0;i<fc;i++) if(strcmp(fn[i],arg)==0&&!fdir[i]) { prtn(fd[i],WHITE); break; } }
    else if(strcmp(cm,"snake")==0) snake();
    else if(strcmp(cm,"tetris")==0) tetris();
    else if(strcmp(cm,"pong")==0) pong();
    else if(strcmp(cm,"lucky7")==0) lucky7();
    else if(strcmp(cm,"matrix")==0) matrix();
    else if(strcmp(cm,"nvidia")==0||strcmp(cm,"linus")==0) nvidia_egg();
    else if(strcmp(cm,"fcc")==0) fcc();
    else if(strcmp(cm,"mousetest")==0) mousetest();
    else if(strcmp(cm,"netchat")==0) {
        ne2000_init();
        prtn("Ethernet chat started. Q to quit.",GREEN);
        char msg[512]; int idx=0;
        while(1){
            if(hask()){
                char c=getk();
                if(c=='q'||c=='Q') break;
                if(c=='\n'){msg[idx]=0; ne2000_send(msg,idx); idx=0; putc('\n');}
                else if(c=='\b'){if(idx>0)idx--; putc('\b');}
                else if(c>=32){msg[idx++]=c; putc(c);}
            }
            char rbuf[1514];
            int len=ne2000_recv(rbuf);
            if(len>14){ rbuf[len]=0; prtn(rbuf+14,LCYAN); }
            for(volatile int d=0;d<5000;d++);
        }
    }
    else if(strcmp(cm,"syscustom")==0) syscustom();
    else if(strcmp(cm,"fxc")==0) {
        if(inst) { prt("PERMANENT (",GREEN); prt(idisk,WHITE); prtn(")",GREEN); }
        else prtn("LIVE (USB)",YELLOW);
    }
    else if(strcmp(cm,"install")==0) {
        while(hask()) getk();
        prt("Install TFD OS to disk? (y/n): ",YELLOW); showcur();
        char ch=0; while(ch!='y'&&ch!='n') ch=getk();
        putc(ch); prtn("",0); hidecur();
        if(ch=='y'){
            outb(ATA_DRIVE_SEL,0xA0); iowait();
            uint8_t st=inb(ATA_STATUS);
            if(st==0xFF||st==0x00){ prtn("No disk detected!",RED); return; }
            ata_write_sector(0,(uint8_t*)0x100000);
            prtn("Installed! Remove USB and reboot.",GREEN);
            inst=1; strcpy(idisk,"HDD/SSD");
        } else prtn("Cancelled.",YELLOW);
    }
    else if(strcmp(cm,"uninstall")==0) {
        while(hask()) getk();
        prt("Uninstall TFD OS? (y/n): ",RED); showcur();
        char ch=0; while(ch!='y'&&ch!='n') ch=getk();
        putc(ch); prtn("",0); hidecur();
        if(ch=='y'){
            uint8_t zero[512]={0};
            ata_write_sector(0,zero);
            prtn("Uninstalled.",GREEN);
            inst=0; idisk[0]=0;
        } else prtn("Cancelled.",YELLOW);
    }
    else if(strcmp(cm,"passwd")==0) {
        prt("New password: ",YELLOW); int i=0;
        while(1){ char c=getk(); if(c=='\n'){pass[i]=0;break;} else if(c>=32&&i<19){pass[i++]=c;putc('*');} }
        haspw=1; prtn(" Set!",GREEN);
    }
    else if(strcmp(cm,"history")==0) { for(int i=0;i<hcnt;i++){ char b[5]; itos(i+1,b); prt(b,WHITE); prt(" ",LGRAY); prtn(hist[i],WHITE); } }
    else if(strcmp(cm,"clearhist")==0) { hcnt=0; prtn("Cleared.",GREEN); }
    else if(strcmp(cm,"dice")==0) { char b[3]; itos((inb(0x40)%6)+1,b); prtn(b,YELLOW); }
    else if(strcmp(cm,"coin")==0) { prtn((inb(0x40)%2)?"Heads!":"Tails!",YELLOW); }
    else if(strcmp(cm,"beep")==0) { beep(1000,200); }
    else if(strcmp(cm,"setcolor")==0) { if(arg[0]) tc=stoi(arg)&0x0F; }
    else if(strcmp(cm,"sysinfo")==0) { prtn("CPU: x86 | RAM: 2MB+ | TFD v2.0",WHITE); }
    else if(strcmp(cm,"mem")==0) { prtn("~200KB used",WHITE); }
    else if(strcmp(cm,"ps")==0) { prtn("PID 1: kernel",WHITE); }
    else if(strcmp(cm,"bench")==0) { prtn("1000 loops: 0.01s",WHITE); }
    else if(strcmp(cm,"colors")==0) { for(int i=0;i<16;i++){ tc=i; prt("[X]",i); } prtn("",WHITE); }
    else if(strcmp(cm,"rainbow")==0) { prt("R",RED);prt("A",YELLOW);prt("I",GREEN);prt("N",CYAN);prt("B",BLUE);prt("O",MAGENTA);prtn("W",RED); }
    else if(strcmp(cm,"calc")==0) { prtn("Calculator (Coming soon)",YELLOW); }
    else {
        prt("?: ",RED); prtn(c,RED);
        invalid_cnt++;
        if(invalid_cnt>3||strlen(c)>40) foxy_blackout();
        else prtn("Type 'help'",YELLOW);
    }
}

/* ==================== BOOT MENU ==================== */
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
    prtat(10,10,"║        by Sadman from foxydox software      ║",LGRAY);
    prtat(10,11,"║                                              ║",CYAN);
    prtat(10,12,"║   ═══════════════════════════════════════    ║",CYAN);
    prtat(10,13,"║   [1] Boot TFD OS v2.0                      ║",GREEN);
    prtat(10,14,"║   [2] Safe Mode (VGA Fallback)              ║",LGREEN);
    prtat(10,15,"║   [3] Memory Test                           ║",LCYAN);
    prtat(10,16,"║   [4] Shutdown                              ║",RED);
    prtat(10,17,"║                                              ║",CYAN);
    prtat(10,18,"╚══════════════════════════════════════════════╝",CYAN);
    while(1) { if(hask()) { char c=getk(); if(c=='1') break; if(c=='4') { shutdown_melody(); outw(0x604,0x2000); outw(0xB004,0x2000); outb(0x64,0xFE); __asm__ volatile("cli;hlt"); } } }
}

/* ==================== SETUP ==================== */
static void setup_wizard(void) {
    cls(); prtn("SETUP WIZARD",CYAN);
    prt("Username: ",YELLOW); showcur(); int i=0;
    while(1){char c=getk();if(c=='\n'){user[i]=0;break;}else if(c=='\b'&&i>0){i--;putc('\b');}else if(c>=32&&i<19){user[i++]=c;putc(c);}}
    prtn("",0);
    prt("Hostname: ",YELLOW); i=0;
    while(1){char c=getk();if(c=='\n'){host[i]=0;break;}else if(c=='\b'&&i>0){i--;putc('\b');}else if(c>=32&&i<19){host[i++]=c;putc(c);}}
    prtn("",0); prtn("Setup complete.",GREEN);
    boot_melody();
    for(volatile int d=0;d<200000;d++); hidecur();
}

/* ==================== MAIN ==================== */
void kernel_main(uint32_t magic, uint32_t addr) {
    (void)magic;(void)addr;
    kwait(); outb(0x64,0xA8);
    kwait(); outb(0x60,0xF0); kwait(); outb(0x60,0x01);
    hidecur();
    vfs_init();
    boot_menu();
    setup_wizard();
    cls();
    prtn("TFD OS v2.0 Ready",GREEN); prtn("Type 'help'",YELLOW);
    while(1) {
        prompt(); showcur(); clen=0;
        while(1){char c=getk();if(c=='\n'){putc('\n');break;}else if(c=='\b'){if(clen>0){clen--;putc('\b');}}else if(c>=32&&c<=126&&clen<255){cmd[clen++]=c;putc(c);}}
        hidecur(); cmd[clen]=0; exec(cmd); upt++;
        update_clock();
    }
}
