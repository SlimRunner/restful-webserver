FROM ubuntu:22.04

ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=1000

ENV DEBIAN_FRONTEND=noninteractive

# Install build tools + libs
RUN apt-get update && apt-get install -y \
  build-essential \
  clangd \
  cmake \
  curl \
  gdb \
  git \
  sudo \
  gcovr \
  lcov \
  libboost-all-dev \
  libgtest-dev \
  libgmock-dev \
  netcat-openbsd \
  && rm -rf /var/lib/apt/lists/*

# Build gtest/gmock (needed on Ubuntu)
RUN cd /usr/src/googletest/googletest && cmake . && make && cp lib/*.a /usr/lib/ && \
  cd /usr/src/googletest/googlemock && cmake . && make && cp lib/*.a /usr/lib/

# Create non-root user for VS Code Remote Container
RUN groupadd --gid ${USER_GID} ${USERNAME} \
  && useradd -s /bin/bash --uid ${USER_UID} --gid ${USER_GID} -m ${USERNAME} \
  && echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/${USERNAME} \
  && chmod 0440 /etc/sudoers.d/${USERNAME}

USER ${USERNAME}
ENV HOME /home/${USERNAME}
WORKDIR /workspace
