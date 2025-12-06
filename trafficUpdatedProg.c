#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #include <windows.h>
    #define SLEEP(ms) Sleep(ms)
    #define CLEAR_SCREEN() system("cls")
#else
    #include <unistd.h>
    #define SLEEP(ms) usleep((ms) * 1000)
    #define CLEAR_SCREEN() printf("\033[2J\033[H")
#endif

/* ============ CONSTANTS ============ */
#define GRID_WIDTH 80
#define GRID_HEIGHT 24
#define INTERSECTION_X 40
#define INTERSECTION_Y 12
#define MAX_CARS 50
#define FRAMES_PER_SECOND 10
#define MS_PER_FRAME 100
#define SPAWN_INTERVAL 30
#define MIN_SIM_TIME 1
#define MAX_SIM_TIME 300

/* ============ ROAD BOUNDARIES ============ */
#define NS_LANE_WIDTH 6
#define EW_LANE_WIDTH 4

#define LEFT_BORDER (INTERSECTION_X - (NS_LANE_WIDTH/2))
#define RIGHT_BORDER (INTERSECTION_X + (NS_LANE_WIDTH/2) - 1)
#define TOP_BORDER (INTERSECTION_Y - (EW_LANE_WIDTH/2))
#define BOTTOM_BORDER (INTERSECTION_Y + (EW_LANE_WIDTH/2) - 1)

/* ============ LANE DEFINITIONS ============ */
/* N-S Lanes - INSIDE the borders, not on them */
#define NS_EAST_LANE (LEFT_BORDER + 2)      /* Inside left border */
#define NS_WEST_LANE (RIGHT_BORDER - 2)     /* Inside right border */

/* E-W Lanes - INSIDE the borders, not on them */
#define EW_NORTH_LANE (TOP_BORDER + 2)      /* Inside top border */
#define EW_SOUTH_LANE (BOTTOM_BORDER - 2)   /* Inside bottom border */

/* ============ STOP LINE POSITIONS ============ */
#define STOP_LINE_DISTANCE 1  /* Cars stop 1 cell before intersection */

/* ============ ANSI COLOR CODES ============ */
#define COLOR_BLACK "\033[30m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"
#define COLOR_RESET "\033[0m"

/* ============ ENUMS ============ */
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

/* ============ STRUCTURES ============ */
typedef struct {
    LightState state;
    int timer;
    int green_duration;
    int yellow_duration;
    int red_duration;
} TrafficLight;

typedef struct {
    int x, y;
    Direction direction;
    char symbol;
    int active;
    int id;
    int has_crossed_intersection;
} Car;

typedef struct {
    char grid[GRID_HEIGHT][GRID_WIDTH];
    TrafficLight nsLight;
    TrafficLight ewLight;
    Car cars[MAX_CARS];
    int carCount;
    int activeCarCount;
    int frameCount;
} Simulation;

/* ============ FUNCTION PROTOTYPES ============ */
void initSimulation(Simulation *sim);
void drawBordersOnly(Simulation *sim);
void displayGrid(Simulation *sim);
void updateTrafficLight(TrafficLight *light);
void displayTrafficLights(Simulation *sim);
int isValidPosition(int x, int y);
int isPositionOccupied(Simulation *sim, int x, int y, int excludeId);
int canCarMove(Car *car, Simulation *sim);
void drawCar(Car *car, int erase, Simulation *sim);
int spawnCar(Simulation *sim);
void updateCars(Simulation *sim);
void displayMenu(void);
int getIntegerInput(const char *prompt, int min, int max);
void runSimulation(Simulation *sim, int durationSeconds);
int isInIntersection(int x, int y);
int isAboutToEnterIntersection(Car *car);
int hasCrossedIntersection(Car *car);
char getCharAtPosition(int x, int y);
int isInSameLaneOppositeDirection(Car *car1, Car *car2);
int isAtStopLine(Car *car);  /* NEW: Check if car is at stop line */

