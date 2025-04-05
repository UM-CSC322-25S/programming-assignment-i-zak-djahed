#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Max number of boats allowed
#define MAX_BOATS 120
// Max length for a boat's name
#define MAX_NAME 128
// Max length for input lines
#define MAX_LINE 256

// Enumeration for boat types
typedef enum {
    SLIP,
    LAND,
    TRAILOR,
    STORAGE
} BoatType;

// Structure for each boat
typedef struct {
    char name[MAX_NAME];
    int length; // measured in feet, max is 100
    BoatType type;
    union {
        int slipNumber;     // slip number (1-85)
        char bay;           // bay letter (A-Z) for land boats
        char *trailerTag;   // trailer license tag (dynamically allocated)
        int storageNumber;  // storage space number (1-50)
    } info;
    double amountOwed;
} Boat;

// Array of pointers to boats and current count
Boat *boats[MAX_BOATS];
int boatCount = 0;

// Function prototypes
void loadBoatData(const char *filename);
void saveBoatData(const char *filename);
void printInventory(void);
void addBoat(const char *csvLine);
void removeBoat(void);
void acceptPayment(void);
void updateMonthlyFees(void);
BoatType getBoatTypeFromString(const char *typeStr);
const char *boatTypeToString(BoatType type);
int findBoatIndexByName(const char *name);
void insertBoatSorted(Boat *newBoat);
void freeBoat(Boat *b);

// Helper function: Trim leading and trailing whitespace (in place)
void trimWhitespace(char *str) {
    char *end;
    // Trim leading spaces
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return;
    // Trim trailing spaces
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    *(end+1) = 0;
}

// Helper function: Case-insensitive string comparison wrapper
int caseInsensitiveCompare(const char *a, const char *b) {
    return strcasecmp(a, b);
}

// Load boat data from CSV file
void loadBoatData(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        // If file doesn't exist, start with an empty inventory.
        return;
    }
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp) != NULL) {
        // Remove any trailing newline
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0)
            continue;
        // Use addBoat unction to parse the CSV string and then insert it
        addBoat(line);
    }
    fclose(fp);
}

// Save boat data to a CSV file and also print the saved data as a sample
void saveBoatData(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("Error opening file %s for writing.\n", filename);
        return;
    }
    for (int i = 0; i < boatCount; i++) {
        Boat *b = boats[i];
        // Write in CSV: name, length, type, specific,amountOwed
        switch(b->type) {
            case SLIP:
                fprintf(fp, "%s,%d,slip,%d,%.2f\n", b->name, b->length, b->info.slipNumber, b->amountOwed);
                break;
            case LAND:
                fprintf(fp, "%s,%d,land,%c,%.2f\n", b->name, b->length, b->info.bay, b->amountOwed);
                break;
            case TRAILOR:
                fprintf(fp, "%s,%d,trailor,%s,%.2f\n", b->name, b->length, b->info.trailerTag, b->amountOwed);
                break;
            case STORAGE:
                fprintf(fp, "%s,%d,storage,%d,%.2f\n", b->name, b->length, b->info.storageNumber, b->amountOwed);
                break;
        }
    }
    fclose(fp);

    // Also print the CSV content:
    printf("\nExiting the Boat Management System\n");
    printf("Here's what the saved .csv file could look like:\n");
    for (int i = 0; i < boatCount; i++) {
        Boat *b = boats[i];
        switch(b->type) {
            case SLIP:
                printf("%s,%d,slip,%d,%.2f\n", b->name, b->length, b->info.slipNumber, b->amountOwed);
                break;
            case LAND:
                printf("%s,%d,land,%c,%.2f\n", b->name, b->length, b->info.bay, b->amountOwed);
                break;
            case TRAILOR:
                printf("%s,%d,trailor,%s,%.2f\n", b->name, b->length, b->info.trailerTag, b->amountOwed);
                break;
            case STORAGE:
                printf("%s,%d,storage,%d,%.2f\n", b->name, b->length, b->info.storageNumber, b->amountOwed);
                break;
        }
    }
}

