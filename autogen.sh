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


checkProg () {
    local name args path # reset first
    local "${@}"

    if !PATH=${path} command -v ${name} &> /dev/null
    then
        # echo "not found"
        return #Prog not found
    else
        PATH=${path} command ${name} ${args} &> /dev/null
        status=$?
        if [[ $status -eq 0 ]]; then
            echo "Working"
        else
            # echo "failed ${path}"
            return #Prog failed
        fi
    fi
}

mkdir -p subprojects
cd subprojects

################################################################
# 
#    Build gettext dependency
#   sudo apt install gettext (tested 1.16.3)
# 
################################################################
# PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SCRT_DIR/subprojects/v4l-utils/dist/lib/pkgconfig \
# pkg-config --exists --print-errors "libv4l2 >= 1.16.3"
# ret=$?
# if [ $ret != 0 ]; then 
PATH=$PATH:$SCRT_DIR/subprojects/gettext/dist/bin
if [ -z "$(checkProg name='gettext' args='--version' path=$PATH)" ]; then
  echo "not found gettext"

  PATH=$PATH:$SCRT_DIR/subprojects/fpc-3.2.2.x86_64-linux/dist/bin
  if [ -z "$(checkProg name='ppcx64' args='-iW' path=$PATH)" ] && [ -z "$(checkProg name='ppcx86' args='-iW' path=$PATH)" ]; then
    wget https://sourceforge.net/projects/freepascal/files/Linux/3.2.2/fpc-3.2.2.x86_64-linux.tar
    tar xf fpc-3.2.2.x86_64-linux.tar
    rm fpc-3.2.2.x86_64-linux.tar
    cd fpc-3.2.2.x86_64-linux
    echo $(pwd)/dist | ./install.sh
    cd ..
  else
    echo "fpc found"
  fi

  git -C gnulib pull 2> /dev/null || git clone https://git.savannah.gnu.org/git/gnulib.git
  git -C gettext pull 2> /dev/null || git clone -b v0.21.1 git://git.savannah.gnu.org/gettext.git 
  cd gettext
  GNULIB_SRCDIR=$(pwd)/../gnulib \
  ./autogen.sh
  make distclean
  ./configure --prefix=$(pwd)/dist/usr/local MAKEINFO=true
  make -j$(nproc)
  make install-exec

else
  echo "gettext already found."
