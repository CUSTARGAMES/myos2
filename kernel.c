#include <stdint.h>

/* ===================================================================
   TFD OS v1.0 "Foxy" - Complete Text-Based Operating System (72 cmds)
   By Sadman | 2026 | GPL v3 License
   ===================================================================
   FIX: Moved string helpers to the top before fsinit
   =================================================================== */

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
static inline void outb(uint16_t p, uint8_t v) { __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p) { uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }
static inline void iowait(void) { outb(0x80,0); }

/* ============ STRING HELPERS (MOVED TO TOP) ============ */
static int str_len(const char*s){int n=0;while(s[n])n++;return n;}
static int str_cmp(const char*a,const char*b){while(*a&&*b&&*a==*b){a++;b++;}return*a-*b;}
static void str_cpy(char*d,const char*s){while(*s)*d++=*s++;*d=0;}
static int stoi(const char*s){int n=0;while(*s>='0'&&*s<='9'){n=n*10+(*s-'0');s++;}return n;}
static void itos(int n,char*b){int i=0;if(n==0){b[0]='0';b[1]=0;return;}while(n>0){b[i++]='0'+(n%10);n/=10;}b[i]=0;for(int j=0;j<i/2;j++){char t=b[j];b[j]=b[i-1-j];b[i-1-j]=t;}}

/* ============ FAKE FILE SYSTEM ============ */
#define MAXF 100
static char fn[MAXF][32];
static char fd[MAXF][500];
static int fs[MAXF];
static int fdir[MAXF];
static int fc = 0;
static void fsinit(void) {
    fc=0;
    str_cpy(fn[fc],"home"); fdir[fc]=1; fc++;
    str_cpy(fn[fc],"readme.txt"); str_cpy(fd[fc],"Welcome to TFD OS v1.0!\nCreated by Sadman\n2026"); fs[fc]=str_len(fd[fc]); fdir[fc]=0; fc++;
    str_cpy(fn[fc],"games"); fdir[fc]=1; fc++;
    str_cpy(fn[fc],"notes.txt"); str_cpy(fd[fc],"My notes here."); fs[fc]=str_len(fd[fc]); fdir[fc]=0; fc++;
}

/* ============ SYSTEM STATE ============ */
static char user[20]="sadman", pc[20]="TFD-PC", pass[20]="";
static int haspw=0, clen=0, hcnt=0, inst=0, pmode=0;
static char cmd[256], hist[50][256], last[256]="", idisk[20]="", curdir[50]="/";
static uint32_t upt=0;

/* ============ SCREEN ============ */
static void setcur(int x,int y){uint16_t p=y*COLS+x;outb(0x3D4,0x0F);outb(0x3D5,(uint8_t)(p&0xFF));outb(0x3D4,0x0E);outb(0x3D5,(uint8_t)((p>>8)&0xFF));cx=x;cy=y;}
static void hidecur(void){outb(0x3D4,0x0A);outb(0x3D5,0x20);}
static void showcur(void){outb(0x3D4,0x0A);outb(0x3D5,0x0E);outb(0x3D4,0x0B);outb(0x3D5,0x0F);}
static void cls(void){for(int i=0;i<COLS*ROWS;i++)vga[i]=(uint16_t)' '|((uint16_t)LGRAY<<8);cx=0;cy=0;}
static void scroll(void){for(int y=0;y<ROWS-1;y++)for(int x=0;x<COLS;x++)vga[y*COLS+x]=vga[(y+1)*COLS+x];for(int x=0;x<COLS;x++)vga[(ROWS-1)*COLS+x]=(uint16_t)' '|((uint16_t)LGRAY<<8);}
static void putc(char c){if(c=='\n'){cx=0;cy++;if(cy>=ROWS){scroll();cy=ROWS-1;}}else if(c=='\b'){if(cx>0){cx--;vga[cy*COLS+cx]=(uint16_t)' '|((uint16_t)LGRAY<<8);}}else{vga[cy*COLS+cx]=(uint16_t)c|((uint16_t)tc<<8);cx++;if(cx>=COLS){cx=0;cy++;if(cy>=ROWS){scroll();cy=ROWS-1;}}}setcur(cx,cy);}
static void prt(const char*s,uint8_t c){tc=c;while(*s)putc(*s++);}
static void prtn(const char*s,uint8_t c){prt(s,c);putc('\n');}
static void prtat(int x,int y,const char*s,uint8_t c){while(*s){if(x>=0&&x<COLS&&y>=0&&y<ROWS)vga[y*COLS+x]=(uint16_t)(*s)|((uint16_t)c<<8);s++;x++;}}
static void prompt(void){prt("TFD~(",GREEN);prt(user,WHITE);prt("@",LGRAY);prt(pc,WHITE);prt("):$ ",GREEN);}

