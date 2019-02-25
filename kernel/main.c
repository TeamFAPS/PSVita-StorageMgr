/*
	StorageMgrKernel by CelesteBlue
	
	Credits:
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
*/

#include <stdio.h>
#include <string.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/cpu.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/io/stat.h>

#include <taihen.h>

#define VERSION_STRING "v3.0"

static void log_write(const char *buffer, size_t length, const char *folderpath, const char *fullpath);
const char *log_folder_ur0_path = "ur0:tai/";
const char *log_ur0_path = "ur0:tai/storagemgr_log.txt";

#define LOG(...) \
	do { \
		char buffer[256]; \
		snprintf(buffer, sizeof(buffer), ##__VA_ARGS__); \
		log_write(buffer, strlen(buffer), log_folder_ur0_path, log_ur0_path); \
	} while (0)



static int (* ksceSysrootGetSystemSwVersion)(void) = NULL;


int module_get_export_func(SceUID pid, const char *modname, uint32_t libnid, uint32_t funcnid, uintptr_t *func);
int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);
const char* default_config_path = "ur0:tai/storage_config.txt";

typedef struct {
	const char *dev;
	const char *dev2;
	const char *blkdev;
	const char *blkdev2;
	int id;
} SceIoDevice;

typedef struct {
	int id;
	const char *dev_unix;
	int unk;
	int dev_major;
	int dev_minor;
	const char *dev_filesystem;
	int unk2;
	SceIoDevice *dev;
	int unk3;
	SceIoDevice *dev2;
	int unk4;
	int unk5;
	int unk6;
	int unk7;
} SceIoMountPoint;

#define SD0_DEV "sd0:"
#define SD0_DEV2 "exfatsd0"
#define SD0_ID 0x100
#define OS0_DEV "os0:"
#define OS0_DEV2 "exfatos0"
#define OS0_ID 0x200
#define TM0_DEV "tm0:"
#define TM0_DEV2 "exfattm0"
#define TM0_ID 0x500
#define UR0_DEV "ur0:"
#define UR0_DEV2 "exfatur0"
#define UR0_ID 0x600
#define UD0_DEV "ud0:"
#define UD0_DEV2 "exfatud0"
#define UD0_ID 0x700
#define UX0_DEV "ux0:"
#define UX0_DEV2 "exfatux0"
#define UX0_ID 0x800
#define GRO0_DEV "gro0:"
#define GRO0_DEV2 "exfatgro0"
#define GRO0_ID 0x900
#define GRW0_DEV "grw0:"
#define GRW0_DEV2 "exfatgrw0"
#define GRW0_ID 0xA00
#define SA0_DEV "sa0:"
#define SA0_DEV2 "exfatsa0"
#define SA0_ID 0xB00
#define PD0_DEV "pd0:"
#define PD0_DEV2 "exfatpd0"
#define PD0_ID 0xC00
#define IMC0_DEV "imc0:"
#define IMC0_DEV2 "exfatimc0"
#define IMC0_ID 0xD00
#define XMC0_DEV "xmc0:"
#define XMC0_DEV2 "exfatxmc0"
#define XMC0_ID 0xE00
#define UMA0_DEV "uma0:"
#define UMA0_DEV2 "exfatuma0"
#define UMA0_ID 0xF00
#define MCD_BLKDEV "sdstor0:xmc-lp-ign-userext"
#define MCD_BLKDEV2 NULL
#define INT_BLKDEV "sdstor0:int-lp-ign-userext"
#define INT_BLKDEV2 NULL
#define UMA_BLKDEV "sdstor0:uma-pp-act-a"
#define UMA_BLKDEV2 "sdstor0:uma-lp-act-entire"
#define GCD_BLKDEV "sdstor0:gcd-lp-ign-entire"
#define GCD_BLKDEV2 NULL

static SceIoMountPoint *(* sceIoFindMountPoint)(int id) = NULL;

#define MOUNT_POINT_ID 0x800
static SceIoDevice uma_ux0_dev = { "ux0:", "exfatux0", "sdstor0:gcd-lp-ign-entire", "sdstor0:gcd-lp-ign-entire", MOUNT_POINT_ID };
static SceIoDevice *ori_dev = NULL, *ori_dev2 = NULL;

static SceIoDevice *tm0_ori_dev = NULL, *tm0_ori_dev2 = NULL;
static SceIoDevice *ur0_ori_dev = NULL, *ur0_ori_dev2 = NULL;
static SceIoDevice *ux0_ori_dev = NULL, *ux0_ori_dev2 = NULL;
static SceIoDevice *gro0_ori_dev = NULL, *gro0_ori_dev2 = NULL;
static SceIoDevice *grw0_ori_dev = NULL, *grw0_ori_dev2 = NULL;
static SceIoDevice *uma0_ori_dev = NULL, *uma0_ori_dev2 = NULL;
static SceIoDevice *imc0_ori_dev = NULL, *imc0_ori_dev2 = NULL;
static SceIoDevice *xmc0_ori_dev = NULL, *xmc0_ori_dev2 = NULL;

static SceIoDevice *tm0_prev_dev = NULL, *tm0_prev_dev2 = NULL;
static SceIoDevice *ur0_prev_dev = NULL, *ur0_prev_dev2 = NULL;
static SceIoDevice *ux0_prev_dev = NULL, *ux0_prev_dev2 = NULL;
static SceIoDevice *gro0_prev_dev = NULL, *gro0_prev_dev2 = NULL;
static SceIoDevice *grw0_prev_dev = NULL, *grw0_prev_dev2 = NULL;
static SceIoDevice *uma0_prev_dev = NULL, *uma0_prev_dev2 = NULL;
static SceIoDevice *imc0_prev_dev = NULL, *imc0_prev_dev2 = NULL;
static SceIoDevice *xmc0_prev_dev = NULL, *xmc0_prev_dev2 = NULL;


int getFileSize(const char *file) {
	SceUID fd = ksceIoOpen(file, SCE_O_RDONLY, 0);
	if (fd < 0)
		return fd;
	int fileSize = ksceIoLseek(fd, 0, SCE_SEEK_END);
	ksceIoClose(fd);
	return fileSize;
}

int checkConfigLineReturnChar(const char *file) {
	int configFileSize = getFileSize(file);
	if (configFileSize < 0) {
		LOG("No config file found.\n");
		return -1;
	}
	char buffer[configFileSize];
	SceUID fd = ksceIoOpen(file, SCE_O_RDWR | SCE_O_APPEND, 0);
	if (fd < 0)
		return fd;
	ksceIoRead(fd, buffer, configFileSize);
	if (memcmp((char[1]){buffer[configFileSize-1]}, (char[2]){0x0A}, 1) != 0)
		ksceIoWrite(fd, (char[2]){'\n', '\0'}, 1);
	ksceIoClose(fd);
	return 0;
}

