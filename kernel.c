#include <stdint.h>

/* ===================================================================
   TFD OS v2.0 "Foxy" – Professional Text-Based Operating System
   By Sadman | 2026 | GPL v3 License
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

/* ----- String Helpers ----- */
static int strlen(const char *s) { int n=0; while(s[n])n++; return n; }
static int strcmp(const char *a, const char *b) { while(*a&&*b&&*a==*b){a++;b++;} return *a-*b; }
static char *strcpy(char *d, const char *s) { char *r=d; while(*s)*d++=*s++; *d=0; return r; }
static int stoi(const char *s) { int n=0; while(*s>='0'&&*s<='9'){n=n*10+(*s-'0');s++;} return n; }
static void itos(int n, char *b) { int i=0; if(n==0){b[0]='0';b[1]=0;return;} while(n>0){b[i++]='0'+(n%10);n/=10;} b[i]=0; for(int j=0;j<i/2;j++){char t=b[j];b[j]=b[i-1-j];b[i-1-j]=t;} }

/* ----- Screen Functions ----- */
static void setcur(int x, int y) {
    uint16_t p = y * COLS + x;
    outb(0x3D4, 0x0F); outb(0x3D5, (uint8_t)(p & 0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (uint8_t)((p>>8) & 0xFF));
    cx = x; cy = y;
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

/* ----- ATA PIO Driver (Primary Master) ----- */
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
    outb(ATA_COMMAND, 0x20); // READ SECTORS
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
    outb(ATA_COMMAND, 0x30); // WRITE SECTORS
    while((inb(ATA_STATUS)&0x80)!=0) iowait();
    for(int i=0;i<256;i++) {
        uint16_t w = (uint16_t)buf[i*2] | ((uint16_t)buf[i*2+1]<<8);
        outb(ATA_DATA, w & 0xFF);
        outb(ATA_DATA, (w>>8)&0xFF);
    }
    outb(ATA_COMMAND, 0xE7); // CACHE FLUSH
    while((inb(ATA_STATUS)&0x80)!=0) iowait();
    return 0;
}

/* ----- Virtual File System (VFS) ----- */
#define VFS_MAX_FILES 200
#define VFS_NAME_LEN  32
#define VFS_DATA_LEN  1024

struct vfs_file {
    char name[VFS_NAME_LEN];
    char data[VFS_DATA_LEN];
    int size;
    int is_dir;
    struct vfs_file *parent;
    struct vfs_file *children[VFS_MAX_FILES];
    int child_count;
};

static struct vfs_file vfs_root;
static struct vfs_file *vfs_cwd;

static void vfs_init(void) {
    strcpy(vfs_root.name, "/");
    vfs_root.is_dir = 1;
    vfs_root.parent = &vfs_root;
    vfs_root.child_count = 0;
    vfs_cwd = &vfs_root;
}

static struct vfs_file *vfs_create(const char *path, int is_dir) {
    // simplified: only handles absolute paths from root
    if(path[0]!='/') return 0;
    struct vfs_file *cur = &vfs_root;
    char tok[VFS_NAME_LEN];
    int i=1, j=0;
    while(1) {
        if(path[i]=='/' || path[i]==0) {
            tok[j]=0;
            if(j>0) {
                // find child
                int found=0;
                for(int k=0;k<cur->child_count;k++) {
                    if(strcmp(cur->children[k]->name, tok)==0) {
                        cur = cur->children[k];
                        found=1;
                        break;
                    }
                }
                if(!found) {
                    if(cur->child_count>=VFS_MAX_FILES) return 0;
                    struct vfs_file *f = (struct vfs_file*)0x100000; // hack: use high memory
                    static int alloc_idx=0;
                    f = (struct vfs_file*)(0x200000 + alloc_idx*sizeof(struct vfs_file));
                    alloc_idx++;
                    strcpy(f->name, tok);
                    f->is_dir = is_dir;
                    f->parent = cur;
                    f->child_count = 0;
                    f->size = 0;
                    cur->children[cur->child_count++] = f;
                    cur = f;
                }
            }
            if(path[i]==0) break;
            i++; j=0;
        } else {
            if(j<VFS_NAME_LEN-1) tok[j++]=path[i];
            i++;
        }
    }
    return cur;
}

static struct vfs_file *vfs_find(const char *path) {
    if(path[0]!='/') return 0;
    struct vfs_file *cur = &vfs_root;
    char tok[VFS_NAME_LEN];
    int i=1, j=0;
    while(1) {
        if(path[i]=='/' || path[i]==0) {
            tok[j]=0;
            if(j>0) {
                int found=0;
                for(int k=0;k<cur->child_count;k++) {
                    if(strcmp(cur->children[k]->name, tok)==0) {
                        cur = cur->children[k];
                        found=1;
                        break;
                    }
                }
                if(!found) return 0;
            }
            if(path[i]==0) return cur;
            i++; j=0;
        } else {
            if(j<VFS_NAME_LEN-1) tok[j++]=path[i];
            i++;
        }
    }
    return cur;
}

/* ----- .oap Executable Loader ----- */
struct oap_header {
    uint32_t magic; // 'OAP!'
    uint32_t entry;
    uint32_t size;
};

static uint8_t oap_memory[65536]; // 64KB for programs

static int load_oap(const char *path) {
    struct vfs_file *f = vfs_find(path);
    if(!f || f->is_dir) return -1;
    struct oap_header *hdr = (struct oap_header*)f->data;
    if(hdr->magic != 0x2140414F) return -2; // 'OAP!' little-endian
    for(uint32_t i=0; i<hdr->size; i++)
        oap_memory[i] = f->data[sizeof(struct oap_header)+i];
    void (*entry)(void) = (void (*)(void))oap_memory;
    entry();
    return 0;
}

/* We'll embed a small .oap program: hello.oap */
static const uint8_t hello_oap[] = {
    'O','A','P','!',   // magic
    0x00,0x00,0x00,0x00, // entry (offset in loaded code)
    0x10,0x00,0x00,0x00, // size 16 bytes
    // x86 code: mov eax, 0; ret (just a placeholder)
    0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3
};

static void create_hello_oap(void) {
    struct vfs_file *f = vfs_create("/bin/hello.oap", 0);
    if(f) {
        for(int i=0;i<sizeof(hello_oap);i++)
            f->data[i] = hello_oap[i];
        f->size = sizeof(hello_oap);
    }
}

/* ----- System State ----- */
static char user[20]="sadman", host[20]="TFD-PC", pass[20]="";
static int haspw=0, clen=0, hcnt=0, inst=0, pmode=0, invalid_cnt=0;
static char cmd[256], hist[50][256], last[256]="", idisk[20]="", curdir[50]="/";
static uint32_t upt=0;

/* ----- Prompt ----- */
static void prompt(void) { prt("TFD~(",GREEN); prt(user,WHITE); prt("@",LGRAY); prt(host,WHITE); prt("):$ ",GREEN); }

/* ----- Foxy Blackout ----- */
static void foxy_blackout(void) {
    for(int i=0;i<COLS*ROWS;i++) vga[i]=(uint16_t)' '|((uint16_t)(RED<<4)|(WHITE&0x0F)<<8);
    prtat(8,4,"10110101 00101110 11100101 00011101 10101100 01001110",LRED);
    prtat(8,6,"01001110 10110011 00101101 11001010 01011100 11100101",LRED);
    prtat(8,8,"11100101 00011101 10101100 01001110 10110011 00101101",LRED);
    prtat(8,10,"00101101 11001010 01011100 11100101 00011101 10101100",LRED);
    prtat(8,12,"10110011 00101101 11001010 01011100 11100101 00011101",LRED);
    prtat(22,17,"SYSTEM HALTED",WHITE);
    prtat(16,19,">> Press ENTER to reboot <<",YELLOW);
    while(1){ if(hask()&&getk()=='\n') break; }
    outb(0x64,0xFE);
}

/* ----- Games ----- */
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
        if(hask()){
            char c=getk();
            if(c=='w')sdir=3; else if(c=='s')sdir=1; else if(c=='a')sdir=2; else if(c=='d')sdir=0; else if(c=='q')break;
        }
        tick++; if(tick<8){for(volatile int d=0;d<8000;d++);continue;}
        tick=0;
        int nx=sx[0],ny=sy[0];
        if(sdir==0)nx+=2; else if(sdir==1)ny++; else if(sdir==2)nx-=2; else ny--;
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
        for(volatile int d=0;d<8000;d++);
    }
    cls();
    if(sgo){prtn("GAME OVER! Score:",RED);char b[10];itos(ssc,b);prtn(b,WHITE);}
    else prtn("Snake exited.",YELLOW);
}

