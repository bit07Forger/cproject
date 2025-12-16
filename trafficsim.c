#include <stdio.h>
#include <stdlib.h> // for funtions like srand in the function 
#include <time.h> // get the current system time 
#include <windows.h> // for the delays in milliseconds 
#include <conio.h> 

#define SLEEP(ms) Sleep(ms)
#define KBHIT() _kbhit()
#define GETCH() _getch()

#define CLEAR "\033[2J\033[H"                   //ANSI escape code for clearing screen 
#define POS(r,c) printf("\033[%d;%dH",(r),(c))  // for getting the positions (rows,colums)
#define RED "\033[31m"  // for the colors in the terminals 
#define YEL "\033[33m"
#define GRN "\033[32m"
#define BLU "\033[34m"
#define CYN "\033[36m"
#define MAG "\033[35m"
#define RST "\033[0m"
  // defining all the contants 
#define W 80 // width of the 2d array representing the grid 
#define H 24 // height of the 2d array representing the grid
#define IX 40 //center of the x cooridinate of 2d array that is defined by w and h 
#define IY 12 // center of the y coordinate of 2d array that is defined by w and h
#define NS_W 8 //north south road width 
#define EW_W 4//east west road width
#define L_BORDER (IX-NS_W/2) // calculates the left border 
#define R_BORDER (IX+NS_W/2-1) // calculates the right border 
#define T_BORDER (IY-EW_W/2) // calculates the top border 
#define B_BORDER (IY+EW_W/2-1) // calculates the bottom border 
#define MAX_CARS 20 // maximum no.of cars 
#define MAX_PEDS 10 // maximum no.of pedestrians 
#define PED_INTERVAL 4 // movement intervals of the pedestrians  

typedef enum {RED_L,YEL_L,GRN_L} LightState; // traffic light states: red, yellow, green
typedef enum {NORTH,SOUTH,EAST,WEST} Dir; // directions: north, south, east, west
typedef enum {DONT_WALK,WALK} PedSignal; // pedestrian signals: don't walk, walk

const char* dir_names[] = {"NORTH", "SOUTH", "EAST", "WEST"}; // string names for directions
// defining the structures for all the entities 
typedef struct {
    int x,y,active,crossed; //x,y are being used as coordintes on the grid 
    Dir dir; // direction of the car
    char sym; // to represent the car 
} Car;

typedef struct {
    int x,y,active;
    Dir dir; // direction of the pedestrian
    char sym; // symbol for pedestrian
} Ped;

typedef struct {
    LightState state; // current light state
    int timer,g,y,r; // timer count downs for each light state 
} Light;

Car cars[MAX_CARS]; // array to hold all cars
Ped peds[MAX_PEDS]; // array to hold all pedestrians
char grid[H][W]; // 2D grid representing the road and intersection
Light nsLight,ewLight; // traffic lights for north-south and east-west
PedSignal ns_ped,ew_ped; // pedestrian signals for north-south and east-west
int ns_interval=3,ew_interval=3,lane_change=0; // speed intervals and lane change flag
char lc_msg[100]=""; // lane change message buffer
int lc_ticks=0; // ticks to display lane change message

void initGrid() {   // setup for clear screen 
    for(int i=0;i<H;i++)      
        for(int j=0;j<W;j++)
            grid[i][j]=' '; // initialize all grid cells to space
    
    for(int j=0;j<W;j++) {
        grid[T_BORDER][j]='-'; // draw top border
        grid[B_BORDER][j]='-'; // draw bottom border
    }
    for(int i=0;i<H;i++) {
        grid[i][L_BORDER]='|'; // draw left border
        grid[i][R_BORDER]='|'; // draw right border
    }
    grid[T_BORDER][L_BORDER]=grid[T_BORDER][R_BORDER]='+'; // draw top-left and top-right corners
    grid[B_BORDER][L_BORDER]=grid[B_BORDER][R_BORDER]='+'; // draw bottom-left and bottom-right corners
    
    int y1=T_BORDER-2,y2=B_BORDER+2,x1=L_BORDER-2,x2=R_BORDER+2; // calculate zebra line positions
    for(int j=L_BORDER;j<=R_BORDER;j++) {
        if(grid[y1][j]==' ') grid[y1][j]=(j%2?' ':':'); // draw zebra lines above intersection
        if(grid[y2][j]==' ') grid[y2][j]=(j%2?' ':':'); // draw zebra lines below intersection
    }
    for(int i=T_BORDER;i<=B_BORDER;i++) {
        if(grid[i][x1]==' ') grid[i][x1]=(i%2?' ':':'); // draw zebra lines left of intersection
        if(grid[i][x2]==' ') grid[i][x2]=(i%2?' ':':'); // draw zebra lines right of intersection
    }
}

