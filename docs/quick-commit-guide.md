- [Quick Commit Guide](#quick-commit-guide)
  - [Committing Style Guide](#committing-style-guide)
    - [Write Awesome Commit Messages](#write-awesome-commit-messages)
    - [How to Save in nano](#how-to-save-in-nano)
  - [Miscelaneus Commands](#miscelaneus-commands)

# Quick Commit Guide

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
This is a list of various commands that you may find useful.

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
