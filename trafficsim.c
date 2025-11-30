#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#ifdef _WIN32
    #include <windows.h>
    #define SLEEP(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define SLEEP(ms) usleep((ms) * 1000)
#endif

#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_RESET   "\033[0m"

#define GRID_WIDTH 100
#define GRID_HEIGHT 30
#define MAX_VEHICLES 30
#define MAX_INTERSECTIONS 4

typedef enum { RED, YELLOW, GREEN } LightState;
typedef enum { NORTH, SOUTH, EAST, WEST } Direction;
typedef enum { CAR, BUS, TRUCK, MOTORCYCLE } VehicleType;

typedef struct {
    VehicleType type;
    float length;
    float maxSpeed;
    float accel;
    float brake;
    char symbol;
    const char* color;
} VehicleSpec;

typedef struct {
    LightState state;
    int timer;
    int green, yellow, red;
} TrafficLight;

typedef struct {
    int x, y;
    TrafficLight lights[4];
} Intersection;

typedef struct {
    float x, y;
    float speed;
    Direction dir;
    VehicleSpec spec;
    int active;
    int passed;
} Vehicle;

char grid[GRID_HEIGHT][GRID_WIDTH];
Vehicle vehicles[MAX_VEHICLES];
Intersection intersections[MAX_INTERSECTIONS];
int numIntersections = 2;

VehicleSpec specs[4] = {
    {CAR, 3.0f, 1.8f, 0.4f, 2.0f, 'C', COLOR_BLUE},
    {BUS, 6.0f, 1.2f, 0.25f, 3.0f, 'B', COLOR_YELLOW},
    {TRUCK, 7.0f, 1.4f, 0.2f, 3.5f, 'T', COLOR_RED},
    {MOTORCYCLE, 2.0f, 2.2f, 0.6f, 1.5f, 'M', COLOR_MAGENTA}
};

void clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void initGrid() {
    for (int i = 0; i < GRID_HEIGHT; i++)
        for (int j = 0; j < GRID_WIDTH; j++)
            grid[i][j] = ' ';

    for (int k = 0; k < numIntersections; k++) {
        int cx = intersections[k].x;
        int cy = intersections[k].y;

        for (int i = 0; i < GRID_HEIGHT; i++) {
            if (i < cy - 2 || i > cy + 2) {
                grid[i][cx - 1] = '|';
                grid[i][cx] = ' ';
                grid[i][cx + 1] = '|';
            }
        }

        for (int j = 0; j < GRID_WIDTH; j++) {
            if (j < cx - 2 || j > cx + 2) {
                grid[cy - 1][j] = '-';
                grid[cy][j] = ' ';
                grid[cy + 1][j] = '-';
            }
        }

        for (int i = cy - 2; i <= cy + 2; i++)
            for (int j = cx - 2; j <= cx + 2; j++)
                grid[i][j] = '.';
    }
}

void drawGrid() {
    printf("\033[H");
    for (int i = 0; i < GRID_HEIGHT; i++) {
        for (int j = 0; j < GRID_WIDTH; j++)
            putchar(grid[i][j]);
        putchar('\n');
    }
}

void initIntersections() {
    intersections[0].x = 25;
    intersections[0].y = 15;
    intersections[1].x = 75;
    intersections[1].y = 15;
    intersections[2].x = 25;
    intersections[2].y = 25;
    intersections[3].x = 75;
    intersections[3].y = 25;

    for (int k = 0; k < numIntersections; k++) {
        for (int dir = 0; dir < 4; dir++) {
            intersections[k].lights[dir].green = 25 + rand() % 15;
            intersections[k].lights[dir].yellow = 4 + rand() % 3;
            intersections[k].lights[dir].red = 25 + rand() % 15;
            intersections[k].lights[dir].timer = (dir % 2 == 0) ? 
                intersections[k].lights[dir].green : intersections[k].lights[dir].red;
            intersections[k].lights[dir].state = (dir % 2 == 0) ? GREEN : RED;
        }
    }
}

void updateLight(TrafficLight *t) {
    if (--t->timer > 0) return;
    if (t->state == GREEN) { t->state = YELLOW; t->timer = t->yellow; }
    else if (t->state == YELLOW) { t->state = RED; t->timer = t->red; }
    else { t->state = GREEN; t->timer = t->green; }
}

void updateAllLights() {
    for (int k = 0; k < numIntersections; k++)
        for (int dir = 0; dir < 4; dir++)
            updateLight(&intersections[k].lights[dir]);
}

