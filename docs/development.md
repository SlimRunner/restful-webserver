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
To keep code clean, do the following to set up auto-formatting:

First, add the following to your `.vscode/settings.json`:
```jsonc
{
  "[cpp]": {
    "editor.tabSize": 4,
    "editor.insertSpaces": true
  },
  "C_Cpp.clang_format_style": "file", // Use .clang-format if available
  "C_Cpp.clang_format_fallbackStyle": "Google", // Or LLVM, Mozilla, etc.
  "editor.formatOnSave": true
}
```
Append this to whatever is in there already. This file is not tracked because it contains noise that the C++ extension adds. With this in place, the `.clang-format` file will set up all the rules we want for formatting. Lastly, this makes it so that the format is applied every time you save, making it easier to keep a consistent code style.

### Devel Environment
This project uses a Docker container to run the project. To set that up, run:
```sh
 ../tools/env/start.sh -u ${User_Name} -r
```

Optional: -r, flag to mirror the boost, gtest, and gmock libraries into the repo to get intellisense and autocomplete

Assuming that you have your devel tools one directory up.

### Run CMake
You can run this manually or use our script `./run_build.sh` to configure everything. The command has the following modes:

- `--clean`: Deletes the `build` and `build_coverage` directories.
- `--build`: Builds the release version only.
- `--int`: Updates the build with CMake and runs integration tests only.
- `--cover`: Builds the coverage version only.
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

## Setup Local Development

### Create VM
1. Download the current Ubuntu LTS from [their website](https://ubuntu.com/download/desktop).
2. [Download and install VirtualBox](https://www.virtualbox.org/wiki/Downloads). Use the platform packages.
3. Once in VirtualBox, click `New`.
4. Select the ISO you downloaded in step one.
   1. Leave "skip unattended install" unchecked.
   2. 8 GB of memory is recommended.
   3. 35 GB of storage is recommended.
   4. Use half of your cores (recommended).
   5. Let the installation proceed.
5. Once it is done, **do not sign in**. Shut down instead.
6. Go to your VM in VirtualBox, and in settings, navigate to display settings and increase the memory to at least 128 MB.
7. Log in to your VM.

> TIP: Name your user the same as your UCLA user so that you can use `$USER` later in the terminal.

### Configure VM

#### The Essentials
```sh
sudo apt update
sudo apt upgrade
# installing essentials
sudo apt update && sudo apt install -y build-essential curl wget git python3 python3-pip openjdk-17-jdk golang-go nodejs npm docker.io unzip zip bash-completion zsh
```
#### Kubernetes
Not sure if needed, but just in case (and GCloud has it).

[Link to reference](https://kubernetes.io/docs/tasks/tools/install-kubectl-linux/#install-using-native-package-management)
```sh
# installing Kubernetes
# this comes straight from their apt instructions
sudo apt-get update
# apt-transport-https may be a dummy package; if so, you can skip that package
sudo apt-get install -y apt-transport-https ca-certificates curl gnupg

# If the folder `/etc/apt/keyrings` does not exist, it should be created before the curl command, read the note below.
# sudo mkdir -p -m 755 /etc/apt/keyrings
curl -fsSL https://pkgs.k8s.io/core:/stable:/v1.33/deb/Release.key | sudo gpg --dearmor -o /etc/apt/keyrings/kubernetes-apt-keyring.gpg
sudo chmod 644 /etc/apt/keyrings/kubernetes-apt-keyring.gpg # allow unprivileged APT programs to read this keyring

# This overwrites any existing configuration in /etc/apt/sources.list.d/kubernetes.list
echo 'deb [signed-by=/etc/apt/keyrings/kubernetes-apt-keyring.gpg] https://pkgs.k8s.io/core:/stable:/v1.33/deb/ /' | sudo tee /etc/apt/sources.list.d/kubernetes.list
sudo chmod 644 /etc/apt/sources.list.d/kubernetes.list   # helps tools such as command-not-found to work correctly

# finally
sudo apt-get update
sudo apt-get install -y kubectl
```
#### Install GCloud
So that you can SSH from VS Code locally.

[Link to reference](https://cloud.google.com/sdk/docs/install#linux)
```sh
curl -O https://dl.google.com/dl/cloudsdk/channels/rapid/downloads/google-cloud-cli-linux-x86_64.tar.gz
tar -xf google-cloud-cli-linux-x86_64.tar.gz
# you will be asked to sign into your UCLA account in Firefox
# say yes when asked to add to PATH
# if asked about compute zone answer no (or yes if you want) it just sets default
./google-cloud-sdk/install.sh
```
Restart the terminal:
```sh
gcloud init
```
If it does not work, you did not set up the PATH.

Before finishing run this command
```sh
docker run hello-world
```
If that fails (which likely will), then you have to add yourself to the docker group:
```sh
sudo usermod -aG docker $USER
```
[Visit this link](https://stackoverflow.com/questions/48957195/how-to-fix-docker-permission-denied) if you need further help with docker

### Setup SSH
```sh
sudo apt update
sudo apt install -y openssh-server
sudo systemctl enable ssh # so that it runs automatically on startup
sudo systemctl start ssh
```
You can check if it worked with:
```sh
sudo systemctl status ssh
```

### Setup Devel (OUTDATED STEP)
Open the terminal:
```sh
cd ~
mkdir cs130
cd cs130
git clone https://code.cs130.org/tools
cd ..
ssh-keygen -t ed25519
gnome-text-editor ~/.ssh/id_ed25519.pub
# copy the contents
```
Now do the following:
1. Open Firefox.
2. Go to [code.cs130.org](https://code.cs130.org/).
   1. You should be signed in because of the earlier step in GCloud.
3. Go to settings > keygen > paste SSH key you copied.
4. Now go to [browse > repos > restful... > SSH](https://code.cs130.org/admin/repos/restful-webserver,general) and copy that command.

**IMPORTANT**: Use the SSH command *with* hook. Otherwise, you won't be able to commit.

```sh
cd ~/cs130
git clone "ssh://USERNAME@code.cs130.org:29418/restful-webserver"
cd restful-webserver
../tools/env/start.sh -u USERNAME -r
```
Now attempt to pull:
```sh
git pull
```
It should fail, but it will ask you to add a new key for Gerrit. Now:
```sh
cat ~/.ssh/id_ed25519.pub
# copy the key
```
Go back to [SSH keys](https://code.cs130.org/settings/#SSHKeys) in the Gerrit website, and add that key. Attempt to pull again. Now it should work.

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