void drawGrid() {
    POS(1,1); // moves the cursor to the top left corner 
    for(int i=0;i<H;i++) {
        fwrite(grid[i],1,W,stdout); // writes 80 characters from grid to s
        putchar('\n');
    }
}

void initLights() {
    nsLight=(Light){GRN_L,50,50,10,50}; // initial state of the north south lights is green 
    ewLight=(Light){RED_L,50,50,10,50}; // opposite for east west lights to avoid collision 
}

void updateLight(Light *t) { // for updating the traffic lights green=5secs yellow=1sec red =5secs 
    if(--t->timer>0) return; // first subtracts one from the timer and then checks if its greater than zero 
    if(t->state==GRN_L) {t->state=YEL_L;t->timer=t->y;} // if greeen light times out then change to yellow and then set the timer to yellow 
    else if(t->state==YEL_L) {t->state=RED_L;t->timer=t->r;} // if yellow light times out then change to the red and set the timer to red 
    else {t->state=GRN_L;t->timer=t->g;} // if red light times out then change to green and set the timer to green 
}

int countCars(Dir d) {
    int c=0; // initialize count
    for(int i=0;i<MAX_CARS;i++) // counts the number of cars in a particular direction 
        if(cars[i].active&&cars[i].dir==d) c++; // increments the count if the car is active and in the specified direction 
    return c; // returns the count of the cars in that direction 
}

void adaptiveControl() {
    int ns=countCars(NORTH)+countCars(SOUTH); // uses the above function to count the number of cars in north south dirn  
    int ew=countCars(EAST)+countCars(WEST); // uses the above function to count the number pf cars in east west dirn 
    
    if(nsLight.state==GRN_L&&nsLight.timer==nsLight.g) {
        if(ns>ew+3) {nsLight.g=80;ewLight.g=50;} // if there are more cars in n-s dirn then increase the green light time to 8 secs and the e-w light remains default 
        else if(ew>ns+3) {ewLight.g=80;nsLight.g=50;} //if there are more cars in e-w dirn then increase the green light time to 8 secs and the n-s light remains default
        else {nsLight.g=ewLight.g=50;}
        if(nsLight.g<20) nsLight.g=20; // minimum green light time is 2 secs 
        if(ewLight.g<20) ewLight.g=20; // minimum green light time is 2 secs
    }
}

void updatePedSignals() {
    ns_ped=(nsLight.state==RED_L)?WALK:DONT_WALK; // if the north south light is red then pedestrians can walk otherwise they cant 
    ew_ped=(ewLight.state==RED_L)?WALK:DONT_WALK; // if the east west light is red then pedestrians can walk otherwise they cant 
}

void drawLights() {
    adaptiveControl(); // update adaptive control
    updatePedSignals(); // update pedestrian signals
    
    int ns=countCars(NORTH)+countCars(SOUTH); // count north-south cars
    int ew=countCars(EAST)+countCars(WEST); // count east-west cars
    
    POS(H+2,1); // position for north-south light display
    printf("N-S: ");
    printf(nsLight.state==RED_L?RED"RED   "RST:
           nsLight.state==YEL_L?YEL"YELLOW"RST:GRN"GREEN "RST); // display light color
    printf("  (%.1fs)   Density:%d/%d          ",nsLight.timer/10.0,ns,MAX_CARS); // display timer and density

    POS(H+3,1); // position for east-west light display
    printf("E-W: ");
    printf(ewLight.state==RED_L?RED"RED   "RST:
           ewLight.state==YEL_L?YEL"YELLOW"RST:GRN"GREEN "RST); // display light color
    printf("  (%.1fs)   Density:%d/%d          ",ewLight.timer/10.0,ew,MAX_CARS); // display timer and density

    POS(H+4,1); // position for speed display
    printf("Speed - N-S:%.2f E-W:%.2f cells/s                    ",
           10.0/ns_interval,10.0/ew_interval); // display speeds
    
    POS(H+5,1); // position for pedestrian signals
    printf("N-S Ped:");
    printf(ns_ped==WALK?GRN"WALK   "RST:RED"DON'T WALK"RST); // display north-south pedestrian signal
    printf("  E-W Ped:");
    printf(ew_ped==WALK?GRN"WALK   "RST:RED"DON'T WALK"RST); // display east-west pedestrian signal
    printf("  [Press 'p' to pause]     "); // pause instruction
    
    POS(H+6,1); // position for lane change message
    if(lc_ticks>0) {
        printf("[LANE CHANGE] %s                              ",lc_msg); // display lane change message
        lc_ticks--; // decrement ticks
    } else printf("                                                                "); // clear line
}