/* ============ MAIN FUNCTION ============ */
int main() {
    Simulation sim;
    int choice;
    int duration;
    
    srand((unsigned int)time(NULL));
    
    while (1) {
        displayMenu();
        choice = getIntegerInput("Enter your choice (1-3): ", 1, 3);
        
        if (choice == 1) {
            duration = getIntegerInput(
                "Enter simulation duration in seconds (1-300): ", 
                MIN_SIM_TIME, 
                MAX_SIM_TIME
            );
            initSimulation(&sim);
            runSimulation(&sim, duration);
        }
        else if (choice == 2) {
            initSimulation(&sim);
            runSimulation(&sim, 60);
        }
        else if (choice == 3) {
            CLEAR_SCREEN();
            printf("%sThank you for using Traffic Simulation!%s\n", COLOR_CYAN, COLOR_RESET);
            return 0;
        }
        else {
            printf("Unexpected error.\n");
            SLEEP(1000);
        }
    }
    
    return 0;
}

/* ============ IMPLEMENTATIONS ============ */

/* NEW: Check if car is at stop line (right before intersection) */
int isAtStopLine(Car *car) {
    if (car == NULL) return 0;
    
    if (car->direction == DIR_NORTH) {
        /* North-bound: stop at BOTTOM_BORDER + STOP_LINE_DISTANCE */
        return (car->y == BOTTOM_BORDER + STOP_LINE_DISTANCE);
    }
    else if (car->direction == DIR_SOUTH) {
        /* South-bound: stop at TOP_BORDER - STOP_LINE_DISTANCE */
        return (car->y == TOP_BORDER - STOP_LINE_DISTANCE);
    }
    else if (car->direction == DIR_EAST) {
        /* East-bound: stop at LEFT_BORDER - STOP_LINE_DISTANCE */
        return (car->x == LEFT_BORDER - STOP_LINE_DISTANCE);
    }
    else if (car->direction == DIR_WEST) {
        /* West-bound: stop at RIGHT_BORDER + STOP_LINE_DISTANCE */
        return (car->x == RIGHT_BORDER + STOP_LINE_DISTANCE);
    }
    
    return 0;
}

int isInSameLaneOppositeDirection(Car *car1, Car *car2) {
    if (car1 == NULL || car2 == NULL) return 0;
    
    if (car1->direction == DIR_NORTH && car2->direction == DIR_SOUTH) {
        return (car1->x == car2->x);
    }
    if (car1->direction == DIR_SOUTH && car2->direction == DIR_NORTH) {
        return (car1->x == car2->x);
    }
    
    if (car1->direction == DIR_EAST && car2->direction == DIR_WEST) {
        return (car1->y == car2->y);
    }
    if (car1->direction == DIR_WEST && car2->direction == DIR_EAST) {
        return (car1->y == car2->y);
    }
    
    return 0;
}

/* FIXED: Only return border characters for border positions */
char getCharAtPosition(int x, int y) {
    /* Check for vertical borders */
    if (x == LEFT_BORDER) {
        /* Only draw if it's in the E-W road area or outside */
        if (y >= TOP_BORDER && y <= BOTTOM_BORDER) {
            return '|';  /* Inside intersection area */
        }
        return '|';  /* Outside intersection area */
    }
    
    if (x == RIGHT_BORDER) {
        if (y >= TOP_BORDER && y <= BOTTOM_BORDER) {
            return '|';
        }
        return '|';
    }
    
    /* Check for horizontal borders */
    if (y == TOP_BORDER) {
        if (x >= LEFT_BORDER && x <= RIGHT_BORDER) {
            return '-';  /* Inside intersection area */
        }
        return '-';  /* Outside intersection area */
    }
    
    if (y == BOTTOM_BORDER) {
        if (x >= LEFT_BORDER && x <= RIGHT_BORDER) {
            return '-';
        }
        return '-';
    }
    
    /* Intersection corners */
    if ((x == LEFT_BORDER || x == RIGHT_BORDER) &&
        (y == TOP_BORDER || y == BOTTOM_BORDER)) {
        return '+';
    }
    
    /* Everything else is empty space */
    return ' ';
}

int isInIntersection(int x, int y) {
    return (x >= LEFT_BORDER && x <= RIGHT_BORDER &&
            y >= TOP_BORDER && y <= BOTTOM_BORDER);
}

int isAboutToEnterIntersection(Car *car) {
    if (car == NULL) return 0;
    
    int nextX = car->x;
    int nextY = car->y;
    
    switch (car->direction) {
        case DIR_NORTH: nextY--; break;
        case DIR_SOUTH: nextY++; break;
        case DIR_EAST:  nextX++; break;
        case DIR_WEST:  nextX--; break;
    }
    
    return (!isInIntersection(car->x, car->y) && isInIntersection(nextX, nextY));
}

