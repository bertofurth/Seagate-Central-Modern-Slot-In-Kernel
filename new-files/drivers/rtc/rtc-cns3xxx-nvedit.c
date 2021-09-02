/*******************************************************************************
 *
 *  drivers/rtc/rtc-nvedit.c
 *
 *  Real Time Clock driver for the CNS3XXX SOCs
 *
 *  Author: Ange Chang
 *  Changed: Bogdan Puscas <bpuscas@softvision.ro>, added dual env functions
 *
 *  Copyright (c) 2011 Cavium Networks
 *
 *  This file is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License, Version 2, as
 *  published by the Free Software Foundation.
 *:
 *  This file is distributed in the hope that it will be useful,
 *  but AS-IS and WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, TITLE, or
 *  NONINFRINGEMENT.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this file; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA or
 *  visit http://www.gnu.org/licenses/.
 *
 *  This file may also be available under a different license from Cavium.
 *  Contact Cavium Networks for more information
 *
 ******************************************************************************/

#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/mtd/mtd.h>
#include "rtc-cns3xxx-nvedit.h"

//#define DEBUGME
#ifdef CONFIG_CIRRUS_DUAL_MTD_ENV
struct env_data {
    /* env block size */
    unsigned int size;
    /* source/dest mtd */
    int mtd;
    /* dual env */
    struct mtd_info *mtds[2];
    /* env block */
    unsigned char *data;
};
static unsigned char active_flag = 1;
static unsigned char obsolete_flag = 0;
#else
static int config_env_size, env_size;
static unsigned char *env_buf;
static struct mtd_info *rtcnved_mtd;
static int env_valid;
#endif

static const uint32_t crc_table[256] = {
  0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
  0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
  0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
  0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
  0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
  0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
  0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
  0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
  0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
  0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
  0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
  0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
  0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
  0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
  0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
  0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
  0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
  0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
  0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
  0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
  0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
  0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
  0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
  0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
  0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
  0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
  0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
  0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
  0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
  0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
  0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
  0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
  0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
  0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
  0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
  0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
  0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
  0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
  0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
  0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
  0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
  0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
  0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
  0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
  0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
  0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
  0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
  0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
  0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
  0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
  0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
  0x2d02ef8dL
};

#ifdef CONFIG_CIRRUS_DUAL_MTD_ENV 
static uint32_t ubcrc32(uint32_t crc, const char *buf, uint32_t len);
static int env_read(struct env_data *ed);
static int env_save(struct env_data *ed);
//static void env_erase_callback(struct erase_info *ei);
#else /*CONFIG_CIRRUS_DUAL_MTD_ENV*/
static void dump_hex_f(const uint8_t *data, int len, int type);
static uint32_t ubcrc32(uint32_t crc, const char *buf, uint32_t len);
static void env_crc_update(void);
static int envmatch(unsigned char *s1, int i2);
static int env_save(void);
#endif /*CONFIG_CIRRUS_DUAL_MTD_ENV*/

#ifndef CONFIG_CIRRUS_DUAL_MTD_ENV
static void dump_hex_f(const uint8_t *data, int len, int type)
{
    int i;

    if (type & DUMP_HEX) {
        for (i = 0; i < len; i++)
            printk(KERN_INFO "%.2x", data[i]);
        printk(KERN_INFO "    ");
    }

    if (type & DUMP_ASCII) {
        for (i = 0; i < len; i++) {
            if (isprint(data[i]))
                printk(KERN_INFO "%c", data[i]);
            else
                printk(KERN_INFO ".");
        }
    }
    printk(KERN_INFO "\n");
}
#endif /*CONFIG_CIRRUS_DUAL_MTD_ENV*/

static uint32_t ubcrc32(uint32_t crc, const char *buf, uint32_t len)
{
    crc = crc ^ 0xffffffffL;

    while (len >= 8) {
        DO8(buf);
        len -= 8;
    }

    if (len)
        do { DO1(buf); } while (--len);
    return crc ^ 0xffffffffL;
}

#ifndef CONFIG_CIRRUS_DUAL_MTD_ENV
static void env_crc_update(void)
{
    uint32_t c = ubcrc32(0, &env_buf[ENV_HEADER_SIZE], env_size);
    *((uint32_t *)&env_buf[0]) = c;
}
#endif /*CONFIG_CIRRUS_DUAL_MTD_ENV*/

