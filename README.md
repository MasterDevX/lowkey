## lowkey
This tool is designed to let you launch your games in a separate X server to achieve the maximum performance, while keeping the ability take screenshots and control in-game volume.

## Why?
> Running a second X server has multiple advantages such as better performance, the ability to "tab" out of your game by using `Ctrl+Alt+F7 / Ctrl+Alt+F8`, no crashing your primary
> X session (which may have open work on) in case a game conflicts with the graphics driver. - [Arch Wiki](https://wiki.archlinux.org/title/Gaming#Starting_games_in_a_separate_X_server)

So basically by starting a game in a separate X server, the game itself is the only graphical application running inside the X server, thus it is being run omitting your desktop
environment as well as its compositor, wich means potentially less overhead and better overall performance.

The problem with such approach is that because there will be no other apps running inside the newly started X server except the actual game, you will be unable to do any interactions
with your system such as taking the in-game screenshots or adjusting volume. **This is not a problem anymore wth lowkey**.

## Features
For now, lowkey has pretty modest yet useful set of features:
- Adjusting in-game volume by pressing `Volume Up` / `Volume Down`
- Taking in-game screenshots by pressing `Prtsc`
- Screenshot path / name fromatting with `strftime`

## Compilation
The project's compilation process is pretty simple:
```
git clone https://github.com/MasterDevX/lowkey.git
cd lowkey
make
```

## Usage
The launcher takes two environments variables that should be necessarily set in order for lowkey to run:
- `LOWKEY_IMGPATH` - a strftime-formatted path for saving screenshots. Visit [strftime.net](https://strftime.net) to easily build your own string.
- `LOWKEY_EXECCMD` - a command to run in a newly started X server.

Lowkey can only be started using `xinit`, which means the package containing `xinit` binary should be present in your system. If you don't have one installed yet, refer to your
distro's package database to find the name of the package you need to install.

### The conception of using xinit
On Linux, `xinit` can be used to run a graphical application in a separate X server. The general use of it assumes the following command syntax:
```
xinit /path/to/app -- :X vtY
```
Where `X` is a display number and `Y` is a virtual terminal (VT) number.

The simplest way to come up with the correct `X` value is to run `echo $DISPLAY` to see the number of current display and let `X` be the number that is `+1` bigger than the number
of your current display (since the current one is already being used by your desktop environment). For example, if it says `:1`, you're fine to go with `:2`.

To choose the `Y` value, you can cycle through the virtual terminals of your system by pressing `Ctrl+Alt+F1` ... `Ctrl+Alt+F12` and choose the one where you see a login prompt
with a blinking cursor. For example, if you see this after pressing `Ctrl+Alt+F4`, you take `4` as an `Y` value. You can then continue cycling through VTs to find out which
one is being used by your desktop environment, so you know how to easily switch between your DE and a game ran by lowkey in the future.

Summing up, the `xinit` usage based on the examples above should look like this:
```
xinit /path/to/app -- :2 vt4
```

### Actually using lowkey
After getting acquainted with lowkey's environment variables and the basic usage of `xinit`, it should not take much time for you to realize how to combine these two to
run your game with lowkey. Here are a few examples for different cases.

- **Example use with native games.** Runs SuperTuxKart with no extra variables set.
```
LOWKEY_IMGPATH="/home/user/Pictures/%Y-%m-%d-%H-%M-%S.png" LOWKEY_EXECCMD="supertuxkart" xinit /path/to/lowkey -- :2 vt4
```
- **Example use with Wine.** Runs Cyberpunk 2077 with some Wine-related variables.
```
LOWKEY_IMGPATH="/home/user/Pictures/%Y-%m-%d-%H-%M-%S.png" LOWKEY_EXECCMD="wine Cyberpunk2077.exe" WINEPREFIX="/home/user/.myprefix" WINEFSYNC=1 xinit /path/to/lowkey -- :2 vt4
```
- **Example use with Steam.** Runs selected game with MangoHud and some Proton-related variables (add this command to the game's launch options).
```
LOWKEY_IMGPATH="/home/user/Pictures/%Y-%m-%d-%H-%M-%S.png" LOWKEY_EXECCMD="%command%" MANGOHUD=1 PROTON_ENABLE_NVAPI=1 xinit /path/to/lowkey -- :2 vt4
```

As you can see, you can add any environment variables you need before `xinit` and they will be exposed to the process ran by lowkey.
