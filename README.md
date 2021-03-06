# meshctld

A Bluetooth mesh gateway daemon based on meshctl of bluez@5.50

## Setup on Raspberry Pi 3

### Install Dependencies

#### Install Dependencies for BlueZ

```bash
$ sudo apt-get install autoconf libtool libtool-bin libglib2.0-dev libdbus-1-dev libjson-c-dev elfutils libelf-dev libdw-dev udev libudev-dev systemd libsystemd-dev libasound2-dev libical-dev libreadline-dev libsbc-dev libspeexdsp-dev automake qt3d5-dev libncurses5-dev pesign
```

#### Install Dependencies for meshctld

```bash
$ sudo apt-get install gtk+-2.0 libgtk2.0-dev
```

#### Install json-c

```bash
$ cd ~
$ wget https://s3.amazonaws.com/json-c_releases/releases/json-c-0.13.tar.gz
$ tar -xvf json-c-0.13.tar.gz
$ cd json-c-0.13/
$ ./configure --prefix=/usr --disable-static && make
$ sudo make install
```
#### Install ell for BlueZ v5.50

```bash
$ cd ~
$ wget https://mirrors.edge.kernel.org/pub/linux/libs/ell/ell-0.6.tar.xz
$ tar -xvf ell-0.6.tar.xz
$ cd ell-0.6/
$ ./configure --prefix=/usr
$ make
$ sudo make install
```
### Get source code

```bash
$ cd ~
$ git clone https://github.com/ahnniu/meshctld.git
$ cd meshctld
$ git checkout meshctld
```
### Compile

```bash
$ ./bootstrap
$ ./configure --enable-mesh --prefix=/usr --mandir=/usr/share/man --sysconfdir=/etc --localstatedir=/var
$ make
```

### Install

TBD

## How to use

TBD