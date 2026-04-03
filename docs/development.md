- [Development](#development)
  - [Setup GCP](#setup-gcp)
  - [C++ Environment](#c-environment)
    - [Setup IDE](#setup-ide)
    - [Devel Environment](#devel-environment)
    - [Run CMake](#run-cmake)
  - [Setup Local Container for Testing](#setup-local-container-for-testing)
    - [Create Docker Container](#create-docker-container)
      - [Create Base](#create-base)
      - [Run Config File to Test and Deploy](#run-config-file-to-test-and-deploy)
      - [Run the Web Server](#run-the-web-server)
  - [Setup Local Development](#setup-local-development)
    - [Create VM](#create-vm)
    - [Configure VM](#configure-vm)
      - [The Essentials](#the-essentials)
      - [Kubernetes](#kubernetes)
      - [Install GCloud](#install-gcloud)
    - [Setup SSH](#setup-ssh)
    - [Setup Devel (OUTDATED STEP)](#setup-devel-outdated-step)
  - [Recommended VS Code Extensions](#recommended-vs-code-extensions)

# Development

> TODO: remove or migrate the `tools/env/start.sh` away from `code.cs130.org`

## Setup GCP
1. create project with name `restful-webserver`
2. set billing (if not done before)
3. go to `IAM & Admin > IAM` and add grants to members (if needed)
4. setup `gcloud init` locally for convenience
5. go to https://console.cloud.google.com/cloud-build/builds
   1. enabled cloud-builds
   2. run `gcloud builds submit --config .\docker\cloudbuild.yaml .`
   3. if it succeeds move on
6. Enable VM instances and create one instance
   1. add additional persistent storage
   2. reserve static IP
   3. NO backups
   4. add `google-logging-enabled: true` under metadata
   5. add container (technically deprecated)

## C++ Environment

### Setup IDE
This project is setup for VS Code. Once you cloned open the command palette and run `Dev Containers: Reopen in Container`

Once inside run
```sh
./dev --makelist
```

Once you load C++ file you might need to restart the clang language server.

### Run CMake
You can run this manually or use our script `./run_build.sh` to configure everything. The command has the following modes:

- `--clean`: Deletes the `build` and `build_coverage` directories.
- `--build`: Builds the release version only.
- `--int`: Updates the build with CMake and runs integration tests only.
- `--cover`: Builds the coverage version only.
- `--makelist`: Generates building tools with CMakeLists only
- `-h`, `--help`: Get this same help in the terminal.

You can still manually `cd` into the desired build directory and run the appropriate `make` commands to test or execute the build.

## Setup Local Container for Testing
There are many ways to set up this project locally. This will describe the recommended way using a VM with Ubuntu.

### Create Docker Container

#### Create Base
Since our config file references itself, you must first build the base and then run the config on the latest:
```sh
docker build -f ./docker/base.Dockerfile -t restful-webserver:base .
```
This command may take several minutes to finish. The final image in Docker takes about 1 GB.

#### Run Config File to Test and Deploy
Run the _almost_ same command again, but with a few modifications:
```sh
docker build -f ./docker/Dockerfile -t restful-webserver:latest .
```

After this, you should see two images in your Docker. One is tagged `base`, and the other one is tagged `latest`.

> NOTE: if re-running this to refresh image does not work append `--no-cache` after `docker build` to the above commands.

If you are forced to do this, remember to cleanup dangling images.
```sh
docker image prune
```
You can see what images are active with this command
```sh
docker images
```

#### Run the Web Server
The moment of truth. Run this command:
```sh
docker run --rm -p 8080:8081 --name my_run restful-webserver:latest
```

Notice the map from 8080 to 8081. This may be changed in the future, but our server is currently configured to run on 8081.

> NOTE: The `--rm` flag in the command is there to auto-delete the `my_run` container when the program finishes.

Another thing to note is that when you run this, the server will take over your terminal. If you want to stop it, open a new one and call:
```sh
docker stop my_run
```
It should close immediately.

## Recommended VS Code Extensions
- [Rewrap](https://marketplace.visualstudio.com/items/?itemName=stkb.rewrap)
  - `ctrl+q` for automatic line wrapping.
- [Bookmarks](https://marketplace.visualstudio.com/items/?itemName=alefragnani.Bookmarks)
  - `alt+ctrl+k` to add bookmarks, `alt+ctrl+l` to go to the next one.
- [Black Formatter](https://marketplace.visualstudio.com/items/?itemName=ms-python.black-formatter)
  -  good formatter for Python code
- [Git Graph](https://marketplace.visualstudio.com/items/?itemName=mhutchie.git-graph)
- [Markdown All in One](https://marketplace.visualstudio.com/items/?itemName=yzhang.markdown-all-in-one)
  - Allows you to automatically update and generate a table of contents.
- [C++ Extension pack](https://marketplace.visualstudio.com/items/?itemName=ms-vscode.cpptools-extension-pack)
