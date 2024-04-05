#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
bool isDirectory(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) return false;
    return S_ISDIR(statbuf.st_mode);
}

void listDirectory(const char *dirName, const char *outputPath, int level) {
    DIR *dir;
    struct dirent *entry;
    struct stat info;
    FILE *outputFile = fopen(outputPath, "a"); 

    if (!outputFile) {
        fprintf(stderr, "Nu se poate deschide fisierul de iesire %s\n", outputPath);
        return;
    }

    if (!(dir = opendir(dirName))) {
        fprintf(outputFile, "Nu se poate deschide directorul %s\n", dirName);
        fclose(outputFile);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dirName, entry->d_name);

        if (stat(path, &info) != 0) {
            fprintf(stderr, "Nu se poate accesa %s\n", path);
            continue;
        }

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        fprintf(outputFile, "%*s%s\n", level * 4, "", entry->d_name);
        if (S_ISDIR(info.st_mode)) {
            listDirectory(path, outputPath, level + 1);
        }
    }

    closedir(dir);
    fclose(outputFile);
}


int main(int argc, char **argv) {
    
    if (argc < 3) {
        fprintf(stderr, "Utilizare: %s -o output_directory input_directory1 [input_directory2 ...]\n", argv[0]);
        return 1;
    }

    
    if (strcmp(argv[1], "-o") != 0) {
        fprintf(stderr, "Primul argument trebuie sa fie optiunea '-o' urmata de directorul de iesire\n");
        return 2;
    }

    char *outputDir = argv[2]; 

   
    if (!isDirectory(outputDir)) {
        fprintf(stderr, "Directorul de iesire specificat nu exista: %s\n", outputDir);
        return 3;
    }

   
    for (int i = 3; i < argc; i++) {
      
        if (!isDirectory(argv[i])) {
            printf("Ignorand argumentul non-director: %s\n", argv[i]);
            continue; 
        }

        
        char snapshotPath[1024];
        snprintf(snapshotPath, sizeof(snapshotPath), "%s/%s_snapshot.txt", outputDir, strrchr(argv[i], '/') ? strrchr(argv[i], '/') + 1 : argv[i]);

        FILE *outputFile = fopen(snapshotPath, "w");
        if (!outputFile) {
            perror("Eroare la deschidere fisier de iesire");
            continue;
        }

        
        listDirectory(argv[i], snapshotPath, 0);
        fclose(outputFile);
        printf("Snapshot-ul directorului %s a fost salvat in %s\n", argv[i], snapshotPath);
    }

    return 0;
}

