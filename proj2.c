#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define MAX_DIRS 100
#define MAX_PATH_LEN 1024


// Funcție pentru a compara conținutul a două fișiere
void compare_files(const char *file1_path, const char *file2_path, int* ok) {
    FILE *file1 = fopen(file1_path, "r");
    FILE *file2 = fopen(file2_path, "r");

    if (file1 == NULL || file2 == NULL) {
        perror("fopen");
        return;
    }

    char line1[MAX_PATH_LEN];
    char line2[MAX_PATH_LEN];
    int line_number = 1;

    while (fgets(line1, sizeof(line1), file1) != NULL && fgets(line2, sizeof(line2), file2) != NULL) {
        if (strcmp(line1, line2) != 0) {
            printf("Difference found at line %d:\n", line_number);
            printf("%s:\n%s", file1_path, line1);
            printf("%s:\n%s", file2_path, line2);
	    *ok=1;
        }
        line_number++;
    }

    // Verificăm dacă unul dintre fișiere are mai multe linii decât celălalt
    if (fgets(line1, sizeof(line1), file1) != NULL) {
        printf("Extra lines found in %s:\n", file1_path);
        while (fgets(line1, sizeof(line1), file1) != NULL) {
            printf("%s", line1);
        }
    } else if (fgets(line2, sizeof(line2), file2) != NULL) {
        printf("Extra lines found in %s:\n", file2_path);
        while (fgets(line2, sizeof(line2), file2) != NULL) {
            printf("%s", line2);
        }
    }

    fclose(file1);
    fclose(file2);
}

// Funcție pentru a compara conținutul a două directoare
void compare_directories(const char *dir1_path, const char *dir2_path, int* ok) {
    DIR *dir1 = opendir(dir1_path);
    DIR *dir2 = opendir(dir2_path);

    if (dir1 == NULL || dir2 == NULL) {
        perror("opendir");
        return;
    }

    struct dirent *entry1;
    struct dirent *entry2;

    // Parcurgem ambele directoare și comparăm conținutul fișierelor de snapshot
    while ((entry1 = readdir(dir1)) != NULL && (entry2 = readdir(dir2)) != NULL) {
        if (strcmp(entry1->d_name, ".") == 0 || strcmp(entry1->d_name, "..") == 0) {
            continue;
        }

        char file1_path[MAX_PATH_LEN];
        char file2_path[MAX_PATH_LEN];
        snprintf(file1_path, sizeof(file1_path), "%s/%s", dir1_path, entry1->d_name);
        snprintf(file2_path, sizeof(file2_path), "%s/%s", dir2_path, entry2->d_name);

        compare_files(file1_path, file2_path,ok);
    }

    closedir(dir1);
    closedir(dir2);
}



char* generate_snapshot_filename(const char *output_dir, int dir_index) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char *snapshot_filename = malloc(MAX_PATH_LEN);
    snprintf(snapshot_filename, MAX_PATH_LEN, "%s/snapshot_%d_%d-%02d-%02d_%02d:%02d:%02d.txt", output_dir,
             dir_index + 1, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    return snapshot_filename;
}

void update_snapshots(const char *output_dir, const char *output_dir2, const char *input_dirs[], int num_dirs) {
    for (int i = 0; i < num_dirs; i++) {
        const char *input_dir = input_dirs[i];
        DIR *dir = opendir(input_dir);
        if (dir == NULL) {
            perror("opendir");
            continue;
        }

        char snapshot_path[MAX_PATH_LEN];
        char snapshot_path2[MAX_PATH_LEN];
        snprintf(snapshot_path, sizeof(snapshot_path), "%s/snapshot_%d.txt", output_dir, i + 1);
        snprintf(snapshot_path2, sizeof(snapshot_path2), "%s/snapshot_%d.txt", output_dir2, i + 1);

        FILE *snapshot_file = fopen(snapshot_path, "w");
        FILE *snapshot_file2 = fopen(snapshot_path2, "w"); // Deschidem în modul "w" pentru a suprascrie întregul conținut la fiecare rulare

        if (snapshot_file == NULL || snapshot_file2 == NULL) {
            perror("fopen");
            closedir(dir);
            continue;
        }

        fprintf(snapshot_file, "Snapshot for directory: %s\n", input_dir);
	fprintf(snapshot_file2, "Snapshot for directory: %s\n", input_dir);

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char entry_path[MAX_PATH_LEN];
            snprintf(entry_path, sizeof(entry_path), "%s/%s", input_dir, entry->d_name);

            struct stat entry_stat;
            if (stat(entry_path, &entry_stat) == -1) {
                perror("stat");
                continue;
            }

            fprintf(snapshot_file2, "Name: %s, Size: %ld bytes, Modified: %ld\n", entry->d_name,
                    (long)entry_stat.st_size, (long)entry_stat.st_mtime);

	    fprintf(snapshot_file, "Name: %s, Size: %ld bytes, Modified: %ld\n", entry->d_name,
        (long)entry_stat.st_size, (long)entry_stat.st_mtime);

        }

        fclose(snapshot_file);
        fclose(snapshot_file2);
        closedir(dir);
    }
}


