#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
    #include <windows.h>
    #define SLEEP(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define SLEEP(ms) usleep((ms) * 1000)
#endif

#define CLEAR_SCREEN "\033[2J\033[H"
#define CURSOR_POS(r,c) printf("\033[%d;%dH", (r), (c))

#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_RESET   "\033[0m"

#define GRID_WIDTH 80
#define GRID_HEIGHT 24
#define INTERSECTION_X 40
#define INTERSECTION_Y 12
#define SPAWN_INTERVAL 10
#define CAR_MOVE_INTERVAL_NS 3
#define CAR_MOVE_INTERVAL_EW 3
#define FRAME_RATE 10

/* Road boundaries */
#define NS_LANE_WIDTH 6
#define EW_LANE_WIDTH 4
#define LEFT_BORDER (INTERSECTION_X - (NS_LANE_WIDTH/2))
#define RIGHT_BORDER (INTERSECTION_X + (NS_LANE_WIDTH/2) - 1)
#define TOP_BORDER (INTERSECTION_Y - (EW_LANE_WIDTH/2))
#define BOTTOM_BORDER (INTERSECTION_Y + (EW_LANE_WIDTH/2) - 1)

/* Lane positions - adjusted to avoid center partition */
#define NS_EAST_LANE (LEFT_BORDER + 2)
#define NS_WEST_LANE (RIGHT_BORDER - 2)
#define EW_NORTH_LANE (TOP_BORDER + 1)
#define EW_SOUTH_LANE (BOTTOM_BORDER - 1)

/* Stop line positions */
#define STOP_LINE_DISTANCE 1

/* Old macro compatibility */
#define IX INTERSECTION_X
#define IY INTERSECTION_Y

int car_move_interval_ns = CAR_MOVE_INTERVAL_NS;
int car_move_interval_ew = CAR_MOVE_INTERVAL_EW;
int enable_lane_change = 0;

/* Define Direction and LightState enums first for use in structs */
typedef enum { RED, YELLOW, GREEN } LightState;
typedef enum { NORTH, SOUTH, EAST, WEST } Direction;

/* Pedestrian crossing state */
typedef enum { DONT_WALK, WALK } PedestrianSignal;

typedef struct {
    int x, y;
    Direction dir;
    char symbol;
    int active;
} Pedestrian;

#define MAX_PEDESTRIANS 10
#define PEDESTRIAN_SPAWN_INTERVAL 15  /* Spawn a pedestrian every 15 frames */
#define PEDESTRIAN_MOVE_INTERVAL 2    /* Pedestrians move every 2 frames */

Pedestrian pedestrians[MAX_PEDESTRIANS];
PedestrianSignal ns_ped_signal = DONT_WALK;   /* N-S pedestrian crossing signal */
PedestrianSignal ew_ped_signal = DONT_WALK;   /* E-W pedestrian crossing signal */
int ped_signal_timer = 0;                       /* Timer for pedestrian signal */

typedef struct {
    LightState state;
    int timer;
    int g, y, r;
} TrafficLight;

typedef struct {
    int x, y;
    Direction dir;
    char symbol;
    int active;
    int has_crossed;
} Car;

#define MAX_CARS 20
Car cars[MAX_CARS];
char grid[GRID_HEIGHT][GRID_WIDTH];
TrafficLight nsLight, ewLight;
char lane_change_msg[100] = "";
int lane_change_msg_ticks = 0;

