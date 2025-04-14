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

COPY --from=builder /usr/src/project/build/bin/webserver .
COPY --from=builder /usr/src/project/configs/server_config.txt .

# this is purely for metadata, does not actually open a port, but it is
# good to keep updated for people reading this file
EXPOSE 8081

# Use ENTRYPOINT to specify the binary name
ENTRYPOINT ["./webserver"]

# Use CMD to specify arguments to ENTRYPOINT
CMD ["server_config.txt"]
