#include <png.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <X11/Xutil.h>
#include <linux/limits.h>
#include <alsa/asoundlib.h>

#define VERSION "v0.1.0"

#define CFG_KEYMAP_QUERY 10000
#define CFG_KEY_PRTSC    107
#define CFG_KEY_VOLDN    122
#define CFG_KEY_VOLUP    123
#define CFG_ALSA_STEP    0.05
#define ENV_VAR_IMGPATH  "LOWKEY_IMGPATH"
#define ENV_VAR_EXECCMD  "LOWKEY_EXECCMD"

void screenshotExport (const char* path, unsigned char* data, int w, int h) {
    FILE *fp;
    png_structp png;
    png_infop info;

    fp = fopen(path, "wb");
    if (!fp) {
        fprintf(stderr, "[lowkey] Failed to save screenshot\n");
        return;
    }

    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    info = png_create_info_struct(png);

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        fprintf(stderr, "[lowkey] Internal PNG error\n");
        fclose(fp);
        return;
    }

    png_init_io(png, fp);
    png_set_IHDR(png, info, w, h, 8,
        PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );

    png_write_info(png, info);
    for (int y = 0; y < h; y++) {
        png_bytep row = data + y * w * 3;
        png_write_row(png, row);
    }

    png_write_end(png, NULL);
    png_destroy_write_struct(&png, &info);
    fclose(fp);
}

void screenshotGrab (Display *display) {
    time_t timestamp;
    struct tm *datetime;

    char imagePath[PATH_MAX];

    XImage *image;
    unsigned char *data;

    Window window;
    int screen, w, h;

    screen = DefaultScreen(display);
    window = RootWindow(display, screen);
    w = DisplayWidth(display, screen);
    h = DisplayHeight(display, screen);

    image = XGetImage(display, window, 0, 0, w, h, AllPlanes, ZPixmap);
    if (!image) {
        fprintf(stderr, "[lowkey] Failed to capture screen\n");
        return;
    }

    data = (unsigned char *)malloc(w * h * 3);
    if (!data) {
        fprintf(stderr, "[lowkey] Failed to allocate memory for image\n");
        XDestroyImage(image);
        return;
    }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int pixelIndex = (y * w + x) * 3;
            unsigned long pixel = *(unsigned long *)
                (image->data + y * image->bytes_per_line + x * (image->bits_per_pixel / 8));
            data[pixelIndex + 0] = (pixel & image->red_mask) >> 16;
            data[pixelIndex + 1] = (pixel & image->green_mask) >> 8;
            data[pixelIndex + 2] = (pixel & image->blue_mask);
        }
    }

    time(&timestamp);
    datetime = localtime(&timestamp);
    strftime(imagePath, PATH_MAX, getenv(ENV_VAR_IMGPATH), datetime);

    screenshotExport(imagePath, data, w, h);
    free(data);
    XDestroyImage(image);
}

void volumeAdjust (int modifier, double multiplier) {
    long volMin, volMax, vol;

    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    snd_mixer_elem_t* elem;

    if (snd_mixer_open(&handle, 0) < 0) {
        fprintf(stderr, "[lowkey] Failed to open ALSA mixer\n");
        return;
    }

    if (snd_mixer_attach(handle, "default") < 0) {
        fprintf(stderr, "[lowkey] Soundcard binding failed\n");
        return;
    }

    if (snd_mixer_selem_register(handle, NULL, NULL) < 0) {
        fprintf(stderr, "[lowkey] Failed to register ALSA mixer elements\n");
        return;
    }

    if (snd_mixer_load(handle) < 0) {
        fprintf(stderr, "[lowkey] Failed to load ALSA mixer elements\n");
        return;
    }

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, "Master");
    elem = snd_mixer_find_selem(handle, sid);

    if (!elem) {
        fprintf(stderr, "[lowkey] Failed to find ALSA Master element\n");
        return;
    }

    snd_mixer_selem_get_playback_volume_range(elem, &volMin, &volMax);
    snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_MONO, &vol);

    vol = vol + modifier * (volMax - volMin) * multiplier;
    if (vol < volMin) {
        vol = volMin;
    }
    if (vol > volMax) {
        vol = volMax;
    }

    snd_mixer_selem_set_playback_volume_all(elem, vol);
    snd_mixer_close(handle);
}

