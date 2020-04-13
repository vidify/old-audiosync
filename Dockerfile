# Multipurpose Dockerfile for tests.

ARG python_version=3.8
FROM python:${python_version}-buster
WORKDIR /audiosync/
# Needed to install programs without interaction
ENV DEBIAN_FRONTEND=noninteractive
# Continuous integration indicator (some tests may be skipped)
ENV CI=true

# Apt build dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    ffmpeg \
    gnuplot \
    libfftw3-dev \
    libpulse-dev \
    pulseaudio \
 && rm -rf /var/lib/apt/lists/*

# Installing the module
COPY . .
RUN pip install .

CMD sh dev/run-tests-docker.sh
