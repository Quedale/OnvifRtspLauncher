#!/bin/bash
ENABLE_RPI=0
ENABLE_LIBAV=0
i=1;
for arg in "$@"
do
    if [ "$arg" == "--enable-libav" ]; then
        ENABLE_LIBAV=1
    elif [ "$arg" == "--enable-rpi" ]; then
        ENABLE_RPI=1
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

wget https://linuxtv.org/downloads/v4l-utils/v4l-utils-1.22.1.tar.bz2
tar xvfj v4l-utils-1.22.1.tar.bz2
rm v4l-utils-1.22.1.tar.bz2
cd v4l-utils-1.22.1
./bootstrap.sh
./configure --prefix=$(pwd)/dist --enable-static --disable-shared --with-udevdir=$(pwd)/dist/udev
make -j$(nproc)
make install 
cd ..

git -C gstreamer pull 2> /dev/null || git clone -b 1.21.3 https://gitlab.freedesktop.org/gstreamer/gstreamer.git
cd gstreamer
rm -rf build


GST_DIR=$(pwd)
# Add tinyalsa fallback subproject
echo "[wrap-git]" > subprojects/tinyalsa.wrap
echo "directory=tinyalsa" >> subprojects/tinyalsa.wrap
echo "url=https://github.com/tinyalsa/tinyalsa.git" >> subprojects/tinyalsa.wrap
echo "revision=v2.0.0" >> subprojects/tinyalsa.wrap

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
# Im not sure how useful libav really is, but fails to compile on rpi
if [ $ENABLE_LIBAV -eq 0 ]; then
  MESON_PARAMS="$MESON_PARAMS -Dlibav=enabled"
fi

if [ $ENABLE_RPI -eq 1 ]; then
  MESON_PARAMS="$MESON_PARAMS -DFFmpeg:omx=enabled"
fi

# Force disable subproject features
# MESON_PARAMS="$MESON_PARAMS -Dcairo:tests=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dfdk-aac:build-programs=disabled"
MESON_PARAMS="$MESON_PARAMS -Dglib:tests=false" #This one did shrink the build
# MESON_PARAMS="$MESON_PARAMS -Dglib:glib_debug=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dglib:glib_assert=false" # remove for stable build
# MESON_PARAMS="$MESON_PARAMS -Dglib:glib_checks=false" # remove for stable build
# MESON_PARAMS="$MESON_PARAMS -Dgst-devtools:tests=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dgst-devtools:docs=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dgst-devtools:tools=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dgst-editing-services:docs=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dgst-editing-services:examples=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dgst-editing-services:tests=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dgst-editing-services:tools=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:examples=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:tests=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:doc=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:glib-checks=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:glib-asserts=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:extra-checks=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:examples=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:tests=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:doc=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-base:tools=disabled"
# MESON_PARAMS="$MESON_PARAMS -DFFmpeg:ffmpeg=disabled"
# MESON_PARAMS="$MESON_PARAMS -DFFmpeg:ffplay=disabled"
# MESON_PARAMS="$MESON_PARAMS -DFFmpeg:ffprobe=disabled"
MESON_PARAMS="$MESON_PARAMS -Dlibdrm:cairo-tests=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dlibdrm:man-pages=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dopenh264:tests=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dpcre2-10:tests=false"
# MESON_PARAMS="$MESON_PARAMS -Dpixman:tests=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dtinyalsa:docs=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dtinyalsa:examples=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dtinyalsa:utils=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dtools=disabled"
# MESON_PARAMS="$MESON_PARAMS -Dexamples=disabled"
MESON_PARAMS="$MESON_PARAMS -Dx264:cli=false"

# Customized build <2K file
meson setup build \
  --buildtype=release \
  --strip \
  --default-library=static \
  --wrap-mode=forcefallback \
  -Dauto_features=disabled \
  $MESON_PARAMS \
  -Dbase=enabled \
  -Dgood=enabled \
  -Dbad=enabled \
  -Dugly=enabled \
  -Dgpl=enabled \
  -Drtsp_server=enabled \
  -Dgst-plugins-base:app=enabled \
  -Dgst-plugins-base:typefind=enabled \
  -Dgst-plugins-base:audiotestsrc=enabled \
  -Dgst-plugins-base:videotestsrc=enabled \
  -Dgst-plugins-base:playback=enabled \
  -Dgst-plugins-base:x11=enabled \
  -Dgst-plugins-base:xvideo=enabled \
  -Dgst-plugins-base:alsa=enabled \
  -Dgst-plugins-base:videoconvertscale=enabled \
  -Dgst-plugins-base:tcp=enabled \
  -Dgst-plugins-base:rawparse=enabled \
  -Dgst-plugins-base:pbtypes=enabled \
  -Dgst-plugins-base:overlaycomposition=enabled \
  -Dgst-plugins-base:audioresample=enabled \
  -Dgst-plugins-base:audioconvert=enabled \
  -Dgst-plugins-base:volume=enabled \
  -Dgst-plugins-good:v4l2=enabled \
  -Dgst-plugins-good:rtsp=enabled \
  -Dgst-plugins-good:rtp=enabled \
  -Dgst-plugins-good:rtpmanager=enabled \
  -Dgst-plugins-good:law=enabled \
  -Dgst-plugins-good:udp=enabled \
  -Dgst-plugins-good:autodetect=enabled \
  -Dgst-plugins-good:pulse=enabled \
  -Dgst-plugins-good:oss=enabled \
  -Dgst-plugins-good:oss4=enabled \
  -Dgst-plugins-good:interleave=enabled \
  -Dgst-plugins-good:audioparsers=enabled \
  -Dgst-plugins-bad:openh264=enabled \
  -Dgst-plugins-bad:nvcodec=enabled \
  -Dgst-plugins-bad:v4l2codecs=enabled \
  -Dgst-plugins-bad:fdkaac=enabled \
  -Dgst-plugins-bad:tinyalsa=enabled \
  -Dgst-plugins-bad:videoparsers=enabled \
  -Dgst-plugins-bad:switchbin=enabled \
  -Dgst-plugins-bad:onvif=enabled \
  -Dgst-plugins-bad:mpegtsmux=enabled \
  -Dgst-plugins-bad:mpegtsdemux=enabled \
  -Dgst-plugins-ugly:x264=enabled \
  --prefix=$GST_DIR/build/dist \
  --libdir=lib

meson compile -C build
meson install -C build


if [ $ENABLE_RPI -eq 1 ]; then
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

cd $WORK_DIR
$SCRT_DIR/configure $@