int hasCrossedIntersection(Car *car) {
    if (car == NULL) return 0;
    
    if (car->direction == DIR_NORTH) {
        return (car->y < TOP_BORDER);
    }
    else if (car->direction == DIR_SOUTH) {
        return (car->y > BOTTOM_BORDER);
    }
    else if (car->direction == DIR_EAST) {
        return (car->x > RIGHT_BORDER);
    }
    else if (car->direction == DIR_WEST) {
        return (car->x < LEFT_BORDER);
    }
    
    return 0;
}

void initSimulation(Simulation *sim) {
    int i, j;
    
    if (sim == NULL) return;
    
    for (i = 0; i < GRID_HEIGHT; i++) {
        for (j = 0; j < GRID_WIDTH; j++) {
            sim->grid[i][j] = ' ';
        }
    }
    
    sim->nsLight.state = LIGHT_GREEN;
    sim->nsLight.timer = 50;
    sim->nsLight.green_duration = 50;
    sim->nsLight.yellow_duration = 10;
    sim->nsLight.red_duration = 50;
    
    sim->ewLight.state = LIGHT_RED;
    sim->ewLight.timer = 50;
    sim->ewLight.green_duration = 50;
    sim->ewLight.yellow_duration = 10;
    sim->ewLight.red_duration = 50;
    
    for (i = 0; i < MAX_CARS; i++) {
        sim->cars[i].active = 0;
        sim->cars[i].id = i;
        sim->cars[i].has_crossed_intersection = 0;
    }
    sim->carCount = 0;
    sim->activeCarCount = 0;
    sim->frameCount = 0;
    
    drawBordersOnly(sim);
}

void drawBordersOnly(Simulation *sim) {
    int x, y;
    
    /* Draw top and bottom borders (full width) */
    for (x = 0; x < GRID_WIDTH; x++) {
        sim->grid[TOP_BORDER][x] = '-';
        sim->grid[BOTTOM_BORDER][x] = '-';
    }
    
    /* Draw left and right borders (full height) */
    for (y = 0; y < GRID_HEIGHT; y++) {
        sim->grid[y][LEFT_BORDER] = '|';
        sim->grid[y][RIGHT_BORDER] = '|';
    }
    
    /* Draw intersection corners */
    sim->grid[TOP_BORDER][LEFT_BORDER] = '+';
    sim->grid[TOP_BORDER][RIGHT_BORDER] = '+';
    sim->grid[BOTTOM_BORDER][LEFT_BORDER] = '+';
    sim->grid[BOTTOM_BORDER][RIGHT_BORDER] = '+';
}

void displayGrid(Simulation *sim) {
    int i, j;
    
    printf("\033[2J\033[H");
    
    for (i = 0; i < GRID_HEIGHT; i++) {
        for (j = 0; j < GRID_WIDTH; j++) {
            putchar(sim->grid[i][j]);
        }
        putchar('\n');
    }
}

void updateTrafficLight(TrafficLight *light) {
    if (light == NULL) return;
    
    light->timer--;
    if (light->timer > 0) return;
    
    switch (light->state) {
        case LIGHT_GREEN:
            light->state = LIGHT_YELLOW;
            light->timer = light->yellow_duration;
            break;
        case LIGHT_YELLOW:
            light->state = LIGHT_RED;
            light->timer = light->red_duration;
            break;
        case LIGHT_RED:
            light->state = LIGHT_GREEN;
            light->timer = light->green_duration;
            break;
    }
}

void displayTrafficLights(Simulation *sim) {
    printf("\033[%d;1H", GRID_HEIGHT + 1);
    
    printf("%s=== TRAFFIC LIGHTS ===%s\n", COLOR_CYAN, COLOR_RESET);
    
    printf("N-S Light: ");
    switch (sim->nsLight.state) {
        case LIGHT_RED:
            printf("%sRED    %s", COLOR_RED, COLOR_RESET);
            break;
        case LIGHT_YELLOW:
            printf("%sYELLOW %s", COLOR_YELLOW, COLOR_RESET);
            break;
        case LIGHT_GREEN:
            printf("%sGREEN  %s", COLOR_GREEN, COLOR_RESET);
            break;
    }
    printf(" (Timer: %2d)\n", sim->nsLight.timer);
    
    printf("E-W Light: ");
    switch (sim->ewLight.state) {
        case LIGHT_RED:
            printf("%sRED    %s", COLOR_RED, COLOR_RESET);
            break;
        case LIGHT_YELLOW:
            printf("%sYELLOW %s", COLOR_YELLOW, COLOR_RESET);
            break;
        case LIGHT_GREEN:
            printf("%sGREEN  %s", COLOR_GREEN, COLOR_RESET);
            break;
    }
    printf(" (Timer: %2d)\n", sim->ewLight.timer);
    
    printf("\nCars: %d active / %d total\n", 
           sim->activeCarCount, sim->carCount);
    printf("Frame: %d\n", sim->frameCount);
}

