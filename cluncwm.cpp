// g++ -o cluncwm cluncwm.cpp -lX11 -lXrandr
// stabl cluncwm
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xrandr.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <cmath>
#include <string>
#include <regex>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <future>
#include <queue>
#include <functional>
#include <condition_variable>

#define MODKEY Mod4Mask


int TERMINAL_WIDTH = 600;
int TERMINAL_HEIGHT = 450;

char* BROWSER = nullptr;
char* TERMINAL = nullptr;

int nmonitors = 0;
Display* display;
Window root;

Window focusedWindow = None;
Window draggingWindow = None;
Window rezizeWindow = None;
int mode = 0; //0 is floating, 1 is tiling
int dragStartX = 0, dragStartY = 0;  // Mouse position at the start of dragging
int zizeStartX = 0, zizeStartY = 0; // rezizse position at start of rezize
int winStartX = 0, winStartY = 0;    // Window position at the start of dragging
int winStartW = 0, winStartH = 0;
int animationspeed = 35000;
int storeanimationspeed = 35000;
int enableanimation = 0;

struct layout {
	int rows=0;
	int colms=0;
};

void setup();
void config();
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
void getlastfocussed();
void identifyPlaace(int pos, Window window);
int quereydesktop();
bool isWindowValid(Window w);
void handleDestroyRequest(XDestroyWindowEvent* event);
void changedesktop(int desknum);
void yeetwindow(int desknum);
void switchmode();
void tiling();
void animeight(Window window, int xtomoveto, int ytomoveto, int xtomovefrom, int ytomovefrom);
void speedupanimeight();
void normaliseanimeight();
layout getposition(int n_windows);
int getnumberofwindows();
int handle_xerror(Display *dpy, XErrorEvent *ee);
void spawn(void* argz);
void setCursor();
std::string trim(const std::string& s);
std::string grabconfigtext(const std::string& line);
int grabconfigval(const std::string& line);
void centerMouseOnMonitor(int monitorIndex);

// Node class to represent each element in the linked list
class Node {
public:
    Window win;
    int x, y;  //window position
    int isfocussed;
    Node* next;

    Node(Window window, int xPos, int yPos, int focus) : win(window), x(xPos), y(yPos), isfocussed(focus), next(nullptr) {}
};

// LinkedList class to manage the nodes
class LinkedList {
private:
    Node* head;  //head of the linked list
	void clear() {
	        Node* current = head;
	        while (current) {
	            Node* next = current->next;
	            delete current;
	            current = next;
	        }
	        head = nullptr;
	    }

public:
    LinkedList() : head(nullptr) {}

	//deeeep constructor
    LinkedList(const LinkedList& other) : head(nullptr) {
        Node* temp = other.head;
        Node** current = &head;
        while (temp) {
            *current = new Node(temp->win, temp->x, temp->y, temp->isfocussed);
            current = &((*current)->next);
            temp = temp->next;
        }
    }

    //deeeeep assignment
    LinkedList& operator=(const LinkedList& other) {
        if (this != &other) {
            clear();
            Node* temp = other.head;
            Node** current = &head;
            while (temp) {
                *current = new Node(temp->win, temp->x, temp->y, temp->isfocussed);
                current = &((*current)->next);
                temp = temp->next;
            }
        }
        return *this;
    }

    //destructor
    ~LinkedList() {
        clear();
    }

