FROM wiiuenv/devkitppc:latest

COPY --from=wiiuenv/libiosuhax:latest /artifacts $DEVKITPRO
COPY --from=wiiulegacy/libfat:1.1.3a /artifacts $DEVKITPRO/wut/usr

CMD dkp-pacman -Syyu --noconfirm ppc-freetype

WORKDIR project