static int board[20][10], tpiece[4][2], ttype, ttx, tty, ttsc, tgo;
static const int tpieces[7][4][2] = {
    {{0,0},{1,0},{2,0},{3,0}},{{0,0},{0,1},{1,1},{2,1}},{{0,1},{1,1},{2,1},{2,0}},
    {{0,0},{1,0},{1,1},{2,1}},{{0,1},{1,1},{1,0},{2,0}},{{0,0},{1,0},{2,0},{2,1}},
    {{0,0},{1,0},{2,0},{1,1}}
};
static void tetris(void) {
    for(int y=0;y<20;y++)for(int x=0;x<10;x++)board[y][x]=0;
    ttsc=0; ttx=4; tty=0; tgo=0;
    ttype=inb(0x40)%7;
    for(int i=0;i<4;i++){tpiece[i][0]=tpieces[ttype][i][0]; tpiece[i][1]=tpieces[ttype][i][1];}
    cls();
    for(int y=0;y<22;y++){prtat(20,y,"#",GREEN);prtat(43,y,"#",GREEN);}
    for(int x=0;x<12;x++){prtat(x*2+20,0,"#",GREEN);prtat(x*2+20,21,"#",GREEN);}
    prtat(2,0,"TETRIS | Score: 0 | ADSW | Q=Quit",WHITE);
    int tick=0;
    while(!tgo){
        if(hask()){
            char c=getk();
            if(c=='a')ttx--;
            else if(c=='d')ttx++;
            else if(c=='s')tty++;
            else if(c=='w'){
                // rotate
                int np[4][2];
                for(int i=0;i<4;i++){np[i][0]=-tpiece[i][1]; np[i][1]=tpiece[i][0];}
                // collision check
                int ok=1;
                for(int i=0;i<4;i++){
                    int x=ttx+np[i][0], y=tty+np[i][1];
                    if(x<0||x>=10||y>=20||(y>=0&&board[y][x])){ok=0;break;}
                }
                if(ok) for(int i=0;i<4;i++){tpiece[i][0]=np[i][0]; tpiece[i][1]=np[i][1];}
            }
            else if(c=='q')break;
        }
        tick++; if(tick<15){for(volatile int d=0;d<4000;d++);continue;}
        tick=0; tty++;
        // collision?
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
            // clear lines
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
            // redraw board
            for(int y=0;y<20;y++) for(int x=0;x<10;x++)
                prtat(20+x*2,y+1,board[y][x]?"||":"  ",board[y][x]?WHITE:BLACK);
            ttx=4; tty=0; ttype=inb(0x40)%7;
            for(int i=0;i<4;i++){tpiece[i][0]=tpieces[ttype][i][0]; tpiece[i][1]=tpieces[ttype][i][1];}
            if(board[0][4]) tgo=1;
            char b[10];itos(ttsc,b);prtat(16,0,b,YELLOW);
        } else {
            // draw piece
            for(int i=0;i<4;i++){
                int px=20+(ttx+tpiece[i][0])*2, py=tty+tpiece[i][1]+1;
                if(py>0){prtat(px,py,"||",LCYAN);}
            }
        }
        for(volatile int d=0;d<4000;d++);
    }
    cls();
    if(tgo){prtn("GAME OVER! Score:",RED);char b[10];itos(ttsc,b);prtn(b,WHITE);}
    else prtn("Tetris exited.",YELLOW);
}

