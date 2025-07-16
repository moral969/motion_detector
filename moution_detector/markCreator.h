#pragma once
#include <string>

bool addFFmpegChapters(const std::string& inputVideo,
                       const std::string& labelsFile,
                       const std::string& outputVideo);

bool removeFFmpegChapters(const std::string& inputVideo,
                          const std::string& outputVideo);
