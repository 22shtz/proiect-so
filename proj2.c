#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>

#define MAX_DIRS 100
#define MAX_PATH_LEN 1024
#define MAX_PATHL 500

void mutaFisierInFolder(const char *absPath, const char *numeFis, const char *numeDir) {
    char pathToDir[MAX_PATHL];
    sprintf(pathToDir, "%s/%s", numeDir, numeFis);

    if (rename(absPath, pathToDir) == 0) {
        printf("Fisier mutat cu succes.\n");
    } else {
        perror("Eroare la mutarea fisierului ");
    }
}

/////functie drepturi
void check_file_permissions(const char *file_path, const char *dir_out) {
    // Verificăm permisiunile fișierului
    
    struct stat file_stat;
    if (stat(file_path, &file_stat) == -1) {
        perror("stat");
        return;
    }

    // Verificăm dacă fișierul nu are nicio permisiune
    if ((file_stat.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)) == 0) {
    	printf("am gasit fisier fara drepturi\n");
        // Dăm drept de citire fișierului
        if (chmod(file_path, S_IRUSR | S_IRGRP | S_IROTH) == -1) {
            perror("chmod");
            return;
        }

        // Creăm un pipe
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            return;
        }

        // Creăm un proces copil
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return;
        }

        if (pid == 0) { // Suntem în procesul copil
            // Redirecționăm stdout către capătul de scriere al pipe-ului
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);

            // Rulăm scriptul verify_script.sh cu calea către fișier ca argument
            execlp("sh", "sh", "verify_script.sh", file_path, (char *)NULL);

            perror("execlp");
            exit(EXIT_FAILURE);
        } else { // Suntem în procesul părinte
            close(pipefd[1]); // Închidem capătul de scriere al pipe-ului în procesul părinte

            // Citim rezultatul din pipe
            char buffer[1024];
            
            ssize_t bytes_read = read(pipefd[0], buffer, sizeof(buffer));
            close(pipefd[0]); // Închidem capătul de citire al pipe-ului în procesul părinte

            // Convertim rezultatul la întreg
            //int result = atoi(buffer);
            

            // Verificăm rezultatul și mutăm fișierul în directorul "dir_out" dacă este cazul
            if (chmod(file_path, S_IRUSR & ~S_IRUSR) == -1) {
        		perror("chmod");
        		exit(EXIT_FAILURE);
    		}
            
            if( strcmp(buffer , "SAFE\n") != 0){
            	// Eliminăm dreptul de citire al fișierului
		if (chmod(file_path, S_IRUSR & ~S_IRUSR) == -1) {
    			perror("chmod");
    			exit(EXIT_FAILURE);
		}

                mutaFisierInFolder(file_path, strrchr(file_path, '/') + 1, dir_out);
                perror("Eroare la mutarea fisierului ");
            }
        }
    }
}
// Funcție pentru a compara conținutul a două fișiere
void compare_files(const char *file1_path, const char *file2_path, int *ok) {
    int file1 = open(file1_path, O_RDONLY);
    int file2 = open(file2_path, O_RDONLY);

    if (file1 == -1 || file2 == -1) {
        perror("open");
        return;
    }

    char buf1[MAX_PATH_LEN];
    char buf2[MAX_PATH_LEN];
    int line_number = 1;

    ssize_t bytes_read1, bytes_read2;
    while ((bytes_read1 = read(file1, buf1, sizeof(buf1))) > 0 &&
           (bytes_read2 = read(file2, buf2, sizeof(buf2))) > 0) {
        if (bytes_read1 != bytes_read2 || memcmp(buf1, buf2, bytes_read1) != 0) {
            printf("Difference found at line %d:\n", line_number);
            *ok = 1;
            break;
        }
        line_number++;
    }

    close(file1);
    close(file2);
}

// Funcție pentru a compara conținutul a două directoare
void compare_directories(const char *dir1_path, const char *dir2_path, int *ok) {
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

        compare_files(file1_path, file2_path, ok);
    }

    closedir(dir1);
    closedir(dir2);
}