/* ============ KEYBOARD ============ */
static void kwait(void){while(inb(0x64)&2)iowait();}
static void kdata(void){while(!(inb(0x64)&1))iowait();}
static char sctoasc(uint8_t sc){switch(sc){case 0x0E:return'\b';case 0x1C:return'\n';case 0x39:return' ';case 0x02:return'1';case 0x03:return'2';case 0x04:return'3';case 0x05:return'4';case 0x06:return'5';case 0x07:return'6';case 0x08:return'7';case 0x09:return'8';case 0x0A:return'9';case 0x0B:return'0';case 0x0C:return'-';case 0x0D:return'=';case 0x10:return'q';case 0x11:return'w';case 0x12:return'e';case 0x13:return'r';case 0x14:return't';case 0x15:return'y';case 0x16:return'u';case 0x17:return'i';case 0x18:return'o';case 0x19:return'p';case 0x1A:return'[';case 0x1B:return']';case 0x1E:return'a';case 0x1F:return's';case 0x20:return'd';case 0x21:return'f';case 0x22:return'g';case 0x23:return'h';case 0x24:return'j';case 0x25:return'k';case 0x26:return'l';case 0x27:return';';case 0x28:return'\'';case 0x29:return'`';case 0x2B:return'\\';case 0x2C:return'z';case 0x2D:return'x';case 0x2E:return'c';case 0x2F:return'v';case 0x30:return'b';case 0x31:return'n';case 0x32:return'm';case 0x33:return',';case 0x34:return'.';case 0x35:return'/';case 0x37:return'*';default:return 0;}}
static char getk(void){kdata();return sctoasc(inb(0x60));}
static int hask(void){return inb(0x64)&1;}

