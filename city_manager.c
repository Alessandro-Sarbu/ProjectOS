#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/wait.h>

#define MAX_NAME 64
#define MAX_CATEGORY 32
#define MAX_DESC 256

typedef struct {
    int report_id;
    char inspector_name[MAX_NAME];
    double latitude;
    double longitude;
    char category[MAX_CATEGORY];
    int severity; 
    time_t timestamp;
    char description[MAX_DESC];
} Report;

// AI-Generated Function 1: Parses "field:operator:value" into separate strings
int parse_condition(const char *input, char *field, char *op, char *value) {
    // We use sscanf to split the string at the colons
    // %[^:] means "read everything until you hit a colon"
    if (sscanf(input, "%[^:]:%[^:]:%s", field, op, value) == 3) {
        return 1; // Successfully parsed all 3 parts
    }
    return 0; // Failed to parse
}

// AI-Generated Function 2: Evaluates a single record against a parsed condition
int match_condition(Report *r, const char *field, const char *op, const char *value) {
    // Handle integer comparisons (Severity)
    if (strcmp(field, "severity") == 0) {
        int val = atoi(value);
        if (strcmp(op, "==") == 0) return r->severity == val;
        if (strcmp(op, "!=") == 0) return r->severity != val;
        if (strcmp(op, "<")  == 0) return r->severity < val;
        if (strcmp(op, "<=") == 0) return r->severity <= val;
        if (strcmp(op, ">")  == 0) return r->severity > val;
        if (strcmp(op, ">=") == 0) return r->severity >= val;
    } 
    // Handle time_t comparisons (Timestamp)
    else if (strcmp(field, "timestamp") == 0) {
        time_t val = (time_t)atol(value);
        if (strcmp(op, "==") == 0) return r->timestamp == val;
        if (strcmp(op, "!=") == 0) return r->timestamp != val;
        if (strcmp(op, "<")  == 0) return r->timestamp < val;
        if (strcmp(op, "<=") == 0) return r->timestamp <= val;
        if (strcmp(op, ">")  == 0) return r->timestamp > val;
        if (strcmp(op, ">=") == 0) return r->timestamp >= val;
    } 
    // Handle string comparisons (Category & Inspector)
    else if (strcmp(field, "category") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->category, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->category, value) != 0;
    } 
    else if (strcmp(field, "inspector") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->inspector_name, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->inspector_name, value) != 0;
    }
    
    return 0; // Default to false if the field or operator is invalid
}

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}
void mode_to_string(mode_t mode, char *str) {
    strcpy(str, "---------"); 
    
    if (mode & S_IRUSR) str[0] = 'r';
    if (mode & S_IWUSR) str[1] = 'w';
    if (mode & S_IXUSR) str[2] = 'x';
    
    if (mode & S_IRGRP) str[3] = 'r';
    if (mode & S_IWGRP) str[4] = 'w';
    if (mode & S_IXGRP) str[5] = 'x';
    
    if (mode & S_IROTH) str[6] = 'r';
    if (mode & S_IWOTH) str[7] = 'w';
    if (mode & S_IXOTH) str[8] = 'x';
}

int check_access(const char *filepath, const char *role, int requires_write) {
    struct stat file_stat;
    if (stat(filepath, &file_stat) < 0) {
        perror("Error checking file permissions");
        return 0; 
    }

    if (strcmp(role, "manager") == 0) {
        if (requires_write && !(file_stat.st_mode & S_IWUSR)) {
            printf("Access Denied: Manager requires Owner-Write (w) permission.\n");
            return 0;
        }
        if (!(file_stat.st_mode & S_IRUSR)) {
            printf("Access Denied: Manager requires Owner-Read (r) permission.\n");
            return 0;
        }
        return 1; 
    } 
    else if (strcmp(role, "inspector") == 0) {
        if (requires_write && !(file_stat.st_mode & S_IWGRP)) {
            printf("Access Denied: Inspector requires Group-Write (w) permission.\n");
            return 0;
        }
        if (!(file_stat.st_mode & S_IRGRP)) {
            printf("Access Denied: Inspector requires Group-Read (r) permission.\n");
            return 0;
        }
        return 1;
    }

    printf("Error: Unknown role '%s'.\n", role);
    return 0;
}

