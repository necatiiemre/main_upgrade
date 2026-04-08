#include "AteMode.h"
#include <stdio.h>

static bool g_ate_mode = false;

static bool ask_question(const char *question) {
    char response;
    int c;

    while (1) {
        printf("%s [y/n]: ", question);
        fflush(stdout);

        response = getchar();

        // Clear rest of line
        while ((c = getchar()) != '\n' && c != EOF);

        if (response == 'y' || response == 'Y') {
            return true;
        } else if (response == 'n' || response == 'N') {
            return false;
        }

        printf("Invalid input! Please enter 'y' or 'n'.\n");
    }
}

void ate_mode_selection(void) {
    printf("=== ATE Test Mode Selection ===\n\n");

    if (ask_question("Do you want to continue in ATE test mode?")) {
        printf("\n[ATE] ATE test mode selected.\n");

        // No Cumulus ATE reconfiguration needed for VMC -
        // the initial Cumulus config from configureSequence is sufficient.

        // Ask for ATE test cables
        while (!ask_question("Are the ATE test mode cables installed?")) {
            printf("\nPlease install the ATE test mode cables and try again.\n\n");
        }

        g_ate_mode = true;
        printf("[ATE] ATE test mode enabled.\n\n");
    } else {
        printf("Continuing in normal test mode.\n\n");
    }
}

bool ate_mode_enabled(void) {
    return g_ate_mode;
}
