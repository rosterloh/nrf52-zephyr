# NRF52-DK Zephyr Application

## Setup
* Install latest SDK from [Zephyr Project](https://www.zephyrproject.org/downloads)

## Build
```
$ source zephyr-env.sh
$ make
```

## Flashing

1. Erase
```
$ pyocd-flashtool -d debug -t nrf52 -ce
```
2. Flash
```
$ pyocd-flashtool -d debug -t nrf52 outdir/nrf52_pca10040/zephyr.hex
```


## Troubleshooting

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info