// Print the boat inventory (sorted by the names of the boats)
void printInventory(void) {
    for (int i = 0; i < boatCount; i++) {
        Boat *b = boats[i];
        printf("%-20s %2d' ", b->name, b->length);
        switch(b->type) {
            case SLIP:
                printf("   slip   # %2d", b->info.slipNumber);
                break;
            case LAND:
                printf("   land      %c", b->info.bay);
                break;
            case TRAILOR:
                printf(" trailor %s", b->info.trailerTag);
                break;
            case STORAGE:
                printf(" storage   # %2d", b->info.storageNumber);
                break;
        }
        printf("   Owes $%8.2f\n", b->amountOwed);
    }
}

// Insert a boat into the array maintaining sorted order by name (case-insensitive)
void insertBoatSorted(Boat *newBoat) {
    int pos = 0;
    while (pos < boatCount && caseInsensitiveCompare(boats[pos]->name, newBoat->name) < 0)
        pos++;
    // Shift boats to make room
    for (int i = boatCount; i > pos; i--) {
        boats[i] = boats[i-1];
    }
    boats[pos] = newBoat;
    boatCount++;
}

// Parse CSV string and add a boat to the inventory
// CSV format: name,length,type,specific,amountOwed
void addBoat(const char *csvLine) {
    if (boatCount >= MAX_BOATS) {
        printf("Boat inventory is full. Cannot add more boats.\n");
        return;
    }
    
    // Make a modifiable copy of the line.
    char line[MAX_LINE];
    strncpy(line, csvLine, MAX_LINE);
    line[MAX_LINE - 1] = '\0';
    
    // Tokenize using comma as delimiter.
    char *token = strtok(line, ",");
    if (!token) return;
    char name[MAX_NAME];
    strncpy(name, token, MAX_NAME);
    name[MAX_NAME - 1] = '\0';
    trimWhitespace(name);
    
    token = strtok(NULL, ",");
    if (!token) return;
    int length = atoi(token);
    
    token = strtok(NULL, ",");
    if (!token) return;
    trimWhitespace(token);
    BoatType type = getBoatTypeFromString(token);
    
    token = strtok(NULL, ",");
    if (!token) return;
    trimWhitespace(token);
    char specificField[MAX_LINE];
    strncpy(specificField, token, sizeof(specificField));
    specificField[sizeof(specificField)-1] = '\0';

    token = strtok(NULL, ",");
    if (!token) return;
    double amountOwed = atof(token);

    
    // Allocate memory for new boat
    Boat *newBoat = malloc(sizeof(Boat));
    if (!newBoat) {
        printf("Memory allocation error.\n");
        exit(1);
    }
    strncpy(newBoat->name, name, MAX_NAME);
    newBoat->length = length;
    newBoat->type = type;
    newBoat->amountOwed = amountOwed;
    
    // Depending on type, set extra information.
    switch(type) {
    case SLIP:
        newBoat->info.slipNumber = atoi(specificField);
        break;
    case LAND:
        newBoat->info.bay = specificField[0];
        break;
    case TRAILOR:
        newBoat->info.trailerTag = strdup(specificField);
        break;
    case STORAGE:
        newBoat->info.storageNumber = atoi(specificField);
        break;
    }

    
    // Insert into sorted array.
    insertBoatSorted(newBoat);
}

// Remove a boat by name (case-insensitive with the helper function above)
void removeBoat(void) {
    char name[MAX_NAME];
    printf("Please enter the boat name                               : ");
    if (fgets(name, sizeof(name), stdin) == NULL)
        return;
    name[strcspn(name, "\n")] = 0;
    trimWhitespace(name);
    
    int index = findBoatIndexByName(name);
    if (index < 0) {
        printf("No boat with that name\n");
        return;
    }
    
    // Free the boat memory
    freeBoat(boats[index]);
    // Shift remaining boats down an element in the array
    for (int i = index; i < boatCount - 1; i++) {
        boats[i] = boats[i+1];
    }
    boatCount--;
}

// Accept a payment for a boat
void acceptPayment(void) {
    char name[MAX_NAME];
    printf("Please enter the boat name                               : ");
    if (fgets(name, sizeof(name), stdin) == NULL)
        return;
    name[strcspn(name, "\n")] = 0;
    trimWhitespace(name);
    
    int index = findBoatIndexByName(name);
    if (index < 0) {
        printf("No boat with that name\n");
        return;
    }
    Boat *b = boats[index];
    
    char amtStr[64];
    printf("Please enter the amount to be paid                       : ");
    if (fgets(amtStr, sizeof(amtStr), stdin) == NULL)
        return;
    double payment = atof(amtStr);
    if (payment > b->amountOwed) {
        printf("That is more than the amount owed, $%.2f\n", b->amountOwed);
        return;
    }
    b->amountOwed -= payment;
}