#ifdef CONFIG_CIRRUS_DUAL_MTD_ENV
static int env_save(struct env_data *ed) {
    int ret;
    size_t retlen = 0;
    int mtd;
    int retcode = 0;

    struct erase_info ei;

#ifdef DEBUGME
    printk(KERN_ERR\
        "********* %s called! ***********\n",\
        __func__);
#endif /*DEBUGME*/

    /* sanity */
    if (NULL == ed) {
        printk(KERN_ERR\
            "%s: Called with invalid data!\n",\
            __func__);
        return -1;
    }

    if (-1 == ed->mtd) {
       printk(KERN_ERR\
           "%s: Nothing to save, no sane env present!\n",\
            __func__);
       return -1;
    }

    /*invert them*/
    mtd = ed->mtd;
    if (0 == mtd) {
        mtd = 1;
    } else {
        mtd = 0;
    }

#ifdef DEBUGME
    printk(KERN_ERR\
        "%s: Current env %d, preparing %d\n",\
        __func__, ed->mtd, mtd);
#endif /*DEBUGME*/

    /* set as active */
    ed->data[ENV_HEADER_SIZE-1] = active_flag;

    /* erase */
    memset(&ei, 0, sizeof(ei));
    ei.addr = 0;
    ei.len = ed->size;

#ifdef DEBUGME
    printk(KERN_ERR\
        "%s: About to call erase on MTD %d!\n",\
        __func__, mtd);
#endif /*DEBUGME*/

    ret = mtd_erase(ed->mtds[mtd], &ei);
    if (ret) {
        printk(KERN_ERR\
            "%s: Call to erase failed!\n",\
            __func__);
        retcode = -EIO;
        goto bail;
    }

    
#ifdef DEBUGME
    printk(KERN_ERR\
        "%s: About to call write on MTD %d!\n",\
        __func__, mtd);
#endif /*DEBUGME*/

    /* write */
    mtd_write(ed->mtds[mtd], 0, ed->size, &retlen, ed->data);

    if (ed->size != retlen) {
        printk(KERN_ERR\
            "%s: Failed to write the new data!\n",\
            __func__);
        retcode = -EIO;
        goto bail;
    }

#ifdef DEBUGME
    printk(KERN_ERR\
        "%s: About to call write on MTD %d for flags!\n",\
        __func__, ed->mtd);
#endif /*DEBUGME*/

    /* write obsolete on current partition */
    mtd_write(ed->mtds[ed->mtd], ENV_HEADER_SIZE - 1, 1, &retlen, &obsolete_flag);
    if (1 != retlen) {
        printk(KERN_ERR\
            "%s: Failed to write the data flag!\n",\
             __func__);
        retcode = -EIO;
    }

bail:

    /* tear down ed*/
    ed->mtd = -1;
    kfree(ed->data);
    ed->data = NULL;

#ifdef DEBUGME
    printk(KERN_ERR\
        "%s: About to put the mdt's down!\n",\
        __func__);
#endif /*DEBUGME*/

    put_mtd_device(ed->mtds[0]); ed->mtds[0] = NULL;
    put_mtd_device(ed->mtds[1]); ed->mtds[1] = NULL;

#ifdef DEBUGME
    printk(KERN_ERR\
        "%s: About to exit!\n",\
        __func__);
#endif /*DEBUGME*/

    return retcode;
}
#else /*CONFIG_CIRRUS_DUAL_MTD_ENV*/
static int env_save(void)
{
    int ret;
    size_t retlen = 0;
    struct erase_info ei;

    if (!env_valid)
        return -1;

    /* erase */
    memset(&ei, 0, sizeof(ei));
    ei.addr = 0;
    ei.len = config_env_size;
    ret = mtd_erase(rtcnved_mtd, &ei);
    if (ret) {
        printk(KERN_ERR "%s: call erase failed!\r\n", __func__);
        return -EIO;
    }


    /* write */
    mtd_write(rtcnved_mtd, 0, config_env_size, &retlen, env_buf);

    return 0;
}

#endif /*CONFIG_CIRRUS_DUAL_MTD_ENV*/