void add_report(const char *district, const char *inspector_name, const char *role) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/reports.dat", district);

    if (!check_access(filepath, role, 1)) {
        return; 
    }

    int fd = open(filepath, O_WRONLY | O_APPEND);
    if (fd < 0) {
        perror("Error opening reports.dat for adding");
        return;
    }

    off_t filesize = lseek(fd, 0, SEEK_END);
    int next_id = (filesize / sizeof(Report)) + 1;

    Report new_report;
    memset(&new_report, 0, sizeof(Report));

    new_report.report_id = next_id;
    strncpy(new_report.inspector_name, inspector_name, MAX_NAME - 1);
    new_report.timestamp = time(NULL);

    printf("Adding new report (ID: %d) for district '%s'\n", next_id, district);
    printf("Inspector: %s\n", inspector_name);
    printf("--------------------------------------------------\n");
    
    printf("Enter latitude: ");
    if (scanf("%lf", &new_report.latitude) != 1) new_report.latitude = 0.0;
    
    printf("Enter longitude: ");
    if (scanf("%lf", &new_report.longitude) != 1) new_report.longitude = 0.0;
    clear_input_buffer(); 

    printf("Enter category (e.g., road, lighting, flooding): ");
    if (fgets(new_report.category, MAX_CATEGORY, stdin)) {
        new_report.category[strcspn(new_report.category, "\n")] = 0; 
    }

    printf("Enter severity (1=minor, 2=moderate, 3=critical): ");
    if (scanf("%d", &new_report.severity) != 1) new_report.severity = 1;
    clear_input_buffer(); 

    printf("Enter description: ");
    if (fgets(new_report.description, MAX_DESC, stdin)) {
        new_report.description[strcspn(new_report.description, "\n")] = 0; 
    }

    if (write(fd, &new_report, sizeof(Report)) != sizeof(Report)) {
        perror("Error writing to reports.dat");
    } else {
        printf("\nSuccessfully saved Report ID %d.\n", next_id);
    }

    close(fd);

    if (chmod(filepath, 0664) < 0) {
        perror("Error setting 664 permissions on reports.dat");
    }
}

void list_reports(const char *district) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/reports.dat", district);

    struct stat file_stat;
    if (stat(filepath, &file_stat) < 0) {
        perror("Error getting file stats");
        return;
    }

    char perms[10];
    mode_to_string(file_stat.st_mode, perms);

    char time_buf[64];
    struct tm *tm_info = localtime(&file_stat.st_mtime);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    printf("\n--- File Info: %s ---\n", filepath);
    printf("Permissions: %s\n", perms);
    printf("Size: %ld bytes\n", (long)file_stat.st_size);
    printf("Last Modified: %s\n", time_buf);
    printf("----------------------------------------\n\n");

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("Error opening reports.dat for reading");
        return;
    }

    Report rep;
    int count = 0;
    
    printf("--- Reports in District: %s ---\n", district);
    
    while (read(fd, &rep, sizeof(Report)) == sizeof(Report)) {
        count++;
        
        char rep_time[64];
        struct tm *rep_tm = localtime(&rep.timestamp);
        strftime(rep_time, sizeof(rep_time), "%Y-%m-%d %H:%M:%S", rep_tm);

        printf("ID: %d | Inspector: %s | Date: %s\n", rep.report_id, rep.inspector_name, rep_time);
        printf("Location: %.4f, %.4f | Category: %s | Severity: %d\n", rep.latitude, rep.longitude, rep.category, rep.severity);
        printf("Description: %s\n", rep.description);
        printf("- - - - - - - - - - - - - - - - - - - - - - - - \n");
    }

    if (count == 0) {
        printf("No reports found in this district.\n");
    } else {
        printf("Total reports: %d\n", count);
    }

    close(fd);
}

