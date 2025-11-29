#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #include <windows.h>
    #define SLEEP(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define SLEEP(ms) usleep((ms) * 1000)
#endif

#define CLEAR_SCREEN "\033[2J"
#define CURSOR_HOME "\033[H"
#define CURSOR_POS(row, col) printf("\033[%d;%dH", (row), (col))
#define COLOR_RED "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_GREEN "\033[32m"
#define COLOR_RESET "\033[0m"
#define COLOR_BLUE "\033[34m"
#define COLOR_CYAN "\033[36m"
#define COLOR_MAGENTA "\033[35m"

#define GRID_WIDTH 80
#define GRID_HEIGHT 24
#define INTERSECTION_X 40
#define INTERSECTION_Y 12

typedef enum { RED, YELLOW, GREEN } LightState;

typedef struct {
    LightState state;
    int timer;
    int greenTime;
    int yellowTime;
    int redTime;
} TrafficLight;

typedef enum { NORTH, SOUTH, EAST, WEST } Direction;

typedef struct {
    int x, y;
    Direction dir;
    char symbol;
    int active;
} Car;

#define MAX_CARS 20
Car cars[MAX_CARS];
TrafficLight nsLight;
TrafficLight ewLight;
char grid[GRID_HEIGHT][GRID_WIDTH];

void initGrid();
void drawGrid();
void initTrafficLights();
void updateTrafficLights();
void drawTrafficLights();
void initCars();
void spawnCar();
void updateCars();
void drawCar(Car* car, int erase);
int isPositionOccupied(int x, int y, int carIndex);
int canMove(Car* car);
void displayMenu();
void runSimulation(int duration);

int main() {
    int choice;
    int duration;
    
    srand(time(NULL));
    
    while (1) {
        displayMenu();
        scanf("%d", &choice);
        
        switch (choice) {
            case 1:
                printf("\nEnter simulation duration in seconds (1-300): ");
                scanf("%d", &duration);
                if (duration < 1) duration = 1;
                if (duration > 300) duration = 300;
                runSimulation(duration);
                break;
            case 2:
                runSimulation(60);
                break;
            case 3:
                printf("\nThank you for using the Traffic Simulation!\n");
                return 0;
            default:
                printf("\nInvalid choice. Please try again.\n");
                SLEEP(2000);
                break;
        }
    }
    
    return 0;
}

void displayMenu() {
    printf(CLEAR_SCREEN);
    printf(CURSOR_HOME);
    printf(COLOR_CYAN);
    printf("||============================================================||n");
    printf("||                                                            ||\n");
    printf("||          TRAFFIC INTERSECTION SIMULATION SYSTEM            ||\n");
    printf("||                                                            ||\n");
    printf("||============================================================||\n");
    printf(COLOR_RESET);
    printf("\n");
    printf(COLOR_MAGENTA "  Features:\n" COLOR_RESET);
    printf("    1. Realistic traffic light timing\n");
    printf("    2.Multi-directional vehicle flow\n");
    printf("    3.Collision detection\n");
    printf("    4.Dynamic car spawning\n");
    printf("\n");
    printf(COLOR_GREEN "  MAIN MENU\n" COLOR_RESET);
    printf("  ------------------------------------------------------------------\n");
    printf("\n");
    printf("    1. Start Custom Simulation\n");
    printf("    2. Start Standard Simulation (60 seconds)\n");
    printf("    3. Exit Program\n");
    printf("\n");
    printf("  --------------------------------------------------------------------\n");
    printf("\n");
    printf("  Enter your choice (1-3): ");
    fflush(stdout);
}

