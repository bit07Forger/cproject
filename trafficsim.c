#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// ANSI color codes for terminal output
#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define BOLD "\033[1m"
#define BG_RED "\033[41m"
#define BG_GREEN "\033[42m"
#define BG_YELLOW "\033[43m"
#define BG_BLACK "\033[40m"

// Traffic light states
typedef enum {
    LIGHT_RED,
    LIGHT_YELLOW,
    LIGHT_GREEN
} TrafficLight;

// Car structure
typedef struct {
    int position;
    char color[20];
    int speed;
    int active;
} Car;

// Function to clear screen (cross-platform)
void clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

// Function to draw traffic light
void drawTrafficLight(TrafficLight state) {
    printf("\n");
    printf("        |=========|\n");
    printf("        |         |\n");
    
    if (state == LIGHT_RED)
        printf("        |  %s● RED %s |\n", BG_RED, RESET);
    else
        printf("        |  %s●%s     |\n", RED, RESET);
    
    printf("        |         |\n");
    
    if (state == LIGHT_YELLOW)
        printf("        │ %s● YELLOW%s│\n", BG_YELLOW, RESET);
    else
        printf("        │  %s●%s     │\n", YELLOW, RESET);
    
    printf("        │         │\n");
    
    if (state == LIGHT_GREEN)
        printf("        │ %s● GREEN%s │\n", BG_GREEN, RESET);
    else
        printf("        │  %s●%s     │\n", GREEN, RESET);
    
    printf("        |==============|\n");
    printf("        |================|\n");
}

// Function to draw road with cars
void drawRoad(Car cars[], int numCars, int roadLength) {
    printf("\n");
    printf("==================================================================\n");
    
    // Create road string
    char road[roadLength + 1];
    memset(road, ' ', roadLength);
    road[roadLength] = '\0';
    
    // Place cars on road
    for (int i = 0; i < numCars; i++) {
        if (cars[i].active && cars[i].position >= 0 && cars[i].position < roadLength - 3) {
            road[cars[i].position] = 'C';
            road[cars[i].position + 1] = 'A';
            road[cars[i].position + 2] = 'R';
        }
    }
    
    // Draw road with colored cars
    printf("║ ");
    for (int i = 0; i < roadLength; i++) {
        if (road[i] == 'C' && i + 2 < roadLength && road[i+1] == 'A' && road[i+2] == 'R') {
            // Find which car this is
            for (int j = 0; j < numCars; j++) {
                if (cars[j].position == i && cars[j].active) {
                    printf("%s[CAR]%s", cars[j].color, RESET);
                    i += 4; // Skip the next characters
                    break;
                }
            }
        } else if (road[i] != 'A' && road[i] != 'R') {
            printf("%c", road[i]);
        }
    }
    printf(" ║\n");
    
    printf("================================================================\n");
}

// Function to update car positions
void updateCars(Car cars[], int numCars, TrafficLight light, int roadLength) {
    int stopPosition = roadLength - 15; // Position before traffic light
    
    for (int i = 0; i < numCars; i++) {
        if (!cars[i].active) continue;
        
        // Check if car should stop at red/yellow light
        if ((light == LIGHT_RED || light == LIGHT_YELLOW) && 
            cars[i].position >= stopPosition - 5 && 
            cars[i].position < stopPosition) {
            continue; // Car stops
        }
        
        // Move car forward
        cars[i].position += cars[i].speed;
        
        // Remove car if it goes past the road
        if (cars[i].position >= roadLength) {
            cars[i].active = 0;
        }
    }
}

// Function to spawn new car
void spawnCar(Car cars[], int numCars) {
    for (int i = 0; i < numCars; i++) {
        if (!cars[i].active) {
            cars[i].position = 0;
            cars[i].active = 1;
            cars[i].speed = 1 + rand() % 2;
            
            // Random car color
            int colorChoice = rand() % 6;
            switch (colorChoice) {
                case 0: strcpy(cars[i].color, RED); break;
                case 1: strcpy(cars[i].color, GREEN); break;
                case 2: strcpy(cars[i].color, BLUE); break;
                case 3: strcpy(cars[i].color, YELLOW); break;
                case 4: strcpy(cars[i].color, MAGENTA); break;
                case 5: strcpy(cars[i].color, CYAN); break;
            }
            break;
        }
    }
}

int main() {
    srand(time(NULL));
    
    const int NUM_CARS = 5;
    const int ROAD_LENGTH = 60;
    Car cars[NUM_CARS];
    
    // Initialize cars
    for (int i = 0; i < NUM_CARS; i++) {
        cars[i].active = 0;
        cars[i].position = 0;
        cars[i].speed = 1;
    }
    
    TrafficLight currentLight = LIGHT_GREEN;
    int timeCounter = 0;
    int nextLightChange = 5 + rand() % 8; // Random time between 5-12 seconds
    int carSpawnCounter = 0;
    
    printf("%s%sTraffic Light Simulation%s\n", BOLD, CYAN, RESET);
    printf("Press Ctrl+C to exit\n");
    sleep(2);
    
    while (1) {
        clearScreen();
        
        printf("%s%s=== TRAFFIC LIGHT SIMULATION ===%s\n", BOLD, CYAN, RESET);
        printf("Time: %d seconds\n", timeCounter);
        printf("Next light change in: %d seconds\n", nextLightChange - timeCounter);
        
        // Draw traffic light
        drawTrafficLight(currentLight);
        
        // Draw road with cars
        drawRoad(cars, NUM_CARS, ROAD_LENGTH);
        
        printf("\n%sActive Cars:%s ", BOLD, RESET);
        int activeCars = 0;
        for (int i = 0; i < NUM_CARS; i++) {
            if (cars[i].active) activeCars++;
        }
        printf("%d\n", activeCars);
        
        // Update car positions
        updateCars(cars, NUM_CARS, currentLight, ROAD_LENGTH);
        
        // Spawn new car randomly
        carSpawnCounter++;
        if (carSpawnCounter >= 3 && activeCars < NUM_CARS) {
            if (rand() % 3 == 0) { // 33% chance
                spawnCar(cars, NUM_CARS);
            }
            carSpawnCounter = 0;
        }
        
        // Change traffic light at random intervals
        if (timeCounter >= nextLightChange) {
            if (currentLight == LIGHT_GREEN) {
                currentLight = LIGHT_YELLOW;
                nextLightChange = timeCounter + 2 + rand() % 2; // Yellow: 2-3 seconds
            } else if (currentLight == LIGHT_YELLOW) {
                currentLight = LIGHT_RED;
                nextLightChange = timeCounter + 5 + rand() % 6; // Red: 5-10 seconds
            } else {
                currentLight = LIGHT_GREEN;
                nextLightChange = timeCounter + 7 + rand() % 8; // Green: 7-14 seconds
            }
        }
        
        timeCounter++;
        sleep(1); // Wait 1 second
    }
    
    return 0;
}
// test text to check 