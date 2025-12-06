#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#ifdef _WIN32
    #include <windows.h>
    #define SLEEP(ms) Sleep(ms)
    #define CLEAR_SCREEN() system("cls")
#else
    #include <unistd.h>
    #define SLEEP(ms) usleep((ms) * 1000)
    #define CLEAR_SCREEN() printf("\033[2J\033[H")
#endif

// ============ CONSTANTS ============
#define GRID_WIDTH 80
#define GRID_HEIGHT 24
#define INTERSECTION_X 40
#define INTERSECTION_Y 12
#define MAX_CARS 50
#define FRAMES_PER_SECOND 10
#define MS_PER_FRAME 100
#define SPAWN_INTERVAL 30  // frames (3 seconds)
#define MIN_SIM_TIME 1
#define MAX_SIM_TIME 300

// ============ ANSI COLOR CODES ============
typedef enum {
    COLOR_BLACK = 30,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_WHITE
} ColorCode;

#define COLOR(code) printf("\033[%dm", (code))
#define COLOR_RESET() printf("\033[0m")

// ============ ENUMS ============
typedef enum { 
    DIR_NORTH, 
    DIR_SOUTH, 
    DIR_EAST, 
    DIR_WEST 
} Direction;

typedef enum { 
    LIGHT_RED, 
    LIGHT_YELLOW, 
    LIGHT_GREEN 
} LightState;

// ============ STRUCTURES ============
typedef struct {
    LightState state;
    int timer;
    struct {
        int green;
        int yellow;
        int red;
    } durations;
} TrafficLight;

typedef struct {
    int x, y;
    Direction direction;
    char symbol;
    bool active;
    int id;
} Car;

typedef struct {
    char grid[GRID_HEIGHT][GRID_WIDTH];
    TrafficLight nsLight;  // North-South
    TrafficLight ewLight;  // East-West
    Car cars[MAX_CARS];
    int carCount;
    int activeCarCount;
    int frameCount;
} Simulation;

// ============ FUNCTION PROTOTYPES ============
void setColor(ColorCode color);
void initSimulation(Simulation *sim);
void drawRoads(Simulation *sim);
void drawIntersection(Simulation *sim);
void displayGrid(const Simulation *sim);
void updateTrafficLight(TrafficLight *light);
void displayTrafficLights(const Simulation *sim);
bool isValidPosition(int x, int y);
bool isPositionOccupied(const Simulation *sim, int x, int y, int excludeId);
bool canCarMove(const Car *car, const Simulation *sim);
void drawCar(const Car *car, bool erase, const Simulation *sim);
bool spawnCar(Simulation *sim);
void updateCars(Simulation *sim);
void displayMenu(void);
int getIntegerInput(const char *prompt, int min, int max);
void runSimulation(Simulation *sim, int durationSeconds);
void cleanupSimulation(Simulation *sim);

// ============ MAIN FUNCTION ============
int main() {
    srand((unsigned int)time(NULL));
    
    Simulation sim;
    int choice;
    
    while (true) {
        displayMenu();
        choice = getIntegerInput("Enter your choice (1-3): ", 1, 3);
        
        switch (choice) {
            case 1: {  // Custom simulation
                int duration = getIntegerInput(
                    "Enter simulation duration in seconds (1-300): ", 
                    MIN_SIM_TIME, 
                    MAX_SIM_TIME
                );
                initSimulation(&sim);
                runSimulation(&sim, duration);
                break;
            }
            case 2: {  // Standard simulation
                initSimulation(&sim);
                runSimulation(&sim, 60);
                break;
            }
            case 3:  // Exit
                CLEAR_SCREEN();
                setColor(COLOR_CYAN);
                printf("Thank you for using Traffic Simulation!\n");
                COLOR_RESET();
                return 0;
                
            default:
                printf("Unexpected error.\n");
                SLEEP(1000);
        }
    }
    
    return 0;
}

// ============ IMPLEMENTATIONS ============

