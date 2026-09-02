# KSU EVT Teensy firmware

Firmware for the KSU Electric Vehicle Team Teensy 4.1 controller. The project
uses Arduino through PlatformIO and includes FreeRTOS, SBUS, and vehicle-control
interfaces.

A successful build proves that the firmware compiles. It does not prove that a
controller or vehicle is safe to operate.

## First-time setup

1. Install Git and VS Code.
2. Open this repository in VS Code.
3. Install the recommended extensions when VS Code prompts you.
4. Install Doxygen. The current PlatformIO build generates documentation after
   compiling and expects the `doxygen` command to be available.
5. In VS Code, run `Tasks: Run Task`, then choose **Build firmware**.

Members may use another IDE. The equivalent command is:

```bash
pio run -e teensy41
```

## Contribution path

1. Start from an issue in the
   [Software Project](https://github.com/orgs/KSU-Electric-Vehicle-Team/projects/2).
2. Create a branch such as `42-fix-can-timeout`.
3. Open a draft pull request early if you need help.
4. Run the build and fix the `quality` check before requesting review.
5. Either Evan or Lilia must approve the finished pull request.

See the organization [start guide](https://github.com/KSU-Electric-Vehicle-Team/.github/blob/main/START-HERE.md)
for the full beginner workflow.

## Hardware commands

The serial monitor is available as a VS Code task or through:

```bash
pio device monitor -b 115200
```

Uploading firmware changes physical hardware behavior. Only upload under the
test procedure and supervision named in the issue. The repository does not run
upload commands in CI or automatic VS Code tasks.

