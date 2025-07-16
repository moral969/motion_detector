#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <iomanip>
#include <getopt.h>
#include <algorithm>

namespace fs = std::filesystem;

struct Options {
    std::string inputVideo = "with_chapters.mp4";
    std::string outputDir = "video_chunks";
    std::string outputVideo = "final_output.mp4";
    int beforeSec = 3;
    int afterSec = 3;
    bool concat = false;
    bool dryRun = false;
    bool mergeChapters = false;
    bool simpleCut = false;  // Новая опция: простая нарезка
    bool timestampMode = false;  // режим: нарезка ± от начала главы

};

struct Chapter {
    int startSec;
    int endSec;
};

// Нарезка строго вокруг временной метки (timestamp ± before/after)
std::vector<Chapter> fixedWindowCuts(const std::vector<Chapter>& chapters, int before, int after) {
    std::vector<Chapter> cuts;
    for (const auto& ch : chapters) {
        int start = std::max(0, ch.startSec - before);
        int end = ch.startSec + after;
        cuts.push_back({start, end});
    }
    return cuts;
}


// Форматирование времени в HH:MM:SS
std::string formatTime(int seconds) {
    std::ostringstream oss;
    int h = seconds / 3600;
    int m = (seconds % 3600) / 60;
    int s = seconds % 60;
    oss << std::setw(2) << std::setfill('0') << h << ":"
        << std::setw(2) << std::setfill('0') << m << ":"
        << std::setw(2) << std::setfill('0') << s;
    return oss.str();
}

// Извлечение глав через ffprobe
std::vector<Chapter> extractChapters(const std::string& video) {
    std::vector<Chapter> chapters;
    std::string tmpJson = "chapters.json";
    std::ostringstream cmd;
    cmd << "ffprobe -loglevel error -print_format json -show_chapters \"" << video << "\" > \"" << tmpJson << "\"";
    if (system(cmd.str().c_str()) != 0) {
        std::cerr << "ffprobe failed to extract chapters.\n";
        return {};
    }

    std::ifstream in(tmpJson);
    if (!in.is_open()) return {};
    
    std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    fs::remove(tmpJson);

    size_t pos = 0;
    while ((pos = json.find("\"start_time\":", pos)) != std::string::npos) {
        size_t startPos = json.find("\"", pos + 13) + 1;
        size_t startEnd = json.find("\"", startPos);
        float start = std::stof(json.substr(startPos, startEnd - startPos));

        pos = json.find("\"end_time\":", startEnd);
        size_t endPos = json.find("\"", pos + 11) + 1;
        size_t endEnd = json.find("\"", endPos);
        float end = std::stof(json.substr(endPos, endEnd - endPos));

        chapters.push_back({static_cast<int>(start), static_cast<int>(end)});
        pos = endEnd;
    }

    return chapters;
}

// Слияние пересекающихся глав
std::vector<Chapter> mergeChapters(const std::vector<Chapter>& original, int before, int after) {
    if (original.empty()) return {};
    
    std::vector<Chapter> result;
    Chapter current = {std::max(0, original[0].startSec - before), original[0].endSec + after};

    for (size_t i = 1; i < original.size(); ++i) {
        int start = std::max(0, original[i].startSec - before);
        int end = original[i].endSec + after;

        if (start <= current.endSec) {
            current.endSec = std::max(current.endSec, end);
        } else {
            result.push_back(current);
            current = {start, end};
        }
    }

    result.push_back(current);
    return result;
}

// Простая нарезка по главам (каждая отдельно, +before/+after)
std::vector<Chapter> expandChaptersIndividually(const std::vector<Chapter>& chapters, int before, int after) {
    std::vector<Chapter> expanded;
    for (const auto& ch : chapters) {
        int start = std::max(0, ch.startSec - before);
        int end = ch.endSec + after;
        expanded.push_back({start, end});
    }
    return expanded;
}

// Обрезка по сегментам
bool cutSegments(const std::string& video, const std::vector<Chapter>& segments, const std::string& dir, bool dryRun) {
    fs::create_directories(dir);
    int index = 1;
    for (const auto& seg : segments) {
        std::ostringstream outName;
        outName << dir << "/cut_" << std::setw(4) << std::setfill('0') << index++ << ".mp4";

        std::ostringstream cmd;
        cmd << "ffmpeg -ss " << formatTime(seg.startSec)
            << " -i \"" << video << "\" -t " << (seg.endSec - seg.startSec)
            << " -c copy \"" << outName.str() << "\" -y";

        std::cout << (dryRun ? "[DRY-RUN] " : "") << "Cutting: " << formatTime(seg.startSec)
                  << " -> " << formatTime(seg.endSec) << "\n";

        if (!dryRun && system(cmd.str().c_str()) != 0) {
            std::cerr << "Error cutting segment.\n";
            return false;
        }
    }
    return true;
}

