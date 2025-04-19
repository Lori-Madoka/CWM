// g++ -o cluncwm cluncwm.cpp -lX11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>

#define MODKEY Mod4Mask // Super key (Windows key)
#define TERMINAL "kitty"
#define TERMINAL_WIDTH 800
#define TERMINAL_HEIGHT 600
#define DISPLAY_WIDTH 1366
#define DISPLAY_HEIGHT 768

Display* display;
Window root;

Window focusedWindow = None;
Window draggingWindow = None;
Window rezizeWindow = None;
int dragStartX = 0, dragStartY = 0;  // Mouse position at the start of dragging
int zizeStartX = 0, zizeStartY = 0; // rezizse position at start of rezize
int winStartX = 0, winStartY = 0;    // Window position at the start of dragging
int winStartW = 0, winStartH = 0;

void setup();
void run();
void handleEvent(XEvent* event);
void handleKeyPress(XKeyEvent* event);
void handleButtonPress(XButtonEvent* event);
void handleButtonRelease(XButtonEvent* event);
void handleMotionNotify(XMotionEvent* event);
void handleMapRequest(XMapRequestEvent* event);
void handleConfigureRequest(XConfigureRequestEvent* event);
void focusWindow(Window window);
void closeWindow(Window window);
void identifyPlaace(int pos, Window window);
int quereydesktop();
void handleDestroyRequest(XDestroyWindowEvent* event);
void changedesktop(int desknum);
void spawn(void* argz);
void setCursor();

// Node class to represent each element in the linked list
class Node {
public:
    Window win;
    int x, y;  // Window position
    Node* next;

    Node(Window window, int xPos, int yPos) : win(window), x(xPos), y(yPos), next(nullptr) {}
};

// LinkedList class to manage the nodes
class LinkedList {
private:
    Node* head;  // Head of the linked list

public:
    LinkedList() : head(nullptr) {}

    // Add a new node at the end of the list with initial window position
    void append(Window window) {
        XWindowAttributes windowAttrs;
        XGetWindowAttributes(display, window, &windowAttrs);
        int x = windowAttrs.x;
        int y = windowAttrs.y;

        Node* newNode = new Node(window, x, y);
        if (!head) {
            head = newNode;
        } else {
            Node* temp = head;
            while (temp->next) {
                temp = temp->next;
            }
            temp->next = newNode;
        }
    }

    // Find a node in the linked list
    Node* find(Window window) {
        Node* temp = head;
        while (temp) {
            if (temp->win == window)
                return temp;
            temp = temp->next;
        }
        return nullptr;
    }

    // Remove a node from the linked list
    void remove(Window window) {
        Node* temp = head;
        Node* prev = nullptr;
        while (temp) {
            if (temp->win == window) {
                if (prev) {
                    prev->next = temp->next;
                } else {
                    head = temp->next;
                }
                delete temp;
                return;
            }
            prev = temp;
            temp = temp->next;
        }
    }

    // Getter for the head node of the linked list
    Node* getHead() const {
        return head;
    }

    // Destructor to free the memory
    ~LinkedList() {
        Node* current = head;
        Node* nextNode;
        while (current) {
            nextNode = current->next;
            delete current;
            current = nextNode;
        }
    }
};

LinkedList windows;
LinkedList desktop1;
LinkedList desktop2;
LinkedList desktop3;
LinkedList desktop4;
LinkedList desktop5;
LinkedList desktop6;
LinkedList desktop7;
LinkedList desktop8;

int currentdesk = 1;

int main() {
    display = XOpenDisplay(NULL);
    if (!display) {
        std::cerr << "Cannot open display\n";
        return 1;
    }
    windows = desktop1;
    desktop1 = windows;
    
    root = RootWindow(display, DefaultScreen(display));

    setup();
    run();

    XCloseDisplay(display);
    return 0;
}

void setup() {
    XSelectInput(display, root, SubstructureRedirectMask | SubstructureNotifyMask | ButtonPressMask | PointerMotionMask);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("T")), MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("Q")), MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("E")), MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("Z")), MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("C")), MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("S")), MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("C")), (MODKEY|ShiftMask), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("1")), (MODKEY), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("2")), (MODKEY), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("3")), (MODKEY), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("4")), (MODKEY), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("5")), (MODKEY), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("6")), (MODKEY), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("7")), (MODKEY), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("8")), (MODKEY), root, True, GrabModeAsync, GrabModeAsync);
    
    // Do not grab Button1 or Button3 without MODKEY
    XUngrabButton(display, Button1, AnyModifier, root);
    XUngrabButton(display, Button3, AnyModifier, root);
    
    XGrabButton(display, Button1, MODKEY, root, True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    //XGrabButton(display, Button1, 0, window, True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(display, Button3, MODKEY, root, True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    setCursor();
}

