# Infineon Radar AIDL HAL

This is a hardware abstraction layer (HAL) for accessing Infineon Xensiv radars within android OS.

This HAL uses AIDL only, hence it is a "binderized" HAL.
That is, there is a HAL "server" which loads the HAL implementation and makes it available to the "client" (usually a Java system service) via `/dev/binder`.

Such HAL "servers" are system services and can be listed using `service list`.
In our case "server" is called `vendor.infineon.radar.IRadarSdk/default`, it is what we start in `service.cpp` with an NDK function `AServiceManager_addService()`.
Hence you can check if it is running via:

```bash
$ adb shell service list | grep infineon
```

There is more information in `dumpsys`:

```bash
$ adb shell dumpsys vendor.infineon.radar.IRadarSdk/default
```

## How to build

Refer to https://github.com/Paradox-Cat-GmbH/aosp.local-manifests.paradoxcat-infineon-radar

## How to run development version

You do not have to flash the whole vendor image every time you change the HAL service. It is enough to:

```bash
$ m vendor.infineon.radar@1.0-service
$ adb sync vendor
$ adb shell killall vendor.infineon.radar@1.0-service
```

The HAL should restart.

## How to run tests

Make sure target device is discoverable via `adb devices`, then execute this:

```bash
$ atest VtsHalRadarTest
```

NOTE: this does not always work via wireless `adb` (device cannot be re-attached). When testing with sensor connected to USB, execute it manually instead:

```bash
$ adb connect IP:PORT
$ adb root # it will lose connection and hang. Simply Ctrl+C it and reconnect to new port
$ adb remount
$ adb sync data
$ adb shell /data/nativetest64/VtsHalRadarTest/VtsHalRadarTest
```

NOTE: currently VTS tests stdout and stderr are redirected to logcat. It is easier to debug that way because one sees HAL logs interlaced with VTS logs.