void initGrid() {
    int i, j;
    
    /* Clear grid */
    for (i = 0; i < GRID_HEIGHT; i++)
        for (j = 0; j < GRID_WIDTH; j++)
            grid[i][j] = ' ';
    
    /* Draw horizontal borders (top and bottom) */
    for (j = 0; j < GRID_WIDTH; j++) {
        grid[TOP_BORDER][j] = '-';
        grid[BOTTOM_BORDER][j] = '-';
    }
    
    /* Draw vertical borders (left and right) */
    for (i = 0; i < GRID_HEIGHT; i++) {
        grid[i][LEFT_BORDER] = '|';
        grid[i][RIGHT_BORDER] = '|';
    }
    
    /* Draw corners */
    grid[TOP_BORDER][LEFT_BORDER] = '+';
    grid[TOP_BORDER][RIGHT_BORDER] = '+';
    grid[BOTTOM_BORDER][LEFT_BORDER] = '+';
    grid[BOTTOM_BORDER][RIGHT_BORDER] = '+';
    
    /* Draw E-W center partition line (divides north and south lanes) */
    int center_y = INTERSECTION_Y;
    for (j = LEFT_BORDER; j <= RIGHT_BORDER; j++) {
        if (grid[center_y][j] == ' ') {
            grid[center_y][j] = '-';
        }
    }
    
    /* Draw zebra crossing lines for pedestrians */
    /* Horizontal zebra line (for N-S pedestrian crossing) - above intersection */
    int ped_cross_ns_y = TOP_BORDER - 2;
    for (j = LEFT_BORDER; j <= RIGHT_BORDER; j++) {
        if (grid[ped_cross_ns_y][j] == ' ') {
            grid[ped_cross_ns_y][j] = (j % 2 == 0) ? ':' : ' ';
        }
    }
    
    /* Horizontal zebra line (for N-S pedestrian crossing) - below intersection */
    ped_cross_ns_y = BOTTOM_BORDER + 2;
    for (j = LEFT_BORDER; j <= RIGHT_BORDER; j++) {
        if (grid[ped_cross_ns_y][j] == ' ') {
            grid[ped_cross_ns_y][j] = (j % 2 == 0) ? ':' : ' ';
        }
    }
    
    /* Vertical zebra line (for E-W pedestrian crossing) - left of intersection */
    int ped_cross_ew_x = LEFT_BORDER - 2;
    for (i = TOP_BORDER; i <= BOTTOM_BORDER; i++) {
        if (grid[i][ped_cross_ew_x] == ' ') {
            grid[i][ped_cross_ew_x] = (i % 2 == 0) ? ':' : ' ';
        }
    }
    
    /* Vertical zebra line (for E-W pedestrian crossing) - right of intersection */
    ped_cross_ew_x = RIGHT_BORDER + 2;
    for (i = TOP_BORDER; i <= BOTTOM_BORDER; i++) {
        if (grid[i][ped_cross_ew_x] == ' ') {
            grid[i][ped_cross_ew_x] = (i % 2 == 0) ? ':' : ' ';
        }
    }
}

void drawGrid() {
    CURSOR_POS(1,1);
    for (int i = 0; i < GRID_HEIGHT; i++) {
        fwrite(grid[i], 1, GRID_WIDTH, stdout);
        putchar('\n');
    }
}

void initTrafficLights() {
    nsLight = (TrafficLight){GREEN, 50, 50, 10, 50};
    ewLight = (TrafficLight){RED, 50, 50, 10, 50};
}

void updateLight(TrafficLight *t) {
    if (--t->timer > 0) return;
    switch (t->state) {
        case GREEN:  t->state = YELLOW; t->timer = t->y; break;
        case YELLOW: t->state = RED;    t->timer = t->r; break;
        case RED:    t->state = GREEN;  t->timer = t->g; break;
    }
}

int countLaneCars(Direction dir) {
    int cnt = 0;
    for (int i = 0; i < MAX_CARS; i++)
        if (cars[i].active && cars[i].dir == dir)
            cnt++;
    return cnt;
}

void adaptiveControl() {
    int ns_density = countLaneCars(NORTH) + countLaneCars(SOUTH);
    int ew_density = countLaneCars(EAST) + countLaneCars(WEST);
    int base_g = 50;
    int max_extension = 30;
    int min_green = 20;
    
    /* Only adjust green times when transitioning to GREEN state */
    if (nsLight.state == GREEN && nsLight.timer == nsLight.g) {
        if (ns_density > ew_density + 3) {
            nsLight.g = base_g + max_extension;
            ewLight.g = base_g;
        }
        else if (ew_density > ns_density + 3) {
            ewLight.g = base_g + max_extension;
            nsLight.g = base_g;
        }
        else {
            nsLight.g = base_g;
            ewLight.g = base_g;
        }
        if (nsLight.g < min_green) nsLight.g = min_green;
        if (ewLight.g < min_green) ewLight.g = min_green;
    }
}

