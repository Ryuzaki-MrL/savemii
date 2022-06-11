FROM devkitpro/devkitppc:latest

COPY --from=wiiuenv/libiosuhax:latest /artifacts $DEVKITPRO

CMD dkp-pacman -Syyu --noconfirm ppc-freetype

RUN git clone -b customheap --single-branch https://github.com/GaryOderNichts/wut && \
  cd wut && \
  make -j$(nproc) && \
  make install && \
  cd .. && \
  rm -rf wut

RUN git clone --recursive https://github.com/Crementif/libfat && \
 cd libfat && \
 make -j$(nproc) wiiu-install && \
 cd .. && \
 rm -rf libfat
WORKDIR project
