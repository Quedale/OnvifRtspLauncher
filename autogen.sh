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

# Customized build <4K file
meson build \
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
  -Dlibav=enabled \
  -Drtsp_server=enabled \
  -Dgst-plugins-base:app=enabled \
  -Dgst-plugins-base:typefind=enabled \
  -Dgst-plugins-base:volume=enabled \
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
  -Dgst-plugins-good:cairo=enabled \
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
  --pkg-config-path=/home/quedale/git/OnvifRtspLauncher/subprojects/gstreamer/build/dist/lib/x86_64-linux-gnu/pkgconfig \
  --prefix=$(pwd)/build/dist

meson compile -C build
meson install -C build


#Remove shared lib to force static resolution to .a
rm -rf $(pwd)/build/dist/lib/x86_64-linux-gnu/*.so
