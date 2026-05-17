#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

pid_t active_hub_mon_pid = -1;
pid_t child_monitor_pid = -1; 

void handle_hub_mon_exit(int sig) {
    (void)sig;
    if (child_monitor_pid > 0) {
        kill(child_monitor_pid, SIGINT);
        waitpid(child_monitor_pid, NULL, 0);
    }
    exit(0); 
}

void handle_start_monitor() {
    pid_t hub_mon_pid = fork();

    if (hub_mon_pid < 0) {
        perror("Error: Forking hub_mon failed");
        return;
    }

    if (hub_mon_pid > 0) {
        active_hub_mon_pid = hub_mon_pid;
        printf("[Hub] Launched background monitor helper (PID %d).\n", hub_mon_pid);
        return; 
    }

    // CHILD PROCESS (hub_mon)
    // From this point down, code executes exclusively inside the background helper

    struct sigaction sa;
    sa.sa_handler = handle_hub_mon_exit;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("hub_mon: sigaction setup failed");
        exit(1);
    }

    int pipe_fds[2];
    if (pipe(pipe_fds) == -1) {
        perror("hub_mon: pipe allocation failed");
        exit(1);
    }

    pid_t monitor_pid = fork();

    if (monitor_pid < 0) {
        perror("hub_mon: monitor fork failed");
        exit(1);
    }

    if (monitor_pid == 0) {
        close(pipe_fds[0]);

        if (dup2(pipe_fds[1], STDOUT_FILENO) == -1) {
            perror("monitor execution: dup2 redirection failed");
            exit(1);
        }
        close(pipe_fds[1]);

        execl("./monitor_reports", "monitor_reports", NULL);
        
        perror("monitor execution: execl failed");
        exit(1);
    }

    child_monitor_pid = monitor_pid;
    close(pipe_fds[1]);

    FILE *pipe_stream = fdopen(pipe_fds[0], "r");
    if (!pipe_stream) {
        perror("hub_mon: fdopen failed");
        exit(1);
    }

    char message_buffer[256];
    while (fgets(message_buffer, sizeof(message_buffer), pipe_stream) != NULL) {
        if (strncmp(message_buffer, "ERROR_ALREADY_RUNNING:", 22) == 0) {
            int active_id = atoi(message_buffer + 22);
            printf("\n[Alert] Cannot launch monitor: Instance already active under PID %d.\n", active_id);
            fflush(stdout);
        } else {
            printf("\n[Monitor System Output]: %s", message_buffer);
            fflush(stdout);
        }
    }

    printf("\n[Notice] The report monitor has terminated.\n");
    fflush(stdout);

    fclose(pipe_stream);
    exit(0); 
}

void handle_calculate_scores(char *args) {
    printf("[Hub] Calculating workload scores for districts: %s (Placeholder)\n", args);
}

int main() {
    char input[256];
    
    printf("=========================================\n");
    printf("         CITY INFRASTRUCTURE HUB        \n");
    printf("=========================================\n");
    printf("Available commands:\n");
    printf("  start_monitor\n");
    printf("  calculate_scores <district1> <district2> ...\n");
    printf("  exit\n\n");

    while (1) {
        printf("hub> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "exit") == 0) {
            if (active_hub_mon_pid > 0) {
                printf("Shutting down background monitor systems (PID %d)...\n", active_hub_mon_pid);
                
                kill(active_hub_mon_pid, SIGINT);
                
                waitpid(active_hub_mon_pid, NULL, 0);
            }

            printf("Exiting City Hub. Goodbye!\n");
            break;
        }
        else if (strcmp(input, "start_monitor") == 0) {
            handle_start_monitor();
        } 
        else if (strncmp(input, "calculate_scores ", 17) == 0) {
            handle_calculate_scores(input + 17);
        } 
        else if (strlen(input) > 0) {
            printf("Unknown command: %s\n", input);
        }
    }

    return 0;
}