void initCars() {
    for(int i=0;i<MAX_CARS;i++)
        cars[i].active=cars[i].crossed=0; // creates inactive cars 20 in number ready to be spawned 
}

void initPeds() {
    for(int i=0;i<MAX_PEDS;i++) // creates inactive pedestrians 10 in number ready to be spawned 
        peds[i].active=0; // set active to 0
}

int inIntersection(int x,int y) {
    return x>=L_BORDER&&x<=R_BORDER&&y>=T_BORDER&&y<=B_BORDER; // check if position is within intersection bounds
}

int atStopLine(Car *c) { 
    if(!c->active) return 0; // inactive car not at stop line
    if(c->dir==NORTH) return c->y==B_BORDER+1; // checks if the car is at the stop line based on the direction it was moving initially 
    if(c->dir==SOUTH) return c->y==T_BORDER-1; 
    if(c->dir==EAST) return c->x==L_BORDER-1;
    if(c->dir==WEST) return c->x==R_BORDER+1;
    return 0; // not at stop line
}

int hasCrossed(Car *c) { // checks whether the car has already crossed the intersection so that it can stop checking for the light signals 
    if(c->dir==NORTH) return c->y<T_BORDER; 
    if(c->dir==SOUTH) return c->y>B_BORDER;
    if(c->dir==EAST) return c->x>R_BORDER;
    if(c->dir==WEST) return c->x<L_BORDER;
    return 0; // not crossed
}

int canMove(Car *c) {
    if(!c->active||c->crossed||inIntersection(c->x,c->y)) return 1; // inactive car , already crossed, at the intersection then no need to check already committed 
    if(atStopLine(c)) { 
        if(c->dir==NORTH||c->dir==SOUTH) 
        return nsLight.state!=RED_L; // can move if not red 
        return ewLight.state!=RED_L;
    }
    return 1; // can move
}

int isOccupied(int x,int y,int skip) { // checks if the position is already occupied or not except for itslef 
    for(int i=0;i<MAX_CARS;i++)
        if(i!=skip //dont check itself 
            &&cars[i].active // car is active 
            &&cars[i].x==x&& // x cooriddinate is the same 
            cars[i].y==y) return 1; // y coordinate is the same 
    return 0; // returns 0 if the position is not occupied and 1 if it is occupied 
}

int pedAt(int x,int y) { // takes the coordinates of the cars next movement as the input 
    for(int i=0;i<MAX_PEDS;i++) // checks whether the position is already occupied by the pedestrian or not 
        if(peds[i].active&&peds[i].x==x&&peds[i].y==y) return 1; // if true then the car cannot move further 
    return 0; // no pedestrian at position
}

int carAt(int x,int y) {
    for(int i=0;i<MAX_CARS;i++)
        if(cars[i].active&&cars[i].x==x&&cars[i].y==y) return 1; // check if car at position
    return 0; // no car at position
}

