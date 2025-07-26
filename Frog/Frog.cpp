#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curses.h>

// Fixed upper limits for cars and obstacles
#define MAX_CARS 100
#define MAX_OBSTACLES 10

// Structure to store configuration data
typedef struct {
	int WIDTH, HEIGHT, CAR_CNT, TOTAL_TIME, MAX_SPEED, OBSTACLE_CNT; // cnt: count
    char CAR, FROG, GOAL, OBSTACLE;
	int FROG_CLR, CAR_CLR, GOAL_CLR, OBSTACLE_CLR; // color pairs
    unsigned int RANDOM_SEED;
	int FRCAR_PERC; // percentage of friendly cars
} Config;


typedef struct {
	int x, y; // location
    int speed;
	int canStop; // controls if the car can stop for the frog
    int isFriendly; 
} Car;


typedef struct {
    int x, y;
} Frog;


typedef struct {
	int x, y; 
} Obstacle;

// Function prototypes
void init_ui(Config* cfg); // launches the user interface and configures color schemes.
void loadConfig(const char* filename, Config* cfg);
void set_cfg_value(Config* cfg, const char* key, const char* value); // sets the configuration values
void initCars(Car cars[], int carCount, Obstacle obstacles[], int obstacleCount, const Config* cfg); // determines random but valid starting positions of cars on the playing field.
void initObstacles(Obstacle obstacles[], int obstacleCount, Frog* frog, const Config* cfg); // it starts the starting positions of the obstacles without overlapping the frog.
void draw(Car cars[], Frog* frog, Obstacle obstacles[], int obstacleCount, int timeLeft, const Config* cfg);
void moveFrog(Frog* frog, int ch, int width, int height);
void moveCars(Car cars[], int carCount, Frog* frog, Obstacle obstacles[], int obstacleCount, const Config* cfg); 
int collision(Car cars[], Frog* frog, int carCount, Obstacle obstacles[], int obstacleCount);
void EndGame(const char* message);
void sleepMilliseconds(int milliseconds); // used to provide delay between cycles
void relocateFrog(Frog* frog, Obstacle obstacles[], int obstacleCount, Car cars[], int carCount, const Config* cfg); // moves the frog to a new location after a collision with a friendly car.


int main() {
    Config cfg;
    loadConfig("config.txt", &cfg);

    Car cars[MAX_CARS];
    Obstacle obstacles[MAX_OBSTACLES]; // empty arrays are defined within the limits of MAX_CARS and MAX_OBSTACLES.
    if (cfg.CAR_CNT > MAX_CARS) {
        printf("Error:value exceeds MAX_CARS.\n");
        return 1;
    }
    if (cfg.OBSTACLE_CNT > MAX_OBSTACLES) {
        printf("Error:value exceeds MAX_OBSTACLES.\n");
        return 1;
    }

    Frog frog = { cfg.WIDTH / 2, cfg.HEIGHT - 1 }; // sets the frog's starting position on the playfield
    clock_t startTime = clock();
    double timeLeft = cfg.TOTAL_TIME;
    int ch;

	srand(cfg.RANDOM_SEED); // random seed is set
    initCars(cars, cfg.CAR_CNT, obstacles, cfg.OBSTACLE_CNT, &cfg);
    initObstacles(obstacles, cfg.OBSTACLE_CNT, &frog, &cfg);

    init_ui(&cfg);


    while (timeLeft > 0) {
        clock_t currentTime = clock();
        double elapsedTime = (double)(currentTime - startTime) / CLOCKS_PER_SEC;
        timeLeft = cfg.TOTAL_TIME - elapsedTime;

        draw(cars, &frog, obstacles, cfg.OBSTACLE_CNT, (int)timeLeft, &cfg);

		ch = getch(); // gets the user input
        moveFrog(&frog, ch, cfg.WIDTH, cfg.HEIGHT);

        moveCars(cars, cfg.CAR_CNT, &frog, obstacles, cfg.OBSTACLE_CNT, &cfg);

        if (collision(cars, &frog, cfg.CAR_CNT, obstacles, cfg.OBSTACLE_CNT)) {
            EndGame("GAME OVER!");
            return 0;
        }

		if (frog.y == 0) { // if the frog reaches the goal
            EndGame("YOU WIN!");
            return 0;
        }
    }

    EndGame("TIME IS UP!!");
    return 0;
}

