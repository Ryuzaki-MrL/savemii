FROM wiiuenv/devkitppc:latest

COPY --from=wiiuenv/libiosuhax:latest /artifacts $DEVKITPRO

CMD dkp-pacman -Syyu --noconfirm ppc-freetype
CMD git clone --recursive https://github.com/Crementif/libfat
WORKDIR libfat 
CMD make wiiu-release && make wiiu-install
CMD cd ..

WORKDIR project