char *generate_snapshot_filename(const char *output_dir, int dir_index) {
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

        int snapshot_file = open(snapshot_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        int snapshot_file2 = open(snapshot_path2, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

        if (snapshot_file == -1 || snapshot_file2 == -1) {
            perror("open");
            closedir(dir);
            continue;
        }

        dprintf(snapshot_file, "Snapshot for directory: %s\n", input_dir);
        dprintf(snapshot_file2, "Snapshot for directory: %s\n", input_dir);

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

            dprintf(snapshot_file2, "Name: %s, Size: %ld bytes, Modified: %ld\n", entry->d_name,
                    (long)entry_stat.st_size, (long)entry_stat.st_mtime);

            dprintf(snapshot_file, "Name: %s, Size: %ld bytes, Modified: %ld\n", entry->d_name,
                    (long)entry_stat.st_size, (long)entry_stat.st_mtime);
        }

        close(snapshot_file);
        close(snapshot_file2);
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
    if (argc < 5 || strcmp(argv[1], "-o") != 0) {
        printf("Use: %s -o out1 out2 directory1 [directory2 ...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char output_dir[MAX_PATH_LEN];
    char output_dir2[MAX_PATH_LEN];
    char dir_out[MAX_PATH_LEN];
    strcpy(output_dir, argv[2]);
    strcpy(output_dir2, argv[3]);
    strcpy(dir_out, argv[4]);
    
    int ok = 0;

    // Comparăm modificările și dacă se găsesc, ok devine 1 și se vor modifica ambele foldere


    const char *input_dirs[MAX_DIRS];
    int num_dirs = argc - 5;
    if (num_dirs > MAX_DIRS) {
        printf("Too many directories specified\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_dirs; i++) {
        input_dirs[i] = argv[i + 5];
    }
////////////////////////mal
for (int i = 0; i < num_dirs; i++) {
    const char *dir_path = input_dirs[i];

    // Verificăm dacă este un director
    struct stat dir_stat;
    if (stat(dir_path, &dir_stat) == -1) {
        perror("stat");
        continue;
    }

    if (!S_ISDIR(dir_stat.st_mode)) {
        printf("Ignore non-directory argument: %s\n", dir_path);
        continue;
    }

    // Verificăm fișierele din director
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir");
        continue;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char file_path[MAX_PATH_LEN];
        snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry->d_name);

        // Verificăm permisiunile fișierului
        check_file_permissions(file_path, argv[4]);

        
    }

    closedir(dir);
}


    /// Dacă e prima rulare, se vor modifica ambele foldere, altfel se va modifica doar al 2-lea
    if (isFirstRun()) {
        for (int i = 0; i < num_dirs; i++) {
            if (!isDirectory(input_dirs[i])) {
                printf("Ignore non-directory argument: %s\n", input_dirs[i]);
                continue;
            }

            pid_t pid = fork();
            if (pid == 0) { // Proces copil
                update_snapshots(output_dir, output_dir2, &input_dirs[0], num_dirs); // Actualizăm instantaneele pentru toate directoarele
                exit(EXIT_SUCCESS);
            } else if (pid < 0) { // Eșec la bifare
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
            if (pid == 0) { // Proces copil
                update_snapshots(output_dir2, output_dir2, &input_dirs[0], num_dirs); // Actualizăm instantaneele doar în output_dir2
                ok = 0;
                compare_directories(output_dir, output_dir2, &ok);
                //printf("\n");
                //printf("%d\n", ok);
                if (ok == 1) {
                    update_snapshots(output_dir, output_dir2, &input_dirs[0], num_dirs);
                }
                exit(EXIT_SUCCESS);
            } else if (pid < 0) { // Eșec la bifare
                perror("Error child process");
                exit(EXIT_FAILURE);
            }
        }
    }
    

    // Proces părinte
int status;
pid_t pid;
while ((pid = wait(&status)) > 0) { 
        if (WIFEXITED(status)) {
            printf("The process with PID %d has ended with code %d.\n", pid, WEXITSTATUS(status));
        }
    }


    // Practic, compară mereu după actualizarea fișierului directorului out2; ok devine 1 dacă există modificări și în acest caz se modifică și dir out1

    return EXIT_SUCCESS;
}

