#include <opencv2/opencv.hpp>
#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>
#include <filesystem>
#include <unistd.h>
#include "markCreator.h"

using namespace cv;
using namespace std;
namespace fs = filesystem;


struct Settings {
    string videoPath = "input.mp4";
    string outputFile = "motion_times.txt";
    string saveDir = "detected_frames";
    string calibrationFile = "calibration.dat";
    Rect detectionArea{100, 100, 200, 200};
    int frameSkip = 20;
    int motionThreshold = 25;
    int minContourArea = 500;
    double cooldownSeconds = 20.0;
    bool calibrateMode = false;
};

Settings settings;
double lastDetectionTime = -settings.cooldownSeconds;
int savedFrameCount = 0;
bool exportChapters = false;
bool removeChapters = false;
std::string outputVideoWithChapters = "with_chapters.mp4";
std::string outputVideoClean = "clean.mp4";

void printHelp() {
    cout << "Видео детектор движения — подробное руководство\n\n";

    cout << "Использование:\n"
         << "  motion_detector [опции]\n\n";

    cout << "Основные параметры:\n"
         << "  -h               Вывести эту справку и выйти\n"
         << "  -i <файл>        Входной видеофайл (по умолчанию: input.mp4)\n"
         << "  -o <файл>        Выходной лог-файл для времени движения (по умолчанию: motion_times.txt)\n"
         << "  -d <дир>         Папка для сохранения кадров с движением (по умолчанию: detected_frames)\n"
         << "  -s <число>       Анализировать каждый n-й кадр (по умолчанию: 20)\n"
         << "  -t <число>       Порог обнаружения движения (чувствительность, по умолчанию: 25)\n"
         << "  -a <число>       Минимальная площадь контура для учета (по умолчанию: 500)\n"
         << "  -C <число>       Время перезарядки между событиями в секундах (по умолчанию: 20.0)\n"
         << "  -z               Режим калибровки (интерактивный выбор области движения)\n\n"
         << "  -M               Добавить главы в видео на основе лог-файла движения. Файл с метками залать по опции -o file.txt\n"
         << "  -R               Удалить главы из видео (если они есть)\n";

    cout << "Параметры области обнаружения движения:\n"
         << "  -x <число>       Координата X левого верхнего угла (по умолчанию: 100)\n"
         << "  -y <число>       Координата Y левого верхнего угла (по умолчанию: 100)\n"
         << "  -w <число>       Ширина области (по умолчанию: 200)\n"
         << "  -H <число>       Высота области (по умолчанию: 200)\n\n";

    cout << "Примеры запуска:\n"
         << "  Просмотр справки:\n"
         << "    ./motion_detector -h\n\n"

         << "  Анализ видео с настройками по умолчанию:\n"
         << "    ./motion_detector\n\n"

         << "  Анализ конкретного видео с сохранением кадров в указанную папку:\n"
         << "    ./motion_detector -i /home/user/videos/video1.mp4 -d /home/user/detections\n\n"

         << "  Анализ каждого 10-го кадра для ускорения обработки:\n"
         << "    ./motion_detector -s 10\n\n"

         << "  Изменение области обнаружения движения:\n"
         << "    ./motion_detector -x 1150 -y 600 -w 600 -H 460\n\n"

         << "  Запуск интерактивной калибровки:\n"
         << "    ./motion_detector -z -i /home/user/videos/video1.mp4\n\n"

         << "  Запуск с параметрами области после калибровки:\n"
         << "    ./motion_detector -i /home/user/videos/video1.mp4 -x 1150 -y 600 -w 600 -H 460\n\n"

         << "  Увеличение чувствительности (понижение порога):\n"
         << "    ./motion_detector -t 15\n\n"

         << "  Игнорирование мелких шумов (увеличение минимальной площади контура):\n"
         << "    ./motion_detector -a 1000\n\n"

         << "  Установка времени перезарядки между событиями (например, 10 секунд):\n"
         << "    ./motion_detector -C 10\n\n"
         
         << "  Добавление меток на видео:\n"
         << "    ./motion_detector -i input.mp4 -M -o timestamp_file.txt\n\n";

    cout << "Описание работы:\n"
         << "  Программа загружает видео и сравнивает каждый n-й кадр с предыдущим.\n"
         << "  Разница анализируется только в заданной области. Если обнаружено движение,\n"
         << "  время фиксируется в лог, а кадр сохраняется в папку с детекциями.\n"
         << "  Между событиями соблюдается пауза cooldown, чтобы избежать частых срабатываний.\n\n";
}