void init_ui(Config* cfg) {

    initscr(); // takes control of the terminal and switches to ncurses mode
    start_color();
    noecho(); // does not display user typed characters in terminal
	curs_set(FALSE); // hides the cursor
	keypad(stdscr, TRUE); // enables the use of special keys such as arrow keys

    // init_pair(pair_number, foreground_color, background_color)
    init_pair(1, cfg->FROG_CLR, COLOR_BLACK);
    init_pair(2, cfg->CAR_CLR, COLOR_BLACK);
    init_pair(3, cfg->GOAL_CLR, COLOR_BLACK);
    init_pair(4, cfg->OBSTACLE_CLR, COLOR_BLACK);

    timeout(50); 

}

void moveFrog(Frog* frog, int ch, int width, int height) { // moves the frog in the direction of user input (ch).
    if (ch == KEY_UP && frog->y > 0) frog->y--; // the y coordinate of the frog is decreased by 1 
	if (ch == KEY_DOWN && frog->y < height - 1) frog->y++; // the y coordinate of the frog is increased by 1
	if (ch == KEY_LEFT && frog->x > 0) frog->x--; // the x coordinate of the frog is decreased by 1
	if (ch == KEY_RIGHT && frog->x < width - 1) frog->x++; // the x coordinate of the frog is increased by 1
}



void loadConfig(const char* filename, Config* cfg) {
    FILE* file;
	if (fopen_s(&file, filename, "r") != 0) { // opens the configuration file
        perror("Configration file could not open.");
        exit(EXIT_FAILURE);
    }

    char line[100];
	while (fgets(line, sizeof(line), file)) { // reads the configuration file line by line
        char key[50], value[50];
        if (sscanf_s(line, "%[^=]=%s", key, (unsigned)_countof(key), value, (unsigned)_countof(value)) == 2) {
			set_cfg_value(cfg, key, value); // sets the configuration values, key=value on cfg
        }
    }

	fclose(file); 
}

void set_cfg_value(Config* cfg, const char* key, const char* value) {
    if (!strcmp(key, "WIDTH")) cfg->WIDTH = atoi(value);
    else if (!strcmp(key, "HEIGHT")) cfg->HEIGHT = atoi(value);
    else if (!strcmp(key, "CAR_CNT")) cfg->CAR_CNT = atoi(value);
    else if (!strcmp(key, "TOTAL_TIME")) cfg->TOTAL_TIME = atoi(value);
    else if (!strcmp(key, "MAX_SPEED")) cfg->MAX_SPEED = atoi(value);
    else if (!strcmp(key, "OBSTACLE_CNT")) cfg->OBSTACLE_CNT = atoi(value);
    else if (!strcmp(key, "CAR")) cfg->CAR = value[0];
    else if (!strcmp(key, "FROG")) cfg->FROG = value[0];
    else if (!strcmp(key, "GOAL")) cfg->GOAL = value[0];
    else if (!strcmp(key, "OBSTACLE")) cfg->OBSTACLE = value[0];
    else if (!strcmp(key, "FROG_CLR")) cfg->FROG_CLR = atoi(value);
    else if (!strcmp(key, "CAR_CLR")) cfg->CAR_CLR = atoi(value);
    else if (!strcmp(key, "GOAL_CLR")) cfg->GOAL_CLR = atoi(value);
    else if (!strcmp(key, "OBSTACLE_CLR")) cfg->OBSTACLE_CLR = atoi(value);
    else if (!strcmp(key, "RANDOM_SEED")) cfg->RANDOM_SEED = atoi(value);
    else if (!strcmp(key, "FRCAR_PERC")) cfg->FRCAR_PERC = atoi(value);
}




void initCars(Car cars[], int carCount, Obstacle obstacles[], int obstacleCount, const Config* cfg) {
    for (int i = 0; i < carCount; i++) {
        int valid = 0;
        while (!valid) {
            cars[i].x = rand() % cfg->WIDTH;
            cars[i].y = rand() % (cfg->HEIGHT - 2) + 1;
            cars[i].speed = (rand() % cfg->MAX_SPEED) + 1;
            cars[i].canStop = rand() % 2; 
            cars[i].isFriendly = (rand() % 100 < cfg->FRCAR_PERC) ? 1 : 0;

            valid = 1;
            for (int j = 0; j < obstacleCount; j++) {
                if (cars[i].y == obstacles[j].y) {
                    valid = 0;
                    break;
                }
            }
        }
    }
}

