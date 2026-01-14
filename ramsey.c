#include <stdio.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

clock_t challenge_start;

typedef struct {
    const char *colorCode;   //print the color code to initiate that color
    const char *resetCode;   // always "\033[0m" print to reset to default
} Color;


// colorCode turns text RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE
// resetCode resets text to default color
Color RED     = { "\033[31m", "\033[0m" };
Color GREEN   = { "\033[32m", "\033[0m" };
Color YELLOW  = { "\033[33m", "\033[0m" };
Color BLUE    = { "\033[34m", "\033[0m" };
Color MAGENTA = { "\033[35m", "\033[0m" };
Color CYAN    = { "\033[36m", "\033[0m" };
Color WHITE   = { "\033[37m", "\033[0m" };

// Bright colors
Color BRIGHT_RED     = { "\033[91m", "\033[0m" };
Color BRIGHT_GREEN   = { "\033[92m", "\033[0m" };
Color BRIGHT_YELLOW  = { "\033[93m", "\033[0m" };
Color BRIGHT_BLUE    = { "\033[94m", "\033[0m" };
Color BRIGHT_MAGENTA = { "\033[95m", "\033[0m" };
Color BRIGHT_CYAN    = { "\033[96m", "\033[0m" };
Color BRIGHT_WHITE   = { "\033[97m", "\033[0m" };


typedef enum {
    FLOUR,
    SUGAR,
    YEAST,
    BAKING_SODA,
    SALT,
    CINNAMON,
    EGG,
    MILK,
    BUTTER
} Ingredient;

Ingredient PANTRY_INGREDIENTS[6] = { FLOUR, SUGAR, YEAST, BAKING_SODA, SALT, CINNAMON };
Ingredient FRIDGE_INGREDIENTS[3] = { EGG, MILK, BUTTER };

typedef struct {
    const char *name;
    Ingredient ingredients[10];   // max ingredients per recipe
    int numIngredients;
} Recipe;

#define NUM_RECIPES 5

Recipe RECIPES[NUM_RECIPES] = {
    // Cookies: Flour, Sugar, Milk, Butter
    {
        .name = "Cookies",
        .ingredients = { FLOUR, SUGAR, MILK, BUTTER },
        .numIngredients = 4
    },

    // Pancakes: Flour, Sugar, Baking soda, Salt, Egg, Milk, Butter
    {
        .name = "Pancakes",
        .ingredients = { FLOUR, SUGAR, BAKING_SODA, SALT, EGG, MILK, BUTTER },
        .numIngredients = 7
    },

    // Homemade pizza dough: Yeast, Sugar, Salt
    {
        .name = "Homemade Pizza Dough",
        .ingredients = { YEAST, SUGAR, SALT },
        .numIngredients = 3
    },

    // Soft Pretzels: Flour, Sugar, Salt, Yeast, Baking Soda, Egg
    {
        .name = "Soft Pretzels",
        .ingredients = { FLOUR, SUGAR, SALT, YEAST, BAKING_SODA, EGG },
        .numIngredients = 6
    },

    // Cinnamon Rolls: Flour, Sugar, Salt, Butter, Eggs, Cinnamon
    {
        .name = "Cinnamon Rolls",
        .ingredients = { FLOUR, SUGAR, SALT, BUTTER, EGG, CINNAMON },
        .numIngredients = 6
    }
};

typedef enum{
    WAITING_FOR_INGREDIENTS,
    IN_THE_PANTRY,
    IN_THE_FRIDGE,
    WAITING_FOR_MIXING_TOOLS,
    MIXING_INGREDIENTS,
    WAITING_FOR_OVEN,
    BAKING,
    FINISHED,
    RAMSIED
} BakerState;

