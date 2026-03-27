<div align="center">
  
  # CERTAMEN
  <p>
  <img src="assets/1.png" width="32%" alt="Demo 1">
  <img src="assets/2.png" width="32%" alt="Demo 2">
  <img src="assets/3.png" width="32%" alt="Demo 3">
  </p>
  
  A *Terminal User Interface* **Quiz Game Engine** written entirely in C++.

  Made with love by [trintlermint](#credits).
  
  ## Author *Quizzes* and seamlessly "test" yourself in a full-screen TUI; host the game over SSH for you and your friends to play!
  > This way, you get to make fun of their... incredibly (lacking) haskell knowledge for example!
  ### Releases
  [![Release](https://img.shields.io/github/v/release/trintlermint/certamen?label=linux%20compatible&color=success)](https://github.com/trintlermint/certamen/releases/latest)
  [![Release](https://img.shields.io/github/v/release/trintlermint/certamen?label=macOS%20compatible&color=success)](https://github.com/trintlermint/certamen/releases/latest)
  [![Windows CI](https://img.shields.io/badge/windows-CI%20failing%20(incompatible)-red)](https://github.com/trintlermint/certamen/actions)
  ### Notes
  [![Dependencies](https://img.shields.io/badge/dependencies-FTXUI%20%7C%20yaml--cpp%20%7C%20libssh-informational)](#credits)
  [![SSH Compatible](https://img.shields.io/badge/SSH-compatible-brightgreen)](#ssh-server-mode)
  [![License: MIT](https://img.shields.io/badge/license-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
  ### One might ask: I'm passing everything, why do I need this?
  
  ## Charlie Brown aces his tests with this!
  
  <img src="./assets/peanuts-quiz.png" alt="certamen-banner"/>
  <br />
</div>

## Table of Contents

- [Building](#building)
  - [Dependencies](#dependencies)
  - [CMake (recommended)](#cmake-recommended)
  - [Manual compilation](#manual-compilation-cli)
  - [macOS and Windows](#macos-and-windows)
- [Usage](#usage)
  - [Offline user mode](#offline-user-mode)
  - [SSH server mode](#ssh-server-mode)
- [Quiz format](#quiz-format)
- [Troubleshooting](#troubleshooting)
- [Credits](#credits)
- [License](#license)

---

## Building

### Releases
I have now setup Github Releases through Actions, if you want to skip the headache of building the program, feel free to head over to [**releases**](https://github.com/trintlermint/certamen/releases/latest).

### Dependencies

Certamen uses these predominant three libraries:

| Library | Version | Install |
|---------|---------|---------|
| [FTXUI](https://github.com/ArthurSonzogni/FTXUI) | v6.1.9 | Fetched automatically by CMake via `FetchContent` |
| [yaml-cpp](https://github.com/jbeder/yaml-cpp) | >= 0.7 | System package required |
| [libssh](https://www.libssh.org/) | >= 0.9 | System package required |

You also need a **C++17 compiler (GCC >= 8 or Clang >= 7)** and **CMake >= 3.14**.

### CMake (recommended)

**Ubuntu / Deb:**

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

> Or any other equivalent function to install these packages on your machine.

> [macOS and Windows](#macos-and-windows) explained further below.

Next, clone and build:

```bash
git clone https://github.com/trintlermint/certamen.git
cd certamen
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The binary compiles into an executable, stored at `build/bin/certamen`. Run:

```bash
./build/bin/certamen ./example_quiz.yaml
```

Without arguments it looks for `./quiz.yaml` in the current working directory being run from.

### Manual compilation (CLI)

If yaml-cpp are installed system-wide and you want to skip CMake entirely, you can compile the CLI, the CLI is the previously, originally known "quizzer" app; faithfully renamed.

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 main.cpp -lyaml-cpp -o certamen
```

> This builds a local-only binary without the TUI or SSH server. Use the CMake build for the included features.

### macOS and Windows

**macOS** (using Homebrew):

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

> REMARK: I am NOT a macOS OR Windows user; Manuals told me this should work, if it does or doesn't please inform me on Github Issues! See [CONTRIBUTING.md](CONTRIBUTING.md)

---

## Usage

### Offline User Mode

```bash
certamen quiz.yaml
```

The TUI presents a menu with keyboard (j/k), (up/down/left/right) and number-key navigation:

1. *Take Quiz*; answer questions (`bool: randomiser`), get scored at the end
2. *Add Question*; compose a question with choices, optional code snippet, and explanation
3. *Remove Question*; delete a question
4. *Change Answer*; update the correct answer for an existing question
5. *Edit Choice*; update the text contents of a selected question's choice.
6. *List Questions*; browse all questions with toggleable answers, code, and explanations
7. *Set Author and Name*; set metadata describing the author/name.
8. *Save and Exit*; write changes back to the open YAML file.
9. *Quit without Saving*; write no changes back, quit the program.

> Press `R` on the main menu to toggle randomized question *and* answer order.
> REMARK: The mouse functionality is WIP.

### SSH server mode

Host a quiz for others to connect to with any SSH client:

```bash
certamen serve quiz.yaml                                  # default
certamen serve --password mysecret --port 2222 quiz.yaml  # port and pass
certamen serve --port 3000 algebra.yaml history.yaml      # port, max client
```

Players connect with `ssh -p <port> <name>@<host>` and get the same TUI in an *isolated* "quiz" session. The server logs scores per player.

| Flag | Default | Desc |
|------|---------|-------------|
| `--port <N>` | 2222 | TCP listen port |
| `--password <pw>` | *(open)* | Require password authentication |
| `--key <path>` | `certamen_host_rsa` | RSA host key (auto gen on first run) |
| `--max-clients <N>` | 8 | Concurrent connection limit |

> Full server shell documentation: **[SERVING.md](SERVING.md)**.

---

## Quiz format

Quizzes are YAML files. Each question is a map in a top-to-down sequence which is nested inside `questions`:

```yaml
- question: "What is the time complexity of binary search?"
  choices:
    - "O(n)"
    - "O(log n)"
    - "O(n log n)"
    - "O(1)"
  answer: 1
  code: |
    int bsearch(int* a, int n, int target) {
        int lo = 0, hi = n - 1;
        while (lo <= hi) {
            int mid = (lo + hi) / 2;
            if (a[mid] == target) return mid;
            else if (a[mid] < target) lo = mid + 1;
            else hi = mid - 1;
        }
        return -1;
    }
  explain: |
    Each iteration halves the search space.
    After k iterations the range is n/2^k, which reaches 1 when k = log2(n).
```

- `question`, `choices` >= 2, and `answer` are required. `code` and `explain` are optional.
- `answer` is a *0-based* index into `choices`. The TUI displays 1-based numbering to the player.
- `language` is an optional field for syntax highlight hinting (e.g. `language: haskell`) (see `syntax.cpp` for what has been implemented thus far).

> See [`example_quiz.yaml`](example_quiz.yaml) for a working template and helping me stop writing stuff.

---

## Troubleshooting

**This project was made rather fast by my standards due to this thing called "Computer Addiction and writing in Helix." Due to these factors, the code is bound to break. I would lovingly accept any Contributions, see [CONTRIBUTING.md](CONTRIBUTING.md), or even a trivial github issue.**

*Anyway*,

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

- [FTXUI](https://github.com/ArthurSonzogni/FTXUI) by Arthur Sonzogni, terminal UI framework
- [yaml-cpp](https://github.com/jbeder/yaml-cpp) by Jesse Beder, YAML parser
- [libssh](https://www.libssh.org/),  SSH protocol implementation (THANK YOU DOCUMENTATION)

A big thank you to all frameworks used, and my friends for emotional support and motivation, specifically [@valyntyler](https://github.com/valyntyler)

The name **Certamen** is Latin for "contest" so I thought "yeah! this works!" thank you [Wikipedia](https://en.wikipedia.org/wiki/Certamen)

---

## License

[<img src="./assets/brainmade-org.png" alt="brainmade.org">](https://brainmade.org)
MIT; See [LICENSE.md](LICENSE.md).
Copyright 2026 Niladri Adhikary.