void parseArguments(int argc, char** argv) {
    int opt;
    while ((opt = getopt(argc, argv, "hi:o:d:s:t:a:C:x:y:w:H:zMR")) != -1) {
        try {
            switch (opt) {
                case 'h':
                    printHelp();
                    exit(0);
                case 'i':
                    settings.videoPath = optarg;
                    break;
                case 'o':
                    settings.outputFile = optarg;
                    break;
                case 'd':
                    settings.saveDir = optarg;
                    break;
                case 's':
                    settings.frameSkip = stoi(optarg);
                    break;
                case 't':
                    settings.motionThreshold = stoi(optarg);
                    break;
                case 'a':
                    settings.minContourArea = stoi(optarg);
                    break;
                case 'C':
                    settings.cooldownSeconds = stod(optarg);
                    break;
                case 'x':
                    settings.detectionArea.x = stoi(optarg);
                    break;
                case 'y':
                    settings.detectionArea.y = stoi(optarg);
                    break;
                case 'w':
                    settings.detectionArea.width = stoi(optarg);
                    break;
                case 'H':
                    settings.detectionArea.height = stoi(optarg);
                    break;
                case 'z':
                    settings.calibrateMode = true;
                    break;
                case '?':
                    cerr << "Unknown option or missing argument." << endl;
                    exit(1);
                case 'M':
                    exportChapters = true;
                    break;
                case 'R':
                    removeChapters = true;
                    break;
                default:
                    cerr << "Error in command line arguments" << endl;
                    exit(1);
            }
        } catch (const exception& e) {
            cerr << "Error parsing argument for option -" << (char)opt << ": " << e.what() << endl;
            exit(1);
        }
    }
}

string formatTimestamp(double seconds) {
    int totalSecs = static_cast<int>(seconds);
    int hours = totalSecs / 3600;
    int minutes = (totalSecs % 3600) / 60;
    int secs = totalSecs % 60;

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hours, minutes, secs);
    return string(buffer);
}


void saveCalibration() {
    ofstream calFile(settings.calibrationFile);
    if (calFile.is_open()) {
        calFile << settings.detectionArea.x << " "
                << settings.detectionArea.y << " "
                << settings.detectionArea.width << " "
                << settings.detectionArea.height;
        calFile.close();
        cout << "Calibration saved to: " << settings.calibrationFile << endl;
    } else {
        cerr << "Failed to save calibration" << endl;
    }
}

void loadCalibration() {
    ifstream calFile(settings.calibrationFile);
    if (calFile.is_open()) {
        int x, y, w, h;
        calFile >> x >> y >> w >> h;
        settings.detectionArea = Rect(x, y, w, h);
        calFile.close();
        cout << "Loaded calibration from: " << settings.calibrationFile << endl;
    }
}

void runCalibration() {
    VideoCapture cap(settings.videoPath);
    if (!cap.isOpened()) {
        cerr << "Error opening video file for calibration" << endl;
        return;
    }

    int targetFrame = 121;
    cap.set(CAP_PROP_POS_FRAMES, targetFrame);
    
    Mat frame;
    cap >> frame;
    if (frame.empty()) {
        cerr << "Error reading calibration frame" << endl;
        return;
    }

    rectangle(frame, settings.detectionArea, Scalar(0, 255, 0), 2);
    putText(frame, "Detection Area", Point(settings.detectionArea.x, settings.detectionArea.y-10),
            FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 1);

    string calImage = "calibration_frame.jpg";
    imwrite(calImage, frame);
    cout << "Calibration frame saved as: " << calImage << endl;

    saveCalibration();
}