void setColor(ColorCode color) {
    COLOR(color);
}

void initSimulation(Simulation *sim) {
    if (!sim) return;
    
    // Initialize grid with spaces
    memset(sim->grid, ' ', sizeof(sim->grid));
    
    // Initialize traffic lights
    sim->nsLight.state = LIGHT_GREEN;
    sim->nsLight.timer = 50;
    sim->nsLight.durations = (struct {int green; int yellow; int red;}){50, 10, 50};
    
    sim->ewLight.state = LIGHT_RED;
    sim->ewLight.timer = 50;
    sim->ewLight.durations = (struct {int green; int yellow; int red;}){50, 10, 50};
    
    // Initialize cars
    for (int i = 0; i < MAX_CARS; i++) {
        sim->cars[i].active = false;
        sim->cars[i].id = i;
    }
    sim->carCount = 0;
    sim->activeCarCount = 0;
    sim->frameCount = 0;
    
    // Draw roads and intersection
    drawRoads(sim);
    drawIntersection(sim);
}

void drawRoads(Simulation *sim) {
    // Draw horizontal roads
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = INTERSECTION_Y - 1; y <= INTERSECTION_Y + 2; y++) {
            if (y >= 0 && y < GRID_HEIGHT) {
                sim->grid[y][x] = '-';
            }
        }
    }
    
    // Draw vertical roads
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = INTERSECTION_X - 1; x <= INTERSECTION_X + 2; x++) {
            if (x >= 0 && x < GRID_WIDTH) {
                sim->grid[y][x] = '|';
            }
        }
    }
}

void drawIntersection(Simulation *sim) {
    // Draw intersection corners
    for (int y = INTERSECTION_Y - 1; y <= INTERSECTION_Y + 2; y++) {
        for (int x = INTERSECTION_X - 1; x <= INTERSECTION_X + 2; x++) {
            if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT) {
                sim->grid[y][x] = '+';
            }
        }
    }
}

void displayGrid(const Simulation *sim) {
    CLEAR_SCREEN();
    
    // Display border
    setColor(COLOR_CYAN);
    printf("+");
    for (int i = 0; i < GRID_WIDTH; i++) printf("-");
    printf("+\n");
    
    // Display grid with border
    for (int y = 0; y < GRID_HEIGHT; y++) {
        printf("|");
        for (int x = 0; x < GRID_WIDTH; x++) {
            putchar(sim->grid[y][x]);
        }
        printf("|\n");
    }
    
    // Bottom border
    printf("+");
    for (int i = 0; i < GRID_WIDTH; i++) printf("-");
    printf("+\n");
    COLOR_RESET();
}

void updateTrafficLight(TrafficLight *light) {
    if (!light) return;
    
    light->timer--;
    if (light->timer > 0) return;
    
    switch (light->state) {
        case LIGHT_GREEN:
            light->state = LIGHT_YELLOW;
            light->timer = light->durations.yellow;
            break;
        case LIGHT_YELLOW:
            light->state = LIGHT_RED;
            light->timer = light->durations.red;
            break;
        case LIGHT_RED:
            light->state = LIGHT_GREEN;
            light->timer = light->durations.green;
            break;
    }
}

void displayTrafficLights(const Simulation *sim) {
    printf("\n");
    setColor(COLOR_CYAN);
    printf("=== TRAFFIC LIGHTS ===\n");
    COLOR_RESET();
    
    // North-South light
    printf("N-S Light: ");
    switch (sim->nsLight.state) {
        case LIGHT_RED:    setColor(COLOR_RED);    printf("RED    "); break;
        case LIGHT_YELLOW: setColor(COLOR_YELLOW); printf("YELLOW "); break;
        case LIGHT_GREEN:  setColor(COLOR_GREEN);  printf("GREEN  "); break;
    }
    COLOR_RESET();
    printf(" (Timer: %2d)\n", sim->nsLight.timer);
    
    // East-West light
    printf("E-W Light: ");
    switch (sim->ewLight.state) {
        case LIGHT_RED:    setColor(COLOR_RED);    printf("RED    "); break;
        case LIGHT_YELLOW: setColor(COLOR_YELLOW); printf("YELLOW "); break;
        case LIGHT_GREEN:  setColor(COLOR_GREEN);  printf("GREEN  "); break;
    }
    COLOR_RESET();
    printf(" (Timer: %2d)\n", sim->ewLight.timer);
    
    // Car statistics
    printf("\nCars: %d active / %d total\n", 
           sim->activeCarCount, sim->carCount);
    printf("Frame: %d\n", sim->frameCount);
}

