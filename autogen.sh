#!/bin/bash
ENABLE_RPI=0
ENABLE_LIBAV=0
ENABLE_CLIENT=0
i=1;
for arg in "$@"
do
    if [ "$arg" == "--enable-libav" ]; then
        ENABLE_LIBAV=1
    elif [ "$arg" == "--enable-rpi" ]; then
        ENABLE_RPI=1
    elif [ "$arg" == "--enable-client" ]; then
        ENABLE_CLIENT=1
    fi
    i=$((i + 1));
done

#Save current working directory to run configure in
WORK_DIR=$(pwd)

#Get project root directory based on autogen.sh file location
SCRT_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)
cd $SCRT_DIR

#Initialize project
aclocal
autoconf
automake --add-missing
autoreconf -i

mkdir -p subprojects
cd subprojects

################################################################
# 
#    Build v4l2-utils dependency
#   sudo apt install libv4l2-dev (tested 1.16.3)
# 
################################################################
PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SCRT_DIR/subprojects/v4l-utils/dist/lib/pkgconfig \
pkg-config --exists --print-errors "libv4l2 >= 1.16.3"
ret=$?
if [ $ret != 0 ]; then 
  git -C v4l-utils pull 2> /dev/null || git clone -b v4l-utils-1.22.1 https://github.com/gjasny/v4l-utils.git
  cd v4l-utils
  ./bootstrap.sh
  ./configure --prefix=$(pwd)/dist --enable-static --disable-shared --with-udevdir=$(pwd)/dist/udev
  make -j$(nproc)
  make install 
  cd ..
else
  echo "libv4l2 already found."
fi

################################################################
# 
#    Build Gstreamer dependency
# 
################################################################
git -C gstreamer pull 2> /dev/null || git clone -b 1.21.3 https://gitlab.freedesktop.org/gstreamer/gstreamer.git
cd gstreamer

GST_DIR=$(pwd)
# Add tinyalsa fallback subproject
# echo "[wrap-git]" > subprojects/tinyalsa.wrap
# echo "directory=tinyalsa" >> subprojects/tinyalsa.wrap
# echo "url=https://github.com/tinyalsa/tinyalsa.git" >> subprojects/tinyalsa.wrap
# echo "revision=v2.0.0" >> subprojects/tinyalsa.wrap

# Full Build >9k Files
# meson build \
#   --buildtype=release \
#   --strip \
#   --default-library=static \
#   --wrap-mode=forcefallback \
#   -Dpango:harfbuzz=disabled \
#   -Dfreetype2:harfbuzz=disabled \
#   -Dpixman:tests=disabled \
#   -Dintrospection=disabled \
#   -Dpython=disabled \
#   -Dtests=disabled \
#   -Dexamples=disabled \
#   -Dgst-plugins-bad:nvcodec=enabled \
#   -Dgst-plugins-bad:iqa=disabled \
#   -Dgpl=enabled \
#   --pkg-config-path=/home/quedale/git/OnvifRtspLauncher/subprojects/gstreamer/build/dist/lib/x86_64-linux-gnu/pkgconfig \
#   --prefix=$(pwd)/build/dist

MESON_PARAMS=""

# Force disable subproject features
MESON_PARAMS="$MESON_PARAMS -Dglib:tests=false"
MESON_PARAMS="$MESON_PARAMS -Dlibdrm:cairo-tests=false"
MESON_PARAMS="$MESON_PARAMS -Dx264:cli=false"

MESON_PARAMS="$MESON_PARAMS -Dbase=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgood=enabled"
MESON_PARAMS="$MESON_PARAMS -Dbad=enabled"
# MESON_PARAMS="$MESON_PARAMS -Dugly=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgpl=enabled"
MESON_PARAMS="$MESON_PARAMS -Drtsp_server=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:app=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:typefind=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:audiotestsrc=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:videotestsrc=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:playback=enabled"
if [ $ENABLE_CLIENT -eq 1 ]; then
  MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:x11=enabled"
  MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:xvideo=enabled"
fi
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:alsa=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:videoconvertscale=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:videorate=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:rawparse=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:pbtypes=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:audioresample=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:audioconvert=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:volume=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:tcp=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:v4l2=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:rtsp=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:rtp=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:rtpmanager=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:law=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:autodetect=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:pulse=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:interleave=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:audioparsers=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:udp=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:openh264=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:nvcodec=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:v4l2codecs=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:fdkaac=enabled"
# MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:tinyalsa=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:videoparsers=enabled"
# MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:switchbin=enabled"
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:onvif=enabled"
# MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-ugly:x264=enabled"

#Below is required for to workaround https://gitlab.freedesktop.org/gstreamer/gstreamer/-/issues/1056
# This is to support v4l2h264enc element with capssetter
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:debugutils=enabled"

# This is required for the snapshot feature
MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:png=enabled"