    //add new node at the end of the list
    void append(Window window) {
        XWindowAttributes windowAttrs;
        XGetWindowAttributes(display, window, &windowAttrs);
        if (!XGetWindowAttributes(display, window, &windowAttrs)) {
	        std::cerr << "Failed to get attributes in append(), skipping window" << std::endl;
	        return;
        }
        int x = windowAttrs.x;
        int y = windowAttrs.y;
        int focus = 1;

        Node* newNode = new Node(window, x, y, focus);
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

    //find a node in the linked list
    Node* find(Window window) {
        Node* temp = head;
        while (temp) {
            if (temp->win == window)
                return temp;
            temp = temp->next;
        }
        return nullptr;
    }

    Node* getTail() {
    	if (head==nullptr) {
    		return nullptr;
    	}
    	else {
    		Node* temp = head;
    		while (temp->next != nullptr) {
    			temp=temp->next;
    		}
    		return temp;
    	}
    }

    //remove a node from the linked list
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

    //getter for the head node of the linked list
    Node* getHead() const {
        return head;
    }

	int size() const {
	    int count = 0;
	    Node* temp = head;
	    while (temp) {
	        count++;
	        temp = temp->next;
	    }
	    return count;
	}

	//move the current window to the head of the list
	void moveToHead(Window window) {
	    if (!head || head->win == window) {
	        return;  //already at head or list is empty
	    }
	    Node* prev = nullptr;
	    Node* curry = head;
	    while (curry && curry->win != window) {
	        prev = curry;
	        curry = curry->next;
	    }
	    if (!curry) {
	        return;  //window not found in the list
	    }
	    //detach current node to move to head
	    if (prev) {
	        prev->next = curry->next;
	        curry->next = head;
	        head = curry;
	    }
	}
	
	void pruneInvalidWindows() {
	    Node* current = head;
	    Node* prev = nullptr;
	
	    while (current) {
	        if (!isWindowValid(current->win)) {
	            std::cerr << "Pruning invalid window: " << current->win << std::endl;
	            if (prev) {
	                prev->next = current->next;
	                delete current;
	                current = prev->next;
	            } else {
	                head = current->next;
	                delete current;
	                current = head;
	            }
	        } else {
	            prev = current;
	            current = current->next;
	        }
	    }
	}

	void listnodes() {
		Node* temp = head;
		while (temp) {
			std::cout << "Window:" << std::endl;
			std::cout << "id: " << temp->win << std::endl;
			std::cout << "stored x: " << temp->x << std::endl;
			std::cout << "stored y: " << temp->y << std::endl;
			std::cout << "focused state: " << temp->isfocussed << std::endl;
			temp = temp->next;
		}
	}
};

//so i needed threading and this is stolen from clunc webserver, I had no idea how it worked there so hopefully it works here x-x

class ThreadPool {
    //I have no idea how threading works
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();
    void enqueue(std::function<void()> task);

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
    void workerThread();
};

ThreadPool::ThreadPool(size_t numThreads) : stop(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back(&ThreadPool::workerThread, this);
    }
}

ThreadPool::~ThreadPool() {
    stop.store(true);
    condition.notify_all();
    for (std::thread &worker : workers) {
        worker.join();
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        tasks.push(std::move(task));
    }
    condition.notify_one();
}

void ThreadPool::workerThread() {
    while (!stop.load()) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this] { return stop.load() || !tasks.empty(); });
            if (stop.load() && tasks.empty()) {
                return;
            }
            task = std::move(tasks.front());
            tasks.pop();
        }
        task();
    }
}

ThreadPool pool(30);

struct displaydimensions {
	int monnumber;
	int width;
	int height;	
	int x;
	int y;
	int currentfocuseddesk;
	int lastdesktop = 1;
	Node* focusedNode;
	LinkedList desktop[9];
};
std::vector<displaydimensions> trackmonitor;

int DISPLAY_WIDTH = 1366;
int DISPLAY_HEIGHT = 768;
int currentmonitor = 0;
int currentdesk = 1;

Node* currentfocusedNode = nullptr;

int main() {
    display = XOpenDisplay(NULL);
    if (!display) {
        std::cerr << "Cannot open display\n";
        return 1;
    }
    root = RootWindow(display, DefaultScreen(display));
	config();
	currentfocusedNode = trackmonitor[currentmonitor].desktop[currentdesk].getHead();
    setup();
    run();

    XCloseDisplay(display);
    return 0;
}

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    auto end = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

std::string grabconfigtext(const std::string& line) {
    auto pos = line.find('=');
    return (pos != std::string::npos) ? line.substr(pos + 1) : "";
}

int grabconfigval(const std::string& line) {
    auto pos = line.find('=');
    if (pos != std::string::npos) {
        try {
            return std::stoi(line.substr(pos + 1));
        } catch (...) {
            std::cerr << "Error parsing integer from line: " << line << std::endl;
        }
    }
    return 0;
}

int handle_xerror(Display *dpy, XErrorEvent *ee) {
    char error_text[1024];
    XGetErrorText(dpy, ee->error_code, error_text, sizeof(error_text));
    fprintf(stderr, "X error: %s (request: %d, resource id: 0x%lx)\n",
            error_text, ee->request_code, ee->resourceid);
    return 0; // Don't terminate
}

void centerMouseOnMonitor(int monitornum) {
    if (monitornum < 0 || monitornum >= nmonitors) {
        std::cerr << "Invalid monitor index\n";
        return;
    }
    
    int centerX = trackmonitor[monitornum].x + (trackmonitor[monitornum].width / 2);
    int centerY = trackmonitor[monitornum].y + (trackmonitor[monitornum].height / 2);
    
    XWarpPointer(display, None, root, 0, 0, 0, 0, centerX, centerY);
    XFlush(display);
}

