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

// ANSI escape codes for terminal control
#define CLEAR_SCREEN "\033[2J"
#define CURSOR_HOME "\033[H"
#define CURSOR_POS(row, col) printf("\033[%d;%dH", (row), (col))
#define COLOR_RED "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_GREEN "\033[32m"
#define COLOR_RESET "\033[0m"
#define COLOR_BLUE "\033[34m"

// Grid dimensions
#define GRID_WIDTH 80
#define GRID_HEIGHT 24
#define INTERSECTION_X 40
#define INTERSECTION_Y 12

// Traffic light states
typedef enum { RED, YELLOW, GREEN } LightState;

// Traffic light structure
typedef struct {
    LightState state;
    int timer;
    int greenTime;
    int yellowTime;
    int redTime;
} TrafficLight;

// Car direction
typedef enum { NORTH, SOUTH, EAST, WEST } Direction;

// Car structure
typedef struct {
    int x, y;
    Direction dir;
    char symbol;
    int active;
} Car;

// Global variables
#define MAX_CARS 20
Car cars[MAX_CARS];
TrafficLight nsLight;  // North-South light
TrafficLight ewLight;  // East-West light
char grid[GRID_HEIGHT][GRID_WIDTH];

// Function prototypes
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

int main() {
    srand(time(NULL));
    
    // Clear screen and initialize
    printf(CLEAR_SCREEN);
    initGrid();
    initTrafficLights();
    initCars();
    drawGrid();
    
    int frame = 0;
    int maxFrames = 600;  // Run for ~60 seconds
    printf("\n\nSimulation running for 60 seconds...\n");
    printf("(Program will auto-exit after simulation)\n");
    fflush(stdout);
    
    // Main simulation loop
    while (frame < maxFrames) {
        // Update traffic lights
        updateTrafficLights();
        drawTrafficLights();
        
        // Spawn new cars occasionally
        if (frame % 30 == 0) {
            spawnCar();
        }
        
        // Update and draw cars
        updateCars();
        
        fflush(stdout);
        SLEEP(100);  // 100ms delay
        frame++;
    }
    
    // Cleanup
    printf(CLEAR_SCREEN);
    printf(CURSOR_HOME);
    printf("Traffic simulation ended. Goodbye!\n");
    
    return 0;
}

// Initialize the road grid
void initGrid() {
    int i, j;
    
    // Fill with spaces
    for (i = 0; i < GRID_HEIGHT; i++) {
        for (j = 0; j < GRID_WIDTH; j++) {
            grid[i][j] = ' ';
        }
    }
    
    // Draw horizontal road (East-West)
    for (j = 0; j < GRID_WIDTH; j++) {
        grid[INTERSECTION_Y - 1][j] = '-';
        grid[INTERSECTION_Y][j] = '-';
        grid[INTERSECTION_Y + 1][j] = '-';
        grid[INTERSECTION_Y + 2][j] = '-';
    }
    
    // Draw vertical road (North-South)
    for (i = 0; i < GRID_HEIGHT; i++) {
        grid[i][INTERSECTION_X - 1] = '|';
        grid[i][INTERSECTION_X] = '|';
        grid[i][INTERSECTION_X + 1] = '|';
        grid[i][INTERSECTION_X + 2] = '|';
    }
    
    // Mark intersection center
    for (i = INTERSECTION_Y - 1; i <= INTERSECTION_Y + 2; i++) {
        for (j = INTERSECTION_X - 1; j <= INTERSECTION_X + 2; j++) {
            grid[i][j] = '+';
        }
    }
}

// Draw the static grid once
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

// Initialize traffic lights
void initTrafficLights() {
    // North-South light starts green
    nsLight.state = GREEN;
    nsLight.timer = 50;
    nsLight.greenTime = 50;
    nsLight.yellowTime = 10;
    nsLight.redTime = 50;
    
    // East-West light starts red
    ewLight.state = RED;
    ewLight.timer = 50;
    ewLight.greenTime = 50;
    ewLight.yellowTime = 10;
    ewLight.redTime = 50;
}