void setCursor() {
    Cursor cursor;
    cursor = XCreateFontCursor(display, XC_left_ptr);
    XDefineCursor(display, root, cursor);
    XFlush(display);
}

void run() {
    XEvent event;
    while (true) {
        XNextEvent(display, &event);
        handleEvent(&event);
        usleep(1600); // limit for stability
    }
}

void handleEvent(XEvent* event) {
    switch (event->type) {
        case KeyPress:
            handleKeyPress(&event->xkey);
            break;
        case ButtonPress:
            handleButtonPress(&event->xbutton);
            break;
        case ButtonRelease:
            handleButtonRelease(&event->xbutton);
            break;
        case MotionNotify:
            handleMotionNotify(&event->xmotion);
            break;
        case MapRequest:
            handleMapRequest(&event->xmaprequest);
            break;
        case ConfigureRequest:
            handleConfigureRequest(&event->xconfigurerequest);
            break;
        case DestroyNotify:
            handleDestroyRequest(&event->xdestroywindow);
            break;
        default:
            break;
    }
}

void handleKeyPress(XKeyEvent* event) {
    if (event->keycode == XKeysymToKeycode(display, XK_T) && event->state == MODKEY) {
        const char* command[] = {TERMINAL, NULL};
        spawn((void*)command);
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_C) && event->state == (MODKEY | ShiftMask) && focusedWindow != None) {
        closeWindow(focusedWindow);
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_S) && event->state == MODKEY) {
        identifyPlaace(1, focusedWindow);
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_E) && event->state == MODKEY) {
        identifyPlaace(2, focusedWindow);
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_C) && event->state == MODKEY) {
        identifyPlaace(3, focusedWindow);
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_Z) && event->state == MODKEY) {
        identifyPlaace(4, focusedWindow);
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_Q) && event->state == MODKEY) {
        identifyPlaace(5, focusedWindow);
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_1) && event->state == MODKEY) {
        changedesktop(1);
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_2) && event->state == MODKEY) {
        changedesktop(2);
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_3) && event->state == MODKEY) {
        changedesktop(3);
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_4) && event->state == MODKEY) {
        changedesktop(4);
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_5) && event->state == MODKEY) {
        changedesktop(5);
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_6) && event->state == MODKEY) {
        changedesktop(6);
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_7) && event->state == MODKEY) {
        changedesktop(7);
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_8) && event->state == MODKEY) {
        changedesktop(8);
    }
}

void identifyPlaace(int pos, Window window){
	if (!window || window == None){
    	std::cout << "uh oh no let None window get manipulated cause otherwise crash" << std::endl;
        return;
	}
    XWindowAttributes windowAttrs;
	if (!XGetWindowAttributes(display, window, &windowAttrs)){
    	std::cout << "also big bad check for windowatttributes failed abysmally" << std::endl;
    	return;
	}
    XGetWindowAttributes(display, window, &windowAttrs);
    int gap = 8;
    int windimw, windimh, winnewx, winnewy;
    if (!window){
        return;
    }
    switch (pos) {
        case 1:// ceantre big boi onnly
            windimw = DISPLAY_WIDTH - (gap*2);
            windimh = DISPLAY_HEIGHT - (gap*2);
            winnewx = gap;
            winnewy = gap;
            break;
        case 2: // top right
            windimw = (DISPLAY_WIDTH/2) - gap;
            windimh = (DISPLAY_HEIGHT/2) - (1.5*gap);
            winnewx = (DISPLAY_WIDTH/2) + (gap/2);
            winnewy = (0) + gap;
            break;
        case 3: // bottom right
            windimw = (DISPLAY_WIDTH/2) - gap;
            windimh = (DISPLAY_HEIGHT/2) - (1.5*gap);
            winnewx = (DISPLAY_WIDTH/2) + (gap/2);
            winnewy = (DISPLAY_HEIGHT/2) + gap;
            break;
        case 4: // bottom left
            windimw = (DISPLAY_WIDTH/2) - gap;
            windimh = (DISPLAY_HEIGHT/2) - (1.5*gap);
            winnewx = gap;
            winnewy = (DISPLAY_HEIGHT/2) + gap;
            break;
        case 5: // top left 
            windimw = (DISPLAY_WIDTH/2) - gap;
            windimh = (DISPLAY_HEIGHT/2) - (1.5*gap);
            winnewx = gap;
            winnewy = gap;
            break;
    }
    XMoveResizeWindow(display, window, winnewx, winnewy, windimw, windimh);
    Node* node = windows.find(window);
        if (node) {
            node->x = winnewx;
            node->y = winnewy;
        }

}

