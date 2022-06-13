
// Change the wallpaper to create an animation.

#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdbool.h>
#include <dirent.h>
#include <signal.h>
#include <sys/time.h>

#include "main.h"

/*
 * Supported Desktop Environment(s):
 * XFCE
 */

bool shouldExit;
int DE;
char *command;

void signalHandler(int sig){
    // Handle SIGINT and SIGTERM
    printf("\nExiting...\n");
    shouldExit = true;
    return;
}

int main()
{
    struct settings settings = readSettings();
    printf("Settings:\n");
    printf("FPS: %d\n", settings.fps);
    printf("Path: %s\n", settings.filepath);

    command = malloc(sizeof(char) * PATH_MAX);
    int currFrame = 1;
    unsigned long long sleepTime = 1000000 / (settings.fps*2);
    shouldExit = false;

    // Get the current wallpaper
    char *de = getenv("XDG_CURRENT_DESKTOP");
    if (de == NULL)
    {
        printf("Error: Could not get current desktop environment.\n");
        return 1;
    }
    if (strcmp(de, "XFCE") == 0)
    {
        DE = XFCE;
    }
    else
    {
        printf("Error: Unsupported desktop environment.\n");
        return 1;
    }


    // Make directory in /tmp/wallpaper if it doesn't exist.
    char path[PATH_MAX];
    strcpy(path, "/tmp/wallpaper");
    if (mkdir(path, 0755) == -1)
    {
        if (errno != EEXIST)
        {
            perror("mkdir");
            return 1;
        }
    }

    // Extract the frames from the video
    extractFrames(settings);

    settings.totalFrames = getTotalFrames();

    // Get current wallpaper
    char *currentWallpaper = getCurrentWallpaper(settings);

    // Set shoudExit when signal to terminate is received.
    if (signal(SIGINT, signalHandler) == SIG_ERR){
        printf("Can't catch SIGINT\n");
        return 1;
    }
    if(signal(SIGTERM, signalHandler) == SIG_ERR){
        printf("Can't catch SIGTERM\n");
        return 1;
    }


    // Main loop
    while (shouldExit == false){
        
        if(currFrame == settings.totalFrames)
            currFrame = 1;
        
        // Get the current frame
        char frame[PATH_MAX];
        snprintf(frame, PATH_MAX, "/tmp/wallpaper/%d.jpeg", currFrame);

        // Set the wallpaper and get the execution time 
        struct timeval start = GetTimeStamp(); 
        setWallpaper(frame, settings);
        struct timeval end = GetTimeStamp();
        unsigned long long executionTime = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
        currFrame++;
        if (executionTime < sleepTime)
        {
            usleep(sleepTime - executionTime);
        }
        else {
            printf("Warning: Execution time is greater than sleep time.\n");
        }
    }

    // Set the current wallpaper back to the original
    setWallpaper(currentWallpaper, settings);

    free(settings.filepath);
    free(settings.configfile);
    free(settings.xfceMonitor);
    free(command);
    free(currentWallpaper);

    return 0;
}

struct settings readSettings()
{
    // Get the path to the config file if its in the current directory.
    char *confInCWD = malloc(PATH_MAX + 32);
    char *cwd = malloc(PATH_MAX);
    getcwd(cwd, PATH_MAX);
    snprintf(confInCWD, PATH_MAX + 32, "%s/animated-wallpaper.conf", cwd);

    // Initialize the settings struct.
    struct settings settings;
    settings.configfile = malloc(sizeof(char) * (PATH_MAX + 32));
    settings.xfceMonitor = malloc(sizeof(char) * 256);

    // The config file is /etc/animated-wallpaper.conf unless animated-wallpaper.conf is present in the current directory.
    if (access(confInCWD, F_OK) == 0)
    {
        strcpy(settings.configfile, confInCWD);
    }
    else
    {
        strcpy(settings.configfile, "/etc/animated-wallpaper.conf");
    }

    // Read the config file for the path to the video file.

    FILE *fp = fopen(settings.configfile, "r");
    if (fp == NULL)
    {
        printf("Error opening config file.\n");
        exit(1);
    }

    char line[1024];
    settings.fps = 0;
    settings.filepath = malloc(sizeof(char) * PATH_MAX);
    while (fgets(line, 1024, fp) != NULL)
    {
        if (strstr(line, "path=") != NULL)
        {
            strcpy(settings.filepath, strstr(line, "=") + 1);
            if (settings.filepath[strlen(settings.filepath) - 1] == '\n')
            {
                settings.filepath[strlen(settings.filepath) - 1] = '\0';
            }
            else
            {
                settings.filepath[strlen(settings.filepath)] = '\0';
            }
        }
        if (strstr(line, "fps=") != NULL)
        {
            settings.fps = atoi(strstr(line, "=") + 1);
        }
        if (strstr(line, "quality=") != NULL)
        {
            settings.quality = atoi(strstr(line, "=") + 1);
        }
        if (strstr(line, "xfce-screen=") != NULL)
        {
            settings.xfceScreen = atoi(strstr(line, "=") + 1);
        }
        if (strstr(line, "xfce-monitor=") != NULL)
        {
            strcpy(settings.xfceMonitor, strstr(line, "=") + 1);
            if (settings.xfceMonitor[strlen(settings.xfceMonitor) - 1] == '\n')
            {
                settings.xfceMonitor[strlen(settings.xfceMonitor) - 1] = '\0';
            }
            else
            {
                settings.xfceMonitor[strlen(settings.xfceMonitor)] = '\0';
            }
        }
        if (strstr(line, "xfce-workspace=") != NULL)
        {
            settings.xfceWorkspace = atoi(strstr(line, "=") + 1);
        }
    }