void updatePedestrianSignals() {
    /* Pedestrians get WALK signal when opposite direction has RED light */
    if (nsLight.state == RED) {
        ns_ped_signal = WALK;
    } else {
        ns_ped_signal = DONT_WALK;
    }
    
    if (ewLight.state == RED) {
        ew_ped_signal = WALK;
    } else {
        ew_ped_signal = DONT_WALK;
    }
}

void drawTrafficLights() {
    adaptiveControl();
    updatePedestrianSignals();
    
    int ns_density = countLaneCars(NORTH) + countLaneCars(SOUTH);
    int ew_density = countLaneCars(EAST) + countLaneCars(WEST);
    
    CURSOR_POS(GRID_HEIGHT + 2, 1);
    double secs = nsLight.timer / 10.0;
    printf("N-S Light: ");
    printf(nsLight.state == RED ? COLOR_RED "RED   " COLOR_RESET :
           nsLight.state == YELLOW ? COLOR_YELLOW "YELLOW" COLOR_RESET :
           COLOR_GREEN "GREEN " COLOR_RESET);
    printf("  (change in %.1fs)   Density: %d/%d          ", secs, ns_density, MAX_CARS);

    CURSOR_POS(GRID_HEIGHT + 3, 1);
    secs = ewLight.timer / 10.0;
    printf("E-W Light: ");
    printf(ewLight.state == RED ? COLOR_RED "RED   " COLOR_RESET :
           ewLight.state == YELLOW ? COLOR_YELLOW "YELLOW" COLOR_RESET :
           COLOR_GREEN "GREEN " COLOR_RESET);
    printf("  (change in %.1fs)   Density: %d/%d          ", secs, ew_density, MAX_CARS);

    CURSOR_POS(GRID_HEIGHT + 4, 1);
    double speed_ns = (double)FRAME_RATE / (double)car_move_interval_ns;
    double speed_ew = (double)FRAME_RATE / (double)car_move_interval_ew;
    printf("Speed - N-S: %.2f cells/s  |  E-W: %.2f cells/s                          ", speed_ns, speed_ew);
    
    /* Display pedestrian signals */
    CURSOR_POS(GRID_HEIGHT + 5, 1);
    printf("N-S Ped: ");
    printf(ns_ped_signal == WALK ? COLOR_GREEN "WALK   " COLOR_RESET : COLOR_RED "DON'T WALK" COLOR_RESET);
    printf("  |  E-W Ped: ");
    printf(ew_ped_signal == WALK ? COLOR_GREEN "WALK   " COLOR_RESET : COLOR_RED "DON'T WALK" COLOR_RESET);
    printf("                  ");
    
    /* Display lane-change message if active */
    if (lane_change_msg_ticks > 0) {
        CURSOR_POS(GRID_HEIGHT + 6, 1);
        printf("[LANE CHANGE] %s                                      ", lane_change_msg);
        lane_change_msg_ticks--;
    } else {
        CURSOR_POS(GRID_HEIGHT + 6, 1);
        printf("                                                                        ");
    }
}

void initCars() {
    for (int i = 0; i < MAX_CARS; i++) {
        cars[i].active = 0;
        cars[i].has_crossed = 0;
    }
}

void initPedestrians() {
    for (int i = 0; i < MAX_PEDESTRIANS; i++) {
        pedestrians[i].active = 0;
    }
    ns_ped_signal = DONT_WALK;
    ew_ped_signal = DONT_WALK;
    ped_signal_timer = 0;
}

int isValidPosition(int x, int y) {
    return (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT);
}

