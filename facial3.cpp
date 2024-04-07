#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring> // Include for memset

extern "C" {
    #include "../Facials/emos/Normal_1.xpm"
    #include "../Facials/emos/Normal_2.xpm"
    #include "../Facials/emos/smile_1.xpm"
    #include "../Facials/emos/smile_2.xpm"
}

void createCenteredPixmap(Display* display, Window window, GC gc, char** xpmData, Pixmap* pixmap, unsigned int windowWidth, unsigned int windowHeight) {
    Pixmap tempPixmap;
    XpmCreatePixmapFromData(display, window, xpmData, &tempPixmap, NULL, NULL);

    // Retrieve the geometry of the created pixmap
    Window root;
    int x, y;
    unsigned int pixmapWidth, pixmapHeight, borderWidth, depth;
    XGetGeometry(display, tempPixmap, &root, &x, &y, &pixmapWidth, &pixmapHeight, &borderWidth, &depth);

    // Calculate the position to center the pixmap
    int posX = (windowWidth - pixmapWidth) / 2;
    int posY = (windowHeight - pixmapHeight) / 2;

    // Create a new pixmap for the window size
    *pixmap = XCreatePixmap(display, window, windowWidth, windowHeight, depth);
    XSetForeground(display, gc, XBlackPixel(display, DefaultScreen(display))); // Assuming black is the desired background color
    XFillRectangle(display, *pixmap, gc, 0, 0, windowWidth, windowHeight); // Fill the background

    // Copy the original pixmap to the new pixmap at the calculated position
    XCopyArea(display, tempPixmap, *pixmap, gc, 0, 0, pixmapWidth, pixmapHeight, posX, posY);

    // Cleanup the temporary pixmap
    XFreePixmap(display, tempPixmap);
}
void toggleFullScreen(Display* display, Window window) {
    Atom wmState = XInternAtom(display, "_NET_WM_STATE", False);
    Atom wmFullScreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);

    XEvent xev;
    xev.xclient.type = ClientMessage;
    xev.xclient.serial = 0;
    xev.xclient.send_event = True;
    xev.xclient.window = window;
    xev.xclient.message_type = wmState;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 2; // _NET_WM_STATE_TOGGLE
    xev.xclient.data.l[1] = wmFullScreen;
    xev.xclient.data.l[2] = 0; // No second property to toggle
    xev.xclient.data.l[3] = 1; // Normal application

    XSendEvent(display, DefaultRootWindow(display), False, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
}
int main() {
   Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Cannot open display\n";
        return -1;
    }

    int screen = DefaultScreen(display);
    unsigned int width = DisplayWidth(display, screen); // Full screen width
    unsigned int height = DisplayHeight(display, screen); // Full screen height

    // Create a borderless window
    XSetWindowAttributes attrs;
    attrs.override_redirect = True; // This removes the window border and decorations, making it borderless.
    
    Window window = XCreateWindow(display, RootWindow(display, screen), 0, 0, width, height, 0,
                                  CopyFromParent, InputOutput, CopyFromParent,
                                  CWOverrideRedirect, &attrs);
    XMapWindow(display, window);

    GC gc = XCreateGC(display, window, 0, nullptr);

    // Assuming you have XPM images named Normal_1_xpm and Normal_2_xpm
    Pixmap pixmap1, pixmap2;
    createCenteredPixmap(display, window, gc, Normal_1_xpm, &pixmap1, width, height);
    createCenteredPixmap(display, window, gc, Normal_2_xpm, &pixmap2, width, height);

    Pixmap currentPixmap = pixmap1;
    bool running = true;
    auto lastUpdate = std::chrono::steady_clock::now();
    bool toggle = false;

    XSelectInput(display, window, ExposureMask | KeyPressMask);

    while (running) {
        if (XPending(display)) {
            XEvent event;
            XNextEvent(display, &event);
            if (event.type == Expose) {
                XCopyArea(display, currentPixmap, window, gc, 0, 0, width, height, 0, 0);
            }
            if (event.type == KeyPress) {
                KeySym key = XLookupKeysym(&event.xkey, 0);
                if (key == XK_Escape) {
                    running = false;
                    exit(0); // Exit the program successfully
                } else if (key == XK_f) {
                    toggleFullScreen(display, window);
                }
                else if (key == XK_F1){
                    createCenteredPixmap(display, window, gc, smile_1_xpm, &pixmap1, width, height);
                    createCenteredPixmap(display, window, gc, smile_2_xpm, &pixmap2, width, height);
                }
                else if (key == XK_F2){
                    createCenteredPixmap(display, window, gc, Normal_1_xpm, &pixmap1, width, height);
                    createCenteredPixmap(display, window, gc, Normal_2_xpm, &pixmap2, width, height);
                }
            }
        } else {
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate).count() > 500) { // Toggle every 500ms
                lastUpdate = now;
                toggle = !toggle;
                currentPixmap = toggle ? pixmap2 : pixmap1;
                XCopyArea(display, currentPixmap, window, gc, 0, 0, width, height, 0, 0);
                XFlush(display); // Ensure the update is immediately visible
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Reduce CPU usage
        }
    }

    XFreePixmap(display, pixmap1);
    XFreePixmap(display, pixmap2);
    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);

    return 0;
}
