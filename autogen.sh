ENABLE_RPI=0
i=1;
for arg in "$@" 
do
    if [ $arg == "--enable-rpi" ]; then
        ENABLE_RPI=1
    fi
    i=$((i + 1));
done

aclocal
autoconf
automake --add-missing

mkdir -p subprojects
cd subprojects

# wget https://linuxtv.org/downloads/v4l-utils/v4l-utils-1.22.1.tar.bz2
# tar xvfj v4l-utils-1.22.1.tar.bz2
# rm v4l-utils-1.22.1.tar.bz2
# cd v4l-utils-1.22.1
# ./configure --prefix=$(pwd)/dist --enable-static --disable-shared
# make -j$(nproc)
# make install 

git -C gstreamer pull 2> /dev/null || git clone -b 1.21.3 https://gitlab.freedesktop.org/gstreamer/gstreamer.git
cd gstreamer
rm -rf build


GST_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)
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

if [ $ENABLE_RPI -eq 0 ]; then
  LIBAV="-Dlibav=enabled"
fi

# Customized build ~3K file
meson setup build \
  --buildtype=release \
  --strip \
  --default-library=static \
  --wrap-mode=forcefallback \
  -Dauto_features=disabled \
  -Dbase=enabled \
  -Dgood=enabled \
  -Dbad=enabled \
  -Dugly=enabled \
  -Dgpl=enabled \
  $LIBAV \
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

./configure $@