fi

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
#    Build gudev-1.0 dependency
#   sudo apt install libgudev-1.0-dev (tested 232)
# 
################################################################
PKG_UDEV=$SCRT_DIR/subprojects/systemd/build/dist/usr/lib/pkgconfig
PKG_GUDEV=$SCRT_DIR/subprojects/libgudev/build/dist/lib/pkgconfig
PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_GUDEV:$PKG_UDEV \
pkg-config --exists --print-errors "gudev-1.0 >= 232"
ret=$?
if [ $ret != 0 ]; then 

  $(python3 -c "import jinja2")
  ret=$?
 if [ $ret != 0 ]; then
    $(python3 -c "import markupsafe")
    ret=$?
    if [ $ret != 0 ]; then
      echo "installing python3 markupsafe module."
      git -C markupsafe pull 2> /dev/null || git clone -b 2.1.1 https://github.com/pallets/markupsafe.git
      cd markupsafe
      python3 setup.py install --user
      cd ..
    else 
      echo "python3 markupsafe module already found."
    fi

    echo "installing python3 jinja2 module."
    git -C jinja pull 2> /dev/null || git clone -b 3.1.2 https://github.com/pallets/jinja.git
    cd jinja
    python3 setup.py install --user
    cd ..
  else
    echo "python3 jinja2 already found."
  fi

  PKG_LIBCAP=$SCRT_DIR/subprojects/libcap/dist/lib64/pkgconfig
  PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_LIBCAP \
  pkg-config --exists --print-errors "libcap >= 2.53" # Or check for sys/capability.h
  ret=$?
  if [ $ret != 0 ]; then 
    git -C libcap pull 2> /dev/null || git clone -b libcap-2.53  git://git.kernel.org/pub/scm/linux/kernel/git/morgan/libcap.git
    cd libcap
    make -j$(nproc)
    make install DESTDIR=$(pwd)/dist
    cd ..
  else
    echo "libcap already found."
  fi


  PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_UDEV \
  pkg-config --exists --print-errors "libudev >= 252" # Or check for sys/capability.h
  ret=$?
  if [ $ret != 0 ]; then 
    git -C systemd pull 2> /dev/null || git clone -b v252 https://github.com/systemd/systemd.git
    cd systemd
    rm -rf build
    PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_LIBCAP \
    C_INCLUDE_PATH=$SCRT_DIR/subprojects/libcap/dist/usr/include \
    meson setup build \
      --default-library=static \
      -Dauto_features=disabled \
      -Dmode=developer \
      -Dlink-udev-shared=false \
      -Dlink-systemctl-shared=false \
      -Dlink-networkd-shared=false \
      -Dlink-timesyncd-shared=false \
      -Dlink-journalctl-shared=false \
      -Dlink-boot-shared=false \
      -Dfirst-boot-full-preset=false \
      -Dstatic-libsystemd=false \
      -Dstatic-libudev=true \
      -Dstandalone-binaries=false \
      -Dinitrd=false \
      -Dcompat-mutable-uid-boundaries=false \
      -Dnscd=false \
      -Ddebug-shell='' \
      -Ddebug-tty='' \
      -Dutmp=false \
      -Dhibernate=false \
      -Dldconfig=false \
      -Dresolve=false \
      -Defi=false \
      -Dtpm=false \
      -Denvironment-d=false \
      -Dbinfmt=false \
      -Drepart=false \
      -Dsysupdate=false \
      -Dcoredump=false \
      -Dpstore=false \
      -Doomd=false \
      -Dlogind=false \
      -Dhostnamed=false \
      -Dlocaled=false \
      -Dmachined=false \
      -Dportabled=false \
      -Dsysext=false \
      -Duserdb=false \
      -Dhomed=false \
      -Dnetworkd=false \
      -Dtimedated=false \
      -Dtimesyncd=false \
      -Dremote=false \
      -Dcreate-log-dirs=false \
      -Dnss-myhostname=false \
      -Dnss-mymachines=false \
      -Dnss-resolve=false \
      -Dnss-systemd=false \
      -Drandomseed=false \
      -Dbacklight=false \
      -Dvconsole=false \
      -Dquotacheck=false \
      -Dsysusers=false \
      -Dtmpfiles=false \
      -Dimportd=false \
      -Dhwdb=false \
      -Drfkill=false \
      -Dxdg-autostart=false \
      -Dman=false \
      -Dhtml=false \
      -Dtranslations=false \
      -Dinstall-sysconfdir=false \
      -Dseccomp=false \
      -Dselinux=false \
      -Dapparmor=false \
      -Dsmack=false \
      -Dpolkit=false \
      -Dima=false \
      -Dacl=false \
      -Daudit=false \
      -Dblkid=false \
      -Dfdisk=false \
      -Dkmod=false \
      -Dpam=false \
      -Dpwquality=false \
      -Dmicrohttpd=false \
      -Dlibcryptsetup=false \
      -Dlibcurl=false \
      -Didn=false \
      -Dlibidn2=false \
      -Dlibidn=false \
      -Dlibiptc=false \
      -Dqrencode=false \
      -Dgcrypt=false \
      -Dgnutls=false \
      -Dopenssl=false \
      -Dcryptolib=auto \
      -Dp11kit=false \
      -Dlibfido2=false \
      -Dtpm2=false \
      -Delfutils=false \
      -Dzlib=false \
      -Dbzip2=false \
      -Dxz=false \
      -Dlz4=false \
      -Dzstd=false \
      -Dxkbcommon=false \
      -Dpcre2=false \
      -Dglib=false \
      -Ddbus=false \
      -Dgnu-efi=false \
      -Dtests=false \
      -Durlify=false \
      -Danalyze=false \
      -Dbpf-framework=false \
      -Dkernel-install=false \
      --libdir=lib

    PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_LIBCAP \
    C_INCLUDE_PATH=$SCRT_DIR/subprojects/libcap/dist/usr/include \
    LIBRARY_PATH=$SCRT_DIR/subprojects/libcap/dist/lib64 \
    meson compile -C build
    DESTDIR=$(pwd)/build/dist meson install -C build
    cd ..
  else
    echo "libudev already found."
  fi

  git -C libgudev pull 2> /dev/null || git clone -b 237 https://gitlab.gnome.org/GNOME/libgudev.git
  cd libgudev
  rm -rf build
  PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_UDEV \
  meson setup build \
    --default-library=static \
    -Dtests=disabled \
    -Dintrospection=disabled \
    -Dvapi=disabled \
    --prefix=$(pwd)/build/dist \
    --libdir=lib

  C_INCLUDE_PATH=$SCRT_DIR/subprojects/systemd/build/dist/usr/include \
  LIBRARY_PATH=$SCRT_DIR/subprojects/libcap/dist/lib64 \
  meson compile -C build
  meson install -C build
  cd ..

