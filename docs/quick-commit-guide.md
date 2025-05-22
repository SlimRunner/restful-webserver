- [Quick Commit Guide](#quick-commit-guide)
  - [Send Patch for Review](#send-patch-for-review)
  - [Retrieve Patch to Make Changes](#retrieve-patch-to-make-changes)
  - [Committing Style Guide](#committing-style-guide)
    - [Write Awesome Commit Messages](#write-awesome-commit-messages)
    - [How to Save in nano](#how-to-save-in-nano)
  - [Miscelaneus Commands](#miscelaneus-commands)

# Quick Commit Guide

## Send Patch for Review
```sh
# simple patch (no reviewer)
git review -f
```
```sh
# simple patch, specify reviewer(s)
git review -f --reviewers USER@g.ucla.edu
```
Below is a list of this command with each of us as reviewers (for easy copy and paste):
```sh
git review -f --reviewers chrisescobar@g.ucla.edu
```
```sh
git review -f --reviewers mariomar314@g.ucla.edu
```
```sh
git review -f --reviewers kimoliatis01@g.ucla.edu
```
```sh
git review -f --reviewers seamar@g.ucla.edu
```

You may remove the `-f` flag and just keep the branch locally but be careful if you do that because you might mess your own commit if you mistakenly add commits on top of your branch instead of main.

## Retrieve Patch to Make Changes
In the https://code.cs130.org/ UI, click the three dots inside your CL and click `download patch`. Copy the checkout command. It looks roughly like this:
```
git fetch ssh://${USER}@code.cs130.org:29418/tutututu-maxverstappen refs/changes/xy/abcxy/n && git checkout FETCH_HEAD
```
> Make sure you are in the devel environment.

This will put you in a detached HEAD state. As you make your changes, **amend** them to HEAD. Once you are done, run:
```sh
git review
```
The `-f` flag is not needed because you are in a detached HEAD. If you are on main, the review will go through, but nothing will happen to main because it cannot be deleted. If you just so happen to be in another local branch, it may be gone.

## Committing Style Guide

### Write Awesome Commit Messages
VS Code can help you write amazing commit messages:
* Press `ctrl + shift + p` to open the command palette.
* Type `change language mode` (or more quickly press `ctrl + k` > `m`).
* Type `git commit message`.

Now you should see two lines. One at 50 characters and one at 72 characters. Your title should not exceed the 50-character line. Your extended message should start two newlines below your commit title. For example:
```
This is a commit title below 50 characters

This is a commit message. Lorem ipsum dolor sit amet consectetur
adipisicing elit. Repellendus voluptatibus mollitia dolore rerum veniam
cupiditate dignissimos nostrum assumenda magni possimus, exercitationem
sit, dicta at rem repellat sequi. Tempora, modi vel?
```
To quickly format the message to the right length, I recommend the [Rewrap extension](https://marketplace.visualstudio.com/items/?itemName=stkb.rewrap). It comes configured for 72 characters already.

Once you are done writing your message, copy it. Then in the terminal (within the devel environment):
```
git commit -a
```
This will open nano. Now you can copy your message in VS Code and right-click in the terminal to paste it into nano.

### How to Save in nano
Make some changes > `ctrl + x` > `y` > `enter`.

## Miscelaneus Commands
This is a list of various commands that you may be useful.

---
Mirror a folder structure to set up testing
```sh
rsync -a -f"+ */" -f"- *" src/ tests/
```

---
Print report of commit count by user
```sh
git shortlog -s -n
```

---
Print report of commit count by email
```sh
git log --format='%ae' | sort | uniq -c | sort -nr
```

---
Line count report of files inside `path`
```sh
find <path> -type f -print0 | wc -l --files0-from=-
```
