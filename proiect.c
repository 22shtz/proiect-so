#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
/*Proiect: Monitorizare
a modificarilor aparute in directoare de-a lungul timpului prin realizarea de capturi (snapshots) la cererea utilizatorului.

Sapt 1: - Utilizatorul va putea specifica directorul de monitorizat ca argument in linia de comanda iar programul va urmari schimbarile care apar in acesta si in subdirectoarele sale.
- La fiecare rulare a programului se va actualiza captura (snapshot-ul) directorului stocand metadatele fiecarei intrari.*/

void listDirectory(const char *dirName, FILE *outputFile, int level)
{
    DIR *dir;
    struct dirent *entry;
    struct stat info;

    if(!(dir=opendir(dirName)))
    {
        return;
    }

    while((entry=readdir(dir))!=NULL)
    {
        char path[1024];
        if(level>0)
        {
            snprintf(path, sizeof(path), "%s/%s", dirName, entry->d_name);
        }
        else
        {
            strncpy(path,entry->d_name,sizeof(path));
        }

        if(stat(path,&info)!=0)
        {
            fprintf(stderr,"Nu se poate accesa %s \n", path);
            continue;
        }

        if(strcmp(entry->d_name,".")==0|| strcmp(entry->d_name,"..")==0)
        {
            continue;
        }

        fprintf(outputFile,"%*s-%s\n", level * 4, "", entry->d_name);
        if(S_ISDIR(info.st_mode))
        {
            listDirectory(path, outputFile, level+1);
        }
    }

    closedir(dir);

}
int main(int argc, char **argv)
{
     printf("bla");

    if(argc<2)
    {
        fprintf(stderr,"Utilizare: %s \n", argv[0]);
        return 1;
    }

    FILE *outputFile=fopen("directory_snapshot.txt","w+");
    if(!outputFile)
    {
        perror("Eroare la deschidere fisier de iesire. :(");
        return 2;
    }

    listDirectory(argv[1],outputFile,0);
    fclose(outputFile);
    printf("Snapshot-ul directorului a fost salvat in 'directory_snapshot.txt'\n");

    return 0;
}