int isInIntersection(int x, int y) {
    return (x >= LEFT_BORDER && x <= RIGHT_BORDER &&
            y >= TOP_BORDER && y <= BOTTOM_BORDER);
}

int isAtStopLine(Car *c) {
    if (!c || !c->active) return 0;
    if (c->dir == NORTH) return (c->y == BOTTOM_BORDER + STOP_LINE_DISTANCE);
    if (c->dir == SOUTH) return (c->y == TOP_BORDER - STOP_LINE_DISTANCE);
    if (c->dir == EAST)  return (c->x == LEFT_BORDER - STOP_LINE_DISTANCE);
    if (c->dir == WEST)  return (c->x == RIGHT_BORDER + STOP_LINE_DISTANCE);
    return 0;
}

int hasCrossedIntersection(Car *c) {
    if (!c) return 0;
    if (c->dir == NORTH) return (c->y < TOP_BORDER);
    if (c->dir == SOUTH) return (c->y > BOTTOM_BORDER);
    if (c->dir == EAST)  return (c->x > RIGHT_BORDER);
    if (c->dir == WEST)  return (c->x < LEFT_BORDER);
    return 0;
}

int canMove(Car* c) {
    if (!c || !c->active) return 1;
    
    /* If car has crossed intersection, always allow movement */
    if (c->has_crossed) return 1;
    
    /* If car is in intersection, allow movement */
    if (isInIntersection(c->x, c->y)) return 1;
    
    /* Check if car is at stop line */
    if (isAtStopLine(c)) {
        if (c->dir == NORTH || c->dir == SOUTH) {
            return (nsLight.state != RED);
        } else {
            return (ewLight.state != RED);
        }
    }
    
    return 1;
}

int isOccupied(int x, int y, int ignore) {
    for (int i = 0; i < MAX_CARS; i++) {
        if (i == ignore || !cars[i].active) continue;
        if (cars[i].x == x && cars[i].y == y) return 1;
    }
    return 0;
}

int isPedestrianAtPosition(int x, int y) {
    for (int i = 0; i < MAX_PEDESTRIANS; i++) {
        if (!pedestrians[i].active) continue;
        if (pedestrians[i].x == x && pedestrians[i].y == y) return 1;
    }
    return 0;
}

int isPedestrianInIntersection(Pedestrian *p) {
    if (!p || !p->active) return 0;
    if (p->dir == NORTH || p->dir == SOUTH) {
        return (p->x >= LEFT_BORDER && p->x <= RIGHT_BORDER &&
                p->y >= TOP_BORDER - 3 && p->y <= BOTTOM_BORDER + 3);
    } else {
        return (p->x >= LEFT_BORDER - 3 && p->x <= RIGHT_BORDER + 3 &&
                p->y >= TOP_BORDER && p->y <= BOTTOM_BORDER);
    }
}

void drawCar(Car* c, int erase) {
    if (!c || c->y < 0 || c->y >= GRID_HEIGHT || c->x < 0 || c->x >= GRID_WIDTH) return;
    CURSOR_POS(c->y + 1, c->x + 1);
    if (erase) {
        putchar(grid[c->y][c->x]);
    } else {
        if (isAtStopLine(c)) {
            printf(COLOR_YELLOW "%c" COLOR_RESET, c->symbol);
        } else if (isInIntersection(c->x, c->y)) {
            printf(COLOR_GREEN "%c" COLOR_RESET, c->symbol);
        } else if (c->has_crossed) {
            printf(COLOR_CYAN "%c" COLOR_RESET, c->symbol);
        } else {
            printf(COLOR_BLUE "%c" COLOR_RESET, c->symbol);
        }
    }
}

void drawPedestrian(Pedestrian* p, int erase) {
    if (!p || p->y < 0 || p->y >= GRID_HEIGHT || p->x < 0 || p->x >= GRID_WIDTH) return;
    CURSOR_POS(p->y + 1, p->x + 1);
    if (erase) {
        putchar(grid[p->y][p->x]);
    } else {
        printf(COLOR_CYAN "%c" COLOR_RESET, p->symbol);
    }
}

