/*
** Copyright 2016, The CyanogenMod Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>

#include "log.h"
#include "property_service.h"
#include "util.h"
#include "vendor_init.h"

#define PHONE_INFO "/factory/SSN"
#define BUF_SIZE 64

/* Serial number */
#define SERIAL_PROP "ro.serialno"
#define SERIAL_OFFSET 0x00
#define SERIAL_LENGTH 17

/* Zram */
#define ZRAM_PROP "ro.config.zram"
#define MEMINFO_FILE "/proc/meminfo"
#define MEMINFO_KEY "MemTotal:"
#define ZRAM_MEM_THRESHOLD 3000000

/* Intel props */
#define IFWI_PATH       "/sys/kernel/fw_update/fw_info/ifwi_version"
#define CHAABI_PATH     "/sys/kernel/fw_update/fw_info/chaabi_version"
#define MIA_PATH        "/sys/kernel/fw_update/fw_info/mia_version"
#define SCU_BS_PATH     "/sys/kernel/fw_update/fw_info/scu_bs_version"
#define SCU_PATH        "/sys/kernel/fw_update/fw_info/scu_version"
#define IA32FW_PATH     "/sys/kernel/fw_update/fw_info/ia32fw_version"
#define VALHOOKS_PATH   "/sys/kernel/fw_update/fw_info/valhooks_version"
#define PUNIT_PATH      "/sys/kernel/fw_update/fw_info/punit_version"
#define UCODE_PATH      "/sys/kernel/fw_update/fw_info/ucode_version"
#define PMIC_NVM_PATH   "/sys/kernel/fw_update/fw_info/pmic_nvm_version"
#define WATCHDOG_PATH   "/sys/devices/virtual/misc/watchdog/counter"
#define OSRELEASE_PATH  "/proc/sys/kernel/osrelease"
#define PROJID_PATH     "/sys/module/intel_mid_sfi/parameters/project_id"

static int read_file2(const char *fname, char *data, int max_size)
{
    int fd, rc;

    if (max_size < 1)
        return 0;

    fd = open(fname, O_RDONLY);
    if (fd < 0) {
        ERROR("failed to open '%s'\n", fname);
        return 0;
    }

    rc = read(fd, data, max_size -1);
    if ((rc > 0) && (rc < max_size ))
        data[rc] = '\0';
    else
        data[0] = '\0';
    data[strcspn(data, "\n")] = 0;
    close(fd);

    return 1;
}

static void get_serial()
{
    int ret = 0;
    char const *path = PHONE_INFO;
    char buf[SERIAL_LENGTH + 1];
    prop_info *pi;

    if(read_file2(path, buf, sizeof(buf))) {
        if (strlen(buf) > 0) {
            pi = (prop_info*) __system_property_find(SERIAL_PROP);
            if(pi)
                ret = __system_property_update(pi, buf,
                        strlen(buf));
            else
                ret = __system_property_add(SERIAL_PROP,
                        strlen(SERIAL_PROP),
                        buf, strlen(buf));
        }
    }
}

static void configure_zram() {
    char buf[128];
    FILE *f;

    if ((f = fopen(MEMINFO_FILE, "r")) == NULL) {
        ERROR("%s: Failed to open %s\n", __func__, MEMINFO_FILE);
        return;
    }

    while (fgets(buf, sizeof(buf), f) != NULL) {
        if (strncmp(buf, MEMINFO_KEY, strlen(MEMINFO_KEY)) == 0) {
            int mem = atoi(&buf[strlen(MEMINFO_KEY)]);
            const char *mode = mem < ZRAM_MEM_THRESHOLD ? "true" : "false";
            INFO("%s: Found total memory to be %d kb, zram enabled: %s\n", __func__, mem, mode);
            property_set(ZRAM_PROP, mode);
            break;
        }
    }

    fclose(f);
}

static void intel_props() {

    char buf[BUF_SIZE];

    if(read_file2(IFWI_PATH, buf, sizeof(buf))) {
            property_set("sys.ifwi.version", buf);
    }

    if(read_file2(CHAABI_PATH, buf, sizeof(buf))) {
            property_set("sys.chaabi.version", buf);
    }

    if(read_file2(MIA_PATH, buf, sizeof(buf))) {
            property_set("sys.mia.version", buf);
    }

    if(read_file2(SCU_BS_PATH, buf, sizeof(buf))) {
            property_set("sys.scubs.version", buf);
    }

    if(read_file2(SCU_PATH, buf, sizeof(buf))) {
            property_set("sys.scu.version", buf);
    }

    if(read_file2(IA32FW_PATH, buf, sizeof(buf))) {
            property_set("sys.ia32.version", buf);
    }

    if(read_file2(VALHOOKS_PATH, buf, sizeof(buf))) {
            property_set("sys.valhooks.version", buf);
    }

    if(read_file2(PUNIT_PATH, buf, sizeof(buf))) {
            property_set("sys.punit.version", buf);
    }

    if(read_file2(UCODE_PATH, buf, sizeof(buf))) {
            property_set("sys.ucode.version", buf);
    }

    if(read_file2(PMIC_NVM_PATH, buf, sizeof(buf))) {
            property_set("sys.pmic.nvm.version", buf);
    }

    if(read_file2(WATCHDOG_PATH, buf, sizeof(buf))) {
            property_set("sys.watchdog.previous.counter", buf);
    }

    if(read_file2(OSRELEASE_PATH, buf, sizeof(buf))) {
            property_set("sys.kernel.version", buf);
    }

}

static int setup_projid() {
    char buf[BUF_SIZE];

    if(read_file2(PROJID_PATH, buf, sizeof(buf))) {
      int projid = atoi(buf);
      if(projid == 5 || projid == 7) { // if T00G (Zenfone 6) is detected
        property_set("ro.product.model", "ASUS_T00G");
        property_set("ro.build.fingerprint", "asus/WW_a600cg/ASUS_T00G:5.0/LRX21V/WW_user_3.24.40.87_20151222_34:user/release-keys");
        property_set("ro.build.description", "a600cg-user 5.0 LRX21V WW_user_3.24.40.87_20151222_34 release-keys");
        property_set("ro.product.device", "T00G");
        property_set("ro.product.name", "WW_a600cg");
        system("mount -o remount,rw /system");
        unlink("/system/lib/hw/sensors.redhookbay.so");
        symlink("/system/lib/hw/a600cg.sensors.redhookbay.so", "/system/lib/hw/sensors.redhookbay.so");
        unlink("/system/lib/libxditk_DIT_Manager.so");
        symlink("/system/lib/ditlib_a600cg/libxditk_DIT_Manager.so", "/system/lib/libxditk_DIT_Manager.so");
        unlink("/system/lib/libxditk_DIT_CloverTrailPlus.so");
        symlink("/system/lib/ditlib_a600cg/libxditk_DIT_CloverTrailPlus.so", "/system/lib/libxditk_DIT_CloverTrailPlus.so");
        system("mount -o remount,ro /system");
      }
    }
   return 0;
}


void vendor_load_properties()
{
    get_serial();
    setup_projid();
    configure_zram();
    intel_props();
}