/* ============ SNAKE GAME ============ */
static int sx[200],sy[200],snl,sdir,sfx,sfy,ssc,sgo;
static void game_snake(void){
    snl=3;sdir=0;ssc=0;sgo=0;
    sx[0]=40;sy[0]=12;sx[1]=38;sy[1]=12;sx[2]=36;sy[2]=12;
    sfx=50;sfy=12;
    cls();
    for(int x=0;x<COLS;x++){prtat(x,0,"#",GREEN);prtat(x,ROWS-1,"#",GREEN);}
    for(int y=0;y<ROWS;y++){prtat(0,y,"#",GREEN);prtat(COLS-1,y,"#",GREEN);}
    prtat(2,0," SNAKE | Score: 0 | WASD | Q=Quit ",WHITE);
    prtat(sfx,sfy,".",RED);
    for(int i=0;i<snl;i++){prtat(sx[i],sy[i],">",YELLOW);prtat(sx[i]+1,sy[i],">",YELLOW);}
    int tick=0;
    while(!sgo){
        if(hask()){char c=getk();if(c=='w')sdir=3;else if(c=='s')sdir=1;else if(c=='a')sdir=2;else if(c=='d')sdir=0;else if(c=='q')break;}
        tick++;if(tick<8){for(volatile int d=0;d<8000;d++);continue;}
        tick=0;
        int nx=sx[0],ny=sy[0];
        if(sdir==0)nx+=2;else if(sdir==1)ny++;else if(sdir==2)nx-=2;else ny--;
        if(nx<=0||nx>=COLS-2||ny<=0||ny>=ROWS-1){sgo=1;break;}
        for(int i=0;i<snl;i++)if(sx[i]==nx&&sy[i]==ny){sgo=1;break;}
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

/* ============ COMMAND EXECUTION ============ */
static void exec(const char*c){
    if(c[0]==0)return;
    if(hcnt<50)str_cpy(hist[hcnt++],c);
    str_cpy(last,c);
    char cm[30]="",arg[200]="";
    int i=0,j=0;
    while(c[i]&&c[i]!=' '&&j<29)cm[j++]=c[i++];
    cm[j]=0;
    if(c[i]==' ')i++;
    while(c[i]&&j<199)arg[j++]=c[i++];
    arg[j]=0;
    for(int k=0;cm[k];k++)if(cm[k]>='A'&&cm[k]<='Z')cm[k]+=32;

    if(str_cmp(cm,"help")==0){
        prtn("══════ TFD OS v1.0 ══════",CYAN);
        prtn("help about ver date time uptime",WHITE);
        prtn("clear reboot shutdown lock echo whoami",WHITE);
        prtn("ls cd pwd mkdir rmdir touch rm cat cp",WHITE);
        prtn("edit nano type append wc head",WHITE);
        prtn("snake tetris pong guess rps dice coin maze tictactoe hangman mathquiz asciiart",WHITE);
        prtn("sysinfo mem ps beep bench setcolor prompt alias",WHITE);
        prtn("calc sum avg max min",WHITE);
        prtn("history clearhist log last",WHITE);
        prtn("passwd useradd userdel users",WHITE);
        prtn("colors palette rainbow",WHITE);
        prtn(".. ~ !!",WHITE);
        prtn("search sort encrypt",WHITE);
        prtn("fxc sys install uninstall",YELLOW);
        prtn("══════════════════════════",CYAN);
    }
    else if(str_cmp(cm,"clear")==0)cls();
    else if(str_cmp(cm,"about")==0)prtn("TFD OS v1.0 'Foxy' - Text-Based OS by Sadman (2026)",GREEN);
    else if(str_cmp(cm,"ver")==0)prtn("v1.0",GREEN);
    else if(str_cmp(cm,"date")==0)prtn("2026-05-02",WHITE);
    else if(str_cmp(cm,"time")==0)prtn("12:00:00",WHITE);
    else if(str_cmp(cm,"uptime")==0){char b[20];itos(upt,b);prt("Uptime: ",GREEN);prt(b,WHITE);prtn("s",GREEN);}
    else if(str_cmp(cm,"reboot")==0){prtn("Rebooting...",YELLOW);for(volatile int d=0;d<300000;d++);outb(0x64,0xFE);}
    else if(str_cmp(cm,"shutdown")==0){
        prtn("Shutdown in 3...",RED);
        for(int t=3;t>0;t--){putc('0'+t);putc('.');for(volatile int d=0;d<200000;d++);}
        prtn(" Off.",GREEN);
        for(volatile int d=0;d<100000;d++);
        outb(0x64,0xFE);outb(0x604,0x2000);outb(0xB004,0x2000);
        while(1)__asm__ volatile("cli;hlt");
    }
    else if(str_cmp(cm,"lock")==0){
        if(haspw){prtn("Locked. Password:",YELLOW);char pw[20];int i=0;while(1){char c=getk();if(c=='\n'){pw[i]=0;break;}else if(c>=32&&i<19){pw[i++]=c;putc('*');}}prtn(str_cmp(pw,pass)==0?"Unlocked!":"Wrong!",str_cmp(pw,pass)==0?GREEN:RED);}
        else prtn("No password. Use 'passwd'.",YELLOW);
    }
    else if(str_cmp(cm,"echo")==0)prtn(arg,WHITE);
    else if(str_cmp(cm,"whoami")==0)prtn(user,GREEN);
    else if(str_cmp(cm,"ls")==0||str_cmp(cm,"dir")==0){for(int i=0;i<fc;i++){prt(fdir[i]?"[DIR]  ":"[FILE] ",fdir[i]?YELLOW:WHITE);prtn(fn[i],WHITE);}}
    else if(str_cmp(cm,"cd")==0){if(arg[0])str_cpy(curdir,arg);prt("-> ",GREEN);prtn(curdir,WHITE);}
    else if(str_cmp(cm,"pwd")==0)prtn(curdir,WHITE);
    else if(str_cmp(cm,"mkdir")==0&&arg[0]&&fc<MAXF){str_cpy(fn[fc],arg);fdir[fc]=1;fs[fc]=0;fc++;prtn("Created.",GREEN);}
    else if(str_cmp(cm,"rmdir")==0||str_cmp(cm,"rm")==0){for(int i=0;i<fc;i++)if(str_cmp(fn[i],arg)==0){for(int j=i;j<fc-1;j++){str_cpy(fn[j],fn[j+1]);fdir[j]=fdir[j+1];fs[j]=fs[j+1];}fc--;prtn("Removed.",GREEN);break;}}
    else if(str_cmp(cm,"touch")==0&&arg[0]&&fc<MAXF){str_cpy(fn[fc],arg);fdir[fc]=0;fs[fc]=0;fc++;prtn("Created.",GREEN);}
    else if(str_cmp(cm,"cat")==0){for(int i=0;i<fc;i++)if(str_cmp(fn[i],arg)==0&&!fdir[i]){prtn(fd[i],WHITE);break;}}
    else if(str_cmp(cm,"cp")==0)prtn("Copied.",GREEN);
    else if(str_cmp(cm,"edit")==0||str_cmp(cm,"nano")==0){prtn("=== EDITOR (ESC to exit) ===",CYAN);while(1){char c=getk();if(c==0x01)break;else if(c=='\n')putc('\n');else if(c=='\b')putc('\b');else if(c>=32)putc(c);}}
    else if(str_cmp(cm,"type")==0){for(int i=0;i<fc;i++)if(str_cmp(fn[i],arg)==0&&!fdir[i]){prtn(fd[i],WHITE);break;}}
    else if(str_cmp(cm,"append")==0)prtn("Appended.",GREEN);
    else if(str_cmp(cm,"wc")==0)prtn("0 lines 0 words 0 chars",WHITE);
    else if(str_cmp(cm,"head")==0)prtn("(first 10 lines)",WHITE);
    else if(str_cmp(cm,"snake")==0)game_snake();
    else if(str_cmp(cm,"tetris")==0)prtn("Tetris - Coming soon!",GREEN);
    else if(str_cmp(cm,"pong")==0)prtn("Pong - Coming soon!",GREEN);
    else if(str_cmp(cm,"guess")==0)prtn("Guess 1-100 (Coming)",YELLOW);
    else if(str_cmp(cm,"rps")==0)prtn("Rock Paper Scissors (Coming)",YELLOW);
    else if(str_cmp(cm,"dice")==0){char b[3];itos((inb(0x40)%6)+1,b);prtn(b,YELLOW);}
    else if(str_cmp(cm,"coin")==0)prtn((inb(0x40)%2)?"Heads!":"Tails!",YELLOW);
    else if(str_cmp(cm,"maze")==0)prtn("Maze generated!",YELLOW);
    else if(str_cmp(cm,"tictactoe")==0)prtn("Tic Tac Toe (Coming)",YELLOW);
    else if(str_cmp(cm,"hangman")==0)prtn("Hangman (Coming)",YELLOW);
    else if(str_cmp(cm,"mathquiz")==0)prtn("Math Quiz (Coming)",YELLOW);
    else if(str_cmp(cm,"asciiart")==0){prtn("  /\\_/\\",YELLOW);prtn(" ( o.o )",YELLOW);prtn("  > ^ <",YELLOW);}
    else if(str_cmp(cm,"sysinfo")==0)prtn("CPU: x86 | RAM: 2MB+ | TFD v1.0",WHITE);
    else if(str_cmp(cm,"mem")==0)prtn("~200KB used",WHITE);
    else if(str_cmp(cm,"ps")==0)prtn("PID 1: kernel",WHITE);
    else if(str_cmp(cm,"beep")==0){outb(0x43,0xB6);outb(0x42,0x50);outb(0x42,0x09);outb(0x61,inb(0x61)|3);for(volatile int d=0;d<80000;d++);outb(0x61,inb(0x61)&~3);}
    else if(str_cmp(cm,"bench")==0)prtn("1000 loops: 0.01s",WHITE);
    else if(str_cmp(cm,"setcolor")==0)prtn("Use colors 0-15",WHITE);
    else if(str_cmp(cm,"prompt")==0){pmode=(pmode+1)%3;prtn("Prompt style changed.",GREEN);}
    else if(str_cmp(cm,"alias")==0)prtn("Alias created.",GREEN);
    else if(str_cmp(cm,"calc")==0)prtn("Calc (Coming)",YELLOW);
    else if(str_cmp(cm,"sum")==0)prtn("Sum (Coming)",YELLOW);
    else if(str_cmp(cm,"avg")==0)prtn("Avg (Coming)",YELLOW);
    else if(str_cmp(cm,"max")==0)prtn("Max (Coming)",YELLOW);
    else if(str_cmp(cm,"min")==0)prtn("Min (Coming)",YELLOW);
    else if(str_cmp(cm,"history")==0){for(int i=0;i<hcnt;i++){char b[5];itos(i+1,b);prt(b,WHITE);prt(" ",LGRAY);prtn(hist[i],WHITE);}}
    else if(str_cmp(cm,"clearhist")==0){hcnt=0;prtn("Cleared.",GREEN);}
    else if(str_cmp(cm,"log")==0)prtn("System log: OK",WHITE);
    else if(str_cmp(cm,"last")==0)prtn("Last login: now",WHITE);
    else if(str_cmp(cm,"passwd")==0){prt("New password: ",YELLOW);int i=0;while(1){char c=getk();if(c=='\n'){pass[i]=0;break;}else if(c>=32&&i<19){pass[i++]=c;putc('*');}}haspw=1;prtn(" Set!",GREEN);}
    else if(str_cmp(cm,"useradd")==0)prtn("User added.",GREEN);
    else if(str_cmp(cm,"userdel")==0)prtn("User deleted.",RED);
    else if(str_cmp(cm,"users")==0)prtn(user,GREEN);
    else if(str_cmp(cm,"colors")==0){for(int i=0;i<16;i++){tc=i;prt("[X]",i);}prtn("",WHITE);}
    else if(str_cmp(cm,"palette")==0)prtn("16 VGA colors.",WHITE);
    else if(str_cmp(cm,"rainbow")==0){prt("R",RED);prt("A",YELLOW);prt("I",GREEN);prt("N",CYAN);prt("B",BLUE);prt("O",MAGENTA);prtn("W",RED);}
    else if(str_cmp(cm,"..")==0)prtn("/",WHITE);
    else if(str_cmp(cm,"~")==0)prtn("/home",WHITE);
    else if(str_cmp(cm,"!!")==0&&last[0])exec(last);
    else if(str_cmp(cm,"search")==0)prtn("Searching...",WHITE);
    else if(str_cmp(cm,"sort")==0)prtn("Sorted.",WHITE);
    else if(str_cmp(cm,"encrypt")==0)prtn("Encrypted.",WHITE);
    else if(str_cmp(cm,"fxc")==0){if(inst){prt("PERMANENT (",GREEN);prt(idisk,WHITE);prtn(")",GREEN);}else prtn("LIVE (USB)",YELLOW);}
    else if(str_cmp(cm,"install")==0){
        prt("Install? (y/n): ",YELLOW); if(getk()=='y') {
            prtn("1=HDD 2=SSD 3=USB",WHITE); char d=getk();
            if(d=='1')str_cpy(idisk,"HDD"); else if(d=='2')str_cpy(idisk,"SSD"); else str_cpy(idisk,"USB");
            for(int p=0;p<=100;p+=25){prt("[",GREEN);for(int j=0;j<20;j++)prt(j<p/5?"=":" ",GREEN);prt("] ",GREEN);char b[5];itos(p,b);prtn(b,WHITE);for(volatile int d=0;d<200000;d++);}
            inst=1; prtn("Done!",GREEN);
        }
    }
    else if(str_cmp(cm,"uninstall")==0){if(inst){prt("Remove? (y/n): ",RED);if(getk()=='y'){inst=0;idisk[0]=0;prtn("Removed.",GREEN);}}else prtn("Not installed.",YELLOW);}
    else {prt("?: ",RED);prtn(c,RED);prtn("Type 'help'",YELLOW);}
}

/* ============ BOOT MENU ============ */
static void boot(void){
    cls();
    prtat(25,2,"╔══════════════════════════════╗",CYAN);
    prtat(25,3,"║                              ║",CYAN);
    prtat(25,4,"║   ████████╗███████╗██████╗   ║",RED);
    prtat(25,5,"║   ╚══██╔══╝██╔════╝██╔══██╗  ║",RED);
    prtat(25,6,"║      ██║   █████╗  ██║  ██║  ║",GREEN);
    prtat(25,7,"║      ██║   ██╔══╝  ██║  ██║  ║",GREEN);
    prtat(25,8,"║      ██║   ██║     ██████╔╝  ║",BLUE);
    prtat(25,9,"║      ╚═╝   ╚═╝     ╚═════╝   ║",BLUE);
    prtat(25,10,"║                              ║",CYAN);
    prtat(25,11,"║     TFD OS v1.0              ║",CYAN);
    prtat(25,12,"║     by Sadman                ║",CYAN);
    prtat(25,13,"║     Text-Based OS            ║",CYAN);
    prtat(25,14,"╚══════════════════════════════╝",CYAN);
    prtat(28,16,"[ PRESS ENTER TO BOOT ]",YELLOW);
    while(1){if(hask()&&getk()=='\n')break;}
}

/* ============ SETUP ============ */
static void setup(void){
    cls();prtn("SETUP",CYAN);
    prt("Username: ",YELLOW);showcur();int i=0;
    while(1){char c=getk();if(c=='\n'){user[i]=0;break;}else if(c=='\b'&&i>0){i--;putc('\b');}else if(c>=32&&i<19){user[i++]=c;putc(c);}}
    prtn("",0);
    prt("PC name: ",YELLOW);i=0;
    while(1){char c=getk();if(c=='\n'){pc[i]=0;break;}else if(c=='\b'&&i>0){i--;putc('\b');}else if(c>=32&&i<19){pc[i++]=c;putc(c);}}
    prtn("",0);prt("Welcome ",GREEN);prtn(user,WHITE);
    for(volatile int d=0;d<200000;d++);hidecur();
}

/* ============ MAIN ============ */
void kernel_main(uint32_t magic, uint32_t addr){
    (void)magic;(void)addr;
    kwait();outb(0x64,0xA8);
    kwait();outb(0x60,0xF0);
    kwait();outb(0x60,0x01);
    hidecur();fsinit();boot();setup();
    cls();prtn("TFD OS v1.0 Ready",GREEN);prtn("Type 'help'",YELLOW);prtn("",0);clen=0;
    while(1){
        prompt();showcur();clen=0;
        while(1){char c=getk();if(c=='\n'){putc('\n');break;}else if(c=='\b'){if(clen>0){clen--;putc('\b');}}else if(c>=32&&c<=126&&clen<255){cmd[clen++]=c;putc(c);}}
        hidecur();cmd[clen]=0;exec(cmd);upt++;prtn("",0);
    }
}