void spawnCar() {
    int slot = -1;
    for (int i = 0; i < MAX_CARS; i++)
        if (!cars[i].active) { slot = i; break; }
    if (slot == -1) return;
    
    Car* c = &cars[slot];
    c->active = 1;
    c->has_crossed = 0;
    c->dir = rand() % 4;
    
    switch (c->dir) {
        case NORTH: c->x = NS_EAST_LANE;     c->y = GRID_HEIGHT - 2; c->symbol = '^'; break;
        case SOUTH: c->x = NS_WEST_LANE;     c->y = 1;               c->symbol = 'v'; break;
        case EAST:  c->x = 1;                c->y = EW_SOUTH_LANE;   c->symbol = '>'; break;
        case WEST:  c->x = GRID_WIDTH - 2;   c->y = EW_NORTH_LANE;   c->symbol = '<'; break;
    }
    
    if (isOccupied(c->x, c->y, slot))
        c->active = 0;
}

void spawnPedestrian() {
    int slot = -1;
    for (int i = 0; i < MAX_PEDESTRIANS; i++)
        if (!pedestrians[i].active) { slot = i; break; }
    if (slot == -1) return;
    
    Pedestrian* p = &pedestrians[slot];
    p->active = 1;
    int ped_dir = rand() % 4;
    p->dir = ped_dir;
    
    /* Spawn pedestrians at crossing edges */
    switch (ped_dir) {
        case NORTH:  /* Crossing N-S, moving south */
            p->x = LEFT_BORDER + (rand() % (NS_LANE_WIDTH));
            p->y = TOP_BORDER - 3;
            p->symbol = 'P';
            break;
        case SOUTH:  /* Crossing N-S, moving north */
            p->x = LEFT_BORDER + (rand() % (NS_LANE_WIDTH));
            p->y = BOTTOM_BORDER + 3;
            p->symbol = 'P';
            break;
        case EAST:   /* Crossing E-W, moving west */
            p->x = RIGHT_BORDER + 3;
            p->y = TOP_BORDER + (rand() % (EW_LANE_WIDTH));
            p->symbol = 'P';
            break;
        case WEST:   /* Crossing E-W, moving east */
            p->x = LEFT_BORDER - 3;
            p->y = TOP_BORDER + (rand() % (EW_LANE_WIDTH));
            p->symbol = 'P';
            break;
    }
}

void updatePedestrians(int tick) {
    for (int i = 0; i < MAX_PEDESTRIANS; i++) {
        Pedestrian* p = &pedestrians[i];
        if (!p->active) continue;
        
        /* Only move pedestrians during WALK signal */
        int can_move = 0;
        if ((p->dir == NORTH || p->dir == SOUTH) && ns_ped_signal == WALK) {
            can_move = 1;
        } else if ((p->dir == EAST || p->dir == WEST) && ew_ped_signal == WALK) {
            can_move = 1;
        }
        
        if (can_move && (tick % PEDESTRIAN_MOVE_INTERVAL) == 0) {
            drawPedestrian(p, 1);
            if (p->dir == NORTH) p->y--;
            else if (p->dir == SOUTH) p->y++;
            else if (p->dir == EAST) p->x++;
            else if (p->dir == WEST) p->x--;
            
            /* Deactivate if off-grid */
            if (p->x < 0 || p->x >= GRID_WIDTH || p->y < 0 || p->y >= GRID_HEIGHT) {
                p->active = 0;
            } else {
                drawPedestrian(p, 0);
            }
        } else if (p->active) {
            drawPedestrian(p, 0);
        }
    }
}

