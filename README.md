# PSVita-StorageMgr
kernel plugin that automatically mounts/redirects any storage device on any mount points you want


## Credits:

CelesteBlue for the rewriting, fixes, tests (I spent about 30hours on this project)

gamesd by motoharu / xyz

usbmc by yifanlu / TheFloW

VitaShell kernel plugin by TheFloW



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


## What is that ?

StorageMgrKernel is a kernel plugin that automatically mounts/redirects any storage device on any mount points you want.

What are its advantages ?

- It is configurable through a config file (an app could be made)
- It is a ALL-IN-ONE Storage driver as it replaces usbmc.skprx and gamesd.skprx
- It fixes many issues in previous drivers
- It works well with taiHENkaku or enso
- When SD2VITA is removed or not working and configured to be redirected to ux0, memcard/internal is by default mounted to ux0

What to improve then ?

- Add 3.65-3.67 support: DONE thanks to TheFloW
- Add more mount points: sd0: for exemple and adjust missing mount points for some devices
- Add more exports (currently only ux0/uma0)
- Include in VitaShell once exports are ready
- Fix suspend/resume occuring once per hour when using SD2VITA
- Find a better way to refresh taiHENkaku bootstrap.self than suspending PSVita (happens when using taiHENkaku and autoremounting ux0)
- Create a user app for installation + configuration (+ real time mounting if VitaShell has not all mount points)
- Create a user library
- Detect a key that being pressed would load an alternative config
- Add multi-configs support
- Create a taihen-free version for @SKGleba and other "early boot testers"
- more...

## Usage:

0) If it exists, remove gamesd.skprx and usbmc.skprx or any other storage plugin

1) Copy storage_config.txt to ur0:tai/.

2) Copy storagemgr.skprx to ur0:tai/.

3) If it exists, copy ux0:tai/config.txt to ur0:tai/config.txt

4) If it exists, remove ux0:tai/config.txt.

5) In ur0:tai/config.txt after *KERNEL create a new line and write:
ur0:tai/storagemgr.skprx

6) Configure ur0:tai/storage_config.txt to what you want.

7) Reboot PSVita.

## Exemple:
![exemple_config](https://user-images.githubusercontent.com/20444249/37112629-46eb83dc-2243-11e8-8aae-c6ff36478c0a.jpg)
![exemple_vitashell](https://user-images.githubusercontent.com/20444249/37112630-4712d5f4-2243-11e8-9da9-29d1750d8767.png)


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
