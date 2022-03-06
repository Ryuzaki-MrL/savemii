FROM wiiuenv/devkitppc:latest

COPY --from=wiiuenv/libiosuhax:latest /artifacts $DEVKITPRO

/bin/sh -c dkp-pacman -Syyu --noconfirm ppc-freetype

WORKDIR project