// Move all windows offscreen without updating their positions in the linked list
void moveWindowsOffscreen(LinkedList& desktop) {
    Node* current = desktop.getHead();
    while (current) {
        // Store the original position of the window
        int originalX = current->x;
        int originalY = current->y;
        std::cout << originalX << originalY << std::endl;
        // Move the window offscreen
        XMoveWindow(display, current->win, originalX, -1000);


        current->x = originalX;
        current->y = originalY;

        // Optionally flush the X server to ensure the move happens immediately
        //XFlush(display);

        current = current->next;
    }
}

// Restore all windows to their original positions using the linked list
void restoreWindowsToPosition(LinkedList& desktop) {
    Node* current = desktop.getHead();
    while (current) {
        // Restore the window to its original position
        int originalX = current->x;
        int originalY = current->y;


        std::cout << originalX << originalY << std::endl;
        
        //XRaiseWindow(display, current->win);
        // Move the window back to its original position
        XMoveWindow(display, current->win, originalX, originalY);

        // Optionally flush the X server to ensure the move happens immediately
        //XFlush(display);
		if (current != NULL){
    	    focusWindow(current->win);
        }
        current = current->next;
    }

  
}

int quereydesktop(int currentdesk, int desktoswitch){
    //save the desktop to the current desktop
    switch(currentdesk) {
        case 1:
            desktop1 = windows;
            break;
        case 2:
            desktop2 = windows;
            break;
        case 3: 
            desktop3 = windows;
            break;
        case 4: 
            desktop4 = windows;
            break;
        case 5: 
            desktop5 = windows;
            break;
        case 6: 
            desktop6 = windows;
            break;
        case 7: 
            desktop7 = windows;
            break;
        case 8: 
            desktop8 = windows;
            break;
    }

    // set  the current working windows to the desktop to switch

    switch(desktoswitch){
        case 1:
            windows = desktop1;
            break;
        case 2:
            windows = desktop2;
            break;
        case 3:
            windows = desktop3;
            break;
        case 4:
            windows = desktop4;
            break;
        case 5:
            windows = desktop5;
            break;
        case 6:
            windows = desktop6;
            break;
        case 7:
            windows = desktop7;
            break;
        case 8:
            windows = desktop8;
            break;
    }

    return desktoswitch;

}

void changedesktop(int desknum) {
    // Move all windows of the current desktop offscreen
    moveWindowsOffscreen(windows);

    // Switch to the selected desktop
    switch (desknum) {
        case 1:
            currentdesk = quereydesktop(currentdesk, 1);
            break;
        case 2:
            currentdesk = quereydesktop(currentdesk, 2);
            break;
        case 3:
            currentdesk = quereydesktop(currentdesk, 3);
            break;
        case 4:
            currentdesk = quereydesktop(currentdesk, 4);
            break;
        case 5:
            currentdesk = quereydesktop(currentdesk, 5);
            break;
        case 6:
            currentdesk = quereydesktop(currentdesk, 6);
            break;
        case 7:
            currentdesk = quereydesktop(currentdesk, 7);
            break;
        case 8:
            currentdesk = quereydesktop(currentdesk, 8);
            break;
    }
    usleep(16000); // wait for stability
    // Restore windows for the newly selected desktop
    restoreWindowsToPosition(windows);
}

