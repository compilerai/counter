FROM ubuntu:18.04

ARG DEBIAN_FRONTEND=noninteractive
# enable i386 packages support
RUN dpkg --add-architecture i386
# install build-essential and other required dependencies
COPY install-dependencies.sh /tmp/install-dependencies.sh
RUN apt-get update && apt-get install -y build-essential python python-pip && bash /tmp/install-dependencies.sh
# add non-root user
RUN groupadd -r user && \
    useradd -r -g user -d /home/user -s /bin/bash -c "Docker image user" user && \
    mkdir -p /home/user && \
    chown -R user:user /home/user
# copy everything to user directory
COPY --chown=user . /home/user/artifact/
WORKDIR /home/user/artifact
# switch to non-root user
USER user
ENV SUPEROPT_TARS_DIR    /home/user/artifact/tars
ENV SUPEROPT_PROJECT_DIR /home/user/artifact
ENV LOGNAME user