// Update traffic light states
void updateTrafficLights() {
    // Update North-South light
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
    
    // Update East-West light
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

// Draw traffic lights on screen
void drawTrafficLights() {
    // North-South light display
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
    
    // East-West light display
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

// Initialize cars array
void initCars() {
    int i;
    for (i = 0; i < MAX_CARS; i++) {
        cars[i].active = 0;
    }
}

// Spawn a new car at a random lane entrance
void spawnCar() {
    int i;
    int slot = -1;
    Direction dir;
    Car* car;
    
    // Find an inactive car slot
    for (i = 0; i < MAX_CARS; i++) {
        if (!cars[i].active) {
            slot = i;
            break;
        }
    }
    if (slot == -1) return;  /* No available slots */
    
    // Choose random direction
    dir = rand() % 4;
    car = &cars[slot];
    car->dir = dir;
    car->active = 1;
    
    // Set starting position and symbol based on direction
    switch (dir) {
        case NORTH:  // Moving up
            car->x = INTERSECTION_X;
            car->y = GRID_HEIGHT - 2;
            car->symbol = '^';
            break;
        case SOUTH:  // Moving down
            car->x = INTERSECTION_X + 1;
            car->y = 1;
            car->symbol = 'v';
            break;
        case EAST:   // Moving right
            car->x = 1;
            car->y = INTERSECTION_Y;
            car->symbol = '>';
            break;
        case WEST:   // Moving left
            car->x = GRID_WIDTH - 2;
            car->y = INTERSECTION_Y + 1;
            car->symbol = '<';
            break;
    }
    
    // Don't spawn if position is occupied
    if (isPositionOccupied(car->x, car->y, slot)) {
        car->active = 0;
    }
}

// Check if a position is occupied by another car
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

// Check if a car can move (traffic light and no collision)
int canMove(Car* car) {
    // Check traffic light
    if (car->dir == NORTH || car->dir == SOUTH) {
        if (nsLight.state == RED) {
            // Stop before intersection
            if ((car->dir == NORTH && car->y <= INTERSECTION_Y + 3 && car->y > INTERSECTION_Y - 2) ||
                (car->dir == SOUTH && car->y >= INTERSECTION_Y - 2 && car->y < INTERSECTION_Y + 3)) {
                return 0;
            }
        }
    } else {
        if (ewLight.state == RED) {
            // Stop before intersection
            if ((car->dir == EAST && car->x <= INTERSECTION_X + 3 && car->x > INTERSECTION_X - 2) ||
                (car->dir == WEST && car->x >= INTERSECTION_X - 2 && car->x < INTERSECTION_X + 3)) {
                return 0;
            }
        }
    }
    
    return 1;
}

/* Update all active cars */
void updateCars() {
    int i;
    int nextX, nextY;
    Car* car;
    
    for (i = 0; i < MAX_CARS; i++) {
        if (!cars[i].active) continue;
        
        car = &cars[i];
        
        /* Calculate next position */
        nextX = car->x;
        nextY = car->y;
        
        switch (car->dir) {
            case NORTH: nextY--; break;
            case SOUTH: nextY++; break;
            case EAST:  nextX++; break;
            case WEST:  nextX--; break;
        }
        
        /* Check if car has left the grid */
        if (nextX < 0 || nextX >= GRID_WIDTH || nextY < 0 || nextY >= GRID_HEIGHT) {
            drawCar(car, 1);  /* Erase old position */
            car->active = 0;
            continue;
        }
        
        /* Check if can move (traffic light and collision) */
        if (!canMove(car) || isPositionOccupied(nextX, nextY, i)) {
            drawCar(car, 0);  /* Redraw at same position */
            continue;
        }
        
        /* Erase old position */
        drawCar(car, 1);
        
        /* Update position */
        car->x = nextX;
        car->y = nextY;
        
        /* Draw new position */
        drawCar(car, 0);
    }
}

/* Draw or erase a car */
void drawCar(Car* car, int erase) {
    if (car->y < 0 || car->y >= GRID_HEIGHT || car->x < 0 || car->x >= GRID_WIDTH) {
        return;
    }
    
    CURSOR_POS(car->y + 1, car->x + 1);
    if (erase) {
        /* Restore background character */
        putchar(grid[car->y][car->x]);
    } else {
        /* Draw car with color */
        printf(COLOR_BLUE "%c" COLOR_RESET, car->symbol);
    }
}