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
#define IX 40
#define IY 12
#define SPAWN_INTERVAL 10
#define CAR_MOVE_INTERVAL_NS 3
#define CAR_MOVE_INTERVAL_EW 3
#define FRAME_RATE 10

int car_move_interval_ns = CAR_MOVE_INTERVAL_NS;
int car_move_interval_ew = CAR_MOVE_INTERVAL_EW;
int enable_lane_change = 0;

typedef enum { RED, YELLOW, GREEN } LightState;
typedef enum { NORTH, SOUTH, EAST, WEST } Direction;

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
} Car;

#define MAX_CARS 20
Car cars[MAX_CARS];
char grid[GRID_HEIGHT][GRID_WIDTH];

TrafficLight nsLight, ewLight;

void initGrid() {
    for (int i = 0; i < GRID_HEIGHT; i++)
        for (int j = 0; j < GRID_WIDTH; j++)
            grid[i][j] = ' ';

    for (int j = 0; j < GRID_WIDTH; j++)
        grid[IY - 1][j] = grid[IY][j] = grid[IY + 1][j] = grid[IY + 2][j] = '-';

    for (int i = 0; i < GRID_HEIGHT; i++)
        grid[i][IX - 1] = grid[i][IX] = grid[i][IX + 1] = grid[i][IX + 2] = '|';

    for (int i = IY - 1; i <= IY + 2; i++)
        for (int j = IX - 1; j <= IX + 2; j++)
            grid[i][j] = '+';
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
    ewLight = (TrafficLight){RED,   50, 50, 10, 50};
}

void updateLight(TrafficLight *t) {
    if (--t->timer > 0) return;
    switch (t->state) {
        case GREEN:  t->state = YELLOW; t->timer = t->y; break;
        case YELLOW: t->state = RED;    t->timer = t->r; break;
        case RED:    t->state = GREEN;  t->timer = t->g; break;
    }
}

void drawTrafficLights() {
    CURSOR_POS(GRID_HEIGHT + 2, 1);
    {
        double secs = nsLight.timer / 10.0;
        printf("N-S Light: ");
        printf(nsLight.state == RED ? COLOR_RED "RED   " COLOR_RESET :
               nsLight.state == YELLOW ? COLOR_YELLOW "YELLOW" COLOR_RESET :
               COLOR_GREEN "GREEN " COLOR_RESET);
        printf("  (change in %.1fs)", secs);
    }

    CURSOR_POS(GRID_HEIGHT + 3, 1);
    {
        double secs = ewLight.timer / 10.0;
        printf("E-W Light: ");
        printf(ewLight.state == RED ? COLOR_RED "RED   " COLOR_RESET :
               ewLight.state == YELLOW ? COLOR_YELLOW "YELLOW" COLOR_RESET :
               COLOR_GREEN "GREEN " COLOR_RESET);
        printf("  (change in %.1fs)", secs);
    }

    CURSOR_POS(GRID_HEIGHT + 4, 1);
    {
        double speed_ns = (double)FRAME_RATE / (double)car_move_interval_ns;
        double speed_ew = (double)FRAME_RATE / (double)car_move_interval_ew;
        printf("N-S speed: %.2f cells/s    E-W speed: %.2f cells/s", speed_ns, speed_ew);
    }
}

void initCars() {
    for (int i = 0; i < MAX_CARS; i++)
        cars[i].active = 0;
}

int countLaneCars(Direction dir) {
    int cnt = 0;
    for (int i = 0; i < MAX_CARS; i++)
        if (cars[i].active && cars[i].dir == dir)
            cnt++;
    return cnt;
}

int isOccupied(int x, int y, int ignore) {
    for (int i = 0; i < MAX_CARS; i++)
        if (i != ignore && cars[i].active && cars[i].x == x && cars[i].y == y)
            return 1;
    return 0;
}

int canMove(Car* c) {
    if (c->dir == NORTH || c->dir == SOUTH) {
        if (nsLight.state == RED)
            if ((c->dir == NORTH && c->y <= IY + 3 && c->y > IY - 2) ||
                (c->dir == SOUTH && c->y >= IY - 2 && c->y < IY + 3))
                return 0;
    } else {
        if (ewLight.state == RED)
            if ((c->dir == EAST && c->x <= IX + 3 && c->x > IX - 2) ||
                (c->dir == WEST && c->x >= IX - 2 && c->x < IX + 3))
                return 0;
    }
    return 1;
}