const char* bakerStateToString(BakerState state) {
    switch(state) {
        case WAITING_FOR_INGREDIENTS: return "Waiting for Ingredients";
        case IN_THE_PANTRY:           return "In the Pantry";
        case IN_THE_FRIDGE:           return "In the Fridge";
        case WAITING_FOR_MIXING_TOOLS: return "Waiting for Mixing Tools";
        case MIXING_INGREDIENTS:      return "Mixing Ingredients";
        case WAITING_FOR_OVEN:        return "Waiting for Oven";
        case BAKING:                  return "Baking";
        case FINISHED:                return "Finished";
        case RAMSIED:                 return "Getting chewed out by Ramsey";
        default:                      return "Unknown";
    }
}

typedef struct {
    int id;
    Color color;
    BakerState state;
    char *currentRecipe;
    bool ramsied;
} Baker;

//semaphores
typedef struct {
    // Resource semaphores
    sem_t mixer;        // count = 2
    sem_t pantry;       // count = 1
    sem_t refrigerator; // count = 2
    sem_t bowl;         // count = 3
    sem_t spoon;        // count = 5
    sem_t oven;         // count = 1

} Kitchen;

//keep track of the resources that a baker is holding
typedef struct {
    bool hasPantry;
    bool hasFridge;
    bool hasBowl;
    bool hasSpoon;
    bool hasMixer;
    bool hasOven;
} HeldResources;

// Global kitchen pointer for signal handler
Kitchen *global_kitchen = NULL;

// Signal handler for graceful shutdown
void signal_handler(int signum) {
    printf("\n\n Received interrupt signal (Ctrl+C). Cleaning up...\n");

    // Destroy semaphores if kitchen was initialized
    if (global_kitchen != NULL) {
        sem_destroy(&global_kitchen->mixer);
        sem_destroy(&global_kitchen->pantry);
        sem_destroy(&global_kitchen->refrigerator);
        sem_destroy(&global_kitchen->bowl);
        sem_destroy(&global_kitchen->spoon);
        sem_destroy(&global_kitchen->oven);
        printf("✓ Semaphores destroyed\n");
    }

    printf("Exiting program.\n");
    exit(0);
}

void colorPrintf(Color *color, const char *format, ...)
{
    va_list args;

    // set text color
    printf("%s", color->colorCode);

    // print the formatted text
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    // reset color
    printf("%s", color->resetCode);
}

void performAction(const char* action) {
    if (strcmp(action, "grab ingredients") == 0) {
        sleep(1); // Simulate time taken to grab ingredients
    }
    else if (strcmp(action, "mix ingredients") == 0) {
        sleep(1); // Simulate mixing time
    }
    else if (strcmp(action, "bake") == 0) {
        sleep(1); // Simulate baking time
    }
}

bool inPantry (Ingredient ingredient){
    for (int i = 0; i < 6; i++){
        if(PANTRY_INGREDIENTS[i] == ingredient){return true;}
    }
    return false;
}
bool inFridge (Ingredient ingredient){
    for (int i = 0; i < 3; i++){
        if(FRIDGE_INGREDIENTS[i] == ingredient){return true;}
    }
    return false;
}

// Struct passed into each baker thread
typedef struct {
    Baker *baker;
    Kitchen *kitchen;
    double ramsiedChance;
} BakerThreadArgs;


