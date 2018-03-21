#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/devctl.h>
#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#include <psp2/appmgr.h>
#include <psp2/ctrl.h>
#include <psp2/power.h>
#include <psp2/registrymgr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug_screen.h"

#define STORAGEMGR_INSTALL_PATH "ur0:tai/storagemgr.skprx"
#define GB_IN_BYTES (1073741824.0f)

#define printf psvDebugScreenPrintf

int configpath = 0;

int _vshIoMount(int id, const char *path, int permission, void *buf);

enum {
	SCREEN_WIDTH = 960,
	SCREEN_HEIGHT = 544,
	PROGRESS_BAR_WIDTH = SCREEN_WIDTH,
	PROGRESS_BAR_HEIGHT = 10,
	LINE_SIZE = SCREEN_WIDTH,
};

static unsigned buttons[] = {
	SCE_CTRL_SELECT,
	SCE_CTRL_START,
	SCE_CTRL_UP,
	SCE_CTRL_RIGHT,
	SCE_CTRL_DOWN,
	SCE_CTRL_LEFT,
	SCE_CTRL_LTRIGGER,
	SCE_CTRL_RTRIGGER,
	SCE_CTRL_TRIANGLE,
	SCE_CTRL_CIRCLE,
	SCE_CTRL_CROSS,
	SCE_CTRL_SQUARE,
};

int vshIoMount(int id, const char *path, int permission, int a4, int a5, int a6) {
	uint32_t buf[3];

	buf[0] = a4;
	buf[1] = a5;
	buf[2] = a6;

	return _vshIoMount(id, path, permission, buf);
}

uint32_t get_key(void) {
	static unsigned prev = 0;
	SceCtrlData pad;
	while (1) {
		memset(&pad, 0, sizeof(pad));
		sceCtrlPeekBufferPositive(0, &pad, 1);
		unsigned new = prev ^ (pad.buttons & prev);
		prev = pad.buttons;
		for (size_t i = 0; i < sizeof(buttons)/sizeof(*buttons); ++i)
			if (new & buttons[i])
				return buttons[i];

		sceKernelDelayThread(1000); // 1ms
	}
}

void press_exit(void) {
	printf("Press any key to exit this application.\n");
	get_key();
	sceKernelExitProcess(0);
}

void press_reboot(void) {
	printf("Press any key to reboot.\n");
	get_key();
	scePowerRequestColdReset();
}

void press_shutdown(void) {
	printf("Press any key to power off.\n");
	get_key();
	scePowerRequestStandby();
}

void draw_rect(int x, int y, int width, int height, uint32_t color) {
	void *base = psvDebugScreenBase();

	for (int j = y; j < y + height; ++j)
		for (int i = x; i < x + width; ++i)
			((uint32_t*)base)[j * LINE_SIZE + i] = color;
}

int exists(const char *path) {
	int fd = sceIoOpen(path, SCE_O_RDONLY, 0);
	if (fd < 0)
		return 0;
	sceIoClose(fd);
	return 1;
}

int check_safe_mode(void) {
	if (sceIoDevctl("ux0:", 0x3001, NULL, 0, NULL, 0) == 0x80010030) {
		return 1;
	} else {
		return 0;
	}
}

int check_enso(void) {
	return exists("ur0:tai/boot_config.txt") || exists("vs0:tai/boot_config.txt");
}