#ifndef CONFIG_CIRRUS_DUAL_MTD_ENV
int rtcnvet_do_printenv(char *name)
{
    int i, j, k, nxt;
    const uint8_t *b;

    if (name == NULL) {
        /* Print all env variables  */
        for (i = 0; env_get_char(i) != '\0'; i = nxt+1) {
            for (nxt = i; env_get_char(nxt) != '\0'; ++nxt)
                ;
            b = (const uint8_t *)&env_buf[i];
            dump_hex_f(b, (nxt-i), DUMP_ASCII);
        }

        printk(KERN_INFO "\nEnvironment size: %d/%d bytes\n",
        i, env_size);

        return 0;
    } else {
        k = -1;

        for (j = 0; env_get_char(j) != '\0'; j = nxt+1) {

            for (nxt = j; env_get_char(nxt) != '\0'; ++nxt)
                ;
            k = envmatch((unsigned char *)name, j);
            if (k < 0)
                continue;

            printk(KERN_INFO "%s=", name);
            b = (const uint8_t *)&env_buf[k];
            dump_hex_f(b, (nxt-k), DUMP_ASCII);
            break;
        }
        if (k < 0)
            printk(KERN_ERR "## Error: \"%s\" not defined\n", name);

        return 0;
    }
}

#endif /*CONFIG_CIRRUS_DUAL_MTD_ENV*/

#ifdef CONFIG_CIRRUS_DUAL_MTD_ENV
int rtcnvet_get_env(char *name, char **buf) {
    struct env_data ed;
    int j, k, nxt;
    char *p;
    char *q;

#ifdef DEBUGME
    printk(KERN_ERR\
        "********* %s called! ***********\n",\
        __func__);
#endif /*DEBUGME*/

    /*sanity*/
    if (NULL == buf || NULL == name) {
        printk(KERN_ERR\
            "%s: Called with invalid data!\n",\
            __func__);
        return -1;
    }
    if (NULL == name) {
        *buf = NULL;
        printk(KERN_ERR\
            "%s: Returnig error, no name provided!\n",\
            __func__);
        return -2;
    }

    /*read env*/
    if (0 == env_read(&ed)) {
#ifdef DEBUGME
        printk(KERN_ERR\
            "%s: Env processing!\n",\
            __func__);
#endif /*DEBUGME*/

        /*get env*/
        k = -1;
        nxt = 0;
        p = ed.data + ENV_HEADER_SIZE; 

        for (j = 0; *(p+j) != '\0' && j+ENV_HEADER_SIZE < ed.size; j = nxt+1) {
            for (nxt = j; *(p+nxt) != '\0' && nxt+ENV_HEADER_SIZE < ed.size; ++nxt);
            /* search name in *(p+j) ~ key=value*/
            if (j+ENV_HEADER_SIZE < ed.size) {
                q = strstr(p+j, "=");
                if (NULL != q) {
                    *q = 0;
                    if (0 == strcmp(name, p+j)) {
                        k = q-p+1;
                        *q = '=';
                        break;
                    }
                    *q = '=';
                } else {
                    printk(KERN_ERR\
                        "%s: \"%s\" is not in the key=value format!\n",\
                        __func__, p+j);
                }
            }
        }

        if (k < 0) {
            printk(KERN_ERR\
                "%s: Error: \"%s\" not defined!\n",\
                __func__, name);
            *buf = NULL;
        } else {
            nxt = strlen(p+k);
            nxt++;
            *buf=kmalloc(nxt,GFP_KERNEL);
            strncpy(*buf, p+k, nxt);
#ifdef DEBUGME
            printk(KERN_ERR\
                "%s: Got: \"%s\"!\n",\
                __func__, *buf);
#endif /*DEBUGME*/
        }

        /* tear down ed*/

#ifdef DEBUGME
        printk(KERN_ERR\
            "%s: Tearing down data!\n",\
             __func__);
#endif /*DEBUGME*/
        ed.mtd = -1;
        kfree(ed.data);
        ed.data = NULL;
        put_mtd_device(ed.mtds[0]); ed.mtds[0] = NULL;
        put_mtd_device(ed.mtds[1]); ed.mtds[1] = NULL;
#ifdef DEBUGME
        printk(KERN_ERR\
            "%s: Tearing down complete!\n",\
             __func__);
#endif /*DEBUGME*/
        return (*buf) ? 0 : -1;
    } else {
        printk(KERN_ERR\
            "%s: Failed to read the env block!\n",\
            __func__);
        return -3;
    }
    return 0;
}
#else /*CONFIG_CIRRUS_DUAL_MTD_ENV*/
int rtcnvet_get_env(char *name, char **buf)
{
    int j, k, nxt;

    if (!env_valid) {
        *buf = NULL;
        return -1;
    }

    k = -1;

    for (j = 0; env_get_char(j) != '\0'; j = nxt+1) {

        for (nxt = j; env_get_char(nxt) != '\0'; ++nxt)
            ;
        k = envmatch((unsigned char *)name, j);
        if (k < 0)
            continue;

        break;
    }

    if (k < 0) {
        printk(KERN_ERR "## Error: \"%s\" not defined\n", name);
        *buf = NULL;
    } else
        *buf = &env_buf[k];

    return (*buf) ? 0 : -1;
}
#endif /*CONFIG_CIRRUS_DUAL_MTD_ENV*/
EXPORT_SYMBOL(rtcnvet_get_env);