int config_read(const char *file) {
	LOG("Reading config...\n");
	int entriesNumber = 0;
	int configFileSize = getFileSize(file);
	if (configFileSize < 0) {
		LOG("No config file found.\n");
		return -1;
	}
	char buffer[configFileSize];
	SceUID fd = ksceIoOpen(file, SCE_O_RDONLY, 0);
	if (fd < 0)
		return fd;
	ksceIoRead(fd, buffer, configFileSize);
	for (int i=0; i<configFileSize; i++) {
		if (memcmp((char[1]){buffer[i]}, (char[1]){0x0A}, 1) == 0)
			entriesNumber++;
	}
	ksceIoClose(fd);
	return entriesNumber;
}

int readLine(int lineId, char *lineBuffer) {
	//LOG("Reading line...\n");
	int entriesNumber = config_read(default_config_path);
	int configFileSize = getFileSize(default_config_path);
	if (configFileSize < 0) {
		LOG("No config file found.\n");
		return -1;
	}
	char configBuffer[configFileSize];
	SceUID fd = ksceIoOpen(default_config_path, SCE_O_RDONLY, 0);
	if (fd < 0)
		return fd;
	ksceIoRead(fd, configBuffer, configFileSize);
	int padding = 0, n = 0;
	for (int currentLine=0; currentLine<entriesNumber; currentLine++) {
		if (currentLine == lineId) {
			//LOG("Reading line %i with padding %i...\n", currentLine, padding);
			for (n=0; memcmp((char[1]){configBuffer[padding+n]}, (char[1]){0x0A}, 1) != 0; n++) {}
			memcpy((void*)lineBuffer, configBuffer+padding, n);
			lineBuffer[n] = 0; // We write the '\0' null character.
			break;
		}
		for (; memcmp((char[1]){configBuffer[padding]}, (char[1]){0x0A}, 1) != 0; padding++) {}
		padding++; // We don't read the '\n' character.
	}
	ksceIoClose(fd);
	LOG("Line of size %i : %s\n", n, lineBuffer);
	return n;
}

int readDeviceByLine(int lineId, char *lineDevice) {
	char lineBuffer[32];
	int lineLength = readLine(lineId, lineBuffer);
	int i = 0;
	for (; memcmp((char[1]){lineBuffer[i]}, (char[1]){0x3D}, 1) != 0; i++) {
		lineDevice[i] = lineBuffer[i];
		if (i == lineLength-1) {
			LOG("Line %i is not valid.\n", lineId);
			return -1;
		}
	}
	lineDevice[i] = 0; // We write the '\0' null character.
	//LOG("Current line device of string length %i : %s\n", i, lineDevice);
	return 0;
}

int isDeviceInConfig(const char *device) {
	int line = -1;
	int entriesNumber = config_read(default_config_path);
	for (int i=0; i<entriesNumber; i++) {
		char lineDevice[32];
		if (!readDeviceByLine(i, lineDevice)) {
			if (!memcmp(lineDevice, device, strlen(device))) {
				line = i;
				break;
			}
		}
	}
	return line;
}

int readMountPointByLine(int lineId, char *lineMountPoint) {
	LOG("Reading mount point for line %i...\n", lineId);
	char lineBuffer[32];
	int lineLength = readLine(lineId, lineBuffer);
	LOG("Line length : %i.\n", lineLength);
	int i = 0;
	for (; memcmp((char[1]){lineBuffer[i]}, (char[1]){0x3D}, 1) != 0; i++) {}
	i++; // We don't read the '=' character.
	LOG("Not read string length (device string) : %i.\n", i);
	int j = 0;
	for (; j<lineLength-i; j++) {
		lineMountPoint[j] = lineBuffer[i+j];
	}
	lineMountPoint[j] = 0; // We write the '\0' null character.
	LOG("Current line mount point of string length %i : %s\n", j, lineMountPoint);
	return 0;
}

int isMountPointInConfig(const char *mountPoint) {
	int line = -1;
	int entriesNumber = config_read(default_config_path);
	for (int i=0; i<entriesNumber; i++) {
		char lineMountPoint[16];
		if (!readMountPointByLine(i, lineMountPoint)) {
			//LOG("Mount point string length : %i.\n", strlen(lineMountPoint));
			if (strlen(mountPoint) == strlen(lineMountPoint)) {
				if (!memcmp(lineMountPoint, mountPoint, strlen(mountPoint))) {
					line = i;
					break;
				}
			}
		}
	}
	return line;
}

int run_on_thread(void *func) {
	int ret = 0;
	int res = 0;
	int uid = 0;

	ret = uid = ksceKernelCreateThread("run_on_thread", func, 64, 0x10000, 0, 0, 0);

	if (ret < 0) {
		LOG("failed to create a thread: 0x%08x\n", ret);
		ret = -1;
		goto cleanup;
	}
	if ((ret = ksceKernelStartThread(uid, 0, NULL)) < 0) {
		LOG("failed to start a thread: 0x%08x\n", ret);
		ret = -1;
		goto cleanup;
	}
	if ((ret = ksceKernelWaitThreadEnd(uid, &res, NULL)) < 0) {
		LOG("failed to wait a thread: 0x%08x\n", ret);
		ret = -1;
		goto cleanup;
	}

	ret = res;

cleanup:
	if (uid > 0)
		ksceKernelDeleteThread(uid);

	return ret;
}

char path_temp_buffer[256];
int exists_on_thread(void) {
	int fd = ksceIoOpen(path_temp_buffer, SCE_O_RDONLY, 0);
	if (fd < 0)
		return 0;
	ksceIoClose(fd);
	return 1;
}
static int exists(const char *path) {
	int state = 0;
	ENTER_SYSCALL(state);
	int result = 0;
	memcpy(path_temp_buffer, path, strlen(path)+1);
	result = run_on_thread(exists_on_thread);
	EXIT_SYSCALL(state);
	return result;
}

int remount_id = 0;
static int io_remount_on_thread(void) {
	int ret = -1;
	ksceIoUmount(remount_id, 0, 0, 0);
	ksceIoUmount(remount_id, 1, 0, 0);
	ret = ksceIoMount(remount_id, NULL, 0, 0, 0, 0);
	return ret;
}
static int io_remount(int id) {
	int state = 0;
	ENTER_SYSCALL(state);
	int result = -1;
	remount_id = id;
	result = run_on_thread(io_remount_on_thread);
	EXIT_SYSCALL(state);
	return result;
}

int shellKernelIsUx0Redirected() {
	SceIoMountPoint *mount = sceIoFindMountPoint(MOUNT_POINT_ID);
	if (!mount)
		return -1;
	if (mount->dev == &uma_ux0_dev && mount->dev2 == &uma_ux0_dev)
		return 1;
	return 0;
}
int shellKernelRedirectUx0() {
	SceIoMountPoint *mount = sceIoFindMountPoint(MOUNT_POINT_ID);
	if (!mount)
		return -1;
	if (mount->dev != &uma_ux0_dev && mount->dev2 != &uma_ux0_dev) {
		ori_dev = mount->dev;
		ori_dev2 = mount->dev2;
	}
	mount->dev = &uma_ux0_dev;
	mount->dev2 = &uma_ux0_dev;
	return 0;
}
int shellKernelUnredirectUx0() {
	LOG("Unredirect ux0 function.\n");
	SceIoMountPoint *mount = sceIoFindMountPoint(MOUNT_POINT_ID);
	if (!mount)
		return -1;
	if (ori_dev && ori_dev2) {
		mount->dev = ori_dev;
		mount->dev2 = ori_dev2;
		ori_dev = NULL;
		ori_dev2 = NULL;
	}
	return 0;
}