void handleButtonPress(XButtonEvent* event) {
    Window window = event->subwindow;
    if (window == None) return;

    focusWindow(window);
    if (event->state == MODKEY) {
        if (event->button == Button1) {
            // Initiate dragging if MODKEY is held and left-click is used
            draggingWindow = window;
            dragStartX = event->x_root;
            dragStartY = event->y_root;
            
            // Get the window's current position
            XWindowAttributes windowAttrs;
            XGetWindowAttributes(display, window, &windowAttrs);
            winStartX = windowAttrs.x;
            winStartY = windowAttrs.y;
        } else if (event->button == Button3) {
            // Initiate dragging if MODKEY is held and left-click is used
            rezizeWindow = window;
            zizeStartX = event->x_root;
            zizeStartY = event->y_root;

            // Get the window's current position
            XWindowAttributes windowAttrs;
            XGetWindowAttributes(display, window, &windowAttrs);
            winStartX = windowAttrs.x;
            winStartY = windowAttrs.y;
            winStartW = windowAttrs.width;
            winStartH = windowAttrs.height;
        }
    }
}

void handleButtonRelease(XButtonEvent* event) {
    if (event->button == Button1 && draggingWindow != None) {
        // Stop dragging when the left mouse button is released
        draggingWindow = None;
    }
    if (event->button == Button3 && rezizeWindow != None) {
        // Stop dragging when the left mouse button is released
        rezizeWindow = None;
    }
}

void handleMotionNotify(XMotionEvent* event) {
    if (draggingWindow != None) {
        // Calculate how much the mouse has moved since dragging started
        int deltaX = event->x_root - dragStartX;
        int deltaY = event->y_root - dragStartY;

        // Move the window based on its original position + movement deltas
        XMoveWindow(display, draggingWindow, winStartX + deltaX, winStartY + deltaY);
        XFlush(display);

        Node* node = windows.find(draggingWindow);
        if (node) {
            node->x = winStartX + deltaX;
            node->y = winStartY + deltaY;
        }
    }
    if (rezizeWindow != None) {
        int deltaX = event->x_root - dragStartX;
        int deltaY = event->y_root - dragStartY;
        int width = winStartW + deltaX;
        int height = winStartH + deltaY;
        if (width < 10) width = 10;
        if (height < 10) height = 10;
        XMoveResizeWindow(display, rezizeWindow, winStartX, winStartY, width, height);
        XFlush(display);
    }
}

void handleMapRequest(XMapRequestEvent* event) {
    XMoveResizeWindow(display, event->window, 
                      (DISPLAY_WIDTH - TERMINAL_WIDTH) / 2, 
                      (DISPLAY_HEIGHT - TERMINAL_HEIGHT) / 2, 
                      TERMINAL_WIDTH, TERMINAL_HEIGHT);
    XMapWindow(display, event->window);
    focusWindow(event->window);

    // Add the window to the current desktop's window list
    windows.append(event->window);
}

void handleConfigureRequest(XConfigureRequestEvent* event) {
    XWindowChanges changes;
    changes.x = event->x;
    changes.y = event->y;
    changes.width = event->width;
    changes.height = event->height;
    changes.border_width = event->border_width;
    changes.sibling = event->above;
    changes.stack_mode = event->detail;
    XConfigureWindow(display, event->window, event->value_mask, &changes);
}

void focusWindow(Window window) {
    XRaiseWindow(display, window);
    focusedWindow = window;
    XSetInputFocus(display, focusedWindow, RevertToPointerRoot, CurrentTime);
}

void closeWindow(Window window) {
    XEvent event;
    event.xclient.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = XInternAtom(display, "WM_PROTOCOLS", True);
    event.xclient.format = 32;
    event.xclient.data.l[0] = XInternAtom(display, "WM_DELETE_WINDOW", False);
    event.xclient.data.l[1] = CurrentTime;
    XSendEvent(display, window, False, NoEventMask, &event);
    XFlush(display);

    // kill window from linked list
    windows.remove(window);

    // Focus another window
    Node* headNode = windows.getHead();  // Use the public getter to access head
    if (headNode) {
        focusWindow(headNode->win);  // Focus the first window in the list
    } else {
        focusedWindow = None;  // No windows left to focus
    }
}

void handleDestroyRequest(XDestroyWindowEvent* event){
    // Find and remove the destroyed window from the linked list
    if (event->window != None) {
        windows.remove(event->window);
        std::cout << "Window " << event->window << " destroyed and removed from the list." << std::endl;
    }

}

void spawn(void* argz) {
    std::cout << "entered spawn" << std::endl;
    if (fork() == 0) {
        char* const* command = (char* const*) argz;
        execvp(command[0], command);
        _exit(1);
    }
}


