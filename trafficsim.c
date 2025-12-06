/*
 * TRAFFIC INTERSECTION SIMULATION
 * A visual simulation of traffic lights and cars at an intersection
 * Designed for beginners to learn C programming concepts
 */

/* Include necessary header files */
#include <stdio.h>      // Standard input/output functions (printf, scanf, putchar, etc.)
#include <stdlib.h>     // Standard library functions (rand, srand, malloc, etc.)
#include <time.h>       // Time functions (time) for random number seeding

/* Platform-specific sleep functions */
#ifdef _WIN32           // Check if compiling on Windows
    #include <windows.h> // Windows API functions
    #define SLEEP(ms) Sleep(ms) // Windows sleep function (milliseconds)
#else                  // For Linux/Mac/Unix systems
    #include <unistd.h> // Unix standard functions
    #define SLEEP(ms) usleep((ms) * 1000) // Unix sleep (microseconds to milliseconds)
#endif

/* ============ ANSI ESCAPE CODES FOR TERMINAL CONTROL ============ */

/* Clear the entire screen and move cursor to top-left corner */
#define CLEAR_SCREEN "\033[2J\033[H"

/* Move cursor to specific row and column (1-based indexing) */
#define CURSOR_POS(r,c) printf("\033[%d;%dH", (r), (c))

/* Text color codes (change terminal text color) */
#define COLOR_RED     "\033[31m"     // Red text
#define COLOR_YELLOW  "\033[33m"     // Yellow text
#define COLOR_GREEN   "\033[32m"     // Green text
#define COLOR_BLUE    "\033[34m"     // Blue text
#define COLOR_CYAN    "\033[36m"     // Cyan text
#define COLOR_MAGENTA "\033[35m"     // Magenta text
#define COLOR_RESET   "\033[0m"      // Reset to default terminal color

/* ============ SIMULATION CONSTANTS ============ */

/* Screen dimensions for our simulation grid */
#define GRID_WIDTH 80    // Width of simulation area (80 columns typical for terminals)
#define GRID_HEIGHT 24   // Height of simulation area (24 rows typical for terminals)

/* Intersection center coordinates (approximately middle of screen) */
#define IX 40    // Intersection X coordinate (column 40)
#define IY 12    // Intersection Y coordinate (row 12)

/* ============ CUSTOM DATA TYPES (ENUMERATIONS) ============ */

/* Define possible states for traffic lights */
typedef enum { 
    RED,    // Value 0 - Stop
    YELLOW, // Value 1 - Caution/Warning
    GREEN   // Value 2 - Go
} LightState;

/* Define possible directions for cars to move */
typedef enum { 
    NORTH,  // Value 0 - Moving upward
    SOUTH,  // Value 1 - Moving downward  
    EAST,   // Value 2 - Moving right
    WEST    // Value 3 - Moving left
} Direction;

/* ============ CUSTOM DATA TYPES (STRUCTURES) ============ */

/* Structure representing a traffic light */
typedef struct {
    LightState state;  // Current state: RED, YELLOW, or GREEN
    int timer;         // Countdown timer for current state
    int g;             // Duration (in frames) for GREEN light
    int y;             // Duration (in frames) for YELLOW light  
    int r;             // Duration (in frames) for RED light
} TrafficLight;

/* Structure representing a single car */
typedef struct {
    int x, y;          // Current position (x = column, y = row)
    Direction dir;     // Current direction of movement
    char symbol;       // Character to display car (^, v, >, <)
    int active;        // 1 = car exists and is active, 0 = inactive/removed
} Car;

/* ============ GLOBAL VARIABLES ============ */

#define MAX_CARS 20         // Maximum number of cars allowed on screen at once
Car cars[MAX_CARS];         // Array to store all car objects (max 20 cars)
char grid[GRID_HEIGHT][GRID_WIDTH]; // 2D character array representing the screen

/* Two traffic lights: one for North-South, one for East-West */
TrafficLight nsLight;       // Traffic light controlling North-South traffic
TrafficLight ewLight;       // Traffic light controlling East-West traffic

/* ============ FUNCTION IMPLEMENTATIONS ============ */

/*
 * Function: initGrid
 * Purpose: Initialize the visual grid with roads and intersection markers
 * Parameters: None
 * Returns: None (void)
 * Description: Sets up the 2D grid array with spaces, then draws:
 *   - Horizontal roads as dashes (-)
 *   - Vertical roads as pipes (|)  
 *   - Intersection corners as plus signs (+)
 */
