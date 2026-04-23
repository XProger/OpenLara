## PORT FOR TRIMUI SMART PRO

https://trimui.com/pages/trimui-smart-pro

* Copied from **sdl2**

### Building

* Get Trimui-SDK build system: https://github.com/Maxwell-SS/trimui-smart-pro-build-system 

```bash
git clone https://github.com/Maxwell-SS/trimui-smart-pro-build-system.git
cd trimui-smart-pro-build-system
```

* Build and deploy docker image in src/platform/trimuismartpro

```bash
docker build -t trimui-sdk .
docker run -it --rm -v $(pwd):/app trimui-sdk bash
```

* Build OpenLara:

```bash
make
```

* Deploy OpenLara to Trimui Smart Pro

    * After building, the complete application will be in `build/OpenLara`. Copy the entire app folder to the `/SDCARD/Apps` folder on your Trimui Smart Pro.


    * Copy game data to `/SDCARD/Roms/OPENLARA` folder. You can change this path in launch.sh

        ```bash
        ./OpenLara -d <path>
        ```