// Release all held resources
void releaseAllResources(Kitchen *kitchen, HeldResources *held) {
    if (held->hasPantry) {
        sem_post(&kitchen->pantry);
        held->hasPantry = false;
    }
    if (held->hasFridge) {
        sem_post(&kitchen->refrigerator);
        held->hasFridge = false;
    }
    if (held->hasBowl) {
        sem_post(&kitchen->bowl);
        held->hasBowl = false;
    }
    if (held->hasSpoon) {
        sem_post(&kitchen->spoon);
        held->hasSpoon = false;
    }
    if (held->hasMixer) {
        sem_post(&kitchen->mixer);
        held->hasMixer = false;
    }
    if (held->hasOven) {
        sem_post(&kitchen->oven);
        held->hasOven = false;
    }
}
bool checkAndHandleRamsied(Baker *baker, Kitchen *kitchen, HeldResources *held,
                        double ramsiedChance) {

    if(!baker->ramsied && ((double) rand() / RAND_MAX) < ramsiedChance){

        releaseAllResources(kitchen, held);
        baker->state = RAMSIED;

        //TODO:output chef ramsey yelling at the baker.
        printf("\n");
        colorPrintf(&baker->color, "CHEF %d", baker->id);
        printf(" YOU BLOODY TWAT! THIS IS DISGRACEFUL! START OVER!\n");

        colorPrintf(&baker->color, "YES CHEF!");

        return true;  // Indicates: should restart/continue
    }
    return false;  // Indicates: keep going normally
}
void printBakerTable(Baker *bakers, int numBakers, Kitchen *kitchen) {//this method was created using ChatGPT

    // Get semaphore values
    int mix_avail, pan_avail, ref_avail, bowl_avail, spoon_avail, oven_avail;
    sem_getvalue(&kitchen->mixer,        &mix_avail);
    sem_getvalue(&kitchen->pantry,       &pan_avail);
    sem_getvalue(&kitchen->refrigerator, &ref_avail);
    sem_getvalue(&kitchen->bowl,         &bowl_avail);
    sem_getvalue(&kitchen->spoon,        &spoon_avail);
    sem_getvalue(&kitchen->oven,         &oven_avail);

    printf("\n##########################################%.2f###################################################\n", (double)(clock() - challenge_start) / CLOCKS_PER_SEC);


    // ────────── Header ──────────
    printf("╔════════╦══════════════════════════╦═══════════════════════╗  ╔═════════════════════════════╗\n");
    printf("║ Baker  ║       State              ║  Current Recipe       ║  ║ Kitchen Resources Available ║\n");
    printf("╠════════╬══════════════════════════╬═══════════════════════╣  ╠═════════════════════════════╣\n");

    // Number of kitchen resources
    int numResources = 6 + 1;
    int maxRows = (numBakers > numResources) ? numBakers : numResources;

    for (int i = 0; i < maxRows; i++) {
        // ── Baker table ──
        if (i < numBakers) {
            const char* recipeName = (bakers[i].currentRecipe != NULL) ? bakers[i].currentRecipe : "None";

            printf("║ ");
            colorPrintf(&bakers[i].color, "  %2d", bakers[i].id);
            printf("   ║ ");
            colorPrintf(&bakers[i].color, "%-24s", bakerStateToString(bakers[i].state));
            printf(" ║ ");
            colorPrintf(&bakers[i].color, "%-21s", recipeName);
            printf(" ║  ");
        }else if (i == numBakers){
            printf("╚════════╩══════════════════════════╩═══════════════════════╝  ");
        }else {
            printf("                                                               ");
        }

        // ── Kitchen resource table ──
        switch(i) {
            case 2:
                printf("║ Mixer       : %d / 2\t     ║\n", mix_avail); break;
            case 0:
                printf("║ Pantry      : %d / 1\t     ║\n", pan_avail); break;
            case 1:
                printf("║ Refrigerator: %d / 2\t     ║\n", ref_avail); break;
            case 3:
                printf("║ Bowl        : %d / 3\t     ║\n", bowl_avail); break;
            case 4:
                printf("║ Spoon       : %d / 5\t     ║\n", spoon_avail); break;
            case 5:
                printf("║ Oven        : %d / 1\t     ║\n", oven_avail); break;
            case 6:
                printf("╚═════════════════════════════╝\n"); break;
            default:
                printf("\n"); break;
        }
    }

    // ────────── Footer ──────────
    //printf("#############################################################################################\n");
}



