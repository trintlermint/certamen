<div align="center">

# CERTAMEN

<p>
  <img src="assets/1.png" width="32%" alt="Demo 1">
  <img src="assets/2.png" width="32%" alt="Demo 2">
  <img src="assets/3.png" width="32%" alt="Demo 3">
</p>

A _Terminal User Interface_ **Quiz Game Engine** written entirely in C++.

Made with love by [trintlermint](#credits).

Read
_[my blog](https://blog.trintler.me/2026/03/28/Certamen-the-TUI-Quizzing-App/)_
documenting the development process for this project.

## Author _Quizzes_ and seamlessly "test" yourself in a full-screen TUI; host the game over SSH for you and your friends to play!

> This way, you get to make fun of their... incredibly (lacking) _haskell_
> knowledge for example!

#### Releases

[![AUR version](https://img.shields.io/aur/version/certamen)](https://aur.archlinux.org/packages/certamen)
[![Release](https://img.shields.io/github/v/release/trintlermint/certamen?label=linux&color=success)](https://github.com/trintlermint/certamen/releases/latest)
[![Release](https://img.shields.io/github/v/release/trintlermint/certamen?label=macOS&color=success)](https://github.com/trintlermint/certamen/releases/latest)
[![Windows CI](https://img.shields.io/badge/windows-CI%20failing%20%28incompatible%29-red)](https://github.com/trintlermint/certamen/actions)

#### Notes

[![Dependencies](https://img.shields.io/badge/dependencies-FTXUI%20%7C%20yaml--cpp%20%7C%20libssh-informational)](#dependencies)
[![SSH Compatible](https://img.shields.io/badge/SSH-compatible-brightgreen)](#ssh-server-mode)
[![License: MIT](https://img.shields.io/badge/license-MIT-yellow.svg)](#license)

#### Version Control

[![Codeberg](https://img.shields.io/badge/Codeberg-certamen-blue?logo=codeberg)](https://codeberg.org/trintlermint/certamen)
[![GitHub](https://img.shields.io/badge/GitHub-certamen-181717?logo=github)](https://github.com/trintlermint/certamen)

### One might ask: I'm passing everything, why do I need this?

## Charlie Brown aces his tests with this!

<img src="./assets/peanuts-quiz.png" alt="certamen-banner"/>
  <br />
</div>

## Table of Contents

- [Building](#building)
  - [Releases](#releases)
    - [AUR](#aur)
    - [Nix](#nix)
  - [Dependencies](#dependencies)
  - [CMake](#cmake)
  - [macOS and Windows](#macos-and-windows)
- [Usage](#usage)
  - [Offline user mode](#offline-user-mode)
  - [SSH server mode](#ssh-server-mode)
  - [CLI mode](#cli-mode)
- [Setup](#setup)
  - [Quiz Format](#quiz-format)
  - [Templates](#templates)
- [Troubleshooting](#troubleshooting)
- [Credits](#credits)
- [License](#license)

---

## Building

### Releases

I have now setup Github Releases through Actions, if you want to skip the
headache of building the program, feel free to head over to
[**releases**](https://github.com/trintlermint/certamen/releases/latest).

> [!WARNING]
> This repository has just been moved from Github to Codeberg due to concerns
> regarding licensing, ownership of code, and ethics. Please wait for me to fix
> the current CI/CD, packages, etc to be written for codeberg. Thank you!

#### AUR

To download **Certamen** from the ArchLinux User Repository, do the following:

```bash
sudo pacman -S certamen
```

> Or any equivalent command using your AUR package helper.

#### Nix

To install this package imperatively, run the following: (make sure to have
experimental features enabled)

```bash
nix profile add github:trintlermint#certamen
```

Or add it to your config with the following

```nix
# flake.nix

{
    inputs.certamen.url = "github:trintlermint/certamen";
}

# ...
# somewhere else in your config

{inputs, ...}: {
    environment.systemPackages = [inputs.certamen.packages."x86_64-linux".default];
}
```

### Dependencies

Certamen uses these predominant three libraries:

| Library                                          | Version | Install                                           |
| ------------------------------------------------ | ------- | ------------------------------------------------- |
| [FTXUI](https://github.com/ArthurSonzogni/FTXUI) | v6.1.9  | Fetched automatically by CMake via `FetchContent` |
| [yaml-cpp](https://github.com/jbeder/yaml-cpp)   | >= 0.7  | System package required                           |
| [libssh](https://www.libssh.org/)                | >= 0.9  | System package required                           |

You also need a **C++17 compiler (GCC >= 8 or Clang >= 7)** and **CMake >=
3.14**.

### CMake

**Ubuntu / Debian:**

```bash
sudo apt-get install build-essential cmake libyaml-cpp-dev libssh-dev
```

**Fedora / RHEL:**

```bash
sudo dnf install gcc-c++ cmake yaml-cpp-devel libssh-devel
```

**Arch Linux (AUR):**

```bash
sudo pacman -S base-devel cmake yaml-cpp libssh
```

> Or any other equivalent function to install these packages on your UNIX
> machine.

Next, clone and build:

```bash
git clone https://github.com/trintlermint/certamen.git
cd certamen
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The binary compiles into an executable, stored at `build/bin/certamen`. Run:

```bash
./build/bin/certamen ./templates/example_quiz.yaml
```

Without arguments it looks for `./quiz.yaml` in the current working directory
being run from.

### macOS and Windows

**Homebrew**:

```bash
brew install cmake yaml-cpp libssh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**Windows** (using vcpkg + MSVC):

```bash
cmake --preset release
cmake --build --preset release
```

> [!WARNING]
> I am NOT a macOS OR Windows user; Manuals told me this should work, if it does
> or doesn't please inform me on Github Issues! See
> [CONTRIBUTING.md](CONTRIBUTING.md)

---

## Usage

### Offline User Mode

```bash
certamen quiz.yaml
```

The TUI (when opened using `certamen`) presents a menu with keyboard (j/k),
(up/down/left/right) and number-key navigation:

1. _Take Quiz_; answer questions (`bool: randomiser`), get scored at the end
2. _Add Question_; compose a question with choices, optional code snippet, and
   explanation
3. _Remove Question_; delete a question
4. _Change Answer_; update the correct answer for an existing question
5. _Edit Choice_; update the text contents of a selected question's choice.
6. _List Questions_; browse all questions with toggleable answers, code, and
   explanations
7. _Set Author and Name_; set metadata describing the author/name.
8. _Save_; Save all changes and view diffs.
9. _Load Quiz File_; (Un)load quiz .yaml files into the current session.

There are a few more functionalities:

- Press `r` on the main menu to toggle randomized question _and_ answer order.
- Press `q` anywhere to QUIT the program.

Furthermore, if you wish, you can import more than one .yaml file, you do this
by executing: `certamen 1.yaml 2.yaml ... n.yaml` for any `n` number of yaml
files you desire. This will then be loaded into the session, to manipulate them
such as loading, unloading them, go to the "Load Quiz File" option. Here, you
can Load new files, or get rid of them in the current session.

### SSH server mode

Host a quiz for others to connect to with any SSH client:

```bash
certamen serve quiz.yaml                                  # default
certamen serve --password mysecret --port 2222 quiz.yaml  # port and pass
certamen serve --port 3000 algebra.yaml history.yaml      # port, max client
```

Players connect with `ssh -p <port> <name>@<host>` and get the same TUI in an
_isolated_ "quiz" session. The server logs scores per player.

| Flag                | Default             | Desc                                 |
| ------------------- | ------------------- | ------------------------------------ |
| `--port <N>`        | 2222                | TCP listen port                      |
| `--password <pw>`   | _(open)_            | Require password authentication      |
| `--key <path>`      | `certamen_host_rsa` | RSA host key (auto gen on first run) |
| `--max-clients <N>` | 8                   | Concurrent connection limit          |

> [!IMPORTANT]
> Full server shell documentation: **[SERVING.md](SERVING.md)**. Also, password
> is currently having issues currently.

### CLI mode

The `certamen` executable bundles a plain-text CLI alongside the TUI:

```bash
./build/bin/certamen --cli quiz.yaml
./build/bin/certamen --cli [a.yaml ... n.yaml]
```

---

## Setup

Below is information regarding how to ideally setup certamen and its corresponding quiz files _locally_.

### Quiz format

Quizzes are YAML files. Each question is a map in a top-to-down sequence which
is nested inside `questions`:

```yaml
name: Certamen DEMO
author: trintlermint
questions:
  - question: Which of the Haskell functions below can calculate the Euclidean norm sqrt(a^2 + b^2)?
    code: |
      nrm1 :: Double -> Double -> Double
      nrm1 a b = sqrt (a^2 + b^2)

      nrm2 :: (Double, Double) -> Double
      nrm2 (a, b) = sqrt (a^2 + b^2)

    explain: |
      Both definitions compute sqrt (a^2 + b^2), one by separate arguments and one by a tuple, using only sqrt and (^).

    choices:
      - Only nrm1 is a correct implementation.
      - Only nrm2 is a correct implementation.
      - nrm1 and nrm2 are both correct implementations.
      - nrm1 and nrm2 are both incorrect implementations.
    answer: 2
  - question: Is the else part in a Haskell if expression mandatory?
    explain: |
      Haskell's if is an expression and requires both then and else branches.
    ...
```

- `question`, `choices` >= 2, and `answer` are required. `code` and `explain`
  are optional.
- `answer` is a _0-based_ index into `choices`. The TUI displays 1-based
  numbering to the player.
- `name`, `author` are optional and are metadata only at the start of the file
- `language` is an optional field for syntax highlight hinting (e.g.
  `language: haskell`) (see `syntax.cpp` for what has been implemented thus
  far).

> See [`./templates/example_quiz.yaml`](./templates/example_quiz.yaml) for a working template and
> helping me stop writing stuff (for more see below.)

### Templates

As is understandable, it is very helpful to look at premade quizzes which arent just the template `example_quiz.yaml`,
and for this reason, in the directory `templates/` you will be able to find various community made quizzes to help you
take some quizzes without needing to make it all by yourself!
Try them out! and if you find errors, you can always send us an issue!

> So far, they are just some university practice exams of mine! ( O - O)

---

## Troubleshooting

**This project was made rather fast by my standards due to this thing called
"Computer Addiction and writing in Helix." Due to these factors, the code is
bound to break. I would lovingly accept any Contributions, see
[CONTRIBUTING.md](CONTRIBUTING.md), or even a trivial github issue.**

_Anyway_,

<details>
<summary><b>Missing yaml-cpp headers</b></summary>
<br/>
Install the development package for your distribution:
<ul>
  <li><b>Debian/Ubuntu:</b> <code>sudo apt-get install libyaml-cpp-dev</code></li>
  <li><b>Fedora/RHEL:</b> <code>sudo dnf install yaml-cpp-devel</code></li>
  <li><b>Arch:</b> <code>sudo pacman -S yaml-cpp</code></li>
</ul>
</details>

<details>
<summary><b>Missing libssh headers</b></summary>
<br/>
<ul>
  <li><b>Debian/Ubuntu:</b> <code>sudo apt-get install libssh-dev</code></li>
  <li><b>Fedora/RHEL:</b> <code>sudo dnf install libssh-devel</code></li>
  <li><b>Arch:</b> <code>sudo pacman -S libssh</code></li>
</ul>
</details>

<details>
<summary><b>Permission denied when saving</b></summary>
<br/>
Certamen needs write access to the YAML file and its parent directory. Check ownership and permissions (varies based on OS).
</details>

<details>
<summary><b>YAML parse errors</b></summary>
<br/>
The file must be a YAML sequence of maps. Required keys per entry: <code>question</code>, <code>choices</code>, <code>answer</code>. Optional: <code>code</code>, <code>explain</code>, <code>language</code>. See `example_quiz.yaml` for more details.
</details>

---

## Credits

Written by [trintlermint](https://github.com/trintlermint).

Certamen is built on:

- [FTXUI](https://github.com/ArthurSonzogni/FTXUI) by Arthur Sonzogni, terminal
  UI framework
- [yaml-cpp](https://github.com/jbeder/yaml-cpp) by Jesse Beder, YAML parser
- [libssh](https://www.libssh.org/), SSH protocol implementation (THANK YOU
  DOCUMENTATION)

A big thank you to all frameworks used, and my friends for emotional support and
motivation, specifically [@valyntyler](https://github.com/valyntyler)

The name **Certamen** is Latin for "contest" so I thought "yeah! this works!"
thank you [Wikipedia](https://en.wikipedia.org/wiki/Certamen).

---

## License

[<img src="./assets/brainmade-org.png" alt="brainmade.org">](https://brainmade.org)
MIT; See [LICENSE.md](LICENSE). Copyright 2026 Niladri Adhikary.