int isValidPosition(int x, int y) {
    return (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT);
}

int isPositionOccupied(Simulation *sim, int x, int y, int excludeId) {
    int i, j;
    Car *currentCar;
    Car *otherCar;
    
    for (i = 0; i < MAX_CARS; i++) {
        if (!sim->cars[i].active || sim->cars[i].id == excludeId) {
            continue;
        }
        
        if (sim->cars[i].x == x && sim->cars[i].y == y) {
            return 1;
        }
        
        currentCar = NULL;
        for (j = 0; j < MAX_CARS; j++) {
            if (sim->cars[j].active && sim->cars[j].id == excludeId) {
                currentCar = &sim->cars[j];
                break;
            }
        }
        
        otherCar = &sim->cars[i];
        
        if (currentCar && isInSameLaneOppositeDirection(currentCar, otherCar)) {
            if (currentCar->direction == DIR_NORTH && otherCar->direction == DIR_SOUTH) {
                if (currentCar->y > otherCar->y && y <= otherCar->y) {
                    return 1;
                }
            }
            else if (currentCar->direction == DIR_SOUTH && otherCar->direction == DIR_NORTH) {
                if (currentCar->y < otherCar->y && y >= otherCar->y) {
                    return 1;
                }
            }
            else if (currentCar->direction == DIR_EAST && otherCar->direction == DIR_WEST) {
                if (currentCar->x < otherCar->x && x >= otherCar->x) {
                    return 1;
                }
            }
            else if (currentCar->direction == DIR_WEST && otherCar->direction == DIR_EAST) {
                if (currentCar->x > otherCar->x && x <= otherCar->x) {
                    return 1;
                }
            }
        }
    }
    
    return 0;
}

/* FIXED: Cars stop at stop line BEFORE intersection, not in it */
int canCarMove(Car *car, Simulation *sim) {
    int atStopLine;
    int hasCrossed;
    int currentlyInIntersection;
    
    if (car == NULL || !car->active) return 0;
    
    hasCrossed = hasCrossedIntersection(car);
    if (hasCrossed) {
        return 1;  /* Once crossed, ignore all lights */
    }
    
    currentlyInIntersection = isInIntersection(car->x, car->y);
    
    if (currentlyInIntersection) {
        return 1;  /* Once in intersection, keep moving */
    }
    
    /* NEW: Check if car is at stop line */
    atStopLine = isAtStopLine(car);
    
    if (!atStopLine) {
        return 1;  /* Not at stop line, can move */
    }
    
    /* Car is AT STOP LINE - check traffic light */
    if (car->direction == DIR_NORTH || car->direction == DIR_SOUTH) {
        if (sim->nsLight.state == LIGHT_RED) {
            return 0;  /* Stop at red light */
        } else {
            return 1;  /* Move on green/yellow */
        }
    } else {
        if (sim->ewLight.state == LIGHT_RED) {
            return 0;  /* Stop at red light */
        } else {
            return 1;  /* Move on green/yellow */
        }
    }
}

void drawCar(Car *car, int erase, Simulation *sim) {
    if (car == NULL || !isValidPosition(car->x, car->y)) return;
    
    printf("\033[%d;%dH", car->y + 1, car->x + 1);
    
    if (erase) {
        /* Always restore the correct character */
        char originalChar = getCharAtPosition(car->x, car->y);
        putchar(originalChar);
    } else {
        /* Color code based on position */
        if (isAtStopLine(car)) {
            printf("%s%c%s", COLOR_YELLOW, car->symbol, COLOR_RESET);
        } else if (isInIntersection(car->x, car->y)) {
            printf("%s%c%s", COLOR_GREEN, car->symbol, COLOR_RESET);
        } else if (hasCrossedIntersection(car)) {
            printf("%s%c%s", COLOR_MAGENTA, car->symbol, COLOR_RESET);
        } else {
            printf("%s%c%s", COLOR_BLUE, car->symbol, COLOR_RESET);
        }
    }
    fflush(stdout);
}

