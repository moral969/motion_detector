#include "markCreator.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>

static bool generateFFmetadata(const std::string& labelsFile, const std::string& metadataFile) {
    std::ifstream in(labelsFile);
    if (!in.is_open()) {
        std::cerr << "Cannot open labels file: " << labelsFile << "\n";
        return false;
    }

    std::ofstream out(metadataFile);
    if (!out.is_open()) {
        std::cerr << "Cannot create metadata file: " << metadataFile << "\n";
        return false;
    }

    out << ";FFMETADATA1\n";
    std::string line;
    int chapterIndex = 1;

    while (std::getline(in, line)) {
        size_t pos = line.find("Motion detected at: ");
        if (pos == std::string::npos) continue;

        std::string timestamp = line.substr(pos + 20);
        if (timestamp.empty()) continue;

        int h, m, s;
        char dummy;
        std::istringstream tsStream(timestamp);
        tsStream >> h >> dummy >> m >> dummy >> s;
        long startSec = h * 3600 + m * 60 + s;

        out << "\n[CHAPTER]\n";
        out << "TIMEBASE=1/1\n";
        out << "START=" << startSec << "\n";
        out << "END=" << (startSec + 1) << "\n";
        out << "title=Motion " << chapterIndex++ << "\n";
    }

    return true;
}

bool addFFmpegChapters(const std::string& inputVideo,
                       const std::string& labelsFile,
                       const std::string& outputVideo) {
    const std::string metadataFile = "ffmetadata_temp.txt";
    if (!generateFFmetadata(labelsFile, metadataFile)) return false;

    std::ostringstream cmd;
    cmd << "ffmpeg -i \"" << inputVideo << "\" -i \"" << metadataFile
        << "\" -map_metadata 1 -c copy -y \"" << outputVideo << "\"";

    int rc = system(cmd.str().c_str());
    std::remove(metadataFile.c_str());
    return rc == 0;
}

bool removeFFmpegChapters(const std::string& inputVideo,
                          const std::string& outputVideo) {
    std::ostringstream cmd;
    cmd << "ffmpeg -i \"" << inputVideo
        << "\" -map_metadata -1 -c copy -y \"" << outputVideo << "\"";
    return system(cmd.str().c_str()) == 0;
}