void *bakerThread(void *arg) {
    // unpack the contents from the argument intiated by the thread call
    BakerThreadArgs *args = (BakerThreadArgs*)arg;

    Baker *baker = args->baker;
    Kitchen *kitchen = args->kitchen;
    double ramsiedChance = args->ramsiedChance;

    HeldResources held;
    bool ramsied = ramsiedChance == 0.0 ? true : false;


    for (int r = 0; r < NUM_RECIPES; r++){
        held.hasPantry = false;
        held.hasFridge = false;
        held.hasBowl = false;
        held.hasSpoon = false;
        held.hasMixer = false;
        held.hasOven = false;

        Recipe recipe = RECIPES[r];

        // Set the current recipe name
        baker->currentRecipe = (char*)recipe.name;


        int pantryIngredientsNeeded = 0;
        int fridgeIngredientsNeeded = 0;
        for(int i=0; i<recipe.numIngredients; i++){
            if(inPantry(recipe.ingredients[i])){pantryIngredientsNeeded++;}
            if(inFridge(recipe.ingredients[i])){fridgeIngredientsNeeded++;}
        }

        //try to aquire each ingredient 1 at a time
        baker->state = WAITING_FOR_INGREDIENTS;
        while (pantryIngredientsNeeded > 0 || fridgeIngredientsNeeded > 0) {
            if (pantryIngredientsNeeded > 0 && sem_trywait(&kitchen->pantry) == 0) {
                held.hasPantry = true;
                baker->state = IN_THE_PANTRY;
                performAction("grab ingredients");
                sem_post(&kitchen->pantry); //release semaphore
                held.hasPantry = false;
                pantryIngredientsNeeded--;
                continue;  // retry fridge next
            }

            if (fridgeIngredientsNeeded > 0 && sem_trywait(&kitchen->refrigerator) == 0) {
                held.hasFridge = true;
                baker->state = IN_THE_FRIDGE;
                performAction("grab ingredients");
                sem_post(&kitchen->refrigerator);//release semaphore
                held.hasFridge = false;
                fridgeIngredientsNeeded--;
                continue;
            }
            baker->state = WAITING_FOR_INGREDIENTS;
            usleep(1000); //prevent 100% memory usage
        }
        //acquire bowl + spoon + mixer semaphores
        baker->state = WAITING_FOR_MIXING_TOOLS;
        while(!(held.hasBowl && held.hasSpoon && held.hasMixer)){
            if(!held.hasBowl && sem_trywait(&kitchen->bowl) == 0){held.hasBowl = true;}
            if(!held.hasSpoon && sem_trywait(&kitchen->spoon) == 0){held.hasSpoon = true;}
            if(!held.hasMixer && sem_trywait(&kitchen->mixer) == 0){held.hasMixer = true;}
            usleep(1000);//prevent 100% memory usage
        }

        //mix ingredients
        baker->state = MIXING_INGREDIENTS;
        performAction("mix ingredients");
        if(checkAndHandleRamsied(baker, kitchen, &held, ramsiedChance)){
            baker->ramsied = true;
            r--;
            continue;
        }

        //release bowl + spoon + mixer semaphores
        sem_post(&kitchen->bowl);
        sem_post(&kitchen->spoon);
        sem_post(&kitchen->mixer);

        held.hasBowl = false;
        held.hasSpoon = false;
        held.hasMixer = false;

        //acquire oven semaphore
        baker->state = WAITING_FOR_OVEN;
        sem_wait(&kitchen->oven);
        held.hasOven = true;

        //bake
        baker->state = BAKING;
        performAction("bake");
        if(checkAndHandleRamsied(baker, kitchen, &held, ramsiedChance)){
            baker->ramsied = true;
            r--;
            continue;
        }

        //release oven semaphore
        sem_post(&kitchen->oven);
        held.hasOven = false;
    }

    // Mark as complete
    baker->currentRecipe = "Complete";
    baker->state = FINISHED;
    return NULL;

}