void initGrid() {
    /* Step 1: Fill entire grid with spaces (empty cells) */
    for (int i = 0; i < GRID_HEIGHT; i++)           // Loop through each row
        for (int j = 0; j < GRID_WIDTH; j++)        // Loop through each column
            grid[i][j] = ' ';                       // Set cell to space (empty)
    
    /* Step 2: Draw horizontal roads (4 rows of dashes) */
    for (int j = 0; j < GRID_WIDTH; j++) {          // Loop through all columns
        grid[IY - 1][j] = '-';  // Row above intersection center
        grid[IY][j] = '-';      // Row at intersection center  
        grid[IY + 1][j] = '-';  // Row below intersection center
        grid[IY + 2][j] = '-';  // Another row below
    }
    
    /* Step 3: Draw vertical roads (4 columns of pipes) */
    for (int i = 0; i < GRID_HEIGHT; i++) {         // Loop through all rows
        grid[i][IX - 1] = '|';  // Column left of intersection center
        grid[i][IX] = '|';      // Column at intersection center
        grid[i][IX + 1] = '|';  // Column right of intersection center
        grid[i][IX + 2] = '|';  // Another column right
    }
    
    /* Step 4: Draw intersection corners (plus signs where roads meet) */
    for (int i = IY - 1; i <= IY + 2; i++)          // Loop through intersection rows
        for (int j = IX - 1; j <= IX + 2; j++)      // Loop through intersection columns
            grid[i][j] = '+';                       // Set to plus sign
}

/*
 * Function: drawGrid  
 * Purpose: Display the grid on the terminal screen
 * Parameters: None
 * Returns: None (void)
 * Description: Prints the entire 2D grid array to the screen, row by row
 */
void drawGrid() {
    CURSOR_POS(1, 1);  // Move cursor to top-left corner (row 1, column 1)
    
    for (int i = 0; i < GRID_HEIGHT; i++) {         // Loop through each row
        /* Write entire row at once (more efficient than character by character) */
        fwrite(grid[i], 1, GRID_WIDTH, stdout);     // Write GRID_WIDTH characters
        putchar('\n');                               // Move to next line
    }
}

/*
 * Function: initTrafficLights
 * Purpose: Initialize traffic lights with starting values
 * Parameters: None  
 * Returns: None (void)
 * Description: Sets up both traffic lights with initial states and timers
 */
void initTrafficLights() {
    /* Initialize North-South light: starts GREEN with 50-frame timer */
    nsLight.state = GREEN;  // Start with green light
    nsLight.timer = 50;     // Countdown from 50 frames
    nsLight.g = 50;         // Green duration = 50 frames
    nsLight.y = 10;         // Yellow duration = 10 frames  
    nsLight.r = 50;         // Red duration = 50 frames
    
    /* Initialize East-West light: starts RED with 50-frame timer */
    ewLight.state = RED;    // Start with red light
    ewLight.timer = 50;     // Countdown from 50 frames
    ewLight.g = 50;         // Green duration = 50 frames
    ewLight.y = 10;         // Yellow duration = 10 frames
    ewLight.r = 50;         // Red duration = 50 frames
}

/*
 * Function: updateLight
 * Purpose: Update a traffic light's state based on its timer
 * Parameters: t - pointer to TrafficLight to update
 * Returns: None (void)  
 * Description: Decrements timer, changes state when timer reaches 0
 */
void updateLight(TrafficLight *t) {
    /* Step 1: Decrease timer by 1 and check if it reached 0 */
    t->timer = t->timer - 1;    // Decrement timer
    if (t->timer > 0)           // If timer still positive
        return;                  // Do nothing, exit function
    
    /* Step 2: Timer reached 0 - change to next state */
    switch (t->state) {         // Check current state
        case GREEN:             // Currently GREEN
            t->state = YELLOW;  // Change to YELLOW
            t->timer = t->y;    // Set timer to yellow duration
            break;
        case YELLOW:            // Currently YELLOW
            t->state = RED;     // Change to RED
            t->timer = t->r;    // Set timer to red duration
            break;
        case RED:               // Currently RED
            t->state = GREEN;   // Change to GREEN
            t->timer = t->g;    // Set timer to green duration
            break;
    }
}

/*
 * Function: drawTrafficLights
 * Purpose: Display current traffic light states on screen
 * Parameters: None
 * Returns: None (void)
 * Description: Shows N-S and E-W light states with colored text
 */
