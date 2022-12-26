# OnvifRtspLauncher
Rtsp Server Launcher to create an onvif compatible rtsp stream.

This is only an rtsp stream launcher that doesn't offer any soap capabilities.  
The goal of this utility is to have an pluggable process that can be launched from an Onvif Server. (e.g. [rpos](https://github.com/Quedale/rpos))

# Working
1. Extended GstRtspMediaFactory to manipulate pipeline dynamically.
2. v4l2 capability discovery to find compatible capture configuration.  (Adjust FPS and Resolution)
 - onvifserver will intentionally fail if configuration requires increasing FPS or scaling up resolution
3. RPi legacy hardware support. (Single stream)
4. openh264enc and x264enc software support.
5. Nvidia support can easily be enabled, but caused unrelated issues on my laptop so I disabled it.
6. Backchannel audio stream.

# WIP
1. RPi Legacy multi-stream support.
2. RPi Bullseye hardware support (v4l2-codec)
3. RockPro64 hardware support (rkmpp)
4. Snapshot launch option. (To support ONVIF Snapshot command)

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
Install the latest build tools from pip
```
sudo apt install python3-pip
python3 -m pip install pip --upgrade
python3 -m pip install setuptools
python3 -m pip install meson
python3 -m pip install ninja
python3 -m pip install libtool
```
I'm not sure if this is true for all distros, but this is required to get the new binary in the system's PATH
```
export PATH=$PATH:$HOME/.local/bin
```

Install gettext
```
sudo apt-get install gettext
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
