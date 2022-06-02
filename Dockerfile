FROM devkitpro/devkitppc:latest

COPY --from=wiiuenv/libiosuhax:latest /artifacts $DEVKITPRO

CMD dkp-pacman -Syyu --noconfirm ppc-freetype

RUN git clone -b customheap_spinlock --single-branch https://github.com/GaryOderNichts/wut && \
  cd wut && \
  git checkout ec0d038f447c4d35db359d6ed8baeb4247f6caf1 && \
  make -j$(nproc) && \
  make install && \
  cd .. && \
  rm -rf wut

WORKDIR project