int copy_file(const char *dst, const char *src) {
	char buffer[0x1000];
	int ret;
	int off;
	SceIoStat stat;

	printf("Copying %s ...\n", src);

	int fd = sceIoOpen(src, SCE_O_RDONLY, 0);
	if (fd < 0) {
		printf("sceIoOpen(%s): 0x%08X\n", src, fd);
		return -1;
	}
	int wfd = sceIoOpen(dst, SCE_O_WRONLY | SCE_O_TRUNC | SCE_O_CREAT, 0777);
	if (wfd < 0) {
		printf("sceIoOpen(%s): 0x%08X\n", dst, wfd);
		sceIoClose(fd);
		return -1;
	}
	ret = sceIoGetstatByFd(fd, &stat);
	if (ret < 0) {
		printf("sceIoGetstatByFd: 0x%08X\n", ret);
		goto error;
	}
	ret = sceIoChstatByFd(wfd, &stat, SCE_CST_CT | SCE_CST_AT | SCE_CST_MT);
	if (ret < 0) {
		printf("sceIoChstat: 0x%08X\n", ret);
		return -1;
	}

	size_t rd, wr, total, write;
	total = 0;
	draw_rect(0, SCREEN_HEIGHT - PROGRESS_BAR_HEIGHT, PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEIGHT, 0xFF666666);
	while ((rd = sceIoRead(fd, buffer, sizeof(buffer))) > 0){
		off = 0;
		while ((off += (wr = sceIoWrite(wfd, buffer+off, rd-off))) < rd){
			if (wr < 0){
				printf("sceIoWrite: 0x%08X\n", wr);
				goto error;
			}
		}
		total += rd;
		draw_rect(1, SCREEN_HEIGHT - PROGRESS_BAR_HEIGHT + 1, ((uint64_t)(PROGRESS_BAR_WIDTH - 2)) * total / stat.st_size, PROGRESS_BAR_HEIGHT - 2, 0xFFFFFFFF);
	}
	if (rd < 0) {
		printf("sceIoRead: 0x%08X\n", rd);
		goto error;
	}

	sceIoClose(fd);
	sceIoClose(wfd);

	return 0;

error:
	sceIoClose(fd);
	sceIoClose(wfd);
	return -1;
}

int copy_directory(const char *dst, const char *src) {
	int fd;
	SceIoDirent dir;
	SceIoStat stat;
	char src_2[256];
	char dst_2[256];
	int ret;

	printf("Reading %s ...\n", src);

	sceIoMkdir(dst, 0777);

	if ((fd = sceIoDopen(src)) < 0) {
		printf("sceIoDopen: 0x%08X\n", fd);
	    return -1;
	}

	while ((ret = sceIoDread(fd, &dir)) > 0) {
		if (dir.d_name[0] == '\0') {
			continue;
		}
		sprintf(src_2, "%s/%s", src, dir.d_name);
		sprintf(dst_2, "%s/%s", dst, dir.d_name);
		if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
			copy_directory(dst_2, src_2);
		} else {
			copy_file(dst_2, src_2);
		}
	}

	sceIoDclose(fd);
	return 0;
error:
	sceIoDclose(fd);
	return -1;
}

int find_config(const char *configpath, int remove) {
	int fd;
	int size;
	char *buffer;
	char *line;
	size_t offset, newsize;

	if ((fd = sceIoOpen(configpath, SCE_O_RDONLY, 0)) < 0) {
		return 0;
	}

	size = sceIoLseek32(fd, 0, SEEK_END);
	if (size < 0) {
		sceIoClose(fd);
		return 0;
	}
	if (sceIoLseek32(fd, 0, SEEK_SET) < 0) {
		sceIoClose(fd);
		return 0;
	}

	buffer = malloc(size);
	if (buffer == NULL) {
		sceIoClose(fd);
		return 0;
	}

	int rd, total;
	total = 0;
	while ((rd = sceIoRead(fd, buffer+total, size-total)) > 0) {
		total += rd;
	}
	sceIoClose(fd);
	if (rd < 0 || total != size) {
		free(buffer);
		return 0;
	}

	if ((line = strstr(buffer, STORAGEMGR_INSTALL_PATH "\n")) == NULL) {
		free(buffer);
		return 0;
	} else {
		if (remove) {
			offset = (line - buffer);
			newsize = size - strlen(STORAGEMGR_INSTALL_PATH "\n");
			memmove(line, line + strlen(STORAGEMGR_INSTALL_PATH "\n"), newsize - offset);
			fd = sceIoOpen(configpath, SCE_O_TRUNC | SCE_O_CREAT | SCE_O_WRONLY, 6);
			sceIoWrite(fd, buffer, newsize);
			sceIoClose(fd);
		}
		free(buffer);
		return 1;
	}
}

int install_config(const char *path) {
	int fd;

	if (exists(path)) {
		printf("%s detected!\n", path);

		if (find_config(path, 0)) {
			printf("already installed to %s\n", path);
		} else {
			printf("installing to %s ", path);
			fd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_APPEND, 0);
			sceIoWrite(fd, "\n*KERNEL\n", strlen("\n*KERNEL\n"));
			sceIoWrite(fd, STORAGEMGR_INSTALL_PATH "\n", strlen(STORAGEMGR_INSTALL_PATH) + 1);
			sceIoClose(fd);
			if (fd < 0) {
				printf("failed.\n");
			} else {
				printf("success.\n");
				return 0;
			}
		}
	}
	return -1;
}