/*int shellKernelRedirectGcdToUx0() {
	SceIoMountPoint *mount = sceIoFindMountPoint(UX0_ID);
	if (!mount) return -1;
	static SceIoDevice gcd_ux0 = {UX0_DEV, UX0_DEV2, GCD_BLKDEV, GCD_BLKDEV2, UX0_ID};
	//if (mount->dev != &gcd_ux0 && mount->dev2 != &gcd_ux0) {
		ux0_prev_dev = mount->dev;
		ux0_prev_dev2 = mount->dev2;
	//}
	mount->dev = &gcd_ux0;
	mount->dev2 = &gcd_ux0;
	io_remount(UX0_ID);
	return 0;
}*/

int isDeviceValid(const char* device) {
	if (!memcmp(device, "UMA", strlen("UMA"))
	|| !memcmp(device, "GCD", strlen("GCD"))
	|| !memcmp(device, "INT", strlen("INT"))
	|| !memcmp(device, "MCD", strlen("MCD")))
		return 1;
	LOG("Invalid device : %s\n", device);
	return 0;
}

int isPartitionValid(const char* partition) {
	if (!memcmp(partition, TM0_DEV, strlen(TM0_DEV))
	|| !memcmp(partition, UR0_DEV, strlen(UR0_DEV))
	|| !memcmp(partition, UX0_DEV, strlen(UX0_DEV))
	|| !memcmp(partition, GRO0_DEV, strlen(GRO0_DEV))
	|| !memcmp(partition, GRW0_DEV, strlen(GRW0_DEV))
	|| !memcmp(partition, IMC0_DEV, strlen(IMC0_DEV))
	|| !memcmp(partition, XMC0_DEV, strlen(XMC0_DEV))
	|| !memcmp(partition, UMA0_DEV, strlen(UMA0_DEV)))
		return 1;
	LOG("Invalid partition : %s\n", partition);
	return 0;
}

int getMountPointIdForPartition(const char* partition) {
	if (!isPartitionValid(partition))
		return -1;
	if (!memcmp(partition, "tm0:", strlen("tm0:")))
		return TM0_ID;
	if (!memcmp(partition, "ur0:", strlen("ur0:")))
		return UR0_ID;
	if (!memcmp(partition, "ux0:", strlen("ux0:")))
		return UX0_ID;
	if (!memcmp(partition, "gro0:", strlen("gro0:")))
		return GRO0_ID;
	if (!memcmp(partition, "grw0:", strlen("grw0:")))
		return GRW0_ID;
	if (!memcmp(partition, "imc0:", strlen("imc0:")))
		return IMC0_ID;
	if (!memcmp(partition, "xmc0:", strlen("xmc0:")))
		return XMC0_ID;
	if (!memcmp(partition, "uma0:", strlen("uma0:")))
		return UMA0_ID;
	return 0;
}

int getPartitionForMountPointId(int mount_point_id, char** partition) {
	if (mount_point_id == TM0_ID) {
		*partition = TM0_DEV;
		return 1;
	} else if (mount_point_id == UX0_ID) {
		*partition = UX0_DEV;
		return 1;
	} else if (mount_point_id == UX0_ID) {
		*partition = UX0_DEV;
		return 1;
	} else if (mount_point_id == UMA0_ID) {
		*partition = UMA0_DEV;
		return 1;
	} else if (mount_point_id == IMC0_ID) {
		*partition = IMC0_DEV;
		return 1;
	} else if (mount_point_id == XMC0_ID) {
		*partition = XMC0_DEV;
		return 1;
	}
	return 0;
}

int getBlkdevForDevice(const char* device, char** blkdev, char** blkdev2) {
	if (!memcmp(device, "UMA", strlen("UMA"))) {
		*blkdev = UMA_BLKDEV;
		*blkdev2 = UMA_BLKDEV2;
		return 1;
	} else if (!memcmp(device, "GCD", strlen("GCD"))) {
		*blkdev = GCD_BLKDEV;
		*blkdev2 = GCD_BLKDEV2;
		return 1;
	} else if (!memcmp(device, "MCD", strlen("MCD"))) {
		*blkdev = MCD_BLKDEV;
		*blkdev2 = MCD_BLKDEV2;
		return 1;
	} else if (!memcmp(device, "INT", strlen("INT"))) {
		*blkdev = INT_BLKDEV;
		*blkdev2 = INT_BLKDEV2;
		return 1;
	}
	return 0;
}

int shellKernelGetCurrentBlkdevForMountPointId(int mount_point_id, char** blkdev, char** blkdev2) {
	LOG("Reading current device blkdev for mount point 0x%03X :\n", mount_point_id);
	SceIoMountPoint *mount = sceIoFindMountPoint(mount_point_id);
	if (!mount)
		return -1;
	LOG("%s\n", mount->dev->blkdev);
	if (mount->dev->blkdev != NULL)
		*blkdev = mount->dev->blkdev;
	if (mount->dev->blkdev2 != NULL)
		*blkdev2 = mount->dev->blkdev2;
	return 0;
}

int shellKernelGetOriginalBlkdevForMountPointId(int mount_point_id, char** blkdev, char** blkdev2) {
	if (mount_point_id == UX0_ID) {
		if (ux0_ori_dev->blkdev != NULL)
			*blkdev = ux0_ori_dev->blkdev;
		if (ux0_ori_dev->blkdev2 != NULL)
			*blkdev2 = ux0_ori_dev->blkdev2;
		return 1;
	}
	if (mount_point_id == UMA0_ID) {
		if (uma0_ori_dev->blkdev != NULL)
			*blkdev = uma0_ori_dev->blkdev;
		if (uma0_ori_dev->blkdev2 != NULL)
			*blkdev2 = uma0_ori_dev->blkdev2;
		return 1;
	}
	if (mount_point_id == IMC0_ID) {
		if (imc0_ori_dev->blkdev != NULL)
			*blkdev = imc0_ori_dev->blkdev;
		if (imc0_ori_dev->blkdev2 != NULL)
			*blkdev2 = imc0_ori_dev->blkdev2;
		return 1;
	}
	return 0;
}