// Update the amount owed for all boats for every new month
void updateMonthlyFees(void) {
    // Define rates per foot per month.
    const double rateSlip = 12.50;
    const double rateLand = 14.00;
    const double rateTrailor = 25.00;
    const double rateStorage = 11.20;
    
    for (int i = 0; i < boatCount; i++) {
        Boat *b = boats[i];
        double fee = 0;
        switch(b->type) {
            case SLIP:
                fee = b->length * rateSlip;
                break;
            case LAND:
                fee = b->length * rateLand;
                break;
            case TRAILOR:
                fee = b->length * rateTrailor;
                break;
            case STORAGE:
                fee = b->length * rateStorage;
                break;
        }
        b->amountOwed += fee;
    }
}

// Find boat index by name (case-insensitive); returns -1 if not found.
int findBoatIndexByName(const char *name) {
    for (int i = 0; i < boatCount; i++) {
        if (caseInsensitiveCompare(boats[i]->name, name) == 0)
            return i;
    }
    return -1;
}

// Convert a string to BoatType
BoatType getBoatTypeFromString(const char *typeStr) {
    if (strcasecmp(typeStr, "slip") == 0)
        return SLIP;
    else if (strcasecmp(typeStr, "land") == 0)
        return LAND;
    else if (strcasecmp(typeStr, "trailor") == 0)
        return TRAILOR;
    else if (strcasecmp(typeStr, "storage") == 0)
        return STORAGE;
    // Default (should not happen if input is valid)
    return SLIP;
}

// Convert BoatType to string for CSV output
const char *boatTypeToString(BoatType type) {
    switch(type) {
        case SLIP: return "slip";
        case LAND: return "land";
        case TRAILOR: return "trailor";
        case STORAGE: return "storage";
        default: return "unknown";
    }
}

// Free the memory associated with a boat, particularly for trailer type
void freeBoat(Boat *b) {
    if (b->type == TRAILOR && b->info.trailerTag != NULL) {
        free(b->info.trailerTag);
    }
    free(b);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s BoatData.csv\n", argv[0]);
        return 1;
    }
    
    // Load boat data from file
    loadBoatData(argv[1]);
    
    printf("Welcome to the Boat Management System\n");
    printf("-------------------------------------\n\n");
    
    char input[MAX_LINE];
    while (1) {
        printf("(I)nventory, (A)dd, (R)emove, (P)ayment, (M)onth, e(X)it : ");
        if (fgets(input, sizeof(input), stdin) == NULL)
            break;
        // Remove newline
        input[strcspn(input, "\n")] = 0;
        // Get the first character (ignore leading whitespace)
        char option = '\0';
        for (int i = 0; input[i] != '\0'; i++) {
            if (!isspace((unsigned char)input[i])) {
                option = input[i];
                break;
            }
        }
        option = toupper(option);
        
        switch(option) {
            case 'I':
                printInventory();
                break;
            case 'A':
                {
                    printf("Please enter the boat data in CSV format                 : ");
                    char csvLine[MAX_LINE];
                    if (fgets(csvLine, sizeof(csvLine), stdin) != NULL) {
                        csvLine[strcspn(csvLine, "\n")] = 0;
                        addBoat(csvLine);
                    }
                }
                break;
            case 'R':
                removeBoat();
                break;
            case 'P':
                acceptPayment();
                break;
            case 'M':
                updateMonthlyFees();
                break;
            case 'X':
                saveBoatData(argv[1]);
                // Free all allocated boats
                for (int i = 0; i < boatCount; i++) {
                    freeBoat(boats[i]);
                }
                return 0;
            default:
                if (option != '\0')
                    printf("Invalid option %c\n", option);
                break;
        }
        printf("\n");
    }
    
    // If we exit the loop unexpectedly, free allocated boats.
    for (int i = 0; i < boatCount; i++) {
        freeBoat(boats[i]);
    }
    
    return 0;
}

