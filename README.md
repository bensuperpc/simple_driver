# simple_kernel_driver

A simple kernel driver for Linux.

## Requirements

- Linux kernel headers
- GCC
- Make

## Build

```sh
make
```

## Install (Load)

```sh
sudo make load
```

## Usage

### Enter sudo mode

```sh
su
```

### Print device output (In sudo mode)

```sh
cat /dev/device_driver0
cat /dev/device_driver1
```

### Write to device (In sudo mode)

```sh
echo "Hello World" > /dev/device_driver0
echo "Hello World" > /dev/device_driver1
```

## Uninstall (Unload)

```sh
sudo make unload
```

## Clean

```sh
make clean
```


## License

[GPLv3](LICENSE)

## Source

- [linux-kernel-labs](https://linux-kernel-labs.github.io/refs/heads/master/labs/device_drivers.html)
- [embeddedguruji](http://embeddedguruji.blogspot.com/2019/01/linux-character-driver-creating.html)
- [apriorit](https://www.apriorit.com/dev-blog/195-simple-driver-for-linux-os)
- [sysprog21](https://sysprog21.github.io/lkmpg/)