void runSimulation(int duration) {
    printf(CLEAR_SCREEN);
    initGrid();
    initTrafficLights();
    initCars();
    drawGrid();
    
    int frame = 0;
    int maxFrames = duration * 10;
    printf("\n\nSimulation running for %d seconds...\n", duration);
    printf("(Program will return to menu after simulation)\n");
    fflush(stdout);
    
    while (frame < maxFrames) {
        updateTrafficLights();
        drawTrafficLights();
        
        if (frame % 30 == 0) {
            spawnCar();
        }
        
        updateCars();
        
        fflush(stdout);
        SLEEP(100);
        frame++;
    }
    
    printf(CLEAR_SCREEN);
    printf(CURSOR_HOME);
    printf(COLOR_GREEN "Simulation completed!\n" COLOR_RESET);
    printf("Press Enter to return to menu...");
    getchar();
    getchar();
}

void initGrid() {
    int i, j;
    
    for (i = 0; i < GRID_HEIGHT; i++) {
        for (j = 0; j < GRID_WIDTH; j++) {
            grid[i][j] = ' ';
        }
    }
    
    for (j = 0; j < GRID_WIDTH; j++) {
        grid[INTERSECTION_Y - 1][j] = '-';
        grid[INTERSECTION_Y][j] = '-';
        grid[INTERSECTION_Y + 1][j] = '-';
        grid[INTERSECTION_Y + 2][j] = '-';
    }
    
    for (i = 0; i < GRID_HEIGHT; i++) {
        grid[i][INTERSECTION_X - 1] = '|';
        grid[i][INTERSECTION_X] = '|';
        grid[i][INTERSECTION_X + 1] = '|';
        grid[i][INTERSECTION_X + 2] = '|';
    }
    
    for (i = INTERSECTION_Y - 1; i <= INTERSECTION_Y + 2; i++) {
        for (j = INTERSECTION_X - 1; j <= INTERSECTION_X + 2; j++) {
            grid[i][j] = '+';
        }
    }
}

void drawGrid() {
    int i, j;
    printf(CURSOR_HOME);
    for (i = 0; i < GRID_HEIGHT; i++) {
        for (j = 0; j < GRID_WIDTH; j++) {
            putchar(grid[i][j]);
        }
        putchar('\n');
    }
}

void initTrafficLights() {
    nsLight.state = GREEN;
    nsLight.timer = 50;
    nsLight.greenTime = 50;
    nsLight.yellowTime = 10;
    nsLight.redTime = 50;
    
    ewLight.state = RED;
    ewLight.timer = 50;
    ewLight.greenTime = 50;
    ewLight.yellowTime = 10;
    ewLight.redTime = 50;
}

void updateTrafficLights() {
    nsLight.timer--;
    if (nsLight.timer <= 0) {
        switch (nsLight.state) {
            case GREEN:
                nsLight.state = YELLOW;
                nsLight.timer = nsLight.yellowTime;
                break;
            case YELLOW:
                nsLight.state = RED;
                nsLight.timer = nsLight.redTime;
                break;
            case RED:
                nsLight.state = GREEN;
                nsLight.timer = nsLight.greenTime;
                break;
        }
    }
    
    ewLight.timer--;
    if (ewLight.timer <= 0) {
        switch (ewLight.state) {
            case GREEN:
                ewLight.state = YELLOW;
                ewLight.timer = ewLight.yellowTime;
                break;
            case YELLOW:
                ewLight.state = RED;
                ewLight.timer = ewLight.redTime;
                break;
            case RED:
                ewLight.state = GREEN;
                ewLight.timer = ewLight.greenTime;
                break;
        }
    }
}

void drawTrafficLights() {
    CURSOR_POS(GRID_HEIGHT + 2, 1);
    printf("N-S Light: ");
    switch (nsLight.state) {
        case RED:
            printf(COLOR_RED "RED   " COLOR_RESET);
            break;
        case YELLOW:
            printf(COLOR_YELLOW "YELLOW" COLOR_RESET);
            break;
        case GREEN:
            printf(COLOR_GREEN "GREEN " COLOR_RESET);
            break;
    }
    
    CURSOR_POS(GRID_HEIGHT + 3, 1);
    printf("E-W Light: ");
    switch (ewLight.state) {
        case RED:
            printf(COLOR_RED "RED   " COLOR_RESET);
            break;
        case YELLOW:
            printf(COLOR_YELLOW "YELLOW" COLOR_RESET);
            break;
        case GREEN:
            printf(COLOR_GREEN "GREEN " COLOR_RESET);
            break;
    }
}

