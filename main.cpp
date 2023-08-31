#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <iostream>
#include <cstring>

void printSelection(Display* display, Window window) {
    static std::string lastSelection;
    Atom type;
    int format;
    unsigned long len, bytes_left, dummy;
    unsigned char *data;
    Atom selection = XInternAtom(display, "CLIPBOARD", False);
    Atom utf8 = XInternAtom(display, "UTF8_STRING", False);

    XConvertSelection(display, selection, utf8, None, window, CurrentTime);
    XFlush(display);

    XEvent event;
    XNextEvent(display, &event);

    if (event.type == SelectionNotify) {
        if (event.xselection.property) {
            XGetWindowProperty(display, window, event.xselection.property, 0, 0, False,
                               AnyPropertyType, &type, &format, &len, &bytes_left, &data);
            if (bytes_left > 0) {
                int result = XGetWindowProperty(display, window, event.xselection.property, 0, bytes_left, False,
                                                AnyPropertyType, &type, &format, &len, &dummy, &data);
                if (result == Success) {
                    std::string newSelection(reinterpret_cast<char*>(data));
                    if (newSelection != lastSelection) {
                        lastSelection = newSelection;
                        std::cout << "Clipboard updated: " << newSelection << std::endl;
                    }
                }
                XFree(data);
            }
        }
    }
}

void handleSelectionClear(XEvent event) {
    std::cout << "Lost ownership of clipboard" << std::endl;
}

void handleSelectionRequest(Display* display, XEvent event) {
    XSelectionRequestEvent* req = &event.xselectionrequest;

    if (req->target == XA_STRING) {
        const char* text = "Hello, clipboard!";
        XChangeProperty(display, req->requestor, req->property, XA_STRING, 8, PropModeReplace, (unsigned char*)text, strlen(text));
    } else {
        XChangeProperty(display, req->requestor, req->property, None, 0, PropModeReplace, NULL, 0);
    }

    XSelectionEvent sev;
    sev.type = SelectionNotify;
    sev.requestor = req->requestor;
    sev.selection = req->selection;
    sev.target = req->target;
    sev.property = req->property;
    sev.time = req->time;

    XSendEvent(display, req->requestor, True, NoEventMask, (XEvent*)&sev);
}

int main() {
    Display* display = XOpenDisplay(nullptr);
    if (display == nullptr) {
        std::cerr << "Cannot open display" << std::endl;
        return 1;
    }

    int screen_num = DefaultScreen(display);
    Window root = RootWindow(display, screen_num);

    Window window = XCreateSimpleWindow(display, root, 0, 0, 200, 200, 1,
                                        BlackPixel(display, screen_num), WhitePixel(display, screen_num));

    Atom clipboard = XInternAtom(display, "CLIPBOARD_MANAGER", False);
    XSetSelectionOwner(display, clipboard, window, CurrentTime);

    if (XGetSelectionOwner(display, clipboard) != window) {
        std::cerr << "Cannot get clipboard ownership" << std::endl;
        return 1;
    }

    while (true) {
        XEvent event;
        XNextEvent(display, &event);

        switch (event.type) {
            case SelectionClear:
                handleSelectionClear(event);
                break;
            case SelectionRequest:
                handleSelectionRequest(display, event);
                break;
            default:
                printSelection(display, window);
                break;
        }
    }


    XCloseDisplay(display);
    return 0;
}
