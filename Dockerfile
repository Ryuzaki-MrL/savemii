FROM wiiuenv/devkitppc:latest

COPY --from=wiiuenv/libiosuhax:latest /artifacts $DEVKITPRO

RUN git clone -b customheap --single-branch https://github.com/GaryOderNichts/wut && \
 cd wut && \
 git checkout 741dbe68671ab5a3df64b19366590d5ec7b5eb0e && \
 make -j$(nproc) && \
 make install && \
 cd .. && \
 rm -rf wut

CMD dkp-pacman -Syyu --noconfirm ppc-freetype

WORKDIR project
