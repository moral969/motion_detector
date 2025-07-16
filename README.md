# 🎥 Motion Detection Tool

A command-line motion detection utility built with OpenCV, designed for analyzing video footage and automatically identifying moments of motion within a specified region. The program supports frame saving, interactive calibration, logging, and optional chapter tagging in output videos using FFmpeg.

## 📦 Features
- Detect motion within a user-defined rectangular area.
- Save frames where motion is detected.
- Skip frames to improve performance on long videos.
- Configure sensitivity, detection area, and minimum motion size.
- Interactive GUI calibration of the detection area.
- Export motion timestamps to a text file.
- Optional integration with FFmpeg to add or remove chapters in video based on motion events.

## 🛠️ Requirements
- C++17
- OpenCV (tested with OpenCV 4.x)
- FFmpeg (for chapter export/removal)

## ⚙️ Command-Line Usage
```bash
./motion_detector [options]
Basic Options
Option	Description
-h	Show help and exit
-i <file>	Input video file (default: input.mp4)
-o <file>	Output log file for detected motion timestamps (default: motion_times.txt)
-d <dir>	Directory to save detected motion frames (default: detected_frames)
-s <number>	Analyze every n-th frame (default: 20)
-t <number>	Motion threshold (pixel difference; lower = more sensitive; default: 25)
-a <number>	Minimum contour area in pixels to count as motion (default: 500)
-C <seconds>	Cooldown period in seconds between detections (default: 20.0)
-z	Enter interactive calibration mode (define detection area with mouse)
Detection Area Options
Option	Description
-x <number>	X coordinate of detection area's top-left corner (default: 100)
-y <number>	Y coordinate of detection area's top-left corner (default: 100)
-w <number>	Width of detection area (default: 200)
-H <number>	Height of detection area (default: 200)
Chapter Tagging (FFmpeg Integration)
Option	Description
-M	Add chapters to the video using timestamps in the output file
-R	Remove existing chapters from the video
🧠 How It Works
Frame Comparison: The tool processes every n-th frame (configurable with -s) and compares it to the previous processed frame.

Motion Area: Only a specific rectangular region (default or calibrated) is analyzed for motion.

Thresholding: If pixel differences between two frames exceed a threshold (-t), it is considered motion.

Contours: Binary difference images are analyzed to find contours (connected motion regions). Only contours larger than a given area (-a) are counted as valid motion.

Cooldown: After detecting motion, a cooldown period (-C) is enforced to avoid repeated detection of the same event.

Saving Frames: Detected motion frames are saved as .jpg files in the specified directory with timestamps in the filename.

Logging: Timestamps of motion events are written to a text file for further processing.

🧪 Example Commands
bash
# Show help
./motion_detector -h

# Basic usage with defaults
./motion_detector

# Use a custom video and detection directory
./motion_detector -i video.mp4 -d results/

# Analyze every 10th frame for faster processing
./motion_detector -s 10

# Set a custom detection area
./motion_detector -x 1000 -y 500 -w 600 -H 400

# Calibrate area interactively (use mouse)
./motion_detector -z

# Increase sensitivity (lower threshold)
./motion_detector -t 15

# Ignore small movements (increase contour area)
./motion_detector -a 1000

# Set shorter cooldown (in seconds)
./motion_detector -C 10

# Add chapters to video based on motion
./motion_detector -i video.mp4 -M -o motion_times.txt

# Remove chapters from video
./motion_detector -i video.mp4 -R
🖼️ Calibration Mode (-z)
Enter an interactive window where you can drag and resize the detection rectangle using your mouse. Press Enter or s to save the configuration, which will be stored in calibration.dat.

Use the output printed in terminal to reuse the calibrated values:

bash
Use the following CLI options for detection:
-x 1000 -y 500 -w 600 -H 400
📁 Output
Log File (motion_times.txt): Contains readable timestamps for when motion was detected.

Detection Frames: Saved as JPEG files in the specified directory, named like:

bash
detected_frames/frame_0001_00h02m45s.jpg
📌 Key Parameters Explained
Parameter	Meaning
frameSkip (-s)	Number of frames to skip between checks. Lower = more accurate but slower.
motionThreshold (-t)	Pixel intensity difference needed to register motion. Lower = more sensitive.
minContourArea (-a)	Minimum size (in pixels) of detected object to be considered motion. Useful to filter out noise.
cooldownSeconds (-C)	Time delay after a motion event before detecting again. Prevents multiple triggers for the same motion.
detectionArea (-x, -y, -w, -H)	Defines the rectangular region where motion is checked. Motion outside is ignored.
🎬 FFmpeg Integration
If compiled with FFmpeg support, the tool can embed chapter metadata into the video based on motion log, or remove existing chapter tracks:

bash
# Add chapters:
./motion_detector -i input.mp4 -M -o motion_times.txt

# Remove chapters:
./motion_detector -i input.mp4 -R
📂 File Structure (after build)
bash
├── motion_detector          # Motion detection binary
├── calibration.dat          # Saved detection area after calibration
├── motion_times.txt         # Log file with motion timestamps
├── detected_frames/         # Saved frames with motion
├── input.mp4                # Input video file
✅ License
MIT License (or specify yours)