int shellKernelIsPartitionRedirected(const char* partition, char** blkdev, char** blkdev2) {
	if (!isPartitionValid(partition))
		return -1;
	int mount_point_id = getMountPointIdForPartition(partition);
	LOG("mount point id : 0x%04X\n", mount_point_id);
	if (!shellKernelGetCurrentBlkdevForMountPointId(mount_point_id, blkdev, blkdev2)) {
		LOG("current blkdev : %s %s\n", *blkdev, *blkdev2);
		SceIoMountPoint *mount = sceIoFindMountPoint(mount_point_id);
		if (!mount) return -1;
		if (!memcmp(partition, UX0_DEV, strlen(UX0_DEV))) {
			if (mount->dev != ux0_ori_dev || mount->dev2 != ux0_ori_dev2)
				return 1;
			else return 0;
		} else if (!memcmp(partition, UMA0_DEV, strlen(UMA0_DEV))) {
			if (mount->dev != uma0_ori_dev || mount->dev2 != uma0_ori_dev2)
				return 1;
			else return 0;
		} else if (!memcmp(partition, GRO0_DEV, strlen(GRO0_DEV))) {
			if (mount->dev != gro0_ori_dev || mount->dev2 != gro0_ori_dev2)
				return 1;
			else return 0;
		} else if (!memcmp(partition, GRW0_DEV, strlen(GRW0_DEV))) {
			if (mount->dev != grw0_ori_dev || mount->dev2 != grw0_ori_dev2)
				return 1;
			else return 0;
		} else if (!memcmp(partition, IMC0_DEV, strlen(IMC0_DEV))) {
			if (mount->dev != imc0_ori_dev || mount->dev2 != imc0_ori_dev2)
				return 1;
			else return 0;
		} else if (!memcmp(partition, XMC0_DEV, strlen(XMC0_DEV))) {
			if (mount->dev != xmc0_ori_dev || mount->dev2 != xmc0_ori_dev2)
				return 1;
			else return 0;
		} else return -1;
	} else return -1;
	return 0;
}

int mountPointIdList[4];
int* isDeviceMounted(const char* blkdev, const char* blkdev2) {
	char* blkdev_local = NULL;
	char* blkdev2_local = NULL;
	int ret = -1;
	int j = 0;
	for (int i=0x100; i<=0xF00; i+=0x100) {
		ret = shellKernelGetCurrentBlkdevForMountPointId(i, &blkdev_local, &blkdev2_local);
		if (ret != -1) {
			LOG("found : %s\n", blkdev_local);
			if (!memcmp(blkdev_local, blkdev, strlen(blkdev))) {
				mountPointIdList[j] = i;
				//memcpy(((uintptr_t)mountPointIdList)+j*sizeof(int), i, sizeof(int));
				j++;
				//LOG("working\n");
			}
		}
	}
	return mountPointIdList;
}

int shellKernelUnredirect(const char* partition, int isReady) {
	LOG("Unredirecting %s\n", partition);
	char* device = NULL;
	char* device2 = NULL;
	if (!isPartitionValid(partition))
		return -1;
	int mount_point_id = getMountPointIdForPartition(partition);
	SceIoMountPoint *mount = sceIoFindMountPoint(mount_point_id);
	if (!mount) return -1;
	LOG("Is ux0: redirected : %08X\n", shellKernelIsPartitionRedirected(partition, &device, &device2));
	LOG("ux0: current device : %s\noriginal device %s\n", device, ux0_ori_dev->blkdev);
	if (shellKernelIsPartitionRedirected(partition, &device, &device2) == 1) { 
		if (!isReady) {
			char* blkdev = NULL;
			char* blkdev2 = NULL;
			shellKernelGetOriginalBlkdevForMountPointId(mount_point_id, &blkdev, &blkdev2);
			int* mount_point_id_s_list = isDeviceMounted(blkdev, blkdev2);
			char *partition_s = NULL;
			for (int i=0; mount_point_id_s_list[i] != 0; i++) {
				LOG("%s is currently mounted on : %04X\n", blkdev, mount_point_id_s_list[i]);
				getPartitionForMountPointId(mount_point_id_s_list[i], &partition_s);
				shellKernelUnredirect(partition_s, 1);
			}
		}
		ux0_prev_dev = mount->dev;
		ux0_prev_dev2 = mount->dev2;
		mount->dev = ux0_ori_dev;
		mount->dev2 = ux0_ori_dev2;
		io_remount(mount_point_id);
		return 1;
	} else {
		LOG("%s is not currently redirected. Aborting unredirection.\n", partition);
		return 0;
	}
	return 0;
}

static SceIoDevice new_dev;
static SceIoDevice ux0_new_dev;
static SceIoDevice gro0_new_dev;
static SceIoDevice grw0_new_dev;
static SceIoDevice imc0_new_dev;
static SceIoDevice xmc0_new_dev;
static SceIoDevice uma0_new_dev;
int shellKernelRedirect(const char* partition, const char* device) {
	if (!isPartitionValid(partition))
		return -1;
	int mount_point_id = getMountPointIdForPartition(partition);
	if (!isDeviceValid(device))
		return -1;
	SceIoMountPoint *mount = sceIoFindMountPoint(mount_point_id);
	if (!mount) return -1;
	if (!memcmp(device, "GCD", strlen("GCD"))) {
		new_dev.blkdev = GCD_BLKDEV;
		new_dev.blkdev2 = GCD_BLKDEV2;
	} else if (!memcmp(device, "UMA", strlen("UMA"))) {
		new_dev.blkdev = UMA_BLKDEV;
		new_dev.blkdev2 = UMA_BLKDEV2;
	} else if (!memcmp(device, "INT", strlen("INT"))) {
		new_dev.blkdev = INT_BLKDEV;
		new_dev.blkdev2 = INT_BLKDEV2;
	} else if (!memcmp(device, "MCD", strlen("MCD"))) {
		new_dev.blkdev = MCD_BLKDEV;
		new_dev.blkdev2 = MCD_BLKDEV2;
	} else return -1;
	if (mount_point_id == UX0_ID) {
		ux0_new_dev.blkdev = new_dev.blkdev;
		ux0_new_dev.blkdev2 = new_dev.blkdev2;
		ux0_new_dev.dev2 = UX0_DEV2;
		ux0_new_dev.dev = UX0_DEV;
		ux0_new_dev.dev2 = UX0_DEV2;
		ux0_new_dev.id = UX0_ID;
		ux0_prev_dev = mount->dev;
		ux0_prev_dev2 = mount->dev2;
		mount->dev = &ux0_new_dev;
		mount->dev2 = &ux0_new_dev;
	} else if (mount_point_id == GRO0_ID) {
		gro0_new_dev.blkdev = new_dev.blkdev;
		gro0_new_dev.blkdev2 = new_dev.blkdev2;
		gro0_new_dev.dev = GRO0_DEV;
		gro0_new_dev.dev2 = GRO0_DEV2;
		gro0_new_dev.id = GRO0_ID;
		gro0_prev_dev = mount->dev;
		gro0_prev_dev2 = mount->dev2;
		mount->dev = &gro0_new_dev;
		mount->dev2 = &gro0_new_dev;
	} else if (mount_point_id == GRW0_ID) {
		grw0_new_dev.blkdev = new_dev.blkdev;
		grw0_new_dev.blkdev2 = new_dev.blkdev2;
		grw0_new_dev.dev = GRW0_DEV;
		grw0_new_dev.dev2 = GRW0_DEV2;
		grw0_new_dev.id = GRW0_ID;
		grw0_prev_dev = mount->dev;
		grw0_prev_dev2 = mount->dev2;
		mount->dev = &grw0_new_dev;
		mount->dev2 = &grw0_new_dev;
	} else if (mount_point_id == IMC0_ID) {
		imc0_new_dev.blkdev = new_dev.blkdev;
		imc0_new_dev.blkdev2 = new_dev.blkdev2;
		imc0_new_dev.dev = IMC0_DEV;
		imc0_new_dev.dev2 = IMC0_DEV2;
		imc0_new_dev.id = IMC0_ID;
		imc0_prev_dev = mount->dev;
		imc0_prev_dev2 = mount->dev2;
		mount->dev = &imc0_new_dev;
		mount->dev2 = &imc0_new_dev;
	} else if (mount_point_id == XMC0_ID) {
		xmc0_new_dev.blkdev = new_dev.blkdev;
		xmc0_new_dev.blkdev2 = new_dev.blkdev2;
		xmc0_new_dev.dev = XMC0_DEV;
		xmc0_new_dev.dev2 = XMC0_DEV2;
		xmc0_new_dev.id = XMC0_ID;
		xmc0_prev_dev = mount->dev;
		xmc0_prev_dev2 = mount->dev2;
		mount->dev = &xmc0_new_dev;
		mount->dev2 = &xmc0_new_dev;
	} else if (mount_point_id == UMA0_ID) {
		uma0_new_dev.blkdev = new_dev.blkdev;
		uma0_new_dev.blkdev2 = new_dev.blkdev2;
		uma0_new_dev.dev = UMA0_DEV;
		uma0_new_dev.dev2 = UMA0_DEV2;
		uma0_new_dev.id = UMA0_ID;
		uma0_prev_dev = mount->dev;
		uma0_prev_dev2 = mount->dev2;
		mount->dev = &uma0_new_dev;
		mount->dev2 = &uma0_new_dev;
	} else return -1;
	LOG("mount is now : %s\n", mount->dev->blkdev);
	io_remount(mount_point_id);
	return 0;
}