void drawTrafficLights() {
    /* Display North-South light status */
    CURSOR_POS(GRID_HEIGHT + 2, 1);     // Move cursor 2 rows below grid
    printf("N-S Light: ");               // Label
    
    /* Print with appropriate color based on state */
    if (nsLight.state == RED)
        printf(COLOR_RED "RED   " COLOR_RESET);     // Red text
    else if (nsLight.state == YELLOW)  
        printf(COLOR_YELLOW "YELLOW" COLOR_RESET);   // Yellow text
    else /* GREEN */
        printf(COLOR_GREEN "GREEN " COLOR_RESET);    // Green text
    
    /* Display East-West light status */
    CURSOR_POS(GRID_HEIGHT + 3, 1);     // Move cursor 3 rows below grid
    printf("E-W Light: ");               // Label
    
    /* Print with appropriate color based on state */
    if (ewLight.state == RED)
        printf(COLOR_RED "RED   " COLOR_RESET);     // Red text
    else if (ewLight.state == YELLOW)
        printf(COLOR_YELLOW "YELLOW" COLOR_RESET);   // Yellow text
    else /* GREEN */
        printf(COLOR_GREEN "GREEN " COLOR_RESET);    // Green text
}

/*
 * Function: initCars
 * Purpose: Initialize all car slots as inactive
 * Parameters: None
 * Returns: None (void)
 * Description: Sets all cars' active flag to 0 (inactive)
 */
void initCars() {
    for (int i = 0; i < MAX_CARS; i++)   // Loop through all car slots
        cars[i].active = 0;              // Mark as inactive (no car exists)
}

/*
 * Function: isOccupied
 * Purpose: Check if a position is occupied by any other car
 * Parameters: x,y - coordinates to check, ignore - car index to skip
 * Returns: 1 if occupied, 0 if free
 * Description: Checks all active cars except the one specified by 'ignore'
 */
int isOccupied(int x, int y, int ignore) {
    for (int i = 0; i < MAX_CARS; i++) {          // Loop through all cars
        /* Skip if: this is the car we're ignoring OR car is inactive */
        if (i == ignore || !cars[i].active)
            continue;                              // Skip to next car
        
        /* Check if this car occupies the (x,y) position */
        if (cars[i].x == x && cars[i].y == y)
            return 1;  // Position is occupied
    }
    return 0;  // Position is free (not occupied)
}

/*
 * Function: canMove
 * Purpose: Check if a car can move based on traffic lights
 * Parameters: c - pointer to Car to check
 * Returns: 1 if can move, 0 if must stop
 * Description: Checks if car is at intersection and light is red
 */
int canMove(Car* c) {
    /* For cars moving NORTH or SOUTH (vertical movement) */
    if (c->dir == NORTH || c->dir == SOUTH) {
        /* If North-South light is RED */
        if (nsLight.state == RED) {
            /* Check if car is approaching the intersection area */
            if ((c->dir == NORTH && c->y <= IY + 3 && c->y > IY - 2) ||
                (c->dir == SOUTH && c->y >= IY - 2 && c->y < IY + 3))
                return 0;  // Cannot move (must stop at red light)
        }
    }
    /* For cars moving EAST or WEST (horizontal movement) */
    else {
        /* If East-West light is RED */
        if (ewLight.state == RED) {
            /* Check if car is approaching the intersection area */
            if ((c->dir == EAST && c->x <= IX + 3 && c->x > IX - 2) ||
                (c->dir == WEST && c->x >= IX - 2 && c->x < IX + 3))
                return 0;  // Cannot move (must stop at red light)
        }
    }
    return 1;  // Can move (light is green or not at intersection)
}

/*
 * Function: drawCar
 * Purpose: Draw or erase a car on the screen
 * Parameters: c - pointer to Car, erase - 1 to erase, 0 to draw
 * Returns: None (void)
 * Description: Moves cursor to car position and draws/erases it
 */
void drawCar(Car* c, int erase) {
    /* Step 1: Check if car position is within screen bounds */
    if (c->y < 0 || c->y >= GRID_HEIGHT || c->x < 0 || c->x >= GRID_WIDTH)
        return;  // Car is off-screen, don't draw anything
    
    /* Step 2: Move cursor to car's position (convert to 1-based indexing) */
    CURSOR_POS(c->y + 1, c->x + 1);
    
    /* Step 3: Either erase or draw the car */
    if (erase) {
        /* Erase: Draw what's in the grid at that position (road character) */
        putchar(grid[c->y][c->x]);
    } else {
        /* Draw: Print car symbol in blue color */
        printf(COLOR_BLUE "%c" COLOR_RESET, c->symbol);
    }
}

