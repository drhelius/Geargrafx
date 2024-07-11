FROM ubuntu:focal

ENV LANG="C.UTF-8"

# Install basic build tools
RUN apt-get update \
    && apt-get upgrade -y \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
        make \
        unzip \
        zip \
        build-essential \
        libsdl2-dev \
        libglew-dev \
        libgtk-3-dev \
        gdb \
        vim \
        cmake \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get autoremove -y && apt-get clean -y

# Default to bash shell (other shells available at /usr/bin/fish and /usr/bin/zsh)
ENV SHELL=/usr/bin/zsh \
    DOCKER_BUILDKIT=1

# Mount for docker-in-docker 
VOLUME [ "/var/lib/docker" ]

# Fire Docker/Moby script if needed
ENTRYPOINT [ "/usr/local/share/docker-init.sh", "/usr/local/share/ssh-init.sh"]
CMD [ "sleep", "infinity" ]
