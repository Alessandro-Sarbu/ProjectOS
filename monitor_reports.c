#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

volatile sig_atomic_t keep_running = 1;
volatile sig_atomic_t report_added = 0;

void handle_sigint(int sig) {
    (void) sig;
    keep_running = 0; 
}

void handle_sigusr1(int sig) {
    (void) sig;
    report_added = 1; 
}

int main() {
    // --- NEW: Phase 3 Startup Check ---
    FILE *check_file = fopen(".monitor_pid", "r");
    if (check_file != NULL) {
        int existing_pid = -1;
        if (fscanf(check_file, "%d", &existing_pid) == 1) {
            printf("ERROR_ALREADY_RUNNING:%d\n", existing_pid);
            fflush(stdout);
            fclose(check_file);
            exit(1);
        }
        fclose(check_file);
    }
    pid_t pid = getpid();
    FILE *pid_file = fopen(".monitor_pid", "w");
    if (pid_file == NULL) {
        perror("Error creating .monitor_pid");
        exit(1);
    }
    fprintf(pid_file, "%d\n", pid);
    fclose(pid_file);

    printf("Monitor started successfully!\n");
    printf("PID: %d saved to .monitor_pid\n", pid);
    printf("Waiting for signals... (Press Ctrl+C to stop)\n\n");

    struct sigaction sa_int;
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("Error setting up SIGINT");
        exit(1);
    }

    struct sigaction sa_usr;
    sa_usr.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr.sa_mask);
    sa_usr.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa_usr, NULL) == -1) {
        perror("Error setting up SIGUSR1");
        exit(1);
    }

    while (keep_running) {
        pause(); 

        if (report_added) {
            printf("Notification: A new report has been added!\n");
            fflush(stdout); 
            report_added = 0;
        }
    }

    printf("\nReceived SIGINT. Shutting down monitor...\n");
    
    if (unlink(".monitor_pid") == 0) {
        printf("Successfully removed .monitor_pid file.\n");
    } else {
        perror("Error removing .monitor_pid");
    }

    return 0;
}