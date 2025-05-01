- [Development](#development)
  - [C++ Environment](#c-environment)
    - [Setup IDE](#setup-ide)
    - [Devel Environment](#devel-environment)
    - [Run CMake](#run-cmake)
  - [Setup Local Environment](#setup-local-environment)
    - [Create Docker Container](#create-docker-container)
      - [Clone Repo from Gerrit](#clone-repo-from-gerrit)
      - [Create Base](#create-base)
      - [Run Config File to Test and Deploy](#run-config-file-to-test-and-deploy)
      - [Run the Web Server](#run-the-web-server)
    - [Create VM](#create-vm)
    - [Configure VM](#configure-vm)
      - [The Essentials](#the-essentials)
      - [Kubernetes](#kubernetes)
      - [Install GCloud](#install-gcloud)
    - [Setup SSH](#setup-ssh)
    - [Setup Devel](#setup-devel)

# Development

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
 ../tools/env/start.sh -u mariomar314 -r
```
Assuming that you have your devel tools one directory up.

### Run CMake
You can run this manually or use our script `./run_build.sh` to configure everything. The command has the following modes:

* `--clean`: Deletes the `build` and `build_coverage` directories.
* `--build`: Builds the release version only.
* `--int`: Updates the build with CMake and runs integration tests only.
* `--cover`: Builds the coverage version only.
* `-h`, `--help`: Get this same help in the terminal.

You can still manually `cd` into the desired build directory and run the appropriate `make` commands to test or execute the build.

## Setup Local Environment
There are many ways to set up this project locally. This will describe the recommended way using a VM with Ubuntu.

### Create Docker Container

#### Clone Repo from Gerrit
First, create SSH keys for your local computer just the same way you did for the GCloud console.
```sh
ssh-keygen -t ed25519
```
Now find the file `~/.ssh/id_ed25519.pub` or `C:\Users\user_name\.ssh\id_ed25519.pub` (in Windows) and copy its contents to a new SSH key entry in your Gerrit settings.

Now clone using SSH to get the repo locally. You can find the commands here:
> [https://code.cs130.org/admin/repos/tutututu-maxverstappen,general](https://code.cs130.org/admin/repos/tutututu-maxverstappen,general)

**IMPORTANT**: Use the one with the hook. Otherwise, you won't be able to commit.

#### Create Base
Since our config file references itself, you must first build the base and then run the config on the latest:
```sh
docker build -f ./docker/base.Dockerfile -t tutututu-maxverstappen:base .
```
This command may take several minutes to finish. The final image in Docker takes about 1 GB.

#### Run Config File to Test and Deploy
Run the _almost_ same command again, but with a few modifications:
```sh
docker build -f ./docker/Dockerfile -t tutututu-maxverstappen:latest .
```

After this, you should see two images in your Docker. One is tagged `base`, and the other one is tagged `latest`.

#### Run the Web Server
The moment of truth. Run this command:
```sh
docker run --rm -p 8080:8081 --name my_run tutututu-maxverstappen:latest
```

Notice the map from 8080 to 8081. This may be changed in the future, but our server is currently configured to run on 8081.

> NOTE: The `--rm` flag in the command is there to auto-delete the `my_run` container when the program finishes.

Another thing to note is that when you run this, the server will take over your terminal. If you want to stop it, open a new one and call:
```sh
docker stop my_run
```
It takes about 10 seconds to shut it down.

### Create VM
1. Download the current Ubuntu LTS from [their website](https://ubuntu.com/download/desktop).
2. [Download and install VirtualBox](https://www.virtualbox.org/wiki/Downloads). Use the platform packages.
3. Once in VirtualBox, click `New`.
4. Select the ISO you downloaded in step one.
   1. Leave the unattended install checked.
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

### Setup Devel
Open the terminal:
```sh
cd ~
mkdir cs130
cd cs130
git clone https://code.cs130.org/tools
cd ..
ssh-keygen -t ed25519
gnome-text-editor .bashrc
# copy the contents
```
Now do the following:
1. Open Firefox.
2. Go to [code.cs130.org](https://code.cs130.org/).
   1. You should be signed in because of the earlier step in GCloud.
3. Go to settings > keygen > paste SSH key you copied.
4. Now go to [browse > repos > tututu... > SSH without hook](https://code.cs130.org/admin/repos/tutututu-maxverstappen,general) and copy that command.

```sh
cd ~/cs130
git clone "ssh://USERNAME@code.cs130.org:29418/tutututu-maxverstappen"
cd tutututu-maxverstappen
../tools/env/start.sh -u ${USER} -r
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