void view_report(const char *district, int target_id) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/reports.dat", district);

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("Error opening reports.dat for viewing");
        return;
    }

    Report rep;
    int found = 0;

    while (read(fd, &rep, sizeof(Report)) == sizeof(Report)) {
        if (rep.report_id == target_id) {
            found = 1; 
            
            char rep_time[64];
            struct tm *rep_tm = localtime(&rep.timestamp);
            strftime(rep_time, sizeof(rep_time), "%Y-%m-%d %H:%M:%S", rep_tm);

            printf("\n--- Report Details (ID: %d) ---\n", rep.report_id);
            printf("District:  %s\n", district);
            printf("Inspector: %s\n", rep.inspector_name);
            printf("Date:      %s\n", rep_time);
            printf("Location:  Latitude %.6f, Longitude %.6f\n", rep.latitude, rep.longitude);
            printf("Category:  %s\n", rep.category);
            printf("Severity:  %d\n", rep.severity);
            printf("Description:\n%s\n", rep.description);
            printf("-------------------------------\n\n");
            
            break;
        }
    }

    if (!found) {
        printf("Report with ID %d not found in district '%s'.\n", target_id, district);
    }

    close(fd);
}

void remove_report(const char *district, int target_id, const char *role) {
    if (strcmp(role, "manager") != 0) {
        printf("Error: Only users with the 'manager' role can remove reports.\n");
        return;
    }

    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/reports.dat", district);

    if (!check_access(filepath, role, 1)) {
        return;
    }

    int fd = open(filepath, O_RDWR);
    if (fd < 0) {
        perror("Error opening reports.dat for removal");
        return;
    }

    struct stat file_stat;
    if (fstat(fd, &file_stat) < 0) {
        perror("Error getting file stats");
        close(fd);
        return;
    }
    off_t original_size = file_stat.st_size;

    Report rep;
    off_t target_pos = -1;
    
    while (read(fd, &rep, sizeof(Report)) == sizeof(Report)) {
        if (rep.report_id == target_id) {
            target_pos = lseek(fd, 0, SEEK_CUR) - sizeof(Report);
            break;
        }
    }

    if (target_pos == -1) {
        printf("Report ID %d not found.\n", target_id);
        close(fd);
        return;
    }

    off_t read_pos = target_pos + sizeof(Report); 
    off_t write_pos = target_pos;                 

    while (1) {
        lseek(fd, read_pos, SEEK_SET);
        int bytes_read = read(fd, &rep, sizeof(Report));
        
        if (bytes_read <= 0) break; 

        lseek(fd, write_pos, SEEK_SET);
        write(fd, &rep, sizeof(Report));

        read_pos += sizeof(Report);
        write_pos += sizeof(Report);
    }

    off_t new_size = original_size - sizeof(Report);
    if (ftruncate(fd, new_size) < 0) {
        perror("Error truncating file");
    } else {
        printf("\nSuccessfully removed Report ID %d.\n", target_id);
        printf("File size changed from %ld bytes to %ld bytes.\n", (long)original_size, (long)new_size);
    }

    close(fd);
}

void update_threshold(const char *district, int new_value, const char *role) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/district.cfg", district);

    struct stat file_stat;
    if (stat(filepath, &file_stat) < 0) {
        perror("Error checking district.cfg");
        return;
    }

    if ((file_stat.st_mode & 0777) != 0640) {
        printf("Diagnostic Error: district.cfg permissions have been tampered with!\n");
        printf("Expected 640, but found different permissions. Refusing to write.\n");
        return;
    }

    if (!check_access(filepath, role, 1)) {
        return;
    }

    FILE *file = fopen(filepath, "w");
    if (!file) {
        perror("Error opening district.cfg for writing");
        return;
    }

    fprintf(file, "%d\n", new_value);
    fclose(file);

    printf("Successfully updated severity threshold for '%s' to %d.\n", district, new_value);
}

