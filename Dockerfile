FROM wiiuenv/devkitppc:latest

COPY --from=wiiuenv/libiosuhax:latest /artifacts $DEVKITPRO

CMD dkp-pacman -Syyu --noconfirm ppc-freetype
CMD git clone --recursive https://github.com/Crementif/libfat
WORKDIR libfat 
RUN make wiiu-release && \
	find $DEVKITPRO/portlibs -maxdepth 3 -type f -delete && \
	make -C wiiu install && \
	cp -r ${DEVKITPRO}/portlibs /artifacts

WORKDIR ../project