#ifndef CONFIG_CIRRUS_DUAL_MTD_ENV
static int envmatch(unsigned char *s1, int i2)
{

    while (*s1 == env_get_char(i2++))
        if (*s1++ == '=')
            return i2;
    if (*s1 == '\0' && env_get_char(i2-1) == '=')
        return i2;
    return -1;
}
#endif /*CONFIG_CIRRUS_DUAL_MTD_ENV*/

#ifdef CONFIG_CIRRUS_DUAL_MTD_ENV
int rtcnvet_do_setenv(char *name, char *val) {
    struct env_data ed;
    char *p, *q;
    int j, k, nxt;
    unsigned int crc32;
    int len;

#ifdef DEBUGME
    printk(KERN_ERR\
        "********* %s called! ***********\n",\
        __func__);
#endif /*DEBUGME*/
	
    /*sanity*/
    if (NULL == name) {
        printk(KERN_ERR\
            "%s: Called with incorrect data!\n",
            __func__);
        return -1;
    }
    if (strchr(name, '=')) {
        printk(KERN_ERR\
            "%s: Illegal character '=' in variable name \"%s\"\n",\
            __func__, name);
        return -2;
    }

    /*read env*/
    if (0 == env_read(&ed)) {
#ifdef DEBUGME
    printk(KERN_ERR\
        "%s: Env read, processing!\n",\
        __func__);
#endif /*DEBUGME*/
        /* find prev. value, if any 
         * If found, move env over it!
         */
        k = -1;
        nxt = 0;
        p = ed.data + ENV_HEADER_SIZE;

        for (j = 0; *(p+j) != '\0' && j+ENV_HEADER_SIZE < ed.size; j = nxt+1) {
            for (nxt = j; *(p+nxt) != '\0' && nxt+ENV_HEADER_SIZE < ed.size; ++nxt);
            /* search name in *(p+j) ~ key=value*/
            if (j+ENV_HEADER_SIZE < ed.size) {
                q = strstr(p+j, "=");
                if (NULL != q) {
                    *q = 0;
                    if (0 == strcmp(name, p+j)) {
                        /* remove it! */
                        *q = '=';
                        k = 1;
                        break;
                    }
                    *q = '=';
                }
            }
        }
        if ( 1 == k) {
#ifdef DEBUGME
            printk(KERN_ERR\
                "%s: Prev. value found, removing...!\n",\
                __func__);
#endif /*DEBUGME*/
            /* p+j - start of env */
            /* p+nxt - start of next env - 1  */
            nxt = nxt + 1;
            /* last thing on the env? */
            if (*(p+nxt) == '\0') {
                *(p+j) = '\0';
            } else {
                for (;;) {
                    *(p+j) = *(p+nxt);
                    nxt++;
                    if ((*(p+j) == '\0') && (*(p+nxt) == '\0')) break;
                    j++;
                }
            }
            j++;
            *(p+j) = '\0';
        } 
        /* set something? */
        if (NULL != val) {
#ifdef DEBUGME
            printk(KERN_ERR\
                "%s: Inserting a new value!\n",\
                __func__);
#endif /*DEBUGME*/
            for (j = 0; (*(p+j) || *(p+j+1)) && (j < ed.size - ENV_HEADER_SIZE); j++);
            if (j > 0 && j < ed.size - ENV_HEADER_SIZE) j++;
            /* j is before the last 0 */
            /* check for overflow */
            /* name + "=" + val + "\0\0" > ed.size - ENV_HEADER_SIZE - j */
            len = strlen(name);
            len = len + 1;
            len = strlen(val);
            len = len + 2;
            if (len > ed.size - ENV_HEADER_SIZE - j) {
                printk(KERN_ERR \
                    "%s: Environment overflow, \"%s\" deleted!\n",\
                    __func__, name);
            } else {
                /* set it */
                while ((*(p+j) = *name++) != '\0') j++;
                *(p+j) = '=';
                j++;
                while ((*(p+j) = *val++) != '\0') j++;
                /* end is marked with double '\0' */
                j++;
                *(p+j) = '\0';
            }
        }

#ifdef DEBUGME
        printk(KERN_ERR\
            "%s: Env changed, updating CRC!\n",\
            __func__);
#endif /*DEBUGME*/
        /* update crc now */
        crc32 = ubcrc32(0, &ed.data[ENV_HEADER_SIZE], ed.size - ENV_HEADER_SIZE);
        *((uint32_t *)&(ed.data[0])) = crc32;
        env_save(&ed);
        /* ed is not valid anymore! */
	// return 0;
    } else {
        printk(KERN_ERR\
            "%s: Cannot find a valid env!\n",\
            __func__);
        return -1;
    }
#ifdef DEBUGME
    printk(KERN_ERR\
        "%s: All done, exiting!\n",\
        __func__);
#endif /*DEBUGME*/
    return 0;
}
#else /*CONFIG_CIRRUS_DUAL_MTD_ENV*/
int rtcnvet_do_setenv(char *name, char *val)
{
    int len, oldval;
    unsigned char *env, *nxt = NULL;
    unsigned char *env_data = NULL;

    rtcnvet_reload_env();
    env_data = (unsigned char *)&env_buf[0];

    if (!env_valid)
        return -1;

    if (strchr(name, '=')) {
        printk(KERN_ERR \
        "Illegal character '=' in variable name \"%s\"\n", name);
        return -2;
    }

    /*
     * search if variable with this name already exists
     */
    oldval = -1;
    for (env = env_data; *env; env = nxt+1) {
        for (nxt = env; *nxt; ++nxt)
            ;
        oldval = envmatch((unsigned char *)name, env-env_data);
        if (oldval >= 0)
            break;
    }


    /*
     * Delete any existing definition
     */
    if (oldval >= 0) {
        if (*++nxt == '\0')
            if (env > env_data) {
                env--;
            } else {
                *env = '\0';
        } else {
            for (;;) {
                *env = *nxt++;
                if ((*env == '\0') && (*nxt == '\0'))
                    break;
                ++env;
            }
        }
        *++env = '\0';
    }

    /*
     * Append new definition at the end
     */
    for (env = env_data; *env || *(env+1); ++env)
        ;
    if (env > env_data)
        ++env;
    /*
     * Overflow when:
     * "name" + "=" + "val" +"\0\0"  > ENV_SIZE - (env-env_data)
     */
    len = strlen(name) + 2;
    /* add '=' for first arg, ' ' for all others */
    len += strlen(val) + 1;
    if (len > (&env_data[env_size]-env)) {
        printk(KERN_ERR \
        "Environment overflow, \"%s\" deleted\n", name);
        return 1;
    }
    while ((*env = *name++) != '\0')
        env++;

    *env = '=';
    while ((*++env = *val++) != '\0')
        ;

    /* end is marked with double '\0' */
    *++env = '\0';

    /* Update CRC */
    env_crc_update();

    /* save every time */
    env_save();

    return 0;
}
#endif /*CONFIG_CIRRUS_DUAL_MTD_ENV*/
EXPORT_SYMBOL(rtcnvet_do_setenv);