#  -Dgst-plugins-base:overlaycomposition=enabled \
#  -Dgst-plugins-good:oss=enabled \
#  -Dgst-plugins-good:oss4=enabled \
#  -Dgst-plugins-bad:mpegtsmux=enabled \
#  -Dgst-plugins-bad:mpegtsdemux=enabled \

# Customized build <2K file
rm -rf build
meson setup build \
  --buildtype=release \
  --strip \
  --default-library=static \
  --wrap-mode=forcefallback \
  -Dauto_features=disabled \
  $MESON_PARAMS \
  --prefix=$GST_DIR/build/dist \
  --libdir=lib

meson compile -C build
meson install -C build

if [ $ENABLE_LIBAV -eq 1 ]; then
  echo "LIBAV Feature enabled..."
  cd ..
  #######################
  #
  # Custom FFmpeg build
  #   For some reason, Gstreamer's meson dep doesn't build any codecs
  #
  #######################
  git -C FFmpeg pull 2> /dev/null || git clone -b n5.1.2 https://github.com/FFmpeg/FFmpeg.git
  cd FFmpeg

  ./configure --prefix=$(pwd)/dist \
      --disable-lzma \
      --disable-doc \
      --disable-shared \
      --enable-static \
      --enable-nonfree \
      --enable-version3 \
      --enable-gpl 
  make -j$(nproc)
  make install
  rm -rf dist/lib/*.so
  cd ..

  #######################
  #
  # Rebuild gstreamer with libav with nofallback
  #   TODO : optionally add omx or rkmpp etc
  #
  #######################
  #Rebuild with custom ffmpeg build
  cd gstreamer
  MESON_PARAMS="-Dlibav=enabled"
  rm -rf libav_build
  LIBRARY_PATH=$LIBRARY_PATH:$GST_DIR/build/dist/lib:$SCRT_DIR/subprojects/FFmpeg/dist/lib \
  LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$GST_DIR/build/dist/lib:$SCRT_DIR/subprojects/FFmpeg/dist/lib \
  PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$GST_DIR/build/dist/lib/pkgconfig:$SCRT_DIR/subprojects/FFmpeg/dist/lib/pkgconfig \
  meson setup libav_build \
  --buildtype=release \
  --strip \
  --default-library=static \
  --wrap-mode=nofallback \
  -Dauto_features=disabled \
  $MESON_PARAMS \
  --prefix=$GST_DIR/libav_build/dist \
  --libdir=lib
  LIBRARY_PATH=$LIBRARY_PATH:$GST_DIR/build/dist/lib:$SCRT_DIR/subprojects/FFmpeg/dist/lib \
  LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$GST_DIR/build/dist/lib:$SCRT_DIR/subprojects/FFmpeg/dist/lib \
  PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$GST_DIR/build/dist/lib/pkgconfig:$SCRT_DIR/subprojects/FFmpeg/dist/lib/pkgconfig \
  meson compile -C libav_build
  LIBRARY_PATH=$LIBRARY_PATH:$GST_DIR/build/dist/lib:$SCRT_DIR/subprojects/FFmpeg/dist/lib \
  LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$GST_DIR/build/dist/lib:$SCRT_DIR/subprojects/FFmpeg/dist/lib \
  PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$GST_DIR/build/dist/lib/pkgconfig:$SCRT_DIR/subprojects/FFmpeg/dist/lib/pkgconfig \
  meson install -C libav_build

  #Remove shared lib to force static resolution to .a
  rm -rf $GST_DIR/libav_build/dist/lib/*.so
  rm -rf $GST_DIR/libav_build/dist/lib/gstreamer-1.0/*.so
fi

if [ $ENABLE_RPI -eq 1 ]; then
  echo "Legacy RPi OMX Feature enabled..."
  # OMX is build sperately, because it depends on a build variable defined in the shared build.
  #   So we build both to get the static ones.
  rm -rf build_omx

  PKG_CONFIG_PATH=$GST_DIR/build/dist/lib/pkgconfig \
  meson setup build_omx \
    --buildtype=release \
    --strip \
    --default-library=both \
    -Dauto_features=disabled \
    -Domx=enabled \
    -Dgst-omx:header_path=/opt/vc/include/IL \
    -Dgst-omx:target=rpi \
    -Dgst-plugins-good:rpicamsrc=enabled \
    --prefix=$GST_DIR/build_omx/dist \
    --libdir=lib

  meson compile -C build_omx
  meson install -C build_omx


  #Remove shared lib to force static resolution to .a
  rm -rf $GST_DIR/build_omx/dist/lib/*.so
  rm -rf $GST_DIR/build_omx/dist/lib/gstreamer-1.0/*.so
fi

#Remove shared lib to force static resolution to .a
#We used the shared libs to recompile gst-omx plugins
rm -rf $GST_DIR/build/dist/lib/*.so
rm -rf $GST_DIR/build/dist/lib/gstreamer-1.0/*.so

################################################################
# 
#    Configure project
# 
################################################################
cd $WORK_DIR
$SCRT_DIR/configure $@