else
  echo "gudev-1.0 already found."
fi

################################################################
# 
#    Build alsa-lib dependency
#   sudo apt install llibasound2-dev (tested 1.2.7.2)
# 
################################################################
PKG_ALSA=$SCRT_DIR/subprojects/alsa-lib/build/dist/lib/pkgconfig
PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_ALSA \
pkg-config --exists --print-errors "alsa >= 1.2.7.2"
ret=$?
if [ $ret != 0 ]; then  
  git -C alsa-lib pull 2> /dev/null || git clone -b v1.2.8 git://git.alsa-project.org/alsa-lib.git
  cd alsa-lib
  autoreconf -vif
  ./configure --prefix=$(pwd)/build/dist --enable-static=yes --enable-shared=yes
  make -j$(nproc)
  make install
  cd ..
else
  echo "alsa-lib already found."
fi

################################################################
# 
#    Build libpulse dependency
#   sudo apt install libpulse-dev (tested 16.1)
# 
################################################################
PKG_PULSE=$SCRT_DIR/subprojects/pulseaudio/build/dist/lib/pkgconfig
PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_PULSE \
pkg-config --exists --print-errors "libpulse >= 16.1"
ret=$?
if [ $ret != 0 ]; then 
  PKG_LIBSNDFILE=$SCRT_DIR/subprojects/libsndfile/dist/lib/pkgconfig
  PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_LIBSNDFILE \
  pkg-config --exists --print-errors "sndfile >= 1.2.0"
  ret=$?
  if [ $ret != 0 ]; then 
    git -C libsndfile pull 2> /dev/null || git clone -b 1.2.0 https://github.com/libsndfile/libsndfile.git
    cd libsndfile
    # autoreconf -vif
    cmake --target clean
    cmake -G "Unix Makefiles" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$(pwd)/dist" \
        -DBUILD_EXAMPLES=off \
        -DBUILD_TESTING=off \
        -DBUILD_SHARED_LIBS=on \
        -DINSTALL_PKGCONFIG_MODULE=on \
        -DINSTALL_MANPAGES=off 
    make -j$(nproc)
    make install
    cd ..
  else
    echo "sndfile already found."
  fi

  PKG_CHECK=$SCRT_DIR/subprojects/check/build/dist/lib/pkgconfig
  PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_CHECK \
  pkg-config --exists --print-errors "check >= 0.15.2"
  ret=$?
  if [ $ret != 0 ]; then 
    git -C check pull 2> /dev/null || git clone -b 0.15.2 https://github.com/libcheck/check.git
    cd check
    autoreconf -vif
    ./configure --prefix=$(pwd)/build/dist --enable-static=yes --enable-shared=no --enable-build-docs=no --enable-timeout-tests=no
    make -j$(nproc)
    make install
    cd ..
  else
    echo "check already found."
  fi

  git -C pulseaudio pull 2> /dev/null || git clone -b v16.1 https://gitlab.freedesktop.org/pulseaudio/pulseaudio.git
  cd pulseaudio
  rm -rf build
  PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_LIBSNDFILE \
  PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_CHECK \
  meson setup build \
    --default-library=static \
    -Ddaemon=false \
    -Ddoxygen=false \
    -Dman=false \
    -Dtests=false \
    -Ddatabase=simple \
    -Dbashcompletiondir=no \
    -Dzshcompletiondir=no \
    --prefix=$(pwd)/build/dist \
    --libdir=lib

  meson compile -C build
  meson install -C build
  cd ..
else
  echo "libpulse already found."
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
PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_GUDEV:$PKG_ALSA:$PKG_PULSE:$PKG_GUDEV:$PKG_UDEV \
meson setup build \
  --buildtype=release \
  --strip \
  --default-library=static \
  --wrap-mode=forcefallback \
  -Dauto_features=disabled \
  $MESON_PARAMS \
  --prefix=$GST_DIR/build/dist \
  --libdir=lib

LIBRARY_PATH=$LIBRARY_PATH:$SCRT_DIR/subprojects/systemd/build/dist/usr/lib \
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
  
  LIBRARY_PATH=$SCRT_DIR/subprojects/systemd/build/dist/usr/lib \
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