void config() {
	XRRMonitorInfo* monitors = XRRGetMonitors(display, root, True, &nmonitors);
	trackmonitor.clear();
	trackmonitor.resize(nmonitors);
	for (int i=0; i<nmonitors; ++i) {
		trackmonitor[i].monnumber = i;
		trackmonitor[i].width = monitors[i].width;
		trackmonitor[i].height = monitors[i].height;
		trackmonitor[i].x = monitors[i].x;
		trackmonitor[i].y = monitors[i].y;
	}
	RROutput primary = XRRGetOutputPrimary(display, root);
	int primary_index = 0;
	for (int i = 0; i < nmonitors; ++i) {
	    for (int j = 0; j < monitors[i].noutput; ++j) {
	        if (monitors[i].outputs[j] == primary) {
	            primary_index = i;
	            break;
	        }
	    }
	}
	DISPLAY_WIDTH = trackmonitor[primary_index].width;
	DISPLAY_HEIGHT = trackmonitor[primary_index].height;
	std::cout << get_current_dir_name() << std::endl;
    std::ifstream inputFile("cluncconfig.txt");
    centerMouseOnMonitor(primary_index);
    if (!inputFile) {
        //create default config if its missing
        std::cerr << "Unable to open cluncconfig.txt. Creating default config.\n";
        std::ofstream configFile("cluncconfig.txt");
        if (configFile.is_open()) {
            configFile << "terminal=kitty\n";
            configFile << "terminal_width=400\n";
            configFile << "terminal_height=300\n";
            configFile << "animationspeed=40000\n";
            configFile << "browser=chromium\n";
            configFile << "enableanimation=0\n";
            configFile.close();
        } else {
            std::cerr << "Unable to write default config.\n";
            return;
        }
        //reopen for reading
        inputFile.open("cluncconfig.txt");
        if (!inputFile) {
            std::cerr << "Still unable to open config.txt after writing.\n";
            return;
        }
    }

    std::string line;
    int currentline = 0;

    while (std::getline(inputFile, line)) {
        switch (currentline) {
            case 0:
                if (TERMINAL) free(TERMINAL);  //avoid memory leak but its inevitable anyway
                TERMINAL = strdup(grabconfigtext(line).c_str());
                break;
            case 1:
                TERMINAL_WIDTH = grabconfigval(line);
                break;
            case 2:
                TERMINAL_HEIGHT = grabconfigval(line);
                break;
            case 3:
            	animationspeed = grabconfigval(line);
            	break;
           	case 4:
           		BROWSER = strdup(grabconfigtext(line).c_str());
           		break;
           	case 5:
           		enableanimation = grabconfigval(line);
            default:
                //prevent people doing sillies and trying to run arbritary code but thats possible by abusing terminal ig
                break;
        }
        currentline++;
    }

    inputFile.close();
    XRRFreeMonitors(monitors);
}