void initCars() {
    int i;
    for (i = 0; i < MAX_CARS; i++) {
        cars[i].active = 0;
    }
}

void spawnCar() {
    int i;
    int slot = -1;
    Direction dir;
    Car* car;
    
    for (i = 0; i < MAX_CARS; i++) {
        if (!cars[i].active) {
            slot = i;
            break;
        }
    }
    if (slot == -1) return;
    
    dir = rand() % 4;
    car = &cars[slot];
    car->dir = dir;
    car->active = 1;
    
    switch (dir) {
        case NORTH:
            car->x = INTERSECTION_X;
            car->y = GRID_HEIGHT - 2;
            car->symbol = '^';
            break;
        case SOUTH:
            car->x = INTERSECTION_X + 1;
            car->y = 1;
            car->symbol = 'v';
            break;
        case EAST:
            car->x = 1;
            car->y = INTERSECTION_Y;
            car->symbol = '>';
            break;
        case WEST:
            car->x = GRID_WIDTH - 2;
            car->y = INTERSECTION_Y + 1;
            car->symbol = '<';
            break;
    }
    
    if (isPositionOccupied(car->x, car->y, slot)) {
        car->active = 0;
    }
}

int isPositionOccupied(int x, int y, int carIndex) {
    int i;
    for (i = 0; i < MAX_CARS; i++) {
        if (i != carIndex && cars[i].active && 
            cars[i].x == x && cars[i].y == y) {
            return 1;
        }
    }
    return 0;
}

int canMove(Car* car) {
    if (car->dir == NORTH || car->dir == SOUTH) {
        if (nsLight.state == RED) {
            if ((car->dir == NORTH && car->y <= INTERSECTION_Y + 3 && car->y > INTERSECTION_Y - 2) ||
                (car->dir == SOUTH && car->y >= INTERSECTION_Y - 2 && car->y < INTERSECTION_Y + 3)) {
                return 0;
            }
        }
    } else {
        if (ewLight.state == RED) {
            if ((car->dir == EAST && car->x <= INTERSECTION_X + 3 && car->x > INTERSECTION_X - 2) ||
                (car->dir == WEST && car->x >= INTERSECTION_X - 2 && car->x < INTERSECTION_X + 3)) {
                return 0;
            }
        }
    }
    
    return 1;
}

void updateCars() {
    int i;
    int nextX, nextY;
    Car* car;
    
    for (i = 0; i < MAX_CARS; i++) {
        if (!cars[i].active) continue;
        
        car = &cars[i];
        
        nextX = car->x;
        nextY = car->y;
        
        switch (car->dir) {
            case NORTH: nextY--; break;
            case SOUTH: nextY++; break;
            case EAST:  nextX++; break;
            case WEST:  nextX--; break;
        }
        
        if (nextX < 0 || nextX >= GRID_WIDTH || nextY < 0 || nextY >= GRID_HEIGHT) {
            drawCar(car, 1);
            car->active = 0;
            continue;
        }
        
        if (!canMove(car) || isPositionOccupied(nextX, nextY, i)) {
            drawCar(car, 0);
            continue;
        }
        
        drawCar(car, 1);
        
        car->x = nextX;
        car->y = nextY;
        
        drawCar(car, 0);
    }
}

void drawCar(Car* car, int erase) {
    if (car->y < 0 || car->y >= GRID_HEIGHT || car->x < 0 || car->x >= GRID_WIDTH) {
        return;
    }
    
    CURSOR_POS(car->y + 1, car->x + 1);
    if (erase) {
        putchar(grid[car->y][car->x]);
    } else {
        printf(COLOR_BLUE "%c" COLOR_RESET, car->symbol);
    }
}