struct settings{
    int fps;
    int quality;
    char *filepath;
    char *configfile;
    int totalFrames;
    int xfceScreen;
    char *xfceMonitor;
    int xfceWorkspace;
};

struct settings readSettings();
void extractFrames(struct settings settings);
int getTotalFrames(void);
void setWallpaper(char *frame, struct settings settings);
char *getCurrentWallpaper(struct settings settings);
void signalHandler(int sig);
struct timeval GetTimeStamp();

enum{
    XFCE,
    GNOME
};