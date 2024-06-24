#include <iostream>
#include <thread>
#include <string>
#include <opencv2/opencv.hpp>
#include <cstdlib>
#include <termios.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/XInput2.h>
#define PART 7

int x_gl, y_gl; // window position
bool pauseFl;
bool stopFl;

void handleEvent(XIDeviceEvent* ev) {
    if ((ev->evtype == XI_KeyPress) && (ev->detail == 33) ) {
        pauseFl = !pauseFl; 
        if(pauseFl) std::cout << "Pause on" << std::endl;
        else std::cout << "Pause off" << std::endl;
    } else if ((ev->evtype == XI_KeyRelease) && (ev->detail == 24)) {
        stopFl = true;
        std::cout << "Good luck!" << std::endl;
    }
}
// Функция для установки терминала в режим без эха
void setEcho(bool enable) {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    if (!enable) {
        // Отключаем эхо
        tty.c_lflag &= ~ECHO;
    } else {
        // Включаем эхо
        tty.c_lflag |= ECHO;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

std::string exec(std::string command) { // give output from command
   char buffer[128];
   std::string result = "";

   // Open pipe to file
   FILE* pipe = popen(command.c_str(), "r");
   if (!pipe) {
      return "popen failed!";
   }
   // read till end of process:
   while (!feof(pipe)) {

      // use buffer to read and add to result
      if (fgets(buffer, 128, pipe) != NULL)
         result += buffer;
   }

   pclose(pipe);
   return result;
}

void getWindowPosition(int windowId, int *x, int *y) {
    std::string command = "xdotool getwindowgeometry " + std::to_string(windowId);
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Failed to run xdotool command" << std::endl;
        exit(1);
    }

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        std::string line(buffer);
        if (line.find("Position:") != std::string::npos) {
            sscanf(buffer, "  Position: %d,%d", x, y);
        }
    }
    pclose(pipe);
}


void click(Display* display, int x, int y) {
    XTestFakeMotionEvent(display, -1, x, y, CurrentTime);
    XTestFakeButtonEvent(display, 1, True, CurrentTime);
    XTestFakeButtonEvent(display, 1, False, CurrentTime);
    XFlush(display);
}

void handler(Display *display){ // keyboardHandler
    int opcode, event, error;
    if (!XQueryExtension(display, "XInputExtension", &opcode, &event, &error)) {
        std::cerr << "X Input extension не доступен" << std::endl;
        return;
    }
    XIEventMask evmask;
    unsigned char mask[(XI_LASTEVENT + 7)/8] = { 0 };
    evmask.deviceid = XIAllDevices;
    evmask.mask_len = sizeof(mask);
    evmask.mask = mask;
    XISetMask(mask, XI_KeyPress);
    XISetMask(mask, XI_KeyRelease);

    Window root = DefaultRootWindow(display);
    XISelectEvents(display, root, &evmask, 1);

    XEvent ev;
    while (!stopFl) {
        XNextEvent(display, &ev);
        if (ev.xcookie.type == GenericEvent && ev.xcookie.extension == opcode) {
            XGetEventData(display, &ev.xcookie);
            handleEvent(reinterpret_cast<XIDeviceEvent*>(ev.xcookie.data));
            XFreeEventData(display, &ev.xcookie);
        }
    }
}

bool isWhite(int red, int green, int blue){
    return (red == 255) && (green == 255) && (blue == 255);
}
cv::Mat image, mask;
Display* display;

void ParsePieceImage(int sY, int stY, int stX){
    for(int y = sY; y < stY; y+=13){
        for(int x = 0; x < stX; x+=13){
            if(mask.at<uchar>(cv::Point(x,y)) == 255){
               click(display, x+x_gl, y+y_gl+4);
            }
        }
    }
}

int main(void){
    setEcho(false);
    pauseFl = true;
    stopFl = false;

    display = XOpenDisplay(NULL); // for fast ckick and keyboardHandler
    if (display == NULL) {
        std::cerr << "X display is not open" << std::endl;
        return 1;
    }

    std::thread keyboardHandler = std::thread(handler, display);

    std::cout << "Start blum abuse! Select window with blum" << std::endl;

    std::string windowId = exec("xdotool selectwindow"); // get window ID
    windowId.pop_back(); // remove '\n'
                        
    std::string maim_com = "maim -i " + windowId + " area.png"; // get first screenshot
    std::system(maim_com.c_str());
    getWindowPosition(std::stoi(windowId), &x_gl, &y_gl); // init window coords

    std::cout << std::endl;
    std::cout << "Nice. Tap 'play' and press 'p'" << std::endl;
    std::cout << "---------------------------------------------------------" << std::endl;
    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << "Press 'p' for pause, 'q' to exit" << std::endl;

    image = cv::imread("area.png", cv::IMREAD_COLOR); // read image
    cv::Mat hsv_image;
    cv::cvtColor(image, hsv_image, cv::COLOR_BGR2HSV);
    cv::Scalar lower_green(32, 100, 100); // bottom
    cv::Scalar upper_green(42, 255, 255); // top                                      
    cv::inRange(hsv_image, lower_green, upper_green, mask); // make middle of snow white, other - black

    if(image.empty()) {
        std::cerr << "Failed to open image area.png!" << std::endl;
        return -1;
    }

    // get image size
    int width = image.cols;
    int height = image.rows;
    int x_click, y_click;
    int countPiece = (height / PART);

    std::vector<std::thread> threads;
    while(!stopFl){
        if(pauseFl) continue; // pause
                              //
        for(int y = countPiece; y < height; y += countPiece){
            threads.emplace_back(ParsePieceImage, y - countPiece, y, width); 
        }
        for(auto& th : threads){
            th.join();
        }

        threads.clear();

        std::system(maim_com.c_str());

        image = cv::imread("area.png", cv::IMREAD_COLOR);

        if(image.empty()) {
            std::cerr << "Failed to open image area.png!" << std::endl;
            return -1;
        }
        cv::cvtColor(image, hsv_image, cv::COLOR_BGR2HSV);
        cv::inRange(hsv_image, lower_green, upper_green, mask);

    }

    XCloseDisplay(display);
    keyboardHandler.join();
    setEcho(true);
    return 0;
}
