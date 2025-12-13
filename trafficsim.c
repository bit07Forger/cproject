#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <conio.h>

#define SLEEP(ms) Sleep(ms)
#define KBHIT() _kbhit()
#define GETCH() _getch()

#define CLEAR "\033[2J\033[H"
#define POS(r,c) printf("\033[%d;%dH",(r),(c))
#define RED "\033[31m"
#define YEL "\033[33m"
#define GRN "\033[32m"
#define BLU "\033[34m"
#define CYN "\033[36m"
#define MAG "\033[35m"
#define RST "\033[0m"

#define W 80
#define H 24
#define IX 40
#define IY 12
#define NS_W 6
#define EW_W 4
#define L_BORDER (IX-NS_W/2)
#define R_BORDER (IX+NS_W/2-1)
#define T_BORDER (IY-EW_W/2)
#define B_BORDER (IY+EW_W/2-1)
#define MAX_CARS 20
#define MAX_PEDS 10
#define PED_INTERVAL 4

typedef enum {RED_L,YEL_L,GRN_L} LightState;
typedef enum {NORTH,SOUTH,EAST,WEST} Dir;
typedef enum {DONT_WALK,WALK} PedSignal;

typedef struct {
    int x,y,active,crossed;
    Dir dir;
    char sym;
} Car;

typedef struct {
    int x,y,active;
    Dir dir;
    char sym;
} Ped;

typedef struct {
    LightState state;
    int timer,g,y,r;
} Light;

Car cars[MAX_CARS];
Ped peds[MAX_PEDS];
char grid[H][W];
Light nsLight,ewLight;
PedSignal ns_ped,ew_ped;
int ns_interval=3,ew_interval=3,lane_change=0;
char lc_msg[100]="";
int lc_ticks=0;

void initGrid() {
    for(int i=0;i<H;i++)
        for(int j=0;j<W;j++)
            grid[i][j]=' ';
    
    for(int j=0;j<W;j++) {
        grid[T_BORDER][j]='-';
        grid[B_BORDER][j]='-';
    }
    for(int i=0;i<H;i++) {
        grid[i][L_BORDER]='|';
        grid[i][R_BORDER]='|';
    }
    grid[T_BORDER][L_BORDER]=grid[T_BORDER][R_BORDER]='+';
    grid[B_BORDER][L_BORDER]=grid[B_BORDER][R_BORDER]='+';
    
    for(int j=L_BORDER;j<=R_BORDER;j++)
        if(grid[IY][j]==' ') grid[IY][j]='-';
    
    int y1=T_BORDER-2,y2=B_BORDER+2,x1=L_BORDER-2,x2=R_BORDER+2;
    for(int j=L_BORDER;j<=R_BORDER;j++) {
        if(grid[y1][j]==' ') grid[y1][j]=(j%2?' ':':');
        if(grid[y2][j]==' ') grid[y2][j]=(j%2?' ':':');
    }
    for(int i=T_BORDER;i<=B_BORDER;i++) {
        if(grid[i][x1]==' ') grid[i][x1]=(i%2?' ':':');
        if(grid[i][x2]==' ') grid[i][x2]=(i%2?' ':':');
    }
}

void drawGrid() {
    POS(1,1);
    for(int i=0;i<H;i++) {
        fwrite(grid[i],1,W,stdout);
        putchar('\n');
    }
}

void initLights() {
    nsLight=(Light){GRN_L,50,50,10,50};
    ewLight=(Light){RED_L,50,50,10,50};
}

void updateLight(Light *t) {
    if(--t->timer>0) return;
    if(t->state==GRN_L) {t->state=YEL_L;t->timer=t->y;}
    else if(t->state==YEL_L) {t->state=RED_L;t->timer=t->r;}
    else {t->state=GRN_L;t->timer=t->g;}
}

int countCars(Dir d) {
    int c=0;
    for(int i=0;i<MAX_CARS;i++)
        if(cars[i].active&&cars[i].dir==d) c++;
    return c;
}

void adaptiveControl() {
    int ns=countCars(NORTH)+countCars(SOUTH);
    int ew=countCars(EAST)+countCars(WEST);
    
    if(nsLight.state==GRN_L&&nsLight.timer==nsLight.g) {
        if(ns>ew+3) {nsLight.g=80;ewLight.g=50;}
        else if(ew>ns+3) {ewLight.g=80;nsLight.g=50;}
        else {nsLight.g=ewLight.g=50;}
        if(nsLight.g<20) nsLight.g=20;
        if(ewLight.g<20) ewLight.g=20;
    }
}