void drawCar(Car* c, int erase) {
    if (c->y < 0 || c->y >= GRID_HEIGHT || c->x < 0 || c->x >= GRID_WIDTH) return;
    CURSOR_POS(c->y + 1, c->x + 1);
    if (erase) putchar(grid[c->y][c->x]);
    else printf(COLOR_BLUE "%c" COLOR_RESET, c->symbol);
}

void spawnCar() {
    int slot = -1;
    for (int i = 0; i < MAX_CARS; i++)
        if (!cars[i].active) { slot = i; break; }
    if (slot == -1) return;

    Car* c = &cars[slot];
    c->active = 1;
    c->dir = rand() % 4;

    switch (c->dir) {
        case NORTH: c->x = IX;     c->y = GRID_HEIGHT - 2; c->symbol = '^'; break;
        case SOUTH: c->x = IX + 1; c->y = 1;               c->symbol = 'v'; break;
        case EAST:  c->x = 1;      c->y = IY;              c->symbol = '>'; break;
        case WEST:  c->x = GRID_WIDTH - 2; c->y = IY + 1;  c->symbol = '<'; break;
    }

    if (isOccupied(c->x, c->y, slot))
        c->active = 0;
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

        if (!canMove(c) || isOccupied(nx, ny, i)) {
            /* attempt lane change if enabled and lane is congested */
            if (enable_lane_change && countLaneCars(c->dir) > 5) {
                int swapped = 0;
                if (c->dir == NORTH || c->dir == SOUTH) {
                    /* try shift left then right */
                    int lx = c->x - 1;
                    int rx = c->x + 1;
                    if (lx >= 0 && !isOccupied(lx, c->y, i)) {
                        drawCar(c, 1);
                        c->x = lx;
                        drawCar(c, 0);
                        swapped = 1;
                    } else if (rx < GRID_WIDTH && !isOccupied(rx, c->y, i)) {
                        drawCar(c, 1);
                        c->x = rx;
                        drawCar(c, 0);
                        swapped = 1;
                    }
                } else {
                    /* E/W: try shift up then down */
                    int uy = c->y - 1;
                    int dy = c->y + 1;
                    if (uy >= 0 && !isOccupied(c->x, uy, i)) {
                        drawCar(c, 1);
                        c->y = uy;
                        drawCar(c, 0);
                        swapped = 1;
                    } else if (dy < GRID_HEIGHT && !isOccupied(c->x, dy, i)) {
                        drawCar(c, 1);
                        c->y = dy;
                        drawCar(c, 0);
                        swapped = 1;
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
        drawCar(c, 0);
    }
}

void displayMenu() {
    printf(CLEAR_SCREEN);
    printf(COLOR_CYAN);
    printf("||================ TRAFFIC SIMULATION ================||\n");
    printf(COLOR_RESET);
    printf("\n");
    printf("  Key Features: \n");
    printf("    1. Configurable spawn rate\n");
    printf("    2. Per-lane speed control (N-S / E-W)\n");
    printf("    3. Live countdown to next light change\n");
    printf("    4. On-screen per-lane speeds\n\n");
    printf(COLOR_GREEN "  MAIN MENU\n" COLOR_RESET);
    printf("  -----------------------------------------------\n\n");
    printf("    1. Start Custom Simulation (set durations & speeds)\n");
    printf("    2. Start Standard Simulation (60 seconds, defaults)\n");
    printf("    3. Start Simulation with Lane-Change enabled\n");
    printf("    4. Exit\n\n");
    printf("  -----------------------------------------------\n\n");
    printf("  Enter choice (1-4): ");
    fflush(stdout);
}

void runSimulation(int duration) {
    printf(CLEAR_SCREEN);
    initGrid();
    initTrafficLights();
    initCars();
    drawGrid();

    int frames = duration * 10;
    int tick = 0;

    while (frames--) {
        tick++;
        updateLight(&nsLight);
        updateLight(&ewLight);
        drawTrafficLights();

        if (frames % SPAWN_INTERVAL == 0)
            spawnCar();

        updateCars(tick);
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