#ifndef CONFIG_CIRRUS_DUAL_MTD_ENV
void rtcnvet_test(void)
{
    int ii;

    rtcnvet_do_setenv(RTC_LOG_NAME, "12345ABCDE");
    for (ii = 0; ii <= 512; ii += 16)
        dump_hex_f((uint8_t *)&env_buf[ii], 16, DUMP_ALL);
}

void rtcnvet_reload_env(void)
{
    rtcnvet_exit();
    rtcnvet_init();
}
EXPORT_SYMBOL(rtcnvet_reload_env);

#endif /*CONFIG_CIRRUS_DUAL_MTD_ENV*/

#ifdef CONFIG_CIRRUS_DUAL_MTD_ENV
/* will fill ed from the good partition
 * if no env is ok, it will not try to correct this!
 * read NAME1, do crc1, extract flag1
 * read NAME2, do crc2, extract flag2
 * if (crc1 && !crc2) env=1; stop
 * if (!crc1 && crc2) env=2; stop
 * if (!crc1 && !crc2) env=0; stop
 * if (crc1 && crc2) {
 *    if (flag1 A && flag2 O) env=1; stop
 *    if (flag1 O && flag2 A) env=2; stop
 *    if (flag1 == flag2) env=1; stop
 *    if (flag1 == 0xff) env=1; stop
 *    if (flag2 = 0xff) env=2; stop
 *    env=0; stop
 * }
 * rtcnved_mtd
 */