int main(int argc, char *argv[]){

    //init variables

    Color COLORS[] = {
        RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE,
        BRIGHT_RED, BRIGHT_GREEN, BRIGHT_YELLOW, BRIGHT_BLUE, BRIGHT_MAGENTA, BRIGHT_CYAN, BRIGHT_WHITE
    };

    int NUM_COLORS = sizeof(COLORS)/sizeof(Color);
    int NUM_BAKERS;

    // Initialize Kitchen struct
    Kitchen kitchen;

    // Initialize each semaphore with its initial count
    sem_init(&kitchen.mixer, 0, 2);        // 2 mixers
    sem_init(&kitchen.pantry, 0, 1);       // 1 pantry
    sem_init(&kitchen.refrigerator, 0, 2); // 2 fridges
    sem_init(&kitchen.bowl, 0, 3);         // 3 bowls
    sem_init(&kitchen.spoon, 0, 5);        // 5 spoons
    sem_init(&kitchen.oven, 0, 1);         // 1 oven

    // Set global pointer for signal handler
    global_kitchen = &kitchen;

    // Register signal handler for Ctrl+C (SIGINT)
    signal(SIGINT, signal_handler);
    printf("✓ Signal handler registered (press Ctrl+C to exit gracefully)\n\n");

    //input number of bakers
    int numBakers;
    Baker bakers[20]; //max 20 bakers

    if (argc == 2) {
        numBakers = atoi(argv[1]);  // atoi = ASCII to int
    }else {
        printf("How many bakers are involved in this challenge?\n");
        scanf("%d", &numBakers);
    }
    while (numBakers <= 0){
        printf("please input a valid number > 0\n");
        scanf("%d", &numBakers);
    }
    NUM_BAKERS = numBakers;
    printf("NUM_BAKERS: %d\n", NUM_BAKERS);

    for (int i = 0; i < NUM_BAKERS; i++){
        bakers[i].id = i;
        bakers[i].color = COLORS[i % NUM_COLORS];
        bakers[i].state = WAITING_FOR_INGREDIENTS;
        bakers[i].currentRecipe = NULL;  // Initialize to NULL
    }


    //start threads
    pthread_t threads[NUM_BAKERS];
    BakerThreadArgs threadArgs[NUM_BAKERS];
    challenge_start = clock();
    for(int i = 0; i<NUM_BAKERS; i++)
    {
        threadArgs[i].baker = &bakers[i];
        threadArgs[i].kitchen = &kitchen;
        threadArgs[i].ramsiedChance = 0.1; //10% chance
        pthread_create(&threads[i], NULL, bakerThread, &threadArgs[i]);
    }

    //monitoring the bakers
    Baker winners[NUM_BAKERS];
    int numFinished = 0;
    while(1){
        bool allFinished = true;
        for(int i = 0; i < NUM_BAKERS; i++){
            if(bakers[i].state == FINISHED){
                bool alreadyFinished = false;
                for(int j = 0; j < numFinished; j++){
                    if(winners[j].id == bakers[i].id){
                        alreadyFinished = true;
                        break;
                    }
                }
                if(!alreadyFinished){
                    colorPrintf(&bakers[i].color, "Baker %d has finished\n", i);
                    pthread_join(threads[i], NULL);
                    winners[numFinished] = bakers[i];
                    numFinished++;
                }
            }
            else{
                allFinished = false;
            }
        }
        if(allFinished){
            break;
        }

        //print all baker states
        printBakerTable(bakers, numBakers, global_kitchen);
        usleep(500 * 1000);
    }

    //print the winners of the event
    printf("\n🎉 ALL BAKERS FINISHED! 🎉\n\n");
    printf("Final Results:\n");
    for (int i = 0; i < numFinished; i++) {
        printf("%d. ", i + 1);
        colorPrintf(&winners[i].color, "Baker %d", winners[i].id);
        printf("\n");
    }
    printf("\n");

    // Clean up semaphores
    sem_destroy(&kitchen.mixer);
    sem_destroy(&kitchen.pantry);
    sem_destroy(&kitchen.refrigerator);
    sem_destroy(&kitchen.bowl);
    sem_destroy(&kitchen.spoon);
    sem_destroy(&kitchen.oven);
    global_kitchen = NULL;  // Mark as destroyed

    printf("✓ Cleanup complete\n");

    return 0;
}
