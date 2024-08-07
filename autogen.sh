#!/bin/bash
SKIP=0
#Save current working directory to run configure in
WORK_DIR=$(pwd)

#Get project root directory based on autogen.sh file location
SCRT_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd)
SUBPROJECT_DIR=$SCRT_DIR/subprojects

#Cache folder for downloaded sources
SRC_CACHE_DIR=$SUBPROJECT_DIR/.cache

#Failure marker
FAILED=0

#Handle script arguments
ENABLE_RPI=0
ENABLE_LIBAV=0
ENABLE_CLIENT=0
ENABLE_LATEST=0
ENABLE_LIBCAM=0
i=1;
for arg in "$@"
do
  if [ "$arg" == "--enable-libav=yes" ] || [ "$arg" == "--enable-libav=true" ]; then
    ENABLE_LIBAV=1
  elif [ "$arg" == "--enable-rpi=yes" ] || [ "$arg" == "--enable-rpi=true" ]; then
    ENABLE_RPI=1
  elif [ "$arg" == "--enable-libcam=yes" ] || [ "$arg" == "--enable-libcam=true" ]; then
    ENABLE_LIBCAM=1
  elif [ "$arg" == "--enable-client=yes" ] || [ "$arg" == "--enable-client=true" ]; then
    ENABLE_CLIENT=1
  elif [ "$arg" == "----enable-latest" ]; then
    ENABLE_LATEST=1
  fi
  i=$((i + 1));
done

# Define color code constants
ORANGE='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

############################################
#
# Function to print time for human
#
############################################
function displaytime {
  local T=$1
  local D=$((T/60/60/24))
  local H=$((T/60/60%24))
  local M=$((T/60%60))
  local S=$((T%60))
  printf "${ORANGE}*****************************\n${NC}"
  printf "${ORANGE}*** "
  (( $D > 0 )) && printf '%d days ' $D
  (( $H > 0 )) && printf '%d hours ' $H
  (( $M > 0 )) && printf '%d minutes ' $M
  (( $D > 0 || $H > 0 || $M > 0 )) && printf 'and '
  printf "%d seconds\n${NC}" $S
  printf "${ORANGE}*****************************\n${NC}"
}

############################################
#
# Function to download and extract tar package file
# Can optionally use a caching folder defined with SRC_CACHE_DIR
#
############################################
downloadAndExtract (){
  local path file # reset first
  local "${@}"

  if [ $SKIP -eq 1 ]
  then
      printf "${ORANGE}*****************************\n${NC}"
      printf "${ORANGE}*** Skipping Download ${path} ***\n${NC}"
      printf "${ORANGE}*****************************\n${NC}"
      return
  fi

  dest_val=""
  if [ ! -z "$SRC_CACHE_DIR" ]; then
    dest_val="$SRC_CACHE_DIR/${file}"
  else
    dest_val=${file}
  fi

  if [ ! -f "$dest_val" ]; then
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** Downloading : ${path} ***\n${NC}"
    printf "${ORANGE}*** Destination : $dest_val ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
    wget ${path} -O $dest_val
  else
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** Source already downloaded : ${path} ***\n${NC}"
    printf "${ORANGE}*** Destination : $dest_val ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
  fi

  printf "${ORANGE}*****************************\n${NC}"
  printf "${ORANGE}*** Extracting : ${file} ***\n${NC}"
  printf "${ORANGE}*****************************\n${NC}"
  if [[ $dest_val == *.tar.gz ]]; then
    tar xfz $dest_val
  elif [[ $dest_val == *.tar.xz ]]; then
    tar xf $dest_val
  elif [[ $dest_val == *.tar.bz2 ]]; then
    tar xjf $dest_val
  else
    echo "ERROR FILE NOT FOUND ${path} // ${file}"
    FAILED=1
  fi
}

############################################
#
# Function to clone a git repository or pull if it already exists locally
# Can optionally use a caching folder defined with SRC_CACHE_DIR
#
############################################
pullOrClone (){
  local path tag depth recurse # reset first
  local "${@}"

  if [ $SKIP -eq 1 ]
  then
      printf "${ORANGE}*****************************\n${NC}"
      printf "${ORANGE}*** Skipping Pull/Clone ${tag}@${path} ***\n${NC}"
      printf "${ORANGE}*****************************\n${NC}"
      return
  fi

  recursestr=""
  if [ ! -z "${recurse}" ] 
  then
    recursestr="--recurse-submodules"
  fi
  depthstr=""
  if [ ! -z "${depth}" ] 
  then
    depthstr="--depth ${depth}"
  fi 

  tgstr=""
  tgstr2=""
  if [ ! -z "${tag}" ] 
  then
    tgstr="origin tags/${tag}"
    tgstr2="-b ${tag}"
  fi

  printf "${ORANGE}*****************************\n${NC}"
  printf "${ORANGE}*** Cloning ${tag}@${path} ***\n${NC}"
  printf "${ORANGE}*****************************\n${NC}"
  IFS='/' read -ra ADDR <<< "$path"
  namedotgit=${ADDR[-1]}
  IFS='.' read -ra ADDR <<< "$namedotgit"
  name=${ADDR[0]}

  dest_val=""
  if [ ! -z "$SRC_CACHE_DIR" ]; then
    dest_val="$SRC_CACHE_DIR/$name"
  else
    dest_val=$name
  fi
  if [ ! -z "${tag}" ]; then
    dest_val+="-${tag}"
  fi

  if [ -z "$SRC_CACHE_DIR" ]; then 
    #TODO Check if it's the right tag
    git -C $dest_val pull $tgstr 2> /dev/null || git clone -j$(nproc) $recursestr $depthstr $tgstr2 ${path} $dest_val
  elif [ -d "$dest_val" ] && [ ! -z "${tag}" ]; then #Folder exist, switch to tag
    currenttag=$(git -C $dest_val tag --points-at ${tag})
    echo "TODO Check current tag \"${tag}\" == \"$currenttag\""
    git -C $dest_val pull $tgstr 2> /dev/null || git clone -j$(nproc) $recursestr $depthstr $tgstr2 ${path} $dest_val
  elif [ -d "$dest_val" ] && [ -z "${tag}" ]; then #Folder exist, switch to main
    currenttag=$(git -C $dest_val tag --points-at ${tag})
    echo "TODO Handle no tag \"${tag}\" == \"$currenttag\""
    git -C $dest_val pull $tgstr 2> /dev/null || git clone -j$(nproc) $recursestr $depthstr $tgstr2 ${path} $dest_val
  elif [ ! -d "$dest_val" ]; then #fresh start
    git -C $dest_val pull $tgstr 2> /dev/null || git clone -j$(nproc) $recursestr $depthstr $tgstr2 ${path} $dest_val
  else
    if [ -d "$dest_val" ]; then
      echo "yey destval"
    fi
    if [ -z "${tag}" ]; then
      echo "yey tag"
    fi
    echo "1 $dest_val : $(test -f \"$dest_val\")"
    echo "2 ${tag} : $(test -z \"${tag}\")"
  fi

  if [ ! -z "$SRC_CACHE_DIR" ]; then
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** Copy repo from cache ***\n${NC}"
    printf "${ORANGE}*** $dest_val ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
    rm -rf $name
    cp -r $dest_val ./$name
  fi
}

