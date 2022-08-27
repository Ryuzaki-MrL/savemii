FROM wiiuenv/devkitppc:20220806 AS final

RUN apt -y install gettext
CMD dkp-pacman -Syyu --noconfirm ppc-freetype

RUN git clone --recursive https://github.com/wiiu-env/libmocha --single-branch && \
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
RUN git clone --recursive https://github.com/yawut/libromfs-wiiu --single-branch && \
 cd libromfs-wiiu && \
 make -j$(nproc) && \
 make install && \
 cd .. && \
 rm -rf libromfs-wiiu

WORKDIR /project
