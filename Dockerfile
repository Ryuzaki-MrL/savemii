FROM wiiuenv/devkitppc:latest

COPY --from=wiiuenv/libiosuhax:latest /artifacts $DEVKITPRO

CMD dkp-pacman -Syyu --noconfirm ppc-freetype
CMD git clone --recursive https://github.com/Crementif/libfat
WORKDIR libfat 
CMD make wiiu-release && make wiiu-install
CMD tar -xf distribute/1.2.0/libfat-ogc-1.2.0.tar.bz2
CMD cp -r $DEVKITPRO/wut/usr

WORKDIR ../project
