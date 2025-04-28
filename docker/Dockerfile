### Build/test container ###
# Define builder stage
FROM tutututu-maxverstappen:base AS builder

# Share work directory
COPY . /usr/src/project
WORKDIR /usr/src/project/build

# Build and test
RUN cmake ..
RUN make
RUN ctest --output-on_failure


### Deploy container ###
# Define deploy stage
FROM ubuntu:noble AS deploy

# set default config file (local)
ARG CONFIG_FILE=server_config.txt
# make sure it is added to the environment
ENV CONFIG_FILE=${CONFIG_FILE}

COPY --from=builder /usr/src/project/build/bin/webserver .
# NOTE: the config files are renamed inside the container to ./server_config.txt
COPY --from=builder /usr/src/project/configs/${CONFIG_FILE} ./server_config.txt
# Copy the static directory
COPY --from=builder /usr/src/project/static ./static

# this is purely for metadata, does not actually open a port, but it is
# good to keep updated for people reading this file
EXPOSE 8081
EXPOSE 80

# use the webserver as entry point
ENTRYPOINT ["./webserver"]

# read the ./server_config.txt inside the container
CMD ["./server_config.txt"]