void updateCars(int tick) {
    for (int i = 0; i < MAX_CARS; i++) {
        Car* c = &cars[i];
        if (!c->active) continue;
        
        int interval = (c->dir == NORTH || c->dir == SOUTH) ? car_move_interval_ns : car_move_interval_ew;
        if (interval > 1 && (tick % interval) != 0) {
            drawCar(c, 0);
            continue;
        }
        
        int nx = c->x, ny = c->y;
        if (c->dir == NORTH) ny--;
        if (c->dir == SOUTH) ny++;
        if (c->dir == EAST)  nx++;
        if (c->dir == WEST)  nx--;
        
        if (nx < 0 || nx >= GRID_WIDTH || ny < 0 || ny >= GRID_HEIGHT) {
            drawCar(c, 1);
            c->active = 0;
            continue;
        }
        
        if (!canMove(c) || isOccupied(nx, ny, i) || isPedestrianAtPosition(nx, ny)) {
            if (enable_lane_change && countLaneCars(c->dir) > 5) {
                int swapped = 0;
                if (c->dir == NORTH || c->dir == SOUTH) {
                    int lx = c->x - 1;
                    int rx = c->x + 1;
                    if (lx >= 0 && !isOccupied(lx, c->y, i)) { 
                        drawCar(c, 1); c->x = lx; drawCar(c, 0); swapped = 1; 
                        snprintf(lane_change_msg, sizeof(lane_change_msg), "Car %d changed lane LEFT (N-S)", i); 
                        lane_change_msg_ticks = 20; 
                    }
                    else if (rx < GRID_WIDTH && !isOccupied(rx, c->y, i)) { 
                        drawCar(c, 1); c->x = rx; drawCar(c, 0); swapped = 1; 
                        snprintf(lane_change_msg, sizeof(lane_change_msg), "Car %d changed lane RIGHT (N-S)", i); 
                        lane_change_msg_ticks = 20; 
                    }
                } else {
                    int uy = c->y - 1;
                    int dy = c->y + 1;
                    if (uy >= 0 && !isOccupied(c->x, uy, i)) { 
                        drawCar(c, 1); c->y = uy; drawCar(c, 0); swapped = 1; 
                        snprintf(lane_change_msg, sizeof(lane_change_msg), "Car %d changed lane UP (E-W)", i); 
                        lane_change_msg_ticks = 20; 
                    }
                    else if (dy < GRID_HEIGHT && !isOccupied(c->x, dy, i)) { 
                        drawCar(c, 1); c->y = dy; drawCar(c, 0); swapped = 1; 
                        snprintf(lane_change_msg, sizeof(lane_change_msg), "Car %d changed lane DOWN (E-W)", i); 
                        lane_change_msg_ticks = 20; 
                    }
                }
                if (swapped) continue;
            }
            drawCar(c, 0);
            continue;
        }
        
        drawCar(c, 1);
        c->x = nx;
        c->y = ny;
        
        if (!c->has_crossed) {
            c->has_crossed = hasCrossedIntersection(c);
        }
        
        drawCar(c, 0);
    }
}

void displayMenu() {
    printf(CLEAR_SCREEN);
    printf(COLOR_CYAN);
    printf("||============================================================||\n");
    printf("||          TRAFFIC INTERSECTION SIMULATION SYSTEM            ||\n");
    printf("||============================================================||\n");
    printf(COLOR_RESET "\n");
    printf(COLOR_MAGENTA "  Features:\n" COLOR_RESET);
    printf("    1. Adaptive traffic light control\n2. Per-lane speed control (N-S / E-W)\n3. Live countdown to next light change\n4. Lane-change when congested (>5 cars)\n5. Pedestrian crossing with WALK/DON'T WALK signals\n6. Zebra crossing lines\n\n");
    printf(COLOR_GREEN "  MAIN MENU\n" COLOR_RESET);
    printf("  -----------------------------------------------\n\n");
    printf("    1. Start Custom Simulation (set durations & speeds)\n2. Start Standard Simulation (60 seconds, defaults)\n3. Start Simulation with Lane-Change enabled\n4. Exit\n\n");
    printf("  -----------------------------------------------\n\n");
    printf("  Enter choice (1-4): ");
    fflush(stdout);
}