/* FIXED: Spawn cars INSIDE lanes, NOT on borders */
int spawnCar(Simulation *sim) {
    int slot;
    Car *car;
    
    if (sim->activeCarCount >= MAX_CARS) return 0;
    
    slot = -1;
    for (slot = 0; slot < MAX_CARS; slot++) {
        if (!sim->cars[slot].active) {
            break;
        }
    }
    if (slot == MAX_CARS) return 0;
    
    car = &sim->cars[slot];
    car->direction = rand() % 4;
    car->active = 1;
    car->has_crossed_intersection = 0;
    
    /* Spawn cars INSIDE the road area, NOT on borders */
    switch (car->direction) {
        case DIR_NORTH:
            car->x = NS_EAST_LANE;      /* Inside left border */
            car->y = GRID_HEIGHT - 3;   /* Not on bottom edge */
            car->symbol = '^';
            break;
        case DIR_SOUTH:
            car->x = NS_WEST_LANE;      /* Inside right border */
            car->y = 2;                 /* Not on top edge */
            car->symbol = 'v';
            break;
        case DIR_EAST:
            car->x = 2;                 /* Not on left edge */
            car->y = EW_SOUTH_LANE;     /* Inside bottom border */
            car->symbol = '>';
            break;
        case DIR_WEST:
            car->x = GRID_WIDTH - 3;    /* Not on right edge */
            car->y = EW_NORTH_LANE;     /* Inside top border */
            car->symbol = '<';
            break;
    }
    
    /* Ensure car is not spawning in intersection or at stop line */
    if (isInIntersection(car->x, car->y) || isAtStopLine(car)) {
        /* Adjust position if spawned in wrong place */
        if (car->direction == DIR_NORTH) car->y = BOTTOM_BORDER + 3;
        else if (car->direction == DIR_SOUTH) car->y = TOP_BORDER - 3;
        else if (car->direction == DIR_EAST) car->x = LEFT_BORDER - 3;
        else if (car->direction == DIR_WEST) car->x = RIGHT_BORDER + 3;
    }
    
    if (isPositionOccupied(sim, car->x, car->y, car->id)) {
        car->active = 0;
        return 0;
    }
    
    sim->carCount++;
    sim->activeCarCount++;
    return 1;
}

void updateCars(Simulation *sim) {
    int i;
    Car *car;
    int nextX, nextY;
    int wasInIntersection, willBeInIntersection;
    
    for (i = 0; i < MAX_CARS; i++) {
        car = &sim->cars[i];
        if (!car->active) continue;
        
        nextX = car->x;
        nextY = car->y;
        
        switch (car->direction) {
            case DIR_NORTH:
                nextY--;
                break;
            case DIR_SOUTH:
                nextY++;
                break;
            case DIR_EAST:
                nextX++;
                break;
            case DIR_WEST:
                nextX--;
                break;
        }
        
        if (!isValidPosition(nextX, nextY)) {
            drawCar(car, 1, sim);
            car->active = 0;
            sim->activeCarCount--;
            continue;
        }
        
        if (!canCarMove(car, sim) || 
            isPositionOccupied(sim, nextX, nextY, car->id)) {
            drawCar(car, 0, sim);
            continue;
        }
        
        drawCar(car, 1, sim);
        car->x = nextX;
        car->y = nextY;
        drawCar(car, 0, sim);
        
        wasInIntersection = isInIntersection(car->x, car->y);
        willBeInIntersection = isInIntersection(nextX, nextY);
        
        if (wasInIntersection && !willBeInIntersection) {
            car->has_crossed_intersection = 1;
        }
    }
}