############################################
#
# Function to build a project configured with autotools
#
############################################
buildMakeProject(){
  local srcdir prefix autogen autoreconf configure make cmakedir cmakeargs installargs
  local "${@}"

  build_start=$SECONDS
  if [ $SKIP -eq 1 ]; then
      printf "${ORANGE}*****************************\n${NC}"
      printf "${ORANGE}*** Skipping Make ${srcdir} ***\n${NC}"
      printf "${ORANGE}*****************************\n${NC}"
      return
  fi

  printf "${ORANGE}*****************************\n${NC}"
  printf "${ORANGE}* Building Project ***\n${NC}"
  printf "${ORANGE}* Src dir : ${srcdir} ***\n${NC}"
  printf "${ORANGE}* Prefix : ${prefix} ***\n${NC}"
  printf "${ORANGE}*****************************\n${NC}"

  curr_dir=$(pwd)
  cd ${srcdir}

  if [ -f "./bootstrap.sh" ]; then
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** bootstrap.sh ${srcdir} ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
    ./bootstrap.sh
    status=$?
    if [ $status -ne 0 ]; then
        printf "${RED}*****************************\n${NC}"
        printf "${RED}*** ./bootstrap.sh failed ${srcdir} ***\n${NC}"
        printf "${RED}*****************************\n${NC}"
        FAILED=1
        cd $curr_dir
        return
    fi
  fi
  if [ -f "./autogen.sh" ] && [ "${autogen}" != "skip" ]; then
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** autogen ${srcdir} ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
    ./autogen.sh ${autogen}
    status=$?
    if [ $status -ne 0 ]; then
        printf "${RED}*****************************\n${NC}"
        printf "${RED}*** Autogen failed ${srcdir} ***\n${NC}"
        printf "${RED}*****************************\n${NC}"
        FAILED=1
        cd $curr_dir
        return
    fi
  fi

  if [ ! -z "${autoreconf}" ] 
  then
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** autoreconf ${srcdir} ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
    autoreconf ${autoreconf}
    status=$?
    if [ $status -ne 0 ]; then
        printf "${RED}*****************************\n${NC}"
        printf "${RED}*** Autoreconf failed ${srcdir} ***\n${NC}"
        printf "${RED}*****************************\n${NC}"
        FAILED=1
        cd $curr_dir
        return
    fi
  fi
  if [ ! -z "${cmakedir}" ] 
  then
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** cmake ${srcdir} ***\n${NC}"
    printf "${ORANGE}*** Args ${cmakeargs} ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
    cmake -G "Unix Makefiles" \
      ${cmakeargs} \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX="${prefix}" \
      -DENABLE_TESTS=OFF \
      -DENABLE_SHARED=on \
      "${cmakedir}"
    status=$?
    if [ $status -ne 0 ]; then
      printf "${RED}*****************************\n${NC}"
      printf "${RED}*** CMake failed ${srcdir} ***\n${NC}"
      printf "${RED}*****************************\n${NC}"
    fi
  fi

  if [ -f "./configure" ]; then
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** configure ${srcdir} ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
    ./configure \
        --prefix=${prefix} \
        ${configure}
    status=$?
    if [ $status -ne 0 ]; then
      printf "${RED}*****************************\n${NC}"
      printf "${RED}*** ./configure failed ${srcdir} ***\n${NC}"
      printf "${RED}*****************************\n${NC}"
      echo "PATH?? $(pwd)"
      FAILED=1
      cd $curr_dir
      return
    fi
  else
    printf "${ORANGE}*****************************\n${NC}"
    printf "${ORANGE}*** no configuration available ${srcdir} ***\n${NC}"
    printf "${ORANGE}*****************************\n${NC}"
  fi

  printf "${ORANGE}*****************************\n${NC}"
  printf "${ORANGE}*** compile ${srcdir} ***\n${NC}"
  printf "${ORANGE}*** Make Args : ${makeargs} ***\n${NC}"
  printf "${ORANGE}*****************************\n${NC}"
  make -j$(nproc) ${make}
  status=$?
  if [ $status -ne 0 ]; then
      printf "${RED}*****************************\n${NC}"
      printf "${RED}*** Make failed ${srcdir} ***\n${NC}"
      printf "${RED}*****************************\n${NC}"
      FAILED=1
      cd $curr_dir
      return
  fi


  printf "${ORANGE}*****************************\n${NC}"
  printf "${ORANGE}*** install ${srcdir} ***\n${NC}"
  printf "${ORANGE}*** Make Args : ${makeargs} ***\n${NC}"
  printf "${ORANGE}*****************************\n${NC}"
  make -j$(nproc) ${make} install ${installargs}
  status=$?
  if [ $status -ne 0 ]; then
      printf "${RED}*****************************\n${NC}"
      printf "${RED}*** Make failed ${srcdir} ***\n${NC}"
      printf "${RED}*****************************\n${NC}"
      FAILED=1
      cd $curr_dir
      return
  fi

  build_time=$(( SECONDS - build_start ))
  displaytime $build_time

  cd $curr_dir
}