bool isPressed (char keymap[32], int key) {
    return keymap[key / 8] & (1 << (key % 8));
}

void keyHandler () {
    Display *display;

    char keymap[32];
    bool pressedPrevPrtsc, pressedCurrPrtsc;
    bool pressedPrevVolDn, pressedCurrVolDn;
    bool pressedPrevVolUp, pressedCurrVolUp;

    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "[lowkey] Failed to open X11 display\n");
        return;
    }

    pressedPrevPrtsc = false;
    pressedCurrPrtsc = false;
    pressedPrevVolDn = false;
    pressedCurrVolDn = false;
    pressedPrevVolUp = false;
    pressedCurrVolUp = false;

    fprintf(stdout, "[lowkey] Key handler running\n");

    while (1) {
        usleep(CFG_KEYMAP_QUERY);
        XQueryKeymap(display, keymap);
        pressedCurrPrtsc = isPressed(keymap, CFG_KEY_PRTSC);
        pressedCurrVolDn = isPressed(keymap, CFG_KEY_VOLDN);
        pressedCurrVolUp = isPressed(keymap, CFG_KEY_VOLUP);
        if (!pressedPrevPrtsc && pressedCurrPrtsc) {
            screenshotGrab(display);
        }
        if (!pressedPrevVolDn && pressedCurrVolDn) {
            volumeAdjust(-1, CFG_ALSA_STEP);
        }
        if (!pressedPrevVolUp && pressedCurrVolUp) {
            volumeAdjust(1, CFG_ALSA_STEP);
        }
        pressedPrevPrtsc = pressedCurrPrtsc;
        pressedPrevVolDn = pressedCurrVolDn;
        pressedPrevVolUp = pressedCurrVolUp;
    }

    XCloseDisplay(display);
}

void cmdRunner () {
    fprintf(stdout, "[lowkey] Command running\n");
    system(getenv(ENV_VAR_EXECCMD));
}

bool checkXinit () {
    FILE *fp;
    pid_t ppid;
    char procPath[PATH_MAX];
    char procName[PATH_MAX];

    ppid = getppid();
    sprintf(procPath, "/proc/%d/comm", ppid);

    fp = fopen(procPath, "r");
    if (!fp) {
        fprintf(stderr, "[lowkey] Failed to perform Xinit check\n");
        return false;
    }

    fgets(procName, PATH_MAX, fp);
    fclose(fp);

    procName[strcspn(procName, "\n")] = 0; 
    return !strcmp(procName, "xinit");
}

int main () {
    pthread_t handlerID, runnerID;

    if (!getenv(ENV_VAR_IMGPATH)) {
        fprintf(stderr, "[lowkey] %s is not set\n", ENV_VAR_IMGPATH);
        return 1;
    }

    if (!getenv(ENV_VAR_EXECCMD)) {
        fprintf(stderr, "[lowkey] %s is not set\n", ENV_VAR_EXECCMD);
        return 1;
    }

    if (!checkXinit()) {
        fprintf(stderr, "[lowkey] Lowkey can only be started by xinit\n");
        return 1;
    }

    if (pthread_create(&handlerID, NULL, (void *)keyHandler, NULL) != 0) {
        fprintf(stderr, "[lowkey] Failed to create key handler thread\n");
        return 1;
    }

    if (pthread_create(&runnerID, NULL, (void *)cmdRunner, NULL) != 0) {
        fprintf(stderr, "[lowkey] Failed to create command runner thread\n");
        return 1;
    }

    pthread_join(runnerID, NULL);
    return 0;
}