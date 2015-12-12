#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <time.h>

Pixmap getWallpaperPixmap(Display *display, Window root)
{
    Atom prop_root, prop_esetroot, type;
    int format;
    unsigned long length, after;
    unsigned char *data_root, *data_esetroot;

    prop_root = XInternAtom(display, "_XROOTPMAP_ID", True);
    prop_esetroot = XInternAtom(display, "ESETROOT_PMAP_ID", True);

    //The current pixmap being used in the atom
    Pixmap pixmap = NULL;

    if (prop_root != None && prop_esetroot != None) {
        XGetWindowProperty(display, root, prop_root, 0L, 1L, False, AnyPropertyType, &type, &format, &length, &after, &data_root);
        if (type == XA_PIXMAP) {
            XGetWindowProperty(display, root, prop_esetroot, 0L, 1L, False, AnyPropertyType, &type, &format, &length, &after, &data_esetroot);
            if (data_root && data_esetroot) {
                if (type == XA_PIXMAP && *((Pixmap *)data_root) == *((Pixmap *)data_esetroot)) {
                    //Record it
                    pixmap = *((Pixmap *)data_root);
                }
            }
            XFree(data_esetroot);
        }
        XFree(data_root);
    }

    return pixmap;
}

static inline int
setWallpaper(Display *display, Window root, int depth, unsigned int width, unsigned int height, const char *pixels)
{
    Pixmap pixmapCurrent = getWallpaperPixmap(display, root);

    //Let's overwrite the properties with our pixmap now
    Atom prop_root, prop_esetroot;
    prop_root = XInternAtom(display, "_XROOTPMAP_ID", False);
    prop_esetroot = XInternAtom(display, "ESETROOT_PMAP_ID", False);

    if (prop_root == None || prop_esetroot == None) {
        fprintf(stderr, "error: creation of wallpaper property failed.");
        return 1;
    }

    //Create the pixmap
    GC gc = XCreateGC(display, root, 0, 0);
    Pixmap pixmap = XCreatePixmap(display, root, width, height, depth);
    XImage *img = XCreateImage(display, CopyFromParent, depth, ZPixmap, 0, (char *)pixels, width, height, 32, 0);
    XPutImage(display, pixmap, gc, img, 0, 0, 0, 0, width, height);
    img->data = NULL;
    XDestroyImage(img);
    XFreeGC(display, gc);

    //Change the properties for the compositor
    XChangeProperty(display, root, prop_root, XA_PIXMAP, 32, PropModeReplace, (unsigned char *)&pixmap, 1);
    XChangeProperty(display, root, prop_esetroot, XA_PIXMAP, 32, PropModeReplace, (unsigned char *)&pixmap, 1);

    //Change the root window
    XSetWindowBackgroundPixmap(display, root, pixmap);
    XClearWindow(display, root);
    XFlush(display);
    XKillClient(display, pixmapCurrent);

    return 0;
}

static inline int
getWallpaper(Display *display, Window root, unsigned int *width, unsigned int *height, char **pixels)
{
    Pixmap pixmap = getWallpaperPixmap(display, root);

    //Get the root window size
    XWindowAttributes xwa;
    XGetWindowAttributes(display, root, &xwa);
    *width = xwa.width;
    *height = xwa.height;

    *pixels = malloc(*width * *height * 4);
    memset(*pixels, 0, *width * *height * 4);

    //Now read the data from the pixmap
    XImage *img = XGetImage(display, pixmap, 0, 0, *width, *height, AllPlanes, ZPixmap);

    //Copy data
    int x, y;
    for (y = 0; y < *height; y++) {
        for (x = 0; x < *width; x++) {
            unsigned long pixel = XGetPixel(img, x, y);
            memcpy(*pixels + (y * *width + x) * 4, &pixel, 3);
        }
    }

    return 0;
}

int
main(int argc, char **argv)
{
    if (argc < 2) {
        return 1;
    }

    int rc = 0;

    //X11
    Display *display = NULL;
    Screen *screen = NULL;
    Window root = NULL;
    int depth = 0;

    //Image data
    unsigned int width, height;
    char *pixels;

    //Ready the display
    if (!(display = XOpenDisplay(NULL))) {
        fprintf(stderr, "error: could not open X display.\n");
        rc = 1;
        goto end;
    }

    //Ready X11 stuff
    XSetCloseDownMode(display, RetainPermanent);
    screen = DefaultScreenOfDisplay(display);
    root = DefaultRootWindow(display);
    depth = DefaultDepthOfScreen(screen);
    if (depth < 24) {
        fprintf(stderr, "error: color depth must be at least 24.\n");
        rc = 1;
        goto end;
    }

    if (!strcmp(argv[1], "set")) {
        if (argc < 3) {
            rc = 1;
            goto end;
        }

        //Read raw image data
        FILE *f = fopen(argv[2], "rb");
        fread(&width, 4, 1, f);
        fread(&height, 4, 1, f);
        pixels = malloc(width * height * 4);
        fread(pixels, width * height * 4, 1, f);
        fclose(f);

        rc = setWallpaper(display, root, depth, width, height, pixels);
        free(pixels);
    } else if (!strcmp(argv[1], "get")) {
        rc = getWallpaper(display, root, &width, &height, &pixels);

        //Save data
        srand(time(NULL));
        char filename[256];
        sprintf(filename, "%d", rand());
        FILE *f = fopen(filename, "wb");
        fwrite(&width, 4, 1, f);
        fwrite(&height, 4, 1, f);
        fwrite(pixels, width * height * 4, 1, f);
        fclose(f);

        char *cwd = getcwd(NULL, 0);
        printf("%s/%s\n", cwd, filename);
        free(cwd);

        free(pixels);
    } else {
        rc = 1;
    }

end:
    if (display) {
        XCloseDisplay(display);
    }
    return rc;
}
