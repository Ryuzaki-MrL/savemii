FROM wiiuenv/devkitppc:latest

COPY --from=wiiuenv/libiosuhax:latest /artifacts $DEVKITPRO

CMD dkp-pacman -Syyu --noconfirm ppc-freetype

WORKDIR project
CMD git clone --recursive https://github.com/Crementif/libfat && cd libfat && make wiiu-release && make wiiu-install && cd ..