int saveOriginalDevicesForMountPoints() {
	SceIoMountPoint *mount = sceIoFindMountPoint(TM0_ID);
	if (!mount) LOG("No tm0: mount point");
	else {
		tm0_ori_dev = mount->dev;
		tm0_ori_dev2 = mount->dev2;
		LOG("tm0 : %s\n%s\n%s\n%s\n%08X\n\n", tm0_ori_dev->dev, tm0_ori_dev->dev2, tm0_ori_dev->blkdev, tm0_ori_dev->blkdev2, tm0_ori_dev->id);
	}
	mount = sceIoFindMountPoint(UR0_ID);
	if (!mount) LOG("No ur0: mount point");
	else {
		ur0_ori_dev = mount->dev;
		ur0_ori_dev2 = mount->dev2;
		LOG("ur0 : %s\n%s\n%s\n%s\n%08X\n\n", ur0_ori_dev->dev, ur0_ori_dev->dev2, ur0_ori_dev->blkdev, ur0_ori_dev->blkdev2, ur0_ori_dev->id);
	}
	mount = sceIoFindMountPoint(UX0_ID);
	if (!mount) LOG("No ux0: mount point");
	else {
		ux0_ori_dev = mount->dev;
		ux0_ori_dev2 = mount->dev2;
		LOG("ux0 : %s\n%s\n%s\n%s\n%08X\n\n", ux0_ori_dev->dev, ux0_ori_dev->dev2, ux0_ori_dev->blkdev, ux0_ori_dev->blkdev2, ux0_ori_dev->id);
	}
	mount = sceIoFindMountPoint(GRO0_ID);
	if (!mount) LOG("No gro0: mount point");
	else {
		gro0_ori_dev = mount->dev;
		gro0_ori_dev2 = mount->dev2;
		LOG("gro0 : %s\n%s\n%s\n%s\n%08X\n\n", gro0_ori_dev->dev, gro0_ori_dev->dev2, gro0_ori_dev->blkdev, gro0_ori_dev->blkdev2, gro0_ori_dev->id);
	}
	mount = sceIoFindMountPoint(GRW0_ID);
	if (!mount) LOG("No grw0: mount point");
	else {
		grw0_ori_dev = mount->dev;
		grw0_ori_dev2 = mount->dev2;
		LOG("grw0 : %s\n%s\n%s\n%s\n%08X\n\n", grw0_ori_dev->dev, grw0_ori_dev->dev2, grw0_ori_dev->blkdev, grw0_ori_dev->blkdev2, grw0_ori_dev->id);
	}
	mount = sceIoFindMountPoint(IMC0_ID);
	if (!mount) LOG("No imc0: mount point");
	else {
		imc0_ori_dev = mount->dev;
		imc0_ori_dev2 = mount->dev2;
		LOG("imc0 : %s\n%s\n%s\n%s\n%08X\n\n", imc0_ori_dev->dev, imc0_ori_dev->dev2, imc0_ori_dev->blkdev, imc0_ori_dev->blkdev2, imc0_ori_dev->id);
	}
	mount = sceIoFindMountPoint(XMC0_ID);
	if (!mount) LOG("No gro0: mount point");
	else {
		xmc0_ori_dev = mount->dev;
		xmc0_ori_dev2 = mount->dev2;
		LOG("xmc0 : %s\n%s\n%s\n%s\n%08X\n\n", xmc0_ori_dev->dev, xmc0_ori_dev->dev2, xmc0_ori_dev->blkdev, xmc0_ori_dev->blkdev2, xmc0_ori_dev->id);
	}
	mount = sceIoFindMountPoint(UMA0_ID);
	if (!mount) LOG("No uma0: mount point");
	else {
		uma0_ori_dev = mount->dev;
		uma0_ori_dev2 = mount->dev2;
		LOG("uma0 : %s\n%s\n%s\n%s\n%08X\n\n", uma0_ori_dev->dev, uma0_ori_dev->dev2, uma0_ori_dev->blkdev, uma0_ori_dev->blkdev2, uma0_ori_dev->id);
	}
	return 0;
}

// allow Memory Card remount, patch by TheFloW
void patch_appmgr() {
	tai_module_info_t sceappmgr_modinfo;
	sceappmgr_modinfo.size = sizeof(tai_module_info_t);
	if (taiGetModuleInfoForKernel(KERNEL_PID, "SceAppMgr", &sceappmgr_modinfo) >= 0) {
		uint32_t nop_nop_opcode = 0xBF00BF00;
		switch (sceappmgr_modinfo.module_nid) {
			case 0xDBB29DB7: // 3.60 retail/testkit/devkit
			case 0x1C9879D6: // 3.65 retail/testkit/devkit
				taiInjectDataForKernel(KERNEL_PID, sceappmgr_modinfo.modid, 0, 0xB338, &nop_nop_opcode, 4);
				taiInjectDataForKernel(KERNEL_PID, sceappmgr_modinfo.modid, 0, 0xB368, &nop_nop_opcode, 2);
				break;
			case 0x54E2E984: // 3.67 retail/testkit/devkit
			case 0xC3C538DE: // 3.68 retail/testkit/devkit
				taiInjectDataForKernel(KERNEL_PID, sceappmgr_modinfo.modid, 0, 0xB344, &nop_nop_opcode, 4);
				taiInjectDataForKernel(KERNEL_PID, sceappmgr_modinfo.modid, 0, 0xB374, &nop_nop_opcode, 2);
				break;
		}
	}
}

