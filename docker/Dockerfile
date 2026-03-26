### Build/test container ###
FROM restful-webserver:base AS build

# Share work directory
COPY . /usr/src/project
WORKDIR /usr/src/project/build

# Build and test
RUN cmake ..
RUN make


### Test container ###
FROM build AS test

RUN ctest --output-on-failure


### Deploy container ###
FROM test AS deploy

# NOTE: usually you want to avoid putting development tools in the
# deployment Previously this step had `ubuntu:noble` instead of `test`.
# Base currently installs all of the devtools so it does not really
# matter which one you do because the dev tools are already on
# `ubuntu:noble`.

# Set the working directory to /usr/src/project
WORKDIR /usr/src/project

# set default config file (local)
ARG CONFIG_FILE=server_config.txt
# make sure it is added to the environment
ENV CONFIG_FILE=${CONFIG_FILE}

COPY --from=build /usr/src/project/build/bin/webserver .
# NOTE: the config files are renamed inside the container to ./server_config.txt
COPY --from=build /usr/src/project/configs/${CONFIG_FILE} ./server_config.txt
# Copy the static directory
COPY --from=build /usr/src/project/static ./static

# this is purely for metadata, does not actually open a port, but it is
# good to keep updated for people reading this file
EXPOSE 8081
EXPOSE 80

# use the webserver as entry point
ENTRYPOINT ["./webserver"]

# read the ./server_config.txt inside the container
CMD ["./server_config.txt"]