void setup() {
	XSetErrorHandler(handle_xerror);
    XSelectInput(display, root, SubstructureRedirectMask | SubstructureNotifyMask | ButtonPressMask | PointerMotionMask);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("T")), MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("B")), MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("Q")), MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("E")), MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("Z")), MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("C")), MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("S")), MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("D")), MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("A")), MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("L")), MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("F")), MODKEY, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("C")), (MODKEY|ShiftMask), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("I")), (MODKEY|ShiftMask), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("1")), (MODKEY), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("2")), (MODKEY), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("3")), (MODKEY), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("4")), (MODKEY), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("5")), (MODKEY), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("6")), (MODKEY), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("7")), (MODKEY), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("8")), (MODKEY), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("1")), (MODKEY|ShiftMask), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("2")), (MODKEY|ShiftMask), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("3")), (MODKEY|ShiftMask), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("4")), (MODKEY|ShiftMask), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("5")), (MODKEY|ShiftMask), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("6")), (MODKEY|ShiftMask), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("7")), (MODKEY|ShiftMask), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("8")), (MODKEY|ShiftMask), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("Tab")), (MODKEY), root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("Tab")), (MODKEY|ShiftMask), root, True, GrabModeAsync, GrabModeAsync);

    XGrabButton(display, Button1, MODKEY, root, True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(display, Button3, MODKEY, root, True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    setCursor();
}

void setCursor() {
    Cursor cursor;
    cursor = XCreateFontCursor(display, 142);
    XDefineCursor(display, root, cursor);
    XSync(display, false);
}

void run() {
    XEvent event;
    try{
        while (true) {
            XNextEvent(display, &event);
            handleEvent(&event);
        } 
    } catch (const std::exception &e) {
        std::cerr << "Caught exception in event loop: " << e.what() << std::endl;
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
    if (event->window == None) {
    std::cerr << "Invalid window or window attributes cannot be retrieved." << std::endl;
    return;  //prevent further handling
}
    if (event->keycode == XKeysymToKeycode(display, XK_T) && event->state == MODKEY) {
        const char* command[] = {TERMINAL, NULL};
        spawn((void*)command);
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_B) && event->state == MODKEY) {
        const char* command[] = {BROWSER, NULL};
        spawn((void*)command);
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_C) && event->state == (MODKEY|ShiftMask) && focusedWindow != None) {
        closeWindow(focusedWindow);
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_1) && event->state == (MODKEY|ShiftMask) && focusedWindow != None) {    
		yeetwindow(1);
		if (mode == 1) {tiling();}
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_2) && event->state == (MODKEY|ShiftMask) && focusedWindow != None) {    
		yeetwindow(2);
		if (mode == 1) {tiling();}
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_3) && event->state == (MODKEY|ShiftMask) && focusedWindow != None) {    
		yeetwindow(3);
		if (mode == 1) {tiling();}
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_4) && event->state == (MODKEY|ShiftMask) && focusedWindow != None) {    
		yeetwindow(4);
		if (mode == 1) {tiling();}
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_5) && event->state == (MODKEY|ShiftMask) && focusedWindow != None) {    
		yeetwindow(5);
		if (mode == 1) {tiling();}
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_6) && event->state == (MODKEY|ShiftMask) && focusedWindow != None) {    
		yeetwindow(6);
		if (mode == 1) {tiling();}
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_7) && event->state == (MODKEY|ShiftMask) && focusedWindow != None) {    
		yeetwindow(7);
		if (mode == 1) {tiling();}
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_8) && event->state == (MODKEY|ShiftMask) && focusedWindow != None) {    
		yeetwindow(8);
		if (mode == 1) {tiling();}
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
    else if (event->keycode == XKeysymToKeycode(display, XK_D) && event->state == MODKEY) {
        identifyPlaace(6, focusedWindow);
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_A) && event->state == MODKEY) {
        identifyPlaace(7, focusedWindow);
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_L) && event->state == MODKEY) {
    	switchmode();
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_F) && event->state == MODKEY) {
    	if (focusedWindow != None) {
  	        trackmonitor[currentmonitor].desktop[currentdesk].moveToHead(focusedWindow);
  	        if (mode == 1) {tiling();}
  	    } 
  	    else {
  	        std::cerr << "Mod+F pressed but no window is focused." << std::endl;
  	    }
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_I) && event->state == (MODKEY | ShiftMask)) {
		config();
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
    else if (event->keycode == XKeysymToKeycode(display, XK_Tab) && event->state == MODKEY) {
		trackmonitor[currentmonitor].desktop[currentdesk].pruneInvalidWindows();
		if (trackmonitor[currentmonitor].desktop[currentdesk].size() == 0) {
		        std::cerr << "No focused window to cycle.\n";
		        return;
	    }
		if (currentfocusedNode!=nullptr && currentfocusedNode->next!=nullptr) {
			focusWindow(currentfocusedNode->next->win);
		}
		else if (currentfocusedNode!=nullptr && currentfocusedNode->next==nullptr) {
			Node* temp = trackmonitor[currentmonitor].desktop[currentdesk].getHead();
			currentfocusedNode=temp;
			focusWindow(currentfocusedNode->win);
		}
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_Tab) && event->state == (MODKEY | ShiftMask) && focusedWindow != None) {
		trackmonitor[currentmonitor].desktop[currentdesk].pruneInvalidWindows();
		if (trackmonitor[currentmonitor].desktop[currentdesk].size() == 0) {
		        std::cerr << "No focused window to cycle.\n";
		        return;
		}
    	Node* temp = trackmonitor[currentmonitor].desktop[currentdesk].getHead();
    	if (temp!=nullptr && temp->next!=nullptr && currentfocusedNode!=temp) {
    		while (temp->next != currentfocusedNode) {
    			temp=temp->next;
    		}
    		currentfocusedNode = temp;
    		focusWindow(currentfocusedNode->win);
    	}
    	else if (temp==currentfocusedNode && temp->next!=nullptr) {
    		currentfocusedNode = trackmonitor[currentmonitor].desktop[currentdesk].getTail();
   			focusWindow(currentfocusedNode->win);
    	}
    }
    else {
    	return;
    }
}

