#pragma once

#define NOP 0x00
#define SWRESET 0x01
#define RDDID 0x04
#define RDDST 0x09

#define SLPIN 0x10
#define SLPOUT 0x11
#define PTLON 0x12
#define NORON 0x13

#define INVOFF 0x20
#define INVON 0x21
#define DISPOFF 0x28
#define DISPON 0x29
#define CASET 0x2A
#define RASET 0x2B
#define RAMWR 0x2C
#define RAMRD 0x2E

#define PTLAR 0x30
#define TEOFF 0x34
#define TEON 0x35
#define MADCTL 0x36
#define COLMOD 0x3A
#define COLMOD_16BIT 0x55
#define PORCTRL 0xB2
#define PORCTRL_DISABLE 0x00

#define MADCTL_MY 0x80
#define MADCTL_MX 0x40
#define MADCTL_MV 0x20
#define MADCTL_ML 0x10
#define MADCTL_RGB 0x00

#define GCTRL 0xB7
#define VGH_13_65V 0x40
#define VGL_10_43V 0x05

#define VCOMS 0xBB
#define VCOMS_1_175V 0x2B

#define LCMCTRL 0xC0
#define LCM_XMY 0x40
#define LCM_XBGR 0x20
#define LCM_XINV 0x10
#define LCM_XMX 0x08
#define LCM_XMH 0x04
#define LCM_XMV 0x02
#define LCM_XGS 0x01

#define VDVVRHEN 0xC2
#define VDVVRH_CMDEN 0x01
#define VDVVRH_DUMMY 0xFF
#define VRHS 0xC3
#define VRH_4_4V 0x11
#define VDVS 0xC4
#define VDV_0V 0x20

#define FRCTRL2 0xC6
#define FR_60HZ 0x0F

#define PWCTRL1 0xD0
#define PWCTRL_DUMMY 0xA4
#define AVDD_6_8V 0x80
#define AVCL_4_8V 0x20
#define VDDS_2_3V 0x01

#define PVGAMCTRL 0xE0
#define NVGAMCTRL 0xE1

#define RDID1 0xDA
#define RDID2 0xDB
#define RDID3 0xDC
#define RDID4 0xDD