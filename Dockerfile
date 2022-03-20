FROM wiiuenv/devkitppc:latest

COPY --from=wiiuenv/libiosuhax:latest /artifacts $DEVKITPRO
COPY --from=wiiuenv/libfat:latest /artifacts $DEVKITPRO/wut/usr

CMD dkp-pacman -Syyu --noconfirm ppc-freetype

WORKDIR project