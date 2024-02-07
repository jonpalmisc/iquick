# iQuick

iQuick is a small utility for interacting with iOS devices. Under the hood, it
uses [`libimobiledevice`](https://github.com/libimobiledevice) and essentially
re-implements a lot of the functionality contained in its provided utilities,
but under a single tool with command line flags that I can more easily
remember.

## Features

iQuick can be used to:

- show the ECID of a device;
- show the UDID of a device;
- shut down a device;
- restart a device;
- put a device into recovery mode; and
- take a device out of recovery mode.

See `iquick -h` for a listing of which flags correspond to which actions.

## Dependencies

You will need `libimobiledevice` and `libirecovery` installed. If you are
building on macOS, you can acquire them via [Homebrew](https://brew.sh):

```sh
$ brew install libimobiledevice libirecovery
```

## Building

With these dependencies installed, simply running `make` should be sufficient
to build `iquick`. You may have to set `CFLAGS` and `LDFLAGS` in the
environment so that `make` can find Homebrew-installed libraries.

Upon a successful build, the `iquick` executable will be available in the
project's root directory.

## Installing

If you would like to install iQuick system-wide, you can run `make install`,
optionally setting the `PREFIX` variable first to adjust the install location.

## License

Licensed under the [BSD 3-Clause](LICENSE.txt) license.
