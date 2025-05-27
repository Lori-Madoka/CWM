// g++ -o testimp testimp.cpp -lX11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <cmath>
#include <string>

#define MODKEY Mod4Mask // Super key (Windows key)
int TERMINAL_WIDTH = 600;
int TERMINAL_HEIGHT = 450;

char* TERMINAL = nullptr;
int DISPLAY_WIDTH = 1366;
int DISPLAY_HEIGHT = 768;

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
void identifyPlaace(int pos, Window window);
int quereydesktop();
void handleDestroyRequest(XDestroyWindowEvent* event);
void changedesktop(int desknum);
void switchmode();
void tiling();
layout getposition(int n_windows);
int getnumberofwindows();
void spawn(void* argz);
void setCursor();
std::string trim(const std::string& s);
std::string grabconfigtext(const std::string& line);
int grabconfigval(const std::string& line);

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
    Node* head;  // Head of the linked list
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
	config();
    windows = desktop1;
    desktop1 = windows;
    
    root = RootWindow(display, DefaultScreen(display));

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

void config() {
	std::cout << get_current_dir_name() << std::endl;
    std::ifstream inputFile("cluncconfig.txt");
    if (!inputFile) {
        //create default config if its missing
        std::cerr << "Unable to open cluncconfig.txt. Creating default config.\n";
        std::ofstream configFile("cluncconfig.txt");
        if (configFile.is_open()) {
            configFile << "terminal=kitty\n";
            configFile << "terminal_width=400\n";
            configFile << "terminal_height=300\n";
            configFile << "window_width=1366\n";
            configFile << "window_height=768\n";
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
                std::cout << "received TERMINAL = " << TERMINAL << std::endl;
                break;
            case 1:
                TERMINAL_WIDTH = grabconfigval(line);
                std::cout << "received TERMINAL_WIDTH = " << TERMINAL_WIDTH << std::endl;
                break;
            case 2:
                TERMINAL_HEIGHT = grabconfigval(line);
                std::cout << "received TERMINLALHEIGHT = " << TERMINAL_HEIGHT <<std::endl;
                break;
            case 3:
                DISPLAY_WIDTH = grabconfigval(line);
                std::cout << "receiverd DISPLAY_WIDTH = " << DISPLAY_WIDTH << std::endl;
                break;
            case 4:
                DISPLAY_HEIGHT = grabconfigval(line);
                std::cout << "receiverd DISPLAY_HEIGHT = " << DISPLAY_HEIGHT << std::endl;
                break;
            default:
                //prevent people doing sillies and trying to run arbritary code but thats possible by abusing terminal ig
                break;
        }
        currentline++;
    }

    inputFile.close();
    // Debug print
    std::cout << "Loaded config:\n";
    std::cout << "TERMINAL=" << TERMINAL << "\n";
    std::cout << "TERMINAL_WIDTH=" << TERMINAL_WIDTH << "\n";
    std::cout << "TERMINAL_HEIGHT=" << TERMINAL_HEIGHT << "\n";
    std::cout << "DISPLAY_WIDTH=" << DISPLAY_WIDTH << "\n";
    std::cout << "DISPLAY_HEIGHT=" << DISPLAY_HEIGHT << "\n";
}


