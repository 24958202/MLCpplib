#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#define BUFFER_SIZE 1024

volatile sig_atomic_t flag = 1;

void handler(int sig) {
    flag = 0;
}

int main(void) {
	//"/Volumes/WorkDisk/WatchDog/c_process_term/process_list.txt", "r"
    FILE *file = fopen("/home/ronnieji/watchdog/process_list.txt", "r");

    if (file == NULL) {
        perror("Cannot open process list file");
        return 1;
    }

    printf("Watch-dog is running...\n");
    fflush(stdout);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    char buffer[BUFFER_SIZE];
    pid_t pid;

    while (flag) {
        fseek(file, 0, SEEK_SET); // Reset file pointer to the beginning
        while (fgets(buffer, BUFFER_SIZE, file) && flag) {
            char *process_name = strtok(buffer, "\n");
    
            if (process_name == NULL) {
                continue;
            }
    
            pid = fork();
    
            if (pid == 0) {
                // Child process
                execl("/usr/bin/killall", "killall", "-9", process_name, NULL);
                //perror("execl");
                //_exit(1); // Exit immediately if execl fails
            } else if (pid > 0) {
                // Parent process
                int status;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    printf("Terminated process '%s'\n", process_name);
                } else {
                    fprintf(stderr, "Failed to terminate process '%s'\n", process_name);
                }
            } else {
                // Error forking
                perror("fork");
                flag = 0; // Set flag to 0 to exit the loop
            }
        }

        sleep(1); // Wait for 3 seconds before checking again
    }

    fclose(file); // Close the file before exiting
    printf("Watch-dog terminated.\n");
    return 0;
}
