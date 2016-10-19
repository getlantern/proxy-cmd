# sysproxy-cmd

A command line tool to change HTTP(s) proxy settings of operation system.

Binaries included in repo. Simply `make` to build it again.

Note - you will need to run make separately on each platform.

# Usage

```sh
sysproxy on <proxy host> <proxy port>
sysproxy off <proxy host> <proxy port>
```

`sysproxy off` will turn off proxy setting only if the existing proxy host and \
port equal <proxy host> and <proxy port>.

#Notes

*  **Mac**

Setting the proxy is a privileged action on Mac OS. `sudo` or elevate it as
below.

There's an additional option to chown itself to root:wheel and add setuid bit.

```sh
sysproxy setuid
```

*  **Windows**

Install [MinGW-W64](http://sourceforge.net/projects/mingw-w64) to build
sysproxy, as it has up to date SDK headers we require.

*  **Linux**

`sudo apt-get install libgtk2.0-dev`
