# OnvifRtspLauncher
Rtsp Server Launcher to create an onvif compatible rtsp stream.

This is only an rtsp stream launcher that doesn't offer any soap capabilities.  
The goal of this utility is to have an pluggable process that can be launched from an Onvif Server. (e.g. [rpos](https://github.com/Quedale/rpos))

# Working
1. Extended GstRtspMediaFactory to manipulate pipeline dynamically.
2. v4l2 capability discovery to find compatible capture configuration.  (Adjust FPS and Resolution)
 - onvifserver will intentionally fail if configuration requires increasing FPS or scaling up resolution
3. Raw Capture Input (YUV2 for now)
4. RPi Legacy OMX hardware support. (Single stream)
5. RPi Bullseye v4l2 hardware support (Single stream)
6. openh264, x264, libav software support.
7. Nvidia support can easily be enabled, but causes unrelated issues on my laptop so I disabled it for now.
8. Backchannel audio stream.
9. Snapshots capabilities (To support ONVIF Snapshot command)

# WIP/TODO
1. RPi multi-stream support.
2. RockPro64 hardware support (rkmpp)
3. autogen.sh error handling
4. H264 Capture input support. (e.g. Picam)

# Usage
```
Usage: onvifserver [OPTION...]
Your program description.

  -a, --audio=AUDIO          Input audio device. (Default: auto)
  -e, --encoder=ENCODER      Gstreamer encoder. (Default: auto)
  -f, --fps=FPS              Video output framerate. (Default: 10)
  -h, --height=HEIGHT        Video output height. (Default: 480)
  -m, --mount=MOUNT          URL mount point. (Default: h264)
  -p, --port=PORT            Network port to listen. (Default: 8554)
  -v, --video=VIDEO          Input video device. (Default: /dev/video0)
  -w, --width=WIDTH          Video output width. (Default: 640)
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version
```

# Build Dependencies
Note that I don't like depending on sudo. I will eventually get around to identifying and building missing dependencies.

Install the latest build tools from pip and apt
```
sudo apt install python3-pip
python3 -m pip install pip --upgrade
python3 -m pip install meson
python3 -m pip install ninja
sudo apt install gettext
sudo apt install pkg-config
sudo apt install libtool
sudo apt install flex
sudo apt install bison
sudo apt install nasm
sudo apt install libasound2-dev
sudo apt install libpulse-dev
sudo apt install libgudev-1.0-dev
sudo apt install libv4l-dev
```
I'm not sure if this is true for all distros, but this is required to get the new binary in the system's PATH
```
export PATH=$PATH:$HOME/.local/bin
```

# How to build
### Clone repository
```
git clone https://github.com/Quedale/OnvifRtspLauncher.git
cd OnvifRtspLauncher
```
### autogen gstreamer and v4l-utils dependencies
This project support out-of-tree build to keep src directory clean.

Generic build:
```
mkdir build && cd build
../autogen.sh --prefix=$(pwd)/dist
```

Raspberry Pi Legacy build:
```
mkdir build && cd build
../autogen.sh --enable-rpi --prefix=$(pwd)/dist
```
### Compile and install GUI App
```
make
make install
```

Static executables can be found under OnvifRtspLauncher/build/dist/bin

# 
# Feedback is more than welcome