void switchmode() {
	if (mode==0) {
		mode=1;
		tiling();
	}
	else mode=0;
}

bool isWindowValid(Window w) {
    XWindowAttributes attrs;
    return XGetWindowAttributes(display, w, &attrs) != 0;
}


layout getposition(int n_windows, bool usedOddSlot) {
	layout structure;
	int factor = 100*n_windows; //unneccesarily high start number to make sure one factor is found at least
	int testfactor = 1;
	while (testfactor<=sqrt(n_windows)) { //define a limit to stop repeat considering factor pairs
	  if (n_windows%testfactor==0) { //make sure that the factor being considered is a valid factor
	    if (abs(testfactor-(n_windows/testfactor))<abs(factor-(n_windows/factor))) { //check if factor pair is closer than the closest
	      factor = testfactor; 
	    }
	  }
	  testfactor++;
	}
	if (n_windows+1==3 && usedOddSlot==true) {
		structure.rows=n_windows/factor; //assign the numbers to be sent back, put this in whatever order u want
		structure.colms=factor;
	}
	else {	
		structure.colms=n_windows/factor; //assign the numbers to be sent back, put this in whatever order u want
		structure.rows=factor;
	}
	return structure;
}

void tiling() { //in here we tile brilliantly and intuitively
	trackmonitor[currentmonitor].desktop[currentdesk].pruneInvalidWindows();
	int height=DISPLAY_HEIGHT;
	int width=DISPLAY_WIDTH;
	int n_windows=trackmonitor[currentmonitor].desktop[currentdesk].size();
	int subwidth=0;
	int subheight=0;
	int remainwidth=DISPLAY_WIDTH;
	double xsize=0.0;
	int i=0;
	int j=0;
	const int gap_inner=4; //gaps
	const int gap_outer=8;
	bool usedOddSlot = false;
	layout structure; //for getting back multiple values (i'm lazy)
	Node* mover = trackmonitor[currentmonitor].desktop[currentdesk].getHead(); //grabbed head of queue
	if (!mover || !mover->next) return;
	if (n_windows==0) return;
	else if (n_windows==1) {
		identifyPlaace(1, trackmonitor[currentmonitor].desktop[currentdesk].getHead()->win);
		return;
	}
	else if (n_windows%2!=0) { //if there is odd number handle the dom window here
		xsize=std::min(width * 0.6, width / double(n_windows - 1)); //dom window musnt take over 40% of screen
		usedOddSlot = true;
		remainwidth=width-xsize;
		n_windows--;
		mover->x = trackmonitor[currentmonitor].x+gap_outer; //need to be able to move the window back to its position afterwards
		mover->y = trackmonitor[currentmonitor].y+gap_outer;
		XWindowAttributes attrs;
		XGetWindowAttributes(display, mover->win, &attrs);
		pool.enqueue([=]() {animeight(mover->win, mover->x, mover->y, attrs.x, attrs.y);});
		XResizeWindow(display, mover->win, xsize-(2*gap_outer), DISPLAY_HEIGHT-(2*gap_outer));
		mover=mover->next;
	}
	structure=getposition(n_windows, usedOddSlot);
	int totalgap_x=gap_inner*(structure.colms - 1)+gap_outer*2; //more gaps
	int totalgap_y=gap_inner*(structure.rows - 1)+gap_outer*2;

	subwidth=(remainwidth-totalgap_x)/structure.colms; //new measurements for the even number of windows
	subheight=(height-totalgap_y)/structure.rows;
	int windex=0;
	while (j<structure.rows) { //treating the screen as a matrix
		while (i<structure.colms) {
			if (windex >= n_windows) break;
			int x=gap_outer+(i*(subwidth+gap_inner))+trackmonitor[currentmonitor].x; //very proud of this method for filling in over an area
			if (usedOddSlot) x+=xsize;
			int y=gap_outer+j*(subheight+gap_inner)+trackmonitor[currentmonitor].y;
			mover->x = x;
			mover->y = y;
			XWindowAttributes attrs;
			XGetWindowAttributes(display, mover->win, &attrs);
			pool.enqueue([=]() {animeight(mover->win, mover->x, mover->y, attrs.x, attrs.y);});
			XResizeWindow(display, mover->win, subwidth, subheight);
			mover=mover->next;
			i++;
			windex++;
		}
		j++;
		i=0;
	}
}

