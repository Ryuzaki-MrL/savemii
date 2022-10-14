FROM wiiuenv/devkitppc:20220917 AS final

COPY --from=wiiuenv/libmocha:20220919 /artifacts $DEVKITPRO

RUN apt -y install gettext
CMD dkp-pacman -Syyu --noconfirm ppc-freetype

RUN git clone --recursive https://github.com/Xpl0itU/libfat --single-branch && \
  cd libfat && \
  make -j$(nproc) wiiu-install && \
  cd .. && \
  rm -rf libfat

RUN git clone --recursive https://github.com/yawut/libromfs-wiiu --single-branch && \
  cd libromfs-wiiu && \
  make -j$(nproc) && \
  make install && \
  cd .. && \
  rm -rf libromfs-wiiu

WORKDIR /project
