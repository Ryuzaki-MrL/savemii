FROM wiiuenv/devkitppc:20220806 AS final

CMD dkp-pacman -Syyu --noconfirm ppc-freetype

RUN git clone --recursive https://github.com/wiiu-env/libmocha -b devoptab --single-branch && \
 cd libmocha && \
 make -j$(nproc) && \
 make install && \
 cd .. && \
 rm -rf libmocha

RUN git clone --recursive https://github.com/Xpl0itU/libfat && \
  cd libfat && \
  make -j$(nproc) wiiu-install && \
  cd .. && \
  rm -rf libfat

WORKDIR /project