void updatePedSignals() {
    ns_ped=(nsLight.state==RED_L)?WALK:DONT_WALK;
    ew_ped=(ewLight.state==RED_L)?WALK:DONT_WALK;
}

void drawLights() {
    adaptiveControl();
    updatePedSignals();
    
    int ns=countCars(NORTH)+countCars(SOUTH);
    int ew=countCars(EAST)+countCars(WEST);
    
    POS(H+2,1);
    printf("N-S: ");
    printf(nsLight.state==RED_L?RED"RED   "RST:
           nsLight.state==YEL_L?YEL"YELLOW"RST:GRN"GREEN "RST);
    printf("  (%.1fs)   Density:%d/%d          ",nsLight.timer/10.0,ns,MAX_CARS);

    POS(H+3,1);
    printf("E-W: ");
    printf(ewLight.state==RED_L?RED"RED   "RST:
           ewLight.state==YEL_L?YEL"YELLOW"RST:GRN"GREEN "RST);
    printf("  (%.1fs)   Density:%d/%d          ",ewLight.timer/10.0,ew,MAX_CARS);

    POS(H+4,1);
    printf("Speed - N-S:%.2f E-W:%.2f cells/s                    ",
           10.0/ns_interval,10.0/ew_interval);
    
    POS(H+5,1);
    printf("N-S Ped:");
    printf(ns_ped==WALK?GRN"WALK   "RST:RED"DON'T WALK"RST);
    printf("  E-W Ped:");
    printf(ew_ped==WALK?GRN"WALK   "RST:RED"DON'T WALK"RST);
    printf("  [Press 'p' to pause]     ");
    
    POS(H+6,1);
    if(lc_ticks>0) {
        printf("[LANE CHANGE] %s                              ",lc_msg);
        lc_ticks--;
    } else printf("                                                                ");
}

void initCars() {
    for(int i=0;i<MAX_CARS;i++)
        cars[i].active=cars[i].crossed=0;
}

void initPeds() {
    for(int i=0;i<MAX_PEDS;i++)
        peds[i].active=0;
}

int inIntersection(int x,int y) {
    return x>=L_BORDER&&x<=R_BORDER&&y>=T_BORDER&&y<=B_BORDER;
}

int atStopLine(Car *c) {
    if(!c->active) return 0;
    if(c->dir==NORTH) return c->y==B_BORDER+1;
    if(c->dir==SOUTH) return c->y==T_BORDER-1;
    if(c->dir==EAST) return c->x==L_BORDER-1;
    if(c->dir==WEST) return c->x==R_BORDER+1;
    return 0;
}

int hasCrossed(Car *c) {
    if(c->dir==NORTH) return c->y<T_BORDER;
    if(c->dir==SOUTH) return c->y>B_BORDER;
    if(c->dir==EAST) return c->x>R_BORDER;
    if(c->dir==WEST) return c->x<L_BORDER;
    return 0;
}

int canMove(Car *c) {
    if(!c->active||c->crossed||inIntersection(c->x,c->y)) return 1;
    if(atStopLine(c)) {
        if(c->dir==NORTH||c->dir==SOUTH) return nsLight.state!=RED_L;
        return ewLight.state!=RED_L;
    }
    return 1;
}

int isOccupied(int x,int y,int skip) {
    for(int i=0;i<MAX_CARS;i++)
        if(i!=skip&&cars[i].active&&cars[i].x==x&&cars[i].y==y) return 1;
    return 0;
}

int pedAt(int x,int y) {
    for(int i=0;i<MAX_PEDS;i++)
        if(peds[i].active&&peds[i].x==x&&peds[i].y==y) return 1;
    return 0;
}

int carAt(int x,int y) {
    for(int i=0;i<MAX_CARS;i++)
        if(cars[i].active&&cars[i].x==x&&cars[i].y==y) return 1;
    return 0;
}

void drawCar(Car *c,int erase) {
    if(c->y<0||c->y>=H||c->x<0||c->x>=W) return;
    POS(c->y+1,c->x+1);
    if(erase) putchar(grid[c->y][c->x]);
    else {
        if(atStopLine(c)) printf(YEL"%c"RST,c->sym);
        else if(inIntersection(c->x,c->y)) printf(GRN"%c"RST,c->sym);
        else if(c->crossed) printf(CYN"%c"RST,c->sym);
        else printf(BLU"%c"RST,c->sym);
    }
}