############################################
#
# Function to build a project configured with meson
#
############################################
buildMesonProject() {
  local srcdir mesonargs prefix setuppatch bindir destdir builddir defaultlib clean
  local "${@}"

  build_start=$SECONDS
  if [ $SKIP -eq 1 ]
  then
      printf "${ORANGE}*****************************\n${NC}"
      printf "${ORANGE}*** Skipping Meson ${srcdir} ***\n${NC}"
      printf "${ORANGE}*****************************\n${NC}"
      return
  fi

  printf "${ORANGE}*****************************\n${NC}"
  printf "${ORANGE}* Building Github Project ***\n${NC}"
  printf "${ORANGE}* Src dir : ${srcdir} ***\n${NC}"
  printf "${ORANGE}* Prefix : ${prefix} ***\n${NC}"
  printf "${ORANGE}*****************************\n${NC}"

  curr_dir=$(pwd)

  build_dir=""
  if [ ! -z "${builddir}" ] 
  then
    build_dir="${builddir}"
  else
    build_dir="build_dir"
  fi

  default_lib=""
  if [ ! -z "${defaultlib}" ] 
  then
    default_lib="${defaultlib}"
  else
    default_lib="static"
  fi

  bindir_val=""
  if [ ! -z "${bindir}" ]; then
      bindir_val="--bindir=${bindir}"
  fi
  
  if [ ! -z "${clean}" ]; then
    rm -rf ${srcdir}/$build_dir
  fi

  if [ ! -d "${srcdir}/$build_dir" ]; then
      mkdir -p ${srcdir}/$build_dir

      cd ${srcdir}
      if [ -d "./subprojects" ]; then
          printf "${ORANGE}*****************************\n${NC}"
          printf "${ORANGE}*** Download Subprojects ${srcdir} ***\n${NC}"
          printf "${ORANGE}*****************************\n${NC}"
          #     meson subprojects download
      fi

      echo "setup patch : ${setuppatch}"
      if [ ! -z "${setuppatch}" ]; then
          printf "${ORANGE}*****************************\n${NC}"
          printf "${ORANGE}*** Meson Setup Patch ${srcdir} ***\n${NC}"
          printf "${ORANGE}*** ${setuppatch} ***\n${NC}"
          printf "${ORANGE}*****************************\n${NC}"
          bash -c "${setuppatch}"
          status=$?
          if [ $status -ne 0 ]; then
              printf "${RED}*****************************\n${NC}"
              printf "${RED}*** Bash Setup failed ${srcdir} ***\n${NC}"
              printf "${RED}*****************************\n${NC}"
              FAILED=1
              cd $curr_dir
              return
          fi
      fi

      printf "${ORANGE}*****************************\n${NC}"
      printf "${ORANGE}*** Meson Setup ${srcdir} ***\n${NC}"
      printf "${ORANGE}*****************************\n${NC}"
      meson setup $build_dir \
          ${mesonargs} \
          --default-library=$default_lib \
          --prefix=${prefix} \
          $bindir_val \
          --libdir=lib \
          --includedir=include \
          --buildtype=release 
      status=$?
      if [ $status -ne 0 ]; then
          printf "${RED}*****************************\n${NC}"
          printf "${RED}*** Meson Setup failed ${srcdir} ***\n${NC}"
          printf "${RED}*****************************\n${NC}"
          FAILED=1
          cd $curr_dir
          return
      fi
  else
      cd ${srcdir}
      if [ -d "./subprojects" ]; then
          printf "${ORANGE}*****************************\n${NC}"
          printf "${ORANGE}*** Meson Update ${srcdir} ***\n${NC}"
          printf "${ORANGE}*****************************\n${NC}"
          #     meson subprojects update
      fi

      if [ ! -z "${setuppatch}" ]; then
          printf "${ORANGE}*****************************\n${NC}"
          printf "${ORANGE}*** Meson Setup Patch ${srcdir} ***\n${NC}"
          printf "${ORANGE}*** ${setuppatch} ***\n${NC}"
          printf "${ORANGE}*****************************\n${NC}"
          bash -c "${setuppatch}"
          status=$?
          if [ $status -ne 0 ]; then
              printf "${RED}*****************************\n${NC}"
              printf "${RED}*** Bash Setup failed ${srcdir} ***\n${NC}"
              printf "${RED}*****************************\n${NC}"
              FAILED=1
              cd $curr_dir
              return
          fi
      fi

      printf "${ORANGE}*****************************\n${NC}"
      printf "${ORANGE}*** Meson Reconfigure $(pwd) ${srcdir} ***\n${NC}"
      printf "${ORANGE}*****************************\n${NC}"
      meson setup $build_dir \
          ${mesonargs} \
          --default-library=$default_lib \
          --prefix=${prefix} \
          $bindir_val \
          --libdir=lib \
          --includedir=include \
          --buildtype=release \
          --reconfigure

      status=$?
      if [ $status -ne 0 ]; then
          printf "${RED}*****************************\n${NC}"
          printf "${RED}*** Meson Setup failed ${srcdir} ***\n${NC}"
          printf "${RED}*****************************\n${NC}"
          FAILED=1
          cd $curr_dir
          return
      fi
  fi

  printf "${ORANGE}*****************************\n${NC}"
  printf "${ORANGE}*** Meson Compile ${srcdir} ***\n${NC}"
  printf "${ORANGE}*****************************\n${NC}"
  meson compile -C $build_dir
  status=$?
  if [ $status -ne 0 ]; then
      printf "${RED}*****************************\n${NC}"
      printf "${RED}*** meson compile failed ${srcdir} ***\n${NC}"
      printf "${RED}*****************************\n${NC}"
      FAILED=1
      cd $curr_dir
      return
  fi

  printf "${ORANGE}*****************************\n${NC}"
  printf "${ORANGE}*** Meson Install ${srcdir} ***\n${NC}"
  printf "${ORANGE}*****************************\n${NC}"
  DESTDIR=${destdir} meson install -C $build_dir
  status=$?
  if [ $status -ne 0 ]; then
      printf "${RED}*****************************\n${NC}"
      printf "${RED}*** Meson Install failed ${srcdir} ***\n${NC}"
      printf "${RED}*****************************\n${NC}"
      FAILED=1
      cd $curr_dir
      return
  fi

  build_time=$(( SECONDS - build_start ))
  displaytime $build_time
  cd $curr_dir

}

############################################
#
# Function to check if a program exists
#
############################################
checkProg () {
  local name args path # reset first
  local "${@}"

  if !PATH=$PATH:${path} command -v ${name} &> /dev/null
  then
    return #Prog not found
  else
    PATH=$PATH:${path} command ${name} ${args} &> /dev/null
    status=$?
    if [[ $status -eq 0 ]]; then
        echo "Working"
    else
        return #Prog failed
    fi
  fi
}

checkCMake () {

  cmake --version
  ret=$?
  if [ $ret != 0 ]; then
    printf "not found cmake\n"
    python3 -m pip install cmake
    status=$?
    if [ $status -ne 0 ]; then
        printf "${RED}*****************************\n${NC}"
        printf "${RED}*** Failed to install cmake from pip ${srcdir} ***\n${NC}"
        printf "${RED}*****************************\n${NC}"
        exit 1
    fi
  else
    printf "cmake already found.\n"
  fi
}

isMesonInstalled(){
  local major minor micro # reset first
  local "${@}"

  ret=0
  MESONVERSION=$(meson --version)
  ret=$?
  if [[ $status -eq 0 ]]; then
    IFS='.' read -ra ADDR <<< "$MESONVERSION"
    index=0
    success=0
    for i in "${ADDR[@]}"; do
      if [ "$index" == "0" ]; then
        valuetocomapre="${major}" #Major version
      elif [ "$index" == "1" ]; then
        valuetocomapre="${minor}" #Minor version
      elif [ "$index" == "2" ]; then
        valuetocomapre="${micro}" #Micro version
      fi
      
      if [ "$i" -gt "$valuetocomapre" ]; then
        success=1
        break
      elif [ "$i" -lt "$valuetocomapre" ]; then
        success=0
        break
      fi

      if [ "$index" == "2" ]; then
        success=1
      fi
      ((index=index+1))
    done

    if [ $success -eq "1" ]; then
      return
    else
      echo "nope"
    fi

  else
    echo "nope"
  fi
}