tai_hook_ref_t hook_get_partition;
tai_hook_ref_t hook_write;
tai_hook_ref_t hook_mediaid;
int magic = 0x7FFFFFFF;
void *sdstor_mediaid;

void *my_get_partition(const char *name, size_t len) {
	void *ret = TAI_CONTINUE(void*, hook_get_partition, name, len);
	if (!ret && len == 18 && !strcmp(name, "gcd-lp-act-mediaid"))
		return &magic;
	return ret;
}

int my_write(uint8_t *dev, void *buf, int sector, int size) {
	if (dev[36] == 1 && sector == magic)
		return 0;
	return TAI_CONTINUE(int, hook_write, dev, buf, sector, size);
}

int my_mediaid(uint8_t *dev) {
	int ret = TAI_CONTINUE(int, hook_mediaid, dev);
 	if (dev[36] == 1) {
		memset(dev + 20, 0xFF, 16);
		memset(sdstor_mediaid, 0xFF, 16);
 		return magic;
	}
	return ret;
}

// allow microSD cards, patch by motoharu
int GCD_patch_scesdstor() {
	tai_module_info_t scesdstor_modinfo;
	scesdstor_modinfo.size = sizeof(tai_module_info_t);
	if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdstor", &scesdstor_modinfo) >= 0) {
		module_get_offset(KERNEL_PID, scesdstor_modinfo.modid, 1, 0x1720, (uintptr_t *) &sdstor_mediaid);
		// patch for proc_initialize_generic_2 - so that sd card type is not ignored
		char zeroCallOnePatch[4] = {0x01, 0x20, 0x00, 0xBF};
		taiInjectDataForKernel(KERNEL_PID, scesdstor_modinfo.modid, 0, 0x2498, zeroCallOnePatch, 4); // patch (BLX) to (MOVS R0, #1 ; NOP)
		// patch and hooks to bypass sporadic wakeups by xyz
		taiInjectDataForKernel(KERNEL_PID, scesdstor_modinfo.modid, 0, 0x2940, zeroCallOnePatch, 4);
		taiHookFunctionOffsetForKernel(KERNEL_PID, &hook_get_partition, scesdstor_modinfo.modid, 0, 0x142C, 1, my_get_partition);
		taiHookFunctionOffsetForKernel(KERNEL_PID, &hook_write, scesdstor_modinfo.modid, 0, 0x2C58, 1, my_write);
		taiHookFunctionOffsetForKernel(KERNEL_PID, &hook_mediaid, scesdstor_modinfo.modid, 0, 0x3D54, 1, my_mediaid);
	} else return -1;
	return 0;
}

int GCD_poke() {
	tai_module_info_t scesdstor_modinfo;
	scesdstor_modinfo.size = sizeof(tai_module_info_t);
	if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdstor", &scesdstor_modinfo) < 0)
		return -1;

	void *args = 0;
	int (*SceSdstorCardInsert)() = 0;
	int (*SceSdstorCardRemove)() = 0;

	module_get_offset(KERNEL_PID, scesdstor_modinfo.modid, 0, 0x3BD5, (uintptr_t *)&SceSdstorCardInsert);
	module_get_offset(KERNEL_PID, scesdstor_modinfo.modid, 0, 0x3BC9, (uintptr_t *)&SceSdstorCardRemove);
	module_get_offset(KERNEL_PID, scesdstor_modinfo.modid, 1, 0x1B20 + 40 * 1, (uintptr_t *)&args);

	SceSdstorCardRemove(0, args);
	ksceKernelDelayThread(200 * 1000);
	SceSdstorCardInsert(0, args);
	ksceKernelDelayThread(200 * 1000);

	return 0;
}

int uma0_used = 0;

int uma0_suspend_workaround_thread(SceSize args, void *argp) {
	ksceKernelDelayThread(50 * 1000); // this delay is needed else uma0: remount fails
	// wait ~5 seconds max for USB mass to be detected
	// this may look bad but the PSVita does this to detect ux0: so ¯\_(ツ)_/¯
	if (!uma0_used) {
		LOG("uma0: is not used: no need to remount it.\n");
		ksceKernelExitDeleteThread(0);
		return 0;
	}
	int i = 0;
	for (; i <= 250; i++) {
		if (exists(UMA0_DEV)) { // try to detect uma0: 250 times for 0.02s each
			LOG("USB mass detected.\n");
			break;
		} else if (ksceIoMount(UMA0_ID, NULL, 0, 0, 0, 0) == 0) { // try to detect uma0: 25 times for 0.2s each
			LOG("uma0: remounting success.\n");
			break;
		}
		else
			ksceKernelDelayThread(20 * 1000);
	}
	if (i > 250)
		LOG("uma0: remounting failed. Aborting after 5s.\n");
	ksceKernelExitDeleteThread(0);
	return 0;
}

SceUID sub_81000000_patched_hook = -1;
static tai_hook_ref_t sub_81000000_patched_ref;
static SceUID sub_81000000_patched(int resume, int eventid, void *args, void *opt) {
	int ret = TAI_CONTINUE(SceUID, sub_81000000_patched_ref, resume, eventid, args, opt);
	if (eventid == 0x100000) {
		SceUID thid = ksceKernelCreateThread("uma0_suspend_workaround_thread", uma0_suspend_workaround_thread, 0x40, 0x1000, 0, 0, NULL);
		if (thid >= 0)
			ksceKernelStartThread(thid, 0, NULL);
	}
	return ret;
}

int suspend_workaround(void) {
	tai_module_info_t scesblssmgr_modinfo;
	taiGetModuleInfoForKernel(KERNEL_PID, "SceSblSsMgr", &scesblssmgr_modinfo);
	LOG("Installing SceSblSsMgr hook...\n");
	sub_81000000_patched_hook = taiHookFunctionOffsetForKernel(KERNEL_PID, &sub_81000000_patched_ref, scesblssmgr_modinfo.modid, 0, 0x0, 1, sub_81000000_patched);
	return 0;
}

static SceUID ksceSysrootIsSafeMode_hookid = -1;
static tai_hook_ref_t ksceSysrootIsSafeMode_hookref;
static int ksceSysrootIsSafeMode_patched() {
	return 1;
}

static tai_hook_ref_t ksceSblAimgrIsDolce_hookref;
static int ksceSblAimgrIsDolce_patched() {
	return 1;
}