void filter_reports(const char *district, int condition_count, char *conditions[]) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/reports.dat", district);

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("Error opening reports.dat for filtering");
        return;
    }

    Report rep;
    int match_count = 0;
    
    printf("\n--- Filtered Reports for '%s' ---\n", district);

    while (read(fd, &rep, sizeof(Report)) == sizeof(Report)) {
        int passes_all_conditions = 1; 

        for (int i = 0; i < condition_count; i++) {
            char field[64], op[8], value[128];
            
            if (parse_condition(conditions[i], field, op, value)) {
                if (!match_condition(&rep, field, op, value)) {
                    passes_all_conditions = 0;
                    break;
                }
            } else {
                printf("Warning: Could not parse condition '%s'\n", conditions[i]);
                passes_all_conditions = 0;
                break;
            }
        }

        if (passes_all_conditions) {
            match_count++;
            
            char rep_time[64];
            struct tm *rep_tm = localtime(&rep.timestamp);
            strftime(rep_time, sizeof(rep_time), "%Y-%m-%d %H:%M:%S", rep_tm);

            printf("ID: %d | Inspector: %s | Date: %s\n", rep.report_id, rep.inspector_name, rep_time);
            printf("Category: %s | Severity: %d\n", rep.category, rep.severity);
            printf("- - - - - - - - - - - - - - - - - - - - - - - - \n");
        }
    }

    if (match_count == 0) {
        printf("No reports matched the given conditions.\n");
    } else {
        printf("Total matches: %d\n", match_count);
    }

    close(fd);
}
void create_file_if_missing(const char *filepath, mode_t permissions) {
    int fd = open(filepath, O_CREAT | O_APPEND | O_RDWR, permissions);
    if (fd < 0) {
        perror("Error creating file");
        exit(1);
    }
    close(fd);

    if (chmod(filepath, permissions) < 0) {
        perror("Error setting file permissions");
        exit(1);
    }
}

void setup_district(const char *district) {
    if (mkdir(district, 0750) < 0) {
        if (errno != EEXIST) {
            perror("Error creating district directory");
            exit(1);
        }
    }
    chmod(district, 0750);

    char filepath[512];

    snprintf(filepath, sizeof(filepath), "%s/reports.dat", district);
    create_file_if_missing(filepath, 0664);

    snprintf(filepath, sizeof(filepath), "%s/district.cfg", district);
    create_file_if_missing(filepath, 0640);

    snprintf(filepath, sizeof(filepath), "%s/logged_district", district);
    create_file_if_missing(filepath, 0644);

    char symlink_name[256];
    snprintf(symlink_name, sizeof(symlink_name), "active_reports-%s", district);
    
    char target_path[512];
    snprintf(target_path, sizeof(target_path), "%s/reports.dat", district);

    unlink(symlink_name); 
    
    if (symlink(target_path, symlink_name) < 0) {
        perror("Error creating symbolic link");
    }
}