static int env_read(struct env_data *ed) {
    size_t retlen = 0;
    unsigned int    crc1, crc2;
    unsigned char flag1, flag2;
    struct mtd_info *mtd1;
    struct mtd_info *mtd2;
    int    config_env_size1, config_env_size2;
    int    env_size1, env_size2;
    unsigned char *env_buf1;   
    unsigned char *env_buf2;   
    int env = -1;

#ifdef DEBUGME
    printk(KERN_ERR\
        "********* %s called! ***********\n",\
        __func__);
#endif /*DEBUGME*/

    /*sanity check*/
    if (NULL == ed) {
        printk(KERN_ERR\
            "%s: Incorrect function call!\n",\
            __func__);
        return 1;
    }

    flag1 = 0;
    flag2 = 0;

#ifdef DEBUGME
    printk(KERN_ERR\
        "%s: About to read first partition!\n",\
        __func__);
#endif /*DEBUGME*/
    
    /* First partition */
    mtd1 = get_mtd_device_nm(UBOOTENV_MTD_NAME1);

    if (IS_ERR(mtd1)) {
        printk(KERN_ERR\
            "%s: Not found this MTD(%s)\n",\
            __func__,UBOOTENV_MTD_NAME1);
        return 1;
    }

    config_env_size1 = mtd1->size;
    env_size1 = config_env_size1 - ENV_HEADER_SIZE;
    env_buf1 = kmalloc(config_env_size1, GFP_KERNEL);
    mtd_read(mtd1, 0, config_env_size1, &retlen, env_buf1);

    if (retlen != config_env_size1) {
        printk(KERN_ERR\
            "%s: Failed to read data from partition!\n",\
            __func__);
        return 1;
    }

    crc1 = ubcrc32(0, &env_buf1[ENV_HEADER_SIZE], env_size1);
    if (*((uint32_t *)&env_buf1[0]) != crc1) {
        crc1 = 0;
#ifdef DEBUGME
        printk(KERN_ERR\
            "%s: CRC is not ok on partition one!\n",\
            __func__);
#endif /*DEBUGME*/
    } else {
        crc1 = 1;
        flag1 = env_buf1[ENV_HEADER_SIZE - 1];
#ifdef DEBUGME
        printk(KERN_ERR\
            "%s: CRC is ok on partition one!\n",\
            __func__);
#endif /*DEBUGME*/
    }

    /* Second partition */
#ifdef DEBUGME
    printk(KERN_ERR\
        "%s: About to read second partition!\n",\
        __func__);
#endif /*DEBUGME*/
 
    mtd2 = get_mtd_device_nm(UBOOTENV_MTD_NAME2);

    if (IS_ERR(mtd2)) {
        printk(KERN_ERR\
            "%s: Not found this MTD(%s)\n",\
            __func__, UBOOTENV_MTD_NAME2);
        kfree(env_buf1);
        put_mtd_device(mtd1);
        return 1;
    }

    config_env_size2 = mtd2->size;
    env_size2 = config_env_size2 - ENV_HEADER_SIZE;
    env_buf2 = kmalloc(config_env_size2, GFP_KERNEL);
    mtd_read(mtd2, 0, config_env_size2, &retlen, env_buf2);
    
    if (retlen != config_env_size2) {
        printk(KERN_ERR\
            "%s: Failed to read data from partition!\n",\
            __func__);
        kfree(env_buf1);
        put_mtd_device(mtd1);
        kfree(env_buf2);
        put_mtd_device(mtd2);
        return 1;
    }

    crc2 = ubcrc32(0, &env_buf2[ENV_HEADER_SIZE], env_size2);
    if (*((uint32_t *)&env_buf2[0]) != crc2) {
        crc2 = 0;
#ifdef DEBUGME
        printk(KERN_ERR\
            "%s: CRC is not ok on partition two!\n",\
            __func__);
#endif /*DEBUGME*/
    } else {
        crc2 = 1;
        flag2 = env_buf2[ENV_HEADER_SIZE - 1];
#ifdef DEBUGME
        printk(KERN_ERR\
            "%s: CRC is ok on partition two!\n",\
            __func__);
#endif /*DEBUGME*/
    }

    /* do the dance */
    if (crc1 && !crc2) {
        env = 0;
    } else if (!crc1 && crc2) {
        env = 1;
    } else if (!crc1 && !crc2) {
        /* 2 bad crcs! */
        printk(KERN_ERR \
            "%s: Not a single partition is good for env!\n",\
            __func__);
        env = -1;
    } else {
        if (flag1 == active_flag && flag2 == obsolete_flag) {
            env = 0;
        } else if (flag1 == obsolete_flag && flag2 == active_flag) {
            env = 1;
        } else if (flag1 == flag2) {
            env = 0;
        } else if (flag1 == 0xff) {
            env = 0;
        } else if (flag2 == 0xff) {
            env = 1;
        } else {
            /* 2 bad flags!*/
            env = 0;
        }
    }

    if (-1 == env) {
        printk(KERN_ERR\
            "%s: No good env found, giving up!\n",\
            __func__);

        kfree(env_buf1);
        kfree(env_buf2);
        put_mtd_device(mtd1);
        put_mtd_device(mtd2);
        ed->size = 0;
        ed->data = NULL;
        ed->mtds[0] = NULL;
        ed->mtds[1] = NULL;
        return 1;
    }

    ed->mtd = env;  

    if (0 == env) {
        kfree(env_buf2);
        ed->size = config_env_size1;
        ed->data = env_buf1;
    }
    if (1 == env) {
        kfree(env_buf1);
        ed->size = config_env_size2;
        ed->data = env_buf2;
    }

    ed->mtds[0] = mtd1;
    ed->mtds[1] = mtd2;

#ifdef DEBUGME
    printk(KERN_ERR\
        "%s: Read in env %d as good!\n",\
        __func__, env);
#endif /*DEBUGME*/
    return 0;
}
/* nothing to init, always read! */
void rtcnvet_init(void)
{
#ifdef DEBUGME
    printk(KERN_ERR\
        "********* %s called! ***********\n",\
        __func__);
#endif /*DEBUGME*/
    return;
}
#else /*CONFIG_CIRRUS_DUAL_MTD_ENV*/
void rtcnvet_init(void)
{
    size_t retlen = 0;
    int _crc32;

    rtcnved_mtd = get_mtd_device_nm(UBOOTENV_MTD_NAME);

    if (IS_ERR(rtcnved_mtd)) {
        printk(KERN_ERR \
        "Not found this MTD(%s)\r\n", UBOOTENV_MTD_NAME);
        return;
    }

    /*
        #define ENV_SIZE (CONFIG_ENV_SIZE - ENV_HEADER_SIZE)
        Assume mtd->size is CONFIG_ENV_SIZE in uboot
     */
    config_env_size = rtcnved_mtd->size;
    env_size = config_env_size - ENV_HEADER_SIZE;
    env_buf = kmalloc(config_env_size, GFP_KERNEL);
    mtd_read(rtcnved_mtd, 0, config_env_size, &retlen, env_buf);

    _crc32 = ubcrc32(0, &env_buf[ENV_HEADER_SIZE], env_size);
    if (*((uint32_t *)&env_buf[0]) != _crc32) {
        printk(KERN_ERR \
            "%s: invalid env, crc check failed! 08%x vs ", \
            __func__, _crc32);
        dump_hex_f((uint8_t *)&env_buf[0], 4, DUMP_HEX);
    } else
        env_valid = 1;

    return;
}
#endif /*CONFIG_CIRRUS_DUAL_MTD_ENV*/
EXPORT_SYMBOL(rtcnvet_init);

#ifdef CONFIG_CIRRUS_DUAL_MTD_ENV
void rtcnvet_exit(void)
{
#ifdef DEBUGME
    printk(KERN_ERR\
        "********* %s called! ***********\n",\
        __func__);
#endif /*DEBUGME*/
    return;
}
#else /*CONFIG_CIRRUS_DUAL_MTD_ENV*/
void rtcnvet_exit(void)
{
    env_valid = 0;
    kfree(env_buf);
}
#endif /*CONFIG_CIRRUS_DUAL_MTD_ENV*/
EXPORT_SYMBOL(rtcnvet_exit);

