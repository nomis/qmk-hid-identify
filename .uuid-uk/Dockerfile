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
	--mount=type=cache,target=/root/.cache/pip,sharing=locked,id=debian-testing-root-cache-pip \
	pip3 install --break-system-packages -U meson ninja

RUN \
	--mount=type=cache,sharing=locked,target=/var/cache/apt,id=debian-testing-var-cache-apt \
	--mount=type=cache,sharing=locked,target=/var/lib/apt,id=debian-testing-var-lib-apt \
	apt-get install -y mingw-w64