void moveCars(Car cars[], int carCount, Frog* frog, Obstacle obstacles[], int obstacleCount, const Config* cfg) {
    static int timeCounter = 0;
    timeCounter++;

    for (int i = 0; i < carCount; i++) {
        int oldX = cars[i].x;

        if (cars[i].canStop && cars[i].y == frog->y && cars[i].x > frog->x - 2 && cars[i].x < frog->x) {
            continue;
        }

        cars[i].x += cars[i].speed; // Updates its position according to the speed of the car

        if (timeCounter % 50 == 0) { 
            cars[i].speed = (rand() % cfg->MAX_SPEED) + 1;
        }

        if (cars[i].x >= cfg->WIDTH) {
            cars[i].x = 0; 
            int valid = 0; 
            while (!valid) { // The statement tries an operation repeatedly until the valid variable is true (1).
                cars[i].y = rand() % (cfg->HEIGHT - 2) + 1; 
                valid = 1;
                for (int j = 0; j < obstacleCount; j++) { 
                    if (cars[i].y == obstacles[j].y) { 
                        valid = 0; 
                        break;
                    }
                }
            }
            cars[i].speed = (rand() % cfg->MAX_SPEED) + 1; 
        }

        for (int x = oldX; x <= cars[i].x; x++) {
            if (x == frog->x && cars[i].y == frog->y) {
                if (cars[i].isFriendly) {
                    relocateFrog(frog, obstacles, obstacleCount, cars, carCount, cfg);
                }
                else {
                    EndGame("GAME OVER!");
                    exit(0);
                }
            }
        }
    }
}

void relocateFrog(Frog* frog, Obstacle obstacles[], int obstacleCount, Car cars[], int carCount, const Config* cfg) {
    int valid = 0;
    while (!valid) {
        frog->x = rand() % cfg->WIDTH;
        frog->y = rand() % cfg->HEIGHT;
        valid = 1;

        for (int i = 0; i < obstacleCount; i++) {
            if (frog->x == obstacles[i].x && frog->y == obstacles[i].y) {
                valid = 0;
                break;
            }
        }

        for (int i = 0; i < carCount; i++) {
            if (frog->x == cars[i].x && frog->y == cars[i].y) {
                valid = 0;
                break;
            }
        }
    }
}

void initObstacles(Obstacle obstacles[], int obstacleCount, Frog* frog, const Config* cfg) {
    for (int i = 0; i < obstacleCount; i++) {
        do {
            obstacles[i].x = rand() % cfg->WIDTH;
            obstacles[i].y = rand() % (cfg->HEIGHT - 2) + 1;
        } while (obstacles[i].x == frog->x && obstacles[i].y == frog->y);
    }
}

void draw(Car cars[], Frog* frog, Obstacle obstacles[], int obstacleCount, int timeLeft, const Config* cfg) {
    clear();
    
    // mvprintw(row, col, format_char, variables);
    attron(COLOR_PAIR(3));
    mvprintw(0, cfg->WIDTH / 2, "%c", cfg->GOAL);
    attroff(COLOR_PAIR(3));

    for (int i = 0; i < cfg->CAR_CNT; i++) {
        attron(COLOR_PAIR(2));
        mvprintw(cars[i].y, cars[i].x, "%c", cfg->CAR);
        attroff(COLOR_PAIR(2));
    }

    for (int i = 0; i < obstacleCount; i++) {
        attron(COLOR_PAIR(4));
        mvprintw(obstacles[i].y, obstacles[i].x, "%c", cfg->OBSTACLE);
        attroff(COLOR_PAIR(4));
    }

    attron(COLOR_PAIR(1));
    mvprintw(frog->y, frog->x, "%c", cfg->FROG);
    attroff(COLOR_PAIR(1));

    mvprintw(cfg->HEIGHT, 0, "Time Left: %d", timeLeft);
    refresh();
}

int collision(Car cars[], Frog* frog, int carCount, Obstacle obstacles[], int obstacleCount) {
    for (int i = 0; i < carCount; i++) {
        if (cars[i].x == frog->x && cars[i].y == frog->y) return 1;
    }

    for (int i = 0; i < obstacleCount; i++) {
        if (obstacles[i].x == frog->x && obstacles[i].y == frog->y) return 1;
    }

    return 0;
}

void EndGame(const char* message) {
    clear();
    mvprintw(10, 20, "%s", message);
    refresh();
    sleepMilliseconds(2000);
    endwin();
}

void sleepMilliseconds(int milliseconds) {
    clock_t startTime = clock();
    while (clock() < startTime + milliseconds * (CLOCKS_PER_SEC / 1000));
}