void drawCar(Car *c,int erase) {  // erase can be either 0 or 1 
    if(c->y<0||c->y>=H||c->x<0||c->x>=W) return; // check bounds
    POS(c->y+1,c->x+1); // moves the cursor 
    if(erase) putchar(grid[c->y][c->x]); // clear the background  
    else { // 
        if(atStopLine(c)) printf(YEL"%c"RST,c->sym); // if at the stop light then turns yellow 
        else if(inIntersection(c->x,c->y)) printf(GRN"%c"RST,c->sym); // print in green when at the intersection 
        else if(c->crossed) printf(CYN"%c"RST,c->sym);// cyan when the car has crossed 
        else printf(BLU"%c"RST,c->sym); // blue if in the lanes in travel 
    }
}

void drawPed(Ped *p,int erase) {
    if(p->y<0||p->y>=H||p->x<0||p->x>=W) return; // check bounds
    POS(p->y+1,p->x+1); // position cursor
    erase?putchar(grid[p->y][p->x]):printf(CYN"%c"RST,p->sym); // erase or draw pedestrian
}

void spawnCar() {  // spawns a car at random direction 
    int s=-1; // not found marker 
    for(int i=0;i<MAX_CARS;i++) // checks for all the 20 slots for anny empty places 
        if(!cars[i].active) {s=i;break;} // remembers the slot index and exits the loop 
    if(s<0) return; // checks if found otherwise exits spawning 
    
    Car *c=&cars[s]; // pointer to the car slot 
    c->active=1; // -> is the arrow operator used instead of . for accesing struct memebers with pointers 
    c->crossed=0;
    c->dir=rand()%4; // used to give the values 0 1 2 3 because (rand no.)%4 never exceed 3 as the range of the value is 0 to m-1
    // 25% chance for each direction 
    switch(c->dir) {
        case NORTH:c->x=L_BORDER+2;c->y=H-2;c->sym='^';break; // spawns at the position (39,22) and sets the symbol for the car spwaning 
        case SOUTH:c->x=R_BORDER-2;c->y=1;c->sym='v';break;// spawns at position (40,1)
        case EAST:c->x=1;c->y=B_BORDER-1;c->sym='>';break;// spawns at position (1,13)
        case WEST:c->x=W-2;c->y=T_BORDER+1;c->sym='<';break;// spawns at position (78,12)
    }
    if(isOccupied(c->x,c->y,s)) c->active=0;  // if the spawn point is occupies then deactivate the car 
}

void spawnPed() {
    int s=-1; // not found marker searches all the slots for the pedestrians 
    for(int i=0;i<MAX_PEDS;i++)
        if(!peds[i].active) {s=i;break;} // find inactive pedestrian slot
    if(s<0) return; // no slot available
    
    Ped *p=&peds[s]; // pointer to pedestrian slot
    p->active=1; // activate pedestrian
    p->dir=rand()%4; // random direction
    p->sym='P'; // always P only 
    
    switch(p->dir) {
        case EAST:
            p->x=L_BORDER; // spawn at left border at (37,8)
            p->y=T_BORDER-2; // above intersection
            break;
        case WEST:
            p->x=R_BORDER; // spawn at right border at(42,8)
            p->y=T_BORDER-2; // above intersection
            break;
        case SOUTH:
            p->x=L_BORDER-2; // left of intersection at (35,10)
            p->y=T_BORDER; // at top border
            break;
        case NORTH:
            p->x=L_BORDER-2; // left of intersection ta (35,13))
            p->y=B_BORDER; // at bottom border
            break;
    }
}

void updatePeds(int tick) {
    for(int i=0;i<MAX_PEDS;i++) { // loop through all pedestrians
        Ped *p=&peds[i]; // ptr to pedestrian
        if(!p->active) continue; // skip inactive
        
        int move=0; // can move flag
        
        if((p->dir==NORTH||p->dir==SOUTH)&&ew_ped==WALK) move=1; // can move if east-west pedestrian signal is walk
        else if((p->dir==EAST||p->dir==WEST)&&ns_ped==WALK) move=1; // can move if north-south pedestrian signal is walk
        
        if(move&&tick%PED_INTERVAL==0) { // move if allowed and interval reached
            int nx=p->x,ny=p->y; // next position
            if(p->dir==NORTH) ny--; // move north
            else if(p->dir==SOUTH) ny++; // move south
            else if(p->dir==EAST) nx++; // move east
            else nx--; // move west
            
            if(carAt(nx,ny)) { // check if car at next position
                drawPed(p,0); // stops the pedestrian until the car is out of the way 
                continue; // don't move
            }
            
            drawPed(p,1); // erase old position
            p->x=nx; // update x
            p->y=ny; // update y
            
            if(p->dir==EAST&&p->x>R_BORDER) p->active=0; // deactivate if crossed right border
            else if(p->dir==WEST&&p->x<L_BORDER) p->active=0; // deactivate if crossed left border
            else if(p->dir==SOUTH&&p->y>B_BORDER) p->active=0; // deactivate if crossed bottom border
            else if(p->dir==NORTH&&p->y<T_BORDER) p->active=0; // deactivate if crossed top border
            else if(p->x<0||p->x>=W||p->y<0||p->y>=H) p->active=0; // deactivate if out of bounds
            else drawPed(p,0); // draw at new position
        } else if(p->active) drawPed(p,0); // redraw if active but not moving
    }
}

