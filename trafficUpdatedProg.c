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

/* ============ LANE POSITIONS ============ */
/* N-S Lanes - safely inside borders */
#define NS_EAST_LANE (LEFT_BORDER + 2)
#define NS_WEST_LANE (RIGHT_BORDER - 2)

/* E-W Lanes - safely inside borders */
#define EW_NORTH_LANE (TOP_BORDER + 2)
#define EW_SOUTH_LANE (BOTTOM_BORDER - 2)

/* ============ STOP LINE POSITIONS ============ */
#define STOP_LINE_DISTANCE 1

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
int isAtStopLine(Car *car);
int isOnBorder(int x, int y);

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

int isOnBorder(int x, int y) {
    return (x == LEFT_BORDER || x == RIGHT_BORDER || 
            y == TOP_BORDER || y == BOTTOM_BORDER);
}

int isAtStopLine(Car *car) {
    if (car == NULL) return 0;
    
    if (car->direction == DIR_NORTH) {
        return (car->y == BOTTOM_BORDER + STOP_LINE_DISTANCE);
    }
    else if (car->direction == DIR_SOUTH) {
        return (car->y == TOP_BORDER - STOP_LINE_DISTANCE);
    }
    else if (car->direction == DIR_EAST) {
        return (car->x == LEFT_BORDER - STOP_LINE_DISTANCE);
    }
    else if (car->direction == DIR_WEST) {
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

char getCharAtPosition(int x, int y) {
    /* Border characters */
    if (x == LEFT_BORDER || x == RIGHT_BORDER) {
        return '|';
    }
    
    if (y == TOP_BORDER || y == BOTTOM_BORDER) {
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
    
    for (x = 0; x < GRID_WIDTH; x++) {
        sim->grid[TOP_BORDER][x] = '-';
        sim->grid[BOTTOM_BORDER][x] = '-';
    }
    
    for (y = 0; y < GRID_HEIGHT; y++) {
        sim->grid[y][LEFT_BORDER] = '|';
        sim->grid[y][RIGHT_BORDER] = '|';
    }
    
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
    /* Position must be on screen */
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

/* SIMPLIFIED: Car movement logic */
int canCarMove(Car *car, Simulation *sim) {
    if (car == NULL || !car->active) return 0;
    
    /* If car has already crossed intersection, it can always move */
    if (car->has_crossed_intersection) {
        return 1;
    }
    
    /* If car is in intersection, it can always move */
    if (isInIntersection(car->x, car->y)) {
        return 1;
    }
    
    /* Check if car is at stop line */
    if (isAtStopLine(car)) {
        /* Check traffic light */
        if (car->direction == DIR_NORTH || car->direction == DIR_SOUTH) {
            return (sim->nsLight.state != LIGHT_RED);
        } else {
            return (sim->ewLight.state != LIGHT_RED);
        }
    }
    
    /* Car is not at stop line, not in intersection, hasn't crossed - it can move */
    return 1;
}

void drawCar(Car *car, int erase, Simulation *sim) {
    if (car == NULL) return;
    
    printf("\033[%d;%dH", car->y + 1, car->x + 1);
    
    if (erase) {
        /* Restore the original character at this position */
        char original = getCharAtPosition(car->x, car->y);
        putchar(original);
    } else {
        if (isAtStopLine(car)) {
            printf("%s%c%s", COLOR_YELLOW, car->symbol, COLOR_RESET);
        } else if (isInIntersection(car->x, car->y)) {
            printf("%s%c%s", COLOR_GREEN, car->symbol, COLOR_RESET);
        } else if (car->has_crossed_intersection) {
            printf("%s%c%s", COLOR_MAGENTA, car->symbol, COLOR_RESET);
        } else {
            printf("%s%c%s", COLOR_BLUE, car->symbol, COLOR_RESET);
        }
    }
    fflush(stdout);
}

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
    
    /* Spawn cars at appropriate positions */
    switch (car->direction) {
        case DIR_NORTH:
            car->x = NS_EAST_LANE;
            car->y = GRID_HEIGHT - 2;
            car->symbol = '^';
            break;
        case DIR_SOUTH:
            car->x = NS_WEST_LANE;
            car->y = 1;
            car->symbol = 'v';
            break;
        case DIR_EAST:
            car->x = 1;
            car->y = EW_SOUTH_LANE;
            car->symbol = '>';
            break;
        case DIR_WEST:
            car->x = GRID_WIDTH - 2;
            car->y = EW_NORTH_LANE;
            car->symbol = '<';
            break;
    }
    
    /* Ensure car is not spawning in occupied position */
    if (isPositionOccupied(sim, car->x, car->y, car->id)) {
        car->active = 0;
        return 0;
    }
    
    sim->carCount++;
    sim->activeCarCount++;
    return 1;
}

/* SIMPLIFIED: Cars can now properly cross the intersection */
void updateCars(Simulation *sim) {
    int i;
    Car *car;
    int nextX, nextY;
    
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
        
        /* Check if car is leaving the screen */
        if (nextX < 0 || nextX >= GRID_WIDTH || nextY < 0 || nextY >= GRID_HEIGHT) {
            drawCar(car, 1, sim);
            car->active = 0;
            sim->activeCarCount--;
            continue;
        }
        
        /* Check if car can move */
        if (!canCarMove(car, sim) || 
            isPositionOccupied(sim, nextX, nextY, car->id)) {
            drawCar(car, 0, sim);
            continue;
        }
        
        /* Move the car */
        drawCar(car, 1, sim);
        car->x = nextX;
        car->y = nextY;
        
        /* Update crossing status */
        if (!car->has_crossed_intersection) {
            car->has_crossed_intersection = hasCrossedIntersection(car);
        }
        
        drawCar(car, 0, sim);
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
    printf("  * Cars move through intersection properly\n");
    printf("  * Cars stop at stop line when light is red\n");
    printf("  * Cars continue after crossing intersection\n");
    
    printf("\n%sCar States:%s\n", COLOR_GREEN, COLOR_RESET);
    printf("  %sBlue%s = Normal moving towards intersection\n", COLOR_BLUE, COLOR_RESET);
    printf("  %sYellow%s = At stop line (checking light)\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %sGreen%s = In intersection\n", COLOR_GREEN, COLOR_RESET);
    printf("  %sMagenta%s = Crossed intersection (continuing)\n", COLOR_MAGENTA, COLOR_RESET);
    
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
    
    printf("\n%sTRAFFIC SIMULATION ACTIVE%s\n", COLOR_CYAN, COLOR_RESET);
    printf("  Cars will properly cross the intersection!\n");
    printf("  N-S Light: Green = North/South cars can go\n");
    printf("  E-W Light: Green = East/West cars can go\n");
    
    printf("\n%sCar States:%s\n", COLOR_CYAN, COLOR_RESET);
    printf("  %sBlue%s = Normal moving\n", COLOR_BLUE, COLOR_RESET);
    printf("  %sYellow%s = At stop line (checking light)\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %sGreen%s = In intersection\n", COLOR_GREEN, COLOR_RESET);
    printf("  %sMagenta%s = Crossed intersection\n", COLOR_MAGENTA, COLOR_RESET);
    
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
