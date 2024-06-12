#include <iostream>
#include <thread>
#include <string>
#include <opencv2/opencv.hpp>
#include <cstdlib>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/XInput2.h>
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

bool isGreen(int r, int g, int b){
    return (102 <= r && r <= 220) && (200 <= g && g <= 255) && (0 <= b && b <= 125);
}

int main(void){

    pauseFl = true;
    stopFl = false;

    Display* display = XOpenDisplay(NULL); // for fast ckick and keyboardHandler
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

    cv::Mat image = cv::imread("area.png", cv::IMREAD_COLOR); // read image

    if(image.empty()) {
        std::cerr << "Не удалось открыть или найти изображение!" << std::endl;
        return -1;
    }
    
    // get image size
    int width = image.cols;
    int height = image.rows;
    
    while(!stopFl){
        if(pauseFl) continue; // pause

        for(int y = 0; y < height; y+=20) {
            for(int x = 0; x < width; x+=20) {
                if(stopFl) break;
                cv::Vec3b color = image.at<cv::Vec3b>(cv::Point(x, y));

                int blue = color[0];
                int green = color[1];
                int red = color[2];

                if(isGreen(red, green, blue)){
                    click(display, x_gl + x, y_gl + y);
                    empty = false;
                }
            }
            if(stopFl) break;
        }
        
        std::system(maim_com.c_str());
        image = cv::imread("area.png", cv::IMREAD_COLOR);
        if(image.empty()) {
            std::cerr << "Failed to open image area.png!" << std::endl;
            return -1;
        }
    }
    XCloseDisplay(display);
    keyboardHandler.join();
    return 0;
}