/*
 * Function: spawnCar
 * Purpose: Create a new car at a random screen edge
 * Parameters: None
 * Returns: None (void)
 * Description: Finds inactive slot, creates car with random direction
 */
void spawnCar() {
    int slot = -1;  // Will store index of available car slot (-1 = none found)
    
    /* Step 1: Find first inactive car slot */
    for (int i = 0; i < MAX_CARS; i++) {
        if (!cars[i].active) {  // Found inactive slot
            slot = i;           // Remember slot index
            break;              // Stop searching
        }
    }
    
    /* Step 2: If no slots available, exit (max cars reached) */
    if (slot == -1)
        return;
    
    /* Step 3: Get pointer to the car slot for easier access */
    Car* c = &cars[slot];
    
    /* Step 4: Initialize car properties */
    c->active = 1;              // Mark as active
    c->dir = rand() % 4;        // Random direction (0-3)
    
    /* Step 5: Set starting position and symbol based on direction */
    switch (c->dir) {
        case NORTH:                     // Coming from bottom, going up
            c->x = IX;                  // Center horizontally
            c->y = GRID_HEIGHT - 2;     // Near bottom edge
            c->symbol = '^';            // Up arrow symbol
            break;
        case SOUTH:                     // Coming from top, going down
            c->x = IX + 1;              // Slightly right of center
            c->y = 1;                   // Near top edge
            c->symbol = 'v';            // Down arrow symbol
            break;
        case EAST:                      // Coming from left, going right
            c->x = 1;                   // Near left edge
            c->y = IY;                  // Center vertically
            c->symbol = '>';            // Right arrow symbol
            break;
        case WEST:                      // Coming from right, going left
            c->x = GRID_WIDTH - 2;      // Near right edge
            c->y = IY + 1;              // Slightly below center
            c->symbol = '<';            // Left arrow symbol
            break;
    }
    
    /* Step 6: If starting position already occupied, deactivate car */
    if (isOccupied(c->x, c->y, slot))
        c->active = 0;  // Can't spawn here - make car inactive
}

/*
 * Function: updateCars
 * Purpose: Update positions of all active cars
 * Parameters: None
 * Returns: None (void)
 * Description: Moves each car based on direction, checks boundaries and collisions
 */
void updateCars() {
    for (int i = 0; i < MAX_CARS; i++) {   // Loop through all car slots
        Car* c = &cars[i];                 // Get pointer to current car
        
        /* Skip if car is inactive */
        if (!c->active)
            continue;
        
        /* Step 1: Calculate next position based on direction */
        int nx = c->x;  // Next x coordinate (start with current)
        int ny = c->y;  // Next y coordinate (start with current)
        
        /* Adjust coordinates based on movement direction */
        if (c->dir == NORTH) ny--;    // Moving up: decrease y
        if (c->dir == SOUTH) ny++;    // Moving down: increase y
        if (c->dir == EAST)  nx++;    // Moving right: increase x
        if (c->dir == WEST)  nx--;    // Moving left: decrease x
        
        /* Step 2: Check if car has moved off-screen */
        if (nx < 0 || nx >= GRID_WIDTH || ny < 0 || ny >= GRID_HEIGHT) {
            drawCar(c, 1);     // Erase car from screen
            c->active = 0;     // Mark as inactive (removed)
            continue;          // Skip to next car
        }
        
        /* Step 3: Check if car can move (traffic light) and if spot is free */
        if (!canMove(c) || isOccupied(nx, ny, i)) {
            drawCar(c, 0);     // Redraw car in current position (stopped)
            continue;          // Skip movement for this car
        }
        
        /* Step 4: Car can move - update its position */
        drawCar(c, 1);         // Erase car from old position
        c->x = nx;             // Update x coordinate to new position
        c->y = ny;             // Update y coordinate to new position
        drawCar(c, 0);         // Draw car at new position
    }
}

/*
 * Function: displayMenu
 * Purpose: Display the main menu screen
 * Parameters: None
 * Returns: None (void)
 * Description: Shows program title, features, and menu options
 */