// Склейка всех сегментов
bool concatenateSegments(const std::string& dir, const std::string& output) {
    std::vector<std::string> files;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() == ".mp4")
            files.push_back(entry.path().string());
    }

    std::sort(files.begin(), files.end());

    std::string listFile = dir + "/concat_list.txt";
    std::ofstream out(listFile);
    for (const auto& f : files)
        out << "file '" << fs::absolute(f).string() << "'\n";
    out.close();

    std::ostringstream cmd;
    cmd << "ffmpeg -f concat -safe 0 -i \"" << listFile << "\" -c copy \"" << output << "\" -y";
    return system(cmd.str().c_str()) == 0;
}

// Справка
void printUsage() {
    std::cout << "Video Cropper: обрезка видео по главам\n\n"
              << "Использование:\n"
              << "  video_cropper [опции]\n\n"
              << "Опции:\n"
              << "  -i <файл>      Входной видеофайл с главами (default: with_chapters.mp4)\n"
              << "  -b <сек>       Секунд ДО начала каждой главы (default: 3)\n"
              << "  -a <сек>       Секунд ПОСЛЕ окончания каждой главы (default: 3)\n"
              << "  -o <папка>     Папка для вырезанных фрагментов (default: video_chunks)\n"
              << "  -v <файл>      Путь к итоговому склеенному видео (если указан -c)\n"
              << "  -c             Склеить фрагменты в один итоговый файл\n"
              << "  -t             Нарезка по метке: только по времени начала главы (timestamp),\n"
              << "                 сдвигается на -b и +a секунд\n"
              << "  -m             Объединить пересекающиеся/смежные главы в один фрагмент\n"
              << "  -s             Простая нарезка: каждая глава расширяется на -b/-a секунд,\n"
              << "                 обрабатывается отдельно (игнорирует -m)\n"
              << "  -d             Dry-run: только показать, что будет сделано\n"
              << "  -h             Показать справку и выйти\n\n"
              << "Примеры:\n"
              << "  ./video_cropper -i input.mp4 -b 3 -a 3 -s\n"
              << "      Простая нарезка: каждая глава как отдельный файл, с расширением.\n\n"
              << "  ./video_cropper -i input.mp4 -b 5 -a 5 -m -c -v output.mp4\n"
              << "      Объединить главы, вырезать, склеить в output.mp4\n";
}

// Главная функция
int main(int argc, char** argv) {
    Options opt;
    int ch;
    while ((ch = getopt(argc, argv, "i:b:a:o:v:cmsdth")) != -1) {
        switch (ch) {
            case 'i': opt.inputVideo = optarg; break;
            case 'b': opt.beforeSec = std::stoi(optarg); break;
            case 'a': opt.afterSec = std::stoi(optarg); break;
            case 't': opt.timestampMode = true; break;
            case 'o': opt.outputDir = optarg; break;
            case 'v': opt.outputVideo = optarg; break;
            case 'c': opt.concat = true; break;
            case 'm': opt.mergeChapters = true; break;
            case 's': opt.simpleCut = true; break;
            case 'd': opt.dryRun = true; break;
            case 'h': printUsage(); return 0;
            default: printUsage(); return 1;
        }
    }

    if (!fs::exists(opt.inputVideo)) {
        std::cerr << "Input video not found: " << opt.inputVideo << "\n";
        return 1;
    }

    auto chapters = extractChapters(opt.inputVideo);
    if (chapters.empty()) {
        std::cerr << "No chapters found in input video.\n";
        return 1;
    }

    std::vector<Chapter> segments;

    if (opt.timestampMode) {
        segments = fixedWindowCuts(chapters, opt.beforeSec, opt.afterSec);
    } else if (opt.simpleCut) {
        segments = expandChaptersIndividually(chapters, opt.beforeSec, opt.afterSec);
    } else if (opt.mergeChapters) {
        segments = mergeChapters(chapters, opt.beforeSec, opt.afterSec);
    } else {
        segments = chapters;
    }

    if (!cutSegments(opt.inputVideo, segments, opt.outputDir, opt.dryRun))
        return 1;

    if (opt.concat && !opt.dryRun) {
        if (!concatenateSegments(opt.outputDir, opt.outputVideo)) {
            std::cerr << "Failed to concatenate segments.\n";
            return 1;
        }
        std::cout << "Final video saved to: " << opt.outputVideo << "\n";
    }

    return 0;
}