void check_links() {
    DIR *dir = opendir(".");
    if (!dir) {
        perror("Error opening current directory");
        return;
    }

    struct dirent *entry;
    int link_count = 0;
    int dangling_count = 0;

    printf("\n--- Scanning for Symlinks ---\n");

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "active_reports-", 15) == 0) {
            struct stat link_stat;
            
            if (lstat(entry->d_name, &link_stat) == 0) {
                
                if (S_ISLNK(link_stat.st_mode)) {
                    link_count++;
                    struct stat target_stat;
                    
                    if (stat(entry->d_name, &target_stat) < 0) {
                        printf("WARNING: Dangling link detected: %s (Target missing!)\n", entry->d_name);
                        dangling_count++;
                    } else {
                        printf("Healthy link found: %s\n", entry->d_name);
                    }
                }
            }
        }
    }

    if (link_count == 0) {
        printf("No active_reports symlinks found in this directory.\n");
    } else {
        printf("Scan complete. Found %d total links (%d dangling).\n", link_count, dangling_count);
    }

    closedir(dir);
}
// Function to delete a district folder and its symlink using a child process
void remove_district(const char *district, const char *role) {
    // 1. Role Check
    if (strcmp(role, "manager") != 0) {
        printf("Error: Only users with the 'manager' role can remove districts.\n");
        return;
    }

    // 2. CRITICAL SAFETY CHECK
    // Prevent directory traversal attacks or root deletion (e.g., "../" or "/")
    if (strchr(district, '/') != NULL || strstr(district, "..") != NULL) {
        printf("Error: Invalid district name. Unsafe characters detected.\n");
        return;
    }

    // 3. Remove the symbolic link using unlink()
    char symlink_name[256];
    snprintf(symlink_name, sizeof(symlink_name), "active_reports-%s", district);
    if (unlink(symlink_name) == 0) {
        printf("Removed symbolic link: %s\n", symlink_name);
    } else {
        perror("Warning: Could not remove symbolic link (maybe it didn't exist?)");
    }

    // 4. Create a child process to run 'rm -rf'
    printf("Preparing to delete district folder: %s...\n", district);
    
    pid_t pid = fork();

    if (pid < 0) {
        // Fork failed
        perror("Error: Failed to fork process");
    } 
    else if (pid == 0) {
        // CHILD PROCESS
        // execlp replaces this child process with the 'rm' command.
        // Arguments: executable name, arg0, arg1, arg2, NULL (to terminate the list)
        execlp("rm", "rm", "-rf", district, NULL);
        
        // If execlp is successful, the child process is completely replaced.
        // Therefore, the code below this line ONLY runs if execlp fails!
        perror("Error: execlp failed to execute rm");
        exit(1); 
    } 
    else {
        // PARENT PROCESS
        // Wait for the specific child process to finish its job
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            printf("Success: District '%s' and all its contents have been deleted.\n", district);
        } else {
            printf("Error: The rm command did not complete successfully.\n");
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 6) {
        fprintf(stderr, "Usage: %s --role <role> --user <username> --<command> <district> [args...]\n", argv[0]);
        return 1;
    }

    char *role = NULL;
    char *user = NULL;
    char *command = NULL;
    char *district = NULL;
    char *extra_arg = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0 && i + 1 < argc) {
            role = argv[++i];
        } else if (strcmp(argv[i], "--user") == 0 && i + 1 < argc) {
            user = argv[++i];
        } else if (strncmp(argv[i], "--", 2) == 0) {
            command = argv[i] + 2; 
            if (i + 1 < argc) {
                district = argv[++i];
            }
            if (i + 1 < argc) {
                extra_arg = argv[i + 1]; 
            }
            break; 
        }
    }

    if (!role || !user || !command || !district) {
        fprintf(stderr, "Error: Missing required arguments.\n");
        return 1;
    }

    if (strcmp(command, "check_links") != 0 && strcmp(command, "remove_district") != 0) {
        setup_district(district);
    }

    if (strcmp(command, "add") == 0) {
        add_report(district, user, role);
    } else if (strcmp(command, "list") == 0) {
        list_reports(district);
    } else if (strcmp(command, "view") == 0) {
        if (!extra_arg) fprintf(stderr, "Error: --view requires a Report ID.\n");
        else view_report(district, atoi(extra_arg)); 
    } else if (strcmp(command, "remove_report") == 0) {
        if (!extra_arg) fprintf(stderr, "Error: --remove_report requires a Report ID.\n");
        else remove_report(district, atoi(extra_arg), role); 
    } else if (strcmp(command, "update_threshold") == 0) {
        if (!extra_arg) fprintf(stderr, "Error: --update_threshold requires a numeric value.\n");
        else update_threshold(district, atoi(extra_arg), role); 
    } else if (strcmp(command, "filter") == 0) {
        int condition_start_index = 0;
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], district) == 0) {
                condition_start_index = i + 1;
                break;
            }
        }
        int num_conditions = argc - condition_start_index;
        if (num_conditions <= 0) fprintf(stderr, "Error: --filter requires at least one condition.\n");
        else filter_reports(district, num_conditions, &argv[condition_start_index]);
    } else if (strcmp(command, "check_links") == 0) {
        check_links();
    // --- NEW: Phase 2 Remove District ---
    } else if (strcmp(command, "remove_district") == 0) {
        // District name is already parsed into the 'district' variable!
        remove_district(district, role);
    } else {
        printf("Command '--%s' is not implemented yet.\n", command);
    }

    return 0;
}