void animeight(Window window, int xtomoveto, int ytomoveto, int xtomovefrom, int ytomovefrom) {
	if (enableanimation == 1) {	
		float animationratio[8] = {1.0/32.0, 2.0/32.0, 4.0/32.0, 9.0/32.0, 9.0/32.0, 4.0/32.0, 2.0/32.0, 1.0/32.0};
		float distancex = xtomoveto - xtomovefrom;
		float distancey = ytomoveto - ytomovefrom;
		float tempx = xtomovefrom;
		float tempy = ytomovefrom;
		for (int i=0; i<8; i++) {
			tempx+=(animationratio[i]*distancex);
			tempy+=(animationratio[i]*distancey);
			XMoveWindow(display, window, tempx, tempy);
			XFlush(display);
			usleep(animationspeed);
		}
	}
	else {
		XMoveWindow(display, window, xtomoveto, ytomoveto);
	}
}

void speedupanimeight() {
	storeanimationspeed=animationspeed;
	animationspeed = 50;
}

void normaliseanimeight() {
	animationspeed = storeanimationspeed;
}

void identifyPlaace(int pos, Window window){
    if (!window || window == None){
           return;
   	}
       XWindowAttributes windowAttrs;
   	if (!XGetWindowAttributes(display, window, &windowAttrs)){
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
        case 6: // right
            windimw = (DISPLAY_WIDTH/2) - gap;
            windimh = DISPLAY_HEIGHT - gap;
            winnewx = (DISPLAY_WIDTH/2) + (gap/2);
            winnewy = gap/2;
            break;
        case 7: // left
            windimw = (DISPLAY_WIDTH/2) - gap;
            windimh = DISPLAY_HEIGHT - gap;
            winnewx = gap;
            winnewy = gap/2;
            break;
    }
    Node* node = trackmonitor[currentmonitor].desktop[currentdesk].find(window);
    XWindowAttributes attrs;
    winnewx+=trackmonitor[currentmonitor].x;
    winnewy+=trackmonitor[currentmonitor].y;
	XGetWindowAttributes(display, window, &attrs);
	pool.enqueue([=]() {animeight(window, winnewx, winnewy, node->x, node->y);});
	XResizeWindow(display, window, windimw, windimh);
    if (node) {
        node->x = winnewx;
        node->y = winnewy;
    }
}

//move all windows offscreen without updating their positions in the linked list
void moveWindowsOffscreen(LinkedList& workingdesktop) {
    Node* current = workingdesktop.getHead();
    while (current) {
        //store the original position of the window
        int originalX = current->x;
        int originalY = current->y;
        std::cout << originalX << originalY << std::endl;
        //move the window offscreen
        XMoveWindow(display, current->win, originalX, -DISPLAY_HEIGHT);
        current->x = originalX;
        current->y = originalY;
        current = current->next;
    }
}


//restore all windows to their original positions using the linked list
void restoreWindowsToPosition(LinkedList& desktop) {
    Node* current = desktop.getHead();
	desktop.listnodes();
    while (current) {
        //restore the window to its original position
        int originalX = current->x;
        int originalY = current->y;
        //move the window back to its original position
        XMoveWindow(display, current->win, originalX, originalY);
        if (current != NULL){
    	    getlastfocussed();
        }	
        current = current->next;
    }
}

void changedesktop(int desknum) {
    if (desknum == currentdesk) {
    	return;
    }
	speedupanimeight();
    // Move all windows of the current desktop offscreen
    moveWindowsOffscreen(trackmonitor[currentmonitor].desktop[currentdesk]);
    //switch to the selected desktop
    if (desknum >= 1 && desknum <= 8) {
        currentdesk = desknum;
        trackmonitor[currentmonitor].lastdesktop = currentdesk;
    }
    else {
    	return;
    }
    //restore windows for the newly selected desktop
    restoreWindowsToPosition(trackmonitor[currentmonitor].desktop[currentdesk]);
    normaliseanimeight();
}

