/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "../../common.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */

/* Definitions of physical drive number for each drive */
#define DEV_SD		0

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

extern "C" DSTATUS disk_status (
    BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
    return 0;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

extern "C" DSTATUS disk_initialize (
    BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
    switch (pdrv)
    {
        case DEV_SD:
        {
            if (!gSdCard.TryInitialize())
            {
                return STA_NOINIT;
            }
            return 0;
        }
    }
    return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

extern "C" DRESULT __time_critical_func(disk_read) (
    BYTE pdrv,		/* Physical drive nmuber to identify the drive */
    BYTE *buff,		/* Data buffer to store read data */
    LBA_t sector,	/* Start sector in LBA */
    UINT count		/* Number of sectors to read */
)
{
    DRESULT res;
    int result;

    switch (pdrv)
    {
        case DEV_SD:
        {
            while (!gSdCard.TryReadSectorsSync(buff, sector, count));
            return RES_OK;
        }
    }

    return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

extern "C" DRESULT disk_write (
    BYTE pdrv,			/* Physical drive nmuber to identify the drive */
    const BYTE *buff,	/* Data to be written */
    LBA_t sector,		/* Start sector in LBA */
    UINT count			/* Number of sectors to write */
)
{
    DRESULT res;
    int result;

    switch (pdrv)
    {
        case DEV_SD:
        {
            while (!gSdCard.TryWriteSectorsSync(buff, sector, count));
            return RES_OK;
        }
    }

    return RES_PARERR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

extern "C" DRESULT disk_ioctl (
    BYTE pdrv,		/* Physical drive nmuber (0..) */
    BYTE cmd,		/* Control code */
    void *buff		/* Buffer to send/receive control data */
)
{
    return RES_PARERR;
}