# Hard dependency check
MISSING_DEP=0

if [ -z "$(checkProg name='automake' args='--version' path=$PATH)" ]; then
  MISSING_DEP=1
  echo "automake build utility not found! Aborting..."
fi

if [ -z "$(checkProg name='autoconf' args='--version' path=$PATH)" ]; then
  MISSING_DEP=1
  echo "autoconf build utility not found! Aborting..."
fi

if [ -z "$(checkProg name='libtoolize' args='--version' path=$PATH)" ]; then
  MISSING_DEP=1
  echo "libtool build utility not found! Aborting..."
fi

if [ -z "$(checkProg name='pkg-config' args='--version' path=$PATH)" ]; then
  MISSING_DEP=1
  echo "pkg-config build utility not found! Aborting..."
fi

if [ $MISSING_DEP -eq 1 ]; then
  exit 1
fi


#Change to the project folder to run autoconf and automake
cd $SCRT_DIR

#Initialize project
aclocal
autoconf
automake --add-missing
autoreconf -i

mkdir -p $SUBPROJECT_DIR
mkdir -p $SRC_CACHE_DIR

cd $SUBPROJECT_DIR

#Download virtualenv tool
if [ ! -f "virtualenv.pyz" ]; then
  wget https://bootstrap.pypa.io/virtualenv.pyz
fi

#Setting up Virtual Python environment
if [ ! -f "./venvfolder/bin/activate" ]; then
  python3 virtualenv.pyz ./venvfolder
fi

#Activate virtual environment
source venvfolder/bin/activate

#Setup meson
if [ ! -z "$(isMesonInstalled major=0 minor=63 micro=2)" ]; then
  echo "ninja build utility not found. Installing from pip..."
  python3 -m pip install meson --upgrade
else
  echo "Meson $(meson --version) already installed."
fi

if [ -z "$(checkProg name='ninja' args='--version' path=$PATH)" ]; then
  echo "ninja build utility not found. Installing from pip..."
  python3 -m pip install ninja --upgrade
else
  echo "Ninja $(ninja --version) already installed."
fi

FFMPEG_PKG=$SUBPROJECT_DIR/FFmpeg/dist/lib/pkgconfig
PKG_GLIB=$SUBPROJECT_DIR/glib-2.74.1/dist/lib/pkgconfig
PKG_UDEV=$SUBPROJECT_DIR/systemd-255/build/dist/lib/pkgconfig
PKG_GUDEV=$SUBPROJECT_DIR/libgudev/build/dist/lib/pkgconfig
PKG_XMACRO=$SUBPROJECT_DIR/macros/build/dist/pkgconfig
GST_OMX_PKG_PATH=$SUBPROJECT_DIR/gstreamer/build_omx/dist/lib/gstreamer-1.0/pkgconfig
PKG_XV=$SUBPROJECT_DIR/libxv/build/dist/lib/pkgconfig
PKG_LIBCAP=$SUBPROJECT_DIR/libcap/dist/lib64/pkgconfig
PKG_UTIL_LINUX=$SUBPROJECT_DIR/util-linux/dist/lib/pkgconfig
PKG_PULSE=$SUBPROJECT_DIR/pulseaudio/build/dist/lib/pkgconfig
PKG_LIBSNDFILE=$SUBPROJECT_DIR/libsndfile/dist/lib/pkgconfig
PKG_LIBV4L2=$SUBPROJECT_DIR/v4l-utils/dist/lib/pkgconfig
PKG_ALSA=$SUBPROJECT_DIR/alsa-lib/build/dist/lib/pkgconfig
LIBCAM_PKG=$SUBPROJECT_DIR/libcamera/build/dist/lib/pkgconfig
GST_LIBAV_PKG_PATH=$SUBPROJECT_DIR/gstreamer/libav_build/dist/lib/pkgconfig:$SUBPROJECT_DIR/gstreamer/libav_build/dist/lib/gstreamer-1.0/pkgconfig
GST_PKG_PATH=:$SUBPROJECT_DIR/gstreamer/build/dist/lib/pkgconfig:$SUBPROJECT_DIR/gstreamer/build/dist/lib/gstreamer-1.0/pkgconfig
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$PKG_PULSE:$LIBCAM_PKG:$PKG_ALSA:$PKG_LIBV4L2:$PKG_LIBSNDFILE:$PKG_UTIL_LINUX:$FFMPEG_PKG:$PKG_LIBCAP:$PKG_XV:$PKG_GLIB:$PKG_UDEV:$PKG_GUDEV:$GST_OMX_PKG_PATH:$GST_LIBAV_PKG_PATH:$GST_PKG_PATH:$PKG_XMACRO


################################################################
# 
#    Build v4l2-utils dependency
#   sudo apt install libv4l-dev (tested 1.16.3)
# 
################################################################
pkg-config --exists --print-errors "libv4l2 >= 1.16.3"
ret=$?
if [ $ret != 0 ]; then
  echo "not found libv4l2"
  pullOrClone path=https://git.linuxtv.org/v4l-utils.git tag=stable-1.26
  ACLOCAL_PATH=$SUBPROJECT_DIR/gettext-0.21.1/dist/share/aclocal \
  buildMesonProject srcdir="v4l-utils" prefix="$SUBPROJECT_DIR/v4l-utils/dist" mesonargs="-Dsystemdsystemunitdir=$SUBPROJECT_DIR/v4l-utils/dist/systemd -Dudevdir=$SUBPROJECT_DIR/v4l-utils/dist/udev"
  if [ $FAILED -eq 1 ]; then exit 1; fi
else
  echo "libv4l2 already found."
fi

################################################################
# 
#    Build alsa-lib dependency
#   sudo apt install libasound2-dev (tested 1.1.8)
# 
################################################################
pkg-config --exists --print-errors "alsa >= 1.1.8"
ret=$?
if [ $ret != 0 ]; then
  echo "not found alsa"
  pullOrClone path=git://git.alsa-project.org/alsa-lib.git tag=v1.2.8
  ACLOCAL_PATH=$SUBPROJECT_DIR/gettext-0.21.1/dist/share/aclocal \
  # buildMakeProject srcdir="alsa-lib" prefix="$SUBPROJECT_DIR/alsa-lib/build/dist" configure="--enable-pic --enable-static=yes --enable-shared=no" autoreconf="-vif"
  buildMakeProject srcdir="alsa-lib" prefix="$SUBPROJECT_DIR/alsa-lib/build/dist" configure="--enable-pic --enable-static=no --enable-shared=yes" autoreconf="-vif"
  if [ $FAILED -eq 1 ]; then exit 1; fi
