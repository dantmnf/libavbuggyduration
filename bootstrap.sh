#!/bin/bash

FFMPEG_REPOSITORY="git://source.ffmpeg.org/ffmpeg.git"
FFMPEG_CONFIGURE_OPTIONS="--enable-gpl --enable-version3 --disable-hwaccels --disable-decoders --disable-encoders \
                          --enable-encoder=flac,alac,mjpeg,png --disable-decoder=h263,hevc,vc1 \
                          --enable-decoder=h264,mpeg2video,aac,mp3,mp2,vorbis,opus,flac,alac \
                          --disable-filters --disable-devices --enable-shared --enable-ffmpeg --enable-ffprobe"
pushd $PWD
error() {
  popd
  exit $1
}

mkdir -p bootstrap
cd bootstrap

echo 'getting FFmpeg...'
if [ -d ffmpeg ]
then
  cd ffmpeg
  git pull
  cd ..
else
  git clone "$FFMPEG_REPOSITORY" ffmpeg
fi

echo 'configuring FFmpeg...'
ffmpeg/configure --prefix="$PWD/prefix" $FFMPEG_CONFIGURE_OPTIONS || exit $!

echo 'building FFmpeg...'
cd ffmpeg
make || error $!
make install || error $!
cd ..

echo 'building avbuggyduration...'
cd ..
export CFLAGS="$CFLAGS -I${PWD}/bootstrap/prefix/include"
export LDFLAGS="$LDFLAGS -L${PWD}/bootstrap/prefix/lib"
make || error $!

echo 'done.'