void setup() {
    XSelectInput(display, root, SubstructureRedirectMask | SubstructureNotifyMask | ButtonPressMask | PointerMotionMask);
    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym("T")), MODKEY, root, True, GrabModeAsync, GrabModeAsync);
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
    else if (event->keycode == XKeysymToKeycode(display, XK_C) && event->state == (MODKEY | ShiftMask) && focusedWindow != None) {
        closeWindow(focusedWindow);
        std::cout << "pressed mod+shift+C" << std::endl;
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_S) && event->state == MODKEY) {
        identifyPlaace(1, focusedWindow);
        std::cout << "pressed mod+S" << std::endl;
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_E) && event->state == MODKEY) {
        identifyPlaace(2, focusedWindow);
        std::cout << "pressed mod+E" << std::endl;
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_C) && event->state == MODKEY) {
        identifyPlaace(3, focusedWindow);
        std::cout << "pressed mod+C" << std::endl;
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_Z) && event->state == MODKEY) {
        identifyPlaace(4, focusedWindow);
        std::cout << "pressed mod+Z" << std::endl;
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_Q) && event->state == MODKEY) {
        identifyPlaace(5, focusedWindow);
        std::cout << "pressed mod+Q" << std::endl;
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_D) && event->state == MODKEY) {
        identifyPlaace(6, focusedWindow);
        std::cout << "pressed mod+D" << std::endl;
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_A) && event->state == MODKEY) {
        identifyPlaace(7, focusedWindow);
        std::cout << "pressed mod+A" << std::endl;
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_L) && event->state == MODKEY) {
    	switchmode();
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_F) && event->state == MODKEY) {
    	if (focusedWindow != None) {
  	        windows.moveToHead(focusedWindow);
  	        if (mode == 1) {
  	            tiling();
  	        }
  	    } 
  	    else {
  	        std::cerr << "Mod+F pressed but no window is focused." << std::endl;
  	    }
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_I) && event->state == (MODKEY | ShiftMask) && focusedWindow != None) {
		config();
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_1) && event->state == MODKEY) {
        changedesktop(1);
        std::cout << "changed desktop to 1" << std::endl;
    }
    else if (event->keycode == XKeysymToKeycode(display, XK_2) && event->state == MODKEY) {
        changedesktop(2);
        std::cout << "changed desktop to 2" << std::endl;
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

void switchmode() {
	if (mode==0) {
		mode=1;
		tiling();
	}
	else mode=0;
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
	int height=DISPLAY_HEIGHT;
	int width=DISPLAY_WIDTH;
	int n_windows=windows.size();
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
	Node* mover = windows.getHead(); //grabbed head of queue
	if (!mover || !mover->next) return;
	if (n_windows==0) return;
	else if (n_windows==1) {
		identifyPlaace(1, focusedWindow);
		return;
	}
	else if (n_windows%2!=0) { //if there is odd number handle the dom window here
		xsize=std::min(width * 0.6, width / double(n_windows - 1)); //dom window musnt take over 40% of screen
		usedOddSlot = true;
		remainwidth=width-xsize;
		n_windows--;
		mover->x = gap_outer; //need to be able to move the window back to its position afterwards
		mover->y = gap_outer;
		XMoveResizeWindow(display, mover->win, gap_outer, gap_outer, xsize-(2*gap_outer), DISPLAY_HEIGHT-(2*gap_outer));
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
			int x=gap_outer+(i*(subwidth+gap_inner)); //very proud of this method for filling in over an area
			if (usedOddSlot) x+=xsize;
			int y=gap_outer+j*(subheight+gap_inner);
			mover->x = x;
			mover->y = y;
			XMoveResizeWindow(display, mover->win, x, y, subwidth, subheight);
			mover=mover->next;
			i++;
			windex++;
		}
		j++;
		i=0;
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
    //if (!XMoveResizeWindow(display, window, winnewx, winnewy, windimw, windimh)); return;
    XMoveResizeWindow(display, window, winnewx, winnewy, windimw, windimh);
    std::cout << "window moved to " << winnewx << "x " << winnewy << "y" << std::endl;
    Node* node = windows.find(window);
        if (node) {
            node->x = winnewx;
            node->y = winnewy;
        }
}

// Move all windows offscreen without updating their positions in the linked list
void moveWindowsOffscreen(LinkedList& desktop) {
    std::cout << "Entered  moveWindowsOffscreen" << std::endl;
    Node* current = desktop.getHead();
    while (current) {
        // Store the original position of the window
        int originalX = current->x;
        int originalY = current->y;
        std::cout << originalX << originalY << std::endl;
        // Move the window offscreen
        XMoveWindow(display, current->win, originalX, -1000);

        std::cout << "current x:" << current->x << " current y:" << current->y << std::endl;
        std::cout << current << std::endl;

        current->x = originalX;
        current->y = originalY;

        // Optionally flush the X server to ensure the move happens immediately
        //XFlush(display);

        current = current->next;
    }

    std::cout << "success in removing windows from current desktop" << std::endl;
}


// Restore all windows to their original positions using the linked list
void restoreWindowsToPosition(LinkedList& desktop) {
    std::cout << "Entered restoreWindowsToPosition" << std::endl;
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
    	    focusWindow(desktop.getHead()->win);
        }	
        current = current->next;
    }
    std::cout << "success in remapping windows to current desktop" << std::endl;
}