bool isValidPosition(int x, int y) {
    return x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT;
}

bool isPositionOccupied(const Simulation *sim, int x, int y, int excludeId) {
    for (int i = 0; i < MAX_CARS; i++) {
        if (sim->cars[i].active && 
            sim->cars[i].id != excludeId && 
            sim->cars[i].x == x && 
            sim->cars[i].y == y) {
            return true;
        }
    }
    return false;
}

bool canCarMove(const Car *car, const Simulation *sim) {
    if (!car || !car->active) return false;
    
    // Check if car is in intersection zone
    bool inIntersectionZone = false;
    if (car->direction == DIR_NORTH || car->direction == DIR_SOUTH) {
        inIntersectionZone = (car->y >= INTERSECTION_Y - 2 && 
                             car->y <= INTERSECTION_Y + 3);
    } else {
        inIntersectionZone = (car->x >= INTERSECTION_X - 2 && 
                             car->x <= INTERSECTION_X + 3);
    }
    
    if (!inIntersectionZone) return true;
    
    // Check traffic light
    if (car->direction == DIR_NORTH || car->direction == DIR_SOUTH) {
        return sim->nsLight.state != LIGHT_RED;
    } else {
        return sim->ewLight.state != LIGHT_RED;
    }
}

void drawCar(const Car *car, bool erase, const Simulation *sim) {
    if (!car || !isValidPosition(car->x, car->y)) return;
    
    // Move cursor to car position (1-based for ANSI)
    printf("\033[%d;%dH", car->y + 2, car->x + 2);  // +2 for border
    
    if (erase) {
        putchar(sim->grid[car->y][car->x]);
    } else {
        setColor(COLOR_BLUE);
        putchar(car->symbol);
        COLOR_RESET();
    }
}

bool spawnCar(Simulation *sim) {
    if (sim->activeCarCount >= MAX_CARS) return false;
    
    // Find inactive car slot
    int slot = -1;
    for (int i = 0; i < MAX_CARS; i++) {
        if (!sim->cars[i].active) {
            slot = i;
            break;
        }
    }
    if (slot == -1) return false;
    
    Car *car = &sim->cars[slot];
    car->direction = rand() % 4;
    car->active = true;
    
    // Set starting position based on direction
    switch (car->direction) {
        case DIR_NORTH:
            car->x = INTERSECTION_X;
            car->y = GRID_HEIGHT - 2;
            car->symbol = '^';
            break;
        case DIR_SOUTH:
            car->x = INTERSECTION_X + 1;
            car->y = 1;
            car->symbol = 'v';
            break;
        case DIR_EAST:
            car->x = 1;
            car->y = INTERSECTION_Y;
            car->symbol = '>';
            break;
        case DIR_WEST:
            car->x = GRID_WIDTH - 2;
            car->y = INTERSECTION_Y + 1;
            car->symbol = '<';
            break;
    }
    
    // Check if starting position is free
    if (isPositionOccupied(sim, car->x, car->y, car->id)) {
        car->active = false;
        return false;
    }
    
    sim->carCount++;
    sim->activeCarCount++;
    return true;
}