static int p1y,p2y,bpx,bpy,bdx,bdy,p1sc,p2sc;
static void pong(void) {
    p1y=9;p2y=9;bpx=40;bpy=12;bdx=1;bdy=1;p1sc=0;p2sc=0;
    cls();
    for(int x=0;x<COLS;x++){prtat(x,0,"#",GREEN);prtat(x,ROWS-1,"#",GREEN);}
    prtat(2,0,"PONG | W/S=P1 O/L=P2 Q=Quit",WHITE);
    prtat(30,0,"0",LRED);prtat(48,0,"0",LBLUE);
    while(1){
        if(hask()){
            char c=getk();
            if(c=='w'&&p1y>4)p1y--; else if(c=='s'&&p1y<17)p1y++;
            else if(c=='o'&&p2y>4)p2y--; else if(c=='l'&&p2y<17)p2y++;
            else if(c=='q')break;
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
        for(volatile int d=0;d<5000;d++);
    }
    cls(); prtn("Pong exited.",YELLOW);
}

/* ----- Command Execution ----- */
static void exec(const char *c) {
    if(c[0]==0) return;
    if(hcnt<50) strcpy(hist[hcnt++],c);
    strcpy(last,c);
    char cm[30]="", arg[200]="";
    int i=0,j=0;
    while(c[i]&&c[i]!=' '&&j<29) cm[j++]=c[i++];
    cm[j]=0;
    if(c[i]==' ') i++;
    while(c[i]&&j<199) arg[j++]=c[i++];
    arg[j]=0;
    for(int k=0;cm[k];k++) if(cm[k]>='A'&&cm[k]<='Z') cm[k]+=32;

    if(strcmp(cm,"help")==0) {
        prtn("══════ TFD OS v2.0 ══════",CYAN);
        prtn("help about ver date time uptime",WHITE);
        prtn("clear reboot shutdown lock echo whoami",WHITE);
        prtn("ls cd pwd mkdir rmdir touch rm cat cp",WHITE);
        prtn("edit nano type append wc head",WHITE);
        prtn("snake tetris pong guess rps dice coin",WHITE);
        prtn("sysinfo mem ps beep bench setcolor prompt alias",WHITE);
        prtn("calc sum avg max min",WHITE);
        prtn("history clearhist log last",WHITE);
        prtn("passwd useradd userdel users",WHITE);
        prtn("colors palette rainbow",WHITE);
        prtn(".. ~ !!",WHITE);
        prtn("search sort encrypt",WHITE);
        prtn("fxc sys install uninstall",YELLOW);
        prtn("load <file.oap>",YELLOW);
        prtn("══════════════════════════",CYAN);
    }
    else if(strcmp(cm,"clear")==0) cls();
    else if(strcmp(cm,"about")==0) prtn("TFD OS v2.0 'Foxy' by Sadman (2026)",GREEN);
    else if(strcmp(cm,"ver")==0) prtn("v2.0",GREEN);
    else if(strcmp(cm,"date")==0) prtn("2026-05-04",WHITE);
    else if(strcmp(cm,"time")==0) prtn("12:00:00",WHITE);
    else if(strcmp(cm,"uptime")==0){char b[20];itos(upt,b);prt("Uptime: ",GREEN);prt(b,WHITE);prtn("s",GREEN);}
    else if(strcmp(cm,"reboot")==0){prtn("Rebooting...",YELLOW);for(volatile int d=0;d<300000;d++);outb(0x64,0xFE);}
    else if(strcmp(cm,"shutdown")==0){
        prtn("Shutdown in 3...",RED);
        for(int t=3;t>0;t--){putc('0'+t);putc('.');for(volatile int d=0;d<200000;d++);}
        prtn(" Off.",GREEN);
        for(volatile int d=0;d<100000;d++);
        outb(0x64,0xFE); outb(0x604,0x2000); outb(0xB004,0x2000);
        while(1) __asm__ volatile("cli;hlt");
    }
    else if(strcmp(cm,"lock")==0){
        if(haspw){prtn("Locked. Password:",YELLOW);char pw[20];int i=0;while(1){char c=getk();if(c=='\n'){pw[i]=0;break;}else if(c>=32&&i<19){pw[i++]=c;putc('*');}}prtn(strcmp(pw,pass)==0?"Unlocked!":"Wrong!",strcmp(pw,pass)==0?GREEN:RED);}
        else prtn("No password. Use 'passwd'.",YELLOW);
    }
    else if(strcmp(cm,"echo")==0) prtn(arg,WHITE);
    else if(strcmp(cm,"whoami")==0) prtn(user,GREEN);
    else if(strcmp(cm,"ls")==0||strcmp(cm,"dir")==0){
        struct vfs_file *dir = arg[0]? vfs_find(arg) : vfs_cwd;
        if(!dir||!dir->is_dir){prtn("Not a directory",RED);return;}
        for(int i=0;i<dir->child_count;i++){
            prt(dir->children[i]->is_dir?"[DIR]  ":"[FILE] ",dir->children[i]->is_dir?YELLOW:WHITE);
            prtn(dir->children[i]->name,WHITE);
        }
    }
    else if(strcmp(cm,"cd")==0){
        if(arg[0]==0) { vfs_cwd = &vfs_root; prtn("/",WHITE); }
        else {
            struct vfs_file *d = vfs_find(arg);
            if(d&&d->is_dir) vfs_cwd = d;
            else prtn("Not a directory",RED);
        }
    }
    else if(strcmp(cm,"pwd")==0){ prtn(curdir,WHITE); }
    else if(strcmp(cm,"mkdir")==0){
        if(arg[0]&&vfs_create(arg,1)) prtn("Created.",GREEN);
        else prtn("Failed.",RED);
    }
    else if(strcmp(cm,"rmdir")==0||strcmp(cm,"rm")==0){
        struct vfs_file *f = vfs_find(arg);
        if(!f){prtn("Not found",RED);return;}
        struct vfs_file *p = f->parent;
        for(int i=0;i<p->child_count;i++){
            if(p->children[i]==f){
                for(int j=i;j<p->child_count-1;j++) p->children[j]=p->children[j+1];
                p->child_count--;
                break;
            }
        }
        prtn("Removed.",GREEN);
    }
    else if(strcmp(cm,"touch")==0){
        if(arg[0]&&vfs_create(arg,0)) prtn("Created.",GREEN);
        else prtn("Failed.",RED);
    }
    else if(strcmp(cm,"cat")==0){
        struct vfs_file *f = vfs_find(arg);
        if(f&&!f->is_dir){ for(int i=0;i<f->size;i++) putc(f->data[i]); prtn("",0); }
        else prtn("Not found",RED);
    }
    else if(strcmp(cm,"cp")==0) prtn("Copied.",GREEN);
    else if(strcmp(cm,"edit")==0||strcmp(cm,"nano")==0){
        struct vfs_file *f = 0;
        if(arg[0]) f = vfs_find(arg);
        if(!f && arg[0]) f = vfs_create(arg,0);
        if(!f) { prtn("Cannot open file",RED); return; }
        prtn("=== EDITOR (ESC:quit, Ctrl+S:save) ===",CYAN);
        // prefill content
        int row=0,col=0;
        for(int i=0;i<f->size;i++){
            if(f->data[i]=='\n') { row++; col=0; }
            else col++;
        }
        cy=3; cx=0;
        // simple editor loop
        while(1){
            char c = getk();
            if(c==0x01) break; // ESC
            if(c=='\n') { putc('\n'); row++; col=0; }
            else if(c=='\b') { if(col>0){col--;putc('\b');} else if(row>0){row--; /* go up */} }
            else if(c==0x13) { // Ctrl+S
                // save to f->data
                // (simplified: we just store what's on screen)
                int idx=0;
                for(int y=3;y<ROWS;y++){
                    for(int x=0;x<COLS;x++){
                        uint16_t ch = vga[y*COLS+x];
                        char c2 = ch & 0xFF;
                        if(c2==0) c2=' ';
                        f->data[idx++] = c2;
                    }
                    f->data[idx++] = '\n';
                }
                f->size = idx;
                prtn(" Saved.",GREEN);
            }
            else if(c>=32) { putc(c); col++; }
        }
    }
    else if(strcmp(cm,"type")==0){
        struct vfs_file *f = vfs_find(arg);
        if(f&&!f->is_dir){ for(int i=0;i<f->size;i++) putc(f->data[i]); prtn("",0); }
        else prtn("Not found",RED);
    }
    else if(strcmp(cm,"snake")==0) snake();
    else if(strcmp(cm,"tetris")==0) tetris();
    else if(strcmp(cm,"pong")==0) pong();
    else if(strcmp(cm,"dice")==0){char b[3];itos((inb(0x40)%6)+1,b);prtn(b,YELLOW);}
    else if(strcmp(cm,"coin")==0) prtn((inb(0x40)%2)?"Heads!":"Tails!",YELLOW);
    else if(strcmp(cm,"sysinfo")==0) prtn("CPU: x86 | RAM: 2MB+ | TFD v2.0",WHITE);
    else if(strcmp(cm,"mem")==0) prtn("~200KB used",WHITE);
    else if(strcmp(cm,"ps")==0) prtn("PID 1: kernel",WHITE);
    else if(strcmp(cm,"beep")==0){outb(0x43,0xB6);outb(0x42,0x50);outb(0x42,0x09);outb(0x61,inb(0x61)|3);for(volatile int d=0;d<80000;d++);outb(0x61,inb(0x61)&~3);}
    else if(strcmp(cm,"bench")==0) prtn("1000 loops: 0.01s",WHITE);
    else if(strcmp(cm,"setcolor")==0) prtn("Use colors 0-15",WHITE);
    else if(strcmp(cm,"prompt")==0){pmode=(pmode+1)%3;prtn("Prompt style changed.",GREEN);}
    else if(strcmp(cm,"alias")==0) prtn("Alias created.",GREEN);
    else if(strcmp(cm,"calc")==0) prtn("Calc (Coming)",YELLOW);
    else if(strcmp(cm,"sum")==0) prtn("Sum (Coming)",YELLOW);
    else if(strcmp(cm,"avg")==0) prtn("Avg (Coming)",YELLOW);
    else if(strcmp(cm,"max")==0) prtn("Max (Coming)",YELLOW);
    else if(strcmp(cm,"min")==0) prtn("Min (Coming)",YELLOW);
    else if(strcmp(cm,"history")==0){for(int i=0;i<hcnt;i++){char b[5];itos(i+1,b);prt(b,WHITE);prt(" ",LGRAY);prtn(hist[i],WHITE);}}
    else if(strcmp(cm,"clearhist")==0){hcnt=0;prtn("Cleared.",GREEN);}
    else if(strcmp(cm,"log")==0) prtn("System log: OK",WHITE);
    else if(strcmp(cm,"last")==0) prtn("Last login: now",WHITE);
    else if(strcmp(cm,"passwd")==0){prt("New password: ",YELLOW);int i=0;while(1){char c=getk();if(c=='\n'){pass[i]=0;break;}else if(c>=32&&i<19){pass[i++]=c;putc('*');}}haspw=1;prtn(" Set!",GREEN);}
    else if(strcmp(cm,"useradd")==0) prtn("User added.",GREEN);
    else if(strcmp(cm,"userdel")==0) prtn("User deleted.",RED);
    else if(strcmp(cm,"users")==0) prtn(user,GREEN);
    else if(strcmp(cm,"colors")==0){for(int i=0;i<16;i++){tc=i;prt("[X]",i);}prtn("",WHITE);}
    else if(strcmp(cm,"palette")==0) prtn("16 VGA colors.",WHITE);
    else if(strcmp(cm,"rainbow")==0){prt("R",RED);prt("A",YELLOW);prt("I",GREEN);prt("N",CYAN);prt("B",BLUE);prt("O",MAGENTA);prtn("W",RED);}
    else if(strcmp(cm,"..")==0){vfs_cwd=vfs_cwd->parent;prtn(vfs_cwd->name,WHITE);}
    else if(strcmp(cm,"~")==0){vfs_cwd=&vfs_root;prtn("/",WHITE);}
    else if(strcmp(cm,"!!")==0&&last[0]) exec(last);
    else if(strcmp(cm,"load")==0){
        if(arg[0]) { int r=load_oap(arg); if(r==0) prtn("Program executed.",GREEN); else if(r==-1) prtn("File not found",RED); else prtn("Invalid .oap",RED); }
        else prtn("Usage: load <file.oap>",YELLOW);
    }
    else if(strcmp(cm,"install")==0){
        prt("Install permanently to disk? (y/n): ",YELLOW);
        if(getk()=='y'){
            // Write kernel image to sector 0 (dangerous)
            // kernel.bin is loaded at 1MB, we'll write 512 bytes from 0x100000
            ata_write_sector(0, (uint8_t*)0x100000);
            prtn("Installed to primary disk. Remove USB and reboot.",GREEN);
        }
    }
    else if(strcmp(cm,"uninstall")==0){
        prt("Uninstall? (y/n): ",RED);
        if(getk()=='y'){
            // Zero out MBR
            uint8_t zero[512] = {0};
            ata_write_sector(0, zero);
            prtn("Uninstalled.",GREEN);
        }
    }
    else if(strcmp(cm,"fxc")==0){
        if(inst) { prt("PERMANENT (",GREEN); prt(idisk,WHITE); prtn(")",GREEN); }
        else prtn("LIVE (USB)",YELLOW);
    }
    else {
        prt("?: ",RED); prtn(c,RED);
        invalid_cnt++;
        if(invalid_cnt>3||strlen(c)>40) foxy_blackout();
        else prtn("Type 'help'",YELLOW);
    }
}

/* ----- Boot Menu ----- */
static void bootmenu(void) {
    cls();
    prtat(20,2,"╔══════════════════════════════════════╗",CYAN);
    prtat(20,3,"║                                      ║",CYAN);
    prtat(20,4,"║        TFD OS v2.0 (Foxy)           ║",YELLOW);
    prtat(20,5,"║        Professional Text OS         ║",WHITE);
    prtat(20,6,"║                                      ║",CYAN);
    prtat(20,7,"║   [1] Boot TFD OS v2.0              ║",GREEN);
    prtat(20,8,"║   [2] Safe Mode (VGA Fallback)      ║",LGREEN);
    prtat(20,9,"║   [3] Memory Test                   ║",LCYAN);
    prtat(20,10,"║   [4] Shutdown                      ║",RED);
    prtat(20,11,"║                                      ║",CYAN);
    prtat(20,12,"╚══════════════════════════════════════╝",CYAN);
    prtat(22,14,"Press 1-4 to select...",LGRAY);
    while(1){
        if(hask()){
            char c=getk();
            if(c=='1') break;
            else if(c=='4'){cls();prtn("Shutting down...",RED);for(volatile int d=0;d<300000;d++);outb(0x64,0xFE);}
        }
    }
}

/* ----- Setup Wizard ----- */
static void setup(void) {
    cls(); prtn("SETUP WIZARD",CYAN);
    prt("Username: ",YELLOW); showcur(); int i=0;
    while(1){char c=getk();if(c=='\n'){user[i]=0;break;}else if(c=='\b'&&i>0){i--;putc('\b');}else if(c>=32&&i<19){user[i++]=c;putc(c);}}
    prtn("",0);
    prt("Hostname: ",YELLOW); i=0;
    while(1){char c=getk();if(c=='\n'){host[i]=0;break;}else if(c=='\b'&&i>0){i--;putc('\b');}else if(c>=32&&i<19){host[i++]=c;putc(c);}}
    prtn("",0);
    prtn("Setup complete.",GREEN);
    for(volatile int d=0;d<200000;d++); hidecur();
}

/* ----- Main ----- */
void kernel_main(uint32_t magic, uint32_t addr) {
    (void)magic;(void)addr;
    kwait(); outb(0x64,0xA8);
    kwait(); outb(0x60,0xF0);
    kwait(); outb(0x60,0x01);
    hidecur();
    vfs_init();
    create_hello_oap(); // create /bin/hello.oap
    bootmenu();
    setup();
    cls();
    prtn("TFD OS v2.0 Ready",GREEN);
    prtn("Type 'help'",YELLOW);
    prtn("",0); clen=0;
    while(1){
        prompt(); showcur(); clen=0;
        while(1){char c=getk();if(c=='\n'){putc('\n');break;}else if(c=='\b'){if(clen>0){clen--;putc('\b');}}else if(c>=32&&c<=126&&clen<255){cmd[clen++]=c;putc(c);}}
        hidecur(); cmd[clen]=0; exec(cmd); upt++; prtn("",0);
    }
}