int quereydesktop(int currentdesk, int desktoswitch){
    std::cout << "Entered quereydesktop" << std::endl;
    if (currentdesk == desktoswitch){
        return desktoswitch;
    }
    else{
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

    std::cout << "success in reassigning linked list" << std::endl;
    return desktoswitch;
    }
}

void changedesktop(int desknum) {
    std::cout << "Entered changedesktop" << std::endl;
    // Move all windows of the current desktop offscreen
    moveWindowsOffscreen(windows);
    //switch to the selected desktop
    if (desknum >= 1 && desknum <= 8) {
        currentdesk = quereydesktop(currentdesk, desknum);
    }
    //usleep(16000); // wait for stability
    std::cout << "success moved and reassigned linked list windows" << std::endl;
    // Restore windows for the newly selected desktop
    restoreWindowsToPosition(windows);
}

void handleButtonPress(XButtonEvent* event) {
    std::cout << "Entered handleButtonPress" << std::endl;
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
    std::cout << "Entered handleButtonRelease" << std::endl;
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
    std::cout << "Entered handleMotionNotify" << std::endl;
    if (event->window == None){
        return;
    }
    if (draggingWindow != None) {
        // Calculate how much the mouse has moved since dragging started
        int deltaX = event->x_root - dragStartX;
        int deltaY = event->y_root - dragStartY;

        // Move the window based on its original position + movement deltas
        XMoveWindow(display, draggingWindow, winStartX + deltaX, winStartY + deltaY);
        XSync(display, false);

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
        XSync(display, false);
    }
}


void handleMapRequest(XMapRequestEvent* event) {
    std::cout << "entered handleMapRequest" << std::endl;
    if (event->window == None){
        return;
    }
    XSelectInput(display, event->window, EnterWindowMask | FocusChangeMask | StructureNotifyMask);
    XMapWindow(display, event->window);
    windows.append(event->window);
    focusWindow(event->window);
    // Add the window to the current desktop's window list
    XMoveResizeWindow(display, event->window, 
                      (DISPLAY_WIDTH - TERMINAL_WIDTH) / 2, 
                      (DISPLAY_HEIGHT - TERMINAL_HEIGHT) / 2, 
                      TERMINAL_WIDTH, TERMINAL_HEIGHT);
    if (mode==1) {
    	tiling();
    }  
}

void handleConfigureRequest(XConfigureRequestEvent* event) {
    std::cout << "entered hendleConfigureRequest" << std::endl;
    if (event->window == None) return;
    XWindowChanges changes;
    std::cout << "mad changes below" << std::endl;
    changes.x = event->x;
    std::cout << "changes.x called for x parameter from server" << std::endl;
    changes.y = event->y;
    std::cout << "changes.y called for y parameter from server" << std::endl;
    changes.width = event->width;
    std::cout << "changes.width called for width from server" << std::endl;
    changes.height = event->height;
    std::cout << "changes.height called for height  from server" << std::endl;
    changes.border_width = event->border_width;
    std::cout << "border width requested from server" << std::endl;
    changes.sibling = event->above;
    std::cout << "siblings requested from server" << std::endl;
    changes.stack_mode = event->detail;
    std::cout << "stackmode requested from server" << std::endl;
    XConfigureWindow(display, event->window, event->value_mask, &changes);
    std::cout << "configured window using XConfigureWindow" << std::endl;

}

void focusWindow(Window window) {
    std::cout << "Entered focusWindow" << std::endl;
    XRaiseWindow(display, window);
    std::cout << "raised window" << std::endl;
    focusedWindow = window;
    XSetInputFocus(display, focusedWindow, RevertToPointerRoot, CurrentTime);
    std::cout << "server called for XsetInputFocus" << std::endl;
}

void closeWindow(Window window) {
    if (window == None) return;
    std::cout << "entered closeWindow" << std::endl;
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
    windows.remove(window);
    //focus another window
    Node* headNode = windows.getHead();  //grab some head
    if (headNode) {
        focusWindow(headNode->win);  //focus the first window in the list
    } else {
        focusedWindow = None;  //uo windows left to focus
    }
    std::cout << "success in killling focused window" << std::endl;
    if (mode==1) tiling();
}

void handleDestroyRequest(XDestroyWindowEvent* event){
    //find and remove the destroyed window from the linked list
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