void updateCars(Simulation *sim) {
    for (int i = 0; i < MAX_CARS; i++) {
        Car *car = &sim->cars[i];
        if (!car->active) continue;
        
        // Calculate next position
        int nextX = car->x;
        int nextY = car->y;
        
        switch (car->direction) {
            case DIR_NORTH: nextY--; break;
            case DIR_SOUTH: nextY++; break;
            case DIR_EAST:  nextX++; break;
            case DIR_WEST:  nextX--; break;
        }
        
        // Check if car leaves screen
        if (!isValidPosition(nextX, nextY)) {
            drawCar(car, true, sim);
            car->active = false;
            sim->activeCarCount--;
            continue;
        }
        
        // Check if can move
        if (!canCarMove(car, sim) || 
            isPositionOccupied(sim, nextX, nextY, car->id)) {
            drawCar(car, false, sim);
            continue;
        }
        
        // Move car
        drawCar(car, true, sim);
        car->x = nextX;
        car->y = nextY;
        drawCar(car, false, sim);
    }
}

void displayMenu(void) {
    CLEAR_SCREEN();
    
    setColor(COLOR_CYAN);
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║           TRAFFIC INTERSECTION SIMULATION SYSTEM            ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    COLOR_RESET();
    
    printf("\n");
    setColor(COLOR_MAGENTA);
    printf("Features:\n");
    COLOR_RESET();
    printf("  • Realistic traffic light timing with configurable intervals\n");
    printf("  • Multi-directional vehicle flow with collision avoidance\n");
    printf("  • Dynamic car spawning with intelligent placement\n");
    printf("  • Visual statistics and real-time monitoring\n");
    
    printf("\n");
    setColor(COLOR_GREEN);
    printf("MAIN MENU\n");
    COLOR_RESET();
    printf("════════════════════════════════════════════════════════════════\n\n");
    printf("  1. Start Custom Simulation\n");
    printf("  2. Start Standard Simulation (60 seconds)\n");
    printf("  3. Exit Program\n\n");
    printf("════════════════════════════════════════════════════════════════\n");
}

int getIntegerInput(const char *prompt, int min, int max) {
    char input[100];
    int value;
    
    while (true) {
        printf("\n%s", prompt);
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            printf("Error reading input. Please try again.\n");
            continue;
        }
        
        // Remove newline character
        input[strcspn(input, "\n")] = 0;
        
        if (sscanf(input, "%d", &value) != 1) {
            printf("Invalid input. Please enter a number between %d and %d.\n", min, max);
            continue;
        }
        
        if (value < min || value > max) {
            printf("Please enter a number between %d and %d.\n", min, max);
            continue;
        }
        
        return value;
    }
}

void runSimulation(Simulation *sim, int durationSeconds) {
    int totalFrames = durationSeconds * FRAMES_PER_SECOND;
    
    displayGrid(sim);
    
    for (sim->frameCount = 0; sim->frameCount < totalFrames; sim->frameCount++) {
        // Update traffic lights
        updateTrafficLight(&sim->nsLight);
        updateTrafficLight(&sim->ewLight);
        
        // Spawn new cars
        if (sim->frameCount % SPAWN_INTERVAL == 0) {
            spawnCar(sim);
        }
        
        // Update cars
        updateCars(sim);
        
        // Display status
        printf("\033[%d;0H", GRID_HEIGHT + 3);  // Move below grid
        displayTrafficLights(sim);
        
        fflush(stdout);
        SLEEP(MS_PER_FRAME);
    }
    
    // Simulation complete
    CLEAR_SCREEN();
    setColor(COLOR_GREEN);
    printf("\n╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    SIMULATION COMPLETE                      ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");
    COLOR_RESET();
    
    printf("Duration: %d seconds\n", durationSeconds);
    printf("Total cars spawned: %d\n", sim->carCount);
    printf("Active cars at end: %d\n", sim->activeCarCount);
    printf("\nPress Enter to return to menu...");
    
    // Clear input buffer
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    getchar();
}

void cleanupSimulation(Simulation *sim) {
    // Currently nothing dynamic to free, but structure is ready for expansion
    (void)sim;  // Mark as used to avoid compiler warning
}