void drawTrafficLights() {
    printf(COLOR_BOLD "Lights: " COLOR_RESET);
    char dirLabels[4] = {'N', 'S', 'E', 'W'};
    for (int k = 0; k < numIntersections; k++) {
        printf("I%d[", k + 1);
        for (int dir = 0; dir < 4; dir++) {
            LightState s = intersections[k].lights[dir].state;
            printf("%c:", dirLabels[dir]);
            printf(s == RED ? COLOR_RED "R" : s == YELLOW ? COLOR_YELLOW "Y" : COLOR_GREEN "G");
            printf(COLOR_RESET);
            if (dir < 3) printf(" ");
        }
        printf("] ");
    }
    printf("\n");
}

void initVehicles() {
    for (int i = 0; i < MAX_VEHICLES; i++)
        vehicles[i].active = 0;
}

int checkCollision(int idx, float nx, float ny) {
    Vehicle* v1 = &vehicles[idx];
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (i == idx || !vehicles[i].active) continue;
        Vehicle* v2 = &vehicles[i];
        if (v1->dir != v2->dir) continue;

        float dx = fabs(nx - v2->x);
        float dy = fabs(ny - v2->y);
        float minDist = (v1->spec.length + v2->spec.length) / 2.0f + 1.5f;

        if ((v1->dir == NORTH || v1->dir == SOUTH) && dx < 1.5f && dy < minDist) return 1;
        if ((v1->dir == EAST || v1->dir == WEST) && dy < 1.5f && dx < minDist) return 1;
    }
    return 0;
}

int canMove(Vehicle* v) {
    if (v->passed) return 1;

    for (int k = 0; k < numIntersections; k++) {
        Intersection* in = &intersections[k];
        float stopDist = v->spec.brake + 4.0f;

        if (v->dir == NORTH && v->y > in->y + 3 && v->y - in->y < stopDist + 3) {
            if (in->lights[NORTH].state != GREEN) return 0;
        } else if (v->dir == SOUTH && v->y < in->y - 3 && in->y - v->y < stopDist + 3) {
            if (in->lights[SOUTH].state != GREEN) return 0;
        } else if (v->dir == EAST && v->x < in->x - 3 && in->x - v->x < stopDist + 3) {
            if (in->lights[EAST].state != GREEN) return 0;
        } else if (v->dir == WEST && v->x > in->x + 3 && v->x - in->x < stopDist + 3) {
            if (in->lights[WEST].state != GREEN) return 0;
        }

        if ((v->dir == NORTH && v->y < in->y - 3) ||
            (v->dir == SOUTH && v->y > in->y + 3) ||
            (v->dir == EAST && v->x > in->x + 3) ||
            (v->dir == WEST && v->x < in->x - 3))
            v->passed = 1;
    }
    return 1;
}

void drawVehicle(Vehicle* v) {
    int px = (int)(v->x + 0.5f);
    int py = (int)(v->y + 0.5f);
    if (py >= 0 && py < GRID_HEIGHT && px >= 0 && px < GRID_WIDTH) {
        printf("\033[%d;%dH%s%c%s", py + 1, px + 1, v->spec.color, v->spec.symbol, COLOR_RESET);
    }
}

void spawnVehicle() {
    int slot = -1;
    for (int i = 0; i < MAX_VEHICLES; i++)
        if (!vehicles[i].active) { slot = i; break; }
    if (slot == -1) return;

    Vehicle* v = &vehicles[slot];
    v->active = 1;
    v->spec = specs[rand() % 4];
    v->speed = v->spec.maxSpeed;
    v->dir = rand() % 4;
    v->passed = 0;

    int targetInter = rand() % numIntersections;
    int ix = intersections[targetInter].x;
    int iy = intersections[targetInter].y;

    if (v->dir == NORTH) { v->x = ix; v->y = GRID_HEIGHT - 2; }
    else if (v->dir == SOUTH) { v->x = ix; v->y = 1; }
    else if (v->dir == EAST) { v->x = 1; v->y = iy; }
    else { v->x = GRID_WIDTH - 2; v->y = iy; }

    if (checkCollision(slot, v->x, v->y))
        v->active = 0;
}