else
  echo "alsa-lib already found."
fi

################################################################
# 
#    Build Gstreamer dependency
# 
################################################################
ret1=0
ret2=0
ret3=0
ret4=0
ret5=0
gst_version=1.24.5

pkg-config --exists --print-errors "gstreamer-1.0 >= 1.21.90"
ret1=$?
pkg-config --exists --print-errors "gstreamer-rtsp-server-1.0 >= 1.21.90"
ret2=$?
if [ $ENABLE_RPI -eq 1 ]; then
  pkg-config --exists --print-errors "gstrpicamsrc >= 1.14.4"
  ret3=$?
fi
if [ $ENABLE_LIBAV -eq 1 ]; then
  pkg-config --exists --print-errors "gstlibav >= 1.14.4"
  ret4=$?
fi

#Check to see if gstreamer exist on the system
if [ $ret1 != 0 ] || [ $ret2 != 0 ] || [ $ret3 != 0 ] || [ $ret4 != 0 ] || [ $ENABLE_LATEST != 0 ]; then

  pkg-config --exists --print-errors "gstreamer-1.0 >= $gst_version"
  ret1=$?
  pkg-config --exists --print-errors "gstreamer-rtsp-server-1.0 >= $gst_version"
  ret2=$?
  if [ $ENABLE_RPI -eq 1 ]; then
    pkg-config --exists --print-errors "gstrpicamsrc >= $gst_version"
    ret3=$?
  fi
  if [ $ENABLE_LIBAV -eq 1 ]; then
    pkg-config --exists  --print-errors "gstlibav >= $gst_version"
    ret4=$?
  fi
  #Check if the feature was previously built
  if [ $ENABLE_CLIENT -eq 1 ]; then
    pkg-config --exists --print-errors "gstximagesink >= $gst_version"
    ret5=$?

    if [ $ret5 != 0 ]; then


      AC_XMACRO=$SUBPROJECT_DIR/macros/build/dist/aclocal
      pkg-config --exists --print-errors "xorg-macros >= 1.19.1"
      xvret=$?
      if [ $xvret != 0 ]; then
        echo "not found xorg-macros"
        pullOrClone path="https://gitlab.freedesktop.org/xorg/util/macros.git" tag="util-macros-1.19.1"
        if [ $FAILED -eq 1 ]; then exit 1; fi
        buildMakeProject srcdir="macros" prefix="$SUBPROJECT_DIR/macros/build/dist" configure="--datarootdir=$SUBPROJECT_DIR/macros/build/dist"
        if [ $FAILED -eq 1 ]; then exit 1; fi
      else
        echo "xorg-macros already found"
      fi

      pkg-config --exists --print-errors "xv >= 1.0.12"
      xvret=$?
      if [ $xvret != 0 ]; then
        echo "not found xv"
        pullOrClone path="https://gitlab.freedesktop.org/xorg/lib/libxv.git" tag=libXv-1.0.12
        if [ $FAILED -eq 1 ]; then exit 1; fi
        ACLOCAL_PATH=$AC_XMACRO \
        buildMakeProject srcdir="libxv" prefix="$SUBPROJECT_DIR/libxv/build/dist"
        if [ $FAILED -eq 1 ]; then exit 1; fi
      else
        echo "libxv already found"
      fi
    fi

  fi

  #Global check if gstreamer is already built
  if [ $ret1 != 0 ] || [ $ret2 != 0 ] || [ $ret3 != 0 ] || [ $ret4 != 0 ] || [ $ret5 != 0 ]; then
    ################################################################
    # 
    #    Build gettext and libgettext dependency
    #   sudo apt install gettext libgettext-dev (tested 1.16.3)
    # 
    ################################################################
    PATH=$PATH:$SUBPROJECT_DIR/gettext-0.21.1/dist/bin
    if [ -z "$(checkProg name='gettextize' args='--version' path=$PATH)" ] || [ -z "$(checkProg name='autopoint' args='--version' path=$PATH)" ]; then
      echo "not found gettext"
      downloadAndExtract file="gettext-0.21.1.tar.gz" path="https://ftp.gnu.org/pub/gnu/gettext/gettext-0.21.1.tar.gz"
      if [ $FAILED -eq 1 ]; then exit 1; fi
      buildMakeProject srcdir="gettext-0.21.1" prefix="$SUBPROJECT_DIR/gettext-0.21.1/dist" autogen="skip"
      if [ $FAILED -eq 1 ]; then exit 1; fi
    else
      echo "gettext already found."
    fi

    ################################################################
    # 
    #     Build glib dependency
    #   sudo apt-get install libglib2.0-dev (gstreamer minimum 2.64.0)
    # 
    ################################################################
    pkg-config --exists --print-errors "glib-2.0 >= 2.64.0"
    ret=$?
    if [ $ret != 0 ]; then 
      echo "not found glib-2.0"
      downloadAndExtract file="glib-2.74.1.tar.xz" path="https://download.gnome.org/sources/glib/2.74/glib-2.74.1.tar.xz"
      if [ $FAILED -eq 1 ]; then exit 1; fi
      buildMesonProject srcdir="glib-2.74.1" prefix="$SUBPROJECT_DIR/glib-2.74.1/dist" mesonargs="-Dpcre2:test=false -Dpcre2:grep=false -Dxattr=false -Db_lundef=false -Dtests=false -Dglib_debug=disabled -Dglib_assert=false -Dglib_checks=false"
      if [ $FAILED -eq 1 ]; then exit 1; fi
    else
      echo "glib already found."
    fi

    ################################################################
    # 
    #    Build gudev-1.0 dependency
    #   sudo apt install libgudev-1.0-dev (tested 232)
    # 
    ################################################################
    pkg-config --exists --print-errors "gudev-1.0 >= 232"
    ret=$?
    if [ $ret != 0 ]; then 
      echo "not found gudev-1.0"
      if [ -z "$(checkProg name='gperf' args='--version' path=$SUBPROJECT_DIR/gperf-3.1/dist/bin)" ]; then
        echo "not found gperf"
        downloadAndExtract file="gperf-3.1.tar.gz" path="http://ftp.gnu.org/pub/gnu/gperf/gperf-3.1.tar.gz"
        if [ $FAILED -eq 1 ]; then exit 1; fi
        buildMakeProject srcdir="gperf-3.1" prefix="$SUBPROJECT_DIR/gperf-3.1/dist" autogen="skip"
        if [ $FAILED -eq 1 ]; then exit 1; fi
      else
        echo "gperf already found."
      fi

      pkg-config --exists --print-errors "libcap >= 2.53" # Or check for sys/capability.h
      ret=$?
      if [ $ret != 0 ]; then
        echo "not found libcap"
        pullOrClone path="https://git.kernel.org/pub/scm/libs/libcap/libcap.git" tag=libcap-2.53
        buildMakeProject srcdir="libcap" prefix="$SUBPROJECT_DIR/libcap/dist" installargs="DESTDIR=$SUBPROJECT_DIR/libcap/dist"
        if [ $FAILED -eq 1 ]; then exit 1; fi
      else
        echo "libcap already found."
      fi

      pkg-config --exists --print-errors "mount >= 2.38.0"
      ret=$?
      if [ $ret != 0 ]; then
        echo "not found mount"
        pullOrClone path=https://github.com/util-linux/util-linux.git tag=v2.38.1
        buildMakeProject srcdir="util-linux" prefix="$SUBPROJECT_DIR/util-linux/dist" configure="--disable-rpath --disable-bash-completion --disable-makeinstall-setuid --disable-makeinstall-chown"
        if [ $FAILED -eq 1 ]; then exit 1; fi
      else
        echo "mount already found."
      fi

      python3 -c 'import jinja2'
      ret=$?
      if [ $ret != 0 ]; then
        python3 -m pip install jinja2
      else
        echo "python3 jinja2 already found."
      fi

      pkg-config --exists --print-errors "libudev >= 255" # Or check for sys/capability.h
      ret=$?
      if [ $ret != 0 ]; then
        echo "not found libudev"
        SYSD_MESON_ARGS="-Dauto_features=disabled"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dmode=developer"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlink-udev-shared=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlink-systemctl-shared=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlink-networkd-shared=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlink-timesyncd-shared=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlink-journalctl-shared=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlink-boot-shared=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dfirst-boot-full-preset=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dstatic-libsystemd=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dstatic-libudev=true"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dstandalone-binaries=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dinitrd=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dcompat-mutable-uid-boundaries=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dnscd=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Ddebug-shell=''"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Ddebug-tty=''"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dutmp=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dhibernate=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dldconfig=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dresolve=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Defi=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dtpm=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Denvironment-d=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dbinfmt=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Drepart=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dsysupdate=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dcoredump=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dpstore=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Doomd=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlogind=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dhostnamed=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlocaled=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dmachined=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dportabled=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dsysext=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Duserdb=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dhomed=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dnetworkd=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dtimedated=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dtimesyncd=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dremote=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dcreate-log-dirs=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dnss-myhostname=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dnss-mymachines=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dnss-resolve=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dnss-systemd=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Drandomseed=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dbacklight=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dvconsole=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dquotacheck=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dsysusers=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dtmpfiles=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dimportd=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dhwdb=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Drfkill=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dxdg-autostart=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dman=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dhtml=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dtranslations=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dinstall-sysconfdir=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dseccomp=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dselinux=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dapparmor=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dsmack=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dpolkit=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dima=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dacl=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Daudit=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dblkid=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dfdisk=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dkmod=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dpam=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dpwquality=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dmicrohttpd=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlibcryptsetup=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlibcurl=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Didn=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlibidn2=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlibidn=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlibiptc=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dqrencode=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dgcrypt=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dgnutls=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dopenssl=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dcryptolib=auto"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dp11kit=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlibfido2=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dtpm2=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Delfutils=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dzlib=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dbzip2=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dxz=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlz4=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dzstd=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dxkbcommon=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dpcre2=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dglib=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Ddbus=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dtests=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Durlify=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Danalyze=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dbpf-framework=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dkernel-install=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dsysvinit-path=$SUBPROJECT_DIR/systemd-255/build/dist/init.d"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dbashcompletiondir=$SUBPROJECT_DIR/systemd-255/build/dist/bash-completion"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dcreate-log-dirs=false"
        SYSD_MESON_ARGS="$SYSD_MESON_ARGS -Dlocalstatedir=$SUBPROJECT_DIR/systemd-255/build/dist/localstate"
        downloadAndExtract file="v255.tar.gz" path="https://github.com/systemd/systemd/archive/refs/tags/v255.tar.gz"
        if [ $FAILED -eq 1 ]; then exit 1; fi
        PATH=$PATH:$SUBPROJECT_DIR/gperf-3.1/dist/bin \
        C_INCLUDE_PATH=$SUBPROJECT_DIR/libcap/dist/usr/include \
        LIBRARY_PATH=$SUBPROJECT_DIR/libcap/dist/lib64 \
        buildMesonProject srcdir="systemd-255" prefix="$SUBPROJECT_DIR/systemd-255/build/dist" mesonargs="$SYSD_MESON_ARGS"
        if [ $FAILED -eq 1 ]; then exit 1; fi

      else
        echo "libudev already found."
      fi

      pullOrClone path=https://gitlab.gnome.org/GNOME/libgudev.git tag=237
      C_INCLUDE_PATH=$SUBPROJECT_DIR/systemd-255/dist/usr/local/include \
      LIBRARY_PATH=$SUBPROJECT_DIR/libcap/dist/lib64:$SUBPROJECT_DIR/systemd-255/dist/usr/lib \
      PATH=$PATH:$SUBPROJECT_DIR/glib-2.74.1/dist/bin \
      buildMesonProject srcdir="libgudev" prefix="$SUBPROJECT_DIR/libgudev/build/dist" mesonargs="-Dvapi=disabled -Dtests=disabled -Dintrospection=disabled"
      if [ $FAILED -eq 1 ]; then exit 1; fi

    else
      echo "gudev-1.0 already found."
    fi

    ################################################################
    # 
    #    Build libpulse dependency
    #   sudo apt install libpulse-dev (tested 12.2)
    # 
    ################################################################
    pkg-config --exists --print-errors "libpulse >= 12.2"
    ret=$?
    if [ $ret != 0 ]; then 
      echo "not found libpulse"

      checkCMake

      pkg-config --exists --print-errors "sndfile >= 1.2.0"
      ret=$?
      if [ $ret != 0 ]; then
        echo "not found sndfile"
        LIBSNDFILE_CMAKEARGS="-DBUILD_EXAMPLES=off"
        LIBSNDFILE_CMAKEARGS="$LIBSNDFILE_CMAKEARGS -DBUILD_TESTING=off"
        LIBSNDFILE_CMAKEARGS="$LIBSNDFILE_CMAKEARGS -DBUILD_SHARED_LIBS=on"
        LIBSNDFILE_CMAKEARGS="$LIBSNDFILE_CMAKEARGS -DINSTALL_PKGCONFIG_MODULE=on"
        LIBSNDFILE_CMAKEARGS="$LIBSNDFILE_CMAKEARGS -DINSTALL_MANPAGES=off"
        pullOrClone path="https://github.com/libsndfile/libsndfile.git" tag=1.2.0
        mkdir "libsndfile/build"
        buildMakeProject srcdir="libsndfile/build" prefix="$SUBPROJECT_DIR/libsndfile/dist" cmakedir=".." cmakeargs="$LIBSNDFILE_CMAKEARGS"
        if [ $FAILED -eq 1 ]; then exit 1; fi
      else
        echo "sndfile already found."
      fi


      PULSE_MESON_ARGS="-Ddaemon=false"
      PULSE_MESON_ARGS="$PULSE_MESON_ARGS -Ddoxygen=false"
      PULSE_MESON_ARGS="$PULSE_MESON_ARGS -Dman=false"
      PULSE_MESON_ARGS="$PULSE_MESON_ARGS -Dtests=false"
      PULSE_MESON_ARGS="$PULSE_MESON_ARGS -Ddatabase=simple"
      PULSE_MESON_ARGS="$PULSE_MESON_ARGS -Dbashcompletiondir=no"
      PULSE_MESON_ARGS="$PULSE_MESON_ARGS -Dzshcompletiondir=no"
      pullOrClone path="https://gitlab.freedesktop.org/pulseaudio/pulseaudio.git" tag=v16.1
      buildMesonProject srcdir="pulseaudio" prefix="$SUBPROJECT_DIR/pulseaudio/build/dist" mesonargs="$PULSE_MESON_ARGS"
      if [ $FAILED -eq 1 ]; then exit 1; fi

    else
      echo "libpulse already found."
    fi

    ################################################################
    # 
    #    Build nasm dependency
    #   sudo apt install nasm
    # 
    ################################################################
    NASM_BIN=$SUBPROJECT_DIR/nasm-2.15.05/dist/bin
    if [ -z "$(checkProg name='nasm' args='--version' path=$NASM_BIN)" ]; then
      echo "not found nasm"
      downloadAndExtract file=nasm-2.15.05.tar.bz2 path=https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/nasm-2.15.05.tar.bz2
      buildMakeProject srcdir="nasm-2.15.05" prefix="$SUBPROJECT_DIR/nasm-2.15.05/dist"
      if [ $FAILED -eq 1 ]; then exit 1; fi
    else
        echo "nasm already installed."
    fi

    #Core Gstreamer
    if [ $ret1 != 0 ] || [ $ret2 != 0 ] || [ $ret5 != 0 ]; then
      pullOrClone path="https://gitlab.freedesktop.org/gstreamer/gstreamer.git" tag=$gst_version
      MESON_PARAMS=""

      # Force disable subproject features
      MESON_PARAMS="$MESON_PARAMS -Dglib:tests=false"
      MESON_PARAMS="$MESON_PARAMS -Dlibdrm:cairo-tests=false"
      MESON_PARAMS="$MESON_PARAMS -Dx264:cli=false"

      # Gstreamer options
      MESON_PARAMS="$MESON_PARAMS -Dbase=enabled"
      MESON_PARAMS="$MESON_PARAMS -Dgood=enabled"
      MESON_PARAMS="$MESON_PARAMS -Dbad=enabled"
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
      MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:jpeg=enabled"
      MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:openh264=enabled"
      MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:nvcodec=enabled"
      MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:v4l2codecs=enabled"
      MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:fdkaac=enabled"
      MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:videoparsers=enabled"
      MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:onvif=enabled"
      MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:jpegformat=enabled"
      MESON_PARAMS="$MESON_PARAMS -Dugly=enabled"
      MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-ugly:x264=enabled"
      # MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:x265=enabled"

      #Below is required for to workaround https://gitlab.freedesktop.org/gstreamer/gstreamer/-/issues/1056
      # This is to support v4l2h264enc element with capssetter
      MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:debugutils=enabled"

      # This is required for the snapshot feature
      MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:png=enabled"

      MESON_PARAMS="-Dauto_features=disabled $MESON_PARAMS"
      MESON_PARAMS="--strip $MESON_PARAMS"

      # Add tinyalsa fallback subproject
      # echo "[wrap-git]" > subprojects/tinyalsa.wrap
      # echo "directory=tinyalsa" >> subprojects/tinyalsa.wrap
      # echo "url=https://github.com/tinyalsa/tinyalsa.git" >> subprojects/tinyalsa.wrap
      # echo "revision=v2.0.0" >> subprojects/tinyalsa.wrap
      # MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-bad:tinyalsa=enabled"

      LIBRARY_PATH=$LD_LIBRARY_PATH:$SUBPROJECT_DIR/systemd-255/dist/usr/lib \
      PATH=$PATH:$SUBPROJECT_DIR/glib-2.74.1/dist/bin:$NASM_BIN \
      buildMesonProject srcdir="gstreamer" prefix="$SUBPROJECT_DIR/gstreamer/build/dist" mesonargs="$MESON_PARAMS" builddir="build"
      if [ $FAILED -eq 1 ]; then exit 1; fi
    else
      echo "Core latest gstreamer already built"
    fi

    if [ $ENABLE_LIBAV -eq 1 ] && [ $ret4 != 0 ]; then
      echo "LIBAV Feature enabled..."

      FFMPEG_BIN=$SUBPROJECT_DIR/FFmpeg/dist/bin
      pkg-config --exists --print-errors "libavcodec >= 58.20.100"
      ret1=$?
      pkg-config --exists --print-errors "libavfilter >= 7.40.101"
      ret2=$?
      pkg-config --exists --print-errors "libavformat >= 58.20.100"
      ret3=$?
      pkg-config --exists --print-errors "libavutil >= 56.22.100"
      ret4=$?
      pkg-config --exists --print-errors "libpostproc >= 55.3.100"
      ret5=$?
      pkg-config --exists --print-errors "libswresample >= 3.3.100"
      ret6=$?
      pkg-config --exists --print-errors "libswscale >= 5.3.100"
      ret7=$?
      if [ $ret1 != 0 ] || [ $ret2 != 0 ] || [ $ret3 != 0 ] || [ $ret4 != 0 ] || [ $ret5 != 0 ] || [ $ret6 != 0 ] || [ $ret7 != 0 ]; then
        #######################
        #
        # Custom FFmpeg build
        #   For some reason, Gstreamer's meson dep doesn't build any codecs
        #   
        #   sudo apt install libavcodec-dev libavfilter-dev libavformat-dev libswresample-dev
        #######################

        FFMPEG_CONFIGURE_ARGS="--disable-lzma"
        FFMPEG_CONFIGURE_ARGS="$FFMPEG_CONFIGURE_ARGS --disable-doc"
        FFMPEG_CONFIGURE_ARGS="$FFMPEG_CONFIGURE_ARGS --disable-shared"
        FFMPEG_CONFIGURE_ARGS="$FFMPEG_CONFIGURE_ARGS --enable-static"
        FFMPEG_CONFIGURE_ARGS="$FFMPEG_CONFIGURE_ARGS --enable-nonfree"
        FFMPEG_CONFIGURE_ARGS="$FFMPEG_CONFIGURE_ARGS --enable-version3"
        FFMPEG_CONFIGURE_ARGS="$FFMPEG_CONFIGURE_ARGS --enable-gpl"


        pullOrClone path="https://github.com/FFmpeg/FFmpeg.git" tag=n5.0.2
        PATH=$PATH:$NASM_BIN \
        buildMakeProject srcdir="FFmpeg" prefix="$SUBPROJECT_DIR/FFmpeg/dist" configure="$FFMPEG_CONFIGURE_ARGS"
        if [ $FAILED -eq 1 ]; then exit 1; fi
        rm -rf $SUBPROJECT_DIR/FFmpeg/dist/lib/*.so
      else
          echo "FFmpeg already installed."
      fi

      MESON_PARAMS="-Dlibav=enabled"
      MESON_PARAMS="$MESON_PARAMS --strip"
      MESON_PARAMS="$MESON_PARAMS --wrap-mode=nofallback"
      MESON_PARAMS="$MESON_PARAMS -Dauto_features=disabled"
      MESON_PARAMS="-Dauto_features=disabled $MESON_PARAMS"

      LIBRARY_PATH=$LIBRARY_PATH:$SUBPROJECT_DIR/gstreamer/build/dist/lib:$SUBPROJECT_DIR/FFmpeg/dist/lib \
      LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$SUBPROJECT_DIR/gstreamer/build/dist/lib:$SUBPROJECT_DIR/FFmpeg/dist/lib \
      PATH=$PATH:$SUBPROJECT_DIR/glib-2.74.1/dist/bin:$NASM_BIN \
      PKG_CONFIG_PATH=$GST_PKG_PATH:$PKG_CONFIG_PATH  \
      buildMesonProject srcdir="gstreamer" prefix="$SUBPROJECT_DIR/gstreamer/libav_build/dist" mesonargs="$MESON_PARAMS" builddir="libav_build"
      if [ $FAILED -eq 1 ]; then exit 1; fi

      #Remove shared lib to force static resolution to .a
      rm -rf $SUBPROJECT_DIR/gstreamer/libav_build/dist/lib/*.so
      rm -rf $SUBPROJECT_DIR/gstreamer/libav_build/dist/lib/gstreamer-1.0/*.so
    else
        echo "Gstreamer libav already installed/built."
    fi

    if [ $ENABLE_RPI -eq 1 ] && [ $ret3 != 0 ]; then
      echo "Legacy RPi OMX Feature enabled..."
      # OMX is build sperately, because it depends on a build variable defined in the shared build.
      #   So we build both to get the static ones.
      MESON_PARAMS="--strip"
      MESON_PARAMS="$MESON_PARAMS -Dauto_features=disabled"
      MESON_PARAMS="$MESON_PARAMS -Domx=enabled"
      MESON_PARAMS="$MESON_PARAMS -Dgst-omx:header_path=/opt/vc/include/IL"
      MESON_PARAMS="$MESON_PARAMS -Dgst-omx:target=rpi"
      MESON_PARAMS="$MESON_PARAMS -Dgst-plugins-good:rpicamsrc=enabled"
      MESON_PARAMS="-Dauto_features=disabled $MESON_PARAMS"
      
      LIBRARY_PATH=$LIBRARY_PATH:$SUBPROJECT_DIR/gstreamer/build/dist/lib:$SUBPROJECT_DIR/FFmpeg/dist/lib \
      LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$SUBPROJECT_DIR/gstreamer/build/dist/lib:$SUBPROJECT_DIR/FFmpeg/dist/lib \
      PATH=$PATH:$SUBPROJECT_DIR/glib-2.74.1/dist/bin:$NASM_BIN \
      PKG_CONFIG_PATH=$GST_PKG_PATH:$PKG_CONFIG_PATH  \
      buildMesonProject srcdir="gstreamer" prefix="$SUBPROJECT_DIR/gstreamer/build_omx/dist" mesonargs="$MESON_PARAMS" builddir="build_omx" defaultlib=both
      if [ $FAILED -eq 1 ]; then exit 1; fi

      #Remove shared lib to force static resolution to .a
      rm -rf $SUBPROJECT_DIR/gstreamer/build_omx/dist/lib/*.so
      rm -rf $SUBPROJECT_DIR/gstreamer/build_omx/dist/lib/gstreamer-1.0/*.so
    else
        echo "Gstreamer omx already installed/built."
    fi

    #Remove shared lib to force static resolution to .a
    #We used the shared libs to recompile gst-omx plugins
    rm -rf $SUBPROJECT_DIR/gstreamer/build/dist/lib/*.so
    rm -rf $SUBPROJECT_DIR/gstreamer/build/dist/lib/gstreamer-1.0/*.so

  else
      echo "Latest Gstreamer already installed/built."
  fi
else
    echo "Gstreamer already installed."
fi

pkg-config --exists --print-errors "libcamera >= 0.1.0"
ret1=$?
if [ $ENABLE_LIBCAM -eq 1 ] && [ $ret1 != 0 ]; then

  python3 -c 'import jinja2'
  ret=$?
  if [ $ret != 0 ]; then
    python3 -m pip install jinja2
  else
    echo "python3 jinja2 already found."
  fi

  python3 -c 'import ply'
  ret=$?
  if [ $ret != 0 ]; then
    python3 -m pip install ply
  else
    echo "python3 ply already found."
  fi

  python3 -c 'import yaml'
  ret=$?
  if [ $ret != 0 ]; then
    python3 -m pip install pyyaml
  else
    echo "python3 pyyaml already found."
  fi
  
  checkCMake

  LIBCAM_PARAMS="-Dcam=disabled"
  LIBCAM_PARAMS="$LIBCAM_PARAMS -Ddocumentation=disabled"
  LIBCAM_PARAMS="$LIBCAM_PARAMS -Dgstreamer=enabled"
  LIBCAM_PARAMS="$LIBCAM_PARAMS -Dlc-compliance=disabled"
  LIBCAM_PARAMS="$LIBCAM_PARAMS -Dtracing=disabled"
  LIBCAM_PARAMS="$LIBCAM_PARAMS -Dv4l2=true"

  pullOrClone path="https://git.libcamera.org/libcamera/libcamera.git" tag=v0.1.0
  if [ $FAILED -eq 1 ]; then exit 1; fi
  #libcamera is hardcoded shared_library for some reason. Change to allow static build
  grep -rl shared_library ./libcamera | xargs sed -i 's/shared_library/library/g'
  buildMesonProject srcdir="libcamera" prefix="$SUBPROJECT_DIR/libcamera/build/dist" mesonargs="$LIBCAM_PARAMS"
  if [ $FAILED -eq 1 ]; then exit 1; fi

fi

################################################################
# 
#    Configure project
# 
################################################################
cd $WORK_DIR
$SCRT_DIR/configure $@