void drawPed(Ped *p,int erase) {
    if(p->y<0||p->y>=H||p->x<0||p->x>=W) return;
    POS(p->y+1,p->x+1);
    erase?putchar(grid[p->y][p->x]):printf(CYN"%c"RST,p->sym);
}

void spawnCar() {
    int s=-1;
    for(int i=0;i<MAX_CARS;i++)
        if(!cars[i].active) {s=i;break;}
    if(s<0) return;
    
    Car *c=&cars[s];
    c->active=1;
    c->crossed=0;
    c->dir=rand()%4;
    
    switch(c->dir) {
        case NORTH:c->x=L_BORDER+2;c->y=H-2;c->sym='^';break;
        case SOUTH:c->x=R_BORDER-2;c->y=1;c->sym='v';break;
        case EAST:c->x=1;c->y=B_BORDER-1;c->sym='>';break;
        case WEST:c->x=W-2;c->y=T_BORDER+1;c->sym='<';break;
    }
    if(isOccupied(c->x,c->y,s)) c->active=0;
}

void spawnPed() {
    int s=-1;
    for(int i=0;i<MAX_PEDS;i++)
        if(!peds[i].active) {s=i;break;}
    if(s<0) return;
    
    Ped *p=&peds[s];
    p->active=1;
    p->dir=rand()%4;
    p->sym='P';
    
    switch(p->dir) {
        case EAST:
            p->x=L_BORDER;
            p->y=T_BORDER-2;
            break;
        case WEST:
            p->x=R_BORDER;
            p->y=T_BORDER-2;
            break;
        case SOUTH:
            p->x=L_BORDER-2;
            p->y=T_BORDER;
            break;
        case NORTH:
            p->x=L_BORDER-2;
            p->y=B_BORDER;
            break;
    }
}

void updatePeds(int tick) {
    for(int i=0;i<MAX_PEDS;i++) {
        Ped *p=&peds[i];
        if(!p->active) continue;
        
        int can=0;
        
        if((p->dir==NORTH||p->dir==SOUTH)&&ew_ped==WALK) can=1;
        else if((p->dir==EAST||p->dir==WEST)&&ns_ped==WALK) can=1;
        
        if(can&&tick%PED_INTERVAL==0) {
            int nx=p->x,ny=p->y;
            if(p->dir==NORTH) ny--;
            else if(p->dir==SOUTH) ny++;
            else if(p->dir==EAST) nx++;
            else nx--;
            
            if(carAt(nx,ny)) {
                drawPed(p,0);
                continue;
            }
            
            drawPed(p,1);
            p->x=nx;
            p->y=ny;
            
            if(p->dir==EAST&&p->x>R_BORDER) p->active=0;
            else if(p->dir==WEST&&p->x<L_BORDER) p->active=0;
            else if(p->dir==SOUTH&&p->y>B_BORDER) p->active=0;
            else if(p->dir==NORTH&&p->y<T_BORDER) p->active=0;
            else if(p->x<0||p->x>=W||p->y<0||p->y>=H) p->active=0;
            else drawPed(p,0);
        } else if(p->active) drawPed(p,0);
    }
}

void updateCars(int tick) {
    for(int i=0;i<MAX_CARS;i++) {
        Car *c=&cars[i];
        if(!c->active) continue;
        
        int iv=(c->dir==NORTH||c->dir==SOUTH)?ns_interval:ew_interval;
        if(iv>1&&tick%iv) {drawCar(c,0);continue;}
        
        int nx=c->x,ny=c->y;
        if(c->dir==NORTH) ny--;
        else if(c->dir==SOUTH) ny++;
        else if(c->dir==EAST) nx++;
        else nx--;
        
        if(nx<0||nx>=W||ny<0||ny>=H) {
            drawCar(c,1);
            c->active=0;
            continue;
        }
        
        if(!canMove(c)||isOccupied(nx,ny,i)||pedAt(nx,ny)) {
            if(lane_change&&countCars(c->dir)>5) {
                int sw=0;
                if(c->dir==NORTH||c->dir==SOUTH) {
                    if(c->x>0&&!isOccupied(c->x-1,c->y,i)) {
                        drawCar(c,1);c->x--;drawCar(c,0);sw=1;
                        snprintf(lc_msg,100,"Car %d lane LEFT",i);lc_ticks=20;
                    } else if(c->x<W-1&&!isOccupied(c->x+1,c->y,i)) {
                        drawCar(c,1);c->x++;drawCar(c,0);sw=1;
                        snprintf(lc_msg,100,"Car %d lane RIGHT",i);lc_ticks=20;
                    }
                } else {
                    if(c->y>0&&!isOccupied(c->x,c->y-1,i)) {
                        drawCar(c,1);c->y--;drawCar(c,0);sw=1;
                        snprintf(lc_msg,100,"Car %d lane UP",i);lc_ticks=20;
                    } else if(c->y<H-1&&!isOccupied(c->x,c->y+1,i)) {
                        drawCar(c,1);c->y++;drawCar(c,0);sw=1;
                        snprintf(lc_msg,100,"Car %d lane DOWN",i);lc_ticks=20;
                    }
                }
                if(sw) continue;
            }
            drawCar(c,0);
            continue;
        }
        
        drawCar(c,1);
        c->x=nx;
        c->y=ny;
        if(!c->crossed) c->crossed=hasCrossed(c);
        drawCar(c,0);
    }
}

