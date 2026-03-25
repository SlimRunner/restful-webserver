# coverage.Dockerfile
# Define build stage
FROM restful-webserver:base AS build

# Copy project source
COPY . /usr/src/project
WORKDIR /usr/src/project/build

# Build
RUN cmake ..
RUN make

# generate code coverage reports
RUN cmake -DCMAKE_BUILD_TYPE=Coverage ..
RUN make coverage

# Generate coverage summary (prints to logs)
RUN gcovr -r ..