void displayMenu() {
    printf(CLEAR_SCREEN);  // Clear the screen
    
    /* Display program title with decorative border */
    printf(COLOR_CYAN);
    printf("||============================================================||\n");
    printf("||          TRAFFIC INTERSECTION SIMULATION SYSTEM            ||\n");
    printf("||============================================================||\n");
    printf(COLOR_RESET "\n");  // Reset color and add blank line
    
    /* Display program features */
    printf(COLOR_MAGENTA "  Features:\n" COLOR_RESET);
    printf("    1. Realistic traffic light timing\n");
    printf("    2. Multi-directional vehicle flow\n");
    printf("    3. Collision detection\n");
    printf("    4. Dynamic car spawning\n\n");
    
    /* Display menu options */
    printf(COLOR_GREEN "  MAIN MENU\n" COLOR_RESET);
    printf("  ------------------------------------------------------------------\n\n");
    printf("    1. Start Custom Simulation\n");
    printf("    2. Start Standard Simulation (60 seconds)\n");
    printf("    3. Exit Program\n\n");
    printf("  --------------------------------------------------------------------\n\n");
    
    /* Prompt for user input */
    printf("  Enter your choice (1-3): ");
    fflush(stdout);  // Force output to appear immediately (no buffering)
}

/*
 * Function: runSimulation
 * Purpose: Run the main simulation loop
 * Parameters: duration - simulation length in seconds
 * Returns: None (void)
 * Description: Initializes and runs simulation for specified duration
 */
void runSimulation(int duration) {
    /* Step 1: Setup simulation */
    printf(CLEAR_SCREEN);  // Clear screen
    initGrid();            // Initialize road grid
    initTrafficLights();   // Initialize traffic lights
    initCars();            // Initialize all cars as inactive
    drawGrid();            // Display the road grid
    
    /* Step 2: Calculate total frames (10 frames per second) */
    int frames = duration * 10;  // Each frame = 100ms, so 10 frames = 1 second
    
    /* Step 3: Main simulation loop */
    while (frames--) {           // Loop until frames count reaches 0
        /* Update traffic lights */
        updateLight(&nsLight);   // Update North-South light
        updateLight(&ewLight);   // Update East-West light
        drawTrafficLights();     // Display current light states
        
        /* Spawn new car every 3 seconds (30 frames * 100ms = 3000ms = 3s) */
        if (frames % 30 == 0)
            spawnCar();
        
        /* Update all cars' positions */
        updateCars();
        
        /* Ensure output is displayed and wait 100ms for next frame */
        fflush(stdout);  // Force output to screen
        SLEEP(100);      // Wait 100 milliseconds (0.1 second)
    }
    
    /* Step 4: Simulation completed - show message */
    printf(CLEAR_SCREEN);
    printf(COLOR_GREEN "Simulation completed!\n" COLOR_RESET);
    printf("Press Enter to return to menu...");
    
    /* Clear input buffer and wait for Enter key */
    getchar();  // Clear any leftover newline from previous input
    getchar();  // Wait for user to press Enter
}

/*
 * Function: main
 * Purpose: Program entry point - handles main menu and user interaction
 * Parameters: None (command line arguments not used)
 * Returns: 0 on successful execution
 * Description: Displays menu, gets user choice, runs simulation
 */
int main() {
    /* Initialize random number generator with current time */
    srand(time(NULL));
    
    int choice;     // Store user's menu choice (1, 2, or 3)
    int duration;   // Store custom simulation duration
    
    /* Main program loop - runs until user chooses to exit */
    while (1) {  // Infinite loop (exits with return statement)
        /* Display menu and get user choice */
        displayMenu();          // Show menu screen
        scanf("%d", &choice);   // Read user's choice
        
        /* Process user's choice */
        if (choice == 1) {      // Custom simulation
            printf("\nEnter simulation duration in seconds (1-300): ");
            scanf("%d", &duration);  // Read custom duration
            
            /* Validate input range */
            if (duration < 1) duration = 1;      // Minimum 1 second
            if (duration > 300) duration = 300;  // Maximum 300 seconds
            
            runSimulation(duration);  // Run simulation with custom duration
        }
        else if (choice == 2) {  // Standard 60-second simulation
            runSimulation(60);   // Run simulation for 60 seconds
        }
        else if (choice == 3) {  // Exit program
            return 0;            // Exit main function (ends program)
        }
        else {  // Invalid choice (not 1, 2, or 3)
            printf("\nInvalid choice.\n");  // Show error message
            SLEEP(1500);                    // Wait 1.5 seconds
        }
    }
    
    return 0;  // Program ends successfully (though loop never reaches here)
}