void menu() {
    printf(CLEAR CYN);
    printf("||============================================================||\n");
    printf("||          TRAFFIC INTERSECTION SIMULATION SYSTEM            ||\n");
    printf("||============================================================||\n"RST"\n");
    printf(MAG"  Features:\n"RST);
    printf("1. Adaptive traffic control\n2. Speed control (N-S/E-W)\n3. Live countdown\n4. Lane-change (>5 cars)\n5. Pedestrian crossing\n\n"GRN"  MENU\n"RST);
    printf("-----------------------------------------------\n-----------------------------------------------\n");
    printf("1. Custom Simulation\n2. Standard (60s)\n3. With Lane-Change\n4. Exit\n\n");
    printf("-----------------------------------------------\n-----------------------------------------------\n  Choice (1-4): ");
    fflush(stdout);
}

void run(int dur) {
    printf(CLEAR);
    initGrid();
    initLights();
    initCars();
    initPeds();
    drawGrid();
    int f=dur*10,t=0,paused=0;
    while(f>0) {
        if(KBHIT()) {
            char c=GETCH();
            if(c=='p'||c=='P') {
                paused=!paused;
                POS(H+7,1);
                if(paused) printf(YEL"[PAUSED - Press 'p' to resume]                    "RST);
                else printf("                                                   ");
                fflush(stdout);
            }
        }
        
        if(!paused) {
            t++;
            f--;
            updateLight(&nsLight);
            updateLight(&ewLight);
            drawLights();
            if(f%10==0) spawnCar();
            if(f%15==0) spawnPed();
            updateCars(t);
            updatePeds(t);
        }
        fflush(stdout);
        SLEEP(100);
    }
    printf(CLEAR GRN"Simulation completed!\n"RST"Press Enter...");
    getchar();getchar();
}

int main() {
    srand(time(NULL));
    int ch,dur;
    double ns,ew;
    while(1) {
        menu();
        scanf("%d",&ch);
        if(ch==1) {
            printf("\nDuration (1-300): ");
            scanf("%d",&dur);
            if(dur<1) dur=1;
            if(dur>300) dur=300;
            printf("N-S speed (cells/s, 0=default): ");
            scanf("%lf",&ns);
            printf("E-W speed (cells/s, 0=default): ");
            scanf("%lf",&ew);
            ns_interval=(ns>0)?(int)(10.0/ns+0.5):3;
            ew_interval=(ew>0)?(int)(10.0/ew+0.5):3;
            if(ns_interval<1) ns_interval=1;
            if(ew_interval<1) ew_interval=1;
            printf("Lane-change? (0/1): ");
            scanf("%d",&lane_change);
            run(dur);
        } else if(ch==2) {
            ns_interval=ew_interval=3;
            lane_change=0;
            run(60);
        } else if(ch==3) {
            printf("\nDuration (1-300): ");
            scanf("%d",&dur);
            if(dur<1) dur=1;
            if(dur>300) dur=300;
            printf("N-S speed (0=default): ");
            scanf("%lf",&ns);
            printf("E-W speed (0=default): ");
            scanf("%lf",&ew);
            ns_interval=(ns>0)?(int)(10.0/ns+0.5):3;
            ew_interval=(ew>0)?(int)(10.0/ew+0.5):3;
            if(ns_interval<1) ns_interval=1;
            if(ew_interval<1) ew_interval=1;
            lane_change=1;
            run(dur);
        } else if(ch==4) return 0;
        else {
            printf("\nInvalid.\n");
            SLEEP(1500);
        }
    }
}