void updateCars(int tick) {
    for(int i=0;i<MAX_CARS;i++) { // checks for all the 20 cars 
        Car *c=&cars[i]; // gets the pointer to the car slot 
        if(!c->active) continue; //if the car is not active then skip to the next car 
        
        int iv=(c->dir==NORTH||c->dir==SOUTH)?ns_interval:ew_interval; // gets the speed intervals based on the direction of the car as entered by the user 
        if(iv>1&&tick%iv) {drawCar(c,0);continue;} // if the interval is greater than 1 and the tick is not a multiple of the interval then skip to the next car 
        // controls how often the car moves showing the diffrent speeds 
        int nx=c->x,ny=c->y; // starts with the current x an dy position 
        if(c->dir==NORTH) ny--; // for moving up the y coordinated decreases 
        else if(c->dir==SOUTH) ny++; // for moving south the y coordinate increases 
        else if(c->dir==EAST) nx++; // for moving east the x coordinate increases 
        else nx--; // for moving west the x coordinate decreases 
        
        if(nx<0||nx>=W||ny<0||ny>=H) {  // maintains the boundary conditions for the grid 
            drawCar(c,1); 
            c->active=0; // if the car moves out of the grid then deactivate it and remove it from the grid 
            continue; 
        }
        
        if(!canMove(c)||isOccupied(nx,ny,i)||pedAt(nx,ny)) { // checks for the following : traffic light, pedestrian, already occupied 
            if(lane_change&&countCars(c->dir)>5) { // when either option 3 is choosen or option 1 choosen and the car in that direction are more than 5 
                int sw=0; // flag variable for the switching 
                if(c->dir==NORTH||c->dir==SOUTH) { // vertical movement 
                    int dy = (c->dir == NORTH) ? -1 : 1; // direction increase for y
                    if(c->x>0 && c->x-1 >= L_BORDER && c->y + dy >= 0 && c->y + dy < H && !isOccupied(c->x-1,c->y,i)) { // check left lane change
                        drawCar(c,1);c->x--;drawCar(c,0);sw=1; // move left
                        snprintf(lc_msg,100,"Car %d (%s) bound has moved a lane LEFT",i,dir_names[c->dir]);lc_ticks=20; // log message and shows it shows it for 2 section 
                    } else if(c->x<W-1 && c->x+1 <= R_BORDER && c->y + dy >= 0 && c->y + dy < H && !isOccupied(c->x+1,c->y,i)) { // check right lane change
                        drawCar(c,1);c->x++;drawCar(c,0);sw=1; // move right
                        snprintf(lc_msg,100,"Car %d (%s) bound has moved a lane RIGHT",i,dir_names[c->dir]);lc_ticks=20; // log message
                    }
                } else { // e-w lane change logic 
                    int dx = (c->dir == EAST) ? 1 : -1; // direction increment for x
                    if(c->y>0 && c->y-1 >= T_BORDER && c->x + dx >= 0 && c->x + dx < W && !isOccupied(c->x,c->y-1,i)) { // check up lane change
                        drawCar(c,1);c->y--;drawCar(c,0);sw=1; // move up
                        snprintf(lc_msg,100,"Car %d (%s) bound has moved a lane UP",i,dir_names[c->dir]);lc_ticks=20; // log message
                    } else if(c->y<H-1 && c->y+1 <= B_BORDER && c->x + dx >= 0 && c->x + dx < W && !isOccupied(c->x,c->y+1,i)) { // check down lane change
                        drawCar(c,1);c->y++;drawCar(c,0);sw=1; // move down
                        snprintf(lc_msg,100,"Car %d (%s) bound has moved a lane DOWN",i,dir_names[c->dir]);lc_ticks=20; // log message
                    }
                }
                if(sw) continue; // skip forward move if lane changed
            }
            drawCar(c,0); // redraw car if not moving
            continue; // skip to next car
        }
        
        drawCar(c,1); // erase old position
        c->x=nx; // update x
        c->y=ny; // update y
        if(!c->crossed) c->crossed=hasCrossed(c); // check if crossed
        drawCar(c,0); // draw at new position
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
    printf("1. Custom Simulation\n2. Standard (60s)\n3. Lane-Change Simulation\n4. Exit\n\n");
    printf("-----------------------------------------------\n-----------------------------------------------\n  Choice (1-4): ");
    fflush(stdout);
}

void run(int dur) {
    printf(CLEAR);  // clears the scrn
    initGrid(); // builds the road layouts 
    initLights(); // setup the traffic lights 
    initCars(); // reset car slots 
    initPeds(); // reset pedestrian slots 
    drawGrid(); // display the roads 
    int f=dur*10,t=0,paused=0; // total frames(divide by 10 for 10fps),time counter, paused or not 
    while(f>0) { // if 60 seconds then the 
        if(KBHIT()) {           // checks if key pressed or not 
            char c=GETCH(); // gets the key pressed
            if(c=='p'||c=='P') { // is it p or P
                paused=!paused;
                POS(H+7,1); // moves the cursor 
                if(paused) printf(YEL"[PAUSED - Press 'p' to resume]                    "RST); // shows the message in yellow 
                else printf("                                                   ");
                fflush(stdout);
            }
        }
        
        if(!paused) {
            t++;
            f--;
            updateLight(&nsLight);  // check timer and update light states 
            updateLight(&ewLight); // check timer and update light states
            drawLights();
            if(f%10==0) spawnCar();  // spawn a car every second that is every 10 ticks 
            if(f%15==0) spawnPed(); // spawn a pedestrian every 1.5 seconds 
            updateCars(t); // move the cars on the grid 
            updatePeds(t); // show the pedestrians on the grid  
        }
        fflush(stdout); // forces the screen to update immediately without any delay 
        SLEEP(100); // pauses for 0.1 seconds for controlling frames per second 
    }
    printf(CLEAR GRN"Simulation completed!\n"RST"Press Enter...   :-)");
    getchar(); // clears the input buffer 
    getchar(); // second waits for the user 
}

int main() {
    srand(time(NULL));
    int ch,dur;
    double ns,ew;
    while(1) { // while 1 because we wna the menu to keep apppering continously until the user enters 4 to exit 
        menu();
        scanf("%d",&ch);
        if(ch==1) {
            printf("\nDuration (1-300): ");
            scanf("%d",&dur);
            if(dur<1) dur=1; // keeps the range valid 
            if(dur>300) dur=300;
            printf("N-S speed (cells/s, default=3 range: 3-10): ");
            scanf("%lf",&ns);
            printf("E-W speed (cells/s, default=3 range 3-10): "); // max speed of the car is 10 because the of the grid size 
            scanf("%lf",&ew);
            ns_interval=(ns>0)?(int)(10.0/ns+0.5):3; // using ternary operator to set speed intervals 
            ew_interval=(ew>0)?(int)(10.0/ew+0.5):3;
            if(ns_interval<1) ns_interval=1;
            if(ew_interval<1) ew_interval=1;
            printf("Lane-change? (0/1): ");
            scanf("%d",&lane_change);
            run(dur);
        } else if(ch==2) {
            ns_interval=ew_interval=3; // default speed 
            lane_change=0; // lane change disbled 
            run(60);
        } else if(ch==3) {
            printf("\nDuration (1-300): ");
            scanf("%d",&dur);
            if(dur<1) dur=1;
            if(dur>300) dur=300;
            printf("N-S speed (default=3.3): ");
            scanf("%lf",&ns);
            printf("E-W speed (default=3.3): ");
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