int UMA_workaround_on_thread(void) {
	int (* _ksceKernelMountBootfs)(const char *bootImagePath);
	int (* _ksceKernelUmountBootfs)(void);
	int ret;
	
	ret = module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0xC445FA63, 0x01360661, (uintptr_t *)&_ksceKernelMountBootfs);
	if (ret < 0)
		ret = module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0x92C9FFC2, 0x185FF1BC, (uintptr_t *)&_ksceKernelMountBootfs);
	if (ret < 0)
		return SCE_KERNEL_START_NO_RESIDENT;

	ret = module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0xC445FA63, 0x9C838A6B, (uintptr_t *)&_ksceKernelUmountBootfs);
	if (ret < 0)
		ret = module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0x92C9FFC2, 0xBD61AD4D, (uintptr_t *)&_ksceKernelUmountBootfs);
	if (ret < 0)
		return SCE_KERNEL_START_NO_RESIDENT;
	
	// Load SceUsbMass kernel module
	SceUID sceusbmass_modid;
	LOG("Loading SceUsbMass from os0:.\nMounting bootfs:...\n");
	if ((ret = _ksceKernelMountBootfs("os0:kd/bootimage.skprx")) >= 0) {
		sceusbmass_modid = ksceKernelLoadModule("os0:kd/umass.skprx", 0x800, NULL);
		LOG("Unmounting bootfs: : %i\n", _ksceKernelUmountBootfs());
	} else {
		LOG("Error mounting bootfs: %08X\nLoading VitaShell umass.skprx...\n", ret);
		sceusbmass_modid = ksceKernelLoadModule("ux0:/VitaShell/module/umass.skprx", 0, NULL);
	}
	LOG("SceUsbMass module id : %08X.\n", (int)sceusbmass_modid);
	
	// Temporary fake SAFE mode and IsDolce (PSTV) in order that SceUsbMass can be started
	SceUID tmp1, tmp2;
	tmp1 = taiHookFunctionExportForKernel(KERNEL_PID, &ksceSysrootIsSafeMode_hookref, "SceSysmem", 0x2ED7F97A, 0x834439A7, ksceSysrootIsSafeMode_patched);
	if (tmp1 < 0)
		return tmp1;
	tmp2 = taiHookFunctionExportForKernel(KERNEL_PID, &ksceSblAimgrIsDolce_hookref, "SceSysmem", 0xFD00C69A, 0x71608CA3, ksceSblAimgrIsDolce_patched);
	if (tmp2 < 0)
		return tmp2;
	
	// Start SceUsbMass kernel module
	if (sceusbmass_modid >= 0) ret = ksceKernelStartModule(sceusbmass_modid, 0, NULL, 0, NULL, NULL); 
	else ret = sceusbmass_modid;
	
	// Release SAFE and DOLCE exports hooks
	if (tmp1 >= 0) taiHookReleaseForKernel(tmp1, ksceSysrootIsSafeMode_hookref);
	if (tmp2 >= 0) taiHookReleaseForKernel(tmp2, ksceSblAimgrIsDolce_hookref);

	if (ret < 0)// Check result
		return ret;
	
	// Fake SAFE mode in SceUsbServ
	ksceSysrootIsSafeMode_hookid = taiHookFunctionImportForKernel(KERNEL_PID, &ksceSysrootIsSafeMode_hookref, "SceUsbServ", 0x2ED7F97A, 0x834439A7, ksceSysrootIsSafeMode_patched);
	
	return ret;
}

int UMA_workaround(void) {
	int state = 0;
	ENTER_SYSCALL(state);
	int result = -1;
	result = run_on_thread(UMA_workaround_on_thread);
	EXIT_SYSCALL(state);
	return result;
}