void yeetwindow(int desknum) {
	if (desknum == currentdesk) {return;}
    int originalX = currentfocusedNode->x;
    int originalY = currentfocusedNode->y;
    //move the window offscreen
    XMoveWindow(display, currentfocusedNode->win, originalX, -DISPLAY_HEIGHT);
    Node* movedwindow;
	trackmonitor[currentmonitor].desktop[desknum].append(currentfocusedNode->win);
	movedwindow = trackmonitor[currentmonitor].desktop[desknum].find(currentfocusedNode->win);
	movedwindow->x = originalX;
	movedwindow->y = originalY;
	trackmonitor[currentmonitor].desktop[currentdesk].remove(currentfocusedNode->win);
	Node* head = trackmonitor[currentmonitor].desktop[currentdesk].getHead();
    if (head) {
        currentfocusedNode = head;
        focusedWindow = head->win;
        focusWindow(focusedWindow);
    } 
    else {
        currentfocusedNode = nullptr;
        focusedWindow = None;
    }
}

int grabMonitoratmouse(int x, int y) {
    for (int i = 0; i < nmonitors; ++i) {
        if (x >= trackmonitor[i].x &&
            x < trackmonitor[i].x + trackmonitor[i].width &&
            y >= trackmonitor[i].y &&
            y < trackmonitor[i].y + trackmonitor[i].height)
            return i;
    }
    return -1;
}