int enable_modem(void) {
	printf("\nenabling 3G modem... ");
	if (sceRegMgrSetKeyInt("/CONFIG/TEL/", "use_debug_settings", 0) < 0) {
		printf("failed.\n");
	} else {
		printf("success.\n");
	}

	return 0;
}

int disable_modem(void) {
	printf("\ndisabling 3G modem... ");
	if (sceRegMgrSetKeyInt("/CONFIG/TEL/", "use_debug_settings", 1) < 0) {
		printf("failed.\n");
	} else {
		printf("success.\n");
	}

	return 0;
}

int install_plugin(void) {
	printf("writing plugin...\n");
	if (copy_file(STORAGEMGR_INSTALL_PATH, "app0:storagemgr.skprx") < 0) {
		printf("failed, plugin does not exists.\n");
		return -1;
	} else {
		printf("success.\n");
	}

	if (install_config("ur0:tai/config.txt") < 0) {
		printf("failed install to ur0:tai/config.txt, perhaps you should upgrade HENkaku\n");
		return -1;
	}

	vshIoMount(0xD00, NULL, 2, 0, 0, 0);
	install_config("imc0:tai/config.txt");
	install_config("ux0:tai/config.txt");

	disable_modem();

	return 0;
}

int uninstall_plugin(void) {
	printf("deleting plugin... ");
	if (sceIoRemove(STORAGEMGR_INSTALL_PATH) < 0) {
		printf("failed.\n");
		return -1;
	} else {
		printf("success.\n");
	}

	if (find_config("ur0:tai/config.txt", 1)) {
		printf("removed from ur0:tai/config.txt\n");
	}

	vshIoMount(0xD00, NULL, 2, 0, 0, 0);
	if (find_config("imc0:tai/config.txt", 1)) {
		printf("removed from imc0:tai/config.txt\n");
	}
	if (find_config("ux0:tai/config.txt", 1)) {
		printf("removed from ux0:tai/config.txt\n");
	}

	enable_modem();

	return 0;
}

int configure_modem(void) {

	printf("  CROSS      Disable 3G Modem.\n");
	printf("  TRIANGLE   Enable 3g Modem.\n");
	printf("  CIRCLE     Exit App.\n");

	switch (get_key()) {
	case SCE_CTRL_CROSS:
		disable_modem();
		break;
	case SCE_CTRL_TRIANGLE:
		enable_modem();
		break;
	case SCE_CTRL_CIRCLE:
		press_exit();
		break;
	}

	return 0;
}

int storage_config(const char *path, char *memcard, char *sd2vita, char *intmem, char *psvsd) {
	if (!exists(path)){
		printf("storage_config.txt does not exist. Creating it... %s ", "\n");
			FILE *fd = fopen("ur0:tai/storage_config.txt", "w");
	}
	else {
		printf("Clearing storage_config.txt for new settings... %s ", "\n");
			FILE *fd = fopen("ur0:tai/storage_config.txt", "w");
	}

	printf("Writing storage mount points to storage_config.txt... %s ", "\n");
		int fd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_APPEND, 0);
		if (memcard != ""){
			sceIoWrite(fd, "MCD=", strlen("MCD="));
			sceIoWrite(fd, memcard, strlen(memcard) + 1);
		}
		if (intmem != ""){
			sceIoWrite(fd, "\nINT=", strlen("INT=") +1);
			sceIoWrite(fd, intmem, strlen(intmem) + 1);
		}
		if (psvsd != ""){
			sceIoWrite(fd, "\nUMA=", strlen("UMA=") + 1);
			sceIoWrite(fd, psvsd, strlen(psvsd) + 1);
		}
		if (sd2vita != ""){
			sceIoWrite(fd, "\nGCD=", strlen("GCD=") + 1);
			sceIoWrite(fd, sd2vita, strlen(sd2vita) + 1);
		}
		sceIoClose(fd);
		if (fd < 0) {
			printf("failed.\n");
		} else {
			printf("success.\n");
			return 0;
		}
}