void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {
	
	ksceIoRemove(log_ur0_path);
	LOG("StorageMgrKernel ");
	LOG(VERSION_STRING);
	LOG(" started.\n");
	
	int ret = module_get_export_func(KERNEL_PID, "SceSysmem", 0x2ED7F97A, 0x67AAB627, (uintptr_t *)&ksceSysrootGetSystemSwVersion);
	if (ret < 0)
		return SCE_KERNEL_START_NO_RESIDENT;
	LOG("system_sw_version: %08X\n", ksceSysrootGetSystemSwVersion());
	
	tai_module_info_t sceiofilemgr_modinfo;
	sceiofilemgr_modinfo.size = sizeof(tai_module_info_t);
	if (taiGetModuleInfoForKernel(KERNEL_PID, "SceIofilemgr", &sceiofilemgr_modinfo) < 0)
		return -1;
	
	// Get important function
	switch (sceiofilemgr_modinfo.module_nid) {
		case 0x9642948C: // 3.60 retail/testkit/devkit
			module_get_offset(KERNEL_PID, sceiofilemgr_modinfo.modid, 0, 0x138C1, (uintptr_t *)&sceIoFindMountPoint);
			break;
		case 0xA96ACE9D: // 3.65 retail/testkit/devkit
		case 0x3347A95F: // 3.67 retail/testkit/devkit
		case 0x90DA33DE: // 3.68 retail/testkit/devkit
			module_get_offset(KERNEL_PID, sceiofilemgr_modinfo.modid, 0, 0x182F5, (uintptr_t *)&sceIoFindMountPoint);
			break;
		default:
			return -1;
	}
	
	patch_appmgr(); // this way we can exit HENkaku bootstrap.self after ux0: has been remounted
	suspend_workaround(); // To keep uma0: mounted after that PSVita exits suspend mode
	
	saveOriginalDevicesForMountPoints();
	
	char *device = NULL;
	char *device2 = NULL;
	LOG("Is ux0: redirected : %i\n", shellKernelIsPartitionRedirected(UX0_DEV, &device, &device2));
	LOG("ux0: current device : %s %s\n", device, device2);
	
	if (checkConfigLineReturnChar(default_config_path) != 0)
		return -1;
	int entriesNumber = config_read(default_config_path);
	//LOG("Config entries : %i.\n", entriesNumber);
	if (entriesNumber > 0) {
		LOG("Checking if UMA is in config.\n");
		int UMAline = isDeviceInConfig("UMA");
		LOG("Checking if GCD is in config.\n");
		int GCDline = isDeviceInConfig("GCD");
		LOG("Checking if MCD is in config.\n");
		int MCDline = isDeviceInConfig("MCD");
		LOG("Checking if INT is in config.\n");
		int INTline = isDeviceInConfig("INT");
		if (UMAline != -1) {
			LOG("UMA config found at line %i.\n", UMAline);
			UMA_workaround();
			// wait ~5 seconds max for USB mass to be detected
			// this may look bad but the PSVita does this to detect ux0: so ¯\_(ツ)_/¯
			for (int i = 0; i <= 25; i++) { // try to detect USB mass 25 times for 0.2s each
				LOG("USB mass detection...\n");
				if (exists(UMA_BLKDEV) || exists(UMA_BLKDEV2)) {
					LOG("USB mass detected.\n");
					char UMAmountPoint[16];
					if (readMountPointByLine(UMAline, UMAmountPoint) == 0) {
						if (memcmp(UMAmountPoint, "uma0", strlen("uma0")) == 0) {
							ksceIoMount(UMA0_ID, NULL, 0, 0, 0, 0);
							uma0_used = 1;
							break;
						} else if (memcmp(UMAmountPoint, "ux0", strlen("ux0")) == 0) {
							if (shellKernelRedirect(UX0_DEV, "UMA") == -1) {
								LOG("No ux0: mount point.\n");
								return SCE_KERNEL_START_FAILED;
							}
							break;
						} else if (memcmp(UMAmountPoint, "grw0", strlen("grw0")) == 0) {
							if (shellKernelRedirect(GRW0_DEV, "UMA") == -1) {
								LOG("No grw0: mount point.\n");
								return SCE_KERNEL_START_FAILED;
							}
							break;
						} else if (memcmp(UMAmountPoint, "imc0", strlen("imc0")) == 0) {
							if (shellKernelRedirect(IMC0_DEV, "UMA") == -1) {
								LOG("No imc0: mount point.\n");
								return SCE_KERNEL_START_FAILED;
							}
							break;
						}
					}
				} else ksceKernelDelayThread(200 * 1000);
			}
			if (!exists(UMA_BLKDEV) && !exists(UMA_BLKDEV2))
				LOG("USB mass still not detected. Aborting USB mass detection.\n");
		} else LOG("No UMA config found.\n");
		if (GCDline != -1) {
			LOG("GCD config found at line %i.\n", GCDline);
			if (GCD_patch_scesdstor() != 0)
				return -1;
			GCD_poke();
			LOG("GC2SD detection...\n");
			if (exists(GCD_BLKDEV))
				LOG("GC2SD detected.\n");
			else
				LOG("GC2SD not detected.\n");
			char GCDmountPoint[16];
			if (readMountPointByLine(GCDline, GCDmountPoint) == 0) {
				if (memcmp(GCDmountPoint, "ux0", strlen("ux0")) == 0 && exists(GCD_BLKDEV)) {
					if (shellKernelRedirect(UX0_DEV, "GCD") == -1) {
						LOG("No ux0: mount point.\n");
						return SCE_KERNEL_START_FAILED;
					}
				} else if (memcmp(GCDmountPoint, "grw0", strlen("grw0")) == 0) {
					if (shellKernelRedirect(GRW0_DEV, "GCD") == -1) {
						LOG("No grw0: mount point.\n");
						return SCE_KERNEL_START_FAILED;
					}
				} else if (memcmp(GCDmountPoint, "uma0", strlen("uma0")) == 0) {
					if (shellKernelRedirect(UMA0_DEV, "GCD") == -1) {
						LOG("No uma0: mount point.\n");
						return SCE_KERNEL_START_FAILED;
					}
					uma0_used = 1;
				} else if (memcmp(GCDmountPoint, "imc0", strlen("imc0")) == 0) {
					if (shellKernelRedirect(IMC0_DEV, "GCD") == -1) {
						LOG("No imc0: mount point.\n");
						return SCE_KERNEL_START_FAILED;
					}
				}
			}
		} else LOG("No GCD config found.\n");
		if (INTline != -1) {
			LOG("INT config found at line %i.\n", INTline);
			LOG("Internal storage detection...\n");
			if (exists(INT_BLKDEV)) {
				LOG("Internal storage detected.\n");
				char INTmountPoint[16];
				if (readMountPointByLine(INTline, INTmountPoint) == 0) {
					if (memcmp(INTmountPoint, "imc0", strlen("imc0")) == 0) {
						ksceIoMount(IMC0_ID, NULL, 0, 0, 0, 0);
					} else if (memcmp(INTmountPoint, "ux0", strlen("ux0")) == 0) {
						if (shellKernelRedirect(UX0_DEV, "INT") == -1) {
							LOG("No ux0: mount point.\n");
							return SCE_KERNEL_START_FAILED;
						}
					} else if (memcmp(INTmountPoint, "grw0", strlen("grw0")) == 0) {
						if (shellKernelRedirect(GRW0_DEV, "INT") == -1) {
							LOG("No grw0: mount point.\n");
							return SCE_KERNEL_START_FAILED;
						}
					} else if (memcmp(INTmountPoint, "uma0", strlen("uma0")) == 0) {
						if (shellKernelRedirect(UMA0_DEV, "INT") == -1) {
							LOG("No uma0: mount point.\n");
							return SCE_KERNEL_START_FAILED;
						}
						uma0_used = 1;
					}
				}
			} else LOG("Internal storage not detected.\n");
		} else LOG("No INT config found.\n");
		if (MCDline != -1) {
			LOG("MCD config found at line %i.\n", MCDline);
			LOG("MCD detection...\n");
			if (exists(MCD_BLKDEV)) {
				LOG("MCD detected.\n");
				char MCDmountPoint[16];
				if (readMountPointByLine(MCDline, MCDmountPoint) == 0) {
					if (memcmp(MCDmountPoint, "xmc0", strlen("xmc0")) == 0) {
						ksceIoMount(XMC0_ID, NULL, 0, 0, 0, 0);
					} else if (memcmp(MCDmountPoint, "imc0", strlen("imc0")) == 0) {
						if (shellKernelRedirect(IMC0_DEV, "MCD") == -1) {
							LOG("No imc0: mount point.\n");
							return SCE_KERNEL_START_FAILED;
						}
					} else if (memcmp(MCDmountPoint, "ux0", strlen("ux0")) == 0) {
						if (shellKernelRedirect(UX0_DEV, "MCD") == -1) {
							LOG("No ux0: mount point.\n");
							return SCE_KERNEL_START_FAILED;
						}
					} else if (memcmp(MCDmountPoint, "grw0", strlen("grw0")) == 0) {
						if (shellKernelRedirect(GRW0_DEV, "MCD") == -1) {
							LOG("No grw0: mount point.\n");
							return SCE_KERNEL_START_FAILED;
						}
					} else if (memcmp(MCDmountPoint, "uma0", strlen("uma0")) == 0) {
						if (shellKernelRedirect(UMA0_DEV, "MCD") == -1) {
							LOG("No uma0: mount point.\n");
							return SCE_KERNEL_START_FAILED;
						}
						uma0_used = 1;
					}
				}
			} else LOG("MCD not detected.\n");
		} else LOG("No MCD config found.\n");
	} else LOG("No device config found.\n");

	LOG("Is ux0: redirected : %i\n", shellKernelIsPartitionRedirected(UX0_DEV, &device, &device2));
	LOG("ux0: current device : %s %s\n", device, device2);
	LOG("Is grw0: redirected : %i\n", shellKernelIsPartitionRedirected(GRW0_DEV, &device, &device2));
	LOG("grw0: current device : %s %s\n", device, device2);
	LOG("Is uma0: redirected : %i\n", shellKernelIsPartitionRedirected(UMA0_DEV, &device, &device2));
	LOG("uma0: current device : %s %s\n", device, device2);
	
	LOG("StorageMgrKernel finished starting with success.\n");
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
	// Should never be called because StorageMgrKernel cannot be unloaded.
	// Indeed, after partition redirection StorageMgrKernel embeds SceIoFileMgr mount points pointers...
	return SCE_KERNEL_STOP_SUCCESS;
}

void log_write(const char *buffer, size_t length, const char *folderpath, const char *fullpath) {
	ksceIoMkdir(folderpath, 6);
	SceUID fd = ksceIoOpen(fullpath, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 6);
	if (fd < 0)
		return;
	ksceIoWrite(fd, buffer, length);
	ksceIoClose(fd);
}