void handleButtonPress(XButtonEvent* event) {
    Window window = event->subwindow;
    if (window == None) return;
    int checkmonitor = grabMonitoratmouse(event->x_root, event->y_root);
	if (checkmonitor != -1 && checkmonitor != currentmonitor) {
		currentmonitor = checkmonitor;
	    currentdesk = trackmonitor[currentmonitor].lastdesktop;
	    DISPLAY_WIDTH = trackmonitor[checkmonitor].width;
	    DISPLAY_HEIGHT = trackmonitor[checkmonitor].height;
	}
    focusWindow(window);
    if (event->state == MODKEY) {
        if (event->button == Button1) {
            //initiate dragging if MODKEY is held and left click is used
            draggingWindow = window;
            dragStartX = event->x_root;
            dragStartY = event->y_root;
            
            //get the windows current position
            XWindowAttributes windowAttrs;
            XGetWindowAttributes(display, window, &windowAttrs);
            winStartX = windowAttrs.x;
            winStartY = windowAttrs.y;
        } else if (event->button == Button3) {
            //initiate dragging if MODKEY is held and left click is used
            rezizeWindow = window;
            zizeStartX = event->x_root;
            zizeStartY = event->y_root;

            //get the windows current position
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
        //stop dragging when the left mouse button is released
        draggingWindow = None;
    }
    if (event->button == Button3 && rezizeWindow != None) {
        //stop dragging when the left mouse button is released
        rezizeWindow = None;
    }
}

void handleMotionNotify(XMotionEvent* event) {
    if (event->window == None){
        return;
    }
    if (draggingWindow != None) {
        //calculate how much the mouse has moved since dragging started
        int deltaX = event->x_root - dragStartX;
        int deltaY = event->y_root - dragStartY;
        //if window crosses to a different display then it needs to be added to that display's list of windows
		int checkmonitor = grabMonitoratmouse(event->x_root, event->y_root);
		if (checkmonitor != -1 && checkmonitor != currentmonitor) {
			int tempx = currentfocusedNode->x;
			int tempy = currentfocusedNode->y;
			trackmonitor[checkmonitor].desktop[trackmonitor[checkmonitor].lastdesktop].append(currentfocusedNode->win);
			
		    Node* movedwin = trackmonitor[checkmonitor].desktop[trackmonitor[checkmonitor].lastdesktop].find(currentfocusedNode->win);
		    if (movedwin) {
		        movedwin->x = tempx;
		        movedwin->y = tempy;
		    }
		
		    trackmonitor[currentmonitor].desktop[currentdesk].remove(currentfocusedNode->win);
		    currentmonitor = checkmonitor;
		    currentdesk = trackmonitor[currentmonitor].lastdesktop;
		    DISPLAY_WIDTH = trackmonitor[checkmonitor].width;
		    DISPLAY_HEIGHT = trackmonitor[checkmonitor].height;
		    currentfocusedNode = movedwin;
		    if (currentfocusedNode) {
                focusedWindow = currentfocusedNode->win;
            }
            else {
            	focusedWindow = None;
            }
		}
        //move the window based on its original position + movement deltas
        XMoveWindow(display, draggingWindow, winStartX + deltaX, winStartY + deltaY);
        XSync(display, false);

        Node* node = trackmonitor[currentmonitor].desktop[currentdesk].find(draggingWindow);
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
        XSync(display, false);
    }
    int checkmonitor = grabMonitoratmouse(event->x_root, event->y_root);
    		if (checkmonitor != -1 && checkmonitor != currentmonitor) {
    		currentmonitor = checkmonitor;
		    currentdesk = trackmonitor[currentmonitor].lastdesktop;
		    DISPLAY_WIDTH = trackmonitor[checkmonitor].width;
		    DISPLAY_HEIGHT = trackmonitor[checkmonitor].height;
		    getlastfocussed();
		    currentfocusedNode = trackmonitor[currentmonitor].desktop[currentdesk].find(focusedWindow);
	}
}

void getlastfocussed() {
	Node* temphead = trackmonitor[currentmonitor].desktop[currentdesk].getHead();
	Node* temp = temphead;
	while (temp!=nullptr) {
		if (temp->isfocussed == 1) {
			focusWindow(temp->win);
			break;
		}
		else {temp=temp->next;}
	}
}

void handleMapRequest(XMapRequestEvent* event) {
    XWindowAttributes windowAttrs;
    if (event->window == None) return;
    if (!XGetWindowAttributes(display, event->window, &windowAttrs) || windowAttrs.override_redirect) return;
    XSelectInput(display, event->window, EnterWindowMask | FocusChangeMask | StructureNotifyMask | PointerMotionMask);
    XMapWindow(display, event->window);
    trackmonitor[currentmonitor].desktop[currentdesk].append(event->window);
    focusWindow(event->window);
    DISPLAY_HEIGHT = trackmonitor[currentmonitor].height;
    DISPLAY_WIDTH = trackmonitor[currentmonitor].width;
    XMoveResizeWindow(display, event->window, 
                      trackmonitor[currentmonitor].x+((DISPLAY_WIDTH - TERMINAL_WIDTH) / 2), 
                      trackmonitor[currentmonitor].y+((DISPLAY_HEIGHT - TERMINAL_HEIGHT) / 2), 
                      TERMINAL_WIDTH, TERMINAL_HEIGHT);
    currentfocusedNode->x = trackmonitor[currentmonitor].x + (DISPLAY_WIDTH/2);
    currentfocusedNode->y = trackmonitor[currentmonitor].y + (DISPLAY_HEIGHT/2);
    if (mode==1) {
    	tiling();
    }  
}

void handleConfigureRequest(XConfigureRequestEvent* event) {
	XWindowAttributes windowAttrs;
	if (!XGetWindowAttributes(display, event->window, &windowAttrs) || windowAttrs.override_redirect) return;
    if (event->window == None) return;
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
	if (window!=None) {	
	    XRaiseWindow(display, window);
	    if (trackmonitor[currentmonitor].desktop[currentdesk].find(focusedWindow)) {
	    	currentfocusedNode->isfocussed = 0;
	    	trackmonitor[currentmonitor].desktop[currentdesk].find(window)->isfocussed = 1;
	    }
	    focusedWindow = window;
	    XSetInputFocus(display, focusedWindow, RevertToPointerRoot, CurrentTime);
	    currentfocusedNode = trackmonitor[currentmonitor].desktop[currentdesk].find(focusedWindow);
	    currentfocusedNode->isfocussed = 1;
	}
	else {
		return;
	}
}

void closeWindow(Window window) {
    if (window == None) return;
    XEvent event;
    event.xclient.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = XInternAtom(display, "WM_PROTOCOLS", True);
    event.xclient.format = 32;
    event.xclient.data.l[0] = XInternAtom(display, "WM_DELETE_WINDOW", False);
    event.xclient.data.l[1] = CurrentTime;
    XSendEvent(display, window, False, NoEventMask, &event);
    XSync(display, false);
    //kill window from linked list
    trackmonitor[currentmonitor].desktop[currentdesk].remove(window);
    //focus another window
    Node* headNode = trackmonitor[currentmonitor].desktop[currentdesk].getHead();  //grab some head
    if (headNode) {
        focusWindow(headNode->win);  //focus the first window in the list
    } else {
        focusedWindow = None;  //uo windows left to focus
    }
    if (mode==1) tiling();
}

void handleDestroyRequest(XDestroyWindowEvent* event){
    //find and remove the destroyed window from the linked list
    if (event->window != None) {
        trackmonitor[currentmonitor].desktop[currentdesk].remove(event->window);
    }
}

void spawn(void* argz) {
    if (fork() == 0) {
        char* const* command = (char* const*) argz;
        execvp(command[0], command);
        _exit(1);
    }
}
