FROM debian:testing
ENV DEBIAN_FRONTEND=noninteractive

RUN echo 'APT::Keep-Downloaded-Packages "true";' >/etc/apt/apt.conf.d/50keep-cache
RUN \
	--mount=type=cache,sharing=locked,target=/var/cache/apt,id=debian-testing-var-cache-apt \
	--mount=type=cache,sharing=locked,target=/var/lib/apt,id=debian-testing-var-lib-apt \
	apt-get update
RUN \
	--mount=type=cache,sharing=locked,target=/var/cache/apt,id=debian-testing-var-cache-apt \
	--mount=type=cache,sharing=locked,target=/var/lib/apt,id=debian-testing-var-lib-apt \
	apt-get install -y build-essential git procps python3-pip
RUN \
	--mount=type=cache,sharing=locked,target=/var/cache/apt,id=debian-testing-var-cache-apt \
	--mount=type=cache,sharing=locked,target=/var/lib/apt,id=debian-testing-var-lib-apt \
	apt-get install -y gcc-mingw-w64-x86-64-win32 g++-mingw-w64-x86-64-win32

ADD https://pypi.org/pypi/ninja/json /dev/shm/ninja.json
RUN \
	--mount=type=cache,target=/root/.cache/pip,sharing=locked,id=debian-testing-root-cache-pip \
	pip3 install --break-system-packages -U ninja

ADD https://pypi.org/pypi/meson/json /dev/shm/meson.json
RUN \
	--mount=type=cache,target=/root/.cache/pip,sharing=locked,id=debian-testing-root-cache-pip \
	pip3 install --break-system-packages -U meson