    fclose(fp);

    free(cwd);
    free(confInCWD);
    
    return settings;
}

void extractFrames(struct settings settings){
    /* 
     * Check if frames are already extracted by extracting the first frame and
     * checking if it is the same size as the already extracted frame.
     */
    char path[PATH_MAX];
    char command[PATH_MAX + 128];

    strcpy(path, "/tmp/wallpaper/1.jpeg");
    if (access(path, F_OK) == 0)
    {
        snprintf(command, PATH_MAX + 128, "ffmpeg -i %s -y -loglevel 24 -qmin 1 -qscale:v %d -r 1/1 -frames:v 1 /tmp/wallpaper/compare.jpeg", settings.filepath, settings.quality);
        system(command);
        if (access("/tmp/wallpaper/compare.jpeg", F_OK) == 0)
        {
            // Now check if compare.jpeg is the same as the 1.jpeg by comparing file sizes.
            FILE *file1 = fopen("/tmp/wallpaper/compare.jpeg", "r");
            FILE *file2 = fopen("/tmp/wallpaper/1.jpeg", "r");

            if (fseek(file1, 0, SEEK_END) == 0 && fseek(file2, 0, SEEK_END) == 0)
            {
                long size1 = ftell(file1);
                long size2 = ftell(file2);
                fclose(file1);
                fclose(file2);
                // Remove compare.jpeg
                remove("/tmp/wallpaper/compare.jpeg");
                if (size1 == size2)
                {
                    printf("Frames already extracted.\n");
                    return;
                }
                else
                {
                    printf("Files not the same, re-extracting frames\n");
                    // Clear the frames directory
                    system("rm -rf /tmp/wallpaper/*");
                    snprintf(command, PATH_MAX + 128, "ffmpeg -i %s -y -loglevel 24 -qmin 1 -qscale:v %d -vf fps=%d /tmp/wallpaper/%%d.jpeg", settings.filepath, settings.quality, settings.fps);
                    system(command);
                    return;
                }
            }
            else 
            {
                printf("Error checking file sizes.\n");
                exit(1);
            }
        }
        else
        {
            printf("Error extracting frames.\n");
            exit(1);
        }
    }
    else
    {
        printf("Extracting frames\n");
        snprintf(command, PATH_MAX + 128, "ffmpeg -i %s -y -loglevel 24 -qmin 1 -qscale:v %d -vf fps=%d /tmp/wallpaper/%%d.jpeg ", settings.filepath, settings.quality, settings.fps);
        system(command);
    }

}

int getTotalFrames(void){
    // List files in /tmp/wallpaper and count how many there are.
    DIR *dir;
    struct dirent *ent;
    int count = 1;

    dir = opendir("/tmp/wallpaper");
    if (dir != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            if (strstr(ent->d_name, ".jpeg") != NULL)
            {
                count++;
            }
        }
        closedir(dir);
    }
    else
    {
        perror("opendir");
        exit(1);
    }
    return count;
}

void setWallpaper(char *frame, struct settings settings){
    switch (DE)
    {
        case XFCE:
            // Get the wallpaper using xfce4-desktop.
            snprintf(command, PATH_MAX, "xfconf-query -c xfce4-desktop -p /backdrop/screen%d/monitor%s/workspace%d/last-image -s %s", settings.xfceScreen, settings.xfceMonitor, settings.xfceWorkspace, frame);
            break;
        default:
            printf("Error getting DE.\n");
            exit(1);
    }

    system(command);

    return;
}

char *getCurrentWallpaper(struct settings settings){
    char *wallpaper = malloc(sizeof(char) * PATH_MAX);

    switch (DE)
    {
        case XFCE:
            // Get the wallpaper using xfce4-desktop.
            snprintf(command, PATH_MAX, "xfconf-query -c xfce4-desktop -p /backdrop/screen%d/monitor%s/workspace%d/last-image", settings.xfceScreen, settings.xfceMonitor, settings.xfceWorkspace);
            break;
        default:
            printf("Error getting DE.\n");
            exit(1);
    }
    
    FILE *fp = popen(command, "r");
    if (fp == NULL)
    {
        printf("Error getting wallpaper.\n");
        exit(1);
    }
    fgets(wallpaper, PATH_MAX, fp);
    pclose(fp);

    return wallpaper;
}

struct timeval GetTimeStamp() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv;
}