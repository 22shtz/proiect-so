#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

void snapshot_(char* name, int count){
    printf("%s\n",name);
    char filename[50];
    sprintf(filename, "snapshot%d.txt", count);
    FILE *snapshot_file = fopen(filename, "w");
    if(snapshot_file == NULL) {
        perror("Cannot create snapshot file");
        return;
    }

    fprintf(snapshot_file, "Snapshot for folder: %s\n", name);
    
    struct dirent *pf;
    DIR *folder = opendir(name);
    if(folder == NULL){
        perror("Can't open folder");
        fclose(snapshot_file);
        return;
    }
 
    while((pf = readdir(folder)) != NULL){
		fprintf(snapshot_file, "|_ %s\n", pf->d_name);
		struct stat st;
		char path[300];
		sprintf(path,"%s/%s", name, pf->d_name);
		if(stat(path, &st) == -1){
		    perror("Error at stat");
		    fclose(snapshot_file);
		    closedir(folder);
		    return;
		}
		if(S_ISREG(st.st_mode) == 1){
			FILE *text_file = fopen(path, "r");
			char buffer[1024];
			while (fgets(buffer, sizeof(buffer), text_file) != NULL) {
				fputs(buffer, snapshot_file); //open,read & write
			}

		}
        
    }
    closedir(folder);
    fclose(snapshot_file);
}

int readfolder(char* name,int count){
    printf("%s\n",name);
    struct dirent *pf;
    DIR *folder = opendir(name);
    if(folder == NULL){
        perror("Can't open folder");
        return -1;
    }
    while((pf = readdir(folder)) != NULL){
        printf("|_ %s\n",pf->d_name);
        struct stat st;
        char path[300];
        sprintf(path,"%s/%s",name,pf->d_name);
        if(stat(path,&st) == -1){
            perror("Error at stat");
            return -1;
        }
        if(S_ISDIR(st.st_mode) == 1 && strcmp(pf->d_name, "..")!=0){
            snapshot_(path, count);
            count++;
        }
    }
    closedir(folder);
    return count;
}

void processes(int argc, char** argv){
    int count = 1;
    for(int i=1; i<argc; i++){
        int pid= fork();
        if(pid < 0){
            perror("Error at fork");
            return;
        }
        if(pid == 0){
            count = readfolder(argv[i],count);
            exit(0);
        }
    }
    for(int i=1; i<argc; i++){
        int status;
        wait(&status);
    }
    
}

int main(int argc, char** argv){
    if(argc < 2){
        perror("Expected folder as argument");
        return 1;
    }
    processes(argc,argv);
    return 0;
}