void displayMenu(void) {
    CLEAR_SCREEN();
    
    printf("%s==============================================================%s\n", 
           COLOR_CYAN, COLOR_RESET);
    printf("           TRAFFIC INTERSECTION SIMULATION SYSTEM            \n");
    printf("%s==============================================================%s\n", 
           COLOR_CYAN, COLOR_RESET);
    
    printf("\n%sFeatures:%s\n", COLOR_MAGENTA, COLOR_RESET);
    printf("  * CLEAN design - ONLY borders visible\n");
    printf("  * Cars spawn INSIDE lanes, NOT on borders\n");
    printf("  * Cars stop at stop line (1 cell BEFORE intersection)\n");
    printf("  * No border erasure - borders stay intact\n");
    printf("  * One-way lanes only\n");
    
    printf("\n%sSTOP LINE POSITIONS:%s\n", COLOR_GREEN, COLOR_RESET);
    printf("  North-bound: stops at Y=%d (1 above intersection)\n", BOTTOM_BORDER + 1);
    printf("  South-bound: stops at Y=%d (1 below intersection)\n", TOP_BORDER - 1);
    printf("  East-bound: stops at X=%d (1 left of intersection)\n", LEFT_BORDER - 1);
    printf("  West-bound: stops at X=%d (1 right of intersection)\n", RIGHT_BORDER + 1);
    
    printf("\n%sMAIN MENU%s\n", COLOR_GREEN, COLOR_RESET);
    printf("==============================================================\n\n");
    printf("  1. Start Custom Simulation\n");
    printf("  2. Start Standard Simulation (60 seconds)\n");
    printf("  3. Exit Program\n\n");
    printf("==============================================================\n");
}

int getIntegerInput(const char *prompt, int min, int max) {
    char input[100];
    int value;
    char *endptr;
    
    while (1) {
        printf("\n%s", prompt);
        fflush(stdout);
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Error reading input. Please try again.\n");
            continue;
        }
        
        input[strcspn(input, "\n")] = '\0';
        value = strtol(input, &endptr, 10);
        
        if (endptr == input || *endptr != '\0') {
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
    int totalFrames;
    int i;
    
    totalFrames = durationSeconds * FRAMES_PER_SECOND;
    
    displayGrid(sim);
    displayTrafficLights(sim);
    
    printf("\n%sCONFIGURATION:%s\n", COLOR_CYAN, COLOR_RESET);
    printf("  Borders: X[%d,%d], Y[%d,%d]\n", 
           LEFT_BORDER, RIGHT_BORDER, TOP_BORDER, BOTTOM_BORDER);
    printf("  N-S Lanes: East=%d (^), West=%d (v)\n", NS_EAST_LANE, NS_WEST_LANE);
    printf("  E-W Lanes: North=%d (<), South=%d (>)\n", EW_NORTH_LANE, EW_SOUTH_LANE);
    printf("  Stop line: %d cell(s) before intersection\n", STOP_LINE_DISTANCE);
    printf("  Cars NEVER spawn or move on borders!\n");
    
    printf("\n%sCar States:%s\n", COLOR_CYAN, COLOR_RESET);
    printf("  %sBlue%s = Normal moving\n", COLOR_BLUE, COLOR_RESET);
    printf("  %sYellow%s = At stop line (checking light)\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %sGreen%s = In intersection\n", COLOR_GREEN, COLOR_RESET);
    printf("  %sMagenta%s = Crossed intersection\n", COLOR_MAGENTA, COLOR_RESET);
    printf("\nCars stop ONLY at YELLOW positions\n");
    
    fflush(stdout);
    SLEEP(1000);
    
    for (sim->frameCount = 0; sim->frameCount < totalFrames; sim->frameCount++) {
        updateTrafficLight(&sim->nsLight);
        updateTrafficLight(&sim->ewLight);
        
        if (sim->frameCount % SPAWN_INTERVAL == 0) {
            spawnCar(sim);
        }
        
        updateCars(sim);
        
        printf("\033[%d;1H", GRID_HEIGHT + 1);
        displayTrafficLights(sim);
        
        fflush(stdout);
        SLEEP(MS_PER_FRAME);
    }
    
    CLEAR_SCREEN();
    printf("%s==============================================================%s\n", 
           COLOR_GREEN, COLOR_RESET);
    printf("                    SIMULATION COMPLETE                      \n");
    printf("%s==============================================================%s\n\n", 
           COLOR_GREEN, COLOR_RESET);
    
    printf("Duration: %d seconds\n", durationSeconds);
    printf("Total cars spawned: %d\n", sim->carCount);
    printf("Active cars at end: %d\n", sim->activeCarCount);
    printf("\nPress Enter to return to menu...");
    
    while (getchar() != '\n');
    getchar();
}
