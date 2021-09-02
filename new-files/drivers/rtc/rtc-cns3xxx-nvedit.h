/*******************************************************************************
 *
 *  drivers/rtc/rtc-cns3xxx-nvedit.h
 *
 *  Real Time Clock driver for the CNS3XXX SOCs
 *
 *  Author: Ange Chang
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

#ifndef __RTC_NVEDIT_H__
#define __RTC_NVEDIT_H__

#ifdef CONFIG_CIRRUS_DUAL_MTD_ENV
/* dual partitions */
#define UBOOTENV_MTD_NAME1	"SPI-UBootEnv1"
#define UBOOTENV_MTD_NAME2  "SPI-UBootEnv2"
#else /*CONFIG_CIRRUS_DUAL_MTD_ENV*/
#define UBOOTENV_MTD_NAME1	"SPI-UBootEnv"
#endif /*CONFIG_CIRRUS_DUAL_MTD_ENV*/

#define RTC_LOG_NAME		"rtclog"

/* test used */
#define MTD_READ_LEN		512

#ifdef CFG_REDUNDAND_ENVIRONMENT
# define ENV_HEADER_SIZE	(sizeof(uint32_t) + 1)
#else
# define ENV_HEADER_SIZE	(sizeof(uint32_t))
#endif

#ifdef CONFIG_CIRRUS_DUAL_MTD_ENV
/* CRC32 + 1 flag */
#undef ENV_HEADER_SIZE
#define ENV_HEADER_SIZE (sizeof(uint32_t) + 1)
#endif /*CONFIG_CIRRUS_DUAL_MTD_ENV*/

#define DO1(buf) crc = crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  do { DO1(buf); DO1(buf); } while (0);
#define DO4(buf)  do { DO2(buf); DO2(buf); } while (0);
#define DO8(buf)  do { DO4(buf); DO4(buf); } while (0);

#define DUMP_HEX       (1<<0)
#define DUMP_ASCII     (1<<1)
#define DUMP_ALL       (DUMP_HEX | DUMP_ASCII)

#ifdef CONFIG_CIRRUS_DUAL_MTD_ENV

void rtcnvet_init(void);
int rtcnvet_do_setenv(char *name, char *val);
int rtcnvet_get_env(char *name, char **buf);
void rtcnvet_exit(void);

#else /*CONFIG_CIRRUS_DUAL_MTD_ENV*/

#define env_get_char(idx)		env_buf[idx]

void rtcnvet_init(void);
void rtcnvet_reload_env(void);
int rtcnvet_do_printenv(char *name);
int rtcnvet_do_setenv(char *name, char *val);
int rtcnvet_get_env(char *name, char **buf);
void rtcnvet_exit(void);

void rtcnvet_test(void);
#endif /* CONFIG_CIRRUS_DUAL_MTD_ENV */

#endif /* __RTC_NVEDIT_H__ */