int alternate_config(const char *path, char *memcard, char *sd2vita, char *intmem, char *psvsd) {
	if (!exists(path)){
		printf("storage_config_mod.txt does not exist. Creating it... %s ", "\n");
			FILE *fd = fopen("ur0:tai/storage_config_mod.txt", "w");
	}
	else {
		printf("Clearing storage_config_mod.txt for new settings... %s ", "\n");
			FILE *fd = fopen("ur0:tai/storage_config_mod.txt", "w");
	}

	printf("Writing storage mount points to storage_config_mod.txt... %s ", "\n");
		int fd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_APPEND, 0);
		if (memcard != ""){
			sceIoWrite(fd, "MCD=", strlen("MCD="));
			sceIoWrite(fd, memcard, strlen(memcard) + 1);
		}
		if (intmem != ""){
			sceIoWrite(fd, "\nINT=", strlen("INT=") +1);
			sceIoWrite(fd, intmem, strlen(intmem) + 1);
		}
		if (psvsd != ""){
			sceIoWrite(fd, "\nUMA=", strlen("UMA=") + 1);
			sceIoWrite(fd, psvsd, strlen(psvsd) + 1);
		}
		if (sd2vita != ""){
			sceIoWrite(fd, "\nGCD=", strlen("GCD=") + 1);
			sceIoWrite(fd, sd2vita, strlen(sd2vita) + 1);
		}
		sceIoClose(fd);
		if (fd < 0) {
			printf("failed.\n");
		} else {
			printf("success.\n");
			return 0;
		}
}

char * mount_selection(void) {
	char *selection = NULL;

	printf("  CROSS      Mount as uma0.\n");
	printf("  TRIANGLE   Mount as imc0.\n");
	printf("  SQUARE     Mount as grw0.\n");
	printf("  CIRCLE     Mount as ux0.\n");
	printf("  RTRIGGER   Do not mount.\n");

again:
	switch (get_key()) {
	case SCE_CTRL_CROSS:
		selection = "uma0";
		break;
	case SCE_CTRL_TRIANGLE:
		selection = "imc0";
		break;
	case SCE_CTRL_SQUARE:
		selection = "grw0";
		break;
	case SCE_CTRL_CIRCLE:
		selection = "ux0";
		break;
	case SCE_CTRL_RTRIGGER:
		selection = "";
		break;
	default:
		goto again;
	}

	return selection;
}

int configure_plugin(int charpath) {

	printf("\nSelect how to mount the Sony Memory Card.\n\n");
	char *memcard = mount_selection();

	printf("\nSelect how to mount the SD2Vita Card.\n\n");
	char *sd2vita = mount_selection();

	printf("\nSelect how to mount the Internal Memory.\n\n");
	char *intmem = mount_selection();

	printf("\nSelect how to mount the PSVSD Card.\n\n");
	char *psvsd = mount_selection();

	if (charpath == 0) {
		storage_config("ur0:tai/storage_config.txt", memcard, sd2vita, intmem, psvsd);
	}
	else {
		alternate_config("ur0:tai/storage_config_mod.txt", memcard, sd2vita, intmem, psvsd);
	}

	return 0;
}

int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;

	int ret = 0;

	psvDebugScreenInit();

	if (check_safe_mode()) {
		printf("Please enable HENkaku unsafe homebrew from Settings before running this installer.\n\n");
		press_exit();
	}

	if (!check_enso()) {
		printf("HENkaku Enso must be installed and not deactivated. Visit https://enso.henkaku.xyz/ for more information.\n\n");
		press_exit();
	}

	if (!find_config("ur0:tai/config.txt", 0)) {
		printf("Press CROSS to install and configure the plugin and reboot or any other key to exit.\n\n");

		if (get_key() == SCE_CTRL_CROSS) {
			install_plugin();
			configure_plugin(0);
			press_reboot();
		}

		sceKernelExitProcess(0);
	}

	printf("Options:\n\n");
	printf("  CROSS      Configure StorageMgr plugin. Warning this will clear current settings.\n");
	printf("  TRIANGLE   Configure Alternate StorageMgr mount points.\n");
	printf("  SQUARE     Enable or Disable the internal modem. (OLED 3G Models only)\n");
	printf("  CIRCLE     Uninstall StorageMgr plugin.\n");
	printf("  RTRIGGER   Exit without doing anything.\n\n");

again:
	switch (get_key()) {
	case SCE_CTRL_CROSS:
		configpath = 0;
		configure_plugin(configpath);
		press_reboot();
		break;
	case SCE_CTRL_TRIANGLE:
		configpath = 1;
		configure_plugin(configpath);
		break;
	case SCE_CTRL_SQUARE:
		configure_modem();
		press_reboot();
		break;
	case SCE_CTRL_CIRCLE:
		uninstall_plugin();
		break;
	case SCE_CTRL_RTRIGGER:
		break;
	default:
		goto again;
	}

	press_exit();

	return 0;
}