void updateVehicles() {
    for (int i = 0; i < MAX_VEHICLES; i++) {
        Vehicle* v = &vehicles[i];
        if (!v->active) continue;

        float nx = v->x, ny = v->y;
        
        if (!canMove(v))
            v->speed = fmax(0.3f, v->speed - v->spec.brake * 0.15f);
        else if (v->speed < v->spec.maxSpeed)
            v->speed = fmin(v->spec.maxSpeed, v->speed + v->spec.accel * 0.15f);

        if (v->dir == NORTH) ny -= v->speed;
        else if (v->dir == SOUTH) ny += v->speed;
        else if (v->dir == EAST) nx += v->speed;
        else nx -= v->speed;

        if (nx < 0 || nx >= GRID_WIDTH || ny < 0 || ny >= GRID_HEIGHT) {
            v->active = 0;
            continue;
        }

        if (checkCollision(i, nx, ny)) {
            v->speed = fmax(0.3f, v->speed * 0.6f);
            continue;
        }

        v->x = nx;
        v->y = ny;
    }
}

void drawAllVehicles() {
    for (int i = 0; i < MAX_VEHICLES; i++)
        if (vehicles[i].active)
            drawVehicle(&vehicles[i]);
}

void drawStats() {
    int active = 0, counts[4] = {0};
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (vehicles[i].active) {
            active++;
            counts[vehicles[i].spec.type]++;
        }
    }
    printf(COLOR_BOLD "Stats:" COLOR_RESET " Active:%d | Cars:%d Buses:%d Trucks:%d Bikes:%d\n",
           active, counts[CAR], counts[BUS], counts[TRUCK], counts[MOTORCYCLE]);
}

void displayMenu() {
    clearScreen();
    printf(COLOR_CYAN COLOR_BOLD);
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                                                            ║\n");
    printf("║      ADVANCED TRAFFIC INTERSECTION SIMULATION SYSTEM       ║\n");
    printf("║                                                            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET "\n");
    printf(COLOR_MAGENTA "Features:\n" COLOR_RESET);
    printf("  • Single-lane intersections with synchronized lights\n");
    printf("  • Multiple vehicle types (Car, Bus, Truck, Motorcycle)\n");
    printf("  • Realistic physics (acceleration, braking, collision)\n");
    printf("  • Multi-intersection support\n\n");
    printf(COLOR_GREEN COLOR_BOLD "MAIN MENU\n" COLOR_RESET);
    printf("──────────────────────────────────────────────────────────────\n\n");
    printf("  1. Custom Simulation\n");
    printf("  2. Standard Simulation (60s, 2 intersections)\n");
    printf("  3. Extended Simulation (120s, 4 intersections)\n");
    printf("  4. Exit\n\n");
    printf("──────────────────────────────────────────────────────────────\n\n");
    printf("Enter choice (1-4): ");
    fflush(stdout);
}

void runSimulation(int duration, int intersections) {
    numIntersections = intersections;
    clearScreen();
    printf("\033[2J");
    
    initGrid();
    initIntersections();
    initVehicles();

    int frames = duration * 10;
    int spawnRate = 10;

    while (frames-- > 0) {
        drawGrid();
        updateAllLights();
        drawTrafficLights();
        
        if (frames % spawnRate == 0)
            spawnVehicle();

        updateVehicles();
        drawAllVehicles();
        drawStats();

        fflush(stdout);
        SLEEP(100);
    }

    clearScreen();
    printf(COLOR_GREEN COLOR_BOLD "Simulation completed!\n" COLOR_RESET);
    printf("Press Enter to return to menu...");
    while(getchar() != '\n');
    getchar();
}

int main() {
    srand(time(NULL));
    int choice, duration, intersections;

    while (1) {
        displayMenu();
        if (scanf("%d", &choice) != 1) {
            while(getchar() != '\n');
            continue;
        }

        if (choice == 1) {
            printf("\nEnter duration (10-300s): ");
            scanf("%d", &duration);
            if (duration < 10) duration = 10;
            if (duration > 300) duration = 300;
            printf("Enter intersections (1-4): ");
            scanf("%d", &intersections);
            if (intersections < 1) intersections = 1;
            if (intersections > 4) intersections = 4;
            runSimulation(duration, intersections);
        }
        else if (choice == 2)
            runSimulation(60, 2);
        else if (choice == 3)
            runSimulation(120, 4);
        else if (choice == 4) {
            clearScreen();
            printf(COLOR_CYAN "Thanks for using Traffic Simulation!\n" COLOR_RESET);
            return 0;
        }
        else {
            printf("\nInvalid choice.\n");
            SLEEP(1000);
        }
    }
}