bool isDirectory(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) return false;
    return S_ISDIR(statbuf.st_mode);
}

bool isFirstRun() {
    DIR *dir = opendir("out1");
    if (dir == NULL) {
        return true; // Directorul out1 nu există
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            closedir(dir);
            return false; // Directorul out1 conține fișiere
        }
    }
    closedir(dir);
    return true; // Directorul out1 este gol
}

int main(int argc, char *argv[]) {
    if (argc < 4 || strcmp(argv[1], "-o") != 0) {
        printf("Use: %s -o out1 out2 directory1 [directory2 ...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char output_dir[MAX_PATH_LEN];
    char output_dir2[MAX_PATH_LEN];
    strcpy(output_dir, argv[2]);
    strcpy(output_dir2, argv[3]);
    int ok=0;

    //comparam modif si daca se gasesc se face ok 1 si se vor modifica ambele foldere
    compare_directories(output_dir,output_dir2,&ok);
    printf("ok value before forking: %d\n", ok);

    const char *input_dirs[MAX_DIRS];
    int num_dirs = argc - 4;
    if (num_dirs > MAX_DIRS) {
        printf("Too many directories specified\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_dirs; i++) {
        input_dirs[i] = argv[i + 4];
    }

    ///daca e prima rulare se vor modifica ambele foldere, altfel se va modifica doar al 2 lea
    
    if (isFirstRun()) {
        for (int i = 0; i < num_dirs; i++) {
            if (!isDirectory(input_dirs[i])) {
                printf("Ignore non-directory argument: %s\n", input_dirs[i]);
                continue;
            }

            pid_t pid = fork();
            if (pid == 0) { // Child process
                update_snapshots(output_dir, output_dir2, &input_dirs[0], num_dirs); // Update snapshots for all directories
        
                exit(EXIT_SUCCESS);
            } else if (pid < 0) { // Fork failed
                perror("Error child process");
                exit(EXIT_FAILURE);
            }
        }
    } else {
        for (int i = 0; i < num_dirs; i++) {
            if (!isDirectory(input_dirs[i])) {
                printf("Ignore non-directory argument: %s\n", input_dirs[i]);
                continue;
            }

            pid_t pid = fork();
            if (pid == 0) { // Child process
            	update_snapshots(output_dir2, output_dir2, &input_dirs[0], num_dirs); // Update snapshots only in output_dir2
            	ok=0;
            	compare_directories(output_dir,output_dir2,&ok);
            	printf("\n");
            	printf("%d\n",ok);
		if(ok==1){
		  update_snapshots(output_dir, output_dir2, &input_dirs[0], num_dirs);
		}
		
                exit(EXIT_SUCCESS);
            } else if (pid < 0) { // Fork failed
                perror("Error child process");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Parent process
    int status;
    pid_t pid;
    while ((pid = wait(&status)) > 0) { 
        if (WIFEXITED(status)) {
            printf("The process with PID %d has ended with code %d.\n", pid, WEXITSTATUS(status));
        }
    }

    compare_directories(output_dir,output_dir2,&ok);
    printf("ok value after update: %d\n", ok);
    ///practic compara mereu dupa updatarea fisierului directorului out2 ok se face 1 daca exista modific si in acest caz se modifica si dir out1
    
    return EXIT_SUCCESS;
}



