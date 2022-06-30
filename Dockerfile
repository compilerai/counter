FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive
# enable i386 packages support
RUN dpkg --add-architecture i386
# install build-essential and other required dependencies
COPY install-dependencies.sh /tmp/install-dependencies.sh
RUN apt-get update && bash /tmp/install-dependencies.sh
# add non-root user
RUN groupadd -r sbansal && \
    useradd -r -g sbansal -d /home/sbansal -s /bin/bash -c "Docker image user" sbansal && \
    mkdir -p /home/sbansal && \
    chown -R sbansal:sbansal /home/sbansal
# copy everything to sbansal directory
COPY --chown=sbansal . /home/sbansal/counter/
WORKDIR /home/sbansal/counter
# switch to non-root sbansal
USER sbansal
ENV SUPEROPT_TARS_DIR    /home/sbansal/counter/tars
ENV SUPEROPT_PROJECT_DIR /home/sbansal/counter
ENV LOGNAME sbansal