void ensureDirectoryExists(const string& path) {
    if (!fs::exists(path)) {
        fs::create_directory(path);
    }
}

string generateFilename(double timestamp) {
    char buffer[80];
    int h = static_cast<int>(timestamp) / 3600;
    int m = (static_cast<int>(timestamp) % 3600) / 60;
    int s = static_cast<int>(timestamp) % 60;
    snprintf(buffer, sizeof(buffer), "%s/frame_%04d_%02dh%02dm%02ds.jpg", 
            settings.saveDir.c_str(), savedFrameCount++, h, m, s);
    return string(buffer);
}

void saveDetectionFrame(const Mat& frame, double timestamp) {
    string filename = generateFilename(timestamp);
    if (!imwrite(filename, frame)) {
        cerr << "Failed to save detection frame: " << filename << endl;
    } else {
        cout << "Saved detection frame: " << filename << endl;
    }
}

bool isCoolingDown(double currentTime) {
    return (currentTime - lastDetectionTime) < settings.cooldownSeconds;
}

void detectMotion(const Mat& grayPrev, const Mat& grayCurrent, const Mat& originalFrame, ofstream& outFile, double timestamp) {
    if (isCoolingDown(timestamp)) return;

    Mat roiPrev = grayPrev(settings.detectionArea);
    Mat roiCurrent = grayCurrent(settings.detectionArea);

    Mat frameDiff;
    absdiff(roiPrev, roiCurrent, frameDiff);

    Mat thresholdDiff;
    threshold(frameDiff, thresholdDiff, settings.motionThreshold, 255, THRESH_BINARY);

    Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
    morphologyEx(thresholdDiff, thresholdDiff, MORPH_OPEN, kernel);

    vector<vector<Point>> contours;
    findContours(thresholdDiff, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    for (const auto& contour : contours) {
        if (contourArea(contour) > settings.minContourArea) {
            lastDetectionTime = timestamp;
            string timeStr = formatTimestamp(timestamp);
            outFile << "Motion detected at: " << timeStr << endl;
            cout << "Motion detected at: " << timeStr << endl;
            saveDetectionFrame(originalFrame, timestamp);
            break;
        }
    }
}

void runDetection() {
    ensureDirectoryExists(settings.saveDir);
    loadCalibration();

    VideoCapture cap(settings.videoPath);
    if (!cap.isOpened()) {
        cerr << "Error opening video file: " << settings.videoPath << endl;
        exit(1);
    }

    ofstream outFile(settings.outputFile);
    if (!outFile.is_open()) {
        cerr << "Error opening output file: " << settings.outputFile << endl;
        exit(1);
    }

    Mat prevFrame;
    cap >> prevFrame;
    if (prevFrame.empty()) {
        cerr << "Error reading first frame" << endl;
        exit(1);
    }

    Mat grayPrev;
    cvtColor(prevFrame, grayPrev, COLOR_BGR2GRAY);

    int frameCount = 0;
    double fps = cap.get(CAP_PROP_FPS);
    if (fps <= 0) fps = 30;

    cout << "Starting motion detection with settings:" << endl;
    cout << "  Video: " << settings.videoPath << endl;
    cout << "  Detection area: [" << settings.detectionArea.x << ", " << settings.detectionArea.y 
         << ", " << settings.detectionArea.width << ", " << settings.detectionArea.height << "]" << endl;
    cout << "  Frame skip: " << settings.frameSkip << endl;
    cout << "  Motion threshold: " << settings.motionThreshold << endl;
    cout << "  Min contour area: " << settings.minContourArea << endl;
    cout << "  Cooldown: " << settings.cooldownSeconds << " seconds" << endl;

    while (true) {
        Mat currentFrame;
        cap >> currentFrame;
        if (currentFrame.empty()) break;

        frameCount++;
        if (frameCount % settings.frameSkip != 0) continue;

        double timestamp = cap.get(CAP_PROP_POS_MSEC) / 1000.0;

        Mat grayCurrent;
        cvtColor(currentFrame, grayCurrent, COLOR_BGR2GRAY);

        detectMotion(grayPrev, grayCurrent, currentFrame, outFile, timestamp);

        grayCurrent.copyTo(grayPrev);
    }

    cap.release();
    outFile.close();
    cout << "Processing complete. Results saved to " << settings.outputFile << endl;
    cout << "Detection frames saved in: " << settings.saveDir << endl;
}

// Helpers made global so onMouse can use them
static bool inside(const cv::Point& p, const cv::Rect& r) {
    return r.contains(p);
}

static bool nearCorner(const cv::Point& p, const cv::Rect& r, int radius = 10) {
    return (abs(p.x - (r.x + r.width)) < radius && abs(p.y - (r.y + r.height)) < radius);
}

void interactiveCalibration() {
    VideoCapture cap(settings.videoPath);
    if (!cap.isOpened()) {
        cerr << "Error opening video file for calibration" << endl;
        return;
    }

    int targetFrame = 121;
    cap.set(CAP_PROP_POS_FRAMES, targetFrame);
    Mat frame;
    cap >> frame;
    if (frame.empty()) {
        cerr << "Error reading frame" << endl;
        return;
    }

    struct DragState {
        Point dragStart;
        enum DragMode { NONE, MOVE, RESIZE } mode = NONE;
    };

    DragState drag;
    string winName = "Calibration - Press 's' or Enter to save";
    namedWindow(winName, WINDOW_NORMAL);

    // Mouse callback function
    auto onMouse = [](int event, int x, int y, int flags, void* userdata) {
        auto* state = static_cast<DragState*>(userdata);
        Point pt(x, y);
        Rect& box = settings.detectionArea;

        switch (event) {
            case EVENT_LBUTTONDOWN:
                if (nearCorner(pt, box)) {
                    state->mode = DragState::RESIZE;
                } else if (inside(pt, box)) {
                    state->mode = DragState::MOVE;
                } else {
                    state->mode = DragState::NONE;
                }
                state->dragStart = pt;
                break;

            case EVENT_MOUSEMOVE:
                if (flags & EVENT_FLAG_LBUTTON) {
                    Point delta = pt - state->dragStart;
                    if (state->mode == DragState::MOVE) {
                        box.x += delta.x;
                        box.y += delta.y;
                    } else if (state->mode == DragState::RESIZE) {
                        box.width += delta.x;
                        box.height += delta.y;
                    }
                    state->dragStart = pt;
                }
                break;

            case EVENT_LBUTTONUP:
                state->mode = DragState::NONE;
                break;
        }
    };

    setMouseCallback(winName, onMouse, &drag);

    while (true) {
        Mat display;
        frame.copyTo(display);
        rectangle(display, settings.detectionArea, Scalar(0, 255, 0), 2);
        putText(display, "Drag to adjust area. Press 's' or Enter to save.", Point(10, 25),
                FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 1);
        imshow(winName, display);
        int key = waitKey(20);
        if (key == 13 || key == 's') break;  // Enter or 's'
        if (key == 27) return;              // Esc to cancel
    }

    destroyWindow(winName);
    saveCalibration();

    cout << "Use the following CLI options for detection:" << endl;
    cout << "-x " << settings.detectionArea.x
         << " -y " << settings.detectionArea.y
         << " -w " << settings.detectionArea.width
         << " -H " << settings.detectionArea.height << endl;
}




int main(int argc, char** argv) {
    try {
        parseArguments(argc, argv);
        if (settings.calibrateMode) {
            // runCalibration();
            interactiveCalibration();
        } else {
            runDetection();
        }

        if (exportChapters) {
            if (addFFmpegChapters(settings.videoPath, settings.outputFile, outputVideoWithChapters)) {
                cout << "Chapters added to video: " << outputVideoWithChapters << endl;
            } else {
                cerr << "Failed to add chapters.\n";
            }
        }

        if (removeChapters) {
            if (removeFFmpegChapters(settings.videoPath, outputVideoClean)) {
                cout << "Chapters removed. Clean video saved as: " << outputVideoClean << endl;
            } else {
                cerr << "Failed to remove chapters.\n";
            }
        }

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}
