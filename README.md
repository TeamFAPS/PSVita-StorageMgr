[![Travis Build Status](https://travis-ci.org/celesteblue-dev/vitasdk-docker-testapp-trevis.svg?branch=master)](https://travis-ci.org/celesteblue-dev/vitasdk-docker-testapp-trevis)

# PSVita-StorageMgr
kernel plugin that automatically mounts/redirects any storage device on any mount points you want


## Credits
* CelesteBlue for the rewriting, fixes, tests (I spent about 40 hours on this project)
* gamesd by motoharu / xyz
* usbmc by yifanlu / TheFloW
* VitaShell kernel plugin by TheFloW

## What is that ?
StorageMgrKernel is a kernel plugin that automatically mounts/redirects any storage device on any mount points you want.

What are its advantages ?
- It is configurable through a config file (an app could be made)
- It is a ALL-IN-ONE Storage driver as it replaces usbmc.skprx and gamesd.skprx
- It fixes many issues in previous drivers
- Fixed sporadic wakeups when using SD2VITA thanks to xyz
- It works with taiHEN under henkaku, h-encore or enso
- When SD2VITA is removed or not working and configured to be redirected to ux0, memcard/internal is by default mounted to ux0
- Compatible with 3.60, 3.65, 3.67 and 3.68

What to improve then ?
- Add 3.65-3.67-3.68 support: DONE thanks to TheFloW but maybe it would be better to autoresolve imports by .yml than by get_module_export function or in future create a tool to batch resolve
- Add more mount points: sd0 for example and adjust missing mount points for some devices
- Add more exports (currently only ux0/uma0)
- Include in VitaShell once exports are ready
- Create a user app for installation + configuration (+ real time mounting if VitaShell has not all mount points)
- Create a user library
- Detect a key that being pressed would load an alternative config
- Add multi-configs support
- Create a taihen-free version for @SKGleba and other "early boot testers"
- more...

## Usage
NOTE: for using USB mass on h-encore, you have to boot PSTV, plug out USB mass, plug in USB mass then launch h-encore.

NOTE: For mounting a device as ux0:, this device must have already been mounted as ux0: using VitaShell at least once to have necessary files on it.

0. If it exists, remove gamesd.skprx and usbmc.skprx or any other storage plugin
1. Copy storage_config.txt to ur0:tai/.
2. Copy storagemgr.skprx to ur0:tai/.
3. If it exists, copy ux0:tai/config.txt to ur0:tai/config.txt
4. If it exists, remove ux0:tai/config.txt.
5. In ur0:tai/config.txt after *KERNEL create a new line and write: ur0:tai/storagemgr.skprx
6. Configure ur0:tai/storage_config.txt to what you want.
7. Reboot PSVita.

## Example
![example_config](https://user-images.githubusercontent.com/20444249/37112629-46eb83dc-2243-11e8-8aae-c6ff36478c0a.jpg)
![example_vitashell](https://user-images.githubusercontent.com/20444249/37112630-4712d5f4-2243-11e8-9da9-29d1750d8767.png)


## How to configure StorageMgr ?

On each line you have to write following this structure :
	<device>=<mount_point>

The available devices are :
- MCD : official SONY memory card
- INT : internal memory 1GB (on all PSVita SLIM and PSTV)
- GCD : microSD inserted into SD2VITA (also called gamecard2sd)
- UMA : USB mass (for PSTV) OR microSD inserted into PSVSD (for PSVita 3G)

The available mount points are :
- ux0
- xmc0
- imc0
- uma0
- grw0

## License

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