void runSimulation(int duration) {
    printf(CLEAR_SCREEN);
    initGrid();
    initTrafficLights();
    initCars();
    initPedestrians();
    drawGrid();
    int frames = duration * 10;
    int tick = 0;
    while (frames--) {
        tick++;
        updateLight(&nsLight);
        updateLight(&ewLight);
        adaptiveControl();
        drawTrafficLights();
        if (frames % SPAWN_INTERVAL == 0)
            spawnCar();
        if (frames % PEDESTRIAN_SPAWN_INTERVAL == 0)
            spawnPedestrian();
        updateCars(tick);
        updatePedestrians(tick);
        fflush(stdout);
        SLEEP(100);
    }
    printf(CLEAR_SCREEN);
    printf(COLOR_GREEN "Simulation completed!\n" COLOR_RESET);
    printf("Press Enter to return to menu...");
    getchar();
    getchar();
}

int main() {
    srand(time(NULL));
    int choice, duration;
    while (1) {
        displayMenu();
        scanf("%d", &choice);
        if (choice == 1) {
            double ns_speed, ew_speed;
            int lane_opt;
            printf("\nEnter simulation duration in seconds (1-300): ");
            scanf("%d", &duration);
            if (duration < 1) duration = 1;
            if (duration > 300) duration = 300;
            printf("Enter N-S speed (cells/sec, e.g. 1.0): ");
            scanf("%lf", &ns_speed);
            if (ns_speed <= 0) ns_speed = (double)FRAME_RATE / (double)CAR_MOVE_INTERVAL_NS;
            printf("Enter E-W speed (cells/sec, e.g. 1.0): ");
            scanf("%lf", &ew_speed);
            if (ew_speed <= 0) ew_speed = (double)FRAME_RATE / (double)CAR_MOVE_INTERVAL_EW;
            car_move_interval_ns = (int)( (FRAME_RATE / ns_speed) + 0.5 );
            if (car_move_interval_ns < 1) car_move_interval_ns = 1;
            car_move_interval_ew = (int)( (FRAME_RATE / ew_speed) + 0.5 );
            if (car_move_interval_ew < 1) car_move_interval_ew = 1;
            printf("Enable lane-change when congested? (0 = no, 1 = yes): ");
            scanf("%d", &lane_opt);
            enable_lane_change = lane_opt ? 1 : 0;
            runSimulation(duration);
        }
        else if (choice == 2) {
            car_move_interval_ns = CAR_MOVE_INTERVAL_NS;
            car_move_interval_ew = CAR_MOVE_INTERVAL_EW;
            enable_lane_change = 0;
            runSimulation(60);
        }
        else if (choice == 3) {
            double ns_speed, ew_speed;
            printf("\nEnter simulation duration in seconds (1-300): ");
            scanf("%d", &duration);
            if (duration < 1) duration = 1;
            if (duration > 300) duration = 300;
            printf("Enter N-S speed (cells/sec, or 0 for default): ");
            scanf("%lf", &ns_speed);
            printf("Enter E-W speed (cells/sec, or 0 for default): ");
            scanf("%lf", &ew_speed);
            if (ns_speed > 0) {
                car_move_interval_ns = (int)( (FRAME_RATE / ns_speed) + 0.5 );
                if (car_move_interval_ns < 1) car_move_interval_ns = 1;
            } else car_move_interval_ns = CAR_MOVE_INTERVAL_NS;
            if (ew_speed > 0) {
                car_move_interval_ew = (int)( (FRAME_RATE / ew_speed) + 0.5 );
                if (car_move_interval_ew < 1) car_move_interval_ew = 1;
            } else car_move_interval_ew = CAR_MOVE_INTERVAL_EW;
            enable_lane_change = 1;
            runSimulation(duration);
        }
        else if (choice == 4)
            return 0;
        else {
            printf("\nInvalid choice.\n");
            SLEEP(1500);
        }
    }
}
