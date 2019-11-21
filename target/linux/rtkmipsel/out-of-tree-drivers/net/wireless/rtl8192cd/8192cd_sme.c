/*
 *   Handling routines for 802.11 SME (Station Management Entity)
 *
 *  $Id: 8192cd_sme.c,v 1.90.2.36 2011/01/10 07:49:07 jerryko Exp $
 *
 *  Copyright (c) 2009 Realtek Semiconductor Corp.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#define _8192CD_SME_C_

#include <linux/list.h>
#include <linux/random.h>
#include "./8192cd_cfg.h"
#include "./8192cd.h"
#include "./wifi.h"
#include "./8192cd_hw.h"
#include "./8192cd_headers.h"
#include "./8192cd_rx.h"
#include "./8192cd_debug.h"
#include "./8192cd_psk.h"
#include "./8192cd_security.h"

#include "8192cd_cfg80211.h"

#ifdef WIFI_SIMPLE_CONFIG

#ifdef INCLUDE_WPS
#include "./wps/wsc.h"
#endif

#define TAG_REQUEST_TYPE	0x103a
#define TAG_RESPONSE_TYPE	0x103b

/*add for WPS2DOTX*/
#define TAG_VERSION2		0x1067
#define TAG_VENDOR_EXT		0x1049
#define VENDOR_VERSION2 	0x00
unsigned char WSC_VENDOR_OUI[3] = {0x00, 0x37, 0x2a};
/*add for WPS2DOTX*/

#define MAX_REQUEST_TYPE_NUM 0x3
UINT8 WSC_IE_OUI[4] = {0x00, 0x50, 0xf2, 0x04};
UINT8 NEC_OUI[3] = {0x00, 0x0D, 0x02};
#endif

#ifdef WIFI_WMM
unsigned char WMM_IE[] = {0x00, 0x50, 0xf2, 0x02, 0x00, 0x01};
unsigned char WMM_PARA_IE[6] = {0x00, 0x50, 0xf2, 0x02, 0x01, 0x01};/*cfg p2p cfg p2p*/
#endif

#ifdef CONFIG_IEEE80211W_CLI
#include "./sha256.h"
#endif

#ifdef CONFIG_IEEE80211V
#include "./8192cd_11v.h"
#endif
#define INTEL_OUI_NUM	202
unsigned char INTEL_OUI[INTEL_OUI_NUM][3] =
{{0x00, 0x02, 0xb3}, {0x00, 0x03, 0x47},
{0x00, 0x04, 0x23}, {0x00, 0x07, 0xe9},
{0x00, 0x0c, 0xf1}, {0x00, 0x0e, 0x0c},
{0x00, 0x0e, 0x35}, {0x00, 0x11, 0x11},
{0x00, 0x11, 0x75}, {0x00, 0x12, 0xf0},
{0x00, 0x13, 0x02}, {0x00, 0x13, 0x20},
{0x00, 0x13, 0xce}, {0x00, 0x13, 0xe8},
{0x00, 0x15, 0x00}, {0x00, 0x15, 0x17},
{0x00, 0x16, 0x6f}, {0x00, 0x16, 0x76},
{0x00, 0x16, 0xea}, {0x00, 0x16, 0xeb},
{0x00, 0x17, 0x35}, {0x00, 0x18, 0xde},
{0x00, 0x19, 0xd1}, {0x00, 0x19, 0xd2},
{0x00, 0x1b, 0x21}, {0x00, 0x1b, 0x77},
{0x00, 0x1c, 0xbf}, {0x00, 0x1c, 0xc0},
{0x00, 0x1d, 0x40}, {0x00, 0x1d, 0xe0},
{0x00, 0x1d, 0xe1}, {0x00, 0x1e, 0x64},
{0x00, 0x1e, 0x65}, {0x00, 0x1e, 0x67},
{0x00, 0x1f, 0x3b}, {0x00, 0x1f, 0x3c},
{0x00, 0x20, 0x7b}, {0x00, 0x21, 0x5c},
{0x00, 0x21, 0x5d}, {0x00, 0x21, 0x6a},
{0x00, 0x21, 0x6b}, {0x00, 0x22, 0xfa},
{0x00, 0x22, 0xfb}, {0x00, 0x23, 0x14},
{0x00, 0x23, 0x15}, {0x00, 0x24, 0xd6},
{0x00, 0x24, 0xd7}, {0x00, 0x26, 0xc6},
{0x00, 0x26, 0xc7}, {0x00, 0x27, 0x0e},
{0x00, 0x27, 0x10}, {0x00, 0x28, 0xf8},
{0x00, 0x50, 0xf1}, {0x00, 0x90, 0x27},
{0x00, 0xa0, 0xc9}, {0x00, 0xaa, 0x00},
{0x00, 0xaa, 0x01}, {0x00, 0xaa, 0x02},
{0x00, 0xc2, 0xc6}, {0x00, 0xd0, 0xb7},
{0x00, 0xdb, 0xdf}, {0x08, 0x11, 0x96},
{0x08, 0xd4, 0x0c}, {0x0c, 0x8b, 0xfd},
{0x0c, 0xd2, 0x92}, {0x10, 0x02, 0xb5},
{0x10, 0x0b, 0xa9}, {0x10, 0x4a, 0x7d},
{0x10, 0xf0, 0x05}, {0x14, 0xab, 0xc5},
{0x18, 0x3d, 0xa2}, {0x18, 0x5e, 0x0f},
{0x18, 0xff, 0x0f}, {0x24, 0x77, 0x03},
{0x28, 0x16, 0xad}, {0x28, 0xb2, 0xbd},
{0x2c, 0x6e, 0x85}, {0x30, 0x3a, 0x64},
{0x30, 0xe3, 0x7a}, {0x34, 0x02, 0x86},
{0x34, 0x13, 0xe8}, {0x34, 0xde, 0x1a},
{0x34, 0xe6, 0xad}, {0x34, 0xf3, 0x9a},
{0x3c, 0xa9, 0xf4}, {0x3c, 0xf8, 0x62},
{0x3c, 0xfd, 0xfe}, {0x40, 0x25, 0xc2},
{0x44, 0x85, 0x00}, {0x48, 0x45, 0x20},
{0x48, 0x51, 0xb7}, {0x4c, 0x34, 0x88},
{0x4c, 0x79, 0xba}, {0x4c, 0x80, 0x93},
{0x4c, 0xeb, 0x42}, {0x50, 0x2d, 0xa2},
{0x58, 0x91, 0xcf}, {0x58, 0x94, 0x6b},
{0x58, 0xa8, 0x39}, {0x58, 0xfb, 0x84},
{0x5c, 0x51, 0x4f}, {0x5c, 0xc5, 0xd4},
{0x5c, 0xd2, 0xe4}, {0x5c, 0xe0, 0xc5},
{0x60, 0x36, 0xdd}, {0x60, 0x57, 0x18},
{0x60, 0x67, 0x20}, {0x60, 0x6c, 0x66},
{0x64, 0x80, 0x99}, {0x64, 0xd4, 0xda},
{0x68, 0x05, 0xca}, {0x68, 0x07, 0x15},
{0x68, 0x17, 0x29}, {0x68, 0x5d, 0x43},
{0x6c, 0x29, 0x95}, {0x6c, 0x88, 0x14},
{0x6c, 0xa1, 0x00}, {0x70, 0x1c, 0xe7},
{0x74, 0xe5, 0x0b}, {0x78, 0x0c, 0xb8},
{0x78, 0x92, 0x9c}, {0x78, 0xff, 0x57},
{0x7c, 0x5c, 0xf8}, {0x7c, 0x67, 0xa2},
{0x7c, 0x7a, 0x91}, {0x7c, 0xb0, 0xc2},
{0x7c, 0xcc, 0xb8}, {0x80, 0x00, 0x0b},
{0x80, 0x19, 0x34}, {0x80, 0x86, 0xf2},
{0x80, 0x9b, 0x20}, {0x84, 0x3a, 0x4b},
{0x84, 0x68, 0x3e}, {0x84, 0xa6, 0xc8},
{0x84, 0xef, 0x18}, {0x88, 0x53, 0x2e},
{0x88, 0x78, 0x73}, {0x8c, 0x70, 0x5a},
{0x8c, 0xa9, 0x82}, {0x90, 0x2e, 0x1c},
{0x90, 0x49, 0xfa}, {0x90, 0xe2, 0xba},
{0x94, 0x65, 0x9c}, {0x98, 0x4f, 0xee},
{0x98, 0x54, 0x1b}, {0x9c, 0x4e, 0x36},
{0xa0, 0x36, 0x9f}, {0xa0, 0x88, 0x69},
{0xa0, 0x88, 0xb4}, {0xa0, 0xa8, 0xcd},
{0xa0, 0xc5, 0x89}, {0xa0, 0xd3, 0x7a},
{0xa4, 0x02, 0xb9}, {0xa4, 0x34, 0xd9},
{0xa4, 0x4e, 0x31}, {0xa4, 0xbf, 0x01},
{0xa4, 0xc4, 0x94}, {0xac, 0x2b, 0x6e},
{0xac, 0x72, 0x89}, {0xac, 0x7b, 0xa1},
{0xac, 0xfd, 0xce}, {0xb4, 0x6d, 0x83},
{0xb4, 0x96, 0x91}, {0xb4, 0xb6, 0x76},
{0xb4, 0xd5, 0xbd}, {0xb8, 0x03, 0x05},
{0xb8, 0x08, 0xcf}, {0xb8, 0x81, 0x98},
{0xb8, 0x8a, 0x60}, {0xb8, 0xb8, 0x1e},
{0xb8, 0xbf, 0x83}, {0xbc, 0x0f, 0x64},
{0xbc, 0x77, 0x37}, {0xbc, 0xa8, 0xa6},
{0xc4, 0x85, 0x08}, {0xc4, 0xd9, 0x87},
{0xc8, 0x21, 0x58}, {0xc8, 0x34, 0x8e},
{0xc8, 0xf7, 0x33}, {0xcc, 0x3d, 0x82},
{0xd0, 0x57, 0x7b}, {0xd0, 0x7e, 0x35},
{0xd8, 0xfc, 0x93}, {0xdc, 0x53, 0x60},
{0xdc, 0xa9, 0x71}, {0xe0, 0x94, 0x67},
{0xe0, 0x9d, 0x31}, {0xe4, 0x02, 0x9b},
{0xe4, 0xa4, 0x71}, {0xe4, 0xa7, 0xa0},
{0xe4, 0xb3, 0x18}, {0xe4, 0xf8, 0x9c},
{0xe4, 0xfa, 0xfd}, {0xe8, 0x2a, 0xea},
{0xe8, 0xb1, 0xfc}, {0xf0, 0x42, 0x1c},
{0xf0, 0xd5, 0xbf}, {0xf4, 0x06, 0x69},
{0xf4, 0x8c, 0x50}, {0xf8, 0x16, 0x54},
{0xf8, 0x63, 0x3f}, {0xfc, 0xf8, 0xae}};

#define HTC_OUI_NUM	17
unsigned char HTC_OUI[][3]= {
	{0x00,0x09,0x2D},
	{0x00,0x23,0x76},
	{0x18,0x87,0x96},
	{0x1C,0xB0,0x94},
	{0x38,0xE7,0xD8},
	{0x64,0xA7,0x69},
	{0x7C,0x61,0x93},
	{0x84,0x7A,0x88},
	{0x90,0x21,0x55},
	{0x98,0x0D,0x2E},
	{0xA0,0xF4,0x50},
	{0xA8,0x26,0xD9},
	{0xBC,0xCF,0xCC},
	{0xD4,0x20,0x6D},
	{0xD8,0xB3,0x77},
	{0xE8,0x99,0xC4},
	{0xF8,0xDB,0x7F}
};


#define PSP_OUI_NUM	51
unsigned char PSP_OUI[PSP_OUI_NUM][3] =
{{0x04, 0x76, 0x6E},
{0x00, 0x26, 0x43},
{0x79, 0xC9, 0x74},
{0x8C, 0x7C, 0xB5},
{0x78, 0xDD, 0x08},
{0x50, 0x63, 0x13},
{0x2C, 0x81, 0x58},
{0x66, 0x60, 0xEC},
{0x5C, 0x6D, 0x20},
{0x00, 0x06, 0xF5},
{0xC0, 0x14, 0x3D},
{0x46, 0x8F, 0x25},
{0xD4, 0x4B, 0x5E},
{0x00, 0xD4, 0x4B},
{0x90, 0x34, 0xFC},
{0x4C, 0x0F, 0x6E},
{0xF0, 0xF0, 0x02},
{0x00, 0x07, 0x04},
{0x00, 0x22, 0xCF},
{0x00, 0x06, 0xF7},
{0x60, 0xF4, 0x94},
{0x0C, 0xEE, 0xE6},
{0x00, 0x1D, 0xD9},
{0x00, 0x1C, 0x26},
{0x00, 0x1B, 0xFB},
{0x00, 0x19, 0x7F},
{0x00, 0x19, 0x7E},
{0x00, 0x19, 0x7D},
{0x00, 0x16, 0xCF},
{0x00, 0x16, 0xFE},
{0x00, 0x13, 0xA8},
{0x00, 0x16, 0xCE},
{0x00, 0x13, 0xA9},
{0x00, 0x14, 0xA4},
{0x00, 0x02, 0xC7},
{0x00, 0x19, 0x66},
{0x00, 0x1F, 0x3A},
{0x00, 0x24, 0x33},
{0x00, 0x26, 0x5C},
{0x00, 0x25, 0x56},
{0x00, 0x24, 0x2C},
{0x00, 0x24, 0x2B},
{0x00, 0x23, 0x4E},
{0x00, 0x23, 0x4D},
{0x00, 0x23, 0x06},
{0x00, 0x22, 0x69},
{0x00, 0x22, 0x68},
{0x00, 0x21, 0x4F},
{0x00, 0x1F, 0xE2},
{0x00, 0x1F, 0xE1},
{0x00, 0x01, 0x4A}};


/* for RTL865x suspend mode, the CPU can be suspended initially. */
int gCpuCanSuspend = 1;

static int rtl8192cd_query_psd_cfg80211(struct rtl8192cd_priv *priv, int chnl, int bw, int fft_pts);
static unsigned int OnAssocReq(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnProbeReq(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnProbeRsp(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnBeacon(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnDisassoc(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnAuth(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnDeAuth(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnWmmAction(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int DoReserved(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo);
#ifdef CLIENT_MODE
static unsigned int OnAssocRsp(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnBeaconClnt(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnATIM(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnDisassocClnt(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnAuthClnt(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo);
static unsigned int OnDeAuthClnt(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo);
static void start_clnt_assoc(struct rtl8192cd_priv *priv);
static void calculate_rx_beacon(struct rtl8192cd_priv *priv);
static void updateTSF(struct rtl8192cd_priv *priv);
static void issue_PwrMgt_NullData(struct rtl8192cd_priv * priv);
static unsigned int isOurFrameBuffred(unsigned char* tim, unsigned int aid);
#ifdef WIFI_11N_2040_COEXIST
static void issue_coexist_mgt(struct rtl8192cd_priv *priv);
#endif
#endif

/*Site Survey and sorting result by profile related*/
static int compareTpyeByProfile(struct rtl8192cd_priv *priv , const void *entry1, const void *entry2 , int CompareType);
static int get_profile_index(struct rtl8192cd_priv *priv ,char* SSID2Search);
/*under multi-repeater case when some STA has connect , the other one don't connect to diff channel AP ; skip this*/
static int multiRepeater_startlookup_chk(struct rtl8192cd_priv *priv , int db_idx);
static int multiRepeater_connection_status(struct rtl8192cd_priv *priv );

void SelectLowestInitRate(struct rtl8192cd_priv *priv);
void issue_op_mode_notify(struct rtl8192cd_priv *priv, struct stat_info *pstat, char mode);


#if (BEAMFORMING_SUPPORT == 1)
void DynamicSelectTxBFSTA(struct rtl8192cd_priv *priv);
void DynamicSelect2STA(struct rtl8192cd_priv *priv);
#endif

struct mlme_handler {
	unsigned int   num;
	char* str;
	unsigned int (*func)(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo);
};


struct mlme_handler mlme_ap_tbl[]={
	{WIFI_ASSOCREQ,		"OnAssocReq",	OnAssocReq},
	{WIFI_ASSOCRSP,		"OnAssocRsp",	DoReserved},
	{WIFI_REASSOCREQ,	"OnReAssocReq",	OnAssocReq},
	{WIFI_REASSOCRSP,	"OnReAssocRsp",	DoReserved},
	{WIFI_PROBEREQ,		"OnProbeReq",	OnProbeReq},
	{WIFI_PROBERSP,		"OnProbeRsp",	OnProbeRsp},

	/*----------------------------------------------------------
					below 2 are reserved
	-----------------------------------------------------------*/
	{0,					"DoReserved",	DoReserved},
	{0,					"DoReserved",	DoReserved},
	{WIFI_BEACON,		"OnBeacon",		OnBeacon},
	{WIFI_ATIM,			"OnATIM",		DoReserved},
	{WIFI_DISASSOC,		"OnDisassoc",	OnDisassoc},
	{WIFI_AUTH,			"OnAuth",		OnAuth},
	{WIFI_DEAUTH,		"OnDeAuth",		OnDeAuth},
	{WIFI_WMM_ACTION,	"OnWmmAct",		OnWmmAction}
};
#ifdef CLIENT_MODE
struct mlme_handler mlme_station_tbl[]={
	{WIFI_ASSOCREQ,		"OnAssocReq",	DoReserved},
	{WIFI_ASSOCRSP,		"OnAssocRsp",	OnAssocRsp},
	{WIFI_REASSOCREQ,	"OnReAssocReq",	DoReserved},
	{WIFI_REASSOCRSP,	"OnReAssocRsp",	OnAssocRsp},
	{WIFI_PROBEREQ,		"OnProbeReq",	OnProbeReq},
	{WIFI_PROBERSP,		"OnProbeRsp",	OnProbeRsp},

	/*----------------------------------------------------------
					below 2 are reserved
	-----------------------------------------------------------*/
	{0,					"DoReserved",	DoReserved},
	{0,					"DoReserved",	DoReserved},
	{WIFI_BEACON,		"OnBeacon",		OnBeaconClnt},
	{WIFI_ATIM,			"OnATIM",		OnATIM},
	{WIFI_DISASSOC,		"OnDisassoc",	OnDisassocClnt},
	{WIFI_AUTH,			"OnAuth",		OnAuthClnt},
	{WIFI_DEAUTH,		"OnDeAuth",		OnDeAuthClnt},
	{WIFI_WMM_ACTION,	"OnWmmAct",		OnWmmAction}
};
#endif

#ifdef CONFIG_RTL_WLAN_DOS_FILTER
#define MAX_BLOCK_MAC		4
unsigned char block_mac[MAX_BLOCK_MAC][6];
unsigned int block_mac_idx = 0;
unsigned int block_sta_time = 0;
unsigned long block_priv;
#endif

static int is_support_wpa_aes(struct rtl8192cd_priv *priv, unsigned char *pucIE, unsigned long ulIELength)
{
	unsigned short version, usSuitCount;
	DOT11_RSN_IE_HEADER *pDot11RSNIEHeader;
	DOT11_RSN_IE_SUITE *pDot11RSNIESuite;
	DOT11_RSN_IE_COUNT_SUITE *pDot11RSNIECountSuite;
	unsigned char *ptr;

	if (ulIELength < sizeof(DOT11_RSN_IE_HEADER)) {
		DEBUG_WARN("parseIE err 1!\n");
		return -1;
	}

	pDot11RSNIEHeader = (DOT11_RSN_IE_HEADER *)pucIE;
	ptr = (unsigned char *)&pDot11RSNIEHeader->Version;
	version = (ptr[1] << 8) | ptr[0];

	if (version != RSN_VER1) {
		DEBUG_WARN("parseIE err 2!\n");
		return -1;
	}

	if (pDot11RSNIEHeader->ElementID != RSN_ELEMENT_ID ||
			pDot11RSNIEHeader->Length != ulIELength -2 ||
			pDot11RSNIEHeader->OUI[0] != 0x00 || pDot11RSNIEHeader->OUI[1] != 0x50 ||
			pDot11RSNIEHeader->OUI[2] != 0xf2 || pDot11RSNIEHeader->OUI[3] != 0x01 ) {
		DEBUG_WARN("parseIE err 3!\n");
		return -1;
	}

	ulIELength -= sizeof(DOT11_RSN_IE_HEADER);
	pucIE += sizeof(DOT11_RSN_IE_HEADER);

	//----------------------------------------------------------------------------------
 	// Multicast Cipher Suite processing
	//----------------------------------------------------------------------------------
	if (ulIELength < sizeof(DOT11_RSN_IE_SUITE)) {
		DEBUG_WARN("parseIE err 4!\n");
		return -1;
	}

	pDot11RSNIESuite = (DOT11_RSN_IE_SUITE *)pucIE;
	if (pDot11RSNIESuite->OUI[0] != 0x00 ||
		pDot11RSNIESuite->OUI[1] != 0x50 ||
		pDot11RSNIESuite->OUI[2] != 0xF2) {
		DEBUG_WARN("parseIE err 5!\n");
		return -1;
	}

	ulIELength -= sizeof(DOT11_RSN_IE_SUITE);
	pucIE += sizeof(DOT11_RSN_IE_SUITE);

	//----------------------------------------------------------------------------------
	// Pairwise Cipher Suite processing
	//----------------------------------------------------------------------------------
	if (ulIELength < 2 + sizeof(DOT11_RSN_IE_SUITE)) {
		DEBUG_WARN("parseIE err 6!\n");
		return -1;
	}

	pDot11RSNIECountSuite = (PDOT11_RSN_IE_COUNT_SUITE)pucIE;
	pDot11RSNIESuite = pDot11RSNIECountSuite->dot11RSNIESuite;
	ptr = (unsigned char *)&pDot11RSNIECountSuite->SuiteCount;
	usSuitCount = (ptr[1] << 8) | ptr[0];

	if (usSuitCount != 1 ||
			pDot11RSNIESuite->OUI[0] != 0x00 ||
			pDot11RSNIESuite->OUI[1] != 0x50 ||
			pDot11RSNIESuite->OUI[2] != 0xF2) {
		DEBUG_WARN("parseIE err 7!\n");
		return -1;
	}

	if (pDot11RSNIESuite->Type == DOT11_ENC_CCMP)
		return 1;
	else
		return 0;
}


static int is_support_wpa2_aes(struct rtl8192cd_priv *priv, 	unsigned char *pucIE, unsigned long ulIELength)
{
	unsigned short version, usSuitCount;
	DOT11_WPA2_IE_HEADER *pDot11WPA2IEHeader = NULL;
	DOT11_RSN_IE_SUITE  *pDot11RSNIESuite = NULL;
	DOT11_RSN_IE_COUNT_SUITE *pDot11RSNIECountSuite = NULL;
	unsigned char *ptr;

	if (ulIELength < sizeof(DOT11_WPA2_IE_HEADER)) {
		DEBUG_WARN("ERROR_INVALID_RSNIE_LEN, err 1\n");
		return -1;
	}

	pDot11WPA2IEHeader = (DOT11_WPA2_IE_HEADER *)pucIE;
	ptr = (unsigned char *)&pDot11WPA2IEHeader->Version;
	version = (ptr[1] << 8) | ptr[0];

	if (version != RSN_VER1) {
		DEBUG_WARN("ERROR_UNSUPPORTED_RSNEVERSION, err 2\n");
		return -1;
	}

	if (pDot11WPA2IEHeader->ElementID != WPA2_ELEMENT_ID ||
		pDot11WPA2IEHeader->Length != ulIELength -2 ) {
		DEBUG_WARN("ERROR_INVALID_RSNIE, err 3\n");
		return -1;
	}

	ulIELength -= sizeof(DOT11_WPA2_IE_HEADER);
	pucIE += sizeof(DOT11_WPA2_IE_HEADER);

	//----------------------------------------------------------------------------------
 	// Multicast Cipher Suite processing
	//----------------------------------------------------------------------------------
	if (ulIELength < sizeof(DOT11_RSN_IE_SUITE)) {
		DEBUG_WARN("ERROR_INVALID_RSNIE_LEN, err 4\n");
		return -1;
	}

	pDot11RSNIESuite = (DOT11_RSN_IE_SUITE *)pucIE;
	if (pDot11RSNIESuite->OUI[0] != 0x00 ||
			pDot11RSNIESuite->OUI[1] != 0x0F ||
				pDot11RSNIESuite->OUI[2] != 0xAC) {
		DEBUG_WARN("ERROR_INVALID_RSNIE, err 5\n");
		return -1;
	}

	if (pDot11RSNIESuite->Type > DOT11_ENC_WEP104)	{
		DEBUG_WARN("ERROR_INVALID_MULTICASTCIPHER, err 6\n");
		return -1;
	}

	ulIELength -= sizeof(DOT11_RSN_IE_SUITE);
	pucIE += sizeof(DOT11_RSN_IE_SUITE);

	//----------------------------------------------------------------------------------
	// Pairwise Cipher Suite processing
	//----------------------------------------------------------------------------------
	if (ulIELength < 2 + sizeof(DOT11_RSN_IE_SUITE)) {
		DEBUG_WARN("ERROR_INVALID_RSN_IE_SUITE_LEN, err 7\n");
		return -1;
	}

#ifdef OSK
	memcpy(&stDot11RSNIECountSuite, pucIE, sizeof(DOT11_RSN_IE_COUNT_SUITE));
	pDot11RSNIECountSuite = &stDot11RSNIECountSuite;
	pDot11RSNIESuite = pDot11RSNIECountSuite->dot11RSNIESuite;
	usSuitCount = le16_to_cpu(pDot11RSNIECountSuite->SuiteCount);
#else
	pDot11RSNIECountSuite = (PDOT11_RSN_IE_COUNT_SUITE)pucIE;
	pDot11RSNIESuite = pDot11RSNIECountSuite->dot11RSNIESuite;
	ptr = (unsigned char *)&pDot11RSNIECountSuite->SuiteCount;
	usSuitCount = (ptr[1] << 8) | ptr[0];
#endif

	if (usSuitCount != 1 ||
		pDot11RSNIESuite->OUI[0] != 0x00 ||
			pDot11RSNIESuite->OUI[1] != 0x0F ||
				pDot11RSNIESuite->OUI[2] != 0xAC) {
		DEBUG_WARN("ERROR_INVALID_RSNIE, err 8\n");
		return -1;
	}

	if (pDot11RSNIESuite->Type > DOT11_ENC_WEP104) {
		DEBUG_WARN("ERROR_INVALID_UNICASTCIPHER, err 9\n");
		return -1;
	}

	if (pDot11RSNIESuite->Type == DOT11_ENC_CCMP)
		return 1;
	else
		return 0;
}

static unsigned char check_probe_sta_rssi_valid(struct rtl8192cd_priv *priv,unsigned char* addr, unsigned char rssi)
{
	int i, idx=-1;
	unsigned char *hwaddr;
	hwaddr = addr;
	for (i=0; i<MAX_PROBE_REQ_STA; i++) {
		if (!memcmp(priv->probe_sta[i].addr, addr, MACADDRLEN)){
			idx = i;
			break;// check if it is already in the list
		}
	}
	if (idx < 0){
		return 1;// if probe req sta isn't in the list, allow it
	}
	else{
		if ( priv->probe_sta[idx].rssi && RTL_ABS(priv->probe_sta[idx].rssi,rssi) > 10)
			return 0;
		else
			return 1;
	}
}

static void add_MAC_RSSI_Entry(struct rtl8192cd_priv *priv,unsigned char* addr, unsigned char rssi, unsigned char status, struct sta_mac_rssi *EntryDB, unsigned int *EntryOccupied, unsigned int *EntryNum)
{
	int i, idx=-1, idx2 =0;
	unsigned char *hwaddr = addr;
	unsigned char rssi_input;
	for (i=0; i<MAX_PROBE_REQ_STA; i++) {
		if (EntryDB[i].used == 0) {
			if (idx < 0)
				idx = i; //search for empty entry
			continue;
		}
		if (!memcmp(EntryDB[i].addr, addr, MACADDRLEN)) {
			idx2 = i;
			break;// check if it is already in the list
		}
	}
	if (idx >= 0){
		rssi_input = rssi;
		memcpy(EntryDB[idx].addr, addr, MACADDRLEN);
		EntryDB[idx].used = 1;
		EntryDB[idx].Entry = idx;//check which entry is the probe sta recorded
		EntryDB[idx].rssi = rssi_input;
		EntryDB[idx].status = status;
		(*EntryOccupied)++;
		return;
	}
	else if (idx2){
		rssi_input = ((EntryDB[idx2].rssi * 7)+(rssi * 3)) / 10;
		EntryDB[idx2].rssi = rssi_input;
		EntryDB[idx].status = status;
		return;
	}
	else if ((*EntryOccupied) == MAX_PROBE_REQ_STA) {// sta list full, need to replace sta
			idx = *EntryNum;
			for (i=0; i<MAX_PROBE_REQ_STA; i++) {
				if (!memcmp(EntryDB[i].addr, addr, MACADDRLEN))
					return;		// check if it is already in the list
			}
			memcpy(EntryDB[idx].addr, addr, MACADDRLEN);
			EntryDB[idx].used = 1;
			EntryDB[idx].Entry = idx;
			EntryDB[idx].rssi = rssi;
			EntryDB[idx].status = status;
			(*EntryNum)++;
			if( (*EntryNum) == MAX_PROBE_REQ_STA)
				*EntryNum = 0; // Reset entry counter;
			return;
		}
}
static void add_probe_req_sta(struct rtl8192cd_priv *priv,unsigned char* addr, unsigned char rssi)
{
	add_MAC_RSSI_Entry(priv, addr, rssi, 0, priv->probe_sta, &(priv->ProbeReqEntryOccupied), &(priv->ProbeReqEntryNum));
}
#ifdef STA_ASSOC_STATISTIC
static void add_reject_sta(struct rtl8192cd_priv *priv,unsigned char* addr, unsigned char rssi)
{
	add_MAC_RSSI_Entry(priv, addr, rssi, 0, priv->reject_sta, &(priv->RejectAssocEntryOccupied), &(priv->RejectAssocEntryNum));
}
static void add_disconnect_sta(struct rtl8192cd_priv *priv,unsigned char* addr, unsigned char rssi)
{
	add_MAC_RSSI_Entry(priv, addr, rssi, 0, priv->removed_sta, &(priv->RemoveAssocEntryOccupied), &(priv->RemoveAssocEntryNum));
}

void add_sta_assoc_status(struct rtl8192cd_priv *priv,unsigned char* addr, unsigned char rssi, unsigned char status)
{
	add_MAC_RSSI_Entry(priv, addr, rssi, status, priv->assoc_sta, &(priv->AssocStatusEntryOccupied), &(priv->AssocStatusEntryNum));
}
#endif


#ifdef WIFI_SIMPLE_CONFIG
/* WPS2DOTX   */
unsigned char *search_VendorExt_tag(unsigned char *data, unsigned char id, int len, int *out_len)
{
	unsigned char tag, tag_len;
	int size;

	//skip WFA_VENDOR_LEN
	data+=3;
	len-=3;

	while (len > 0) {
		memcpy(&tag, data, 1);
		memcpy(&tag_len, data+1, 1);
		if (id == tag) {
			if (len >= (2 + tag_len)) {
				*out_len = (int)tag_len;
				return (&data[2]);
			}
			else {
				_DEBUG_ERR("Found VE tag [0x%x], but invalid length!\n", id);
				break;
			}
		}
		size = 2 + tag_len;
		data += size;
		len -= size;
	}

	return NULL;
}
/* WPS2DOTX   */

unsigned char *search_wsc_tag(unsigned char *data, unsigned short id, int len, int *out_len)
{
	unsigned short tag, tag_len;
	int size;

	while (len > 0) {
		memcpy(&tag, data, 2);
		memcpy(&tag_len, data+2, 2);
		tag = ntohs(tag);
		tag_len = ntohs(tag_len);

		if (id == tag) {
			if (len >= (4 + tag_len)) {
				*out_len = (int)tag_len;
				return (&data[4]);
			}
			else {
				_DEBUG_ERR("Found tag [0x%x], but invalid length!\n", id);
				break;
			}
		}
		size = 4 + tag_len;
		data += size;
		len -= size;
	}

	return NULL;
}


static struct wsc_probe_request_info *search_wsc_probe_sta(struct rtl8192cd_priv *priv, unsigned char *addr)
{
	int i, idx=-1;

	for (i=0; i<MAX_WSC_PROBE_STA; i++) {
		if (priv->wsc_sta[i].used == 0) {
			if (idx < 0)
				idx = i;
			continue;
		}
		if (!memcmp(priv->wsc_sta[i].addr, addr, MACADDRLEN))
			break;
	}

	if ( i != MAX_WSC_PROBE_STA)
		return (&priv->wsc_sta[i]); // return sta info for WSC sta

	if (idx >= 0)
		return (&priv->wsc_sta[idx]); // add sta info for WSC sta
	else {
		// sta list full, need to replace sta
		unsigned long oldest_time_stamp=jiffies;

		for (i=0; i<MAX_WSC_PROBE_STA; i++) {
			if (priv->wsc_sta[i].time_stamp < oldest_time_stamp) {
				oldest_time_stamp = priv->wsc_sta[i].time_stamp;
				idx = i;
			}
		}
		memset(&priv->wsc_sta[idx], 0, sizeof(struct wsc_probe_request_info));

		return (&priv->wsc_sta[idx]);
	}
}


static int search_wsc_pbc_probe_sta(struct rtl8192cd_priv *priv, unsigned char *addr)
{
	int i/*, idx=-1*/;
	unsigned long flags;
	SAVE_INT_AND_CLI(flags);

	for (i=0; i<MAX_WSC_PROBE_STA; i++) {
		if (priv->wsc_sta[i].used==1 && priv->wsc_sta[i].pbcactived==1) {

			if (!memcmp(priv->wsc_sta[i].addr, addr, MACADDRLEN)){

				priv->wsc_sta[i].used=0;
				priv->wsc_sta[i].pbcactived=0;
				RESTORE_INT(flags);
				return 1;
			}
		}
	}
	RESTORE_INT(flags);
	return 0;

}

#define TAG_DEVICE_PASSWORD_ID		0x1012
#define PASS_ID_PB					0x4
static void wsc_forward_probe_request(struct rtl8192cd_priv *priv, unsigned char *pframe, unsigned char *IEaddr, unsigned int IElen)
{
	unsigned char *p=IEaddr;
	unsigned int len=IElen;
	unsigned char forwarding=0;
	struct wsc_probe_request_info *wsc_sta=NULL;
	DOT11_PROBE_REQUEST_IND ProbeReq_Ind;
	unsigned long flags;
	unsigned char *p2=IEaddr;
	unsigned int len2=IElen;
	unsigned short pwid=0;

	if (IEaddr == NULL || IElen == 0)
		return;
	if (IElen > PROBEIELEN) {
		DEBUG_WARN("[%s] IElen=%d\n", __FUNCTION__, IElen);
		return;
	}
	p = search_wsc_tag(p+2+4, TAG_REQUEST_TYPE, len-4, (int *)&len);
	if (p && (*p <= MAX_REQUEST_TYPE_NUM)) { //forward WPS IE to wsc daemon
		SAVE_INT_AND_CLI(flags);
		wsc_sta = search_wsc_probe_sta(priv, (unsigned char *)GetAddr2Ptr(pframe));
		p2 = search_wsc_tag(p2+2+4, TAG_DEVICE_PASSWORD_ID, len2-4, (int *)&len2);
		if(p2){
			memcpy(&pwid, p2, len2);
			pwid = ntohs(pwid);
			if(pwid==PASS_ID_PB){
				wsc_sta->pbcactived=1;
			}
		}
		if (wsc_sta->used) {
			if ((wsc_sta->ProbeIELen != IElen) ||
				(memcmp(wsc_sta->ProbeIE, (void *)(IEaddr), IElen) != 0) ||
				((jiffies - wsc_sta->time_stamp) > RTL_SECONDS_TO_JIFFIES(3)))
			{
				memcpy(wsc_sta->ProbeIE, (void *)(IEaddr), IElen);
				wsc_sta->ProbeIELen = IElen;
				wsc_sta->time_stamp = jiffies;
				forwarding = 1;
			}
		}
		else {
			memcpy(wsc_sta->addr, (void *)GetAddr2Ptr(pframe), MACADDRLEN);
			memcpy(wsc_sta->ProbeIE, (void *)(IEaddr), IElen);
			wsc_sta->ProbeIELen = IElen;
			wsc_sta->time_stamp = jiffies;
			wsc_sta->used = 1;
			forwarding = 1;
		}
		RESTORE_INT(flags);

		if (forwarding) {
			memcpy((void *)ProbeReq_Ind.MACAddr, (void *)GetAddr2Ptr(pframe), MACADDRLEN);
			ProbeReq_Ind.EventId = DOT11_EVENT_WSC_PROBE_REQ_IND;
			ProbeReq_Ind.IsMoreEvent = 0;
			ProbeReq_Ind.ProbeIELen = IElen;
			memcpy((void *)ProbeReq_Ind.ProbeIE, (void *)(IEaddr), ProbeReq_Ind.ProbeIELen);
#ifdef INCLUDE_WPS
			//			wps_indicate_evt(priv);
			wps_NonQueue_indicate_evt(priv ,
				(UINT8 *)&ProbeReq_Ind,sizeof(DOT11_PROBE_REQUEST_IND));
#else
			DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&ProbeReq_Ind, sizeof(DOT11_PROBE_REQUEST_IND));
			event_indicate(priv, GetAddr2Ptr(pframe), 1);
#endif
		}
	}
}


static __inline__ void wsc_probe_expire(struct rtl8192cd_priv *priv)
{
	int i;
	//unsigned long flags;

	//SAVE_INT_AND_CLI(flags);
	for (i=0; i<MAX_WSC_PROBE_STA; i++) {
		if (priv->wsc_sta[i].used == 0)
			continue;
		if ((jiffies - priv->wsc_sta[i].time_stamp) > RTL_SECONDS_TO_JIFFIES(180))
			memset(&priv->wsc_sta[i], 0, sizeof(struct wsc_probe_request_info));
	}
	//RESTORE_INT(flags);
}
#endif // WIFI_SIMPLE_CONFIG


static __inline__ UINT8 match_supp_rate(unsigned char *pRate, int len, UINT8 rate)
{
	int idx;
	for (idx=0; idx<len; idx++) {
		if ((pRate[idx] & 0x7f) == rate)
			return 1;
	}

	// TODO: need some more refinement
	if ((rate & 0x80) && ((rate & 0x7f) < 16))
		return 1;

	return 0;
}


// unchainned all the skb chainnned in a given list, like frag_list(type == 0)
void unchainned_all_frag(struct rtl8192cd_priv *priv, struct list_head *phead)
{
	struct rx_frinfo *pfrinfo;
	struct list_head *plist;
	struct sk_buff	 *pskb;

	while (!list_empty(phead)) {
		plist = phead->next;
		list_del(plist);

		pfrinfo = list_entry(plist, struct rx_frinfo, mpdu_list);
		pskb = get_pskb(pfrinfo);
		rtl_kfree_skb(priv, pskb, _SKB_RX_);
	}
}


void rtl8192cd_frag_timer(unsigned long task_priv)
{
	unsigned long flags;
	struct list_head	*phead, *plist;
	struct stat_info	*pstat;
	struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)task_priv;

	struct list_head frag_list;

	if (!(priv->drv_state & DRV_STATE_OPEN))
		return;

	INIT_LIST_HEAD(&frag_list);

	priv->frag_to ^= 0x01;

	phead = &priv->defrag_list;

	DEFRAG_LOCK(flags);

	plist = phead->next;
	while (plist != phead) {
		pstat = list_entry(plist, struct stat_info, defrag_list);
		plist = plist->next;

		if (pstat->frag_to == priv->frag_to) {
			list_del_init(&pstat->defrag_list);
			list_splice_init(&pstat->frag_list, &frag_list);
			pstat->frag_count = 0;
		}
	}

	DEFRAG_UNLOCK(flags);

	unchainned_all_frag(priv, &frag_list);

	mod_timer(&priv->frag_to_filter, jiffies + FRAG_TO);
}


#ifdef USB_PKT_RATE_CTRL_SUPPORT
usb_pktCnt_fn get_usb_pkt_cnt_hook = NULL;
register_usb_pkt_cnt_fn register_usb_hook = NULL;

void register_usb_pkt_cnt_f(void *usbPktFunc)
{
	get_usb_pkt_cnt_hook = (usb_pktCnt_fn)(usbPktFunc);
}


void usbPkt_timer_handler(struct rtl8192cd_priv *priv)
{
	unsigned int pkt_cnt, pkt_diff;

	if (!get_usb_pkt_cnt_hook)
		return;

	pkt_cnt = get_usb_pkt_cnt_hook();
	pkt_diff = pkt_cnt - priv->pre_pkt_cnt;

	if (pkt_diff) {
		priv->auto_rate_mask = 0x803fffff;
		priv->change_toggle = ((priv->change_toggle) ? 0 : 1);
	}

	priv->pre_pkt_cnt = pkt_cnt;
	priv->pkt_nsec_diff += pkt_diff;

	if ((++priv->poll_usb_cnt) % 10 == 0) {
		if ((priv->pkt_nsec_diff) < 10 ) {
			priv->auto_rate_mask = 0;
			priv->pkt_nsec_diff = 0;
		}
	}
}
#endif // USB_PKT_RATE_CTRL_SUPPORT

static void auth_expire(struct rtl8192cd_priv *priv)
{
	struct stat_info	*pstat;
	struct list_head	*phead, *plist;
	struct list_head	local_head;

	INIT_LIST_HEAD(&local_head);


	phead = &priv->auth_list;
	plist = phead->next;

	while (plist != phead) {
		pstat = list_entry(plist, struct stat_info, auth_list);
		plist = plist->next;

// #if defined(CONFIG_RTK_MESH) && defined(MESH_BOOTSEQ_AUTH) // Skip MP node

		pstat->expire_to--;

		if(priv->pshare->rf_ft_var.fix_expire_to_zero == 1) {
			pstat->expire_to = 0;
			priv->pshare->rf_ft_var.fix_expire_to_zero = 0;
		}

		if (pstat->expire_to == 0) {
			list_del(&pstat->auth_list);
			list_add_tail(&pstat->auth_list, &local_head);
		}
	}

	phead = &local_head;
	plist = phead->next;

	while (plist != phead) {
		pstat = list_entry(plist, struct stat_info, auth_list);
		list_del_init(plist);


		//below should be take care... since auth fail, just free the stat info...
		DEBUG_INFO("auth expire %02X%02X%02X%02X%02X%02X\n",
			pstat->hwaddr[0],pstat->hwaddr[1],pstat->hwaddr[2],pstat->hwaddr[3],pstat->hwaddr[4],pstat->hwaddr[5]);

		free_stainfo(priv, pstat);


		plist = phead->next;
	}

}


#if 0 // def RTL8192SE
void reset_1r_sta_RA(struct rtl8192cd_priv *priv, unsigned int sg_rate){
	struct list_head	*phead, *plist;
	struct stat_info	*pstat;

	phead = &priv->asoc_list;

	SMP_LOCK_ASOC_LIST(flags);

	plist = phead->next;
	while(plist != phead)
	{

		unsigned int sta_band = 0;
		pstat = list_entry(plist, struct stat_info, asoc_list);
		plist = plist->next;

		if(pstat && !pstat->ht_cap_len)
			continue;

		if (pstat->tx_ra_bitmap & 0xffff000)
			sta_band |= WIRELESS_11N | WIRELESS_11G | WIRELESS_11B;
		else if (pstat->tx_ra_bitmap & 0xff0)
			sta_band |= WIRELESS_11G |WIRELESS_11B;
		else
			sta_band |= WIRELESS_11B;

		if((pstat->tx_ra_bitmap & 0x0ff00000) == 0 && (pstat->tx_ra_bitmap & BIT(28))!=0 && sg_rate == 0xffff){
			pstat->tx_ra_bitmap &= ~BIT(28); // disable short GI for 1R sta
			set_fw_reg(priv, (0xfd0000a2 | ((REMAP_AID(pstat) & 0x1f)<<4 | (sta_band & 0xf))<<8), pstat->tx_ra_bitmap, 1);
		}
		else if((pstat->tx_ra_bitmap & 0x0ff00000) == 0 && (pstat->tx_ra_bitmap & BIT(28))==0 && sg_rate == 0x7777){
			pstat->tx_ra_bitmap |= BIT(28); // enable short GI for 1R sta
			set_fw_reg(priv, (0xfd0000a2 | ((REMAP_AID(pstat) & 0x1f)<<4 | (sta_band & 0xf))<<8), pstat->tx_ra_bitmap, 1);
		}
	}

	SMP_UNLOCK_ASOC_LIST(flags);

	return;
}
#endif



// for simplify, we consider only two stations. Otherwise we may sorting all the stations and
// hard to maintain the code.
// 0 for path A/B selection(bg only or 1ss rate), 1 for TX Diversity (ex: DIR 655 clone)
#if 0
struct stat_info* switch_ant_enable(struct rtl8192cd_priv *priv, unsigned char flag)
{
	struct stat_info	*pstat, *pstat_chosen = NULL;
	struct list_head	*phead, *plist;
	unsigned int tp_2nd = 0, maxTP = 0;
	unsigned int rssi_2ndTp = 0, rssi_maxTp = 0;
	unsigned int tx_2s_avg = 0;
	unsigned int rx_2s_avg = 0;
	unsigned long total_sum = (priv->pshare->current_tx_bytes+priv->pshare->current_rx_bytes);
	unsigned char th_rssi = 0;

	phead = &priv->asoc_list;

	SMP_LOCK_ASOC_LIST(flags);

	plist = phead->next;
	while (plist != phead)
	{
		pstat = list_entry(plist, struct stat_info, asoc_list);
		plist = plist->next;

		if((pstat->tx_avarage + pstat->rx_avarage) > maxTP){
			tp_2nd = maxTP;
			rssi_2ndTp = rssi_maxTp;

			maxTP = pstat->tx_avarage + pstat->rx_avarage;
			rssi_maxTp = pstat->rssi;

			pstat_chosen = pstat;
		}
	}

	SMP_UNLOCK_ASOC_LIST(flags);

	// for debug
//	printk("maxTP: %d, second: %d\n", rssi_maxTp, rssi_2ndTp);

	if(pstat_chosen == NULL){
//		printk("ERROR! NULL pstat_chosen \n");
		return NULL;
	}

	if(total_sum != 0){
		tx_2s_avg = (unsigned int)((pstat_chosen->current_tx_bytes*100) / total_sum);
		rx_2s_avg = (unsigned int)((pstat_chosen->current_rx_bytes*100) / total_sum);
	}

	if( priv->assoc_num > 1 && (tx_2s_avg+rx_2s_avg) < (100/priv->assoc_num)){ // this is not a burst station
		pstat_chosen = NULL;
//		printk("avg is: %d\n", (tx_2s_avg+rx_2s_avg));
		goto out_switch_ant_enable;
	}

	if(flag == 1)
		goto out_switch_ant_enable;

	if(pstat_chosen && (pstat_chosen->sta_in_firmware == 1) &&
        !getSTABitMap(&priv->pshare->has_2r_sta, REMAP_AID(pstat_chosen)) // 1r STA
	  )
		th_rssi = 40;
	else
		th_rssi = 63;

	if((maxTP < tp_2nd*2 && (rssi_maxTp < th_rssi || rssi_2ndTp < th_rssi)))
		pstat_chosen = NULL;
	else if(maxTP >= tp_2nd*2 && rssi_maxTp < th_rssi)
		pstat_chosen = NULL;

out_switch_ant_enable:
	return pstat_chosen;
}
#endif


void dynamic_response_rate(struct rtl8192cd_priv *priv, short rssi)
{
#if (MU_BEAMFORMING_SUPPORT == 1)
		PRT_BEAMFORMING_INFO 		pBeamInfo = &(priv->pshare->BeamformingInfo);
#endif
		if(rssi<30){
			if(priv->pmib->dot11RFEntry.phyBandSelect & PHY_BAND_5G){
				if(priv->pshare->current_rsp_rate !=0x10){
					RTL_W16(RRSR,0x10);
					priv->pshare->current_rsp_rate=0x10;
					//SDEBUG("current_rsp_rate=%x\n" ,priv->pshare->current_rsp_rate);
				}
			}else if(priv->pmib->dot11RFEntry.phyBandSelect & PHY_BAND_2G){
				if(priv->pshare->current_rsp_rate!=0x1f){
					RTL_W16(RRSR,0x1f);
					priv->pshare->current_rsp_rate=0x1f;
				}
			}

		}else if(rssi>35){
			if(priv->pmib->dot11RFEntry.phyBandSelect & PHY_BAND_5G){
				#if (MU_BEAMFORMING_SUPPORT == 1)
					if(priv->pmib->dot11RFEntry.txbf_mu && (OPMODE & WIFI_STATION_STATE) && (pBeamInfo->beamformer_mu_cnt>0) ){
						if(priv->pshare->current_rsp_rate!=0x950){
							RTL_W16(RRSR,0x950);
							RTL_W8(0x6DF,0x4B); //init CSI rate 54M
							priv->pshare->current_rsp_rate=0x950;
						}

					}else
				#endif
					{
						if(priv->pshare->current_rsp_rate!=0x150){
							RTL_W16(RRSR,0x150);
							priv->pshare->current_rsp_rate=0x150;
						}
					}

			}else if(priv->pmib->dot11RFEntry.phyBandSelect & PHY_BAND_2G){
				if(priv->pshare->current_rsp_rate!=0x15f){
					RTL_W16(RRSR,0x15f);
					priv->pshare->current_rsp_rate=0x15f;
				}
			}
		}
}

#ifdef RSSI_MIN_ADV_SEL
void collect_min_rssi_data(struct rtl8192cd_priv *priv, struct stat_info *pstat)
{
	struct priv_shared_info *pshare = priv->pshare;
	int i;

	if (0 == pshare->rf_ft_var.rssi_min_advsel)
		return;

	for (i = (int)pshare->min_rssi_data_cnt-1; i >= 0; --i) {
		if (pstat->rssi < pshare->min_rssi_data[i].rssi) {
			if (i != NR_RSSI_MIN_DATA-1) {
				memcpy(&pshare->min_rssi_data[i+1],
					&pshare->min_rssi_data[i], sizeof(struct rssi_search_entry));
			}
		} else
			break;
	}

	if (i != NR_RSSI_MIN_DATA-1) {
		++i;
		pshare->min_rssi_data[i].rssi = pstat->rssi;
		pshare->min_rssi_data[i].tx_rate = pstat->current_tx_rate;
		pshare->min_rssi_data[i].throughput =
			(pstat->tx_avarage + pstat->rx_avarage) >> 17;
		if (pshare->min_rssi_data_cnt < NR_RSSI_MIN_DATA)
			pshare->min_rssi_data_cnt++;
	}
}

#define RSSI_MIN_LIMIT		15
void select_rssi_min_from_data(struct rtl8192cd_priv *priv)
{
	struct priv_shared_info *pshare = priv->pshare;
	struct rssi_search_entry *entry;
	unsigned char rssi_min, rssi_max;
	unsigned char loRate;
	int FA_counter;

	if (pshare->min_rssi_data_cnt <= 1) {
		pshare->rssi_min_prev = pshare->rssi_min;
		return;
	}

	entry = &pshare->min_rssi_data[0];
	loRate = ((entry->tx_rate <= _9M_RATE_)
		|| (_MCS0_RATE_ == entry->tx_rate) || (_MCS1_RATE_ == entry->tx_rate));
	rssi_min = entry->rssi;

	rssi_max = pshare->min_rssi_data[pshare->min_rssi_data_cnt-1].rssi;
	if ((rssi_min < RSSI_MIN_LIMIT) && (rssi_max >= RSSI_MIN_LIMIT))
		rssi_min = RSSI_MIN_LIMIT;

	FA_counter = ODMPTR->FalseAlmCnt.Cnt_all;
	if (loRate || (FA_counter >= 512)) {
		if (FA_counter > 2500)
			rssi_min = rssi_min + 8;
		else
			rssi_min = rssi_min + 5;
		if (rssi_min > rssi_max)
			rssi_min = rssi_max;
	}
	if (rssi_min + 2 < pshare->rssi_min_prev)
		rssi_min = pshare->rssi_min_prev -2;

	pshare->rssi_min = rssi_min;
	pshare->rssi_min_prev = pshare->rssi_min;
}
#endif // RSSI_MIN_ADV_SEL

#ifdef RTK_ATM//calc per SSID client's burst size
const u2Byte CCK_RATE[4] = {1, 2, 5,  11 }; /*CCK*/
const u2Byte OFDM_RATE[8] = {6, 9, 12, 18, 24, 36, 48, 54 };/*OFDM*/

static void atm_set_statxrate_idx(struct rtl8192cd_priv *priv, struct stat_info *pstat)
{
	int i;
	//memset(pstat->atm_tx_rate_id, 0x0, sizeof(pstat->atm_tx_rate_id));

	if(pstat->current_tx_rate >= 0x90){//VHT
		pstat->atm_tx_rate_id[0]= 2;//VHT

		pstat->atm_tx_rate_id[1]= MIN_NUM(pstat->tx_bw, 2);//bw
		pstat->atm_tx_rate_id[2]= ((pstat->ht_current_tx_info&TX_USE_SHORT_GI)?1:0);//sg
		pstat->atm_tx_rate_id[3]= (pstat->current_tx_rate - 0x90);

	} else if (is_MCS_rate(pstat->current_tx_rate)){//HT
		pstat->atm_tx_rate_id[0]= 1;//HT

		pstat->atm_tx_rate_id[1]= ((pstat->ht_current_tx_info&BIT(0))?1:0);//bw
		pstat->atm_tx_rate_id[2]= ((pstat->ht_current_tx_info&BIT(1))?1:0);//sg
		pstat->atm_tx_rate_id[3]= (pstat->current_tx_rate&0xf);//phy rate

	} else {
		pstat->atm_tx_rate_id[0]= 0;//OFDM_CCK

		if(priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_2G){
			pstat->atm_tx_rate_id[1]= 0;//2G
			pstat->atm_tx_rate_id[2] = 3;//default

			for(i=0;i<4;i++){
				if(pstat->current_tx_rate/2 == CCK_RATE[i]){
					pstat->atm_tx_rate_id[2]=i;//phy rate
					break;
				}
			}

		}else if(priv->pmib->dot11RFEntry.phyBandSelect & PHY_BAND_5G){
			pstat->atm_tx_rate_id[1]= 1;//5G
			pstat->atm_tx_rate_id[2] = 7;//default

			for(i=0;i<8;i++){
				if(pstat->current_tx_rate/2 == OFDM_RATE[i]){
					pstat->atm_tx_rate_id[2]=i;//phy rate
					break;
				}
			}
		}
	}

	//if ((priv->up_time % 2) == 0)
	//	panic_printk("cap=%d bw=%d sg=%d rid=%d\n",
	//		pstat->atm_tx_rate_id[0], pstat->atm_tx_rate_id[1], pstat->atm_tx_rate_id[2], pstat->atm_tx_rate_id[3]);
}

static void atm_set_statime(struct rtl8192cd_priv *priv)
{
	unsigned int atm_time_per_sta=0;//average percetage of sta
	struct stat_info *pstat;
	struct list_head *phead, *plist;
	int vap_cnt = 0, iftime = 0;

	vap_cnt = GET_ROOT(priv)->vap_count;

	// 1:ssid auto 2:ssid manual 3:sta auto 4:sta manual
	if(priv->pshare->rf_ft_var.atm_mode == 1){
		//equal airtime to every interface then equal interface airtime to interface sta
		iftime = 100/(vap_cnt+1);
		atm_time_per_sta = iftime/priv->atm_sta_num;
	}
	else if(priv->pshare->rf_ft_var.atm_mode == 2){
		//equal pre-set interface airtime to interface sta
		atm_time_per_sta = priv->atm_iftime/priv->atm_sta_num;
	}
	else if(priv->pshare->rf_ft_var.atm_mode == 3){
		//equal airtime to every sta
		atm_time_per_sta = 100/priv->pshare->atm_ttl_stanum;
	}
	else if(priv->pshare->rf_ft_var.atm_mode == 4){
		//equal rest airtime to no pre-set client
		atm_time_per_sta = (100-priv->pshare->atm_ttl_match_statime)/(priv->pshare->atm_ttl_stanum-priv->pshare->atm_ttl_match_stanum);
	}

#if 0//for debug
	if ((priv->up_time % 2)==0){
		printk("\n[ATM] [%s] [1/%d][%d%%] has %d sta(-%d), per_cli=%d%%\n",
				priv->dev->name, (GET_ROOT(priv)->vap_count+1),
				priv->atm_iftime, priv->atm_sta_num,
				priv->atm_match_sta_num, atm_time_per_sta);
	}
#endif

	if(atm_time_per_sta == 0){
		//printk("[ATM][WARNING] atm_time_per_sta = 0\n");
		atm_time_per_sta = 1;
	}

	phead = &priv->asoc_list;
	plist = phead->next;
	extern int query_vht_rate(struct stat_info *pstat);
	unsigned char *rate;

	while(plist != phead){
		pstat = list_entry(plist, struct stat_info, asoc_list);

		//printk("current_tx_rate = %d\n",pstat->current_tx_rate);

		//tx rate of sta
#ifdef RTK_AC_SUPPORT
		if(pstat->current_tx_rate >= VHT_RATE_ID)
			pstat->atm_tx_rate = query_vht_rate(pstat);
		else
#endif
		if (is_MCS_rate(pstat->current_tx_rate))
		{
			rate = (unsigned char *)MCS_DATA_RATEStr[(pstat->ht_current_tx_info&BIT(0))?1:0][(pstat->ht_current_tx_info&BIT(1))?1:0][(pstat->current_tx_rate - HT_RATE_ID)];
			pstat->atm_tx_rate = _atoi(rate, 10);
		}
		else
			pstat->atm_tx_rate = pstat->current_tx_rate/2;

		if(priv->pshare->rf_ft_var.atm_chk_newrty)
			atm_set_statxrate_idx(priv, pstat);
#if 0
		printk("[ATM] STA[%02x] atm_tx_rate=%d\n", pstat->hwaddr[5], pstat->atm_tx_rate);
#endif
		//assign atm_sta_time to client
		if(priv->pshare->rf_ft_var.atm_mode == 4)// mode=4, sta manual
		{
			if(pstat->atm_match_sta_time != 0)
				pstat->atm_sta_time = pstat->atm_match_sta_time;
			else
				pstat->atm_sta_time = atm_time_per_sta;
		}
		else//mode=1,2,3
			pstat->atm_sta_time = atm_time_per_sta;


#if 0//for debug
		if ((priv->up_time % 2) == 0)
			printk("[ATM] STA[%02x] txrate=%d occupy %d %% in [%s][%d%%]\n",
				pstat->hwaddr[5], pstat->atm_tx_rate, pstat->atm_sta_time,
				priv->dev->name, priv->atm_iftime);
#endif
		if (plist == plist->next)
			break;
		plist = plist->next;
	}
}

static void atm_set_burst_size(struct rtl8192cd_priv *priv)
{
	int i, tx_rate=0, txrate_ratio=0;
	int enable_atm_swq=0, chg=0;
	int	is_low_tp=1;
	struct stat_info *pstat;
	unsigned int tx_avarage_all = 0;

	priv->pshare->atm_min_burstsize = 0xffff;
	priv->pshare->atm_min_txrate = 0xffff;
	priv->pshare->atm_max_burstsize = 0;
	priv->pshare->atm_max_txrate = 0;

	//priv->pshare->atm_ttl_stanum = 0;

	//find the min burst packet number
	for(i=0; i<NUM_STAT; i++) {
		if (priv->pshare->aidarray[i] && (priv->pshare->aidarray[i]->used == TRUE)){
			pstat = &(priv->pshare->aidarray[i]->station);

			//sta tx rate
			//min
			if(pstat->atm_tx_rate < priv->pshare->atm_min_txrate)
				priv->pshare->atm_min_txrate = pstat->atm_tx_rate;
			//max
			if(pstat->atm_tx_rate > priv->pshare->atm_max_txrate)
				priv->pshare->atm_max_txrate = pstat->atm_tx_rate;

			//sta burst size
			//min
			if((pstat->atm_tx_rate*pstat->atm_sta_time) < priv->pshare->atm_min_burstsize)
				priv->pshare->atm_min_burstsize = pstat->atm_tx_rate*pstat->atm_sta_time;
			//max
			if((pstat->atm_tx_rate*pstat->atm_sta_time) > priv->pshare->atm_max_burstsize)
				priv->pshare->atm_max_burstsize = pstat->atm_tx_rate*pstat->atm_sta_time;

			pstat->atm_burst_size = pstat->atm_tx_rate*pstat->atm_sta_time;

			//priv->pshare->atm_ttl_stanum++;

			//200K~800K	25000
			//800K~2M	100000
			//2M~4M	250000
			//4M~15M	625000
			//15M~110M	1875000
			//110M~	13750000

			//check if atm swq on/off

			//one client tp > 15M, swq turn on
			if(pstat->tx_avarage >  1875000)//15M~110M
				enable_atm_swq = 1;

			//all client tp < 4M, swq turn off
			if(pstat->tx_avarage > 625000 && priv->pshare->atm_swq_en == 1)//4M~15M
				enable_atm_swq = 1;

			//if(pstat->tx_avarage > 625000 && is_low_tp == 1)
			//	is_low_tp = 0;

			tx_avarage_all += pstat->tx_avarage;

		}
	}

#if 0
	//max/min tx rate
	txrate_ratio = priv->pshare->atm_max_txrate/priv->pshare->atm_min_txrate;


	//adjust factor by max/min txrate ratio
	if(txrate_ratio < priv->pshare->rf_ft_var.atm_rlo)
	{
		//txrate_ratio<6
		//every client have similar tx rate. that max/min < lo
		priv->pshare->atm_burst_base = priv->pshare->rf_ft_var.atm_aggmax;//min burst unit
		priv->pshare->atm_timer = priv->pshare->rf_ft_var.atm_sto;//short timer
	}
	else if(txrate_ratio<priv->pshare->rf_ft_var.atm_rhi &&
			txrate_ratio>=priv->pshare->rf_ft_var.atm_rlo)
	{
		//10>txrate_ratio>=6
		//client has different tx rate, that hi> max.min >lo
		priv->pshare->atm_burst_base = priv->pshare->rf_ft_var.atm_aggmin;//min burst unit
		priv->pshare->atm_timer = priv->pshare->rf_ft_var.atm_mto;//long timer
		if(enable_atm_swq == 0 && !is_low_tp)
			enable_atm_swq = 1;
	}
	else
	{
		//txrate_ratio>=10
		//client has very big gap in txrate, that max.min > hi
		priv->pshare->atm_burst_base = priv->pshare->rf_ft_var.atm_aggmin;//min burst unit
		priv->pshare->atm_timer = priv->pshare->rf_ft_var.atm_lto;//long timer
		if(enable_atm_swq == 0 && !is_low_tp)
			enable_atm_swq = 1;
	}
#else
	//priv->pshare->atm_burst_base = priv->pshare->rf_ft_var.atm_aggmax;
	if (tx_avarage_all > 13750000)			 // 110M~
		priv->pshare->atm_timer = priv->pshare->rf_ft_var.atm_sto;
	else if (tx_avarage_all > 1875000)		//15M~110M
		priv->pshare->atm_timer = priv->pshare->rf_ft_var.atm_mto;
	else if (tx_avarage_all > 625000)		//4M~15M
		priv->pshare->atm_timer = priv->pshare->rf_ft_var.atm_lto;
#endif

	// if there are more than 8 clients, enable atm swq anyway
	if(priv->pshare->atm_ttl_stanum >= 8)
		enable_atm_swq = 1;

	//force atm swq
	if(priv->pshare->rf_ft_var.atm_swqf == 1)
		enable_atm_swq = 1;

	if(enable_atm_swq){
		if(priv->pshare->atm_swq_en == 0){
			printk("[ATM] atm_swq_en on\n");
			priv->pshare->atm_swq_en = 1;
			chg = 1;
		}
	}
	else{
		if(priv->pshare->atm_swq_en == 1){
			printk("[ATM] atm_swq_en off\n");
			priv->pshare->atm_swq_en = 0;
			priv->pshare->atm_timer_init = 0;//turn off timer
			chg = 1;
		}
	}

	//calc burst num
	if(priv->pshare->atm_min_burstsize != 0xffff){
		//printk("[ATM] atm_min_burstsize = %d\n", priv->pshare->atm_min_burstsize);
		for(i=0; i<NUM_STAT; i++) {
			if (priv->pshare->aidarray[i] && (priv->pshare->aidarray[i]->used == TRUE)){
				pstat = &(priv->pshare->aidarray[i]->station);
				pstat->atm_is_maxsta = 0;

				if(pstat->atm_adj_factor == 0)
					pstat->atm_adj_factor = 100;

				if((pstat->atm_tx_rate*pstat->atm_sta_time) == priv->pshare->atm_min_burstsize){
					if(priv->pshare->rf_ft_var.atm_chk_txtime)
						pstat->atm_burst_num = (priv->pshare->rf_ft_var.atm_quota*pstat->atm_adj_factor)/100;
					else
					pstat->atm_burst_num = priv->pshare->rf_ft_var.atm_quota;
				}else{
					if(priv->pshare->rf_ft_var.atm_chk_txtime)
						pstat->atm_burst_num = (((pstat->atm_burst_size*priv->pshare->rf_ft_var.atm_quota)/priv->pshare->atm_min_burstsize)*pstat->atm_adj_factor)/100;
					else
						pstat->atm_burst_num = ((pstat->atm_burst_size*priv->pshare->rf_ft_var.atm_quota)/priv->pshare->atm_min_burstsize);
				}

				//check if the best client
				if(priv->pshare->atm_max_burstsize==pstat->atm_burst_size && priv->pshare->rf_ft_var.atm_chk_hista)
					pstat->atm_is_maxsta = 1;

				//if swq on/off, reset all param in sta
				if(chg){
					pstat->atm_burst_sent = 0;//reset when af swq enable
					pstat->atm_tx_time_static = 0;
					pstat->atm_adj_factor = 100;
					pstat->atm_tx_time = 0;
					pstat->atm_sta_time = 0;

					pstat->atm_full_sent_cnt = 0;
					pstat->atm_swq_sent_cnt = 0;
					pstat->atm_wait_cnt = 0;
					pstat->atm_swq_sent_cnt = 0;
					pstat->atm_drop_cnt = 0;

					pstat->atm_is_maxsta = 0;
					pstat->atm_swq_full = 0;

					pstat->atm_txbd_full[0] = 0;
					pstat->atm_txbd_full[1] = 0;
					pstat->atm_txbd_full[2] = 0;
					pstat->atm_txbd_full[3] = 0;
				}
			}
		}
	}
}

//#define ATM_ADJUST_TIME 10

static void atm_check_statime(struct rtl8192cd_priv *priv)
{
	int i=0,j=0;
	struct stat_info *pstat;

	priv->pshare->atm_ttl_match_statime = 0;//reset to 0
	priv->pshare->atm_ttl_match_stanum = 0;//reset to 0
	priv->pshare->atm_ttl_stanum = 0;

	//check dedicated client exist
	for(i=0; i<NUM_STAT; i++) {
		if (priv->pshare->aidarray[i] && (priv->pshare->aidarray[i]->used == TRUE)){
			pstat = &(priv->pshare->aidarray[i]->station);

			pstat->atm_match_sta_time = 0;
			priv->pshare->atm_ttl_stanum++;//ttl client in this radio 2.4/5G

			//check atm_mode = 4
			if(priv->pshare->rf_ft_var.atm_mode == 4){
				for(j=0; j<NUM_STAT; j++){
					if(priv->pshare->rf_ft_var.atm_sta_info[j].atm_time != 0){
						if(pstat->hwaddr[0] == priv->pshare->rf_ft_var.atm_sta_info[j].hwaddr[0] &&
							pstat->hwaddr[1] == priv->pshare->rf_ft_var.atm_sta_info[j].hwaddr[1] &&
							pstat->hwaddr[2] == priv->pshare->rf_ft_var.atm_sta_info[j].hwaddr[2] &&
							pstat->hwaddr[3] == priv->pshare->rf_ft_var.atm_sta_info[j].hwaddr[3] &&
							pstat->hwaddr[4] == priv->pshare->rf_ft_var.atm_sta_info[j].hwaddr[4] &&
							pstat->hwaddr[5] == priv->pshare->rf_ft_var.atm_sta_info[j].hwaddr[5])
						{
							//sta time accumelate
							priv->pshare->atm_ttl_match_statime += priv->pshare->rf_ft_var.atm_sta_info[j].atm_time;
							priv->pshare->atm_ttl_match_stanum++;
							pstat->atm_match_sta_time = priv->pshare->rf_ft_var.atm_sta_info[j].atm_time;
							break;
						}
						else if(pstat->sta_ip[0] == priv->pshare->rf_ft_var.atm_sta_info[j].ipaddr[0] &&
								pstat->sta_ip[1] == priv->pshare->rf_ft_var.atm_sta_info[j].ipaddr[1] &&
								pstat->sta_ip[2] == priv->pshare->rf_ft_var.atm_sta_info[j].ipaddr[2] &&
								pstat->sta_ip[3] == priv->pshare->rf_ft_var.atm_sta_info[j].ipaddr[3])
						{
							priv->pshare->atm_ttl_match_statime += priv->pshare->rf_ft_var.atm_sta_info[j].atm_time;
							priv->pshare->atm_ttl_match_stanum++;
							pstat->atm_match_sta_time = priv->pshare->rf_ft_var.atm_sta_info[j].atm_time;
							break;
						}
					}
				}
			}
		}
	}
}

#ifdef TXRETRY_CNT
static void atm_calc_txretrytime(struct rtl8192cd_priv *priv, struct stat_info *pstat)
{
	int i, bw=0, sg=0, rate_idx=0;
	int retry_times=pstat->atm_txretry_avg;
	unsigned char *ht_rate;
	unsigned int ht_rate2;

	extern const u2Byte VHT_MCS_DATA_RATE[3][2][20];
	extern const unsigned char* MCS_DATA_RATEStr[2][2][24];

	//if ((priv->up_time % 2) == 0)
	//	panic_printk("cap:%d bw=%d sg=%d rid=%d\n",
	//		pstat->atm_tx_rate_id[0], pstat->atm_tx_rate_id[1], pstat->atm_tx_rate_id[2], pstat->atm_tx_rate_id[3]);

	//VHT rate fall back txtime
	if(pstat->atm_tx_rate_id[0]==2){

		bw = pstat->atm_tx_rate_id[1];
		sg = pstat->atm_tx_rate_id[2];
		rate_idx = pstat->atm_tx_rate_id[3];//init rate

		while(retry_times>0){

			//rate fall back
			if(rate_idx>0)
			{
				rate_idx--;
				//panic_printk("VHT rate fallback from %d to %d\n",
				//	(VHT_MCS_DATA_RATE[bw][sg][rate_idx+1]>>1), (VHT_MCS_DATA_RATE[bw][sg][rate_idx]>>1));
				pstat->atm_tx_time += ((pstat->tx_bytes_1s*8)/((VHT_MCS_DATA_RATE[bw][sg][rate_idx]>>1)*1024));
				retry_times--;
			}

			if(rate_idx==0 && retry_times>0){
				rate_idx = 16;//HT
				pstat->atm_tx_rate_id[0]=1;//fall back to HT rate
				break;
			}
		}
	}

	//HT rate fall back txtime
	if(pstat->atm_tx_rate_id[0]==1){

		bw = pstat->atm_tx_rate_id[1];
		sg = pstat->atm_tx_rate_id[2];
		rate_idx = pstat->atm_tx_rate_id[3];//init rate

		while(retry_times>0){

			//rate fall back
			if(rate_idx>0)
			{
				rate_idx--;

				ht_rate = (unsigned char *)MCS_DATA_RATEStr[bw][sg][rate_idx];
				ht_rate2 = _atoi(ht_rate, 10);
				//panic_printk("HT rate fallback from %s to %s(%d)\n",
				//	MCS_DATA_RATEStr[bw][sg][rate_idx+1], MCS_DATA_RATEStr[bw][sg][rate_idx], ht_rate2);
				pstat->atm_tx_time += ((pstat->tx_bytes_1s*8)/(ht_rate2*1024));
				retry_times--;
			}

			if(rate_idx==0 && retry_times>0){
				if(priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_2G){
					pstat->atm_tx_rate_id[1]=0;
					pstat->atm_tx_rate_id[2]=4;//CCK
				}else if(priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_5G){
					pstat->atm_tx_rate_id[1]=1;
					pstat->atm_tx_rate_id[2]=8;//CCK
				}
				pstat->atm_tx_rate_id[0]=0;//fall back to OFDM/CCK rate
				break;
			}
		}
	}

	//OFDM_CCK rate fall back txtime
	if(pstat->atm_tx_rate_id[0]==0){

		rate_idx = pstat->atm_tx_rate_id[2];//init rate

		if(pstat->atm_tx_rate_id[1]==0){//2G
			while(retry_times>0){

				//rate fall back
				if(rate_idx>0)
					rate_idx--;

				//panic_printk("CCK rate fallback from %d to %d\n",
				//	(CCK_RATE[rate_idx+1]), (CCK_RATE[rate_idx]));
				pstat->atm_tx_time += ((pstat->tx_bytes_1s*8)/(CCK_RATE[rate_idx]*1024));
				retry_times--;
			}
		}
		else if(pstat->atm_tx_rate_id[1]==1){//5G
			while(retry_times>0){

				//rate fall back
				if(rate_idx>0)
					rate_idx--;

				//panic_printk("OFDM rate fallback from %d to %d\n",
				//	(OFDM_RATE[rate_idx+1]), (OFDM_RATE[rate_idx]));
				pstat->atm_tx_time += ((pstat->tx_bytes_1s*8)/(OFDM_RATE[rate_idx]*1024));
				retry_times--;
			}
		}
	}
}
#endif

static void atm_calc_txtime(struct rtl8192cd_priv *priv)
{
	int i;
	struct stat_info *pstat;

	for(i=0; i<NUM_STAT; i++) {
		if (priv->pshare->aidarray[i] && (priv->pshare->aidarray[i]->used == TRUE)){
			pstat = &(priv->pshare->aidarray[i]->station);

			pstat->atm_tx_time = (((pstat->tx_bytes_1s*8)/(pstat->atm_tx_rate*1024))*1000)/1024;
			pstat->atm_tx_time_orig = pstat->atm_tx_time;

#if 0//debug and calc tx time
			if ((priv->up_time % 2) == 0){
				printk("[ATM] STA[%02x] txrate=%d tx_bytes_1s=%d pkt_burst_num=%d atm_burst_sent=%d atm_tx_time=%d\n",
						pstat->hwaddr[5], pstat->atm_tx_rate, pstat->tx_bytes_1s,
						pstat->atm_burst_num, pstat->atm_burst_sent, pstat->atm_tx_time);
			}
#endif

			if(priv->pshare->rf_ft_var.atm_chk_newrty)
			{
#ifdef TXRETRY_CNT
				pstat->atm_txretry_avg = 0;//reset

				pstat->atm_txretry_1s = (pstat->total_tx_retry_cnt-pstat->atm_txretry_pre);
				pstat->atm_txpkt_1s = (pstat->tx_pkts-pstat->atm_txpkt_pre);

				if(priv->pshare->rf_ft_var.atm_chk_newrty && (pstat->total_tx_retry_cnt-pstat->atm_txretry_pre)>0)
					pstat->atm_txretry_avg = ((pstat->total_tx_retry_cnt-pstat->atm_txretry_pre)/(pstat->tx_pkts-pstat->atm_txpkt_pre))+1;
				else
					pstat->atm_txretry_avg = 0;

				//if ((priv->up_time % 2) == 0)
				//	panic_printk("tx_time_orig: %d ms\n", pstat->atm_tx_time);
				if(pstat->atm_txretry_avg > 0)
					atm_calc_txretrytime(priv, pstat);
#endif
			}
			else
			{
				//use retry and retry time inrcease ratio to calc tx time
				//count retry every 1 sec
#ifdef TXRETRY_CNT
				// retry_ratio = ((total packet sent)+(total retry))/(total packet sent)
				if((pstat->tx_pkts-pstat->atm_txpkt_pre)>0){
					pstat->atm_txretry_ratio =
					(((pstat->tx_pkts-pstat->atm_txpkt_pre)+(pstat->total_tx_retry_cnt-pstat->atm_txretry_pre))*100)/(pstat->tx_pkts-pstat->atm_txpkt_pre);
				}else{
					pstat->atm_txretry_ratio = 0;
				}
				//considering rate fall back for retry
				if(priv->pshare->rf_ft_var.atm_rty_ratio > 0)
					pstat->atm_txretry_ratio = (pstat->atm_txretry_ratio*priv->pshare->rf_ft_var.atm_rty_ratio)/100;

				//record packet and retry now
				pstat->atm_txpkt_pre = pstat->tx_pkts;
				pstat->atm_txretry_pre = pstat->total_tx_retry_cnt;

				//actual tx time after including retry num
				if(pstat->atm_txretry_ratio != 0)
					pstat->atm_tx_time = (pstat->atm_tx_time*pstat->atm_txretry_ratio)/100;
#endif
			}

			//accumulate interface tx time
			priv->atm_ttl_txtime += pstat->atm_tx_time;

			//accumulate sta tx time in adj period
			pstat->atm_tx_time_static += pstat->atm_tx_time;

#if 0
			if((priv->up_time % ATM_ADJUST_TIME) == 0){
#ifdef TXRETRY_CNT
				// retry_ratio = ((total packet sent)+(total retry))/(total packet sent)
				pstat->atm_txretry_ratio =
					(((pstat->tx_pkts-pstat->atm_txpkt_pre)+(pstat->total_tx_retry_cnt-pstat->atm_txretry_pre))*100)/(pstat->tx_pkts-pstat->atm_txpkt_pre);

				//record packet and retry now
				pstat->atm_txpkt_pre = pstat->tx_pkts;
				pstat->atm_txretry_pre = pstat->total_tx_retry_cnt;

				//actual tx time after including retry num
				if(pstat->atm_txretry_ratio != 0)
					priv->atm_ttl_txtime += ((pstat->atm_tx_time_static/ATM_ADJUST_TIME)*pstat->atm_txretry_ratio)/100;
				else
#endif

					priv->atm_ttl_txtime += (pstat->atm_tx_time_static/ATM_ADJUST_TIME);

#if 0//debug
				printk("[ATM] STA[%02x] txrate=%d tx_time_avg=%d\n",
					pstat->hwaddr[5],pstat->atm_tx_rate, pstat->atm_tx_time_static/ATM_ADJUST_TIME);
#endif
			}
#endif
		}
	}

	//calc adjust factor for burst num
	int sta_txtime=0;
	if((priv->up_time % priv->pshare->rf_ft_var.atm_adj_time) == 0){
		for(i=0; i<NUM_STAT; i++) {
			if (priv->pshare->aidarray[i] && (priv->pshare->aidarray[i]->used == TRUE)){

				pstat = &(priv->pshare->aidarray[i]->station);

				//the real tx time sta should occupy
				sta_txtime = (priv->atm_ttl_txtime*pstat->atm_sta_time)/100;
				//increase or decrease ratio for this sta
				pstat->atm_adj_factor = (sta_txtime*100)/pstat->atm_tx_time_static;
				//printk("[ATM] STA[%02x] should get %d but now %d tx time, adj=%d/100\n",
				//	pstat->hwaddr[5], sta_txtime, pstat->atm_tx_time_static/ATM_ADJUST_TIME, pstat->atm_adj_factor);

				//reset accumuler tx time
				pstat->atm_tx_time_static = 0;
			}
		}
		//reset interface tx time
		priv->atm_ttl_txtime = 0;
	}
}
#endif//RTK_ATM

#ifdef STA_RATE_STATISTIC
const u2Byte CCK_OFDM_DATA_RATE[12] = {1, 2, 5, 11, /*CCK*/
									6, 9, 12, 18, 24, 36, 48, 54/*OFDM*/};
static void sta_rate_statistics(struct stat_info *pstat)
{
	int tx_idx=0xff, rx_idx=0xff, i=0;
	unsigned int tmp=0;

	//check tx rate
#ifdef RTK_AC_SUPPORT
	if(is_VHT_rate(pstat->current_tx_rate)){
		//NSS1 MCS0~9, NSS2 MCS0~9, NSS3 MCS0~9
		tx_idx = (pstat->current_tx_rate - VHT_RATE_ID) + (CCK_OFDM_RATE_NUM+HT_RATE_NUM);
		//panic_printk("[%d] tx rate = NSS%d MCS%d\n",
		//	tx_idx,
		//	(tx_idx-(CCK_OFDM_RATE_NUM+HT_RATE_NUM))/10+1,
		//	(tx_idx-(CCK_OFDM_RATE_NUM+HT_RATE_NUM))%10);
	}
	else
#endif
	if(is_MCS_rate(pstat->current_tx_rate)){
		//MCS0~15 MCS16~31: 12~
		tx_idx = (pstat->current_tx_rate - HT_RATE_ID) + CCK_OFDM_RATE_NUM;
		//panic_printk("[%d] tx rate = MCS%d\n", tx_idx, tx_idx-CCK_OFDM_RATE_NUM);
	}else{
		//CCK:0~3, OFDM:4~11
		tmp = pstat->current_tx_rate/2;
		for(i=0; i<12; i++){
			if(tmp == CCK_OFDM_DATA_RATE[i]){
				tx_idx = i;
				break;
			}
		}
		//panic_printk("[%d] tx rate = %d\n", tx_idx, tmp);
		if(tx_idx == 0xff)
			panic_printk("\n\nUndefined tx rate=%d\n\n", tmp);
	}

	//check rx rate
#ifdef RTK_AC_SUPPORT
	if(is_VHT_rate(pstat->rx_rate)){
		//NSS1 MCS0~9, NSS2 MCS0~9, NSS3 MCS0~9
		rx_idx = (pstat->rx_rate - VHT_RATE_ID) + (CCK_OFDM_RATE_NUM+HT_RATE_NUM);
		//panic_printk("[%d] rx rate = NSS%d MCS%d\n",
		//	rx_idx,
		//	(rx_idx-(CCK_OFDM_RATE_NUM+HT_RATE_NUM))/10+1,
		//	(rx_idx-(CCK_OFDM_RATE_NUM+HT_RATE_NUM))%10);
	}
	else
#endif
	if(is_MCS_rate(pstat->rx_rate)){
		//MCS0~15 MCS16~31
		rx_idx = (pstat->rx_rate - HT_RATE_ID) + CCK_OFDM_RATE_NUM;
		//panic_printk("[%d] rx rate = MCS%d\n", rx_idx, rx_idx-CCK_OFDM_RATE_NUM);
	}else{
		//CCK:0~3, OFDM:4~11
		tmp = pstat->rx_rate/2;
		for(i=0; i<12; i++){
			if(tmp == CCK_OFDM_DATA_RATE[i]){
				rx_idx = i;
				break;
			}
		}
		//panic_printk("[%d] rx rate = %d\n", rx_idx, tmp);
		if(rx_idx == 0xff)
			panic_printk("\n\nUndefined rx rate=%d\n\n", tmp);
	}

	if(tx_idx < STA_RATE_NUM)
		pstat->txrate_stat[tx_idx]++;
	else
		panic_printk("\n\nUndefined tx rate idx=%d, tx rate=%d\n\n", tx_idx, pstat->current_tx_rate);

	if(rx_idx < STA_RATE_NUM)
		pstat->rxrate_stat[rx_idx]++;
	else
		panic_printk("\n\nUndefined rx rate idx=%d, rx rate=%d\n\n", rx_idx, pstat->rx_rate);
}
#endif


static void translate_txforce_to_rateStr(unsigned char *str, unsigned int txforce)
{
	unsigned char dot11_rate_table[]={2,4,11,22,12,18,24,36,48,72,96,108,0};

	sprintf(str, "%s%s%s%s%s%d",
		((txforce >= 44)) ? "VHT " : "",
		((txforce >= 12) && (txforce < 44))? "MCS" : "",
		((txforce >= 44) && (txforce < 54)) ? "NSS1 " : "",
		((txforce >= 54) && (txforce < 64)) ? "NSS2 " : "",
		((txforce >= 64) && (txforce < 74)) ? "NSS3 " : "",
		(txforce >= 44) ? ((txforce-44)%10):((txforce >= 12)? txforce-12: dot11_rate_table[txforce]/2)
	);
}


static void translate_rateIndex_to_rateStr(unsigned char *str, unsigned int rate_index)
{
	sprintf(str, "%s%s%s%s%s%d",
			((rate_index >= VHT_RATE_ID)) ? "VHT " : "",
			((rate_index >= HT_RATE_ID) && (rate_index < VHT_RATE_ID))? "MCS" : "",
			((rate_index >=_NSS1_MCS0_RATE_) && (rate_index < _NSS2_MCS0_RATE_)) ? "NSS1 " : "",
			((rate_index >=_NSS2_MCS0_RATE_) && (rate_index < _NSS3_MCS0_RATE_)) ? "NSS2 " : "",
			((rate_index >=_NSS3_MCS0_RATE_) && (rate_index < _NSS4_MCS0_RATE_)) ? "NSS3 " : "",
			(rate_index >= VHT_RATE_ID) ? ((rate_index - VHT_RATE_ID)%10):((rate_index >= HT_RATE_ID)? (rate_index - HT_RATE_ID) : rate_index/2)
	);
}


static char * show_sta_trx_rate(struct rtl8192cd_priv *priv, struct stat_info	*pstat)
{
	char strRate[100]="";
	char strTxRate[100]="";
	char strRxRate[100]="";



	if(priv->pshare->rf_ft_var.txforce != 0xff)
	{
		unsigned char txforce = priv->pshare->rf_ft_var.txforce;
		unsigned char dot11_rate_table[]={2,4,11,22,12,18,24,36,48,72,96,108,0};
		unsigned char args[200];

		translate_txforce_to_rateStr(strTxRate, txforce);
		translate_rateIndex_to_rateStr(strRxRate, pstat->rx_rate);

		sprintf(strRate, "txforce %s%s%s rx %s%s%s ",
		strTxRate,
		(pstat->ht_current_tx_info&BIT(1))? "s" : " ",
		(txforce >= 44) ? "":((txforce >= 12)?(txforce >= 22?"	   ":"		"):(dot11_rate_table[txforce]/2 >= 11?" 	   ":"		   ")),
		strRxRate,
		pstat->rx_splcp? "s" : " ",
		(pstat->rx_rate >= VHT_RATE_ID) ? "":((pstat->rx_rate >= HT_RATE_ID)?(pstat->rx_rate >= HT_RATE_ID+10?" 	":" 	 "):(dot11_rate_table[pstat->rx_rate]/2 >= 11?" 	   ":"		   ")));
	} else {
		translate_rateIndex_to_rateStr(strTxRate, pstat->current_tx_rate);
		translate_rateIndex_to_rateStr(strRxRate, pstat->rx_rate);

		sprintf(strRate, "tx %s%s%s rx %s%s%s ",
		strTxRate,
		(pstat->ht_current_tx_info&BIT(1))? "s" : " ",
		(pstat->current_tx_rate >= VHT_RATE_ID) ? "":((pstat->current_tx_rate >= HT_RATE_ID)?(pstat->current_tx_rate >= HT_RATE_ID+10?" 	":" 	 "):(pstat->current_tx_rate/2 >= 11?"		 ":"		 ")),
		strRxRate,
		pstat->rx_splcp? "s" : " ",
		(pstat->rx_rate >= VHT_RATE_ID) ? "  ":((pstat->rx_rate >= HT_RATE_ID)?(pstat->rx_rate >= HT_RATE_ID+10?" 	":" 	 "):(pstat->rx_rate/2 >= 11?"		 ":"		 ")));
	}
	return strRate;
}

#ifdef STA_ROAMING_CHECK
static int RoamingCheck(struct rtl8192cd_priv *priv, struct stat_info *pstat)
{
    int thresh, rssi_gap, time_gap, upvalue, actor;
    thresh = priv->pmib->dot11StationConfigEntry.staAssociateRSSIThreshold;
    rssi_gap = priv->pmib->dot11StationConfigEntry.staRoamingRSSIGap;
    time_gap = priv->pmib->dot11StationConfigEntry.staRoamingTimeGap;
    upvalue = 100*time_gap;

    if(pstat->rssi > thresh + rssi_gap){
        pstat->rtl_link_roaming_value = 0;
        return 0;
    }

    if(pstat->rssi < thresh - rssi_gap){
        return 1;
    }

    if(rssi_gap == 0)
        actor = 100;
    else
        actor = ((thresh+rssi_gap) - pstat->rssi)*100/rssi_gap; //range: 0~200;

    pstat->rtl_link_roaming_value += actor;

    if(pstat->rtl_link_roaming_value >= upvalue)
        return 1;
    else
        return 0;
}
#endif

static void assoc_expire(struct rtl8192cd_priv *priv)
{
	struct stat_info	*pstat;
	struct list_head	*phead, *plist;
	unsigned int	ok_curr, ok_pre;
	unsigned int	highest_tp = 0;
	struct stat_info	*pstat_highest=NULL;
    int i,j;
#ifdef CONFIG_RTL_WLAN_DIAGNOSTIC
	char temp_log[512] = {0};
#endif
#ifdef RTK_ATM
	if(priv->pshare->rf_ft_var.atm_en)
		priv->atm_sta_num = 0;//check sta num of per SSID
#endif

	phead = &priv->asoc_list;
	plist = phead;

	while ((plist = asoc_list_get_next(priv, plist)) != phead)
	{
		pstat = list_entry(plist, struct stat_info, asoc_list);
		pstat->link_time++;

#if (MU_BEAMFORMING_SUPPORT == 1)
		if(pstat) {
			if(pstat->force_rate) {
				unsigned char *rate;
#ifdef RTK_AC_SUPPORT
				if(pstat->force_rate >= 44) {
					pstat->current_tx_rate = pstat->force_rate - 44 + 0xa0;
					pstat->mu_tx_rate = query_vht_rate(pstat);
				}
				else
#endif
				if (pstat->force_rate >=12)
				{
					pstat->current_tx_rate = pstat->force_rate - 12 + 0x80;
					rate = (unsigned char *)MCS_DATA_RATEStr[(pstat->ht_current_tx_info&BIT(0))?1:0][(pstat->ht_current_tx_info&BIT(1))?1:0][(pstat->current_tx_rate- HT_RATE_ID)];
					pstat->mu_tx_rate = _atoi(rate, 10);
				}
				else
					pstat->mu_tx_rate = pstat->force_rate/2;
			} else {
				unsigned char *rate;
#ifdef RTK_AC_SUPPORT
				if(pstat->mu_rate >= 44) {
					pstat->mu_tx_rate = query_mu_vht_rate(pstat);
				}
#endif
			}
		}
#endif

#ifdef CLIENT_MODE
		if ((OPMODE & WIFI_STATION_STATE) && (pstat->expire_to > 0)) {
			if ((priv->pshare->rf_ft_var.sta_mode_ps && !priv->ps_state) ||
				(!priv->pshare->rf_ft_var.sta_mode_ps && priv->ps_state)) {
				if (!priv->ps_state)
					priv->ps_state++;
				else
					priv->ps_state = 0;

				issue_PwrMgt_NullData(priv);
			}
#if 0//def UNIVERSAL_REPEATER
			else if (IS_VXD_INTERFACE(priv)) // repeater send null packet to remote AP every 30sec
			{
				if(IS_BSSID(priv, pstat->hwaddr)){
					if ((priv->up_time % 30) == 0)
						issue_NullData(priv, pstat->hwaddr);
					}
			}
#endif
		}
#endif


		// Check idle using packet transmit....nctu note it
		ok_curr = pstat->tx_pkts - pstat->tx_fail;
		ok_pre = pstat->tx_pkts_pre - pstat->tx_fail_pre;
		if ((ok_curr == ok_pre) &&
			(pstat->rx_pkts == pstat->rx_pkts_pre))
		{
			if (pstat->expire_to > 0)
			{
				// free queued skb if sta is idle longer than 5 seconds
				if ((priv->expire_to - pstat->expire_to) == 5){
                                    //remove this because some sta will idle over 5 sec, than the data path will disconnect for a while
		                    //free_sta_skb(priv, pstat);
				for (i=0; i<8; i++)
					for (j=0; j<TUPLE_WINDOW; j++)
						pstat->tpcache[i][j] = 0xffff;
				pstat->tpcache_mgt = 0xffff;
				}


				// calculate STA number
				if ((pstat->expire_to == 1)
#ifdef A4_STA
                    && !(pstat->state & WIFI_A4_STA)
#endif
				) {

					{
						cnt_assoc_num(priv, pstat, DECREASE, (char *)__FUNCTION__);
						check_sta_characteristic(priv, pstat, DECREASE);
					}

					// CAM entry update
					if (!SWCRYPTO && pstat->dot11KeyMapping.keyInCam) {
						if (CamDeleteOneEntry(priv, pstat->hwaddr, 0, 0)) {
							pstat->dot11KeyMapping.keyInCam = FALSE;
							pstat->tmp_rmv_key = TRUE;
							priv->pshare->CamEntryOccupied--;
						}
						#if defined(CONFIG_RTL_HW_WAPI_SUPPORT)
						if (CamDeleteOneEntry(priv, pstat->hwaddr, 0, 0)) {
							pstat->dot11KeyMapping.keyInCam = FALSE;
							pstat->tmp_rmv_key = TRUE;
							priv->pshare->CamEntryOccupied--;
						}
						#endif
					}

					release_remapAid(priv, pstat);

					LOG_MSG("A STA is expired - %02X:%02X:%02X:%02X:%02X:%02X\n",
						pstat->hwaddr[0],pstat->hwaddr[1],pstat->hwaddr[2],pstat->hwaddr[3],pstat->hwaddr[4],pstat->hwaddr[5]);


				}

				pstat->expire_to--;

				if(priv->pshare->rf_ft_var.fix_expire_to_zero == 1) {
					pstat->expire_to = 0;
					priv->pshare->rf_ft_var.fix_expire_to_zero = 0;
				}

				if (pstat->expire_to == 0) {

#if (BEAMFORMING_SUPPORT == 1)
					if ((priv->pmib->dot11RFEntry.txbf == 1) && (priv->pshare->WlanSupportAbility & WLAN_BEAMFORMING_SUPPORT))
					{
					    PRT_BEAMFORMING_INFO 	pBeamformingInfo = &(priv->pshare->BeamformingInfo);
                                    pBeamformingInfo->CurDelBFerBFeeEntrySel = BFerBFeeEntry;

						ODM_RT_TRACE(ODMPTR, PHYDM_COMP_TXBF, ODM_DBG_LOUD, ("%s,\n", __FUNCTION__));

						if(Beamforming_DeInitEntry(priv, pstat->hwaddr))
							Beamforming_Notify(priv);

					}
#endif

				}
			}
			else {
				free_sta_tx_skb(priv, pstat);
			}
		}
		else
		{
#if !defined(USE_OUT_SRC) || defined(_OUTSRC_COEXIST)
#ifdef _OUTSRC_COEXIST
			if(!IS_OUTSRC_CHIP(priv))
#endif
			{
				/*
				 * pass rssi info to f/w
				 */
			}
#endif
			if((priv->up_time %3)==1) {
#ifdef MULTI_STA_REFINE
				pstat->dropPktTotal += pstat->dropPktCurr;
				pstat->dropPktCurr = 0;
#endif
			}
#ifdef RTK_ATM
			if(priv->pshare->rf_ft_var.atm_en)
				pstat->tx_avarage_atm += pstat->tx_avarage;
#endif
			if (priv->pshare->rf_ft_var.rssi_dump && !(priv->up_time % priv->pshare->rf_ft_var.rssi_dump)) {
				unsigned char mimorssi[4];
				{
					mimorssi[0] = pstat->rf_info.mimorssi[0];
					mimorssi[1] = pstat->rf_info.mimorssi[1];
					mimorssi[2] = pstat->rf_info.mimorssi[2];
					mimorssi[3] = pstat->rf_info.mimorssi[3];
				}

				unsigned int FA_total_cnt=0, CCA_total_cnt=0, FA_OFDM_cnt=0, FA_CCK_cnt=0;

#if (PHYDM_TDMA_DIG_SUPPORT == 1)
				unsigned int tdma_FA_total_cnt[2]={0};
				unsigned int tdma_CCA_total_cnt[2]={0};
				unsigned int tdma_FA_OFDM_cnt[2]={0};
				unsigned int tdma_FA_CCK_cnt[2]={0};
				u1Byte		 tdma_high_igi, tdma_low_igi;

				tdma_high_igi = (ODMPTR->force_high_igi == 0xff) ? ODMPTR->DM_DigTable.tdma_igi[TDMA_DIG_HIGH_STATE] : ODMPTR->force_high_igi;
				tdma_low_igi = (ODMPTR->force_low_igi == 0xff) ? ODMPTR->DM_DigTable.tdma_igi[TDMA_DIG_LOW_STATE] : ODMPTR->force_low_igi;
#endif	//#if (PHYDM_TDMA_DIG_SUPPORT == 1)

#if defined(TXRETRY_CNT)
				unsigned char retry_str[30] = {0};

				if(is_support_TxRetryCnt(priv)) {
					if(pstat->cur_tx_ok || pstat->cur_tx_fail)
						sprintf(retry_str," [%2d%%] ",(pstat->total_tx_retry_cnt*100)/(pstat->cur_tx_ok+pstat->total_tx_retry_cnt+pstat->cur_tx_fail));
					else
						sprintf(retry_str," [N/A] ");
				} else
					sprintf(retry_str," ");
#endif
#ifdef _OUTSRC_COEXIST
				if(IS_OUTSRC_CHIP(priv))
#endif
				{

#if (PHYDM_TDMA_DIG_SUPPORT == 1)
					if(ODMPTR->original_dig_restore == 0) {
						for(i = 0; i<=1; i++) {
							tdma_FA_total_cnt[i] = ODMPTR->FalseAlmCnt_Acc.Cnt_all_1sec[i];
							tdma_CCA_total_cnt[i] = ODMPTR->FalseAlmCnt_Acc.Cnt_CCA_all_1sec[i];
							tdma_FA_CCK_cnt[i] = ODMPTR->FalseAlmCnt_Acc.Cnt_Cck_fail_1sec[i];
							tdma_FA_OFDM_cnt[i] = ODMPTR->FalseAlmCnt_Acc.Cnt_Ofdm_fail_1sec[i];
						}
					} else
#endif	//#if (PHYDM_TDMA_DIG_SUPPORT == 1)
					{
						FA_total_cnt = ODMPTR->FalseAlmCnt.Cnt_all;
						FA_CCK_cnt = ODMPTR->FalseAlmCnt.Cnt_Cck_fail;
						FA_OFDM_cnt = ODMPTR->FalseAlmCnt.Cnt_Ofdm_fail;
						CCA_total_cnt = ODMPTR->FalseAlmCnt.Cnt_CCA_all;
					}
				}

#if !defined(USE_OUT_SRC) || defined(_OUTSRC_COEXIST)
#ifdef _OUTSRC_COEXIST
				if(!IS_OUTSRC_CHIP(priv))
#endif
				{
					FA_total_cnt = priv->pshare->FA_total_cnt;

					FA_CCK_cnt = priv->pshare->cck_FA_cnt;
					FA_OFDM_cnt = priv->pshare->ofdm_FA_cnt1 + priv->pshare->ofdm_FA_cnt2 +
	                             priv->pshare->ofdm_FA_cnt3 + priv->pshare->ofdm_FA_cnt4 + RTL_R16(0xcf0) + RTL_R16(0xcf2);

					CCA_total_cnt = priv->pshare->CCA_total_cnt;
				}
#endif

#if 1 //txforce

				unsigned int dump_tx_tp = (unsigned int)(pstat->tx_avarage>>17);
				#ifdef RTK_ATM
				if(priv->pshare->rf_ft_var.atm_en){
					dump_tx_tp = (unsigned int)((pstat->tx_avarage_atm/priv->pshare->rf_ft_var.rssi_dump)>>17);
					pstat->tx_avarage_atm = 0;
				}
				#endif

				if(priv->pshare->rf_ft_var.txforce != 0xff)
				{
					unsigned char txforce = priv->pshare->rf_ft_var.txforce;
					unsigned char dot11_rate_table[]={2,4,11,22,12,18,24,36,48,72,96,108,0};
					unsigned char args[200];
#if defined(TXRETRY_CNT)
					SPRINTT(args, "[%d]%s%s%d%%%stxforce %s%s%s%s%s%d%s%s rx %s%s%s%s%s%d%s%s ",
							pstat->aid, pstat->aid>9?"":" ", pstat->rssi<100?(pstat->rssi<10?"  ":" "):"", pstat->rssi, retry_str,
#else
					SPRINTT(args, "[%d]%s%s%d%%  txforce %s%s%s%s%s%d%s%s rx %s%s%s%s%s%d%s%s ",
							pstat->aid, pstat->aid>9?"":" ", pstat->rssi<100?(pstat->rssi<10?"  ":" "):"", pstat->rssi,
#endif
					((txforce >= 44)) ? "VHT " : "",
					((txforce >= 12) && (txforce < 44))? "MCS" : "",
					((txforce >= 44) && (txforce < 54)) ? "NSS1 " : "",
					((txforce >= 54) && (txforce < 64)) ? "NSS2 " : "",
					((txforce >= 64) && (txforce < 74)) ? "NSS3 " : "",
					(txforce >= 44) ? ((txforce-44)%10):((txforce >= 12)? txforce-12: dot11_rate_table[txforce]/2),
					(pstat->ht_current_tx_info&BIT(1))? "s" : " ",
					(txforce >= 44) ? "":((txforce >= 12)?(txforce >= 22?"     ":"      "):(dot11_rate_table[txforce]/2 >= 11?"        ":"         ")),
					((pstat->rx_rate) >= VHT_RATE_ID)? "VHT " : "",
					((pstat->rx_rate >=_NSS1_MCS0_RATE_) && (pstat->rx_rate < _NSS2_MCS0_RATE_)) ? "NSS1 " : "",
					((pstat->rx_rate >=_NSS2_MCS0_RATE_) && (pstat->rx_rate < _NSS3_MCS0_RATE_)) ? "NSS2 " : "",
					((pstat->rx_rate >= _NSS3_MCS0_RATE_) && (pstat->rx_rate < _NSS4_MCS0_RATE_)) ? "NSS3 " : "",
					((pstat->rx_rate >= HT_RATE_ID) && (pstat->rx_rate < VHT_RATE_ID))? "MCS" : "",
					((pstat->rx_rate) >= VHT_RATE_ID) ? ((pstat->rx_rate - VHT_RATE_ID)%10) : ((pstat->rx_rate >= HT_RATE_ID)? (pstat->rx_rate - HT_RATE_ID) : pstat->rx_rate/2),
					pstat->rx_splcp? "s" : " ",
					(pstat->rx_rate >= VHT_RATE_ID) ? "":((pstat->rx_rate >= HT_RATE_ID)?(pstat->rx_rate >= HT_RATE_ID+10?"     ":"      "):(dot11_rate_table[pstat->rx_rate]/2 >= 11?"        ":"         ")));
					if(GET_CHIP_VER(priv) == VERSION_8814A)
					SPRINTT(args, " (ss %2d %2d %2d %2d)",
								mimorssi[0],
								mimorssi[1],
								mimorssi[2],
								mimorssi[3]);
					else
					SPRINTT(args, " (ss %2d %2d)",
								mimorssi[0],
								mimorssi[1]);


#if (PHYDM_TDMA_DIG_SUPPORT == 1)
					if (ODMPTR->original_dig_restore == 0) {
						SPRINTT(args, " (FA CCK %d %d OFDM %d %d)(CCA %d %d)(LIG 0x%2x,HIG 0x%2x)(TP %d,%d)",
							tdma_FA_CCK_cnt[0],
							tdma_FA_CCK_cnt[1],
							tdma_FA_OFDM_cnt[0],
							tdma_FA_OFDM_cnt[1],
							tdma_CCA_total_cnt[0],
							tdma_CCA_total_cnt[1],
							tdma_low_igi,
		   					tdma_high_igi,
							dump_tx_tp,
							(unsigned int)(pstat->rx_avarage>>17));
					} else
#endif	//#if (PHYDM_TDMA_DIG_SUPPORT == 1)
					{
						SPRINTT(args, " (FA CCK %d OFDM %d)(CCA %d)(DIG 0x%2x)(TP %d,%d)",
							FA_CCK_cnt,
							FA_OFDM_cnt,
							CCA_total_cnt,
		   					RTL_R8(0xc50),
							dump_tx_tp,
							(unsigned int)(pstat->rx_avarage>>17));
					}

#ifdef RX_CRC_EXPTIMER
					if(priv->pshare->rf_ft_var.crc_enable)
						SPRINTT(args, " (CRC %d %d)",
							priv->ext_stats.rx_packets_exptimer,
                    		priv->ext_stats.rx_crc_exptimer);
#endif
#ifdef CONFIG_RTL_WLAN_DIAGNOSTIC
					output_diag_log(args);
#else
					SPRINTT(args, "\n");
					//panic_printk("%s", args);
#endif
				}
				else
#endif
				{
					unsigned char args[200];

#if defined(TXRETRY_CNT)
					SPRINTT(args, "[%d]%s%s%d%%%stx %s%s%s%s%s%d%s%s rx %s%s%s%s%s%d%s%s ",
							pstat->aid, pstat->aid>9?"":" ", pstat->rssi<100?(pstat->rssi<10?"  ":" "):"", pstat->rssi, retry_str,
#else
					SPRINTT(args, "[%d]%s%s%d%%  tx %s%s%s%s%s%d%s%s rx %s%s%s%s%s%d%s%s ",
							pstat->aid, pstat->aid>9?"":" ", pstat->rssi<100?(pstat->rssi<10?"  ":" "):"", pstat->rssi,
#endif
					((pstat->current_tx_rate >= VHT_RATE_ID)) ? "VHT " : "",
					((pstat->current_tx_rate >= HT_RATE_ID) && (pstat->current_tx_rate < VHT_RATE_ID))? "MCS" : "",
					((pstat->current_tx_rate >=_NSS1_MCS0_RATE_) && (pstat->current_tx_rate < _NSS2_MCS0_RATE_)) ? "NSS1 " : "",
					((pstat->current_tx_rate >=_NSS2_MCS0_RATE_) && (pstat->current_tx_rate < _NSS3_MCS0_RATE_)) ? "NSS2 " : "",
					((pstat->current_tx_rate >=_NSS3_MCS0_RATE_) && (pstat->current_tx_rate < _NSS4_MCS0_RATE_)) ? "NSS3 " : "",
					(pstat->current_tx_rate >= VHT_RATE_ID) ? ((pstat->current_tx_rate - VHT_RATE_ID)%10):((pstat->current_tx_rate >= HT_RATE_ID)? (pstat->current_tx_rate - HT_RATE_ID) : pstat->current_tx_rate/2),
					(pstat->ht_current_tx_info&BIT(1))? "s" : " ",
					(pstat->current_tx_rate >= VHT_RATE_ID) ? "":((pstat->current_tx_rate >= HT_RATE_ID)?(pstat->current_tx_rate >= HT_RATE_ID+10?"     ":"      "):(pstat->current_tx_rate/2 >= 11?"        ":"         ")),
					((pstat->rx_rate) >= VHT_RATE_ID)? "VHT " : "",
					((pstat->rx_rate >=_NSS1_MCS0_RATE_) && (pstat->rx_rate < _NSS2_MCS0_RATE_)) ? "NSS1 " : "",
					((pstat->rx_rate >=_NSS2_MCS0_RATE_) && (pstat->rx_rate < _NSS3_MCS0_RATE_)) ? "NSS2 " : "",
					((pstat->rx_rate >=_NSS3_MCS0_RATE_) && (pstat->rx_rate < _NSS4_MCS0_RATE_)) ? "NSS3 " : "",
					((pstat->rx_rate >= HT_RATE_ID) && (pstat->rx_rate < VHT_RATE_ID))? "MCS" : "",
					((pstat->rx_rate) >= VHT_RATE_ID) ? ((pstat->rx_rate - VHT_RATE_ID)%10) : ((pstat->rx_rate >= HT_RATE_ID)? (pstat->rx_rate-HT_RATE_ID) : pstat->rx_rate/2),
					pstat->rx_splcp? "s" : " ",
					(pstat->rx_rate >= VHT_RATE_ID) ? "":((pstat->rx_rate >= HT_RATE_ID)?(pstat->rx_rate >= HT_RATE_ID+10?"     ":"      "):(pstat->rx_rate/2 >= 11?"        ":"         ")));
					if(GET_CHIP_VER(priv) == VERSION_8814A)
					SPRINTT(args, " (ss %2d %2d %2d %2d)",
								mimorssi[0],
								mimorssi[1],
								mimorssi[2],
								mimorssi[3]);
					else
					SPRINTT(args, " (ss %2d %2d)",
								mimorssi[0],
								mimorssi[1]);

#if (PHYDM_TDMA_DIG_SUPPORT == 1)
					if (ODMPTR->original_dig_restore == 0)
					{
						SPRINTT(args, " (FA CCK %d %d OFDM %d %d)(CCA %d %d)(LIG 0x%2x,HIG 0x%2x)(TP %d,%d)",
							tdma_FA_CCK_cnt[0],
							tdma_FA_CCK_cnt[1],
							tdma_FA_OFDM_cnt[0],
							tdma_FA_OFDM_cnt[1],
							tdma_CCA_total_cnt[0],
							tdma_CCA_total_cnt[1],
							tdma_low_igi,
							tdma_high_igi,
							dump_tx_tp,
							(unsigned int)(pstat->rx_avarage>>17));
					} else
#endif	//#if PHYDM_TDMA_DIG_SUPPORT
					{
						SPRINTT(args, " (FA CCK %d OFDM %d)(CCA %d)(DIG 0x%2x)(TP %d,%d)",
							FA_CCK_cnt,
							FA_OFDM_cnt,
							CCA_total_cnt,
							RTL_R8(0xc50),
							dump_tx_tp,
							(unsigned int)(pstat->rx_avarage>>17));
					}

#ifdef RX_CRC_EXPTIMER
					if(priv->pshare->rf_ft_var.crc_enable)
						SPRINTT(args, " (CRC %d %d)",
							priv->ext_stats.rx_packets_exptimer,
                    		priv->ext_stats.rx_crc_exptimer);
#endif
#ifdef CONFIG_RTL_WLAN_DIAGNOSTIC
					output_diag_log(args);
#else
					SPRINTT(args, "\n");
#endif
				}
#if defined(TXRETRY_CNT)
			pstat->total_tx_retry_pkts = pstat->total_tx_retry_cnt = pstat->cur_tx_ok = pstat->cur_tx_fail = 0;
#endif
#ifdef RX_CRC_EXPTIMER
            priv->ext_stats.rx_crc_exptimer = 0;
            priv->ext_stats.rx_packets_exptimer = 0;
#endif

			}

#ifdef RX_CRC_EXPTIMER
			if (priv->pshare->rf_ft_var.crc_dump  && !(priv->up_time % priv->pshare->rf_ft_var.crc_dump)) {
#ifdef CONFIG_RTL_WLAN_DIAGNOSTIC
				output_diag_log("CRC ");
#else
				panic_printk("CRC ");
#endif
				for(i=_NSS1_MCS0_RATE_;i<=_NSS1_MCS9_RATE_;i++) {
					if(priv->ext_stats.rx_packets_by_rate[i])
#ifdef CONFIG_RTL_WLAN_DIAGNOSTIC
						{
							sprintf(temp_log,"NSS1MCS%d(%d/%d) ", i-_NSS1_MCS0_RATE_,priv->ext_stats.rx_packets_by_rate[i], priv->ext_stats.rx_crc_by_rate[i]);
							output_diag_log(temp_log);
						}
#else
						panic_printk("NSS1MCS%d(%d/%d) ", i-_NSS1_MCS0_RATE_,priv->ext_stats.rx_packets_by_rate[i], priv->ext_stats.rx_crc_by_rate[i]);
#endif
				}
				for(i=_NSS2_MCS0_RATE_;i<=_NSS2_MCS9_RATE_;i++) {
					if(priv->ext_stats.rx_packets_by_rate[i])
#ifdef CONFIG_RTL_WLAN_DIAGNOSTIC
						{
							sprintf(temp_log,"NSS2MCS%d(%d/%d) ", i-_NSS2_MCS0_RATE_,priv->ext_stats.rx_packets_by_rate[i], priv->ext_stats.rx_crc_by_rate[i]);
							output_diag_log(temp_log);
						}
#else
						panic_printk("NSS2MCS%d(%d/%d) ", i-_NSS2_MCS0_RATE_,priv->ext_stats.rx_packets_by_rate[i], priv->ext_stats.rx_crc_by_rate[i]);
#endif
				}
				for(i=_NSS3_MCS0_RATE_;i<=_NSS3_MCS9_RATE_;i++) {
					if(priv->ext_stats.rx_packets_by_rate[i])
#ifdef CONFIG_RTL_WLAN_DIAGNOSTIC
						{
							sprintf(temp_log,"NSS3MCS%d(%d/%d) ", i-_NSS3_MCS0_RATE_,priv->ext_stats.rx_packets_by_rate[i], priv->ext_stats.rx_crc_by_rate[i]);
							output_diag_log(temp_log);
						}
#else
						panic_printk("NSS3MCS%d(%d/%d) ", i-_NSS3_MCS0_RATE_,priv->ext_stats.rx_packets_by_rate[i], priv->ext_stats.rx_crc_by_rate[i]);
#endif
				}
				for(i=_MCS0_RATE_;i<=_MCS23_RATE_;i++) {
					if(priv->ext_stats.rx_packets_by_rate[i])
#ifdef CONFIG_RTL_WLAN_DIAGNOSTIC
						{
							sprintf(temp_log,"MCS%d(%d/%d) ", i-_MCS0_RATE_,priv->ext_stats.rx_packets_by_rate[i], priv->ext_stats.rx_crc_by_rate[i]);
							output_diag_log(temp_log);
						}
#else
						panic_printk("MCS%d(%d/%d) ", i-_MCS0_RATE_,priv->ext_stats.rx_packets_by_rate[i], priv->ext_stats.rx_crc_by_rate[i]);
#endif
				}
#ifdef CONFIG_RTL_WLAN_DIAGNOSTIC
				output_diag_log("\n");
#else
				panic_printk("\n");
#endif
				memset(priv->ext_stats.rx_crc_by_rate, 0, 256*sizeof(unsigned long));
				memset(priv->ext_stats.rx_packets_by_rate, 0, 256*sizeof(unsigned long));
			}
#endif

#ifdef MCR_WIRELESS_EXTEND
			if (IS_HAL_CHIP(priv) && (pstat->IOTPeer == HT_IOT_PEER_CMW)) {
				GET_HAL_INTERFACE(priv)->UpdateHalRAMaskHandler(priv, pstat, 3);
			}
#endif



			// calculate STA number
			if ((pstat->expire_to == 0)
#ifdef A4_STA
                && !(pstat->state & WIFI_A4_STA)
#endif

			) {
				cnt_assoc_num(priv, pstat, INCREASE, (char *)__FUNCTION__);
				check_sta_characteristic(priv, pstat, INCREASE);

				// CAM entry update
				if (!SWCRYPTO) {
					if (priv->pmib->dot118021xAuthEntry.dot118021xAlgrthm ||
							pstat->tmp_rmv_key == TRUE) {
						unsigned int privacy = pstat->dot11KeyMapping.dot11Privacy;

						if (CamAddOneEntry(priv, pstat->hwaddr, 0, privacy<<2, 0,
								pstat->dot11KeyMapping.dot11EncryptKey.dot11TTKey.skey)) {
							pstat->dot11KeyMapping.keyInCam = TRUE;
							pstat->tmp_rmv_key = FALSE;
							priv->pshare->CamEntryOccupied++;
							assign_aggre_mthod(priv, pstat);
						}
						else {
							if (pstat->aggre_mthd != AGGRE_MTHD_NONE)
								pstat->aggre_mthd = AGGRE_MTHD_NONE;
						}
					}
				}

				// Resume Ratid

				if (IS_HAL_CHIP(priv))
					GET_HAL_INTERFACE(priv)->UpdateHalRAMaskHandler(priv, pstat, 3);
				else
				{
				}

				//pstat->dwngrade_probation_idx = pstat->upgrade_probation_idx = 0;	// unused
				LOG_MSG("A expired STA is resumed - %02X:%02X:%02X:%02X:%02X:%02X\n",
					pstat->hwaddr[0],pstat->hwaddr[1],pstat->hwaddr[2],pstat->hwaddr[3],pstat->hwaddr[4],pstat->hwaddr[5]);
			}

			{
				pstat->expire_to = priv->expire_to;
			}

		}

#if defined(RTK_AC_SUPPORT)
			// operating mode notification
			if(priv->pshare->rf_ft_var.opmtest&2) {
				issue_op_mode_notify(priv, pstat, priv->pshare->rf_ft_var.oper_mode_field);
				priv->pshare->rf_ft_var.opmtest &= 1;
			}
#endif

		// update proc bssdesc
		if ((OPMODE & WIFI_STATION_STATE) && isEqualMACAddr(priv->pmib->dot11Bss.bssid, pstat->hwaddr)) {
			priv->pmib->dot11Bss.rssi = pstat->rssi;
			priv->pmib->dot11Bss.sq = pstat->sq;
		}

#if (BEAMFORMING_SUPPORT == 1)
			if (GET_CHIP_VER(priv) != VERSION_8822B)
			if ((priv->pmib->dot11RFEntry.txbf == 1) && (priv->pmib->dot11RFEntry.txbfer == 1) && ((priv->up_time % 3) == 0) &&
				(priv->pshare->WlanSupportAbility & WLAN_BEAMFORMING_SUPPORT))	{
				pstat->bf_score = 0;
				if(((pstat->ht_cap_len && (cpu_to_le32(pstat->ht_cap_buf.txbf_cap)&_HTCAP_RECEIVED_NDP))
#ifdef RTK_AC_SUPPORT
				|| (pstat->vht_cap_len && (cpu_to_le32(pstat->vht_cap_buf.vht_cap_info) & (BIT(SU_BFEE_S)|BIT(SU_BFER_S))))
#endif
				)
				&& (pstat->IOTPeer != HT_IOT_PEER_INTEL)
				) {
					if( ( pstat->rssi > 10) &&
						( pstat->rx_pkts != pstat->rx_pkts_pre) &&
#ifdef DETECT_STA_EXISTANCE
						(!(pstat->leave))
#endif
					){
						u1Byte					Idx = 0;

						PRT_BEAMFORMING_ENTRY	pEntry, pBfeeEntry;
						pEntry = Beamforming_GetEntryByMacId(priv, pstat->aid, &Idx);
						if(pEntry == NULL) {
                    		ODM_RT_TRACE(ODMPTR, PHYDM_COMP_TXBF, ODM_DBG_LOUD, ("[Beamforming]@%s, Beamforming_GetFreeBFeeEntry\n", __FUNCTION__));
							pBfeeEntry = Beamforming_GetFreeBFeeEntry(priv, &Idx, pstat->hwaddr);
							if(pBfeeEntry) {
								Beamforming_Enter(priv, pstat);
							}
						}
					}
					pstat->bf_score = 100 - pstat->rssi;
					if(pstat->tx_byte_cnt> (1<<16)) {			// 0.5M bps
						pstat->bf_score += 100;
					}
					//make vxd connected AP prior to other stations connected to AP with traffic
					if(IS_VXD_INTERFACE(priv))
						pstat->bf_score += 1000;
					if((priv->pmib->dot11nConfigEntry.dot11nLDPC) && (pstat) &&
							((pstat->ht_cap_len && cpu_to_le16(pstat->ht_cap_buf.ht_cap_info) & _HTCAP_SUPPORT_RX_LDPC_)
#ifdef RTK_AC_SUPPORT
							||	(pstat->vht_cap_len && (cpu_to_le32(pstat->vht_cap_buf.vht_cap_info) & BIT(RX_LDPC_E)))
#endif
					))
						pstat->bf_score -=2;
					if(priv->pmib->dot11nConfigEntry.dot11nSTBC &&
								((pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_RX_STBC_CAP_))
#ifdef RTK_AC_SUPPORT
									|| (pstat->vht_cap_buf.vht_cap_info & cpu_to_le32(_VHTCAP_RX_STBC_CAP_))
#endif
					))	{
						pstat->bf_score -=5;
						if(!(pstat->MIMO_ps & _HT_MIMO_PS_STATIC_)) {
#ifdef RTK_AC_SUPPORT
							if (pstat->vht_cap_len) {
								if(((le32_to_cpu(pstat->vht_cap_buf.vht_support_mcs[0]) >> 2) & 3) != 3)
									pstat->bf_score -=3;
							} else
#endif
							{
								pstat->tx_ra_bitmap = 0;
								if (pstat->ht_cap_len) {
									for (i=0; i<16; i++) {
										if (pstat->ht_cap_buf.support_mcs[i/8] & BIT(i%8))
											pstat->tx_ra_bitmap |= BIT(i+12);
									}
									if(pstat->tx_ra_bitmap & 0x0ff00000)
										pstat->bf_score -=3;
								}
							}
						}
					}
				}
			}
#endif
#ifdef RTK_ATM//count tx bytes 1s
		pstat->tx_bytes_1s = pstat->tx_bytes - pstat->tx_bytes_pre;
		pstat->tx_bytes_pre = pstat->tx_bytes;
#endif
		pstat->tx_pkts_pre = pstat->tx_pkts;
		pstat->rx_pkts_pre = pstat->rx_pkts;
		pstat->tx_fail_pre = pstat->tx_fail;

		if ((priv->up_time % 3) == 0) {
#ifndef DRVMAC_LB
#if !defined(USE_OUT_SRC) || defined(_OUTSRC_COEXIST)
			if (is_auto_rate(priv, pstat)
				|| (should_restrict_Nrate(priv, pstat) && is_fixedMCSTxRate(priv, pstat)))
				check_RA_by_rssi(priv, pstat);
#endif
#endif


			/*Now 8812 use txreport for get txreport and inital tx rate*/
			#if	0	//defined(CONFIG_RTL_8812_SUPPORT)
			if(GET_CHIP_VER(priv)==VERSION_8812E){
				check_txrate_by_reg_8812(priv, pstat);
			}
			#endif

			/*
			 *	Check if station is 2T
			 */
		 	if (!pstat->is_2t_mimo_sta && (pstat->highest_rx_rate >= _MCS8_RATE_))
				pstat->is_2t_mimo_sta = TRUE;
#ifdef RTK_AC_SUPPORT
			// Dynamic Enable/Disable LDPC
			if((pstat->IOTPeer == HT_IOT_PEER_REALTEK_8812) &&
				(pstat->WirelessMode & (WIRELESS_MODE_AC_5G|WIRELESS_MODE_AC_24G)) &&
				( (pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SHORTGI_20M_ | _HTCAP_SHORTGI_40M_))
				|| ( cpu_to_le32(pstat->vht_cap_buf.vht_cap_info) & BIT(SHORT_GI80M_E))
			)){
				const char thd = 45;
#ifdef MCR_WIRELESS_EXTEND
				thd = priv->pshare->rf_ft_var.disable_ldpc_thd;
#endif
				if (!pstat->disable_ldpc && pstat->rssi> thd+5)
					pstat->disable_ldpc=1;
				else if(pstat->disable_ldpc && pstat->rssi <thd)
					pstat->disable_ldpc=0;
			}
#endif
#if !defined(USE_OUT_SRC) || defined(_OUTSRC_COEXIST)
#ifdef _OUTSRC_COEXIST
			if(!IS_OUTSRC_CHIP(priv))
#endif
			{

						/*
						 *	Check if station is near by to use lower tx power
						 */
						if((priv->pshare->FA_total_cnt > 1000) || (priv->pshare->FA_total_cnt > 300 && (RTL_R8(0xc50) & 0x7f) >= 0x32)) {
							pstat->hp_level = 0;
						} else {
							if (priv->pshare->rf_ft_var.tx_pwr_ctrl) {
								if ((pstat->hp_level == 0) && (pstat->rssi > HP_LOWER+4)){
									pstat->hp_level = 1;
								}else if ((pstat->hp_level == 1) && (pstat->rssi < (HP_LOWER - 4) )){
									pstat->hp_level = 0;
			                    }
							}
						}
			}
#endif
		}
#if 0
		{
			{
				if (!pstat->useCts2self && pstat->rssi > priv->pshare->rf_ft_var.rts_rssith+5)
					pstat->useCts2self=1; // cts2self
				else if(pstat->useCts2self && pstat->rssi<priv->pshare->rf_ft_var.rts_rssith)
					pstat->useCts2self=0; // RTS
			}
		}
#endif

#ifdef RTK_AC_SUPPORT
		if( (AC_SIGMA_MODE==AC_SIGMA_NONE)
		&& ((GET_CHIP_VER(priv) == VERSION_8812E) || (GET_CHIP_VER(priv) == VERSION_8812E) )
		&& (pstat->WirelessMode == WIRELESS_MODE_AC_5G)
		&& (((le32_to_cpu(pstat->vht_cap_buf.vht_support_mcs[0]) >> 2) & 3) == 3)		// 1T1R
		)
		{
			pstat->useCts2self=1;
		} else
#endif
		{
			pstat->useCts2self=0; // always RTS
		}

#ifdef dybw_tx
		if(OPMODE & WIFI_AP_STATE) {
			if(priv->pshare->rf_ft_var.bws_enable & 0x2) {
				if(GET_CHIP_VER(priv) == VERSION_8812E)
					dynamic_AC_bandwidth(priv,pstat);
#if defined(WIFI_11N_2040_COEXIST_EXT)
                if((GET_CHIP_VER(priv) == VERSION_8192E) && priv->pmib->dot11nConfigEntry.dot11nCoexist)
					dynamic_N_bandwidth(priv,pstat);
#endif
			}
		}
#endif

		if (pstat->IOTPeer == HT_IOT_PEER_INTEL)
		{
			//Intel-7260 RTS IOT	issue
			if (pstat->no_rts && RTL_R8(0xc50) >= 0x30) {
				pstat->no_rts = 0;
			} else if (!pstat->no_rts && RTL_R8(0xc50) < 0x30) {
				pstat->no_rts = 1;
			}
#if 0//def CONFIG_RTL_92D_SUPPORT
			if (GET_CHIP_VER(priv) == VERSION_8192D) {
				if ((((priv->ext_stats.tx_avarage>>17) + (priv->ext_stats.rx_avarage>>17)) > 80) && (pstat->no_rts == 1))
					pstat->no_rts = 0;
				else if ((((priv->ext_stats.tx_avarage>>17) + (priv->ext_stats.rx_avarage>>17)) < 60) && (pstat->no_rts == 0))
					pstat->no_rts = 1;
			}
#endif
#if 0//def CONFIG_RTL_88E_SUPPORT
			if (GET_CHIP_VER(priv)==VERSION_8188E) {
				const char thd = 20;
				if (!pstat->no_rts && pstat->rssi<thd)
					pstat->no_rts=1;
				else if(pstat->no_rts && pstat->rssi>thd+5)
					pstat->no_rts=0;
			}
#endif

			if(priv->pshare->rf_ft_var.rts_iot_th) {
				if (!pstat->no_rts && (pstat->rssi > priv->pshare->rf_ft_var.rts_iot_th))
				{
					pstat->no_rts=1;
					pstat->useCts2self=1;
				}
				else if(pstat->no_rts && (pstat->rssi < (priv->pshare->rf_ft_var.rts_iot_th-3)))
				{
					pstat->no_rts=0;
					pstat->useCts2self=0;
				}
			}

			/* Count every Intel clients with complying throughput margin */
			if ((pstat->tx_byte_cnt + pstat->rx_byte_cnt) >= priv->pshare->rf_ft_var.intel_rtylmt_tp_margin)
				priv->pshare->intel_active_sta++;
		}
#if 0
//#ifdef AC2G_256QAM
	{
		if ( (is_ac2g(priv)) && (GET_CHIP_VER(priv)==VERSION_8812E) )
		{
			if (pstat->rssi < priv->pshare->rf_ft_var.ac2g_thd_ldpc)
			{
				priv->pmib->dot11nConfigEntry.dot11nLDPC = 1;
			}
			else if(pstat->rssi > (priv->pshare->rf_ft_var.ac2g_thd_ldpc+5))
			{
				priv->pmib->dot11nConfigEntry.dot11nLDPC = 0;
			}
		}
	}
#endif

#ifdef _OUTSRC_COEXIST
		if(IS_OUTSRC_CHIP(priv))
#endif
		ChooseIotMainSTA(priv, pstat);

#if !defined(USE_OUT_SRC) || defined(_OUTSRC_COEXIST)
#ifdef _OUTSRC_COEXIST
		if(!IS_OUTSRC_CHIP(priv))
#endif
		choose_IOT_main_sta(priv, pstat);
#endif

#ifdef SW_TX_QUEUE
#ifdef RTK_ATM
		if(!priv->pshare->rf_ft_var.atm_en)
#endif
        {
            SMP_LOCK_XMIT(flags);
            for (i=BK_QUEUE;i<=VO_QUEUE;i++)
            {
                if(priv->pshare->swq_use_hw_timer) {
                    if (priv->pshare->swq_en && pstat->swq.swq_en[i] && (pstat->tx_avarage > 25000) && (pstat->ht_cap_len)) {
                        adjust_swq_setting(priv, pstat, i, CHECK_INC_AGGN);
                    }
                }
                else {
                    int q_aggnumIncSlow = (priv->assoc_num > 1) ? (2+pstat->swq.q_aggnumIncSlow[i]) : (1+pstat->swq.q_aggnumIncSlow[i]);
                    if ((priv->pshare->swq_en == 0) /*|| (((priv->ext_stats.tx_avarage>>17) + (priv->ext_stats.rx_avarage>>17)) < 20)*/) { // disable check for small udp packet(88B) test with veriwave tool
                        pstat->swq.q_aggnumIncSlow[i] = 0;
                    }
                    else if (((priv->up_time % q_aggnumIncSlow) == 0) &&
                            ((priv->pshare->swqen_keeptime != 0) &&
                            (priv->up_time > priv->pshare->swqen_keeptime+3)) &&
                            (pstat->tx_avarage > 25000) && (pstat->ht_cap_len)) {
                        adjust_swq_setting(priv, pstat, i, CHECK_INC_AGGN);
                    }

                }

                /*clear used*/
                pstat->swq.q_used[i] = 0;
                pstat->swq.q_TOCount[i] = 0;
            }
            SMP_UNLOCK_XMIT(flags);
        }
#endif
		// calculate tx/rx throughput
		pstat->tx_avarage = (pstat->tx_avarage/10)*7 + (pstat->tx_byte_cnt/10)*3;
		pstat->tx_byte_cnt_LowMAW = (pstat->tx_byte_cnt_LowMAW/10)*1 + (pstat->tx_byte_cnt/10)*9;
		pstat->tx_byte_cnt = 0;
		pstat->rx_avarage = (pstat->rx_avarage/10)*7 + (pstat->rx_byte_cnt/10)*3;
		pstat->rx_byte_cnt_LowMAW = (pstat->rx_byte_cnt_LowMAW/10)*1 + (pstat->rx_byte_cnt/10)*9;
		pstat->rx_byte_cnt = 0;

#ifdef PREVENT_BROADCAST_STORM
		// reset rx_pkts_bc in every one second
		pstat->rx_pkts_bc = 0;
#endif
		if ((pstat->tx_avarage + pstat->rx_avarage) > highest_tp) {
			highest_tp = pstat->tx_avarage + pstat->rx_avarage;
			pstat_highest = pstat;
		}

#ifdef DYNAMIC_AUTO_CHANNEL//SW#5
		if(priv->pmib->dot11RFEntry.dynamicACS_idle>0 && priv->pshare->all_cli_idle==1)
			{
				//if one client idle time is little than periodicACS_idle, all_cli_idle=0
				if((priv->pmib->dot11OperationEntry.expiretime/100 - pstat->expire_to)<priv->pmib->dot11RFEntry.dynamicACS_idle)
						priv->pshare->all_cli_idle = 0;
			}
#endif


        if((GET_CHIP_VER(priv) == VERSION_8814A)||(GET_CHIP_VER(priv) == VERSION_8822B))
            assign_aggre_mthod(priv,pstat);
		/*
	         * Broadcom, Intel IOT, dynamic inc or dec retry count
        	 */


			if ((pstat->IOTPeer == HT_IOT_PEER_BROADCOM) || (pstat->IOTPeer == HT_IOT_PEER_INTEL))
        	{
                	int i;
					if (((pstat->tx_avarage + pstat->rx_avarage >= RETRY_TRSHLD_H)
#ifdef CLIENT_MODE
					|| ((OPMODE & WIFI_STATION_STATE) && (pstat->IOTPeer == HT_IOT_PEER_BROADCOM) &&  (pstat->rssi >= 45))
#endif
						) && (pstat->retry_inc == 0))
        	        {
#ifdef TX_SHORTCUT
                	        for (i=0; i<TX_SC_ENTRY_NUM; i++)
                	        {
                                if (IS_HAL_CHIP(priv)) {
                                    GET_HAL_INTERFACE(priv)->SetShortCutTxBuffSizeHandler(priv, pstat->tx_sc_ent[i].hal_hw_desc, 0);
                                } else if(CONFIG_WLAN_NOT_HAL_EXIST)
                                {//not HAL
                        	        pstat->tx_sc_ent[i].hwdesc1.Dword7 &= set_desc(~TX_TxBufSizeMask);
                                }
                	        }
#endif
	                        pstat->retry_inc = 1;
        	        }
					else if (((pstat->tx_avarage + pstat->rx_avarage < RETRY_TRSHLD_L)
#ifdef CLIENT_MODE
						|| ((OPMODE & WIFI_STATION_STATE) && (pstat->IOTPeer == HT_IOT_PEER_BROADCOM) &&  (pstat->rssi <= 30))
#endif
						)&& (pstat->retry_inc == 1))
	                {
#ifdef TX_SHORTCUT
    	                for (i=0; i<TX_SC_ENTRY_NUM; i++)
    	                {
                            if (IS_HAL_CHIP(priv)) {
                                GET_HAL_INTERFACE(priv)->SetShortCutTxBuffSizeHandler(priv, pstat->tx_sc_ent[i].hal_hw_desc, 0);
                            } else if(CONFIG_WLAN_NOT_HAL_EXIST)
                            {//not HAL
            	                pstat->tx_sc_ent[i].hwdesc1.Dword7 &= set_desc(~TX_TxBufSizeMask);
                            }
						}
#endif
						pstat->retry_inc = 0;
        	        }
        	}

		if (((int)pstat->expire_to > ((int)priv->expire_to - priv->pshare->rf_ft_var.rssi_expire_to))
#ifdef DETECT_STA_EXISTANCE
			&& (!pstat->leave)
#endif
			)
		{
			if (pstat->rssi < priv->pshare->rssi_min) {
				if ( !IEEE8021X_FUN ||
					(IEEE8021X_FUN && pstat->ieee8021x_ctrlport != DOT11_PortStatus_Unauthorized) )
					priv->pshare->rssi_min = pstat->rssi;

				for(i = 0; i <= 3; i++) {
					if(pstat->rf_info.mimorssi[i] != 0) {
						if(pstat->rf_info.mimorssi[i] < priv->pshare->mimorssi_min)
							priv->pshare->mimorssi_min = pstat->rf_info.mimorssi[i];
					}
				}
			}

#ifdef RSSI_MIN_ADV_SEL
			collect_min_rssi_data(priv, pstat);
#endif
		}

#ifdef MCR_WIRELESS_EXTEND
		if (pstat->IOTPeer == HT_IOT_PEER_CMW)
			priv->pshare->rssi_min = pstat->rssi;
#endif

#ifdef RTK_ATM
		if(priv->pshare->rf_ft_var.atm_en)
			priv->atm_sta_num++;//check sta num of per SSID
#endif//RTK_ATM

		/*
		 *	Periodically clear ADDBAreq sent indicator
		 */
		if ((pstat->expire_to > 0) && pstat->ht_cap_len && (pstat->aggre_mthd & AGGRE_MTHD_MPDU))
			memset(pstat->ADDBA_sent, 0, 8);

		if (pstat->rssi < priv->pmib->dot11StationConfigEntry.RmStaRSSIThreshold) {
			unsigned char sta_mac[16];
			sprintf(sta_mac,"%02X%02X%02X%02X%02X%02X", pstat->hwaddr[0],pstat->hwaddr[1],pstat->hwaddr[2],pstat->hwaddr[3],pstat->hwaddr[4],pstat->hwaddr[5]);
#ifdef STA_ASSOC_STATISTIC
			add_disconnect_sta(priv, pstat->hwaddr, pstat->rssi);
#endif
			del_sta(priv, sta_mac);
		}

#ifdef STA_ROAMING_CHECK
        int ret;
        if(pstat && priv->pmib->dot11StationConfigEntry.staAssociateRSSIThreshold != 0) {
            ret = RoamingCheck(priv, pstat);
            if(ret)
            {
                unsigned char sta_mac[16];
                sprintf(sta_mac,"%02X%02X%02X%02X%02X%02X", pstat->hwaddr[0],pstat->hwaddr[1],pstat->hwaddr[2],pstat->hwaddr[3],pstat->hwaddr[4],pstat->hwaddr[5]);
#ifdef STA_ASSOC_STATISTIC
                add_disconnect_sta(priv, pstat->hwaddr, pstat->rssi);
#endif
                del_sta(priv, sta_mac);
            }
        }
#endif

#ifdef STA_RATE_STATISTIC
		sta_rate_statistics(pstat);
#endif

		if (plist == plist->next)
			break;
	}

#ifdef RTK_ATM
	if(priv->pshare->rf_ft_var.atm_en){
		//set sta time connected to this interface
		if(priv->atm_sta_num > 0)
			atm_set_statime(priv);
	}
#endif

	/*dynamic tuning response date rate*/
	if (IS_ROOT_INTERFACE(priv))
	{
#ifdef MP_TEST
		if (!((OPMODE & WIFI_MP_STATE) || priv->pshare->rf_ft_var.mp_specific))
#endif
		{
			if (priv->up_time % 2){
				if(priv->pshare->rssi_min!=0xff){
						dynamic_response_rate(priv, priv->pshare->rssi_min);
				}
			}
		}
	}



	/*
	 * Intel IOT, dynamic enhance beacon tx AGC
	 */
	pstat = pstat_highest;

	if (pstat && pstat->IOTPeer == HT_IOT_PEER_INTEL)
	{
		const char thd = 25;
		if (!priv->bcnTxAGC) {
			if (pstat->rssi < thd)
				priv->bcnTxAGC = 2;
			else if (pstat->rssi < thd+5)
				priv->bcnTxAGC = 1;
		} else if (priv->bcnTxAGC == 1) {
			if (pstat->rssi < thd)
				priv->bcnTxAGC = 2;
			else if (pstat->rssi > thd+10)
				priv->bcnTxAGC = 0;
		} else if (priv->bcnTxAGC == 2) {
			if (pstat->rssi > thd+10)
				priv->bcnTxAGC = 0;
			else if (pstat->rssi > thd+5)
				priv->bcnTxAGC = 1;
		}
 	} else {
		if (priv->bcnTxAGC)
		 	priv->bcnTxAGC = 0;
	}
}



BOOLEAN check_adaptivity_test(struct rtl8192cd_priv *priv)
{
	unsigned int 	value32;

	if ( !priv->pshare->rf_ft_var.adaptivity_enable )
		return FALSE;

	if (IS_OUTSRC_CHIP(priv))
	{
		if (ODMPTR->SupportICType & ODM_IC_11N_SERIES)
		{
			ODM_SetBBReg(ODMPTR,ODM_REG_DBG_RPT_11N, bMaskDWord, 0x208);
			value32 = ODM_GetBBReg(ODMPTR, ODM_REG_RPT_11N, bMaskDWord);
		}
		else if (ODMPTR->SupportICType & ODM_IC_11AC_SERIES)
		{
			ODM_SetBBReg(ODMPTR,ODM_REG_DBG_RPT_11AC, bMaskDWord, 0x209);
			value32 = ODM_GetBBReg(ODMPTR,ODM_REG_RPT_11AC, bMaskDWord);
		}

		if ((value32 & BIT30) && (ODMPTR->SupportICType & (ODM_RTL8723B|ODM_RTL8188E)))
			return TRUE;
		else if(value32 & BIT29)
			return TRUE;
		else
			return FALSE;
	}
	else
	{
		PHY_SetBBReg(priv,0x908, bMaskDWord, 0x208);
		value32 = PHY_QueryBBReg(priv, 0xdf4, bMaskDWord);

		if ( (GET_CHIP_VER(priv) == VERSION_8188E) && (value32 & BIT30) )	// for OSK platform
			return TRUE;
		else if(value32 & BIT29)
			return TRUE;
		else
			return FALSE;
	}
}

#ifdef CHECK_HANGUP
#ifdef CHECK_TX_HANGUP
static int check_tx_hangup(struct rtl8192cd_priv *priv, int q_num, int *pTail, int *pIsEmpty)
{
	struct tx_desc	*pdescH, *pdesc;
	volatile int	head, tail;
	struct rtl8192cd_hw	*phw;

	phw	= GET_HW(priv);
	head	= get_txhead(phw, q_num);
	tail	= get_txtail(phw, q_num);
	pdescH	= get_txdesc(phw, q_num);

	*pTail = tail;
	if (CIRC_CNT_RTK(head, tail, CURRENT_NUM_TX_DESC))
	{
		*pIsEmpty = 0;
		pdesc = pdescH + (tail);

		if (pdesc && ((get_desc(pdesc->Dword0)) & TX_OWN)) // pending
		{
			//	In adaptivity test, we need avoid check tx hang
		//	if( check_adaptivity_test(priv) )
		//		return 0;
		//	else
			return 1;
		}
	}
	else
		*pIsEmpty = 1;

	return 0;
}
#endif


#ifdef CHECK_RX_HANGUP
static void check_rx_hangup_send_pkt(struct rtl8192cd_priv *priv)
{
	struct stat_info *pstat;
	struct list_head *phead = &priv->asoc_list;
	struct list_head *plist = phead;
	DECLARE_TXINSN(txinsn);

	while ((plist = asoc_list_get_next(priv, plist)) != phead)
	{
		pstat = list_entry(plist, struct stat_info, asoc_list);

		if (pstat->expire_to > 0) {
			txinsn.retry = priv->pmib->dot11OperationEntry.dot11ShortRetryLimit;

			txinsn.q_num = MANAGE_QUE_NUM;
			txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
			txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif
			txinsn.fixed_rate = 1;
			txinsn.fr_type = _PRE_ALLOCHDR_;
			txinsn.phdr = get_wlanhdr_from_poll(priv);
			txinsn.pframe = NULL;

			if (txinsn.phdr == NULL)
				goto send_test_pkt_fail;

			memset((void *)(txinsn.phdr), 0, sizeof (struct	wlan_hdr));

			SetFrameSubType(txinsn.phdr, WIFI_DATA_NULL);

			if (OPMODE & WIFI_AP_STATE) {
				memcpy((void *)GetAddr1Ptr((txinsn.phdr)), pstat->hwaddr, MACADDRLEN);
				memcpy((void *)GetAddr2Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
				memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
			}
			else {
				memcpy((void *)GetAddr1Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
				memcpy((void *)GetAddr2Ptr((txinsn.phdr)), pstat->hwaddr, MACADDRLEN);
				memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
			}

			txinsn.hdr_len = WLAN_HDR_A3_LEN;

			if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS) {
				asoc_list_unref(priv, pstat);
				return;
			}

send_test_pkt_fail:

			if (txinsn.phdr)
				release_wlanhdr_to_poll(priv, txinsn.phdr);
		}
	}
}


static void check_rx_hangup(struct rtl8192cd_priv *priv)
{
	if (priv->rx_start_monitor_running == 0) {
		if (UINT32_DIFF(priv->rx_packets_pre1, priv->rx_packets_pre2) > 20) {
			priv->rx_start_monitor_running = 1;
			//printk("start monitoring = %d\n", priv->rx_start_monitor_running);
		}
	}
	else if (priv->rx_start_monitor_running == 1) {
		if (UINT32_DIFF(priv->net_stats.rx_packets, priv->rx_packets_pre1) == 0)
			priv->rx_start_monitor_running = 2;
		else if (UINT32_DIFF(priv->net_stats.rx_packets, priv->rx_packets_pre1) < 2 &&
			UINT32_DIFF(priv->net_stats.rx_packets, priv->rx_packets_pre1) > 0) {
			priv->rx_start_monitor_running = 0;
			//printk("stop monitoring = %d\n", priv->rx_start_monitor_running);
		}
	}

	if (priv->rx_start_monitor_running >= 2) {
		//printk("\n\n%s %d start monitoring = 2 rx_packets_pre1=%lu; rx_packets_pre2=%lu; net_stats.rx_packets=%lu\n",
			//__FUNCTION__, __LINE__,
			//priv->rx_packets_pre1, priv->rx_packets_pre2,
			//priv->net_stats.rx_packets);
		priv->pshare->rx_hang_checking = 1;
		priv->pshare->selected_priv = priv;;
	}
}


static __inline__ void check_rx_hangup_record_rxpkts(struct rtl8192cd_priv *priv)
{
	priv->rx_packets_pre2 = priv->rx_packets_pre1;
	priv->rx_packets_pre1 = priv->net_stats.rx_packets;
}
#endif // CHECK_RX_HANGUP




int check_hangup(struct rtl8192cd_priv *priv)
{
	unsigned long	flags = 0;

#ifdef CHECK_TX_HANGUP
	int tail, q_num, is_empty, alive=0, max_q_num;
#endif
	int txhangup, rxhangup, beacon_hangup, reset_fail_hangup, fw_error, adaptivity_hangup;
	int pcie_tx_stuck=0, pcie_rx_stuck=0;
    u4Byte hal_hang_state = 0;

	int i;
#ifdef FAST_RECOVERY
	void *info = NULL;
	void *vxd_info = NULL;
#endif // FAST_RECOVERY
#ifdef CHECK_RX_HANGUP
	unsigned int rx_cntreg;
#endif

	void *vap_info[RTL8192CD_NUM_VWLAN];
	memset(vap_info, 0, sizeof(vap_info));

	BOOLEAN check_adaptivity_hangup = FALSE;

/*
#if defined(CHECK_BEACON_HANGUP)
	unsigned int BcnQ_Val = 0;
#endif
*/

// for debug
#if 0
	__DRAM_IN_865X static unsigned char temp_reg_C50, temp_reg_C58,
 		temp_reg_C60, temp_reg_C68, temp_reg_A0A;
  	temp_reg_C50 = 0; temp_reg_C58 = 0; temp_reg_C60 = 0;
	temp_reg_C68 = 0; temp_reg_A0A = 0;
#endif
//---------------------------------------------------------
//	margin = -1;

	txhangup = rxhangup = beacon_hangup = reset_fail_hangup = fw_error = adaptivity_hangup = 0;

	if ( priv->pshare->rf_ft_var.adaptivity_enable == 3 && !priv->pshare->rf_ft_var.isCleanEnvironment )
		check_adaptivity_hangup = TRUE;

    if (IS_HAL_CHIP(priv)) {

#if defined(CHECK_LX_DMA_ERROR)
		if(priv->pshare->rf_ft_var.check_hang &  CHECK_LX_DMA_ERROR) {
	        hal_hang_state = GET_HAL_INTERFACE(priv)->CheckHangHandler(priv);

	        switch (hal_hang_state) {
	        case HANG_VAR_TX_STUCK:
	            txhangup = 1;
				pcie_tx_stuck = 1;
	            break;
	        case HANG_VAR_RX_STUCK:
	            rxhangup = 1;
				pcie_rx_stuck = 1;
	            break;
	        default: // HANG_VAR_NORMAL
	            break;
	        }
		}
#endif
#ifdef CHECK_FW_ERROR
#endif

#ifdef CHECK_TX_HANGUP
		max_q_num = 12;
		if(priv->pshare->rf_ft_var.check_hang &  CHECK_TX_HANGUP) {
			if ( !check_adaptivity_hangup && check_adaptivity_test(priv) ) {
				for(q_num = 0; q_num<=max_q_num; q_num++)
					if(	priv->pshare->Q_info[q_num].pending_tick){
						priv->pshare->Q_info[q_num].pending_tick++;
						//panic_printk("%d q_num = %d,pending_tick %d\n",__LINE__,q_num,priv->pshare->Q_info[q_num].pending_tick);
					}
			} else {
				for(q_num = 0; q_num<=max_q_num; q_num++) {
					PHCI_TX_DMA_MANAGER_88XX	ptx_dma = (PHCI_TX_DMA_MANAGER_88XX)(_GET_HAL_DATA(priv)->PTxDMA88XX);
					PHCI_TX_DMA_QUEUE_STRUCT_88XX	tx_q   = &(ptx_dma->tx_queue[q_num]);

					tail = 	RTL_R32(tx_q->reg_rwptr_idx);
					if( (0xffff & ((tail>>16)^ tail))==0) {
						if(priv->pshare->Q_info[q_num].pending_tick)
							alive++;
						priv->pshare->Q_info[q_num].pending_tick = 0;
						priv->pshare->Q_info[q_num].adaptivity_cnt = 0;
					} else {
						tail = (tail>>16) & 0xffff;
						if (priv->pshare->Q_info[q_num].pending_tick &&
							(tail == priv->pshare->Q_info[q_num].pending_tail) &&
							(UINT32_DIFF(priv->up_time, priv->pshare->Q_info[q_num].pending_tick) >= PENDING_PERIOD)) {
							txhangup++;
							if ( check_adaptivity_hangup && (priv->pshare->Q_info[q_num].adaptivity_cnt >= PENDING_PERIOD) )
								adaptivity_hangup++;
						}

						if ((priv->pshare->Q_info[q_num].pending_tick == 0) ||
							(tail != priv->pshare->Q_info[q_num].pending_tail)) {
							if (tail != priv->pshare->Q_info[q_num].pending_tail)
								alive++;
							priv->pshare->Q_info[q_num].pending_tick = priv->up_time;
							priv->pshare->Q_info[q_num].pending_tail = tail;
							priv->pshare->Q_info[q_num].adaptivity_cnt = 0;
						}

						if ( check_adaptivity_hangup && check_adaptivity_test(priv) )
							priv->pshare->Q_info[q_num].adaptivity_cnt++;
					}
				}

				if(alive) {
					txhangup = adaptivity_hangup = 0;
					for(q_num = 0; q_num<=max_q_num; q_num++) {
						priv->pshare->Q_info[q_num].pending_tick = 0;
						priv->pshare->Q_info[q_num].adaptivity_cnt = 0;
					}
				}
			}
		}
#endif

    } else if(CONFIG_WLAN_NOT_HAL_EXIST)
	{
#ifdef CHECK_TX_HANGUP
	max_q_num = 5; // we check Q5, Q0, Q4, Q3, Q2, Q1
	if(priv->pshare->rf_ft_var.check_hang &  CHECK_TX_HANGUP) {
		if( !check_adaptivity_hangup && check_adaptivity_test(priv) ) {
			for(q_num = 0; q_num<=max_q_num; q_num++){
				if(	priv->pshare->Q_info[q_num].pending_tick){
					priv->pshare->Q_info[q_num].pending_tick++;
					//panic_printk("%d q_num = %d,pending_tick %d\n",__LINE__,q_num,priv->pshare->Q_info[q_num].pending_tick);
				}
			}
		} else {
			for(q_num = 0; q_num<=max_q_num; q_num++) {

				if (check_tx_hangup(priv, q_num, &tail, &is_empty)) {
					if (priv->pshare->Q_info[q_num].pending_tick &&
						(tail == priv->pshare->Q_info[q_num].pending_tail) &&
						(UINT32_DIFF(priv->up_time, priv->pshare->Q_info[q_num].pending_tick) >= PENDING_PERIOD)) {
						// the stopping is over the period => hangup!
						txhangup++;

						if ( check_adaptivity_hangup && (priv->pshare->Q_info[q_num].adaptivity_cnt >= PENDING_PERIOD) )
							adaptivity_hangup++;
		//				break;
					}

					if ((priv->pshare->Q_info[q_num].pending_tick == 0) ||
						(tail != priv->pshare->Q_info[q_num].pending_tail)) {
						if(tail != priv->pshare->Q_info[q_num].pending_tail)
							alive++;
						// the first time stopping or the tail moved
						priv->pshare->Q_info[q_num].pending_tick = priv->up_time;
						priv->pshare->Q_info[q_num].pending_tail = tail;
						priv->pshare->Q_info[q_num].adaptivity_cnt = 0;
					}

					if ( check_adaptivity_hangup && check_adaptivity_test(priv) )
						priv->pshare->Q_info[q_num].adaptivity_cnt++;
					priv->pshare->Q_info[q_num].idle_tick = 0;
		//			break;
				}
				else {
					if(priv->pshare->Q_info[q_num].pending_tick)
						alive++;
					else if (tail != priv->pshare->Q_info[q_num].pending_tail)
						alive++;
					priv->pshare->Q_info[q_num].pending_tail = tail;
					// empty or own bit is cleared
					priv->pshare->Q_info[q_num].pending_tick = 0;
					priv->pshare->Q_info[q_num].adaptivity_cnt = 0;
					if (!is_empty &&
						priv->pshare->Q_info[q_num].idle_tick &&
						(tail == priv->pshare->Q_info[q_num].pending_tail) &&
						(UINT32_DIFF(priv->up_time, priv->pshare->Q_info[q_num].idle_tick) >= PENDING_PERIOD)) {
						// own bit is cleared, but the tail didn't move and is idle over the period => call DSR
						rtl8192cd_tx_dsr((unsigned long)priv);
						priv->pshare->Q_info[q_num].idle_tick = 0;
		//				break;
					}
					else {
						if (is_empty)
							priv->pshare->Q_info[q_num].idle_tick = 0;
						else {
							if ((priv->pshare->Q_info[q_num].idle_tick == 0) ||
								(tail != priv->pshare->Q_info[q_num].pending_tail)) {
								// the first time idle, or the own bit is cleared and the tail moved
								priv->pshare->Q_info[q_num].idle_tick = priv->up_time;
								priv->pshare->Q_info[q_num].pending_tail = tail;
		//						break;
							}
						}
					}
				}

			}
			if(alive) {
				txhangup = adaptivity_hangup = 0;
				for(q_num = 0; q_num<=max_q_num; q_num++) {
					priv->pshare->Q_info[q_num].pending_tick = 0;
					priv->pshare->Q_info[q_num].adaptivity_cnt = 0;
				}
			}
		}
	}
#endif
	}

#ifdef CHECK_RX_HANGUP
	// check for rx stop
	if (txhangup == 0) {
		if ((priv->assoc_num > 0
			) && !priv->pshare->rx_hang_checking)
			check_rx_hangup(priv);

		if (IS_DRV_OPEN(GET_VXD_PRIV(priv)) &&
			GET_VXD_PRIV(priv)->assoc_num > 0 &&
			!priv->pshare->rx_hang_checking)
			check_rx_hangup(GET_VXD_PRIV(priv));
		if (GET_ROOT(priv)->pmib->miscEntry.vap_enable) {
			for (i=0; i<RTL8192CD_NUM_VWLAN; i++) {
				if (IS_DRV_OPEN(priv->pvap_priv[i]) &&
					priv->pvap_priv[i]->assoc_num > 0 &&
					!priv->pshare->rx_hang_checking)
					check_rx_hangup(priv->pvap_priv[i]);
			}
		}

		if (priv->pshare->rx_hang_checking)
		{
			if (priv->pshare->rx_cntreg_log == 0)
				priv->pshare->rx_cntreg_log = RTL_R32(_RXPKTNUM_);
			else {
				rx_cntreg = RTL_R32(_RXPKTNUM_);
				if (priv->pshare->rx_cntreg_log == rx_cntreg) {
					if (priv->pshare->rx_stop_pending_tick) {
						if (UINT32_DIFF(priv->pshare->selected_priv->up_time, priv->pshare->rx_stop_pending_tick) >= (PENDING_PERIOD-1)) {
							rxhangup++;
							//printk("\n\n%s %d rxhangup++ rx_packets_pre1=%lu; rx_packets_pre2=%lu; net_stats.rx_packets=%lu\n",
									//__FUNCTION__, __LINE__,
									//priv->pshare->selected_priv->rx_packets_pre1, priv->pshare->selected_priv->rx_packets_pre2,
									//priv->pshare->selected_priv->net_stats.rx_packets);
						}
					}
					else {
						priv->pshare->rx_stop_pending_tick = priv->pshare->selected_priv->up_time;
						RTL_W32(_RCR_, RTL_R32(_RCR_) | _ACF_);
						check_rx_hangup_send_pkt(priv->pshare->selected_priv);
						//printk("\n\ncheck_rx_hangup_send_pkt!\n");
						//printk("%s %d rx_packets_pre1=%lu; rx_packets_pre2=%lu; net_stats.rx_packets=%lu\n",
								//__FUNCTION__, __LINE__,
								//priv->pshare->selected_priv->rx_packets_pre1, priv->pshare->selected_priv->rx_packets_pre2,
								//priv->pshare->selected_priv->net_stats.rx_packets);
					}
				}
				else {
					//printk("\n\n%s %d Recovered!\n" ,__FUNCTION__, __LINE__);
					priv->pshare->rx_hang_checking = 0;
					priv->pshare->rx_cntreg_log = 0;
					priv->pshare->selected_priv = NULL;
					priv->rx_start_monitor_running = 0;
					if (IS_DRV_OPEN(GET_VXD_PRIV(priv)))
						GET_VXD_PRIV(priv)->rx_start_monitor_running = 0;
					if (GET_ROOT(priv)->pmib->miscEntry.vap_enable) {
						for (i=0; i<RTL8192CD_NUM_VWLAN; i++) {
							if (IS_DRV_OPEN(priv->pvap_priv[i]))
								priv->pvap_priv[i]->rx_start_monitor_running = 0;
						}
					}

					if (priv->pshare->rx_stop_pending_tick) {
						priv->pshare->rx_stop_pending_tick = 0;
						RTL_W32(_RCR_, RTL_R32(_RCR_) & (~_ACF_));
					}
				}
			}
		}

		if (rxhangup == 0) {
			if (priv->assoc_num > 0)
				check_rx_hangup_record_rxpkts(priv);
			else if (priv->rx_start_monitor_running) {
				priv->rx_start_monitor_running = 0;
				//printk("stop monitoring = %d\n", priv->rx_start_monitor_running);
			}
			if (IS_DRV_OPEN(GET_VXD_PRIV(priv))) {
				if (GET_VXD_PRIV(priv)->assoc_num > 0)
					check_rx_hangup_record_rxpkts(GET_VXD_PRIV(priv));
				else if (GET_VXD_PRIV(priv)->rx_start_monitor_running) {
					GET_VXD_PRIV(priv)->rx_start_monitor_running = 0;
					//printk("stop monitoring = %d\n", GET_VXD_PRIV(priv)->rx_start_monitor_running);
				}
			}
			if (GET_ROOT(priv)->pmib->miscEntry.vap_enable) {
				for (i=0; i<RTL8192CD_NUM_VWLAN; i++) {
					if (IS_DRV_OPEN(priv->pvap_priv[i])) {
						if (priv->pvap_priv[i]->assoc_num > 0)
							check_rx_hangup_record_rxpkts(priv->pvap_priv[i]);
						else if (priv->pvap_priv[i]->rx_start_monitor_running) {
							priv->pvap_priv[i]->rx_start_monitor_running = 0;
							//printk("stop monitoring = %d\n", priv->pvap_priv[i]->rx_start_monitor_running);
						}
					}
				}
			}
		}
	}
#endif // CHECK_RX_HANGUP

#ifdef CHECK_RX_DMA_ERROR
	if(priv->pshare->rf_ft_var.check_hang &  CHECK_RX_DMA_ERROR)
	if((GET_CHIP_VER(priv)==VERSION_8192C) || (GET_CHIP_VER(priv)==VERSION_8188C)
		|| (GET_CHIP_VER(priv)==VERSION_8192D)|| (GET_CHIP_VER(priv) == VERSION_8188E) || (GET_CHIP_VER(priv) == VERSION_8192E))
	{
		int rxffptr, rxffpkt;

		rxffptr = RTL_R16(RXFF_PTR+2);
		rxffpkt = RTL_R8(RXPKT_NUM+3);

		if((rxffptr%0x80) && rxffpkt && (RTL_R8(RXPKT_NUM+2)^2)){
			if( (priv->pshare->rx_byte_cnt == priv->net_stats.rx_bytes) &&
				!((rxffptr ^ priv->pshare->rxff_rdptr)|(rxffpkt ^ priv->pshare->rxff_pkt))) {
				if (priv->pshare->rx_dma_err_cnt < 15) { // continue for 15 seconds
					priv->pshare->rx_dma_err_cnt++;
				} else {
					unsigned char tmp;
					tmp = RTL_R8(0x1c);

					// Turn OFF BIT_WLOCK_ALL
					RTL_W8(0x1c, tmp & (~(BIT(0) | BIT(1))));
					// Turn ON BIT_SYSON_DIS_PMCREG_WRMSK
					RTL_W8(0xcc, RTL_R8(0xcc) | BIT(2));
					// Turn ON BIT_STOP_RXQ
					RTL_W8(0x301, RTL_R8(0x301) | BIT(0));
					// Reset flow
					RTL_W8(CR, 0);
					RTL_W8(SYS_FUNC_EN+1, RTL_R8(SYS_FUNC_EN+1) & (~BIT(0)));
					RTL_W8(SYS_FUNC_EN+1, RTL_R8(SYS_FUNC_EN+1) | BIT(0));
					// Turn OFF BIT_STOP_RXQ
					RTL_W8(0x301, RTL_R8(0x301) & (~BIT(0)));
					// Turn OFF BIT_SYSON_DIS_PMCREG_WRMSK
					RTL_W8(0xcc, RTL_R8(0xcc) & (~BIT(2)));
					// Restore BIT_WLOCK_ALL
					RTL_W8(0x1c, tmp);

					rxhangup++;
				}
			} else {
				priv->pshare->rx_dma_err_cnt = 0;
				priv->pshare->rxff_rdptr = rxffptr;
				priv->pshare->rxff_pkt = rxffpkt;
				priv->pshare->rx_byte_cnt = priv->net_stats.rx_bytes;
			}
		}
	}
#endif

#ifdef CHECK_RX_TAG_ERROR
#endif

#ifdef CHECK_BEACON_HANGUP
	if ((priv->pshare->rf_ft_var.check_hang &  CHECK_BEACON_HANGUP)
			&& ((OPMODE & WIFI_AP_STATE)
			&& !(OPMODE &WIFI_SITE_MONITOR)
			&& priv->pBeaconCapability	// beacon has init
			//&& !priv->pmib->miscEntry.func_off
		   )
#if 0//def UNIVERSAL_REPEATER
			|| ((OPMODE & WIFI_STATION_STATE) && GET_VXD_PRIV(priv) &&
						(GET_VXD_PRIV(priv)->drv_state & DRV_STATE_VXD_AP_STARTED))
#endif
		) {
		unsigned long beacon_ok;

#if 0//def UNIVERSAL_REPEATER
		if (OPMODE & WIFI_STATION_STATE){
			beacon_ok = GET_VXD_PRIV(priv)->ext_stats.beacon_ok;
/*
			BcnQ_Val = GET_VXD_PRIV(priv)->ext_stats.beaconQ_sts;
			GET_VXD_PRIV(priv)->ext_stats.beaconQ_sts = RTL_R32(0x120);
			if(BcnQ_Val == GET_VXD_PRIV(priv)->ext_stats.beaconQ_sts)
				beacon_hangup = 1;
*/
		}
		else
#endif
		if(!priv->pmib->miscEntry.func_off)
		{
			beacon_ok = priv->ext_stats.beacon_ok + priv->ext_stats.beacon_er;
/*
			BcnQ_Val = priv->ext_stats.beaconQ_sts;
			priv->ext_stats.beaconQ_sts = RTL_R32(0x120);// firmware beacon Q stats
			if(BcnQ_Val == priv->ext_stats.beaconQ_sts)
				beacon_hangup = 1;
*/
		}
		else if (priv->pmib->miscEntry.vap_enable) {
			for (i=0; i<RTL8192CD_NUM_VWLAN; i++) {
				if (IS_DRV_OPEN(priv->pvap_priv[i])) {
					if ((priv->pvap_priv[i])->pmib->miscEntry.func_off == 0) {
						beacon_ok = (priv->pvap_priv[i])->ext_stats.beacon_ok + (priv->pvap_priv[i])->ext_stats.beacon_er;
						break;
					}
				}
			}
		}
		if((!priv->pmib->miscEntry.func_off)
			|| ((i!=RTL8192CD_NUM_VWLAN) && priv->pmib->miscEntry.vap_enable)
		)
		{
			if (priv->pshare->beacon_wait_cnt == 0) {
				if (priv->pshare->beacon_ok_cnt == beacon_ok) {
					int threshold=5;

					#if 0
					if (priv->pmib->miscEntry.vap_enable)
						threshold=3;
					if (priv->pmib->dot11StationConfigEntry.dot11BeaconPeriod < 650)
						threshold = 0;
					#endif
					if (priv->pshare->beacon_pending_cnt++ >= threshold)
						beacon_hangup = 1;
				}
				else {
					priv->pshare->beacon_ok_cnt =beacon_ok;
					if (priv->pshare->beacon_pending_cnt > 0)
						priv->pshare->beacon_pending_cnt = 0;
				}
			}
			else
				priv->pshare->beacon_wait_cnt--;
		}
	}
#endif

#ifdef CHECK_AFTER_RESET
	if (priv->pshare->reset_monitor_cnt_down > 0) {
		priv->pshare->reset_monitor_cnt_down--;
		if (priv->pshare->reset_monitor_rx_pkt_cnt == priv->net_stats.rx_packets)	{
//			if (priv->pshare->reset_monitor_pending++ > 1)
				reset_fail_hangup = 1;
		}
		else {
			priv->pshare->reset_monitor_rx_pkt_cnt = priv->net_stats.rx_packets;
			if (priv->pshare->reset_monitor_pending > 0)
				priv->pshare->reset_monitor_pending = 0;
		}
	}
#endif

	if ( adaptivity_hangup )
	{
		for(q_num = 0; q_num<=max_q_num; q_num++) {
			priv->pshare->Q_info[q_num].pending_tick = 0;
			priv->pshare->Q_info[q_num].adaptivity_cnt = 0;
		}

		priv->check_cnt_adaptivity++;
		priv->pshare->reg_tapause_bak = RTL_R8(TXPAUSE);

		RTL_W8(TXPAUSE, 0xff);				//it will be recovery after ACS done
		priv->pshare->acs_for_adaptivity_flag = TRUE;

		DEBUG_WARN("Adaptivity hangup occur, change to another channel!!\n");
		rtl8192cd_autochannel_sel(priv);
		return 0;
	}
	else if (txhangup || rxhangup || beacon_hangup || reset_fail_hangup || fw_error) { // hangup happen
		priv->reset_hangup = 1;
		if (IS_DRV_OPEN(GET_VXD_PRIV(priv)))
			GET_VXD_PRIV(priv)->reset_hangup = 1;
		if (GET_ROOT(priv)->pmib->miscEntry.vap_enable) {
			for (i=0; i<RTL8192CD_NUM_VWLAN; i++) {
				if (IS_DRV_OPEN(priv->pvap_priv[i]))
					priv->pvap_priv[i]->reset_hangup = 1;
			}
		}

#ifdef CHECK_HANG_DEBUG
		if (txhangup) {
			if(priv->pshare->rf_ft_var.swq_dbg==0)
				GDEBUG("uptime=%ld,txhang\n", priv->up_time);
			priv->check_cnt_tx++;
		}
		else if (rxhangup) {
			if(priv->pshare->rf_ft_var.swq_dbg==0)
				GDEBUG("uptime=%ld,rxhang\n", priv->up_time);
			priv->check_cnt_rx++;
		}
		else if (beacon_hangup) {
			if(priv->pshare->rf_ft_var.swq_dbg==0)
				GDEBUG("uptime=%ld,beacon_hangup\n", priv->up_time);
			priv->check_cnt_bcn++;
		}
		else if (reset_fail_hangup) {
			if(priv->pshare->rf_ft_var.swq_dbg==0)
				GDEBUG("uptime=%ld,reset_fail_hangup\n", priv->up_time);
			priv->check_cnt_rst++;
		}
		else if (fw_error) {
			priv->check_cnt_fw++;
		}
		return 0;
#else
		if (txhangup)
			priv->check_cnt_tx++;
		else if (rxhangup)
			priv->check_cnt_rx++;
		else if (beacon_hangup)
			priv->check_cnt_bcn++;
		else if (reset_fail_hangup)
			priv->check_cnt_rst++;
		else if (fw_error)
			priv->check_cnt_fw++;
#endif

// for debug
#if 0
		if (txhangup)
			printk("WA: Tx reset, up-time=%lu sec\n", priv->up_time);
		else if (rxhangup)
			printk("WA: Rx reset, up-time=%lu sec\n", priv->up_time);
		else if (beacon_hangup)
			printk("WA: Beacon reset, up-time=%lu sec\n", priv->up_time);
		else if (reset_fail_hangup)
			printk("WA: Reset-fail reset, up-time=%lu sec\n", priv->up_time);
		else if (fw_error)
			printk("WA: FW reset, up-time=%lu sec\n", priv->up_time);
#endif

// Set flag to re-init WDS key in rtl8192cd_open()
//----------------------------- david+2006-06-30

		PRINT_INFO("Status check! Tx[%d] Rx[%d] Bcnt[%d] Rst[%d] ...\n",
			priv->check_cnt_tx, priv->check_cnt_rx, priv->check_cnt_bcn, priv->check_cnt_rst);

		watchdog_kick();


		SAVE_INT_AND_CLI(flags);
        SMP_LOCK(flags);

#ifdef FAST_RECOVERY
		info = backup_sta(priv);
		if (GET_ROOT(priv)->pmib->miscEntry.vap_enable) {
			for (i=0; i<RTL8192CD_NUM_VWLAN; i++) {
				if (IS_DRV_OPEN(priv->pvap_priv[i]))
					vap_info[i] = backup_sta(priv->pvap_priv[i]);
				else
					vap_info[i] = NULL;
			}
		}
		if (IS_DRV_OPEN(GET_VXD_PRIV(priv)))
			vxd_info = backup_sta(GET_VXD_PRIV(priv));
#endif // FAST_RECOVERY

		priv->pmib->dot11OperationEntry.keep_rsnie = 1;
		if (GET_ROOT(priv)->pmib->miscEntry.vap_enable) {
			for (i=0; i<RTL8192CD_NUM_VWLAN; i++) {
				if (IS_DRV_OPEN(priv->pvap_priv[i]))
					priv->pvap_priv[i]->pmib->dot11OperationEntry.keep_rsnie = 1;
			}
		}
		if (IS_DRV_OPEN(GET_VXD_PRIV(priv)))
			GET_VXD_PRIV(priv)->pmib->dot11OperationEntry.keep_rsnie = 1;

		SMP_UNLOCK(flags);
		rtl8192cd_close(priv->dev);



		if(IS_HAL_CHIP(priv) && (pcie_tx_stuck || pcie_rx_stuck)) {
			panic_printk("PCIe eMAC tx stuck:%d, rx stuck:%d\n",pcie_tx_stuck,pcie_rx_stuck);
			GET_HAL_INTERFACE(priv)->ResetHWForSurpriseHandler(priv);
			pcie_tx_stuck = pcie_rx_stuck = 0;
		}
		rtl8192cd_open(priv->dev);
		SMP_LOCK(flags);

#ifdef FAST_RECOVERY
		if (info)
			restore_backup_sta(priv, info);

		if (GET_ROOT(priv)->pmib->miscEntry.vap_enable) {
			for (i=0; i<RTL8192CD_NUM_VWLAN; i++) {
				if (IS_DRV_OPEN(priv->pvap_priv[i]) && vap_info[i])
					restore_backup_sta(priv->pvap_priv[i], vap_info[i]);
			}
		}
		if (IS_DRV_OPEN(GET_VXD_PRIV(priv)) && vxd_info)
			restore_backup_sta(GET_VXD_PRIV(priv), vxd_info);
#endif // FAST_RECOVERY

#ifdef CHECK_AFTER_RESET
		priv->pshare->reset_monitor_cnt_down = 3;
		priv->pshare->reset_monitor_pending = 0;
		priv->pshare->reset_monitor_rx_pkt_cnt = priv->net_stats.rx_packets;
#endif

		RESTORE_INT(flags);
		SMP_UNLOCK(flags);

		watchdog_kick();


		return 1;
	}
	else
		return 0;
}
#endif // CHECK_HANGUP


// quick fix of tx stuck especial in client mode
void tx_stuck_fix(struct rtl8192cd_priv *priv)
{
	struct rtl8192cd_hw *phw = GET_HW(priv);
	unsigned int val32;
	unsigned long flags;

	if (IS_HAL_CHIP(priv))
		return;
	else if(CONFIG_WLAN_NOT_HAL_EXIST)
	{
	SAVE_INT_AND_CLI(flags);
//	RTL_W32(0x350, RTL_R32(0x350) | BIT(26));
	val32 = RTL_R32(0x350);
	if (val32 & BIT(24)) {	// tx stuck
			RTL_W8(0x301, RTL_R8(0x301) | BIT(0));
			delay_us(100);
			rtl8192cd_rx_isr(priv);
			RTL_W8(0x302, RTL_R8(0x302) | BIT(4));
			RTL_W8(0x3, RTL_R8(0x3) & (~BIT(0)));
			RTL_W8(0x3, RTL_R8(0x3) | BIT(0));
#ifdef DELAY_REFILL_RX_BUF
			{
				struct sk_buff	*pskb;

				while (phw->cur_rx_refill != phw->cur_rx) {
					pskb = rtl_dev_alloc_skb(priv, RX_BUF_LEN, _SKB_RX_, 1);
					if (pskb == NULL) {
						printk("[%s] can't allocate skbuff for RX!\n", __FUNCTION__);
					}
					init_rxdesc(pskb, phw->cur_rx_refill, priv);

					phw->cur_rx_refill = (phw->cur_rx_refill + 1) % NUM_RX_DESC_IF(priv);
				}
			}
			phw->cur_rx_refill = 0;
#endif
			phw->cur_rx = 0;
			RTL_W32(RX_DESA, (unsigned int)phw->ring_dma_addr);
			RTL_W8(0x301, RTL_R8(0x301) & (~BIT(0)));
			RTL_W8(PCIE_CTRL_REG, MGQ_POLL | BEQ_POLL);
	}
	RESTORE_INT(flags);
}
}

static struct ac_log_info *aclog_lookfor_entry(struct rtl8192cd_priv *priv, unsigned char *addr)
{
	int i, idx=-1;

	for (i=0; i<MAX_AC_LOG; i++) {
		if (priv->acLog[i].used == 0) {
			if (idx < 0)
				idx = i;
			continue;
		}
		if (isEqualMACAddr(priv->acLog[i].addr, addr))
			break;
	}

	if ( i != MAX_AC_LOG)
		return (&priv->acLog[i]);

	if (idx >= 0)
		return (&priv->acLog[idx]);

	return NULL; // table full
}


static void aclog_update_entry(struct ac_log_info *entry, unsigned char *addr)
{
	if (entry->used == 0) {
		memcpy(entry->addr, addr, MACADDRLEN);
		entry->used = 1;
	}
	entry->cur_cnt++;
	entry->last_attack_time = jiffies;
}


static int aclog_check(struct rtl8192cd_priv *priv)
{
	int i, used=0;

	for (i=0; i<MAX_AC_LOG; i++) {
		if (priv->acLog[i].used) {
			used++;
			if (priv->acLog[i].cur_cnt != priv->acLog[i].last_cnt) {
#if   defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD) || defined(CONFIG_RTL8196C_KLD)
				LOG_MSG_DROP("Unauthorized wireless PC try to connect;note:%02X:%02X:%02X:%02X:%02X:%02X;\n",
					priv->acLog[i].addr[0], priv->acLog[i].addr[1], priv->acLog[i].addr[2],
					priv->acLog[i].addr[3], priv->acLog[i].addr[4], priv->acLog[i].addr[5]);
#else
				LOG_MSG("A wireless client (%02X:%02X:%02X:%02X:%02X:%02X) was rejected due to access control for %d times in 5 minutes\n",
					priv->acLog[i].addr[0], priv->acLog[i].addr[1], priv->acLog[i].addr[2],
					priv->acLog[i].addr[3], priv->acLog[i].addr[4], priv->acLog[i].addr[5],
					priv->acLog[i].cur_cnt - priv->acLog[i].last_cnt);
#endif
				priv->acLog[i].last_cnt = priv->acLog[i].cur_cnt;
			}
			else { // no update, check expired entry
				if ((jiffies - priv->acLog[i].last_attack_time) > AC_LOG_EXPIRE) {
					memset(&priv->acLog[i], '\0', sizeof(struct ac_log_info));
					used--;
				}
			}
		}
	}

	return used;
}


#ifdef WIFI_WMM
static void get_AP_Qos_Info(struct rtl8192cd_priv *priv, unsigned char *temp)
{
	temp[0] = GET_EDCA_PARA_UPDATE;
	temp[0] &= 0x0f;
	if (APSD_ENABLE)
		temp[0] |= BIT(7);
}


static void get_STA_AC_Para_Record(struct rtl8192cd_priv *priv, unsigned char *temp)
{
//BE
	temp[0] = GET_STA_AC_BE_PARA.AIFSN;
	temp[0] &= 0x0f;
	if (GET_STA_AC_BE_PARA.ACM)
		temp[0] |= BIT(4);
	temp[1] = GET_STA_AC_BE_PARA.ECWmax;
	temp[1] <<= 4;
	temp[1] |= GET_STA_AC_BE_PARA.ECWmin;
	temp[2] = GET_STA_AC_BE_PARA.TXOPlimit % 256;
	temp[3] = GET_STA_AC_BE_PARA.TXOPlimit / 256; // 2^8 = 256, for one byte's range

//BK
	temp[4] = GET_STA_AC_BK_PARA.AIFSN;
	temp[4] &= 0x0f;
	if (GET_STA_AC_BK_PARA.ACM)
		temp[4] |= BIT(4);
	temp[4] |= BIT(5);
	temp[5] = GET_STA_AC_BK_PARA.ECWmax;
	temp[5] <<= 4;
	temp[5] |= GET_STA_AC_BK_PARA.ECWmin;
	temp[6] = GET_STA_AC_BK_PARA.TXOPlimit % 256;
	temp[7] = GET_STA_AC_BK_PARA.TXOPlimit / 256;

//VI
	temp[8] = GET_STA_AC_VI_PARA.AIFSN;
	temp[8] &= 0x0f;
	if (GET_STA_AC_VI_PARA.ACM)
		temp[8] |= BIT(4);
	temp[8] |= BIT(6);
	temp[9] = GET_STA_AC_VI_PARA.ECWmax;
	temp[9] <<= 4;
	temp[9] |= GET_STA_AC_VI_PARA.ECWmin;
	temp[10] = GET_STA_AC_VI_PARA.TXOPlimit % 256;
	temp[11] = GET_STA_AC_VI_PARA.TXOPlimit / 256;

//VO
	temp[12] = GET_STA_AC_VO_PARA.AIFSN;
	temp[12] &= 0x0f;
	if (GET_STA_AC_VO_PARA.ACM)
		temp[12] |= BIT(4);
	temp[12] |= BIT(5)|BIT(6);
	temp[13] = GET_STA_AC_VO_PARA.ECWmax;
	temp[13] <<= 4;
	temp[13] |= GET_STA_AC_VO_PARA.ECWmin;
	temp[14] = GET_STA_AC_VO_PARA.TXOPlimit % 256;
	temp[15] = GET_STA_AC_VO_PARA.TXOPlimit /256;
}


void init_WMM_Para_Element(struct rtl8192cd_priv *priv, unsigned char *temp)
{
	if (OPMODE & WIFI_AP_STATE) {
		memcpy(temp, WMM_PARA_IE, 6);
//Qos Info field
		get_AP_Qos_Info(priv, &temp[6]);
//AC Parameters
		get_STA_AC_Para_Record(priv, &temp[8]);
 	}
#ifdef CLIENT_MODE
	else if ((OPMODE & WIFI_STATION_STATE) ||(OPMODE & WIFI_ADHOC_STATE)) {  //  WMM STA
		memcpy(temp, WMM_IE, 6);
		temp[6] = 0x00;  //  set zero to WMM STA Qos Info field
#ifdef WMM_APSD
		if ((OPMODE & WIFI_STATION_STATE) && APSD_ENABLE && priv->uapsd_assoc) {
			if (priv->pmib->dot11QosEntry.UAPSD_AC_BE)
				temp[6] |= BIT(3);
			if (priv->pmib->dot11QosEntry.UAPSD_AC_BK)
				temp[6] |= BIT(2);
			if (priv->pmib->dot11QosEntry.UAPSD_AC_VI)
				temp[6] |= BIT(1);
			if (priv->pmib->dot11QosEntry.UAPSD_AC_VO)
				temp[6] |= BIT(0);
		}
#endif
	}
#endif
}

void default_WMM_para(struct rtl8192cd_priv *priv)
{
#ifdef RTL_MANUAL_EDCA
	if( priv->pmib->dot11QosEntry.ManualEDCA ) {
		GET_STA_AC_BE_PARA.ACM = priv->pmib->dot11QosEntry.STA_manualEDCA[BE].ACM;
		GET_STA_AC_BE_PARA.AIFSN = priv->pmib->dot11QosEntry.STA_manualEDCA[BE].AIFSN;
		GET_STA_AC_BE_PARA.ECWmin = priv->pmib->dot11QosEntry.STA_manualEDCA[BE].ECWmin;
		GET_STA_AC_BE_PARA.ECWmax = priv->pmib->dot11QosEntry.STA_manualEDCA[BE].ECWmax;
		GET_STA_AC_BE_PARA.TXOPlimit = priv->pmib->dot11QosEntry.STA_manualEDCA[BE].TXOPlimit;

		GET_STA_AC_BK_PARA.ACM = priv->pmib->dot11QosEntry.STA_manualEDCA[BK].ACM;
		GET_STA_AC_BK_PARA.AIFSN = priv->pmib->dot11QosEntry.STA_manualEDCA[BK].AIFSN;
		GET_STA_AC_BK_PARA.ECWmin = priv->pmib->dot11QosEntry.STA_manualEDCA[BK].ECWmin;
		GET_STA_AC_BK_PARA.ECWmax = priv->pmib->dot11QosEntry.STA_manualEDCA[BK].ECWmax;
		GET_STA_AC_BK_PARA.TXOPlimit = priv->pmib->dot11QosEntry.STA_manualEDCA[BK].TXOPlimit;

		GET_STA_AC_VI_PARA.ACM = priv->pmib->dot11QosEntry.STA_manualEDCA[VI].ACM;
		GET_STA_AC_VI_PARA.AIFSN = priv->pmib->dot11QosEntry.STA_manualEDCA[VI].AIFSN;
		GET_STA_AC_VI_PARA.ECWmin = priv->pmib->dot11QosEntry.STA_manualEDCA[VI].ECWmin;
		GET_STA_AC_VI_PARA.ECWmax = priv->pmib->dot11QosEntry.STA_manualEDCA[VI].ECWmax;
		GET_STA_AC_VI_PARA.TXOPlimit = priv->pmib->dot11QosEntry.STA_manualEDCA[VI].TXOPlimit; // 6.016ms

		GET_STA_AC_VO_PARA.ACM = priv->pmib->dot11QosEntry.STA_manualEDCA[VO].ACM;
		GET_STA_AC_VO_PARA.AIFSN = priv->pmib->dot11QosEntry.STA_manualEDCA[VO].AIFSN;
		GET_STA_AC_VO_PARA.ECWmin = priv->pmib->dot11QosEntry.STA_manualEDCA[VO].ECWmin;
		GET_STA_AC_VO_PARA.ECWmax = priv->pmib->dot11QosEntry.STA_manualEDCA[VO].ECWmax;
		GET_STA_AC_VO_PARA.TXOPlimit = priv->pmib->dot11QosEntry.STA_manualEDCA[VO].TXOPlimit; // 3.264ms
		} else
#endif
	{
		GET_STA_AC_BE_PARA.ACM = rtl_sta_EDCA[BE].ACM;
		GET_STA_AC_BE_PARA.AIFSN = rtl_sta_EDCA[BE].AIFSN;
#ifdef MULTI_STA_REFINE
		if(priv->pshare->rf_ft_var.msta_refine&4 ) {
			GET_STA_AC_BE_PARA.ECWmin = 8;
		}
		else
#endif
		{
			GET_STA_AC_BE_PARA.ECWmin = rtl_sta_EDCA[BE].ECWmin;
		}
		GET_STA_AC_BE_PARA.ECWmax = rtl_sta_EDCA[BE].ECWmax;
		GET_STA_AC_BE_PARA.TXOPlimit = rtl_sta_EDCA[BE].TXOPlimit;

		GET_STA_AC_BK_PARA.ACM = rtl_sta_EDCA[BK].ACM;
		GET_STA_AC_BK_PARA.AIFSN = rtl_sta_EDCA[BK].AIFSN;
		GET_STA_AC_BK_PARA.ECWmin = rtl_sta_EDCA[BK].ECWmin;
		GET_STA_AC_BK_PARA.ECWmax = rtl_sta_EDCA[BK].ECWmax;
		GET_STA_AC_BK_PARA.TXOPlimit = rtl_sta_EDCA[BK].TXOPlimit;

		GET_STA_AC_VI_PARA.ACM = rtl_sta_EDCA[VI].ACM;
		GET_STA_AC_VI_PARA.AIFSN = rtl_sta_EDCA[VI].AIFSN;
		GET_STA_AC_VI_PARA.ECWmin = rtl_sta_EDCA[VI].ECWmin;
		GET_STA_AC_VI_PARA.ECWmax = rtl_sta_EDCA[VI].ECWmax;
		if (priv->pmib->dot11BssType.net_work_type & (WIRELESS_11G|WIRELESS_11A))
			GET_STA_AC_VI_PARA.TXOPlimit = 94; // 3.008ms							GET_STA_AC_VI_PARA.TXOPlimit = rtl_sta_EDCA[VI_AG].TXOPlimit; // 3.008ms
		else
			GET_STA_AC_VI_PARA.TXOPlimit = 188; // 6.016ms								GET_STA_AC_VI_PARA.TXOPlimit = rtl_sta_EDCA[VI].TXOPlimit; // 6.016ms

		GET_STA_AC_VO_PARA.ACM = rtl_sta_EDCA[VO].ACM;
		GET_STA_AC_VO_PARA.AIFSN = rtl_sta_EDCA[VO].AIFSN;
		GET_STA_AC_VO_PARA.ECWmin = rtl_sta_EDCA[VO].ECWmin;
		GET_STA_AC_VO_PARA.ECWmax = rtl_sta_EDCA[VO].ECWmax;
		if (priv->pmib->dot11BssType.net_work_type & (WIRELESS_11G|WIRELESS_11A))
			GET_STA_AC_VO_PARA.TXOPlimit = 47; // 1.504ms							GET_STA_AC_VO_PARA.TXOPlimit = rtl_sta_EDCA[VO_AG].TXOPlimit; // 1.504ms
		else
			GET_STA_AC_VO_PARA.TXOPlimit = 102; // 3.264ms								GET_STA_AC_VO_PARA.TXOPlimit = rtl_sta_EDCA[VO].TXOPlimit; // 3.264ms
	}
}



#ifdef CLIENT_MODE
#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
__NOMIPS16
#endif
static void process_WMM_para_ie(struct rtl8192cd_priv *priv, unsigned char *p)
{
	int ACI = (p[0] >> 5) & 0x03;
	/*avoid unaligned load*/
	unsigned short txoplimit;
	memcpy(&txoplimit,&p[2],sizeof(unsigned short));
	if ((ACI >= 0) && (ACI <= 3)) {
		switch(ACI) {
			case 0:
				GET_STA_AC_BE_PARA.ACM = (p[0] >> 4) & 0x01;
				GET_STA_AC_BE_PARA.AIFSN = p[0] & 0x0f;
				GET_STA_AC_BE_PARA.ECWmin = p[1] & 0x0f;
				GET_STA_AC_BE_PARA.ECWmax = p[1] >> 4;
				GET_STA_AC_BE_PARA.TXOPlimit = le16_to_cpu(txoplimit);
				break;
			case 3:
				GET_STA_AC_VO_PARA.ACM = (p[0] >> 4) & 0x01;
				GET_STA_AC_VO_PARA.AIFSN = p[0] & 0x0f;
				GET_STA_AC_VO_PARA.ECWmin = p[1] & 0x0f;
				GET_STA_AC_VO_PARA.ECWmax = p[1] >> 4;
				GET_STA_AC_VO_PARA.TXOPlimit = le16_to_cpu(txoplimit);
				break;
			case 2:
				GET_STA_AC_VI_PARA.ACM = (p[0] >> 4) & 0x01;
				GET_STA_AC_VI_PARA.AIFSN = p[0] & 0x0f;
				GET_STA_AC_VI_PARA.ECWmin = p[1] & 0x0f;
				GET_STA_AC_VI_PARA.ECWmax = p[1] >> 4;
				GET_STA_AC_VI_PARA.TXOPlimit = le16_to_cpu(txoplimit);
				break;
			default:
				GET_STA_AC_BK_PARA.ACM = (p[0] >> 4) & 0x01;
				GET_STA_AC_BK_PARA.AIFSN = p[0] & 0x0f;
				GET_STA_AC_BK_PARA.ECWmin = p[1] & 0x0f;
				GET_STA_AC_BK_PARA.ECWmax = p[1] >> 4;
				GET_STA_AC_BK_PARA.TXOPlimit = le16_to_cpu(txoplimit);
				break;
		}
	}
	else
		printk("WMM AP EDCA Parameter IE error!\n");
}


static void sta_config_EDCA_para(struct rtl8192cd_priv *priv)
{
	unsigned int slot_time = 20, ifs_time = 10;
	unsigned int vo_edca = 0, vi_edca = 0, be_edca = 0, bk_edca = 0;

	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N ) ||
		(priv->pmib->dot11BssType.net_work_type & WIRELESS_11G))
		slot_time = 9;

	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)
		ifs_time = 16;
#if 1
	if (GET_STA_AC_VO_PARA.AIFSN) {
		vo_edca = (((unsigned short)(GET_STA_AC_VO_PARA.TXOPlimit)) << 16)
			| (((unsigned char)(GET_STA_AC_VO_PARA.ECWmax)) << 12)
			| (((unsigned char)(GET_STA_AC_VO_PARA.ECWmin)) << 8)
			| (ifs_time + GET_STA_AC_VO_PARA.AIFSN * slot_time);

		RTL_W32(EDCA_VO_PARA, vo_edca);
	}

	if (GET_STA_AC_VI_PARA.AIFSN) {
		vi_edca = (((unsigned short)(GET_STA_AC_VI_PARA.TXOPlimit)) << 16)
			| (((unsigned char)(GET_STA_AC_VI_PARA.ECWmax)) << 12)
			| (((unsigned char)(GET_STA_AC_VI_PARA.ECWmin)) << 8)
			| (ifs_time + GET_STA_AC_VI_PARA.AIFSN * slot_time);

		/* WiFi Client mode WMM test IOT refine */
		if (priv->pmib->dot11OperationEntry.wifi_specific && (GET_STA_AC_VI_PARA.AIFSN == 2))
			vi_edca = (vi_edca & ~0xff) | (ifs_time + (GET_STA_AC_VI_PARA.AIFSN + 1) * slot_time);

		RTL_W32(EDCA_VI_PARA, vi_edca);
	}

	if (GET_STA_AC_BE_PARA.AIFSN) {
		be_edca = (((unsigned short)(GET_STA_AC_BE_PARA.TXOPlimit)) << 16)
			| (((unsigned char)(GET_STA_AC_BE_PARA.ECWmax)) << 12)
			| (((unsigned char)(GET_STA_AC_BE_PARA.ECWmin)) << 8)
			| (ifs_time + GET_STA_AC_BE_PARA.AIFSN * slot_time);

		RTL_W32(EDCA_BE_PARA, be_edca);
	}

	if (GET_STA_AC_BK_PARA.AIFSN) {
		bk_edca = (((unsigned short)(GET_STA_AC_BK_PARA.TXOPlimit)) << 16)
			| (((unsigned char)(GET_STA_AC_BK_PARA.ECWmax)) << 12)
			| (((unsigned char)(GET_STA_AC_BK_PARA.ECWmin)) << 8)
			| (ifs_time + GET_STA_AC_BK_PARA.AIFSN * slot_time);

		RTL_W32(EDCA_BK_PARA, bk_edca);
	}
#else
	if(GET_STA_AC_VO_PARA.AIFSN > 0) {
		RTL_W32(EDCA_VO_PARA, (((unsigned short)(GET_STA_AC_VO_PARA.TXOPlimit)) << 16)
			| (((unsigned char)(GET_STA_AC_VO_PARA.ECWmax)) << 12)
			| (((unsigned char)(GET_STA_AC_VO_PARA.ECWmin)) << 8)
			| (ifs_time + GET_STA_AC_VO_PARA.AIFSN * slot_time));
		if(GET_STA_AC_VO_PARA.ACM > 0)
			RTL_W8(ACMHWCTRL, RTL_R8(ACMHWCTRL)|BIT(3));
	}

	if(GET_STA_AC_VI_PARA.AIFSN > 0) {
		RTL_W32(EDCA_VI_PARA, (((unsigned short)(GET_STA_AC_VI_PARA.TXOPlimit)) << 16)
			| (((unsigned char)(GET_STA_AC_VI_PARA.ECWmax)) << 12)
			| (((unsigned char)(GET_STA_AC_VI_PARA.ECWmin)) << 8)
			| (ifs_time + GET_STA_AC_VI_PARA.AIFSN * slot_time));
		if(GET_STA_AC_VI_PARA.ACM > 0)
			RTL_W8(ACMHWCTRL, RTL_R8(ACMHWCTRL)|BIT(2));
	}

	if(GET_STA_AC_BE_PARA.AIFSN > 0) {
		RTL_W32(EDCA_BE_PARA, (((unsigned short)(GET_STA_AC_BE_PARA.TXOPlimit)) << 16)
			| (((unsigned char)(GET_STA_AC_BE_PARA.ECWmax)) << 12)
			| (((unsigned char)(GET_STA_AC_BE_PARA.ECWmin)) << 8)
			| (ifs_time + GET_STA_AC_BE_PARA.AIFSN * slot_time));
		if(GET_STA_AC_BE_PARA.ACM > 0)
			RTL_W8(ACMHWCTRL, RTL_R8(ACMHWCTRL)|BIT(1));
	}

	if(GET_STA_AC_BK_PARA.AIFSN > 0) {
		RTL_W32(EDCA_BK_PARA, (((unsigned short)(GET_STA_AC_BK_PARA.TXOPlimit)) << 16)
			| (((unsigned char)(GET_STA_AC_BK_PARA.ECWmax)) << 12)
			| (((unsigned char)(GET_STA_AC_BK_PARA.ECWmin)) << 8)
			| (ifs_time + GET_STA_AC_BK_PARA.AIFSN * slot_time));
	}

	if ((GET_STA_AC_VO_PARA.ACM > 0) || (GET_STA_AC_VI_PARA.ACM > 0) || (GET_STA_AC_BE_PARA.ACM > 0))
		RTL_W8(ACMHWCTRL, RTL_R8(ACMHWCTRL)|BIT(0));
#endif

	priv->pmib->dot11QosEntry.EDCA_STA_config = 1;
	priv->pshare->iot_mode_enable = 0;
	if (priv->pshare->rf_ft_var.wifi_beq_iot)
		priv->pshare->iot_mode_VI_exist = 0;
	priv->pshare->iot_mode_VO_exist = 0;
#ifdef WMM_VIBE_PRI
	priv->pshare->iot_mode_BE_exist = 0;
#endif
#ifdef WMM_BEBK_PRI
	priv->pshare->iot_mode_BK_exist = 0;
#endif
#ifdef LOW_TP_TXOP
	priv->pshare->BE_cwmax_enhance = 0;
#endif
}


static void reset_EDCA_para(struct rtl8192cd_priv *priv)
{
	memset((void *)&GET_STA_AC_VO_PARA, 0, sizeof(struct ParaRecord));
	memset((void *)&GET_STA_AC_VI_PARA, 0, sizeof(struct ParaRecord));
	memset((void *)&GET_STA_AC_BE_PARA, 0, sizeof(struct ParaRecord));
	memset((void *)&GET_STA_AC_BK_PARA, 0, sizeof(struct ParaRecord));

#ifdef _OUTSRC_COEXIST
	if(IS_OUTSRC_CHIP(priv))
#endif
	EdcaParaInit(priv);

#if !defined(USE_OUT_SRC) || defined(_OUTSRC_COEXIST)
#ifdef _OUTSRC_COEXIST
	if(!IS_OUTSRC_CHIP(priv))
#endif
	init_EDCA_para(priv, priv->pmib->dot11BssType.net_work_type);
#endif

	priv->pmib->dot11QosEntry.EDCA_STA_config = 0;
}
#endif // CLIENT_MODE
#endif // WIFI_WMM


// Realtek proprietary IE
static void process_rtk_ie(struct rtl8192cd_priv *priv)
{
	struct stat_info *pstat;
	int use_long_slottime=0;
	unsigned int threshold;

	if ((priv->up_time % 3) != 0)
		return;

	if ((get_rf_mimo_mode(priv) == MIMO_1T2R) || (get_rf_mimo_mode(priv) == MIMO_1T1R))
		threshold = 50*1024*1024/8;
	else
		threshold = 100*1024*1024/8;

	if (OPMODE & WIFI_AP_STATE)
	{
		struct list_head *phead = &priv->asoc_list;
		struct list_head *plist;

		SMP_LOCK_ASOC_LIST(flags);

		plist = phead->next;
		while(plist != phead)
		{
			pstat = list_entry(plist, struct stat_info, asoc_list);
			plist = plist->next;

			if ((pstat->expire_to > 0) &&
				(/*priv->pshare->is_giga_exist ||*/ !pstat->is_2t_mimo_sta) &&
				((pstat->is_realtek_sta && (pstat->IOTPeer!= HT_IOT_PEER_RTK_APCLIENT) && ((pstat->tx_avarage + pstat->rx_avarage) > threshold))
				  )) {
				use_long_slottime = 1;
				break;
			}
		}

		SMP_UNLOCK_ASOC_LIST(flags);

		if (priv->pshare->use_long_slottime == 0) {
			if (use_long_slottime) {
				priv->pshare->use_long_slottime = 1;
				set_slot_time(priv, 0);
				priv->pmib->dot11ErpInfo.shortSlot = 0;
				RESET_SHORTSLOT_IN_BEACON_CAP;
				priv->pshare->rtk_ie_buf[5] |= RTK_CAP_IE_USE_LONG_SLOT;
			}
		}
		else {
			if (use_long_slottime == 0) {
				priv->pshare->use_long_slottime = 0;
				check_protection_shortslot(priv);
				priv->pshare->rtk_ie_buf[5] &= (~RTK_CAP_IE_USE_LONG_SLOT);
			}
		}
	}
}

#ifdef RADIUS_ACCOUNTING
void indicate_sta_leaving(struct rtl8192cd_priv *priv,struct stat_info *pstat, unsigned long reason)
{
	DOT11_DISASSOCIATION_IND Disassociation_Ind;

	memcpy((void *)Disassociation_Ind.MACAddr, (void *)(pstat->hwaddr), MACADDRLEN);
	Disassociation_Ind.EventId = DOT11_EVENT_DISASSOCIATION_IND;
	Disassociation_Ind.IsMoreEvent = 0;
	Disassociation_Ind.Reason = reason;
	Disassociation_Ind.tx_packets = pstat->tx_pkts;
	Disassociation_Ind.rx_packets = pstat->rx_pkts;
	Disassociation_Ind.tx_bytes   = pstat->tx_bytes;
	Disassociation_Ind.rx_bytes   = pstat->rx_bytes;
	DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&Disassociation_Ind,
				sizeof(DOT11_DISASSOCIATION_IND));
	psk_indicate_evt(priv, DOT11_EVENT_DISASSOCIATION_IND, pstat->hwaddr, NULL, 0);
}

int cal_statistics_acct(struct rtl8192cd_priv *priv)
{
	unsigned long ret=0;
	struct list_head *phead=NULL, *plist=NULL;
	struct stat_info *pstat=NULL;

	phead = &priv->asoc_list;
	if( list_empty(phead) )
		goto acct_cal_out;

	SMP_LOCK_ASOC_LIST(flags);

	plist = phead->next;
	while (plist != phead) {
		pstat = list_entry(plist, struct stat_info, asoc_list);
		plist = plist->next;

		if( pstat->link_time%ACCT_TP_INT == 0 ){
			pstat->rx_bytes_1m = pstat->rx_bytes - pstat->rx_bytes_1m;
			pstat->tx_bytes_1m = pstat->tx_bytes - pstat->tx_bytes_1m;
		}
	}

	SMP_UNLOCK_ASOC_LIST(flags);

acct_cal_out:
	return ret;
}

int expire_sta_for_radiusacct(struct rtl8192cd_priv *priv)
{
	int ret=0;
	struct list_head *phead=NULL, *plist=NULL;
	struct stat_info *pstat=NULL;

	if( (ACCT_FUN_TIME == 0) && (ACCT_FUN_TP == 0))
		goto acct_expire_out;

	phead = &priv->asoc_list;
	if(list_empty(phead))
		goto acct_expire_out;

	plist = phead;

	while ((plist = asoc_list_get_next(priv, plist)) != phead) {
		pstat = list_entry(plist, struct stat_info, asoc_list);

		if(pstat->link_time > ACCT_FUN_TIME*60 ){
#if !defined(WITHOUT_ENQUEUE) && (defined(INCLUDE_WPA_PSK) || defined(WIFI_HAPD)  || defined(RTK_NL80211))
			indicate_sta_leaving(priv,pstat,_RSON_AUTH_NO_LONGER_VALID_);
#endif
			issue_deauth(priv,pstat->hwaddr,_RSON_AUTH_NO_LONGER_VALID_);
			cnt_assoc_num(priv, pstat, DECREASE, (char *)__FUNCTION__);
			check_sta_characteristic(priv, pstat, DECREASE);
			LOG_MSG("A STA(%02X:%02X:%02X:%02X:%02X:%02X) is deleted for accounting becoz of time-out\n",
				pstat->hwaddr[0], pstat->hwaddr[1], pstat->hwaddr[2], pstat->hwaddr[3], pstat->hwaddr[4], pstat->hwaddr[5]);
		}

		if(pstat->tx_bytes_1m+pstat->rx_bytes_1m < ACCT_FUN_TP*(2^20) ){
#if !defined(WITHOUT_ENQUEUE) && (defined(INCLUDE_WPA_PSK) || defined(WIFI_HAPD) || defined(RTK_NL80211))
			indicate_sta_leaving(priv,pstat,_RSON_AUTH_NO_LONGER_VALID_);
#endif
			issue_deauth(priv,pstat->hwaddr,_RSON_AUTH_NO_LONGER_VALID_);
			cnt_assoc_num(priv, pstat, DECREASE, (char *)__FUNCTION__);
			check_sta_characteristic(priv, pstat, DECREASE);
			LOG_MSG("A STA(%02X:%02X:%02X:%02X:%02X:%02X) is deleted for accounting becoz of low TP\n",
				pstat->hwaddr[0], pstat->hwaddr[1], pstat->hwaddr[2], pstat->hwaddr[3], pstat->hwaddr[4], pstat->hwaddr[5]);
		}
	}

acct_expire_out:
	return ret;
}
#endif	//#ifdef RADIUS_ACCOUNTING


//#if defined(SMART_REPEATER_MODE) && !defined(RTK_NL80211)
#if 0//removed to prevent system hang-up after repeater follow remote AP to switch channel
static void switch_chan_to_vxd(struct rtl8192cd_priv *priv)
{
	unsigned int i;

	priv->pmib->dot11RFEntry.dot11channel = priv->pshare->switch_chan_rp;
	priv->pmib->dot11nConfigEntry.dot11n2ndChOffset = priv->pshare->switch_2ndchoff_rp;
	GET_ROOT(priv)->pmib->dot11nConfigEntry.dot11nUse40M = priv->pshare->band_width_rp;
	RTL_W8(TXPAUSE, 0xff);

	// update vxd channel and 2ndChOffset
    //	GET_VXD_PRIV(priv)->pmib->dot11RFEntry.dot11channel = priv->pshare->switch_chan_rp;
    //	GET_VXD_PRIV(priv)->pmib->dot11nConfigEntry.dot11n2ndChOffset = priv->pshare->switch_2ndchoff_rp;

	DEBUG_INFO("3. Swiching channel to %d!\n", priv->pmib->dot11RFEntry.dot11channel);
	priv->pmib->dot11OperationEntry.keep_rsnie = 1;

	if (GET_ROOT(priv)->pmib->miscEntry.vap_enable) {
		for (i=0; i<RTL8192CD_NUM_VWLAN; i++) {
			if (IS_DRV_OPEN(priv->pvap_priv[i]))
				priv->pvap_priv[i]->pmib->dot11OperationEntry.keep_rsnie = 1;
		}
	}

	if (GET_VXD_PRIV(priv))
		GET_VXD_PRIV(priv)->pmib->dot11OperationEntry.keep_rsnie = 1;

	rtl8192cd_close(priv->dev);
	rtl8192cd_open(priv->dev);

	RTL_W8(TXPAUSE, 0x00);
}
#endif

#ifdef HS2_SUPPORT
#ifdef HS2_CLIENT_TEST
//issue_GASreq for client test used
int issue_GASreq(struct rtl8192cd_priv *priv, DOT11_HS2_GAS_REQ *gas_req, unsigned short qid);
int testflg=0;
#endif
#endif
#if defined(TXREPORT) && defined(CONFIG_WLAN_HAL)
void requestTxReport88XX(struct rtl8192cd_priv *priv)
{
	//unsigned char h2cresult;
	struct stat_info *sta;
	unsigned char H2CCommand[2] = {0xff, 0xff};

	if ( priv->pshare->sta_query_idx == -1)
		return;

#ifdef TXRETRY_CNT
	priv->pshare->sta_query_retry_idx = priv->pshare->sta_query_idx;
#endif

	sta = findNextSTA(priv, &priv->pshare->sta_query_idx);
	if (sta)
	{
		H2CCommand[0] = REMAP_AID(sta);
	}
	else {
		priv->pshare->sta_query_idx = -1;
		return;
	}

	sta = findNextSTA(priv, &priv->pshare->sta_query_idx);
	if (sta)	{
		H2CCommand[1] = REMAP_AID(sta);
	} else {
		priv->pshare->sta_query_idx = -1;
	}

	//WDEBUG("\n");
	//h2cresult = FillH2CCmd8812(priv, H2C_8812_TX_REPORT, 2 , H2CCommand);
	//WDEBUG("h2cresult=%d\n",h2cresult);
	GET_HAL_INTERFACE(priv)->FillH2CCmdHandler(priv, H2C_88XX_AP_REQ_TXREP, 2, H2CCommand);

}
#if (MU_BEAMFORMING_SUPPORT == 1)

void updateActiveGid(struct rtl8192cd_priv *priv)
{
	PRT_BEAMFORMING_INFO pBeamformingInfo = &(priv->pshare->BeamformingInfo);

	unsigned char gid_to_pos0_index[30] = {0,1,0,2,0,3,0,4,0,5,
										   1,2,1,3,1,4,1,5,2,3,
										   2,4,2,5,3,4,3,5,4,5};

	unsigned char gid_to_pos1_index[30] = {1,0,2,0,3,0,4,0,5,0,
											2,1,3,1,4,1,5,1,3,2,
											4,2,5,2,4,3,5,3,5,4};
	unsigned char	isActive;
	int gid;

	for(gid = 0; gid< 30; gid++) {
		isActive = (pBeamformingInfo->beamformee_mu_reg_maping & BIT(gid_to_pos0_index[gid])) && (pBeamformingInfo->beamformee_mu_reg_maping & BIT(gid_to_pos1_index[gid]));
		pBeamformingInfo->active_gid[gid] = isActive;
	}

}
int findNextMUSTA(struct rtl8192cd_priv *priv, int *gid)
{
	int isExist, orig_gid, i;

	PRT_BEAMFORMING_INFO pBeamformingInfo = &(priv->pshare->BeamformingInfo);

	if(pBeamformingInfo->beamformee_mu_reg_maping != pBeamformingInfo->beamformee_mu_reg_maping_old) {
		pBeamformingInfo->beamformee_mu_reg_maping_old = pBeamformingInfo->beamformee_mu_reg_maping;
		updateActiveGid(priv);
	}

	if(*gid < 0x80) {
		panic_printk("(%s)gid (%d) < 0x80\n", __FUNCTION__, *gid);
		return;
	}

	orig_gid = *gid;

	if(pBeamformingInfo->active_gid[orig_gid - 0x80]) {
		for(i = (orig_gid - 0x80) + 1; i<30; i++) {
			if(pBeamformingInfo->active_gid[i])	{
				*gid = 0x80 + i;
				break;
			}
		}
		if(i == 30) {
			*gid = -1;
		}

		return orig_gid;
	}
	*gid = -1;
	return 0;
}

void requestTxReport88XX_MU(struct rtl8192cd_priv *priv)
{
	//unsigned char h2cresult;
	unsigned int i, gid;
	struct stat_info *sta;
	unsigned char H2CCommand[2] = {0xff, 0xff};

	if ( priv->pshare->sta_query_idx == -1)
		return;

#ifdef TXRETRY_CNT
	priv->pshare->sta_query_retry_idx = priv->pshare->sta_query_idx;
#endif
	for(i=0;i<2;i++) {
		if(priv->pshare->sta_query_idx < 0x80) {
			sta = findNextSTA(priv, &priv->pshare->sta_query_idx);
			if (sta)
			{
				H2CCommand[i] = REMAP_AID(sta);
			}
			else {
				priv->pshare->sta_query_idx = 0x80;
				gid = findNextMUSTA(priv, &priv->pshare->sta_query_idx);
				if(gid) {
					H2CCommand[i] = gid;
					if(priv->pshare->sta_query_idx == -1)
						break;
				} else {
					if(i==0)
						return;
				}
			}
		} else {
			gid = findNextMUSTA(priv, &priv->pshare->sta_query_idx);
			if(gid) {
				H2CCommand[i] = gid;
				if(priv->pshare->sta_query_idx == -1)
					break;
			}
			else {
				if(i==0)
					return;
			}


		}
	}

	//WDEBUG("\n");
	//h2cresult = FillH2CCmd8812(priv, H2C_8812_TX_REPORT, 2 , H2CCommand);
	//WDEBUG("h2cresult=%d\n",h2cresult);
	GET_HAL_INTERFACE(priv)->FillH2CCmdHandler(priv, H2C_88XX_AP_REQ_TXREP, 2, H2CCommand);

}
#endif
#ifdef TXRETRY_CNT
void requestTxRetry88XX(struct rtl8192cd_priv *priv)
{
	unsigned char counter = 20;
	struct stat_info *sta;
	unsigned char H2CCommand[3] = {0xff, 0xff, 0x2};

	//panic_printk("%s %d \n", __FUNCTION__, __LINE__);

	if ( priv->pshare->sta_query_retry_idx == -1)
		return;

	while (is_h2c_buf_occupy(priv)) {
		delay_ms(2);
		if (--counter == 0)
			break;
	}

	if (!counter)
		return;

	//panic_printk("%s %d \n", __FUNCTION__, __LINE__);

	sta = findNextSTA(priv, &priv->pshare->sta_query_retry_idx);
	if (sta)
	{
		H2CCommand[0] = REMAP_AID(sta);
	}
	else {
		priv->pshare->sta_query_retry_idx = -1;
		return;
	}

	sta = findNextSTA(priv, &priv->pshare->sta_query_retry_idx);
	if (sta)	{
		H2CCommand[1] = REMAP_AID(sta);
	} else {
		priv->pshare->sta_query_retry_idx = -1;
	}

	priv->pshare->sta_query_retry_macid[0] = H2CCommand[0];
	priv->pshare->sta_query_retry_macid[1] = H2CCommand[1];

	GET_HAL_INTERFACE(priv)->FillH2CCmdHandler(priv, H2C_88XX_AP_REQ_TXREP, 3, H2CCommand);

}
#endif

#endif


int is_intel_connected(struct rtl8192cd_priv *priv)
{
	struct list_head *phead, *plist;
	struct stat_info *pstat;
	unsigned char intel_connected=0;

	phead = &priv->asoc_list;

	if (list_empty(phead)) {
		return 0;
	}

	SMP_LOCK_ASOC_LIST(flags);

	plist = phead->next;
	while (plist != phead) {
		pstat = list_entry(plist, struct stat_info, asoc_list);
		plist = plist->next;

		if(pstat->IOTPeer== HT_IOT_PEER_INTEL)
		{
			intel_connected = 1;
			break;
		}
	}

	SMP_UNLOCK_ASOC_LIST(flags);

	return intel_connected;
}

const int MCS_DATA_RATEFloat[2][2][16] =
{
	{{6.5, 13, 19.5, 26, 39, 52, 58.5, 65, 13, 26, 39, 52, 78, 104, 117, 130},						  // Long GI, 20MHz
	 {7.2, 14.4, 21.7, 28.9, 43.3, 57.8, 65, 72.2, 14.4, 28.9, 43.3, 57.8, 86.7, 115.6, 130, 144.5}}, // Short GI, 20MHz
	{{13.5, 27, 40.5, 54, 81, 108, 121.5, 135, 27, 54, 81, 108, 162, 216, 243, 270},                  // Long GI, 40MHz
	 {15, 30, 45, 60, 90, 120, 135, 150, 30, 60, 90, 120, 180, 240, 270, 300}}                        // Short GI, 40MHz
};
void check_sta_throughput(struct rtl8192cd_priv *priv, unsigned int chan_idx)
{
	struct list_head *phead, *plist;
	struct stat_info *pstat;
	int tx_rate=0, rx_rate=0;
	int tx_time=0, rx_time=0;

	//check every sta throughput
	phead = &priv->asoc_list;
	if (list_empty(phead)) {
		return;
	}
	plist = phead->next;

	while (plist != phead)
	{
		pstat = list_entry(plist, struct stat_info, asoc_list);
		pstat->tx_time_total=0;
		pstat->rx_time_total=0;
		//cal current tx rate
#ifdef RTK_AC_SUPPORT  //vht rate , todo, dump vht rates in Mbps
		if(pstat->current_tx_rate >= 0x90)
		{
			tx_rate = query_vht_rate(pstat);
			rx_rate = query_vht_rate(pstat);
		}
		else
#endif
		if (is_MCS_rate(pstat->current_tx_rate))
		{
			tx_rate = MCS_DATA_RATEFloat[(pstat->ht_current_tx_info&BIT(0))?1:0][(pstat->ht_current_tx_info&BIT(1))?1:0][pstat->current_tx_rate&0xf];
			rx_rate = MCS_DATA_RATEFloat[pstat->rx_bw&0x01][pstat->rx_splcp&0x01][pstat->rx_rate&0xf];
		}
		else
		{
			tx_rate = pstat->current_tx_rate/2;
			rx_rate = pstat->rx_rate/2;
		}

		//last tx/rx time
		if(tx_rate != 0)
			pstat->tx_time_total += (((pstat->tx_bytes - pstat->tx_byte_last)*8*1000)/(tx_rate*1024));
		if(rx_rate != 0)
			pstat->rx_time_total += (((pstat->rx_bytes - pstat->rx_byte_last)*8*1000)/(rx_rate*1024));
		#if 0
		printk("tx: rate[%d] tx_byte[%d] tx_byte_last[%d]\n",
			tx_rate, pstat->tx_bytes - pstat->tx_byte_last,  pstat->tx_byte_last);
		printk("rx: rate[%d] rx_byte[%d] rx_byte_last[%d]\n",
			rx_rate, pstat->rx_bytes - pstat->rx_byte_last,  pstat->rx_byte_last);
		#endif
		//tx/rx byte last
		pstat->tx_byte_last = pstat->tx_bytes;
		pstat->rx_byte_last = pstat->rx_bytes;

		//count all traffic time
		tx_time += pstat->tx_time_total;
		rx_time += pstat->rx_time_total;

		plist = plist->next;
	}
	priv->rtk->survey_info[chan_idx].tx_time = tx_time;
	priv->rtk->survey_info[chan_idx].rx_time = rx_time;

	//printk("tx_time=%d rx_time=%d\n",priv->rtk->survey_info[chan_idx].tx_time, priv->rtk->survey_info[chan_idx].rx_time);
}

int read_noise_report(struct rtl8192cd_priv *priv, unsigned int idx)
{
	s8 noise[RF92CD_PATH_MAX]={0}, noise_max=0, noise_tmp=0;
	int ret=0, i=0, loop=0, valid_cnt[RF92CD_PATH_MAX]={0};

	//8812 not support report noise from hardware
	if(GET_CHIP_VER(priv) == VERSION_8192E) {
		for(loop=0;loop<50;loop++) {
			for (i=RF92CD_PATH_A; i<2; i++) {
				RTL_W32(0x80c, RTL_R32(0x80c)|BIT(25));
                noise_tmp = (s8)((RTL_R32(0x8f8) & ((i==RF92CD_PATH_A)?0xff:0xff00)) >> (i*8));
				if(noise_tmp < 10 && noise_tmp >= -35) {	//advised by Luke.lee
                    noise[i] += noise_tmp;
					valid_cnt[i]++;
				}
				RTL_W32(0x80c, RTL_R32(0x80c)&(~BIT(25)));
				udelay(5);
			}
		}

		for(i=RF92CD_PATH_A; i<2; i++) {
			noise[i] /= valid_cnt[i];
			DEBUG_INFO("%s %d ch:%d noise[%d]:%02x(HEX), %d(INT), valid:%d\n",__func__,__LINE__,priv->site_survey->ss_channel,i,noise[i],noise[i],valid_cnt[i]);
		}

		for(i=RF92CD_PATH_A; i<2; i++) {
			if( noise_max == 0 )
				noise_max = noise[i];
			else if (noise[i] > noise_max)
				noise_max = noise[i];
		}

		noise_max -= 110;
	}


	//fixme, noise pwr should not be positive
	if(noise_max >= 0) {
		DEBUG_INFO("%s %d SNR is positive, force to ignorable value!\n",__func__,__LINE__);
		noise_max = -128;
	}

	priv->rtk->survey_info[idx].noise = noise_max;
	DEBUG_INFO("%s %d channel:%d noise floor:%d\n",__func__,__LINE__,priv->rtk->survey_info[idx].channel,priv->rtk->survey_info[idx].noise);

	return ret;
}

#if defined(TXRETRY_CNT) || defined(CH_LOAD_CAL)
void per_sta_cal(struct rtl8192cd_priv *priv)
{
	struct list_head *phead, *plist;
	struct stat_info *pstat;

#ifdef CH_LOAD_CAL
	int tx_rate=0, rx_rate=0;
	int tx_time=0, rx_time=0;
#endif

	//calculate every sta tx/rx time and retry ratio
	phead = &priv->asoc_list;
	if (list_empty(phead))
	{
		return;
	}
	plist = phead->next;

	while (plist != phead)
	{
		pstat = list_entry(plist, struct stat_info, asoc_list);
#ifdef CH_LOAD_CAL
		pstat->total_tx_time=0;
		pstat->total_rx_time=0;
		//cal current tx rate
#ifdef RTK_AC_SUPPORT  //vht rate , todo, dump vht rates in Mbps
		if(pstat->current_tx_rate >= 0x90)
		{
			tx_rate = query_vht_rate(pstat);
			rx_rate = query_vht_rate(pstat);
		}
		else
#endif
		if (is_MCS_rate(pstat->current_tx_rate))
			tx_rate = MCS_DATA_RATEFloat[(pstat->ht_current_tx_info&BIT(0))?1:0][(pstat->ht_current_tx_info&BIT(1))?1:0][pstat->current_tx_rate&0xf];
		else
			tx_rate = pstat->current_tx_rate/2;

		//rx rate
#ifdef RTK_AC_SUPPORT  //vht rate , todo, dump vht rates in Mbps
		if(pstat->rx_rate >= 0x90)
			rx_rate = query_vht_rate(pstat);
		else
#endif
		if (is_MCS_rate(pstat->rx_rate))
			rx_rate = MCS_DATA_RATEFloat[pstat->rx_bw&0x01][pstat->rx_splcp&0x01][pstat->rx_rate&0xf];
		else
			rx_rate = pstat->rx_rate/2;

		//last tx/rx time
		if(tx_rate != 0)
			pstat->total_tx_time+= ((((pstat->tx_bytes - pstat->prev_tx_byte)*8)/(tx_rate*1024))*1000)/1024;
		if(rx_rate != 0)
			pstat->total_rx_time+= ((((pstat->rx_bytes - pstat->prev_rx_byte)*8)/(rx_rate*1024))*1000)/1024;

		//tx/rx byte last
		pstat->prev_tx_byte = pstat->tx_bytes;
		pstat->prev_rx_byte = pstat->rx_bytes;

		//count all traffic time
		tx_time += pstat->total_tx_time;
		rx_time += pstat->total_rx_time;
#endif
#ifdef TXRETRY_CNT
		int tx_pkts = pstat->tx_pkts-pstat->prev_tx_pkts;
		int retry_pkts = pstat->total_tx_retry_cnt-pstat->prev_tx_retry_pkts;

		if(tx_pkts>0 && retry_pkts>0)
		{
			pstat->txretry_ratio = (retry_pkts*100)/(tx_pkts);
		}
		else
		{
			pstat->txretry_ratio= 0;
		}

		pstat->prev_tx_pkts = pstat->tx_pkts;
		pstat->prev_tx_retry_pkts = pstat->total_tx_retry_cnt;
#endif
		plist = plist->next;
	}

#ifdef CH_LOAD_CAL
	priv->ext_stats.tx_time = tx_time;
	priv->ext_stats.rx_time = rx_time;
#endif

}
#endif

#if defined(CLIENT_MODE) && defined(WIFI_11N_2040_COEXIST)
void start_clnt_coexist_scan(struct rtl8192cd_priv *priv)
{
	static unsigned int to_issue_coexist_mgt = 0;
	int do_scan = 1;
	int throughput = 0;

	if (((priv->pmib->dot11BssType.net_work_type & (WIRELESS_11N | WIRELESS_11G)) == (WIRELESS_11N | WIRELESS_11G)) &&
		priv->pmib->dot11nConfigEntry.dot11nCoexist &&
		priv->coexist_connection &&
		priv->pmib->dot11nConfigEntry.dot11nCoexist_obss_scan &&
		(!(priv->up_time % 85) || to_issue_coexist_mgt))
	{
		throughput = (priv->ext_stats.tx_avarage + priv->ext_stats.rx_avarage) >> 7; /*unit k bps*/

		/*more than 300k bps*/
		if (throughput > 300)
			do_scan = 0;

		if (IS_VXD_INTERFACE(priv)) {
			throughput = (GET_ROOT(priv)->ext_stats.tx_avarage + GET_ROOT(priv)->ext_stats.rx_avarage) >> 7;/*unit k bps*/
			/*more than 300k bps*/
			if (throughput > 300)
				do_scan = 0;

			if (GET_ROOT(priv)->pmib->miscEntry.vap_enable) {
				int i;
				for (i=0; i<RTL8192CD_NUM_VWLAN; i++) {
					if (IS_DRV_OPEN(GET_ROOT(priv)->pvap_priv[i])) {
						throughput = (GET_ROOT(priv)->pvap_priv[i]->ext_stats.tx_avarage + GET_ROOT(priv)->pvap_priv[i]->ext_stats.rx_avarage) >> 7;/*unit k bps*/

						/*more than 300k bps*/
						if (throughput > 300)
							do_scan = 0;
					}
				}
			}
		}

		if(!(priv->up_time % 85) && do_scan) {
			priv->ss_ssidlen = 0;
			DEBUG_INFO("start_clnt_ss, trigger by %s, ss_ssidlen=0\n", (char *)__FUNCTION__);
			priv->ss_req_ongoing = 1;
			start_clnt_ss(priv);
			to_issue_coexist_mgt++;
		}

		if (to_issue_coexist_mgt && !(OPMODE & WIFI_SITE_MONITOR) &&
			(priv->bg_ap_timeout || priv->intolerant_timeout)) {
			issue_coexist_mgt(priv);
			to_issue_coexist_mgt = 0;
		}
	}
}
#endif

void redo_acs_for_adaptivity(struct rtl8192cd_priv *priv)
{
	unsigned int diff_time;

	if (OPMODE & WIFI_AP_STATE)
	{
		if ( !priv->assoc_num )
		{
			if ( !priv->pshare->no_sta_link_tick )
				priv->pshare->no_sta_link_tick = priv->up_time;

			diff_time = UINT32_DIFF(priv->up_time, priv->pshare->no_sta_link_tick);

			if ( diff_time >= 60 )
			{
				priv->pshare->no_sta_link_tick = 0;

				priv->pshare->reg_tapause_bak = RTL_R8(TXPAUSE);
				RTL_W8(TXPAUSE, 0xff);				//it will be recovery after ACS done
				priv->pshare->acs_for_adaptivity_flag = TRUE;

				DEBUG_WARN("No STA connect for 1 mins, do ACS!!\n");
				rtl8192cd_autochannel_sel(priv);
			}
			else if ( diff_time % 10 == 0 )
				DEBUG_WARN("No STA connect for %ds\n", diff_time);
		}
		else
			priv->pshare->no_sta_link_tick = 0;
	}
}


#if defined(CONFIG_IEEE80211W) || defined(CONFIG_IEEE80211V)
void issue_actionFrame(struct rtl8192cd_priv *priv)
{
    	struct stat_info *pstat;
    	struct list_head *phead, *plist;

    	if(priv->pmib->dot11StationConfigEntry.pmftest == 1) { // for PMF case 4.4, protect broadcast mgmt frame by BIP
    		PMFDEBUG("%s(%d) issue broadcast deauth\n", __FUNCTION__, __LINE__);
    		memcpy(priv->pmib->dot11StationConfigEntry.deauth_mac,"\xff\xff\xff\xff\xff\xff",6);
    		issue_deauth(priv, priv->pmib->dot11StationConfigEntry.deauth_mac, _RSON_UNSPECIFIED_);

    		phead = &priv->asoc_list;
    		if (netif_running(priv->dev) && !list_empty(phead)) {
    			plist = phead->next;
    			while (plist != phead) {
    				pstat = list_entry(plist, struct stat_info, asoc_list);
    				PMFDEBUG("pstat=%02x%02x%02x%02x%02x%02x\n", pstat->hwaddr[0]
    																, pstat->hwaddr[1]
    																, pstat->hwaddr[2]
    																, pstat->hwaddr[3]
    																, pstat->hwaddr[4]
    																, pstat->hwaddr[5]);
    				plist = plist->next;
    				del_station(priv, pstat, 0);

    			}
    		}
    		priv->pmib->dot11StationConfigEntry.pmftest = 0;
    	} else if(priv->pmib->dot11StationConfigEntry.pmftest == 2) {
    		PMFDEBUG("issue unicast deauth\n");
    		pstat = get_stainfo(priv, priv->pmib->dot11StationConfigEntry.deauth_mac);
    		if(!pstat) {
    			PMFDEBUG("no associated STA (%02x%02x%02x%02x%02x%02x)\n"
    										,priv->pmib->dot11StationConfigEntry.deauth_mac[0]
    										,priv->pmib->dot11StationConfigEntry.deauth_mac[1]
    										,priv->pmib->dot11StationConfigEntry.deauth_mac[2]
    										,priv->pmib->dot11StationConfigEntry.deauth_mac[3]
    										,priv->pmib->dot11StationConfigEntry.deauth_mac[4]
    										,priv->pmib->dot11StationConfigEntry.deauth_mac[5]);
    		} else {
    			issue_deauth(priv, priv->pmib->dot11StationConfigEntry.deauth_mac, _RSON_UNSPECIFIED_);
    			del_station(priv, pstat, 0);
    		}
    		priv->pmib->dot11StationConfigEntry.pmftest = 0;
    	}
#ifdef CONFIG_IEEE80211W_CLI_DEBUG
        else if(priv->pmib->dot11StationConfigEntry.pmftest == 3){
    		panic_printk("%s(%d) issue unicast disassoc\n", __FUNCTION__, __LINE__);
    		pstat = get_stainfo(priv, priv->pmib->dot11StationConfigEntry.deauth_mac);
    		if(!pstat) {
    			panic_printk("%s(%d) no associated STA (%02x%02x%02x%02x%02x%02x)\n", __FUNCTION__, __LINE__
    										,priv->pmib->dot11StationConfigEntry.deauth_mac[0]
    										,priv->pmib->dot11StationConfigEntry.deauth_mac[1]
    										,priv->pmib->dot11StationConfigEntry.deauth_mac[2]
    										,priv->pmib->dot11StationConfigEntry.deauth_mac[3]
    										,priv->pmib->dot11StationConfigEntry.deauth_mac[4]
    										,priv->pmib->dot11StationConfigEntry.deauth_mac[5]);
    		} else {
    			issue_disassoc(priv, priv->pmib->dot11StationConfigEntry.deauth_mac, _RSON_UNSPECIFIED_);
    			del_station(priv, pstat, 0);
    		}
    		priv->pmib->dot11StationConfigEntry.pmftest = 0;
    	}else if(priv->pmib->dot11StationConfigEntry.pmftest == 4){
    		panic_printk("%s(%d) issue SA Request\n", __FUNCTION__, __LINE__);
    		pstat = get_stainfo(priv, priv->pmib->dot11StationConfigEntry.sa_req_mac);
    		if(!pstat) {
    			panic_printk("%s(%d) no associated STA (%02x%02x%02x%02x%02x%02x)\n", __FUNCTION__, __LINE__
    										,priv->pmib->dot11StationConfigEntry.sa_req_mac[0]
    										,priv->pmib->dot11StationConfigEntry.sa_req_mac[1]
    										,priv->pmib->dot11StationConfigEntry.sa_req_mac[2]
    										,priv->pmib->dot11StationConfigEntry.sa_req_mac[3]
    										,priv->pmib->dot11StationConfigEntry.sa_req_mac[4]
    										,priv->pmib->dot11StationConfigEntry.sa_req_mac[5]);
    		} else {
    				if(pstat->sa_query_count == 0) {
    					panic_printk("sa_query_end=%lu, sa_query_start=%lu\n", pstat->sa_query_end, pstat->sa_query_start);
    					pstat->sa_query_count++;
    					issue_SA_Query_Req(priv->dev,priv->pmib->dot11StationConfigEntry.sa_req_mac);

    					panic_printk("%s(%d), settimer, %x\n", __FUNCTION__, __LINE__, &pstat->SA_timer);

    					if(timer_pending(&pstat->SA_timer))
    						del_timer(&pstat->SA_timer);

    					pstat->SA_timer.data = (unsigned long) pstat;
    					pstat->SA_timer.function = rtl8192cd_sa_query_timer;
    					mod_timer(&pstat->SA_timer, jiffies + RTL_MILISECONDS_TO_JIFFIES(SA_QUERY_RETRY_TO));
    					panic_printk("%s(%d), settimer end\n", __FUNCTION__, __LINE__);
    				}
    		}
    		priv->pmib->dot11StationConfigEntry.pmftest = 0;
    	}else if(priv->pmib->dot11StationConfigEntry.pmftest == 5){ // for PMF case 4.4, protect broadcast mgmt frame by BIP
    		panic_printk("%s(%d) issue broadcast diassoc\n", __FUNCTION__, __LINE__);
    		memcpy(priv->pmib->dot11StationConfigEntry.deauth_mac,"\xff\xff\xff\xff\xff\xff",6);
    		issue_disassoc(priv, priv->pmib->dot11StationConfigEntry.deauth_mac, _RSON_UNSPECIFIED_);

    		phead = &priv->asoc_list;
    		if (netif_running(priv->dev) && !list_empty(phead)) {
    			plist = phead->next;
    			while (plist != phead) {
    				pstat = list_entry(plist, struct stat_info, asoc_list);
    				panic_printk("pstat=%02x%02x%02x%02x%02x%02x\n", pstat->hwaddr[0]
    																, pstat->hwaddr[1]
    																, pstat->hwaddr[2]
    																, pstat->hwaddr[3]
    																, pstat->hwaddr[4]
    																, pstat->hwaddr[5]);
    				plist = plist->next;
    				del_station(priv, pstat, 0);

    			}
    		}
    		priv->pmib->dot11StationConfigEntry.pmftest = 0;
    	}
#endif
#ifdef CONFIG_IEEE80211V
	  else if(WNM_ENABLE && priv->pmib->dot11StationConfigEntry.wnmtest == 1) {
		process_BssTransReq(priv);
		priv->pmib->dot11StationConfigEntry.wnmtest = 0;
	}
#endif
}
#endif

#ifdef PERIODIC_AUTO_CHANNEL
void rtl8192cd_auto_channel_timer(struct rtl8192cd_priv *priv)
{
	int i=0, PATcountdown=1; // periodic auto channel count down
	int fa_lv=0, cca_lv=0, noise_level=0;
	unsigned int fa, cca;

	//when ACS do not count
	if(priv->auto_channel == 1)
		return;

	#ifdef DYNAMIC_AUTO_CHANNEL
	//caculate noise level
	fa = priv->pshare->FA_total_cnt;
	cca = priv->pshare->CCA_total_cnt;

	//FA
	if (fa>16000) {//1000*16
		fa_lv = 100;
	} else if (fa>8000) {//500*16
		fa_lv = 34 * (fa-8000) / 8000 + 66;//500*16
	} else if (fa>3200) {//200*16
		fa_lv = 33 * (fa - 3200) / 4800 + 33;//300*16
	} else if (fa>1600) {//100*16
		fa_lv = 18 * (fa - 1600) / 1600 + 15;//100*16
	} else {
		fa_lv = 15 * fa / 1600;//100*16
	}

	//CCA
	if (cca>12000) {//400*30
		cca_lv = 100;
	} else if (cca>6000) {//200*30
		cca_lv = 34 * (cca - 6000) / 6000 + 66;//200*30
	} else if (cca>2400) {//80*30
		cca_lv = 33 * (cca - 2400) / 3600 + 33;//120*30
	} else if (cca>1200) {//40*30
		cca_lv = 18 * (cca - 1200) / 1200 + 15;//40*30
	} else {
		cca_lv = 15 * cca / 1200;//40*30
	}

	if(fa_lv > cca_lv)
		noise_level = (fa_lv*75 + priv->pshare->noise_level_pre*25)/100;
	else
		noise_level = (cca_lv*75 + priv->pshare->noise_level_pre*25)/100;

	//record old data
	priv->pshare->noise_level_pre = noise_level;

	//debug
	PDEBUG("[%d][%s] noise_level=%d sta_is_idle=%d\n",priv->pshare->PAT, priv->dev->name, noise_level, priv->pshare->all_cli_idle);
	#endif
	//if( priv->auto_channel_backup && IS_ROOT_INTERFACE(priv) && (OPMODE & WIFI_AP_STATE) &&
	if(priv->pmib->dot11RFEntry.periodicAutochannel)
	{
		/*PATcountdown=1;default is 1*/
		if(priv->assoc_num){
		    PATcountdown = 0;
		}

		// when root assoc_num =0  also consider if VAP interfaces assoc_num=0
		if (PATcountdown && priv->pmib->miscEntry.vap_enable)
		{
			for (i=0; i<RTL8192CD_NUM_VWLAN; i++)
	        {
		        if (priv->pvap_priv[i] && IS_DRV_OPEN(priv->pvap_priv[i]) && priv->pvap_priv[i]->assoc_num)
			    {
				    PATcountdown = 0;
					PDEBUG("vap[%d] has client\n",i);
	            }
		    }
	    }

#ifdef DYNAMIC_AUTO_CHANNEL//dynamic ACS must count client into consideration//SW#5
		if(priv->pmib->dot11RFEntry.dynamicACS_noise>0 ||
			priv->pmib->dot11RFEntry.dynamicACS_idle>0)
			PATcountdown = 1;
#endif

		if(PATcountdown==0){
			priv->pshare->PAT = priv->pmib->dot11RFEntry.periodicAutochannel;
			PDEBUG("refill PAT=%d\n",priv->pshare->PAT);
		}else{
			priv->pshare->PAT--;

			if(priv->pshare->PAT==0){
#ifdef DYNAMIC_AUTO_CHANNEL//SW#5
				if(priv->pmib->dot11RFEntry.dynamicACS_noise>0 ||
					priv->pmib->dot11RFEntry.dynamicACS_idle>0){
					//check noise
					if(noise_level > priv->pmib->dot11RFEntry.dynamicACS_noise)
					{
						//check client
						if(priv->pmib->dot11RFEntry.dynamicACS_idle>0)
						{
							if(priv->pshare->all_cli_idle == 1)//all clients are idle
							{
								if(rtl8192cd_autochannel_sel(priv)==0)
								{
									priv->pshare->PAT = priv->pmib->dot11RFEntry.periodicAutochannel;
								    PDEBUG("Dynamic auto-channel case1 finished , refill PAT=%d\n",priv->pshare->PAT);
								}
								else
									goto try_again_next_second;
							}
							else
								goto try_again_next_second;
						}
						else//do not care client idle time
						{
							if(rtl8192cd_autochannel_sel(priv)==0)
							{
								priv->pshare->PAT = priv->pmib->dot11RFEntry.periodicAutochannel;
								PDEBUG("Dynamic auto-channel case2 finished , refill PAT=%d\n",priv->pshare->PAT);
							}
							else
								goto try_again_next_second;
						}
					}
					else
						goto try_again_next_second;
				}
				else
#endif
				{//PERIODIC_AUTO_CHANNEL
					if(rtl8192cd_autochannel_sel(priv)==0)
					{
						priv->pshare->PAT = priv->pmib->dot11RFEntry.periodicAutochannel;
						PDEBUG("period auto-channel finished , refill PAT=%d\n",priv->pshare->PAT);
					}
					else
						goto try_again_next_second;
				}
			}
		}
	}

	return;

try_again_next_second:
	priv->pshare->PAT++;
	#ifdef DYNAMIC_AUTO_CHANNEL//SW#5
	PDEBUG("[1s][%s] noise_level=%d all_cli_idle=%d, try again in %d sec\n",
		priv->dev->name, noise_level, priv->pshare->all_cli_idle, priv->pshare->PAT);
	#endif

	return;

}
#endif

void rtl8192cd_expire_timer(unsigned long task_priv)
{
	struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)task_priv;
	unsigned long	flags=0;
    int somevapopened=0;
	int i;
#if defined(CROSSBAND_REPEATER) || defined(RTK_MESH_METRIC_REFINE)
	int index;
#endif

	if (!(priv->drv_state & DRV_STATE_OPEN))
		return;

	SAVE_INT_AND_CLI(flags);
	SMP_LOCK(flags);


	// advance driver up timer
	priv->up_time++;

#ifdef TLN_STATS
	if (priv->pshare->rf_ft_var.stats_time_interval) {
		if (priv->stats_time_countdown) {
			priv->stats_time_countdown--;
		} else {
			memset(&priv->wifi_stats, 0, sizeof(struct tln_wifi_stats));
			memset(&priv->ext_wifi_stats, 0, sizeof(struct tln_ext_wifi_stats));

			priv->stats_time_countdown = priv->pshare->rf_ft_var.stats_time_interval;
		}
	}
#endif

#ifdef	INCLUDE_WPS
	// mount wsp wps_1sec_routine
	if (IS_ROOT_INTERFACE(priv))
	{

#ifndef CONFIG_MSC		// for verify
		if(priv->pshare->WSC_CONT_S.RdyGetConfig == 0){
			// if not get config from upnp yet ; send request
			unsigned char tmp12[20];
			sprintf(tmp12 , "wps_get_config=1");
			set_mib(priv , tmp12) ;
		}
#endif
		if(priv->pshare->WSC_CONT_S.oneSecTimeStart){
			wps_1sec_routine(priv);
		}
        else{
            printk("%s %d not enter wsc 1sec \n", __FUNCTION__, __LINE__);
        }
	}
#endif



#ifdef CONFIG_32K//tingchu
    if (priv->offload_32k_flag==1){
        SMP_UNLOCK(flags);
		RESTORE_INT(flags);
        return;
    }
#endif


	// check auth_list
	auth_expire(priv);

	if (IS_ROOT_INTERFACE(priv))
	{
		if ((priv->up_time % 2) == 0) {
			priv->pshare->highTP_found_pstat = NULL;
			priv->pshare->highTP_found_root_pstat = NULL;
			priv->pshare->highTP_found_vxd_pstat = NULL;
		}
		priv->pshare->rssi_min = 0xff;
		priv->pshare->mimorssi_min= 0xff;
#ifdef RSSI_MIN_ADV_SEL
		priv->pshare->min_rssi_data_cnt = 0;
#endif
#ifdef CLIENT_MODE
		if((OPMODE & WIFI_ADHOC_STATE) && (priv->prev_tsf)) {
			UINT64		tsf, tsf_diff;
			tsf = RTL_R32(TSFTR+4);
			tsf = (tsf<<32) +  RTL_R32(TSFTR);
			tsf_diff = tsf - priv->prev_tsf;
			priv->prev_tsf = tsf;
			if( (tsf > priv->prev_tsf) && (tsf_diff > BIT(24))) {
				tsf = cpu_to_le64(tsf);
				memcpy(priv->rx_timestamp, (void*)(&tsf), 8);
				updateTSF(priv);
			}
		}
#endif

#ifdef DYNAMIC_AUTO_CHANNEL//SW#5
			priv->pshare->all_cli_idle = 1;//reset when 1sec timer start
#endif
	}

#ifdef RX_CRC_EXPTIMER
	if(GET_CHIP_VER(priv) == VERSION_8814A || GET_CHIP_VER(priv) == VERSION_8197F) {
		if(priv->pshare->rf_ft_var.crc_enable || priv->pshare->rf_ft_var.crc_dump || priv->pshare->rf_ft_var.mp_specific)
			RTL_W16(0x608, RTL_R16(0x608) | BIT(8));
		else
			RTL_W16(0x608, RTL_R16(0x608) & ~BIT(8));
	}
#endif

#ifdef RTK_ATM
	// 1. if atm_mode = 4, check the pre-set client list
	// 	and accumulate all sta_time in client list and asign client sta_time to atm_match_sta_time
	if(IS_ROOT_INTERFACE(priv)) //&& priv->pshare->rf_ft_var.atm_en)
	{
		//if atm_mode=2 client mode, check matched client first
		//if(priv->pshare->atm_ttl_stanum > 0)
		if(priv->pshare->rf_ft_var.atm_en)
			atm_check_statime(priv);
	}
#endif

#if (MU_BEAMFORMING_SUPPORT == 1)
PRT_BEAMFORMING_INFO	pBeamInfo = &(priv->pshare->BeamformingInfo);

			if (GET_CHIP_VER(priv) == VERSION_8822B) {
				unsigned int Reg7D4 = RTL_R8(0x7d4);
				if (pBeamInfo->beamformee_mu_reg_maping && (Reg7D4 & BIT3)) {
					Reg7D4 &= ~BIT3;
					RTL_W8(0x7d4, Reg7D4);
				} else if (!pBeamInfo->beamformee_mu_reg_maping && (Reg7D4 & BIT3)==0) {
					Reg7D4 |= BIT3;
					RTL_W8(0x7d4, Reg7D4);
				}
			}

	if (priv->pshare->rf_ft_var.mu_dump && !(priv->up_time % priv->pshare->rf_ft_var.mu_dump)) {
		int i;
		PDM_ODM_T	pDM_Odm = ODMPTR;
		PRT_BEAMFORMING_ENTRY	pEntry = NULL;
		struct stat_info	*pstat;

		char strMuRate[20];
		unsigned char e1e2_to_gid[MAX_NUM_BEAMFORMEE_MU][MAX_NUM_BEAMFORMEE_MU]={{2,2,4,6,8,10},
  							{1,1,12,14,16,18},
  							{3,11,20,20,22,24},
  							{5,13,19,19,26,28},
  							{7,15,21,25,25,30},
  							{9,17,23,27,29,29}};
		u1Byte partnerIndex, groupIndex;

		for(i = 0; i < BEAMFORMEE_ENTRY_NUM; i++) {
			pEntry = &(pBeamInfo->BeamformeeEntry[i]);
			if( pEntry->bUsed && (pEntry->is_mu_sta == TXBF_TYPE_MU) && pEntry->priv == priv) {
				pstat = pEntry->pSTA;
				if(pstat) {
					if(pstat->muPartner) {
						groupIndex = e1e2_to_gid[pEntry->mu_reg_index][priv->pshare->rf_ft_var.muPairResult[pEntry->mu_reg_index]] - 1;
						pEntry->mu_tx_rate = pDM_Odm->DM_RA_Table.mu1_rate[groupIndex];
						pEntry->pSTA->mu_rate = pDM_Odm->DM_RA_Table.mu1_rate[groupIndex];
					}

					translate_txforce_to_rateStr(strMuRate, (0x7f & pEntry->mu_tx_rate));

					panic_printk("[%d][P %d][SND %d %d] %s (MU TX %s) (CSI %d %d) (TP %3d,%3d)\n",
					   pstat->aid,
					   (pstat->muPartner)?pstat->muPartner->aid:0,
					   priv->pshare->rf_ft_var.mu_ok[pEntry->mu_reg_index],
					   priv->pshare->rf_ft_var.mu_fail[pEntry->mu_reg_index],
					   show_sta_trx_rate(priv, pstat),
					   strMuRate,
					   priv->pshare->rf_ft_var.mu_BB_ok,
					   priv->pshare->rf_ft_var.mu_BB_fail,
					   (unsigned int)(pstat->tx_avarage>>17),
					   (unsigned int)(pstat->rx_avarage>>17)
					   );
				}
			}
		}
	}
#endif

	// check asoc_list
	assoc_expire(priv);

#ifdef SW_TX_QUEUE
	if (priv->pshare->rf_ft_var.totaltp  && !(priv->up_time % priv->pshare->rf_ft_var.totaltp)) {
		panic_printk("totaltp (%d %d) drop(%d) fail(%d) SWQ(%d %d %d)\n", (unsigned int)(priv->ext_stats.tx_avarage>>17), (unsigned int)(priv->ext_stats.rx_avarage>>17),  priv->ext_stats.tx_drops, priv->net_stats.tx_errors, priv->ext_stats.swq_enque_pkt, priv->ext_stats.swq_xmit_out_pkt, priv->ext_stats.swq_drop_pkt);
    }
#endif
#if defined(DRVMAC_LB) && defined(WIFI_WMM)
	if (priv->pmib->miscEntry.drvmac_lb && priv->pmib->miscEntry.lb_tps) {
		unsigned int i = 0;
		for (i = 0; i < priv->pmib->miscEntry.lb_tps; i++) {
			if (priv->pmib->miscEntry.lb_mlmp)
				SendLbQosData(priv);
			else
				SendLbQosNullData(priv);
//			if (i > 4)
//				priv->pmib->miscEntry.lb_tps = 0;
		}
	}
#endif

#ifdef WIFI_SIMPLE_CONFIG
	// check wsc probe request list
	if (priv->pmib->wscEntry.wsc_enable & 2) // work as AP (not registrar)
		wsc_probe_expire(priv);
	wsc_disconn_list_expire(priv);
#endif


	// check link status and start/stop net queue
	priv->link_status = chklink_wkstaQ(priv);


#ifdef RTK_AC_SUPPORT //for 11ac logo
	// channel switch
	if(priv->pshare->rf_ft_var.csa) {
		int ch = priv->pshare->rf_ft_var.csa;
		priv->pshare->rf_ft_var.csa = 0;

		GET_ROOT(priv)->pmib->dot11DFSEntry.DFS_detected = 1;
		priv->pshare->dfsSwitchChannel = ch;
		priv->pshare->dfsSwitchChCountDown = 6;
		if (priv->pmib->dot11StationConfigEntry.dot11DTIMPeriod >= priv->pshare->dfsSwitchChCountDown)
			priv->pshare->dfsSwitchChCountDown = priv->pmib->dot11StationConfigEntry.dot11DTIMPeriod+1;
	}
#endif

	// for SW LED
	if ((LED_TYPE >= LEDTYPE_SW_LINK_TXRX) && (LED_TYPE < LEDTYPE_SW_MAX))
	{
		if (IS_ROOT_INTERFACE(priv))
			calculate_sw_LED_interval(priv);
	}

	{
#ifdef CLIENT_MODE
		if (((OPMODE & WIFI_AP_STATE) ||
			((OPMODE & WIFI_ADHOC_STATE) &&
				((JOIN_RES == STATE_Sta_Ibss_Active) || (JOIN_RES == STATE_Sta_Ibss_Idle)))) &&
			(priv->pmib->dot11BssType.net_work_type & WIRELESS_11G))
#else
		if ((OPMODE & WIFI_AP_STATE) &&
			(priv->pmib->dot11BssType.net_work_type & WIRELESS_11G))
#endif
		{
			if (priv->pmib->dot11ErpInfo.olbcDetected) {
				if (priv->pmib->dot11ErpInfo.olbcExpired > 0)
					priv->pmib->dot11ErpInfo.olbcExpired--;

				if (priv->pmib->dot11ErpInfo.olbcExpired == 0) {
					priv->pmib->dot11ErpInfo.olbcDetected = 0;
					DEBUG_INFO("OLBC expired\n");
					check_protection_shortslot(priv);
				}
			}
		}
	}

#ifdef TX_EARLY_MODE
	priv->pshare->em_tx_byte_cnt  = priv->ext_stats.tx_byte_cnt;
#endif

	// calculate tx/rx throughput
	priv->ext_stats.tx_avarage = (priv->ext_stats.tx_avarage/10)*7 + (priv->ext_stats.tx_byte_cnt/10)*3;
	priv->ext_stats.tx_byte_cnt = 0;
	priv->ext_stats.rx_avarage = (priv->ext_stats.rx_avarage/10)*7 + (priv->ext_stats.rx_byte_cnt/10)*3;
	priv->ext_stats.rx_byte_cnt = 0;



#if defined(TXRETRY_CNT) || defined(CH_LOAD_CAL)
	per_sta_cal(priv);
#endif

#if defined(CROSSBAND_REPEATER) || defined(RTK_MESH_METRIC_REFINE)

//Retrieve channel utilization info
		priv->envinfo.info_record[priv->envinfo.index].cu_value = priv->ext_stats.ch_utilization;
#endif


#if 0 //brian, collect channel load information during sitesurvey in stead of here to make sure freshness
	//survey_dump
	int val = read_bbp_ch_load(priv);
	if(val != -1)
	{
		priv->rtk->chbusytime = (val/1000)*5;
		start_bbp_ch_load(priv);
	}
	check_sta_throughput(priv);
#endif
	//openwrt_psd
	if(priv->rtk->psd_chnl != 0)
		rtl8192cd_query_psd_cfg80211(priv, priv->rtk->psd_chnl, priv->rtk->psd_bw, priv->rtk->psd_pts);
	priv->rtk->psd_chnl=0;
	priv->rtk->psd_bw=0;
	priv->rtk->psd_pts=0;

#ifdef CONFIG_RTL8190_THROUGHPUT
	if (IS_ROOT_INTERFACE(priv))
	{
		unsigned long throughput;

		throughput = (priv->ext_stats.tx_avarage + priv->ext_stats.rx_avarage) * 8 / 1024 / 1024; /* unit: Mbps */
		if (gCpuCanSuspend) {
			if (throughput > TP_HIGH_WATER_MARK) {
				gCpuCanSuspend = 0;
			}
		}
		else {
			if (throughput < TP_LOW_WATER_MARK) {
				gCpuCanSuspend = 1;
			}
		}
	}
#endif

//#ifdef CONFIG_RTL865X_AC
#if defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD) || defined(CONFIG_RTL8196C_KLD)
	if (priv->ext_stats.tx_avarage > priv->ext_stats.tx_peak)
		priv->ext_stats.tx_peak = priv->ext_stats.tx_avarage;

	if (priv->ext_stats.rx_avarage > priv->ext_stats.rx_peak)
		priv->ext_stats.rx_peak = priv->ext_stats.rx_avarage;
#endif

#ifdef CLIENT_MODE
	if (OPMODE & (WIFI_STATION_STATE | WIFI_ADHOC_STATE))
	{

		// calculate how many beacons we received and decide if should roaming
		if (!priv->pshare->dfsSwCh_ongoing)
			calculate_rx_beacon(priv);

		// expire NAT2.5 entry
		nat25_db_expire(priv);

		if (priv->pppoe_connection_in_progress > 0)
			priv->pppoe_connection_in_progress--;
	}
#endif



#ifdef A4_STA
    a4_sta_expire(priv);
#endif

#ifdef TV_MODE
    if(OPMODE & WIFI_AP_STATE && (priv->up_time%2 == 0)) {
        if(priv->tv_mode_status & BIT1) { /*TV mode is auto, check if there is STA that support tv auto*/
            tv_mode_auto_support_check(priv);
        }
    }
#endif



#ifdef MP_TEST
	if ((OPMODE & (WIFI_MP_CTX_BACKGROUND|WIFI_MP_CTX_BACKGROUND_PENDING)) ==
		(WIFI_MP_CTX_BACKGROUND|WIFI_MP_CTX_BACKGROUND_PENDING))
		rtl8192cd_tx_dsr((unsigned long)priv);


	if (OPMODE & WIFI_MP_RX) {
		if (priv->pshare->rf_ft_var.rssi_dump) {
#ifdef _OUTSRC_COEXIST
			if(IS_OUTSRC_CHIP(priv))
#endif
			{
				panic_printk("%d%%  (ss %d %d )(snr %d %d )(sq %d %d)\n",
				priv->pshare->mp_rssi,
				priv->pshare->mp_rf_info.mimorssi[0], priv->pshare->mp_rf_info.mimorssi[1],
				priv->pshare->mp_rf_info.RxSNRdB[0], priv->pshare->mp_rf_info.RxSNRdB[1],
				priv->pshare->mp_rf_info.mimosq[0], priv->pshare->mp_rf_info.mimosq[1]);
		}

#if !defined(USE_OUT_SRC) || defined(_OUTSRC_COEXIST)
#ifdef _OUTSRC_COEXIST
			if(!IS_OUTSRC_CHIP(priv))
#endif
			{
				panic_printk("%d%%  (ss %d %d )(snr %d %d )(sq %d %d)\n",
				priv->pshare->mp_rssi,
				priv->pshare->mp_rf_info.mimorssi[0], priv->pshare->mp_rf_info.mimorssi[1],
				priv->pshare->mp_rf_info.RxSNRdB[0], priv->pshare->mp_rf_info.RxSNRdB[1],
				priv->pshare->mp_rf_info.mimosq[0], priv->pshare->mp_rf_info.mimosq[1]);
			}
#endif
		}
	}
#endif

	// Realtek proprietary IE
		process_rtk_ie(priv);

	// check ACL log event
	if ((OPMODE & WIFI_AP_STATE) && priv->acLogCountdown > 0) {
		if (--priv->acLogCountdown == 0)
			if (aclog_check(priv) > 0) // still have active entry
				priv->acLogCountdown = AC_LOG_TIME;
	}

#ifdef CLIENT_MODE
	if (OPMODE & WIFI_AP_STATE)
#endif
	{
		// 11n protection count down
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
			if (priv->ht_legacy_obss_to > 0)
				priv->ht_legacy_obss_to--;
			if (priv->ht_nomember_legacy_sta_to > 0)
				priv->ht_nomember_legacy_sta_to--;
		}
	}
#ifdef WIFI_11N_2040_COEXIST
	if (priv->pmib->dot11nConfigEntry.dot11nCoexist && ((OPMODE & WIFI_AP_STATE)
#ifdef CLIENT_MODE
		|| ((OPMODE & WIFI_STATION_STATE) && priv->coexist_connection)
#endif
		) && (priv->pmib->dot11BssType.net_work_type & (WIRELESS_11N|WIRELESS_11G))) {
		if (priv->bg_ap_timeout) {
			// don't go back to 40M mode for 6300 wrong channel issue
			//priv->bg_ap_timeout--;
#ifdef CLIENT_MODE
			if (OPMODE & WIFI_STATION_STATE) {
				unsigned int i;
				for (i = 0; i < 14; i++)
					if (priv->bg_ap_timeout_ch[i])
						priv->bg_ap_timeout_ch[i]--;
			}
#endif
		}
#ifdef CLIENT_MODE
		if ((OPMODE & WIFI_STATION_STATE) && priv->intolerant_timeout)
			priv->intolerant_timeout--;
#endif
	}
#endif

	// Dump Rx FiFo overflow count
	if (priv->pshare->rf_ft_var.rxfifoO) {
		panic_printk("RxFiFo Overflow: %d\n", (unsigned int)(priv->ext_stats.rx_fifoO - priv->pshare->rxFiFoO_pre));
		priv->pshare->rxFiFoO_pre = priv->ext_stats.rx_fifoO;
	}

	if (IS_ROOT_INTERFACE(priv))
	{
#ifdef WIFI_WMM
		if (QOS_ENABLE) {
#ifdef CLIENT_MODE
			if((OPMODE & WIFI_STATION_STATE) && (!priv->link_status) && (priv->pmib->dot11QosEntry.EDCA_STA_config)) {
				reset_EDCA_para(priv);
			}
#endif
		}
#endif

		if (fw_was_full(priv) && priv->pshare->fw_free_space > 0) { // there are free space for STA
			// do some algorithms to re-alloc STA into free space
			realloc_RATid(priv);
		}

#ifdef TX_EARLY_MODE
		if (GET_TX_EARLY_MODE) {
			if (!GET_EM_SWQ_ENABLE) {
				if (priv->pshare->em_tx_byte_cnt > EM_TP_UP_BOUND)
					priv->pshare->reach_tx_limit_cnt++;
				else
					priv->pshare->reach_tx_limit_cnt = 0;

				if (priv->pshare->txop_enlarge && priv->pshare->reach_tx_limit_cnt >= WAIT_TP_TIME) {
					GET_EM_SWQ_ENABLE = 1;
					priv->pshare->reach_tx_limit_cnt = 0;
					enable_em(priv);
				}
			}
			else {
				if (priv->pshare->em_tx_byte_cnt < EM_TP_LOW_BOUND)
					priv->pshare->reach_tx_limit_cnt++;
				else
					priv->pshare->reach_tx_limit_cnt = 0;

				if (!priv->pshare->txop_enlarge || priv->pshare->reach_tx_limit_cnt >= WAIT_TP_TIME) {
					GET_EM_SWQ_ENABLE = 0;
					priv->pshare->reach_tx_limit_cnt = 0;
					disable_em(priv);
				}
			}
		}
#endif

		priv->pshare->phw->LowestInitRate	= _NSS4_MCS9_RATE_;
	}

#ifdef USB_PKT_RATE_CTRL_SUPPORT
	usbPkt_timer_handler(priv);
#endif

	if (IS_ROOT_INTERFACE(priv) && GET_VXD_PRIV(priv) &&
			netif_running(GET_VXD_PRIV(priv)->dev)) {
		SMP_UNLOCK(flags);
		rtl8192cd_expire_timer((unsigned long)GET_VXD_PRIV(priv));
		SMP_LOCK(flags);
	}


	if((GET_CHIP_VER(priv) != VERSION_8812E) && (GET_CHIP_VER(priv) != VERSION_8881A))
	if (IS_ROOT_INTERFACE(priv))
	{
		unsigned int tmp_d2c = RTL_R32(0xd2c);
		unsigned char intel_sta_connected = 0;

		if(is_intel_connected(priv))
			intel_sta_connected = 1;

//		SMP_UNLOCK(flags);
		if (priv->pmib->miscEntry.vap_enable) {
			for (i=0; i<RTL8192CD_NUM_VWLAN; i++) {
				if (IS_DRV_OPEN(priv->pvap_priv[i])) {
					if(is_intel_connected(priv->pvap_priv[i])) {
						intel_sta_connected = 1;
						break;
					}
				}
			}
		}
//		SMP_LOCK(flags);

		if(!intel_sta_connected)
		if((tmp_d2c & BIT(11)) == 0)
		{
			tmp_d2c = tmp_d2c | BIT(11);
			RTL_W32(0xd2c, tmp_d2c);
#if 0	//eric-8814 ??
			tmp_d2c = RTL_R32(0xd2c);
			if(tmp_d2c & BIT(11))
				printk("No Intel STA, BIT(11) of 0xd2c = %d, 0xd2c = 0x%x\n", 1, tmp_d2c);
			else
				printk("No Intel STA, BIT(11) of 0xd2c = %d, 0xd2c = 0x%x\n", 0, tmp_d2c);
#endif
		}
	}

	if (IS_ROOT_INTERFACE(priv)) {
		if (priv->pmib->miscEntry.vap_enable) {
			SMP_UNLOCK(flags);
			for (i=0; i<RTL8192CD_NUM_VWLAN; i++) {
				if (IS_DRV_OPEN(priv->pvap_priv[i])) {
					rtl8192cd_expire_timer((unsigned long)priv->pvap_priv[i]);

                    if(priv->pvap_priv[i]->pmib->miscEntry.func_off==0){
                        somevapopened=1;
                    }
                }
			}
			SMP_LOCK(flags);
		}
	}


#ifdef RTK_ATM
	if (IS_ROOT_INTERFACE(priv)){
		if((priv->pshare->rf_ft_var.atm_en && priv->pshare->atm_ttl_stanum>1)||
			priv->pshare->rf_ft_var.atm_swqf == 1){
			//calc packet burst num every 2sec
			if((priv->up_time % priv->pshare->rf_ft_var.atm_adj_bto) == 0)
				atm_set_burst_size(priv);
		}
		if(priv->pshare->rf_ft_var.atm_chk_txtime)
			atm_calc_txtime(priv);//count every 1sec
	}
#endif

    if (IS_ROOT_INTERFACE(priv))
    {
        if (priv->pmib->miscEntry.func_off) { //func_off=1

					if(somevapopened==0)// not any VAP be opened and func_off not be enabled
			   		RTL_W32(TBTT_PROHIBIT, (RTL_R32(TBTT_PROHIBIT)&0xfff000ff) | 0x100);/*minimum prohibit time[19:8] to 1*/
					else
			   		RTL_W32(TBTT_PROHIBIT, (RTL_R32(TBTT_PROHIBIT)&0xfff000ff) | 0x1df00);

					if (!priv->func_off_already){
						if(!IS_HAL_CHIP(priv)){
							RTL_W8(REG_MBSSID_CTRL, RTL_R8(REG_MBSSID_CTRL) & ~(BIT(0)));
			        priv->func_off_already = 1;
						}else{
							if(somevapopened==0)
								priv->func_off_already = 1;
						}
					}

        } else {// func_off=0

            if (priv->func_off_already) {
                RTL_W8(REG_MBSSID_CTRL, RTL_R8(REG_MBSSID_CTRL) | BIT(0));
                priv->func_off_already = 0;

							if(somevapopened==0)
								RTL_W32(TBTT_PROHIBIT, (RTL_R32(TBTT_PROHIBIT)&0xfff000ff) | 0x40000);
							else
								RTL_W32(TBTT_PROHIBIT, (RTL_R32(TBTT_PROHIBIT)&0xfff000ff) | 0x1df00);
            }
        }
    }
    else
    if(IS_VAP_INTERFACE(priv))
    {
		if (priv->pmib->miscEntry.func_off) {
			if(!IS_HAL_CHIP(priv))
			if (!priv->func_off_already) {
				RTL_W8(REG_MBSSID_CTRL, RTL_R8(REG_MBSSID_CTRL) & ~(1 << priv->vap_init_seq));
				priv->func_off_already = 1;
			}
		}
		else {
			if (priv->func_off_already) {
				RTL_W8(REG_MBSSID_CTRL, RTL_R8(REG_MBSSID_CTRL) | (1 << priv->vap_init_seq));
				priv->func_off_already = 0;
			}
		}
	}

	if (IS_ROOT_INTERFACE(priv))
	{
#ifdef MP_TEST
		if (!((OPMODE & WIFI_MP_STATE) || priv->pshare->rf_ft_var.mp_specific))
#endif
		{
#ifdef RSSI_MIN_ADV_SEL
			select_rssi_min_from_data(priv);
#endif

#ifdef INTERFERENCE_CONTROL
			if (priv->pshare->rf_ft_var.rssi_dump && (priv->assoc_num == 0)) {
					panic_printk("(FA %x,%x ", RTL_R8(0xc50), RTL_R8(0xc58));

				panic_printk("%d, %d)\n", priv->pshare->ofdm_FA_total_cnt, priv->pshare->cck_FA_cnt);
			}
#endif

#if (BEAMFORMING_SUPPORT == 1)
	if (((priv->up_time % 3) == 0) &&
		(priv->pmib->dot11RFEntry.txbf == 1) &&
		(priv->pmib->dot11RFEntry.txbfer == 1) &&
		priv->pshare->WlanSupportAbility & WLAN_BEAMFORMING_SUPPORT)
	{
		if (GET_CHIP_VER(priv) == VERSION_8822B)
			DynamicSelectTxBFSTA(priv);
		else
			DynamicSelect2STA(priv);
	}
#endif
		if(!IS_OUTSRC_CHIP(priv))
		{
			if ( ((OPMODE & WIFI_AP_STATE)?(!(priv->assoc_num)):(!(OPMODE & WIFI_ASOC_STATE)))
				&& !(GET_VXD_PRIV(priv)->pmib->dot11OperationEntry.opmode & WIFI_ASOC_STATE)
			)
				priv->pshare->rf_ft_var.bLinked = FALSE;
			else
				priv->pshare->rf_ft_var.bLinked = TRUE;

			rtl8192cd_CheckAdaptivity(priv);
		}

#ifdef _OUTSRC_COEXIST
			if(IS_OUTSRC_CHIP(priv))
#endif
			{
				int idx = 0, link=0;
				struct stat_info* pEntry = findNextSTA(priv, &idx);
				while(pEntry) {
					if(pEntry && pEntry->expire_to) {
						link=1;
						break;
					}
					pEntry = findNextSTA(priv, &idx);
				};
				ODM_CmnInfoUpdate(ODMPTR, ODM_CMNINFO_LINK, link );
				ODM_CmnInfoUpdate(ODMPTR, ODM_CMNINFO_RSSI_MIN, priv->pshare->rssi_min);
				ODM_CmnInfoUpdate(ODMPTR, ODM_CMNINFO_RSSI_MIN_BY_PATH, priv->pshare->mimorssi_min);
#if (BEAMFORMING_SUPPORT == 1)
				Beamforming_CSIRate(priv);
#endif
				link = IS_DRV_OPEN(GET_VXD_PRIV(priv))
					&& (GET_VXD_PRIV(priv)->pmib->dot11OperationEntry.opmode & WIFI_ASOC_STATE);
				ODM_CmnInfoUpdate(ODMPTR, ODM_CMNINFO_VXD_LINK, link);
				if(priv->pshare->rf_ft_var.dig_enable && priv->pshare->DIG_on)
					ODMPTR->DM_DigTable.bStopDIG = FALSE;
				else
					ODMPTR->DM_DigTable.bStopDIG = TRUE;
				if(priv->pshare->rf_ft_var.adaptivity_enable)
					ODM_CmnInfoUpdate(ODMPTR, ODM_CMNINFO_ABILITY, ODMPTR->SupportAbility | ODM_BB_ADAPTIVITY);
				else
					ODM_CmnInfoUpdate(ODMPTR, ODM_CMNINFO_ABILITY, ODMPTR->SupportAbility & (~ ODM_BB_ADAPTIVITY));

				ODM_DMWatchdog(ODMPTR);

#if defined(CROSSBAND_REPEATER) || defined(RTK_MESH_METRIC_REFINE)
				priv->envinfo.info_record[priv->envinfo.index].noise_value = ODMPTR->NoisyDecision;
#endif

				//if(GET_CHIP_VER(priv) != VERSION_8822B) //eric-8822 ?? No call IotEngine
				IotEngine(priv);
			}

#if defined(CROSSBAND_REPEATER) || defined(RTK_MESH_METRIC_REFINE)

	        priv->envinfo.rssi_metric = 0;
		priv->envinfo.cu_metric = 0;
		priv->envinfo.noise_metric = 0;

		index = priv->envinfo.index;

		priv->envinfo.index++;
		if(priv->envinfo.index >= RECORDING_DURATION)//reset index of circular buffer
			priv->envinfo.index = 0;

		for(i=0; i<RECORDING_DURATION; i++){

			if(priv->envinfo.info_record[index].noise_value){
				(i > 11 )?((i>21)?(priv->envinfo.noise_metric += 6):(priv->envinfo.noise_metric += 4)):(priv->envinfo.noise_metric += 1);
			}
			priv->envinfo.cu_metric += priv->envinfo.info_record[index].cu_value;
					priv->envinfo.rssi_metric += priv->envinfo.info_record[index].rssi_value;

			if(index >= (RECORDING_DURATION-1))
				index = 0;
			else
				index++;
		}

		priv->envinfo.cu_metric = (priv->envinfo.cu_metric / RECORDING_DURATION);
		priv->envinfo.rssi_metric = (priv->envinfo.rssi_metric / RECORDING_DURATION);
#endif

            if (IS_HAL_CHIP(priv)) {
                //HAL timer callback entry point
                GET_HAL_INTERFACE(priv)->Timer1SecHandler(priv);
            }

#if !defined(USE_OUT_SRC) || defined(_OUTSRC_COEXIST)
#ifdef _OUTSRC_COEXIST
if(!IS_OUTSRC_CHIP(priv))
#endif
{
{
			if (priv->up_time % 2) {
#ifdef CONFIG_RTL_NEW_AUTOCH
				if( priv->auto_channel ==0 || priv->auto_channel ==2 )
#endif
				FA_statistic(priv);

				if ((priv->up_time > 5) && priv->pshare->rf_ft_var.dig_enable)
					DIG_process(priv);
			}


			if (priv->pshare->rf_ft_var.auto_rts_rate) {
				priv->pshare->phw->RTSInitRate_Candidate =
					get_rate_index_from_ieee_value(find_rts_rate(priv, priv->pshare->phw->LowestInitRate,
					priv->pmib->dot11ErpInfo.protection));

				if (priv->pshare->phw->RTSInitRate_Candidate != priv->pshare->phw->RTSInitRate) {
					priv->pshare->phw->RTSInitRate = priv->pshare->phw->RTSInitRate_Candidate;
					RTL_W8(INIRTS_RATE_SEL, priv->pshare->phw->RTSInitRate);
				}
			}

}

#ifdef SW_ANT_SWITCH
			if ((SW_DIV_ENABLE)  && (priv->up_time % 4==1))
				dm_SW_AntennaSwitch(priv, SWAW_STEP_PEAK);
#endif
}
#endif

#ifdef CONFIG_1RCCA_RF_POWER_SAVING
			if (priv->pshare->rf_ft_var.one_path_cca_ps == 2) {
				if (priv->pshare->rssi_min == 0xff) {	// No Link
					one_path_cca_power_save(priv, 1);
				} else {
					if (priv->pshare->rssi_min > priv->pshare->rf_ft_var.one_path_cca_ps_rssi_thd)
						one_path_cca_power_save(priv, 1);
					else
						one_path_cca_power_save(priv, 0);
				}
			}
#endif // CONFIG_1RCCA_RF_POWER_SAVING
		}

		if (priv->pmib->dot11RFEntry.ther && priv->pshare->rf_ft_var.tpt_period)
			TXPowerTracking(priv);

#ifdef CONFIG_RF_DPK_SETTING_SUPPORT
		if (priv->pmib->dot11RFEntry.ther && priv->pshare->rf_ft_var.dpk_period)
			TX_DPK_Tracking(priv);
#endif


#ifdef CAM_SWAP
		if ((OPMODE & WIFI_AP_STATE) &&
			 (priv->pshare->rf_ft_var.cam_rotation)){
			if (priv->up_time % priv->pshare->rf_ft_var.cam_rotation == 0){

				int sta_use_sw_encrypt = 0;

				sta_use_sw_encrypt = get_sw_encrypt_sta_num(priv);

				printk("%s %d sta_use_sw_encrypt=%d \n", __FUNCTION__, __LINE__, sta_use_sw_encrypt);

				if(sta_use_sw_encrypt){
					cal_sta_traffic(priv, sta_use_sw_encrypt);
					rotate_sta_cam(priv, sta_use_sw_encrypt);
				}
			}
		}
#endif


#if defined(WIFI_11N_2040_COEXIST_EXT)
	if((OPMODE & WIFI_AP_STATE) && priv->pshare->rf_ft_var.bws_enable & 0x1)
		checkBandwidth(priv);
#endif

#if !defined(USE_OUT_SRC) || defined(_OUTSRC_COEXIST)
#ifdef _OUTSRC_COEXIST
if(!IS_OUTSRC_CHIP(priv))
#endif
{

#ifdef HIGH_POWER_EXT_PA
		if(CHIP_VER_92X_SERIES(priv)) {
			if((priv->pshare->rf_ft_var.use_ext_pa) && (priv->pshare->rf_ft_var.tx_pwr_ctrl))
				tx_power_control(priv);
		}
#endif
		IOT_engine(priv);
		rxBB_dm(priv);

}
#endif

#ifdef _OUTSRC_COEXIST
		if(IS_OUTSRC_CHIP(priv))
#endif
		{
			if ((priv->up_time % 3) == 1) {
				if (priv->pshare->rssi_min != 0xff) {
					if (priv->pshare->rf_ft_var.nbi_filter_enable)
						check_NBI_by_rssi(priv, priv->pshare->rssi_min);
				}
			}
		}


		if ( priv->pshare->rf_ft_var.adaptivity_enable == 3 && !priv->pshare->rf_ft_var.isCleanEnvironment )
			redo_acs_for_adaptivity(priv);

#if (CONFIG_WLAN_HAL_8197F)
		if (GET_CHIP_VER(priv)== VERSION_8197F && priv->pshare->rf_ft_var.dym_soml) {
			if ((priv->pshare->rssi_min < priv->pshare->rf_ft_var.dym_soml_thd) && (RTL_R8(0x998) & BIT6))
				PHY_SetBBReg(priv,0x998,BIT6,0);
			else if ((priv->pshare->rssi_min > priv->pshare->rf_ft_var.dym_soml_thd+4) && !(RTL_R8(0x998) & BIT6))
				PHY_SetBBReg(priv,0x998,BIT6,1);
		}
#endif
	}


//remove this because some sta will idle over 1 sec, than the data path will disconnect for a while
#if 0
	if(IS_HAL_CHIP(priv))
	{
		struct list_head	*plist, *phead;
		struct stat_info	*pstat;
#ifndef AP_PS_Offlaod
		phead = &priv->sleep_list;

		SMP_LOCK_SLEEP_LIST(local_flags);

		plist = phead->next;
		while(plist != phead)
		{
			pstat = list_entry(plist, struct stat_info, sleep_list);
			plist = plist->next;

			if (pstat->txpause_flag && (TSF_DIFF(jiffies, pstat->txpause_time) > RTL_SECONDS_TO_JIFFIES(1))) {
				DEBUG_WARN("%s %d expire timeout, set MACID 0 AID = %x \n",__FUNCTION__,__LINE__,REMAP_AID(pstat));
				GET_HAL_INTERFACE(priv)->UpdateHalMSRRPTHandler(priv, pstat, DECREASE);
				pstat->txpdrop_flag = 1;
				pstat->drop_expire++;
				GET_HAL_INTERFACE(priv)->SetMACIDSleepHandler(priv, 0, REMAP_AID(pstat));
				pstat->txpause_flag = 0;
				if(priv->pshare->paused_sta_num)
					priv->pshare->paused_sta_num--;
			}
		}

		SMP_UNLOCK_SLEEP_LIST(local_flags);
#endif
	} else
	{}
#endif

#if defined(TXREPORT)
	if ( CHIP_VER_92X_SERIES(priv) ||(GET_CHIP_VER(priv)==VERSION_8812E)
	) {


		if (!IS_TEST_CHIP(priv))
		#ifdef MP_TEST
		if (!((OPMODE & WIFI_MP_STATE) || priv->pshare->rf_ft_var.mp_specific))
		#endif
		if (IS_ROOT_INTERFACE(priv))
		{

			if (!(priv->up_time%priv->pmib->staDetectInfo.txRprDetectPeriod) && (priv->pshare->sta_query_idx==-1)) {
				priv->pshare->sta_query_idx = 0;
				if(CHIP_VER_92X_SERIES(priv)){
					requestTxReport(priv);
				}
			}
		}
	}
#endif

#ifdef TXREPORT
      	if(IS_HAL_CHIP(priv)) {
#ifdef MP_TEST
			if (!((OPMODE & WIFI_MP_STATE) || priv->pshare->rf_ft_var.mp_specific))
#endif
			if (IS_ROOT_INTERFACE(priv))
			if ( priv->pshare->sta_query_idx == -1) {
				priv->pshare->sta_query_idx = 0;
#if (MU_BEAMFORMING_SUPPORT == 1)
				if(priv->pmib->dot11RFEntry.txbf_mu)
					requestTxReport88XX_MU(priv);
				else
#endif
				requestTxReport88XX(priv);
			}
        }
#endif

#if defined(CONFIG_IEEE80211W) || defined(CONFIG_IEEE80211V)
	issue_actionFrame(priv);
#endif
#ifdef CONFIG_IEEE80211V
	if(WNM_ENABLE) {
		BssTrans_ExpiredTimer(priv);
		BssTrans_DiassocTimer(priv);
	#ifdef CONFIG_IEEE80211V_CLI
		BssTrans_ValidatePrefListTimer(priv);
		BssTrans_TerminationTimer(priv);
	#endif
	}
#endif

#ifdef SW_TX_QUEUE
#ifdef HS2_SUPPORT
#ifdef HS2_CLIENT_TEST
	//printk("swq_dbg=%d\n",priv->pshare->rf_ft_var.swq_dbg);
	if (priv->pshare->rf_ft_var.swq_dbg != 0)
	{
		DOT11_HS2_GAS_REQ gas_req;
		unsigned short query_id;

	//if (priv->pshare->rf_ft_var.swq_dbg == 40 && IS_ROOT_INTERFACE(priv))

		if (priv->pshare->rf_ft_var.swq_dbg == 40  && IS_ROOT_INTERFACE(priv))
		{
			issue_WNM_Notify(priv);
			priv->pshare->rf_ft_var.swq_dbg = 0;
			goto hs_break;

		}
		if (priv->pshare->rf_ft_var.swq_dbg == 41  && IS_ROOT_INTERFACE(priv))
		{
			issue_disassoc(priv,priv->pmib->hs2Entry.sta_mac,_RSON_AUTH_NO_LONGER_VALID_ );
			priv->pshare->rf_ft_var.swq_dbg = 0;
			goto hs_break;

		}
		if (priv->pshare->rf_ft_var.swq_dbg == 44  && IS_ROOT_INTERFACE(priv))
		{
			issue_deauth(priv,priv->pmib->hs2Entry.sta_mac,_RSON_AUTH_NO_LONGER_VALID_ );
			priv->pshare->rf_ft_var.swq_dbg = 0;
			goto hs_break;

		}
		if (priv->pshare->rf_ft_var.swq_dbg == 45  && IS_ROOT_INTERFACE(priv))
		{
			unsigned char zeromac[6]={0x00,0x00,0x00,0x00,0x00,0x00};
			//int issue_BSS_TxMgmt_req(struct rtl8192cd_priv *priv, unsigned char *da);
			if (memcmp(priv->pmib->hs2Entry.sta_mac, zeromac, 6))
			{
				DOT11_HS2_TSM_REQ tsmreq;
				HS2_DEBUG_INFO("send bss tx mgmt Req to STA:[%02x:%02x:%02x:%02x:%02x:%02x]\n",
					priv->pmib->hs2Entry.sta_mac[0], priv->pmib->hs2Entry.sta_mac[1],
					priv->pmib->hs2Entry.sta_mac[2], priv->pmib->hs2Entry.sta_mac[3],
					priv->pmib->hs2Entry.sta_mac[4], priv->pmib->hs2Entry.sta_mac[5]);

				memcpy(tsmreq.MACAddr, priv->pmib->hs2Entry.sta_mac, 6);
				tsmreq.Req_mode = 16; // ESS Disassoc Imminent
				tsmreq.term_len = 0;

				tsmreq.Dialog_token = 5;
				tsmreq.Disassoc_timer = 100;
				tsmreq.Validity_intval = 0;
				tsmreq.url_len = strlen(priv->pmib->hs2Entry.SessionInfoURL);
				memcpy(tsmreq.Session_url, priv->pmib->hs2Entry.SessionInfoURL, tsmreq.url_len);
				tsmreq.list_len = 0;

				issue_BSS_TSM_req(priv, &tsmreq);

               	if (timer_pending(&priv->disassoc_timer))
            		del_timer_sync(&priv->disassoc_timer);

				mod_timer(&priv->disassoc_timer, jiffies + tsmreq.Disassoc_timer * RTL_MILISECONDS_TO_JIFFIES(priv->pmib->dot11StationConfigEntry.dot11BeaconPeriod));
			}
			priv->pshare->rf_ft_var.swq_dbg = 0;
		}
		if (priv->pshare->rf_ft_var.swq_dbg == 46  && IS_ROOT_INTERFACE(priv))
		{
			issue_WNM_Deauth_Req(priv,priv->pmib->hs2Entry.sta_mac, 0,1, "https://deauth_notification.r2-testbed.wi-fi.org");
			priv->pshare->rf_ft_var.swq_dbg = 0;
			goto hs_break;

		}

		if (priv->pshare->rf_ft_var.swq_dbg == 47  && IS_ROOT_INTERFACE(priv))
		{
			priv->pmib->hs2Entry.curQoSMap = 1;
			setQoSMapConf(priv);
			issue_QoS_MAP_Configure(priv, priv->pmib->hs2Entry.sta_mac, priv->pmib->hs2Entry.curQoSMap);
			priv->pshare->rf_ft_var.swq_dbg = 0;
			goto hs_break;

		}

		if (priv->pshare->rf_ft_var.swq_dbg == 48 && IS_ROOT_INTERFACE(priv))
		{
			issue_WNM_Deauth_Req(priv,priv->pmib->hs2Entry.sta_mac, 0,1, "");
			priv->pshare->rf_ft_var.swq_dbg = 0;
			goto hs_break;

		}
		if ((OPMODE & WIFI_ASOC_STATE) == 0)
		{
		    priv->pshare->CurrentChannelBW = HT_CHANNEL_WIDTH_20;
		    SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);
		    SwChnl(priv, priv->pmib->dot11RFEntry.dot11channel, priv->pshare->offset_2nd_chan);
		    delay_us(250);
		}

		if ((priv->pshare->rf_ft_var.swq_dbg >= 30) && (priv->pshare->rf_ft_var.swq_dbg <= 39))
		{
		    unsigned char tmpda[]={0xff,0xff,0xff,0xff,0xff,0xff};
		    issue_probereq(priv,"",0, tmpda);
		    goto hs_break;
		}
		if (priv->pshare->rf_ft_var.swq_dbg == 20)
		{
		    unsigned char list[]={0x03,0x01,0x30,0x03,0x01,0x50};
		    issue_BSS_TSM_query(priv, list, sizeof(list));
		    priv->pshare->rf_ft_var.swq_dbg = 0;
		    goto hs_break;
		}



		// query_id is identified by the issue_GASreq function
		// refer to Table 8-184
		//capability List  (Test Case 4.2 Proc. (a) in HS2.0 R2 Test Plan v.0.01)
		if (priv->pshare->rf_ft_var.swq_dbg == 3)
		    query_id = 257;
		//Roaming Consortium list (Test Case 4.4 Step2 in HS2.0 R2 Test Plan v.0.01)
		else if (priv->pshare->rf_ft_var.swq_dbg == 4)
		    query_id = 261;
		//venue name  (Test Case 4.4 Step8 in HS2.0 R2 Test Plan v.0.01)
		else if (priv->pshare->rf_ft_var.swq_dbg == 5)
		    query_id = 258;
		// Network Authentication Type Information  (Test Case 4.4 Step10 in HS2.0 R2 Test Plan v.0.01)
		else if (priv->pshare->rf_ft_var.swq_dbg == 6)
		    query_id = 260;
		//IP Address Type Availability Information  (Test Case 4.4 Step12 in HS2.0 R2 Test Plan v.0.01)
		else if (priv->pshare->rf_ft_var.swq_dbg == 7)
		    query_id = 262;
		//combine  nai_list+3gpp+domain_list  (Test Case 4.4 Step4 in HS2.0 R2 Test Plan v.0.01)
		else if (priv->pshare->rf_ft_var.swq_dbg == 8)
		    query_id = 10000;
		//HS Capability list and Operator Friendly Name  (Test Case 4.4 Step6 in HS2.0 R2 Test Plan v.0.01)
		else if (priv->pshare->rf_ft_var.swq_dbg == 9)
		    query_id = 501;
		//HS WAN Metrics and Connection Lists  (Test Case 4.4 Step14 in HS2.0 R2 Test Plan v.0.01)
		else if (priv->pshare->rf_ft_var.swq_dbg == 10)
		    query_id = 502;
		//hs wan+hs no support type
		else if (priv->pshare->rf_ft_var.swq_dbg == 18)
		    query_id = 505;
		//hs home realm query, cnt=0
		else if (priv->pshare->rf_ft_var.swq_dbg == 19)
		    query_id = 506;
		//hs operating class
		else if (priv->pshare->rf_ft_var.swq_dbg == 21)
		    query_id = 507;
		//hs home realm query  (Test Case 4.4 Step16 in HS2.0 R2 Test Plan v.0.01)
		else if (priv->pshare->rf_ft_var.swq_dbg == 11)
		    query_id = 503;
		//hs  2 home realm query
		else if (priv->pshare->rf_ft_var.swq_dbg == 17)
		    query_id = 504;
		//advt protocol error
		else if (priv->pshare->rf_ft_var.swq_dbg == 12)
		    query_id = 262;
		// Query element (qid = roaming list) + HS2 WAN Metrics element  + HS2 Operating Class Indication element
		else if (priv->pshare->rf_ft_var.swq_dbg == 13)
		    query_id = 270;
		//roaming list    (qid = roaming list) + HS2 element (subtype = reserved) + HS Query List
		else if (priv->pshare->rf_ft_var.swq_dbg == 14)
		    query_id = 271;
		//nai list
		else if (priv->pshare->rf_ft_var.swq_dbg == 15)
		    query_id = 263;
		//capa+comback request
		else if (priv->pshare->rf_ft_var.swq_dbg == 16)
		{
		    query_id = 257;
		    testflg = 1;
		}
		else if (priv->pshare->rf_ft_var.swq_dbg == 22) // for Test Case 4.3 Step 4
		    query_id = 272;
		else if (priv->pshare->rf_ft_var.swq_dbg == 23) // for Test Case 4.3 Step 6
		    query_id = 0;
		else if (priv->pshare->rf_ft_var.swq_dbg == 42) { // ICON Request
		    query_id = 508;
		}
		else if (priv->pshare->rf_ft_var.swq_dbg == 43) { // Online Sign-Up Providers list
		    query_id = 509;
		}
		else
			goto hs_break;

		gas_req.Dialog_token = 5;
		issue_GASreq(priv, &gas_req, query_id);
		HS2_DEBUG_INFO("send gas req\n");
		priv->pshare->rf_ft_var.swq_dbg = 0;
	}
hs_break:
#endif
#endif

#ifdef SW_TX_QUEUE_SMALL_PACKET_CHECK
// add for small udp packet(88B) test with veriwave tool (let sw tx queue start quickly after first round test)
// the interval between every round test is about 10sec
#if 1
	if ((priv->assoc_num > 1) && (AMPDU_ENABLE) && (priv->pshare->swq_txmac_chg >= priv->pshare->rf_ft_var.swq_en_highthd))
		priv->pshare->swq_boost_delay = 0;
	else
		priv->pshare->swq_boost_delay++;

	if (priv->pshare->swq_boost_delay >= 10)
		priv->pshare->swq_boost_delay = 10;
#else
	priv->pshare->swq_boost_delay = 0;
#endif
#endif

	//for debug
	//if (priv->pshare->rf_ft_var.swq_dbg)
	//	printk("sw cnt:%d:%d,0x%x\n", priv->swq_txmac_chg,priv->swq_en, RTL_R32(EDCA_BE_PARA));

	if (IS_ROOT_INTERFACE(priv))
        priv->pshare->swq_txmac_chg = 0;
#endif

#ifdef PERIODIC_AUTO_CHANNEL
	if (priv->auto_channel_backup && IS_ROOT_INTERFACE(priv) && (OPMODE & WIFI_AP_STATE))
		rtl8192cd_auto_channel_timer(priv);
#endif


#if defined(CLIENT_MODE) && defined(WIFI_11N_2040_COEXIST)
	if ((OPMODE & (WIFI_STATION_STATE | WIFI_ASOC_STATE)) == (WIFI_STATION_STATE | WIFI_ASOC_STATE))
		start_clnt_coexist_scan(priv);
#endif

#ifdef RADIUS_ACCOUNTING
	//brian add for accounting
	if(ACCT_FUN)
	{
		cal_statistics_acct(priv);
		expire_sta_for_radiusacct(priv);
	}
#endif

	if (IS_ROOT_INTERFACE(priv))
	{
		switch(priv->pshare->intel_active_sta) {
		case 0:
		case 1:
			priv->pshare->intel_rty_lmt =  0x30; /* 48 times */
			break;
		case 2:
			priv->pshare->intel_rty_lmt =  0x18; /* 24 times */
			break;
		default:
			priv->pshare->intel_rty_lmt =  0; /* use system default */
			break;
		}

		priv->pshare->intel_active_sta = 0;

//#if defined(SMART_REPEATER_MODE) && !defined(RTK_NL80211)
#if 0//removed to prevent system hang-up after repeater follow remote AP to switch channel
		if (priv->pshare->switch_chan_rp &&
				((priv->pmib->dot11RFEntry.dot11channel != priv->pshare->switch_chan_rp) ||
				  (priv->pmib->dot11nConfigEntry.dot11n2ndChOffset != priv->pshare->switch_2ndchoff_rp) ||
				  (priv->pmib->dot11nConfigEntry.dot11nUse40M != priv->pshare->band_width_rp)) ) {
			DEBUG_INFO("swtich chan=%d\n",  priv->pshare->switch_chan_rp);
			switch_chan_to_vxd(priv);
			priv->pshare->switch_chan_rp = 0;
		}
#endif
	}

    /*for delay wsc client mode do Scan; special on repeater mode
	   if  client mode do Scan too frequent and continous
       will cause the client(that connected with repeater's AP interface) disconnect*/
#if defined( WIFI_SIMPLE_CONFIG	) && defined(UNIVERSAL_REPEATER)
    if(IS_VXD_INTERFACE(priv) && (OPMODE & WIFI_STATION_STATE)){
	    if(priv->wsc_ss_delay){
    	    //STADEBUG("wsc_ss_delay remain[%d]\n",priv->wsc_ss_delay);
	        priv->wsc_ss_delay--;
    	}
	}
#endif

#ifdef BT_COEXIST
	if(GET_CHIP_VER(priv) == VERSION_8192E && priv->pshare->rf_ft_var.btc == 1
		&& IS_ROOT_INTERFACE(priv)
			){
		/*
		*	BT coexist dynamic mechanism
		*/
		if(priv->up_time % 1 == 0){
			bt_coex_dm(priv);
		}
		/*
		* Query BT info
		*/
		if(priv->up_time % 1 == 0){
			u1Byte		H2CCommand[1] = {0};
			H2CCommand[0] |= BIT0;
			FillH2CCmd88XX(priv, H2C_88XX_BT_INFO, 1, H2CCommand);
		}
		if(priv->pshare->rf_ft_var.bt_dump >= 2){
			panic_printk("--------------------------------------------------------------------\n");
			panic_printk("\n0x778:%8x\n",PHY_QueryBBReg(priv, 0x778, bMaskDWord));
			panic_printk("0x92c:%8x,",PHY_QueryBBReg(priv, 0x92c, bMaskDWord));
			panic_printk("   0x930:%8x\n",PHY_QueryBBReg(priv, 0x930, bMaskDWord));
			panic_printk("0x40 :%8x,",PHY_QueryBBReg(priv, 0x40,	bMaskDWord));
			panic_printk("   0x4f :%8x\n",PHY_QueryBBReg(priv, 0x4c,  0xff000000));
			panic_printk("0x550:%8x,",PHY_QueryBBReg(priv, 0x550, bMaskDWord));
			panic_printk("   0x522:%8x\n",PHY_QueryBBReg(priv, 0x520, 0x00ff0000));
			panic_printk("0xc50:%8x,",PHY_QueryBBReg(priv, 0xc50, bMaskDWord));
			panic_printk("   0xc58:%8x\n",PHY_QueryBBReg(priv, 0xc58, bMaskDWord));
			panic_printk("0x6c0:%8x,",PHY_QueryBBReg(priv, 0x6c0, bMaskDWord));
			panic_printk("   0x6c4:%8x,",PHY_QueryBBReg(priv, 0x6c4, bMaskDWord));
			panic_printk("   0x6c8:%8x,",PHY_QueryBBReg(priv, 0x6c8, bMaskDWord));
			panic_printk("   0x6cc:%8x\n",PHY_QueryBBReg(priv, 0x6cc, bMaskDWord));
			panic_printk("0x770: hp rx:%d,",PHY_QueryBBReg(priv, 0x770, bMaskHWord));
			panic_printk("   tx:%d\n",PHY_QueryBBReg(priv, 0x770, bMaskLWord));
			panic_printk("0x774: lp rx:%d,",PHY_QueryBBReg(priv, 0x774, bMaskHWord));
			panic_printk("	 tx:%d\n",PHY_QueryBBReg(priv, 0x774, bMaskLWord));
			panic_printk("--------------------------------------------------------------------\n");
		}
		PHY_SetBBReg(priv, 0x76c, 0x00ff0000, 0x0c); /* reset BT counter*/
	}
#endif


#ifdef COCHANNEL_RTS
	if (priv->cochannel_to)
		priv->cochannel_to --;
#endif

#ifdef THERMAL_CONTROL
	if(priv->up_time % priv->pshare->rf_ft_var.ther_dm_period == 0)
		thermal_control_dm(priv);
#endif

#if defined(CH_LOAD_CAL) || defined(HS2_SUPPORT) || defined(DOT11K)
	if(IS_ROOT_INTERFACE(priv)
		&& priv->pmib->dot11StationConfigEntry.cu_enable){
		channle_loading_measurement(priv);
	}
#endif

	RESTORE_INT(flags);
	SMP_UNLOCK(flags);
}


/*
 *	@brief	System 1 sec timer
 *
 *	@param	task_priv: priv
 *
 *	@retval	void
 */
#ifdef CONFIG_RTL_PCIE_LINK_PROTECTION
	extern void  check_pcie_link_status(void);
#endif
// #define CHECK_CRYPTO
#ifdef THERMAL_PROTECTION
#define DBG_THERMAL_PRO
#define MAX_THERMAL_MARK 63
#define MAX_HIT_COUNTER 10
#define DECREASE_POWER_ACTION 0x8814
#define RETRY_COUNT 60*10
#define NO_ACTION 0
extern int degrade_power(struct rtl8192cd_priv *priv, unsigned char action, unsigned char level);

int rtl8192cd_thermal_value(struct rtl8192cd_priv *priv)
{

	PHY_SetRFReg(priv, RF92CD_PATH_A, 0x42, BIT(17), 0x1);

	 return 0;



}
int rtl8192cd_thermal_value_s2(struct rtl8192cd_priv *priv)
{
	int ther=0;

	 ther = PHY_QueryRFReg(priv, RF92CD_PATH_A, 0x42, 0xfc00, 1);
	 return ther;



}

void rtl8192cd_thermal_protection(struct rtl8192cd_priv *priv)
{
	int ther_val=0;
	if (!(priv->drv_state & DRV_STATE_OPEN))
		return;

	if(priv->read_flag !=0x1234)
	{			rtl8192cd_thermal_value(priv);
				priv->read_flag =0x1234;
				return;
	}
	else
	{
		ther_val= rtl8192cd_thermal_value_s2(priv);
		priv->read_flag =0;
	}
	if(ther_val >= MAX_THERMAL_MARK)
	{
		if(priv->thermal_fireoff != DECREASE_POWER_ACTION)  //stage 1
		{
				priv->thermal_hit_counter++;
				if(priv->thermal_hit_counter > MAX_HIT_COUNTER)
				{
					degrade_power(priv,2,4); //decrease 12dB
					priv->thermal_fireoff=DECREASE_POWER_ACTION;
					#ifdef DBG_THERMAL_PRO
					panic_printk("%s TP:3=>Take Action to Decrease Power\r\n",priv->dev->name);
					#endif

				}
				else
				{
					#ifdef DBG_THERMAL_PRO
					panic_printk("%s TP:2->%d\r\n",priv->dev->name,ther_val);
					#endif
					return;
				}
		}
		else //stage 2
		{
			if(priv->thermal_hit_counter > (MAX_HIT_COUNTER+RETRY_COUNT))
			{
				//Disable The WLAN foreever;
				panic_printk("%s TP:5=>Disable The WLAN foreever\r\n",priv->dev->name);
				rtl8192cd_close(priv->dev);


			}
		}

	}
	else
	{
		if(priv->thermal_fireoff != DECREASE_POWER_ACTION) //stage 1
		{
			priv->thermal_hit_counter=0;
			#ifdef DBG_THERMAL_PRO
			panic_printk("%s TP:0->%d\r\n",priv->dev->name,ther_val);
			#endif
			return;
		}
		else
		{
			priv->thermal_hit_counter--;
			if(priv->thermal_hit_counter ==1)
			{
				degrade_power(priv,0,0); //reset to original Tx power index
				priv->thermal_fireoff = NO_ACTION;
				#ifdef DBG_THERMAL_PRO
				panic_printk("%s TP:4 ==> Reset Tx power\r\n",priv->dev->name);
				#endif
			}

		}
	}



}
#endif
void rtl8192cd_1sec_timer(unsigned long task_priv)
{
	struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)task_priv;
#ifdef CHECK_CRYPTO
	unsigned long	flags;
#endif

	if (!(priv->drv_state & DRV_STATE_OPEN))
		return;



	//Unlock TXPAUSE BCN for 8822B after timeout reached
	if(priv->pshare->rf_ft_var.lock5d1 && (GET_CHIP_VER(priv) == VERSION_8822B))
	if(((priv->up_time +1) >= priv->pshare->rf_ft_var.lock5d1) && (RTL_R8(0x5d1) != 0)){
		RTL_W8(0x5d1, RTL_R8(0x5d1) & ~STOP_BCN);
	}

#ifdef CONFIG_32K//tingchu
    if (priv->offload_32k_flag==1){
        printk("32k rrtl8192cd_1sec_timer return\n");
        goto expire_timer;
    }
#endif


	if (!IS_HAL_CHIP(priv))
	if (!(priv->up_time % 5))
	tx_stuck_fix(priv);

// 2009.09.08
#ifdef CHECK_HANGUP
#ifdef MP_TEST
	if (!((OPMODE & WIFI_MP_STATE) || priv->pshare->rf_ft_var.mp_specific))
#endif
		if (check_hangup(priv))
			return;
#endif

#ifdef CHECK_CRYPTO
    // TODO: Filen, Check code below for 8881A & 92E
	if(GET_CHIP_VER(priv) != VERSION_8192D)
		if((RTL_R32(0x6B8) & 0x3) == 0x3) {
			DEBUG_ERR("Cyrpto checked\n");
			SAVE_INT_AND_CLI(flags);
			RTL_W8(0x522, 0x0F);
			RTL_W8(0x6B8, 0xFF);
			RTL_W8(0x101,0x0);
			RTL_W8(0x21,0x35);
			delay_us(250);
			RTL_W8(0x101,0x02);
			RTL_W8(0x522,0x0);
			RESTORE_INT(flags);
		}
#endif

	// for Rx dynamic tasklet
	if (priv->pshare->rxInt_data_delta > priv->pmib->miscEntry.rxInt_thrd)
		priv->pshare->rxInt_useTsklt = TRUE;
	else
		priv->pshare->rxInt_useTsklt = FALSE;
	priv->pshare->rxInt_data_delta = 0;


	tasklet_schedule(&priv->pshare->oneSec_tasklet);

#if defined(PCIE_POWER_SAVING_TEST)|| defined(CONFIG_32K)
expire_timer:
#endif



#ifdef CONFIG_RTL_WLAN_DOS_FILTER
	if ((block_sta_time > 0) && (block_priv == (unsigned long)priv))
	{
		block_sta_time--;
	}
#endif
	////////////////////////////////////////////////////////////////////
	//patch for reboot issues
	if(GET_CHIP_VER(priv) == VERSION_8812E)
	{
		RTL_W32(0x1c,0x7FEF922);
		RTL_W32(0x1c,0x7FEF962);

	}
	////////////////////////////////////////////////////////////////////


#ifdef THERMAL_PROTECTION
 rtl8192cd_thermal_protection(priv);
#endif
#ifdef CONFIG_RTL_PCIE_LINK_PROTECTION
	check_pcie_link_status();
#endif
	mod_timer(&priv->expire_timer, jiffies + EXPIRE_TO);
}

void pwr_state(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
    unsigned long	flags;
    struct stat_info *pstat;
    unsigned char	*sa, *pframe;

#ifdef HW_DETEC_POWER_STATE
    unsigned char result,i;
#endif

    pframe = get_pframe(pfrinfo);
    sa = pfrinfo->sa;
    pstat = get_stainfo(priv, sa);

    if (pstat == (struct stat_info *)NULL)
        return;

    if (!(pstat->state & WIFI_ASOC_STATE))
        return;

    #ifdef HW_DETEC_POWER_STATE
    if (IS_SUPPORT_HW_DETEC_POWER_STATE(priv)) {
        if (RT_STATUS_SUCCESS == GET_HAL_INTERFACE(priv)->CheckHWMACIDResultHandler(priv,pfrinfo->macid,&result)) {
        if(HWMACID_RESULT_SUCCESS == result)    {
            // hw detect ps state change return
            return;
        }
    }
    #ifdef HW_DETEC_POWER_STATE_PATCH
        if(HWMACID_RESULT_NOT_READY == result){

            printk("%s %d HW_MACID_SEARCH_NOT_READY",__FUNCTION__,__LINE__);

            for(i=0;i<128;i++)
            {
                priv->pshare->HWPwrStateUpdate[pstat->aid]--;
            }

            if(priv->pshare->HWPwrStateUpdate[pstat->aid] > 0)
            {
                // hw already update return
                return;
            }
        }
    #endif //#ifdef HW_DETEC_POWER_STATE_PATCH
    }
    #endif //    #ifdef HW_DETEC_POWER_STATE


    if (GetPwrMgt(pframe))
    {

        if ((pstat->state & WIFI_SLEEP_STATE) == 0) {
            pstat->state |= WIFI_SLEEP_STATE;
            if(CHIP_VER_92X_SERIES(priv))
            {
                if (pstat == priv->pshare->highTP_found_pstat) {
                    if (priv->pshare->txpause_pstat == NULL) {
                        RTL_W8(TXPAUSE, RTL_R8(TXPAUSE)|STOP_BE|STOP_BK);


                        priv->pshare->txpause_pstat = pstat;
                        priv->pshare->txpause_time = jiffies;

                    }
                }
            }

            if (pstat->sta_in_firmware == 1)
            {


                if(IS_HAL_CHIP(priv))
                {
#ifdef AP_PS_Offlaod
                    GET_HAL_INTERFACE(priv)->APPSOffloadMACIDPauseHandler(priv, pstat->aid, 1);
                    priv->pshare->HWPwroldState[pstat->aid>>5] = priv->pshare->HWPwroldState[pstat->aid>>5] | BIT(pstat->aid&0x1F); //manual update pwr state
#else
                    DEBUG_WARN("%s %d client into ps, set MACID sleep AID = %x \n",__FUNCTION__,__LINE__,REMAP_AID(pstat));
                    DEBUG_WARN("Pwr_state jiffies = %lu \n",jiffies);
                    GET_HAL_INTERFACE(priv)->SetMACIDSleepHandler(priv, 1, REMAP_AID(pstat));
					if(pstat->txpause_flag ==0){
						priv->pshare->paused_sta_num++;
						pstat->cnt_sleep ++;
					}
                    pstat->txpause_time = jiffies;
#endif
                    pstat->txpause_flag = 1;

#ifdef MULTI_STA_REFINE
					if( priv->pshare->paused_sta_num > (MAXPAUSEDSTA<<((priv->ext_stats.tx_avarage>>19)?0:1))) {
						{
							struct list_head	*phead = &(priv->sleep_list), *plist=phead;
							struct stat_info *pstatd = pstat, *pstat2;
							while ((plist = plist->next) != phead)
							{
								pstat2 = list_entry(plist, struct stat_info, sleep_list);
								if(pstat2->txpause_flag && (pstat2->dropPktCurr) < (pstatd->dropPktCurr))
									pstatd = pstat2;
								if (plist == plist->next)
									break;
							}
							if(pstatd) {
								GET_HAL_INTERFACE(priv)->UpdateHalMSRRPTHandler(priv, pstatd, DECREASE);
								pstat->txpdrop_flag = 1;
								GET_HAL_INTERFACE(priv)->SetMACIDSleepHandler(priv, 0, REMAP_AID(pstatd));
								pstatd->txpause_flag = 0;
								if(priv->pshare->paused_sta_num)
									priv->pshare->paused_sta_num--;
								priv->pshare->unlock_counter1++;
								pstatd->dropPktCurr ++;
							}
						}
				}
#endif

                } else
                {
                }
            }
        }

        SAVE_INT_AND_CLI(flags);
        if (wakeup_list_del(priv, pstat)) {
            DEBUG_INFO("Del fr wakeup_list %02X%02X%02X%02X%02X%02X\n", sa[0],sa[1],sa[2],sa[3],sa[4],sa[5]);
        }
        if (sleep_list_add(priv, pstat)) {
            DEBUG_INFO("Add to sleep_list %02X%02X%02X%02X%02X%02X\n", sa[0],sa[1],sa[2],sa[3],sa[4],sa[5]);
        }
        RESTORE_INT(flags);

    } else {
        if (pstat->state & WIFI_SLEEP_STATE) {
            pstat->state &= ~(WIFI_SLEEP_STATE);

            if (pstat == priv->pshare->txpause_pstat) {
                RTL_W8(TXPAUSE, RTL_R8(TXPAUSE) & 0xe0);
                priv->pshare->txpause_pstat = NULL;
            }

            if (pstat->sta_in_firmware == 1)
            {

                if(IS_HAL_CHIP(priv))
                {
#ifdef AP_PS_Offlaod
                    GET_HAL_INTERFACE(priv)->APPSOffloadMACIDPauseHandler(priv, pstat->aid, 0);
                    priv->pshare->HWPwroldState[pstat->aid>>5] = priv->pshare->HWPwroldState[pstat->aid>>5] & ~BIT(pstat->aid&0x1F); //manual update pwr state
                    pstat->txpause_flag = 0;
#else
					if(pstat->txpdrop_flag == 1) {
						GET_HAL_INTERFACE(priv)->UpdateHalMSRRPTHandler(priv, pstat, INCREASE);
						pstat->txpdrop_flag = 0;
					}
                    if (pstat->txpause_flag) {
                        DEBUG_WARN("%s %d client leave ps, set MACID sleep AID = %x \n",__FUNCTION__,__LINE__,REMAP_AID(pstat));
                        DEBUG_WARN("Pwr_state jiffies = %lu diff = %lu\n",jiffies,jiffies-pstat->txpause_time);
                        GET_HAL_INTERFACE(priv)->SetMACIDSleepHandler(priv, 0, REMAP_AID(pstat));
                        pstat->txpause_flag = 0;
						if(priv->pshare->paused_sta_num)
							priv->pshare->paused_sta_num--;
                    }
                    pstat->txpause_time = 0;
#endif
                } else
                {
                }
            }
        }

        SAVE_INT_AND_CLI(flags);
        if (sleep_list_del(priv, pstat)) {
            DEBUG_INFO("Del fr sleep_list %02X%02X%02X%02X%02X%02X\n", sa[0],sa[1],sa[2],sa[3],sa[4],sa[5]);
        }
        RESTORE_INT(flags);

        if ((skb_queue_len(&pstat->dz_queue))
#ifdef WIFI_WMM
#ifdef WMM_APSD
                ||(
#ifdef CLIENT_MODE
                (OPMODE & WIFI_AP_STATE) &&
#endif
                (QOS_ENABLE) && (APSD_ENABLE) && (pstat->QosEnabled) && (pstat->apsd_pkt_buffering) &&
                ((!isFFempty(pstat->VO_dz_queue->head, pstat->VO_dz_queue->tail)) ||
                (!isFFempty(pstat->VI_dz_queue->head, pstat->VI_dz_queue->tail)) ||
                (!isFFempty(pstat->BE_dz_queue->head, pstat->BE_dz_queue->tail)) ||
                (!isFFempty(pstat->BK_dz_queue->head, pstat->BK_dz_queue->tail))))
#endif
                || (!isFFempty(pstat->MGT_dz_queue->head, pstat->MGT_dz_queue->tail))
#ifdef DZ_ADDBA_RSP
                || pstat->dz_addba.used
#endif
#endif
        ) {
            SAVE_INT_AND_CLI(flags);
            if (wakeup_list_add(priv, pstat)) {
                DEBUG_INFO("Add to wakeup_list %02X%02X%02X%02X%02X%02X\n", sa[0],sa[1],sa[2],sa[3],sa[4],sa[5]);
            }
            RESTORE_INT(flags);
        }
    }


	if(IS_HAL_CHIP(priv))
	    check_PS_set_HIQLMT(priv);
	return;
}

#ifdef HW_DETEC_POWER_STATE
void detect_hw_pwr_state(struct rtl8192cd_priv *priv, unsigned char macIDGroup)
{
    unsigned char i,newPwrBit,oldPwrBit;
    unsigned int  pwr;

    switch(macIDGroup)
    {
    case 0:
        GET_HAL_INTERFACE(priv)->GetHwRegHandler(priv, HW_VAR_HW_PS_STATE0, (pu1Byte)&pwr);
    break;
    case 1:
        GET_HAL_INTERFACE(priv)->GetHwRegHandler(priv, HW_VAR_HW_PS_STATE1, (pu1Byte)&pwr);
    break;
    case 2:
        GET_HAL_INTERFACE(priv)->GetHwRegHandler(priv, HW_VAR_HW_PS_STATE2, (pu1Byte)&pwr);
    break;
    case 3:
        GET_HAL_INTERFACE(priv)->GetHwRegHandler(priv, HW_VAR_HW_PS_STATE3, (pu1Byte)&pwr);
    break;
    }

    // check PS change
    for(i=0;i<32;i++)
    {
        // getHWPwrStateHander
        //priv->pshare->HWPwrState[i+(macIDGroup<<5)] = (pwr & BIT(i) ? 1:0);

        //if(priv->pshare->HWPwroldState[i+(macIDGroup<<5)]!= priv->pshare->HWPwrState[i+(macIDGroup<<5)])
        newPwrBit= (pwr & BIT(i))>>i;
        oldPwrBit = (priv->pshare->HWPwroldState[macIDGroup] & BIT(i))>>i;
        if(newPwrBit != oldPwrBit)
        {
#ifndef CONFIG_8814_AP_MAC_VERI

      //      printk("[%s][%d] MACID%x HW PS0=%x Seq=%x \n",
      //                     __FUNCTION__,__LINE__,i+(macIDGroup<<5),newPwrBit,RTL_R16(0x1152));

            pwr_state_enhaced(priv,i+(macIDGroup<<5),newPwrBit);
            RTL_W8(0x1150,1);


#endif

#ifdef CONFIG_8814_AP_MAC_VERI
            priv->pwrStateHWCnt[i+(macIDGroup<<5)]++;
            priv->hw_seq[i+(macIDGroup<<5)] = RTL_R16(0x1152);
            priv->pwrHWState[i+(macIDGroup<<5)] = newPwrBit;
            priv->pwroldHWState[i+(macIDGroup<<5)] = pwr;

            if(priv->testResult == true)
            {
              printk("[%s][%d] MACID%x HW PS0=%x Seq=%x Cnt=%x\n",
                __FUNCTION__,__LINE__,i+(macIDGroup<<5),newPwrBit,priv->hw_seq[i+(macIDGroup<<5)],priv->pwrStateHWCnt[i+(macIDGroup<<5)]);
            }
#endif //#ifdef CONFIG_8814_AP_MAC_VERI

        }
    }

    // need for backup new power state
    priv->pshare->HWPwroldState[macIDGroup] = pwr;
}

__IRAM_IN_865X
void pwr_state_enhaced(struct rtl8192cd_priv *priv, unsigned char macID, unsigned char PwrBit)
{
    unsigned long	flags;
    unsigned char   i;
    struct stat_info *pstat;

    if(macID > 0)
    {
        if((macID == 0x7E))
        {
            for(i=0;i<128;i++)
            {
                priv->pshare->HWPwrStateUpdate[i] = false;
            }
            return;
        }

        if((macID == 0x7F))
        {
            return;
        }
    }

    priv->pshare->HWPwrStateUpdate[i] = true;
    pstat = get_HW_mapping_sta(priv,macID);

    // get priv from aidarray

    priv = priv->pshare->aidarray[macID-1]->priv;

	if (pstat == (struct stat_info *)NULL)
	{
        printk("%s %d pstat NULL return\n",__FUNCTION__,__LINE__);
		return;
	}

    if (!(pstat->state & WIFI_ASOC_STATE))
    {
        printk("%s %d pstat NULL return\n",__FUNCTION__,__LINE__);
        return;
    }

    if (PwrBit == 1)
    {
        if ((pstat->state & WIFI_SLEEP_STATE) == 0) {
            pstat->state |= WIFI_SLEEP_STATE;
            if (pstat->sta_in_firmware == 1)
            {
#ifndef AP_PS_Offlaod
                GET_HAL_INTERFACE(priv)->SetMACIDSleepHandler(priv, 1, REMAP_AID(pstat));
                pstat->txpause_time = jiffies;
#endif
                pstat->txpause_flag = 1;

            }
        }

        SAVE_INT_AND_CLI(flags);
        wakeup_list_del(priv, pstat);
        sleep_list_add(priv, pstat);
        RESTORE_INT(flags);

    } else {
        if (pstat->state & WIFI_SLEEP_STATE) {
            pstat->state &= ~(WIFI_SLEEP_STATE);

            if (pstat->sta_in_firmware == 1)
            {
#ifndef AP_PS_Offlaod
                DEBUG_WARN("%s %d client leave ps, set MACID sleep AID = %x \n",__FUNCTION__,__LINE__,REMAP_AID(pstat));
                DEBUG_WARN("Pwr_state jiffies = %x diff = %d\n",jiffies,jiffies-pstat->txpause_time);
                GET_HAL_INTERFACE(priv)->SetMACIDSleepHandler(priv, 0, REMAP_AID(pstat));
                pstat->txpause_time = 0;
#endif
                pstat->txpause_flag = 0;

            }
        }

        SAVE_INT_AND_CLI(flags);
        sleep_list_del(priv, pstat);
        RESTORE_INT(flags);

        if ((skb_queue_len(&pstat->dz_queue))
#ifdef WIFI_WMM
#ifdef WMM_APSD
                ||(
#ifdef CLIENT_MODE
                (OPMODE & WIFI_AP_STATE) &&
#endif
                (QOS_ENABLE) && (APSD_ENABLE) && (pstat->QosEnabled) && (pstat->apsd_pkt_buffering) &&
                ((!isFFempty(pstat->VO_dz_queue->head, pstat->VO_dz_queue->tail)) ||
                (!isFFempty(pstat->VI_dz_queue->head, pstat->VI_dz_queue->tail)) ||
                (!isFFempty(pstat->BE_dz_queue->head, pstat->BE_dz_queue->tail)) ||
                (!isFFempty(pstat->BK_dz_queue->head, pstat->BK_dz_queue->tail))))
#endif
                || (!isFFempty(pstat->MGT_dz_queue->head, pstat->MGT_dz_queue->tail))
#ifdef DZ_ADDBA_RSP
                || pstat->dz_addba.used
#endif
#endif
        ) {
            SAVE_INT_AND_CLI(flags);
            if (wakeup_list_add(priv, pstat)) {
                //DEBUG_INFO("Add to wakeup_list %02X%02X%02X%02X%02X%02X\n", sa[0],sa[1],sa[2],sa[3],sa[4],sa[5]);
            }
            RESTORE_INT(flags);
        }
    }

    if(IS_HAL_CHIP(priv))
        check_PS_set_HIQLMT(priv);

    return;
}
#endif //#ifdef HW_DETEC_POWER_STATE


void check_PS_set_HIQLMT(struct rtl8192cd_priv *priv)
{
    u1Byte HiQLMTEn;
    u1Byte tmp;


        if(priv->sleep_list.next != &priv->sleep_list)  // one client into PS mode
        {
           if (IS_ROOT_INTERFACE(priv)) {
           GET_HAL_INTERFACE(priv)->GetHwRegHandler(priv, HW_VAR_HIQ_NO_LMT_EN, (pu1Byte)&HiQLMTEn);
               HiQLMTEn = HiQLMTEn & (~BIT0);
           GET_HAL_INTERFACE(priv)->SetHwRegHandler(priv, HW_VAR_HIQ_NO_LMT_EN, (pu1Byte)&HiQLMTEn);
            GET_HAL_INTERFACE(priv)->GetHwRegHandler(priv, HW_VAR_HIQ_NO_LMT_EN, (pu1Byte)&tmp);
           }
           else {
               if (GET_ROOT(priv)->pmib->miscEntry.vap_enable) {
               GET_HAL_INTERFACE(priv)->GetHwRegHandler(priv, HW_VAR_HIQ_NO_LMT_EN, (pu1Byte)&HiQLMTEn);
                   HiQLMTEn = HiQLMTEn & ~(BIT(priv->vap_init_seq));
               GET_HAL_INTERFACE(priv)->SetHwRegHandler(priv, HW_VAR_HIQ_NO_LMT_EN, (pu1Byte)&HiQLMTEn);
               GET_HAL_INTERFACE(priv)->GetHwRegHandler(priv, HW_VAR_HIQ_NO_LMT_EN, (pu1Byte)&tmp);
               }
           }
        }
        else {  // all client leave PS mode
           if (IS_ROOT_INTERFACE(priv)) {
           GET_HAL_INTERFACE(priv)->GetHwRegHandler(priv, HW_VAR_HIQ_NO_LMT_EN, (pu1Byte)&HiQLMTEn);
               HiQLMTEn = HiQLMTEn | BIT0;
           GET_HAL_INTERFACE(priv)->SetHwRegHandler(priv, HW_VAR_HIQ_NO_LMT_EN, (pu1Byte)&HiQLMTEn);
           GET_HAL_INTERFACE(priv)->GetHwRegHandler(priv, HW_VAR_HIQ_NO_LMT_EN, (pu1Byte)&tmp);
           }
           else {
               if (GET_ROOT(priv)->pmib->miscEntry.vap_enable) {
               GET_HAL_INTERFACE(priv)->GetHwRegHandler(priv, HW_VAR_HIQ_NO_LMT_EN, (pu1Byte)&HiQLMTEn);
                    HiQLMTEn = HiQLMTEn | BIT(priv->vap_init_seq);
               GET_HAL_INTERFACE(priv)->SetHwRegHandler(priv, HW_VAR_HIQ_NO_LMT_EN, (pu1Byte)&HiQLMTEn);
                GET_HAL_INTERFACE(priv)->GetHwRegHandler(priv, HW_VAR_HIQ_NO_LMT_EN, (pu1Byte)&tmp);
               }
           }
        }
    }
#ifdef CONFIG_IEEE80211W

enum _ROBUST_FRAME_STATE_{
	NOT_ROBUST_MGMT = 0,
	IS_ROBUST_MGMT = 1,
	MGMT_FRAME_MIC_ERR = 2,
};

static int isCorrectRobustFrame(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned char *pframe = get_pframe(pfrinfo);
	unsigned char Category_field;
	struct stat_info *pstat = get_stainfo(priv, pfrinfo->sa);
#ifdef CONFIG_IEEE80211W_CLI
	unsigned char *da = GetAddr1Ptr(pframe);
#endif

	if(pframe[0] == WIFI_DEAUTH  || pframe[0] == WIFI_DISASSOC || pframe[0] == WIFI_WMM_ACTION) {
		if (GetPrivacy(pframe) && pstat
#ifdef CONFIG_IEEE80211W_CLI
		&& !IS_MCAST(da)
#endif
		) {
			unsigned char mic[8];

			memcpy(mic,pframe + pfrinfo->pktlen - 8, 8);
			aesccmp_decrypt(priv, pfrinfo, 1);

			if(!aesccmp_checkmic(priv, pfrinfo, mic)) {
                #ifdef PMF_DEBUGMSG
				if(pframe[0] == WIFI_DEAUTH){
					PMFDEBUG("DEAUTH MIC_ERR[%s]\n", priv->dev->name);
				}else if(pframe[0] == WIFI_DISASSOC){
					PMFDEBUG("DISASSOC MIC_ERR[%s]\n", priv->dev->name);
				}else{
					PMFDEBUG(" MIC_ERR[%s]\n", priv->dev->name);
                }
                #endif
				return MGMT_FRAME_MIC_ERR;
			}
			else {
				memcpy(pframe + pfrinfo->hdr_len, pframe + pfrinfo->hdr_len + 8, pfrinfo->pktlen - pfrinfo->hdr_len - 8 - 8);
				pfrinfo->pktlen = pfrinfo->pktlen - 16;
			}
		}
#ifdef CONFIG_IEEE80211W_CLI
		if(priv->bss_support_pmf && pstat){
			pstat->isPMF = TRUE;
		}
#endif
	}

	if(pframe[0] == WIFI_DEAUTH  || pframe[0] == WIFI_DISASSOC)
	{
		return IS_ROBUST_MGMT;
	}
	//pframe += WLAN_HDR_A3_LEN; // Action Field

	if(pframe[0] == WIFI_WMM_ACTION) {
		Category_field = pframe[WLAN_HDR_A3_LEN];
	} else
		return NOT_ROBUST_MGMT;

	return (Category_field != 4) && (Category_field != 7) && (Category_field != 11)
			&& (Category_field != 15) && (Category_field != 127);
}

#endif

#ifdef CONFIG_IEEE80211W

#ifdef CONFIG_IEEE80211W_CLI


static int isMMICExist(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned char *pframe = get_pframe(pfrinfo);

	if(pfrinfo->pktlen + MMIC_CRC_LEN <= MMIC_ILLEGAL_LEN)
		return FALSE;

	if(pframe[BIP_HEADER_LEN] == _MMIC_IE_){
		PMFDEBUG(" MMIC IE Exists!!\n");
		if(pframe[BIP_HEADER_LEN + MMIC_TAG_IE] == _MMIC_LEN_){
#ifdef CONFIG_IEEE80211W_CLI_DEBUG
			panic_printk("MMIC IE=");
			int idx;
			for(idx = 0; idx < _MMIC_LEN_; idx++)
				panic_printk("%02x", pframe[BIP_HEADER_LEN + MMIC_TAG_IE + MMIC_TAG_LEN + idx]);
			panic_printk("\n");
#endif
		}
		return TRUE;
	}
	else{
		PMFDEBUG(" No MMIC IE Exists!!\n");
		return FALSE;
	}
}

static int CheckBIPMIC(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned char *pframe = get_pframe(pfrinfo);

	if(MMIE_check(priv, pfrinfo)){
		PMFDEBUG("(%s)line=%d, BIP: MIC Check PASS \n", __FUNCTION__, __LINE__);
		return TRUE;
	}else{
		if(pframe[0] == WIFI_DEAUTH) {
			PMFDEBUG("[%s]%s(%d) BIP: DEAUTH MIC_ERR\n", priv->dev->name, __FUNCTION__, __LINE__);
		}else if(pframe[0] == WIFI_DISASSOC) {
			PMFDEBUG("[%s]%s(%d) BIP: DISASSOC MIC_ERR\n", priv->dev->name, __FUNCTION__, __LINE__);
		}else
			PMFDEBUG("[%s]%s(%d) BIP: MIC_ERR\n", priv->dev->name, __FUNCTION__, __LINE__);

		return FALSE;
	}
}


#endif

int legal_mgnt_frame(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned char *pframe = get_pframe(pfrinfo);
	unsigned char *da = GetAddr1Ptr(pframe);
	int ret = isCorrectRobustFrame(priv, pfrinfo);

	if (ret == MGMT_FRAME_MIC_ERR) { // mgmt frame with mic error
		return MGNT_ERR;
	} else if(ret == IS_ROBUST_MGMT) { // robust mgmt frame
		if(priv->pmib->dot1180211AuthEntry.dot11IEEE80211W == NO_MGMT_FRAME_PROTECTION) {
			if (GetPrivacy(pframe))
				return MGNT_PRIVACY_ERR;
		}
		else if(priv->pmib->dot1180211AuthEntry.dot11IEEE80211W == MGMT_FRAME_PROTECTION_OPTIONAL) {
			struct stat_info *pstat = get_stainfo(priv, pfrinfo->sa);
			if (pstat == NULL)
				return MGNT_ERR;

			if (pstat->isPMF == 0) { // MFPC=0
				if (GetPrivacy(pframe))
					return MGNT_PRIVACY_ERR;
			} else { // MFPC = 1
				if (!IS_MCAST(da)) {
					if(GET_UNICAST_ENCRYP_KEY == NULL) {
						if((pframe[0] == WIFI_DEAUTH  || pframe[0] == WIFI_DISASSOC)) {
							if(GetPrivacy(pframe))
								return MGNT_PRIVACY_ERR;
						} else
							return MGNT_PRIVACY_ERR;
					} else {
						if(!GetPrivacy(pframe))
							return MGNT_PRIVACY_ERR;
						else {
							ClearPrivacy(pframe);
							return MGNT_LEGAL;
						}
					}
				}// Does AP receive multicast robust frame from client?
#ifdef CONFIG_IEEE80211W_CLI
				else { // MMPDU has a group RA
					if(!GET_IGROUP_ENCRYP_KEY) {
						if((pframe[0] != WIFI_DEAUTH  && pframe[0] != WIFI_DISASSOC))
							return MGNT_ERR;
					} else {
							if(!isMMICExist(priv ,pfrinfo)) {
								return MGNT_BCAST_PRIVACY_ERR;
							} else {
								if(!CheckBIPMIC(priv, pfrinfo))
									return MGNT_BCAST_PRIVACY_ERR;
							}
					}
			  }
#endif
			}
		}
		else if(priv->pmib->dot1180211AuthEntry.dot11IEEE80211W == MGMT_FRAME_PROTECTION_REQUIRED) {
			struct stat_info *pstat = get_stainfo(priv, pfrinfo->sa);
			if (pstat == NULL)
				return MGNT_ERR;

			if (pstat->isPMF == 0) { // MFPC=0
					return MGNT_ERR;
			} else { // MFPC = 1
				if (!IS_MCAST(da)) {
					if(GET_UNICAST_ENCRYP_KEY == NULL) {
							return MGNT_PRIVACY_ERR;
					} else {
						if(!GetPrivacy(pframe))
							return MGNT_PRIVACY_ERR;
						else {
							ClearPrivacy(pframe);
							return MGNT_LEGAL;
						}
					}
				}
			}
		}

	}
	return MGNT_LEGAL;
}
#endif	// CONFIG_IEEE80211W

void mgt_handler(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
	struct mlme_handler *ptable;
	unsigned int index;
	unsigned char *pframe = get_pframe(pfrinfo);
	unsigned char *sa = pfrinfo->sa;
	unsigned char *da = pfrinfo->da;
	struct stat_info *pstat = NULL;
	unsigned short frame_type;

#if 0	// already flush cache in rtl8192cd_rx_isr()
#endif

	if (OPMODE & WIFI_AP_STATE)
		ptable = mlme_ap_tbl;
#ifdef CLIENT_MODE
	else if (OPMODE & (WIFI_STATION_STATE | WIFI_ADHOC_STATE))
		ptable = mlme_station_tbl;
#endif
	else
	{
		DEBUG_ERR("Currently we do not support opmode=%d\n", OPMODE);
		if (IS_ROOT_INTERFACE(priv) || (pfrinfo->is_br_mgnt == 0))
		rtl_kfree_skb(priv, pfrinfo->pskb, _SKB_RX_);
		return;
	}

	frame_type = GetFrameSubType(pframe);
	index = frame_type >> 4;
	if (index > 13)
	{
		DEBUG_ERR("Currently we do not support reserved sub-fr-type=%d\n", index);
		if (IS_ROOT_INTERFACE(priv) || (pfrinfo->is_br_mgnt == 0))
		rtl_kfree_skb(priv, pfrinfo->pskb, _SKB_RX_);
		return;
	}
	ptable += index;


	pstat = get_stainfo(priv, sa);

	if ((pstat != NULL) && (WIFI_PROBEREQ != frame_type))
	{
#ifdef DETECT_STA_EXISTANCE
		if (IS_HAL_CHIP(priv)) {
			if (pstat->leave)
				GET_HAL_INTERFACE(priv)->UpdateHalMSRRPTHandler(priv, pstat, INCREASE);
		}
		pstat->leave = 0;
#endif
	}

	if (!IS_MCAST(da))
	{
		//pstat = get_stainfo(priv, sa);

		// only check last cache seq number for management frame, david -------------------------
		if (pstat != NULL) {
			if (GetRetry(pframe)) {
				if (GetTupleCache(pframe) == pstat->tpcache_mgt) {
					priv->ext_stats.rx_decache++;
					if (IS_ROOT_INTERFACE(priv) || (pfrinfo->is_br_mgnt == 0))
					rtl_kfree_skb(priv, pfrinfo->pskb, _SKB_RX_);
					SNMP_MIB_INC(dot11FrameDuplicateCount, 1);
					return;
				}
				else
				{
					  pstat->tpcache_mgt = GetTupleCache(pframe);
				}
			}
			pstat->tpcache_mgt = GetTupleCache(pframe);
		}
	}

	// log rx statistics...
	if (pstat != NULL)
	{
		if (!((OPMODE & WIFI_AP_STATE) && ((ptable->num == WIFI_BEACON) || (ptable->num == WIFI_PROBEREQ))))
		{
			rx_sum_up(NULL, pstat, pfrinfo);

			update_sta_rssi(priv, pstat, pfrinfo);
		}

#ifdef CLIENT_MODE
		if ((OPMODE & WIFI_STATION_STATE) && (ptable->num != WIFI_BEACON))
			priv->rxDataNumInPeriod++;
#endif
	}

	// check power save state
	if ((OPMODE & WIFI_AP_STATE) && (pstat != NULL)) {
		if (IS_BSSID(priv, GetAddr1Ptr(pframe)) || IS_MCAST(GetAddr1Ptr(pframe)))
		{
		#ifdef HW_DETEC_POWER_STATE
            if (IS_SUPPORT_HW_DETEC_POWER_STATE(priv)) {
                // 8814 power state control only by HW, not by SW.
                // Only if HW detect macid not ready, SW patch this packet
                if(pfrinfo->macid == HW_MACID_SEARCH_NOT_READY)
                {
                    printk("%s %d HW_MACID_SEARCH_NOT_READY",__FUNCTION__,__LINE__);
                    if(priv->pshare->HWPwrStateUpdate[pstat->aid]==false)
                    {
                        printk("%s %d HW not update By SW Aid = %x \n",__FUNCTION__,__LINE__,pstat->aid);
			pwr_state(priv, pfrinfo);
	}
                }
                else if(pfrinfo->macid > HW_MACID_SEARCH_SUPPORT_NUM)
                {
                    pwr_state(priv, pfrinfo);
                }
            } else
        #endif // #ifdef HW_DETEC_POWER_STATE
                     {
                	pwr_state(priv, pfrinfo);
                     }
                }
	}

	if (
		GET_ROOT(priv)->pmib->miscEntry.vap_enable &&
		IS_VAP_INTERFACE(priv)) {
		if (IS_MCAST(da) || isEqualMACAddr(GET_MY_HWADDR, da))
		{
			ptable->func(priv, pfrinfo);
		}
	}
	else
	{
#ifdef CONFIG_IEEE80211W
		int ret = legal_mgnt_frame(priv,pfrinfo);
		if(ret == MGNT_LEGAL)
#endif
		{
			ptable->func(priv, pfrinfo);
		}
#ifdef CONFIG_IEEE80211W_CLI
		else if(ret == MGNT_PRIVACY_ERR)
		{
			if((pframe[0] == WIFI_DEAUTH  || pframe[0] == WIFI_DISASSOC))
			{
				PMFDEBUG("issue_SA_Query_Req() when Receving Unprotected action frame !\n");
				issue_SA_Query_Req(priv->dev,pstat->hwaddr);
			}
		}else
			PMFDEBUG("(%s)line=%d MGNT_ERR! ret = %d \n", __FUNCTION__, __LINE__, ret);
#endif
	}

	if (IS_VAP_INTERFACE(priv)) {
		if (pfrinfo->is_br_mgnt) {
			rx_sum_up(priv, NULL, pfrinfo);
			return;
		}
	}
	else if (IS_ROOT_INTERFACE(priv) && pfrinfo->is_br_mgnt && (OPMODE & WIFI_AP_STATE)) {
		int i;
#ifdef CONFIG_IEEE80211W
		int retVal;
#endif
		for (i=0; i<RTL8192CD_NUM_VWLAN; i++) {
			if ((IS_DRV_OPEN(priv->pvap_priv[i])) && ((IS_MCAST(da)) ||
				(isEqualMACAddr(priv->pvap_priv[i]->pmib->dot11StationConfigEntry.dot11Bssid, da))))
			{
#ifdef CONFIG_IEEE80211W
				retVal = legal_mgnt_frame(priv->pvap_priv[i],pfrinfo);
				if(retVal == MGNT_LEGAL)
#endif
				{
					mgt_handler(priv->pvap_priv[i], pfrinfo);
				}
#ifdef CONFIG_IEEE80211W_CLI
				else if(retVal == MGNT_PRIVACY_ERR)
				{
					if((pframe[0] == WIFI_DEAUTH  || pframe[0] == WIFI_DISASSOC))
						issue_SA_Query_Req(priv->dev,pstat->hwaddr);
				}else
					PMFDEBUG("(%s)line=%d MGNT_ERR! retVal = %d \n", __FUNCTION__, __LINE__, retVal);
#endif
			}
		 }
	}

	if (pfrinfo->is_br_mgnt) {
		pfrinfo->is_br_mgnt = 0;

// fix hang-up issue when root-ap (A+B) + vxd-client ------------
		if (IS_DRV_OPEN(GET_VXD_PRIV(priv))) {
			if ((OPMODE & WIFI_AP_STATE) ||
				((OPMODE & WIFI_STATION_STATE) &&
				GET_VXD_PRIV(priv) && (GET_VXD_PRIV(priv)->drv_state & DRV_STATE_VXD_AP_STARTED))) {
//--------------------------------------------david+2006-07-17

				mgt_handler(GET_VXD_PRIV(priv), pfrinfo);
				return;
			}
		}
	}

	rtl_kfree_skb(priv, pfrinfo->pskb, _SKB_RX_);
}


/*----------------------------------------------------------------------------
// the purpose of this sub-routine
Any station has changed from sleep to active state, and has data buffer should
be dequeued here!
-----------------------------------------------------------------------------*/
void process_dzqueue(struct rtl8192cd_priv *priv)
{
	struct stat_info *pstat;
	struct sk_buff *pskb;
	struct list_head *phead = &priv->wakeup_list;
	struct list_head *plist = phead->next;
	unsigned long flags;
	int round = 0;

	while(plist != phead)
	{
		pstat = list_entry(plist, struct stat_info, wakeup_list);
		plist = plist->next;

		while(1)
		{
// 2009.09.08
			SAVE_INT_AND_CLI(flags);
#if defined(WIFI_WMM) && defined(WMM_APSD)
			if (
#ifdef CLIENT_MODE
				(OPMODE & WIFI_AP_STATE) &&
#endif
				(QOS_ENABLE) && (APSD_ENABLE) && pstat && (pstat->QosEnabled) && (pstat->apsd_pkt_buffering)) {
				pskb = (struct sk_buff *)deque(priv, &(pstat->VO_dz_queue->head), &(pstat->VO_dz_queue->tail),
					(unsigned long)(pstat->VO_dz_queue->pSkb), NUM_APSD_TXPKT_QUEUE);
				if (pskb == NULL) {
					pskb = (struct sk_buff *)deque(priv, &(pstat->VI_dz_queue->head), &(pstat->VI_dz_queue->tail),
						(unsigned long)(pstat->VI_dz_queue->pSkb), NUM_APSD_TXPKT_QUEUE);
					if (pskb == NULL) {
						pskb = (struct sk_buff *)deque(priv, &(pstat->BE_dz_queue->head), &(pstat->BE_dz_queue->tail),
							(unsigned long)(pstat->BE_dz_queue->pSkb), NUM_APSD_TXPKT_QUEUE);
						if (pskb == NULL) {
							pskb = (struct sk_buff *)deque(priv, &(pstat->BK_dz_queue->head), &(pstat->BK_dz_queue->tail),
								(unsigned long)(pstat->BK_dz_queue->pSkb), NUM_APSD_TXPKT_QUEUE);
							if (pskb == NULL) {
								pstat->apsd_pkt_buffering = 0;
								goto legacy_ps;
							}
							DEBUG_INFO("release BK pkt\n");
						} else {
							DEBUG_INFO("release BE pkt\n");
						}
					} else {
						DEBUG_INFO("release VI pkt\n");
					}
				} else {
					DEBUG_INFO("release VO pkt\n");
				}
			} else
legacy_ps:
#endif
#if defined(WIFI_WMM)
			if (!isFFempty(pstat->MGT_dz_queue->head, pstat->MGT_dz_queue->tail)){
				struct tx_insn *tx_cfg;
				tx_cfg = (struct tx_insn *)deque(priv, &(pstat->MGT_dz_queue->head), &(pstat->MGT_dz_queue->tail),
					(unsigned long)(pstat->MGT_dz_queue->ptx_insn), NUM_DZ_MGT_QUEUE);
				if ((rtl8192cd_firetx(priv, tx_cfg)) == SUCCESS){
					DEBUG_INFO("release MGT pkt\n");
				}else{
					DEBUG_ERR("release MGT pkt failed!\n");
					if (tx_cfg->phdr)
						release_wlanhdr_to_poll(priv, tx_cfg->phdr);
					if (tx_cfg->pframe)
						release_mgtbuf_to_poll(priv, tx_cfg->pframe);
				}
				kfree(tx_cfg);
				RESTORE_INT(flags);
				continue;
			}
			else
#ifdef DZ_ADDBA_RSP
			if (pstat->dz_addba.used) {
				issue_ADDBArsp(priv, pstat->hwaddr, pstat->dz_addba.dialog_token,
						pstat->dz_addba.TID, pstat->dz_addba.status_code, pstat->dz_addba.timeout);
				pstat->dz_addba.used = 0;
				//printk("issue DZ addba!!!!!!!\n");
				RESTORE_INT(flags);
				continue;
			}
			else
#endif
#endif
			pskb = __skb_dequeue(&pstat->dz_queue);

// 2009.09.08
			RESTORE_INT(flags);

			if (pskb == NULL)
				break;

#ifdef ENABLE_RTL_SKB_STATS
			rtl_atomic_dec(&priv->rtl_tx_skb_cnt);
#endif

			if (rtl8192cd_start_xmit_noM2U(pskb, pskb->dev))
				rtl_kfree_skb(priv, pskb, _SKB_TX_);

			if (++round > 10000) {
				panic_printk("%s[%d] while (1) goes too many\n", __FUNCTION__, __LINE__);
				break;
			}
		}

		SAVE_INT_AND_CLI(flags);
		if (wakeup_list_del(priv, pstat)) {
			DEBUG_INFO("Del fr wakeup_list %02X%02X%02X%02X%02X%02X\n",
				pstat->hwaddr[0],pstat->hwaddr[1],pstat->hwaddr[2],pstat->hwaddr[3],pstat->hwaddr[4],pstat->hwaddr[5]);
		}
		RESTORE_INT(flags);
	}
}


void process_mcast_dzqueue(struct rtl8192cd_priv *priv)
{
	struct sk_buff *pskb;
	int round = 0;

	priv->release_mcast = 1;
	while (1) {
		pskb = (struct sk_buff *)deque(priv, &(priv->dz_queue.head), &(priv->dz_queue.tail),
			(unsigned long)(priv->dz_queue.pSkb), NUM_TXPKT_QUEUE);

		if (pskb == NULL)
			break;

#ifdef ENABLE_RTL_SKB_STATS
		rtl_atomic_dec(&priv->rtl_tx_skb_cnt);
#endif

// stanley: I think using pskb->dev is correct IN THE FUTURE, when mesh0 also applies dzqueue
		if (rtl8192cd_start_xmit(pskb, priv->dev))
			rtl_kfree_skb(priv, pskb, _SKB_TX_);

		if (++round > 10000) {
			panic_printk("%s[%d] while (1) goes too many\n", __FUNCTION__, __LINE__);
			break;
		}
	}
	priv->release_mcast = 0;
}


#if 0
 int check_basic_rate(struct rtl8192cd_priv *priv, unsigned char *pRate, int pLen)
{
	int i, match, idx;
	UINT8 rate;

	// david, check if is there is any basic rate existed --
	int any_one_basic_rate_found = 0;

	for (i=0; i<pLen; i++) {
		if (pRate[i] & 0x80) {
			any_one_basic_rate_found = 1;
			break;
		}
	}

	for (i=0; i<AP_BSSRATE_LEN; i++) {
		if (AP_BSSRATE[i] & 0x80) {
			rate = AP_BSSRATE[i] & 0x7f;
			match = 0;
			for (idx=0; idx<pLen; idx++) {
				if ((pRate[idx] & 0x7f) == rate) {
					if (pRate[idx] & 0x80)
						match = 1;
					else {
						if (!any_one_basic_rate_found) {
							pRate[idx] |= 0x80;
							match = 1;
						}
					}
				}
			}
			if (match == 0)
				return FAIL;
		}
	}
	return SUCCESS;
}
#endif

#if (BEAMFORMING_SUPPORT == 1)


void dump_candicate
(	struct rtl8192cd_priv *priv,
	struct stat_info** candidate,
	unsigned char num,
	unsigned char type)
{

	int idx = 0;
	struct stat_info *psta;


	for(idx=0 ; idx < num ; idx ++){
		if(candidate[idx]){
			psta = candidate[idx];

			panic_printk("[Candi %d][Type %d][%02x %02x %02x %02x %02x %02x] bf_score = %d error_csi=%d\n",
			idx, type,
			psta->hwaddr[0], psta->hwaddr[1], psta->hwaddr[2],
			psta->hwaddr[3], psta->hwaddr[4], psta->hwaddr[5],
			psta->bf_score, psta->error_csi);
		}
		else {
			if(idx == 0)
				panic_printk("No Candicate !! \n");
			break;
		}

	}

}

void clear_bfee_entry(struct rtl8192cd_priv *priv, unsigned char type)
{
	int tmp =0;
	struct stat_info *pstat;

	PRT_BEAMFORMING_INFO	pBeamInfo = &(priv->pshare->BeamformingInfo);
	PRT_BEAMFORMING_ENTRY	pEntry;

	//panic_printk("[%s] type = %d +++ \n", __FUNCTION__, type);
	ODM_RT_TRACE(ODMPTR, PHYDM_COMP_TXBF, ODM_DBG_LOUD, ("%s, type=%d\n", __FUNCTION__, type));

	for(tmp = 0; tmp < BEAMFORMEE_ENTRY_NUM; tmp++) {
		ODM_RT_TRACE(ODMPTR, PHYDM_COMP_TXBF, ODM_DBG_LOUD, ("%s Entry %d, bUsed=%d\n", __FUNCTION__, tmp, pBeamInfo->BeamformeeEntry[tmp].bUsed));
		if( pBeamInfo->BeamformeeEntry[tmp].bUsed) {
#if (MU_BEAMFORMING_SUPPORT == 1)
			if(pBeamInfo->BeamformeeEntry[tmp].is_mu_sta == type)
#endif
			{
				Beamforming_Leave(priv, pBeamInfo->BeamformeeEntry[tmp].MacAddr);
			}

		}
	}

	if(type == TXBF_TYPE_MU){
		//STOP MU TX
		unsigned int tmp32 = RTL_R32(0x14c0);

		tmp32 &= (~BIT7);
		RTL_W32(0x14c0, tmp32);
	}

}

unsigned char is_candicate(struct stat_info* psta, unsigned int num_candidate,  struct stat_info** candidate)
{
	int idx = 0;

	if(num_candidate==0)
		return 0;

	for(idx=0 ; idx < num_candidate ; idx ++){
		if(psta == candidate[idx])
			return 1;
	}

	return 0;

}

unsigned char is_support_bf(struct stat_info* psta, unsigned char type)
{

#ifdef RTK_AC_SUPPORT
	if(type == TXBF_TYPE_MU){
		if(psta->error_csi == 1)
			return 0;
		if((psta->vht_cap_len && (cpu_to_le32(psta->vht_cap_buf.vht_cap_info) & BIT(MU_BFEE_S)) && get_sta_vht_mimo_mode(psta) == MIMO_1T1R))
			return 1;
	}
#endif
	else if(type == TXBF_TYPE_SU){
		if(psta->error_csi == 1)
			return 0;
		if((psta->ht_cap_len && (cpu_to_le32(psta->ht_cap_buf.txbf_cap)&_HTCAP_RECEIVED_NDP)) 	  //
#ifdef RTK_AC_SUPPORT
			|| (psta->vht_cap_len && (cpu_to_le32(psta->vht_cap_buf.vht_cap_info) & BIT(SU_BFEE_S))))
#endif
		return 1;
	}

	return 0;

}

void dym_disable_bf_coeff(struct rtl8192cd_priv *priv)
{
	u1Byte i, flags;

	PRT_BEAMFORMING_INFO pBeamformingInfo = &(priv->pshare->BeamformingInfo);
	PRT_SOUNDING_INFOV2 pSoundingInfo = &(pBeamformingInfo->SoundingInfoV2);
	PRT_BEAMFORMING_ENTRY	pBeamformEntry;

	flags = 0;
	for(i = 0; i < BEAMFORMEE_ENTRY_NUM; i++) {
		pBeamformEntry = &(pBeamformingInfo->BeamformeeEntry[i]);
		if( pBeamformEntry->bUsed && pBeamformEntry->pSTA &&
			(pBeamformEntry->pSTA->current_tx_rate >= _NSS1_MCS7_RATE_ && pBeamformEntry->pSTA->current_tx_rate <= _NSS1_MCS9_RATE_ )) {
			flags = 1;
			break;
		}
	}

	Beamforming_dym_disable_bf_coeff_8822B(priv, flags);

}

void get_bf_score(struct rtl8192cd_priv *priv)
{
	unsigned int idx, txtp = 0;
	struct stat_info* psta;

	idx = 0;

	psta = findNextSTA(priv, &idx);

	while(psta) {
		txtp = (unsigned int)(psta->tx_avarage>>17);

		psta->bf_score = phydm_get_mu_bfee_snding_decision(ODMPTR, txtp);

		if(psta->IOTPeer == HT_IOT_PEER_INTEL)
			psta->bf_score = 0;

#if 0
		panic_printk("[%02x %02x %02x %02x %02x %02x] bf_score = %d \n",
			psta->hwaddr[0], psta->hwaddr[1], psta->hwaddr[2],
			psta->hwaddr[3], psta->hwaddr[4], psta->hwaddr[5],
			psta->bf_score);
#endif
		psta = findNextSTA(priv, &idx);

	}

}

void get_txbf_candidate
(	struct rtl8192cd_priv *priv,
	struct stat_info** candidate,
	unsigned char num,
	unsigned char type)
{
	int idx, i = 0;
	struct stat_info* psta;
	struct stat_info* candidate_tmp;
	PRT_BEAMFORMING_INFO pBeamformingInfo = &(priv->pshare->BeamformingInfo);

	candidate[0] = NULL;

	for(i = 0; i<num; i++){

		idx = 0;
		candidate_tmp = NULL;
		psta = findNextSTA(priv, &idx);

		while(psta) {
			if(psta && psta->expire_to) {
				if( (isEqualMACAddr(psta->hwaddr, pBeamformingInfo->DelEntryListByMACAddr.BFeeEntry_Idx0)) ){
					psta = findNextSTA(priv, &idx);
					continue;
				}
				if((isEqualMACAddr(psta->hwaddr, pBeamformingInfo->DelEntryListByMACAddr.BFeeEntry_Idx1))){
					psta = findNextSTA(priv, &idx);
					continue;
				}

				if((!is_support_bf(psta, type)) ||
					(priv->pmib->dot11RFEntry.txbf_mu && type == TXBF_TYPE_SU && is_support_bf(psta, TXBF_TYPE_MU))){
					psta = findNextSTA(priv, &idx);
					continue;
				}

				if(is_candicate(psta, num, candidate)){
					psta = findNextSTA(priv, &idx);
					continue;
				}

				if(candidate_tmp == NULL)
					candidate_tmp = psta;
				else if(psta->bf_score > candidate_tmp->bf_score)
					candidate_tmp = psta;
			}

			psta = findNextSTA(priv, &idx);
		}

		if(candidate_tmp == NULL)
			break;

		candidate[i] = candidate_tmp;
	}

	//dump_candicate(priv,candidate,num,type);

	return;
}

void assign_txbf_candidate_to_entry
(	struct rtl8192cd_priv *priv,
	struct stat_info** candidate,
	unsigned char num,
	unsigned char type)
{
	int idx, tmp, is_in_entry, need_reassign = 0;
	struct stat_info* psta;

	PRT_BEAMFORMING_INFO 	pBeamInfo = &(priv->pshare->BeamformingInfo);
	PRT_BEAMFORMING_ENTRY	pEntry;

	if(candidate[0] == NULL)
		return;

	for(idx = 0; idx < num; idx ++){

		if(candidate[idx] == NULL)
			break;

		psta = candidate[idx];

		is_in_entry = 0;

		for(tmp = 0; tmp < BEAMFORMEE_ENTRY_NUM; tmp++) {
			if( pBeamInfo->BeamformeeEntry[tmp].bUsed &&
				(pBeamInfo->BeamformeeEntry[tmp].MacId == psta->aid))
					is_in_entry = 1;
		}

		if(is_in_entry == 0){
			need_reassign = 1;
			break;
		}

	}

	//panic_printk("[%s] type = %d need_reassign = %d \n", __FUNCTION__, type, need_reassign);

	if(need_reassign){
		ODM_RT_TRACE(ODMPTR, PHYDM_COMP_TXBF, ODM_DBG_LOUD, ("%s, need_reassign\n", __FUNCTION__));
		clear_bfee_entry(priv, type);

		for(idx = 0; idx < num; idx ++){
			if(candidate[idx] == NULL)
				break;

			psta = candidate[idx];
			Beamforming_Enter(priv, psta);
		}
	}


}

void DynamicSelectTxBFSTA(struct rtl8192cd_priv *priv)
{
	unsigned char num_su = MAX_NUM_BEAMFORMEE_SU;
#if (MU_BEAMFORMING_SUPPORT == 1)
	unsigned char num_mu = MAX_NUM_BEAMFORMEE_MU;
#endif
	struct stat_info* candidate[NUM_TXBF_ALL];

	PRT_BEAMFORMING_INFO 	pBeamInfo = &(priv->pshare->BeamformingInfo);
	PRT_BEAMFORMING_ENTRY	pEntry;

	if(priv->pmib->dot11RFEntry.txbf == 0)
	return;

	if((GET_CHIP_VER(priv)== VERSION_8822B) && (GET_CHIP_VER_8822(priv) <= 0x2)) {
#if (MU_BEAMFORMING_SUPPORT == 1)
		if(pBeamInfo->SoundingInfoV2.CandidateMUBFeeCnt)
			num_su = 0;
		else
#endif
			num_su = 1;
	}

	//panic_printk("[%s] used entry number, mu=%d su=%d \n",
		//__FUNCTION__ ,pBeamInfo->beamformee_mu_cnt, pBeamInfo->beamformee_su_cnt);
	dym_disable_bf_coeff(priv);

	memset(candidate, 0, sizeof(candidate));
	get_bf_score(priv);

	if(num_su == 0) {
		clear_bfee_entry(priv, TXBF_TYPE_SU);
	} else {
		get_txbf_candidate(priv, candidate, num_su, TXBF_TYPE_SU);
		assign_txbf_candidate_to_entry(priv, candidate, num_su, TXBF_TYPE_SU);
	}
#if (MU_BEAMFORMING_SUPPORT == 1)
	if(!priv->pmib->dot11RFEntry.txbf_mu)
		return;

	if(num_mu == 0) {
		clear_bfee_entry(priv, TXBF_TYPE_MU);
	} else {
		ODM_RT_TRACE(ODMPTR, PHYDM_COMP_TXBF, ODM_DBG_LOUD, ("%s %d\n", __FUNCTION__, __LINE__));
		get_txbf_candidate(priv, candidate, num_mu, TXBF_TYPE_MU);
		assign_txbf_candidate_to_entry(priv, candidate, num_mu, TXBF_TYPE_MU);
	}
#endif

	return;
}

void DynamicSelect2STA(struct rtl8192cd_priv *priv)
{
#if 1// (RTL8822B_SUPPORT != 1)
	int idx = 0, isCandidate, candidateCtr=0, bfeCtr=0;
	struct stat_info* psta, *candidate1=NULL, *candidate2=NULL;
	PRT_BEAMFORMING_INFO pBeamformingInfo = &(priv->pshare->BeamformingInfo);
	PRT_BEAMFORMING_ENTRY	pEntry;

	if (GET_CHIP_VER(priv) == VERSION_8822B)
		return;

	if(pBeamformingInfo->BeamformeeEntry[0].bUsed && pBeamformingInfo->BeamformeeEntry[1].bUsed) {
		psta = findNextSTA(priv, &idx);
		while(psta) {
			if(psta->expire_to
			) {
				if (candidate1 == NULL || psta->bf_score > candidate1->bf_score)
					candidate1 = psta;
			}
			psta = findNextSTA(priv, &idx);
		}
		if(candidate1 ==NULL)
			return;
		idx = 0;
		psta = findNextSTA(priv, &idx);
		while(psta) {
			if(psta != candidate1 && psta->expire_to
			) {
				if(candidate2 == NULL || psta->bf_score > candidate2->bf_score)
					candidate2 = psta;
			}
			psta = findNextSTA(priv, &idx);
		};

		if(candidate1)
			++candidateCtr;
		if(candidate2)
			++candidateCtr;
		{
			for(idx = 0; idx < BEAMFORMEE_ENTRY_NUM; idx++) {
				if( pBeamformingInfo->BeamformeeEntry[idx].bUsed) {
					isCandidate = 0;
					if(candidate1 && (pBeamformingInfo->BeamformeeEntry[idx].MacId == candidate1->aid))
						++bfeCtr;
					else if(candidate2 && (pBeamformingInfo->BeamformeeEntry[idx].MacId == candidate2->aid))
						++bfeCtr;
				}
			}
			candidateCtr -=bfeCtr;
			for(idx = 0; idx < BEAMFORMEE_ENTRY_NUM; idx++)
			{
				if(	pBeamformingInfo->BeamformeeEntry[idx].bUsed) {
					isCandidate = 0;
					if(candidate1 && (pBeamformingInfo->BeamformeeEntry[idx].MacId == candidate1->aid))
						isCandidate = 1;
					if(candidate2 && (pBeamformingInfo->BeamformeeEntry[idx].MacId == candidate2->aid))
						 isCandidate = 2;
					if(isCandidate==0 && candidateCtr) {

                                        pBeamformingInfo->CurDelBFerBFeeEntrySel = BFeeEntry;

						ODM_RT_TRACE(ODMPTR, PHYDM_COMP_TXBF, ODM_DBG_LOUD, ("%s,\n", __FUNCTION__));

						if(Beamforming_DeInitEntry(priv, pBeamformingInfo->BeamformeeEntry[idx].MacAddr))
							Beamforming_Notify(priv);

						--candidateCtr;
					}
				}
			}

			if(candidate1) {
				pEntry = Beamforming_GetEntryByMacId(priv, candidate1->aid, (unsigned char*)&idx);
				if(pEntry == NULL) {
					Beamforming_Enter(priv, candidate1);
				}
			}
			if(candidate2) 	{
				pEntry = Beamforming_GetEntryByMacId(priv, candidate2->aid, (unsigned char*)&idx);
				if(pEntry == NULL) {
					Beamforming_Enter(priv, candidate2);
				}
			}
		}
	}
#endif
}
#endif

// which: 0: set basic rates as mine, 1: set basic rates as peer's
 void get_matched_rate(struct rtl8192cd_priv *priv, unsigned char *pRate, int *pLen, int which)
{
	int i, j, num=0;
	UINT8 rate;
	UINT8 found_rate[32];

	for (i=0; i<AP_BSSRATE_LEN; i++) {
		// see if supported rate existed and matched
		rate = AP_BSSRATE[i] & 0x7f;
		if (match_supp_rate(pRate, *pLen, rate)) {
			if (!which) {
				if (AP_BSSRATE[i] & 0x80)
					rate |= 0x80;
			}
			else {
				for (j=0; j<*pLen; j++) {
					if (rate == (pRate[j] & 0x7f)) {
						if (pRate[j] & 0x80)
							rate |= 0x80;
						break;
					}
				}
			}
			found_rate[num++] = rate;
		}
	}

	if (which) {
		for (i=0; i<num; i++) {
			if (found_rate[i] & 0x80)
				break;
		}
		if (i == num) { // no any basic rates in found_rate
			j = 0;
			while(pRate[j] & 0x80)
				j++;
			memcpy(&(pRate[j]), found_rate, num);
			*pLen = j + num;
			return;
		}
	}

	memcpy(pRate, found_rate, num);
	*pLen = num;
}


 void update_support_rate(struct	stat_info *pstat, unsigned char* buf, int len)
{
	memset(pstat->bssrateset, 0, sizeof(pstat->bssrateset));
	pstat->bssratelen=len;
	memcpy(pstat->bssrateset, buf, len);
}


int isErpSta(struct	stat_info *pstat)
{
	int i, len=pstat->bssratelen;
	UINT8 *buf=pstat->bssrateset;

	for (i=0; i<len; i++) {
		if ( ((buf[i] & 0x7f) != 2) &&
				((buf[i] & 0x7f) != 4) &&
				((buf[i] & 0x7f) != 11) &&
				((buf[i] & 0x7f) != 22) )
			return 1;	// ERP sta existed
	}
	return 0;
}

void SelectLowestInitRate(struct rtl8192cd_priv *priv)
{
	struct stat_info	*pstat;
	struct list_head	*phead, *plist;

	if (priv->pmib->dot11StationConfigEntry.autoRate)
	{
		phead = &priv->asoc_list;
		plist = phead;

		while ((plist = asoc_list_get_next(priv, plist)) != phead)
		{
			pstat = list_entry(plist, struct stat_info, asoc_list);
			if (pstat->sta_in_firmware == 1 && (pstat->expire_to > 0))
			{

				if(pstat->current_tx_rate < priv->pshare->phw->LowestInitRate)
					 priv->pshare->phw->LowestInitRate = pstat->current_tx_rate;
			}
		}
	}
	else if (priv->pmib->dot11StationConfigEntry.fixedTxRate)
		priv->pshare->phw->LowestInitRate = priv->pmib->dot11StationConfigEntry.fixedTxRate;
	else
		priv->pshare->phw->LowestInitRate = _24M_RATE_;

}

/*----------------------------------------------------------------------------
index: the information element id index, limit is the limit for search
-----------------------------------------------------------------------------*/
/**
 *	@brief	Get Information Element
 *
 *		p (Find ID in limit)		\n
 *	+--- -+------------+-----+---	\n
 *	| ... | element ID | len |...	\n
 *	+--- -+------------+-----+---	\n
 *
 *	@param	pbuf	frame data for search
 *	@param	index	the information element id = index (search target)
 *	@param	limit	limit for search
 *
 *	@retval	p	pointer to element ID
 *	@retval	len	p(IE) len
 */
 unsigned char *get_ie(unsigned char *pbuf, int index, int *len, int limit)
{
	unsigned int tmp,i;
	unsigned char *p;

	if (limit < 1)
		return NULL;

	p = pbuf;
	i = 0;
	*len = 0;
	while(1)
	{
		if (*p == index)
		{
			*len = *(p + 1);
			return (p);
		}
		else
		{
			tmp = *(p + 1);
			p += (tmp + 2);
			i += (tmp + 2);
		}
		if (i >= limit)
			break;
	}
	return NULL;
}

/*----------------------------------------------------------------------------
index: the information element with specific oui, limit is the limit for search
-----------------------------------------------------------------------------*/
/**
 *	@brief	Get Information Element
 *
 *		p (Find ID in limit)		\n
 *	+--- -+------------+-----+---	\n
 *	| ... | element ID | len |...	\n
 *	+--- -+------------+-----+---	\n
 *
 *	@param	pbuf	frame data for search
 *	@param	oui		the information element of 221 with oui = oui (search target)
 *	@param	limit	limit for search
 *
 *	@retval	p	pointer to element ID
 *	@retval	len	p(IE) len
 */
unsigned char *get_oui(unsigned char *pbuf, unsigned char *oui, int *len, int limit)
{
	unsigned int tmp,i;
	unsigned char *p;

	if (limit < 1)
		return NULL;

	p = pbuf;
	i = 0;
	*len = 0;
	while(1)
	{
		if (*p == _VENDOR_SPECIFIC_IE_ && !memcmp(p+2,oui,3))
		{
			*len = *(p + 1);
			return (p);
		}
		else
		{
			tmp = *(p + 1);
			p += (tmp + 2);
			i += (tmp + 2);
		}
		if (i >= limit)
			break;
	}
	return NULL;
}

#ifdef RTL_WPA2
/*----------------------------------------------------------------------------
index: the information element id index, limit is the limit for search
-----------------------------------------------------------------------------*/
static unsigned char *get_rsn_ie(struct rtl8192cd_priv *priv, unsigned char *pbuf, int *len, int limit)
{
	unsigned char *p = NULL;

	if (priv->pmib->dot11RsnIE.rsnielen == 0)
		return NULL;

	if ((priv->pmib->dot11RsnIE.rsnie[0] == _RSN_IE_2_) ||
		((priv->pmib->dot11RsnIE.rsnielen > priv->pmib->dot11RsnIE.rsnie[1]) &&
			(priv->pmib->dot11RsnIE.rsnie[priv->pmib->dot11RsnIE.rsnie[1]+2] == _RSN_IE_2_))) {
		p = get_ie(pbuf, _RSN_IE_2_, len, limit);
		if (p != NULL)
			return p;
		else
			return get_ie(pbuf, _RSN_IE_1_, len, limit);
	}
	else {
		p = get_ie(pbuf, _RSN_IE_1_, len, limit);
		if (p != NULL)
			return p;
		else
			return get_ie(pbuf, _RSN_IE_2_, len, limit);
	}
}
#endif


/**
 *	@brief	Set Information Element
 *
 *	Difference between set_fixed_ie, reserve 2 Byte, for Element ID & length \n
 *	\n
 *	+-------+       +------------+--------+----------------+	\n
 *	| pbuf. | <---  | element ID | length |     source     |	\n
 *	+-------+       +------------+--------+----------------+	\n
 *
 *	@param pbuf		buffer(frame) for set
 *	@param index	IE element ID
 *	@param len		IE length content & set length
 *	@param source	IE data for buffer set
 *	@param frlen	total frame length
 *
 *	@retval	pbuf+len+2	pointer of buffer tail.(+2 because element ID and length total 2 bytes)
 */
 unsigned char *set_ie(unsigned char *pbuf, int index, unsigned int len, unsigned char *source,
				unsigned int *frlen)
{
	*pbuf = index;
	*(pbuf + 1) = len;
	if (len > 0)
		memcpy((void *)(pbuf + 2), (void *)source, len);
	*frlen = *frlen + (len + 2);
	return (pbuf + len + 2);
}


static __inline__ int set_virtual_bitmap(unsigned char *pbuf, unsigned int i)
{
	unsigned int r,s,t;

	r = (i >> 3) << 3;
	t = i - r;

	s = BIT(t);

	*(pbuf + (i >> 3)) |= s;

	return	(i >> 3);
}


static __inline__ unsigned char *update_tim(struct rtl8192cd_priv *priv,
				unsigned char *bcn_buf, unsigned int *frlen)
{
	unsigned int	pre_head;
	unsigned int	i, set_pvb;
	unsigned char	val8;
#if 1//!defined(SMP_SYNC) || (defined(CONFIG_USB_HCI) || defined(CONFIG_SDIO_HCI))
	unsigned long	flags;
#endif
	struct list_head	*plist, *phead;
	struct stat_info	*pstat;
	unsigned char	bitmap[(NUM_STAT/8)+1];
	unsigned char	*pbuf= bcn_buf;
	unsigned char	N1, N2, bitmap_offset;

    unsigned char Q_info[8];
    unsigned char b_calhwqnum = 0;
    if (IS_HAL_CHIP(priv))
        b_calhwqnum = 8;
#ifdef MULTI_STA_REFINE
	if( priv->pshare->paused_sta_num < b_calhwqnum)
#endif
	{
    if(b_calhwqnum == 8) {
        for(i=0; i<4; i++) {
			if (GET_CHIP_VER(priv) == VERSION_8814A || GET_CHIP_VER(priv) == VERSION_8822B || GET_CHIP_VER(priv) == VERSION_8197F) {
				if((RTL_R16(0x1400+i*2) & 0xFFF))
	                Q_info[i] = (RTL_R8(0x400+i*4+3)>>1) & 0x7f; //31:25     7b
	            else
	                Q_info[i] = 0;
	            if((RTL_R16(0x1408+i*2) & 0xFFF))
	                Q_info[i+4] = (RTL_R8(0x468+i*4+3)>>1) & 0x7f; //31:25     7b
	            else
	                Q_info[i+4] = 0;
			}
			else
			{
	            if(RTL_R8(0x400+i*4+1) & 0x7f) // 14:8     7b
	                Q_info[i] = (RTL_R8(0x400+i*4+3)>>1) & 0x7f; //31:25     7b
	            else
	                Q_info[i] = 0;
	            if(RTL_R8(0x468+i*4+1) & 0x7f) // 14:8     7b
	                Q_info[i+4] = (RTL_R8(0x468+i*4+3)>>1) & 0x7f; //31:25     7b
	            else
	                Q_info[i+4] = 0;
			}
        }
    }
    else if(b_calhwqnum == 4) {
        for(i=0; i<4; i++) {
            if(RTL_R8(0x400+i*4+1)){ // 15:8     7b
                Q_info[i] = (RTL_R8(0x400+i*4+3)>>2) & 0x3f; //31:26     7b
            }
            else
                Q_info[i] = 0;
        }

    }
	}

	memset(bitmap, 0, sizeof(bitmap));

#if 0//def MBSSID
	if (GET_ROOT(priv)->pmib->miscEntry.vap_enable && IS_VAP_INTERFACE(priv))
		priv->dtimcount = GET_ROOT(priv)->dtimcount;
	else
#endif
	{
		if (priv->dtimcount == 0)
			priv->dtimcount = (priv->pmib->dot11StationConfigEntry.dot11DTIMPeriod - 1);
		else
			priv->dtimcount--;
	}

	if (!IS_HAL_CHIP(priv))
	{
		//if (priv->pkt_in_dtimQ && (priv->dtimcount == 0))
		SMP_LOCK_XMIT(flags);
		pre_head = get_txhead(priv->pshare->phw, MCAST_QNUM);
		txdesc_rollback(&pre_head);
		SMP_UNLOCK_XMIT(flags);

		if (priv->dtimcount == (priv->pmib->dot11StationConfigEntry.dot11DTIMPeriod - 1)
			&&(*((unsigned char *)priv->beaconbuf + priv->timoffset + 4) & 0x01))  {
			RTL_W16(RD_CTRL, RTL_R16(RD_CTRL) & (~ HIQ_NO_LMT_EN));
		}
		if(priv->dtimcount == (priv->pmib->dot11StationConfigEntry.dot11DTIMPeriod - 1)
			&& !(get_desc((get_txdesc(priv->pshare->phw, MCAST_QNUM) + pre_head)->Dword0) & TX_OWN))
			priv->pkt_in_hiQ = 0;
	}

	if ((priv->dtimcount == 0) &&
		(priv->pkt_in_dtimQ ||
	//		(get_desc((get_txdesc(priv->pshare->phw, MCAST_QNUM) + pre_head)->Dword0) & TX_OWN)))
			priv->pkt_in_hiQ))
		val8 = 0x01;
	else
		val8 = 0x00;


    if (IS_HAL_CHIP(priv))
    {
        u1Byte	macID_list[NUM_AC_QUEUE];
        u1Byte	macID_index;
      	memset(macID_list, 0, NUM_AC_QUEUE);
        if(RT_STATUS_SUCCESS == GET_HAL_INTERFACE(priv)->GetMACIDQueueInTXPKTBUFHandler(priv,macID_list))
        {
            for(macID_index=0;macID_index<NUM_AC_QUEUE;macID_index++)
            {
                if(macID_list[macID_index]!=0)
                {
                    set_virtual_bitmap(bitmap, macID_list[macID_index]);
                }
            }
        }
    }

	*pbuf = _TIM_IE_;
	*(pbuf + 2) = priv->dtimcount;
	*(pbuf + 3) = priv->pmib->dot11StationConfigEntry.dot11DTIMPeriod;

	phead = &priv->sleep_list;

	SAVE_INT_AND_CLI(flags);
	SMP_LOCK_SLEEP_LIST(flags);

	plist = phead->next;
	while(plist != phead)
	{
		pstat = list_entry(plist, struct stat_info, sleep_list);
		plist = plist->next;
		set_pvb = 0;
#if defined(MULTI_STA_REFINE) && defined(SW_TX_QUEUE)
		if( priv->pshare->paused_sta_num >= b_calhwqnum) {
			set_pvb = 0x01;
		} else
#endif
		{
#if defined(WIFI_WMM)
		if(!isFFempty(pstat->MGT_dz_queue->head, pstat->MGT_dz_queue->tail))
			set_pvb++;
		else
#if defined(WMM_APSD)
		if ((QOS_ENABLE) && (APSD_ENABLE) && (pstat) && ((pstat->apsd_bitmap & 0x0f) == 0x0f) &&
			((!isFFempty(pstat->VO_dz_queue->head, pstat->VO_dz_queue->tail)) ||
			 (!isFFempty(pstat->VI_dz_queue->head, pstat->VI_dz_queue->tail)) ||
			 (!isFFempty(pstat->BE_dz_queue->head, pstat->BE_dz_queue->tail)) ||
			 (!isFFempty(pstat->BK_dz_queue->head, pstat->BK_dz_queue->tail)))) {
			set_pvb++;
		}
		else
#endif
#endif
		if (skb_queue_len(&pstat->dz_queue))
			set_pvb++;

        if(set_pvb == 0 && b_calhwqnum) {
            for(i=0; i<b_calhwqnum; i++) {
                if(REMAP_AID(pstat) == Q_info[i]) {
                    set_pvb++;
                    break;
                }
            }
        }
	}

		if (set_pvb) {
			i = pstat->aid;
			i = set_virtual_bitmap(bitmap, i);
		}
	}

	SMP_UNLOCK_SLEEP_LIST(flags);
	RESTORE_INT(flags);

	if(priv->pshare->rf_ft_var.wakeforce) {

		unsigned int idx, txtp = 0;
		idx = 0;

		pstat = findNextSTA(priv, &idx);

		while(pstat) {
				txtp = (unsigned int)(pstat->tx_avarage>>17);

				if(txtp >= priv->pshare->rf_ft_var.waketh) {

					if(priv->pshare->rf_ft_var.wakeforce == 1){
						val8 = 1;
						break;
					}
					else if(priv->pshare->rf_ft_var.wakeforce == 2){
						i = pstat->aid;
						i = set_virtual_bitmap(bitmap, i);
					}
				}

				pstat = findNextSTA(priv, &idx);
		}

	}

	N1 = 0;
	for(i=0; i<(NUM_STAT/8)+1; i++) {
		if(bitmap[i] != 0) {
			N1 = i;
			break;
		}
	}
	N2 = N1;
	for(i=(NUM_STAT/8); i>N1; i--) {
		if(bitmap[i] != 0) {
			N2 = i;
			break;
		}
	}

	// N1 should be an even number
	N1 = (N1 & 0x01)? (N1-1) : N1;
	bitmap_offset = N1 >> 1;	// == N1/2
	*(pbuf + 1) = N2 - N1 + 4;
	*(frlen) = *frlen + *(pbuf + 1) + 2;

	*(pbuf + 4) = val8 | (bitmap_offset << 1);
	memcpy((void *)(pbuf + 5), &bitmap[N1], (N2-N1+1));

	return (bcn_buf + *(pbuf + 1) + 2);
}

/**
 *	@brief	set fixed information element
 *
 *	set_fixed is haven't Element ID & length, Total length is frlen. \n
 *					 len	\n
 *	+-----------+-----------+	\n
 *	|	pbuf	|   source  |	\n
 *	+-----------+-----------+	\n
 *
 *	@param	pbuf	buffer(frame) for set
 *	@param	len		IE set length
 *	@param	source	IE data for buffer set
 *	@param	frlen	total frame length (Note: frlen have side effect??)
 *
 *	@retval	pbuf+len	pointer of buffer tail. \n
 */
 unsigned char *set_fixed_ie(unsigned char *pbuf, unsigned int len, unsigned char *source,
				unsigned int *frlen)
{
	memcpy((void *)pbuf, (void *)source, len);
	*frlen = *frlen + len;
	return (pbuf + len);
}

#if defined(HS2_SUPPORT) || defined(DOT11K)
static unsigned char * construct_BSS_load_ie(struct rtl8192cd_priv *priv, unsigned char	*pbuf, unsigned int *frlen)
{
    *(unsigned short *)priv->pmib->hs2Entry.bssload_ie = cpu_to_le16((unsigned short)priv->assoc_num);
    priv->pmib->hs2Entry.bssload_ie[2] = priv->ext_stats.ch_utilization;
    priv->pmib->hs2Entry.bssload_ie[3] = 0;
    priv->pmib->hs2Entry.bssload_ie[4] = 0;
    pbuf = set_ie(pbuf, _BSS_LOAD_IE_, 5, priv->pmib->hs2Entry.bssload_ie, frlen);
    return pbuf;
}
#endif

void construct_ht_ie(struct rtl8192cd_priv *priv, int use_40m, int offset)
{
	struct ht_cap_elmt	*ht_cap;
	struct ht_info_elmt	*ht_ie;
	int ch_offset;

	if (priv->ht_cap_len == 0) {
		unsigned int sup_mcs = get_supported_mcs(priv);
		// construct HT Capabilities element
		priv->ht_cap_len = sizeof(struct ht_cap_elmt);
		ht_cap = &priv->ht_cap_buf;
		memset(ht_cap, 0, sizeof(struct ht_cap_elmt));
		if (use_40m ==1 || use_40m ==2){
			ht_cap->ht_cap_info |= cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_);
		}
		ht_cap->ht_cap_info |= cpu_to_le16(_HTCAP_SMPWR_DISABLE_);
		ht_cap->ht_cap_info |= cpu_to_le16(priv->pmib->dot11nConfigEntry.dot11nShortGIfor20M? _HTCAP_SHORTGI_20M_ : 0);

		if (use_40m == 1 || use_40m ==2
		|| (priv->pmib->dot11nConfigEntry.dot11nUse40M==1)
		|| (priv->pmib->dot11nConfigEntry.dot11nUse40M==2)) {
			ht_cap->ht_cap_info |= cpu_to_le16(priv->pmib->dot11nConfigEntry.dot11nShortGIfor40M? _HTCAP_SHORTGI_40M_ : 0);
		}
		if ((get_rf_mimo_mode(priv) == MIMO_1T1R)
		)
			ht_cap->ht_cap_info |= cpu_to_le16(priv->pmib->dot11nConfigEntry.dot11nSTBC? _HTCAP_RX_STBC_1S_ : 0);
		else {
			//8822 A/B-cut disable RX STBC
			if(GET_CHIP_VER(priv) == VERSION_8822B && GET_CHIP_VER_8822(priv) <= 0x1)
				ht_cap->ht_cap_info |= cpu_to_le16(priv->pmib->dot11nConfigEntry.dot11nSTBC? _HTCAP_TX_STBC_ : 0);
			else
				ht_cap->ht_cap_info |= cpu_to_le16(priv->pmib->dot11nConfigEntry.dot11nSTBC? (_HTCAP_TX_STBC_ | _HTCAP_RX_STBC_1S_) : 0);
		}

		if (can_enable_rx_ldpc(priv))
		ht_cap->ht_cap_info |= cpu_to_le16((priv->pmib->dot11nConfigEntry.dot11nLDPC&1)? (_HTCAP_SUPPORT_RX_LDPC_) : 0);

		ht_cap->ht_cap_info |= cpu_to_le16(priv->pmib->dot11nConfigEntry.dot11nAMSDURecvMax? _HTCAP_AMSDU_LEN_8K_ : 0);
		ht_cap->ht_cap_info |= cpu_to_le16(_HTCAP_CCK_IN_40M_);
// 64k
		if (GET_CHIP_VER(priv)==VERSION_8192E) {
			if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm)
				ht_cap->ampdu_para = ((_HTCAP_AMPDU_SPC_16_US_ << _HTCAP_AMPDU_SPC_SHIFT_) | _HTCAP_AMPDU_FAC_64K_);
			else
				ht_cap->ampdu_para = ((_HTCAP_AMPDU_SPC_8_US_ << _HTCAP_AMPDU_SPC_SHIFT_) | _HTCAP_AMPDU_FAC_64K_);
		} else
		{
			if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm) {


				if (GET_CHIP_VER(priv)==VERSION_8822B) {

						ht_cap->ampdu_para = ((_HTCAP_AMPDU_SPC_2_US_ << _HTCAP_AMPDU_SPC_SHIFT_) | _HTCAP_AMPDU_FAC_32K_);
				}
				else

				ht_cap->ampdu_para = ((_HTCAP_AMPDU_SPC_16_US_ << _HTCAP_AMPDU_SPC_SHIFT_) | _HTCAP_AMPDU_FAC_32K_);
			}
			else {
#ifdef RTK_AC_SUPPORT //for 11ac logo
				if ((GET_CHIP_VER(priv) == VERSION_8812E) || (GET_CHIP_VER(priv) == VERSION_8881A) || (GET_CHIP_VER(priv) == VERSION_8814A)) {
					if((GET_CHIP_VER(priv) == VERSION_8814A) && priv->pmib->dot11nConfigEntry.dot11nUse40M== HT_CHANNEL_WIDTH_20)
						ht_cap->ampdu_para = ((_HTCAP_AMPDU_SPC_NORES_ << _HTCAP_AMPDU_SPC_SHIFT_) | _HTCAP_AMPDU_FAC_32K_);
					else
						ht_cap->ampdu_para = ((_HTCAP_AMPDU_SPC_NORES_ << _HTCAP_AMPDU_SPC_SHIFT_) | _HTCAP_AMPDU_FAC_64K_);
				} else
#endif
				ht_cap->ampdu_para = ((_HTCAP_AMPDU_SPC_8_US_ << _HTCAP_AMPDU_SPC_SHIFT_) | _HTCAP_AMPDU_FAC_32K_);
			}

		}
		ht_cap->support_mcs[0] = (sup_mcs & 0xff);
		ht_cap->support_mcs[1] = (sup_mcs & 0xff00) >> 8;
		ht_cap->support_mcs[2] = (sup_mcs & 0xff0000) >> 16;
		ht_cap->support_mcs[3] = (sup_mcs & 0xff000000) >> 24;
		ht_cap->ht_ext_cap = 0;
#if (BEAMFORMING_SUPPORT == 1)
		if ((priv->pshare->WlanSupportAbility & WLAN_BEAMFORMING_SUPPORT) && (priv->pmib->dot11RFEntry.txbf == 1)){
			if ((priv->pmib->dot11RFEntry.txbfer == 1) && (priv->pmib->dot11RFEntry.txbfee == 0))
				{
				if(GET_CHIP_VER(priv) == VERSION_8814A)
					ht_cap->txbf_cap = cpu_to_le32(0x18000410);
				else
					ht_cap->txbf_cap = cpu_to_le32(0x08000410);
				}
			else if ((priv->pmib->dot11RFEntry.txbfer == 0) && (priv->pmib->dot11RFEntry.txbfee == 1))
				{
				if(GET_CHIP_VER(priv) == VERSION_8814A)
					ht_cap->txbf_cap = cpu_to_le32(0x01810008);
				else
					ht_cap->txbf_cap = cpu_to_le32(0x00810008);
				}
			else
				{
				if(GET_CHIP_VER(priv) == VERSION_8814A)
					ht_cap->txbf_cap = cpu_to_le32(0x19810418);
				else
					ht_cap->txbf_cap = cpu_to_le32(0x08810418);
				}
		}
		else
#endif
		ht_cap->txbf_cap = 0;
		ht_cap->asel_cap = 0;

#ifdef CLIENT_MODE
		if ((OPMODE & WIFI_AP_STATE) || (OPMODE & WIFI_ADHOC_STATE))
#endif
		{
			// construct HT Information element
			priv->ht_ie_len = sizeof(struct ht_info_elmt);
			ht_ie = &priv->ht_ie_buf;
			memset(ht_ie, 0, sizeof(struct ht_info_elmt));
			ht_ie->primary_ch = priv->pmib->dot11RFEntry.dot11channel;
			if ((use_40m == 1) || (use_40m == 2)) {
				if (offset == HT_2NDCH_OFFSET_BELOW)
					ch_offset = _HTIE_2NDCH_OFFSET_BL_ | _HTIE_STA_CH_WDTH_;
				else
					ch_offset = _HTIE_2NDCH_OFFSET_AB_ | _HTIE_STA_CH_WDTH_;
			} else {
				ch_offset = _HTIE_2NDCH_OFFSET_NO_;
			}

			ht_ie->info0 |= ch_offset;
			ht_ie->info1 = 0;
			ht_ie->info2 = 0;
			ht_ie->basic_mcs[0] = (priv->pmib->dot11nConfigEntry.dot11nBasicMCS & 0xff);
			ht_ie->basic_mcs[1] = (priv->pmib->dot11nConfigEntry.dot11nBasicMCS & 0xff00) >> 8;
			ht_ie->basic_mcs[2] = (priv->pmib->dot11nConfigEntry.dot11nBasicMCS & 0xff0000) >> 16;
			ht_ie->basic_mcs[3] = (priv->pmib->dot11nConfigEntry.dot11nBasicMCS & 0xff000000) >> 24;
		}
	}
	else
#ifdef CLIENT_MODE
	if ((OPMODE & WIFI_AP_STATE) || (OPMODE & WIFI_ADHOC_STATE) )
#endif
	{
#ifdef WIFI_11N_2040_COEXIST
		if (priv->pmib->dot11nConfigEntry.dot11nCoexist && (OPMODE & WIFI_AP_STATE)) {
			ht_ie = &priv->ht_ie_buf;
			ht_ie->info0 &= ~(_HTIE_2NDCH_OFFSET_BL_ | _HTIE_STA_CH_WDTH_);
			if (use_40m && (!COEXIST_ENABLE || !(priv->bg_ap_timeout || orForce20_Switch20Map(priv))) ) {
				if (offset == HT_2NDCH_OFFSET_BELOW)
					ch_offset = _HTIE_2NDCH_OFFSET_BL_ | _HTIE_STA_CH_WDTH_;
				else
					ch_offset = _HTIE_2NDCH_OFFSET_AB_ | _HTIE_STA_CH_WDTH_;
			} else {
				ch_offset = _HTIE_2NDCH_OFFSET_NO_;
			}
			ht_ie->info0 |= ch_offset;
		}
#endif
		if (!priv->pmib->dot11StationConfigEntry.protectionDisabled) {
			if (priv->ht_legacy_obss_to || priv->ht_legacy_sta_num)
				priv->ht_protection = 1;
			else
				priv->ht_protection = 0;;
		}

		if (priv->ht_legacy_sta_num) {
			priv->ht_ie_buf.info1 |= cpu_to_le16(_HTIE_OP_MODE3_);
		} else if (priv->ht_legacy_obss_to || priv->ht_nomember_legacy_sta_to) {
			priv->ht_ie_buf.info1 &= cpu_to_le16(~_HTIE_OP_MODE3_);
			priv->ht_ie_buf.info1 |= cpu_to_le16(_HTIE_OP_MODE1_);
		} else {
			priv->ht_ie_buf.info1 &= cpu_to_le16(~_HTIE_OP_MODE3_);
		}

		if (priv->ht_protection)
			priv->ht_ie_buf.info1 |= cpu_to_le16(_HTIE_OBSS_NHT_STA_);
		else
			priv->ht_ie_buf.info1 &= cpu_to_le16(~_HTIE_OBSS_NHT_STA_);
	}
	if( IS_DRV_OPEN(GET_VXD_PRIV(GET_ROOT(priv)))) {
		ht_cap = &priv->ht_cap_buf;
		if( (GET_ROOT(priv)->pmib->dot11nConfigEntry.dot11nUse40M != HT_CHANNEL_WIDTH_10)
		&& (GET_ROOT(priv)->pmib->dot11nConfigEntry.dot11nUse40M != HT_CHANNEL_WIDTH_5))
			ht_cap->ht_cap_info |= cpu_to_le16( _HTCAP_SUPPORT_CH_WDTH_ | _HTCAP_SHORTGI_40M_);
	}
}


unsigned char *construct_ht_ie_old_form(struct rtl8192cd_priv *priv, unsigned char *pbuf, unsigned int *frlen)
{
	unsigned char old_ht_ie_id[] = {0x00, 0x90, 0x4c};

	*pbuf = _RSN_IE_1_;
	*(pbuf + 1) = 3 + 1 + priv->ht_cap_len;
	memcpy((pbuf + 2), old_ht_ie_id, 3);
	*(pbuf + 5) = 0x33;
	memcpy((pbuf + 6), (unsigned char *)&priv->ht_cap_buf, priv->ht_cap_len);
	*frlen += (*(pbuf + 1) + 2);
	pbuf +=(*(pbuf + 1) + 2);

	*pbuf = _RSN_IE_1_;
	*(pbuf + 1) = 3 + 1 + priv->ht_ie_len;
	memcpy((pbuf + 2), old_ht_ie_id, 3);
	*(pbuf + 5) = 0x34;
	memcpy((pbuf + 6), (unsigned char *)&priv->ht_ie_buf, priv->ht_ie_len);
	*frlen += (*(pbuf + 1) + 2);
	pbuf +=(*(pbuf + 1) + 2);

	return pbuf;
}


#ifdef WIFI_11N_2040_COEXIST
void construct_obss_scan_para_ie(struct rtl8192cd_priv *priv)
{
	struct obss_scan_para_elmt		*obss_scan_para;

	if (priv->obss_scan_para_len == 0) {
		priv->obss_scan_para_len = sizeof(struct obss_scan_para_elmt);
		obss_scan_para = &priv->obss_scan_para_buf;
		memset(obss_scan_para, 0, sizeof(struct obss_scan_para_elmt));

		// except word2, all are default values and meaningless for ap at present
		obss_scan_para->word0 = cpu_to_le16(0x14);
		obss_scan_para->word1 = cpu_to_le16(0x0a);
		obss_scan_para->word2 = cpu_to_le16(180);	// set as 180 second for 11n test plan
		obss_scan_para->word3 = cpu_to_le16(0xc8);
		obss_scan_para->word4 = cpu_to_le16(0x14);
		obss_scan_para->word5 = cpu_to_le16(5);
		obss_scan_para->word6 = cpu_to_le16(0x19);
	}
}

int bg_ap_rssi_chk(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo, int channel)
{
	int i;
	int ch_begin = -1;
	int ch_end = -1;
	int chk_l = 0;
	int chk_h = 0;

	for (i = 0; i < priv->available_chnl_num; i++)
	{
		if (priv->available_chnl[i] <= 14)
		{
			if (ch_begin == -1)
				ch_begin = priv->available_chnl[i];

			ch_end = priv->available_chnl[i];
		}
		else
			break;
	}


	if (priv->pmib->dot11RFEntry.dot11channel && (priv->pmib->dot11RFEntry.dot11channel <= 14))
	{
		if (priv->pshare->offset_2nd_chan == HT_2NDCH_OFFSET_ABOVE)
		{
			chk_l = priv->pmib->dot11RFEntry.dot11channel + 2 - 5;
			chk_h = priv->pmib->dot11RFEntry.dot11channel + 2 + 5;
		}

		if (priv->pshare->offset_2nd_chan == HT_2NDCH_OFFSET_BELOW)
		{
			chk_l = priv->pmib->dot11RFEntry.dot11channel - 2 - 5;
			chk_h = priv->pmib->dot11RFEntry.dot11channel - 2 + 5;
		}

		if (chk_l < ch_begin)
			chk_l = ch_begin;
		if (chk_h > ch_end)
			chk_h = ch_end;

		if ((channel >= chk_l) && (channel <= chk_h))
		{
			if (pfrinfo->rssi >= priv->pmib->dot11nConfigEntry.dot11nBGAPRssiChkTh)
				return 1;
		}
	}

	return 0;
}
#endif



#if 1
//#ifdef PCIE_POWER_SAVING
void fill_bcn_desc(struct rtl8192cd_priv *priv, struct tx_desc *pdesc, void *dat_content, unsigned short txLength, char forceUpdate)
{
/*
     * Intel IOT, dynamic enhance beacon tx AGC
    */
	if (priv->bcnTxAGC_bak != priv->bcnTxAGC || forceUpdate)
    {
		memset((void *)&pdesc->Dword6, 0, 4);

#ifdef HIGH_POWER_EXT_PA
	    if (!priv->pshare->rf_ft_var.use_ext_pa)
#endif
        if (priv->bcnTxAGC)
		{
			pdesc->Dword6 |= set_desc((((priv->bcnTxAGC*6) & 0xfffffffe) & TX_TxAgcAMask) << TX_TxAgcASHIFT);
			pdesc->Dword6 |= set_desc((((priv->bcnTxAGC*6) & 0xfffffffe) & TX_TxAgcBMask) << TX_TxAgcBSHIFT);
		}
        priv->bcnTxAGC_bak = priv->bcnTxAGC;
    }

	if (priv->pshare->is_40m_bw != priv->pshare->is_40m_bw_bak || forceUpdate) {
		memset((void *)&pdesc->Dword4, 0, 4);

		pdesc->Dword4 = set_desc(TX_DisDataFB | TX_UseRate);

		if (priv->pshare->is_40m_bw) {
			if (priv->pshare->offset_2nd_chan == HT_2NDCH_OFFSET_BELOW)
				pdesc->Dword4 |= set_desc(2 << TX_DataScSHIFT);
			else
				pdesc->Dword4 |= set_desc(1 << TX_DataScSHIFT);
		}
		priv->pshare->is_40m_bw_bak = priv->pshare->is_40m_bw;

	}

	if (txLength != priv->tx_beacon_len || forceUpdate)
	{
		memset(pdesc, 0, 24);

		memset((void *)&pdesc->Dword7, 0, 8);

		pdesc->Dword0 |= set_desc(TX_BMC|TX_FirstSeg | TX_LastSeg | ((32)<<TX_OffsetSHIFT));
		pdesc->Dword0 |= set_desc((unsigned short)(txLength) << TX_PktSizeSHIFT);
		pdesc->Dword1 |= set_desc(0x10 << TX_QSelSHIFT);

		if (priv->pmib->dot11RFEntry.bcn2path){
			RTL_W32(0x80c,RTL_R32(0x80c)|BIT(31));
			pdesc->Dword2 &= set_desc(0x03ffffff); // clear related bits
			pdesc->Dword2 |= set_desc(3 << TX_TxAntCckSHIFT);	// Set Default CCK rate with 2T
		}
		pdesc->Dword3 |= set_desc((GetSequence(dat_content) & TX_SeqMask) << TX_SeqSHIFT);
		pdesc->Dword4 = set_desc(TX_DisDataFB | TX_UseRate);

		if (priv->pshare->is_40m_bw)
        {
			if (priv->pshare->offset_2nd_chan == HT_2NDCH_OFFSET_BELOW)
				pdesc->Dword4 |= set_desc(2 << TX_DataScSHIFT);
			else
				pdesc->Dword4 |= set_desc(1 << TX_DataScSHIFT);
		}
        priv->pshare->is_40m_bw_bak = priv->pshare->is_40m_bw;

		if (priv->pmib->dot11RFEntry.txbf == 1) {
			pdesc->Dword2 &= set_desc(0x03ffffff); // clear related bits

			pdesc->Dword2 |= set_desc(1 << TX_TxAntCckSHIFT);	// Set Default CCK rate with 1T
			pdesc->Dword2 |= set_desc(1 << TX_TxAntlSHIFT); 	// Set Default Legacy rate with 1T
			pdesc->Dword2 |= set_desc(1 << TX_TxAntHtSHIFT);	// Set Default Ht rate
		}

		if (priv->pmib->dot11StationConfigEntry.beacon_rate != 0xff)
			pdesc->Dword5 |= set_desc((priv->pmib->dot11StationConfigEntry.beacon_rate & TX_DataRateMask) << TX_DataRateSHIFT);

		priv->tx_beacon_len = txLength;

		pdesc->Dword7 |= set_desc((unsigned short)(txLength) & TX_TxBufSizeMask);
		pdesc->Dword8 = set_desc(get_physical_addr(priv, dat_content, txLength, PCI_DMA_TODEVICE));
	}
	else
	{
		memset((void *)&pdesc->Dword3, 0, 4);
		pdesc->Dword3 |= set_desc((GetSequence(dat_content) & TX_SeqMask) << TX_SeqSHIFT);
	}

}
#endif

void signin_beacon_desc(struct rtl8192cd_priv *priv, unsigned int *beaconbuf, unsigned int frlen)
{
	struct rtl8192cd_hw	*phw=GET_HW(priv);
	struct tx_desc		*pdesc;
//	unsigned int			next_idx = 1;

	if (IS_VAP_INTERFACE(priv)) {
		pdesc = phw->tx_descB + priv->vap_init_seq;
//		next_idx =  priv->vap_init_seq + 1;
	}
	else
		pdesc = phw->tx_descB;

	//memset(pdesc, 0, 32);	// clear all bit

	if (!priv->pmib->dot11DFSEntry.disable_DFS &&
		(timer_pending(&GET_ROOT(priv)->ch_avail_chk_timer)) &&
		(GET_CHIP_VER(priv) == VERSION_8192D)) {
			pdesc->Dword0 &= set_desc(~(TX_OWN));
			RTL_W16(PCIE_CTRL_REG, RTL_R16(PCIE_CTRL_REG)| (BCNQSTOP));
		return;
	}


	fill_bcn_desc(priv, pdesc, (void*)beaconbuf, frlen, 0);

	rtl_cache_sync_wback(priv, (unsigned long)bus_to_virt(get_desc(pdesc->Dword8)), frlen, PCI_DMA_TODEVICE);


if((GET_CHIP_VER(priv)== VERSION_8188C) || (GET_CHIP_VER(priv)== VERSION_8192C) || (GET_CHIP_VER(priv)== VERSION_8192D))
{
	unsigned int tmp_522 = RTL_R8(0x522); //TXPAUSE register
	if (priv->pmib->miscEntry.func_off && IS_ROOT_INTERFACE(priv))
		RTL_W8(0x522, tmp_522 | BIT(6));
	else if(tmp_522 & BIT(6))
		RTL_W8(0x522, tmp_522 & ~BIT(6));
}

	pdesc->Dword0 |= set_desc(TX_OWN);
}



/**
 *	@brief	Update beacon content
 *
 *	IBSS parameter set (STA), TIM (AP), ERP & Ext rate not set  \n \n
 *	+----------------------------+-----+--------------------+-----+---------------------+-----+------------+-----+	\n
 *	| DS parameter (init_beacon) | TIM | IBSS parameter set | ERP | EXT supported rates | RSN | Realtek IE | CRC |	\n
 *	+----------------------------+-----+--------------------+-----+---------------------+-----+------------+-----+	\n
 *	\n
 *	set_desc() set data to hardware, 8190n_hw.h define value
 */


#if 1//def RTK_AC_SUPPORT //for 11ac logo
int get_center_channel(struct rtl8192cd_priv *priv, int channel, int offset, int cur)
{
	int val = channel, bw=0;

	if (cur)
		bw = priv->pshare->CurrentChannelBW;
	else
		bw = priv->pmib->dot11nConfigEntry.dot11nUse40M;

#ifdef RTK_AC_SUPPORT
	if (bw == HT_CHANNEL_WIDTH_80)
	{
#ifdef AC2G_256QAM
		if(is_ac2g(priv))
			val = 7;
		else
#endif
		if(channel <= 48)
			val = 42;
		else if(channel <= 64)
			val = 58;
		else if(channel <= 112)
			val = 106;
		else if(channel <= 128)
			val = 122;
		else if(channel <= 144)
			val = 138;
		else if(channel <= 161)
			val = 155;
		else if(channel <= 177)
			val = 171;
	} else
#endif
	if (bw == HT_CHANNEL_WIDTH_20_40) {
		if (offset == 1)
			val -= 2;
		else
			val += 2;
	}

	//SDEBUG("channel[%d] offset[%d] bw[%d] cent[%d]\n", channel, offset, bw, val);
	return val;
}


#endif


int is_main_AP_interface(struct rtl8192cd_priv *priv)
{
	int i;

	if (IS_ROOT_INTERFACE(priv)) {
		if (priv->pmib->miscEntry.func_off == 0)
			return TRUE;
		else
			return FALSE;
	}
	else {	// vap interfaces
		if (GET_ROOT(priv)->pmib->miscEntry.func_off == 0)
			return FALSE;
		else {
			for (i=0; i<RTL8192CD_NUM_VWLAN; i++) {
				if (IS_DRV_OPEN(GET_ROOT(priv)->pvap_priv[i])) {
					if ((GET_ROOT(priv)->pvap_priv[i])->pmib->miscEntry.func_off == 0) {
						if (priv == (GET_ROOT(priv)->pvap_priv[i]))
							return TRUE;
						break;
					}
				}
			}
			return FALSE;
		}
	}
}


void update_beacon(struct rtl8192cd_priv *priv)
{

    struct wifi_mib *pmib;
    struct rtl8192cd_hw	*phw;
    unsigned int	frlen;
    unsigned char	*pbuf;
    unsigned char	*pbssrate=NULL;
    int				bssrate_len;
#if defined(TV_MODE) || defined(A4_STA) || defined(USER_ADDIE)
    unsigned int	i;
#endif


	unsigned char extended_cap_ie[8];
	memset(extended_cap_ie,0,8);

	pmib = GET_MIB(priv);
	phw = GET_HW(priv);

	if(priv->hiddenAP_backup != priv->pmib->dot11OperationEntry.hiddenAP) {
		if(priv->pmib->dot11OperationEntry.hiddenAP)
			HideAP(priv);
		else
			DehideAP(priv);

		priv->hiddenAP_backup = priv->pmib->dot11OperationEntry.hiddenAP;
	}

    if (priv->update_bcn_period)
    {
        unsigned short val16 = 0;
        pbuf = (unsigned char *)priv->beaconbuf;
        frlen = 0;

        pbuf += 24;
        frlen += 24;

        frlen += _TIMESTAMP_;   // for timestamp
        pbuf += _TIMESTAMP_;

        //setup BeaconPeriod...
        val16 = cpu_to_le16(pmib->dot11StationConfigEntry.dot11BeaconPeriod);
        pbuf = set_fixed_ie(pbuf, _BEACON_ITERVAL_, (unsigned char *)&val16, &frlen);
        priv->update_bcn_period = 0;
    }
    frlen = priv->timoffset;
    pbuf = (unsigned char *)priv->beaconbuf + priv->timoffset;

    // setting tim field...
    if (OPMODE & WIFI_AP_STATE)
        pbuf = update_tim(priv, pbuf, &frlen);

    if (OPMODE & WIFI_ADHOC_STATE) {
        unsigned short val16 = 0;
        pbuf = set_ie(pbuf, _IBSS_PARA_IE_, 2, (unsigned char *)&val16, &frlen);
    }

#ifdef MCR_WIRELESS_EXTEND
	if (!priv->pshare->cmw_link)
#endif
	{
#if defined(DOT11D) || defined(DOT11H) || defined(DOT11K)
    if(priv->countryTableIdx)
    {
        pbuf = construct_country_ie(priv, pbuf, &frlen);
    }
#endif

#if defined(DOT11H) || defined(DOT11K)
    if(priv->pmib->dot11hTPCEntry.tpc_enable || priv->pmib->dot11StationConfigEntry.dot11RadioMeasurementActivated)
    {
        pbuf = set_ie(pbuf, _PWR_CONSTRAINT_IE_, 1, &priv->pshare->rf_ft_var.lpwrc, &frlen);
        pbuf = construct_TPC_report_ie(priv, pbuf, &frlen);
    }
#endif
	}
    if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) {
        // ERP infomation
        unsigned char val8 = 0;
        if (priv->pmib->dot11ErpInfo.protection)
            val8 |= BIT(1);
        if (priv->pmib->dot11ErpInfo.nonErpStaNum)
            val8 |= BIT(0);

        if (!SHORTPREAMBLE || priv->pmib->dot11ErpInfo.longPreambleStaNum)
            val8 |= BIT(2);

        pbuf = set_ie(pbuf, _ERPINFO_IE_, 1, &val8, &frlen);
    }

    // EXT supported rates
    if (get_bssrate_set(priv, _EXT_SUPPORTEDRATES_IE_, &pbssrate, &bssrate_len))
        pbuf = set_ie(pbuf, _EXT_SUPPORTEDRATES_IE_, bssrate_len, pbssrate, &frlen);

    #if defined(HS2_SUPPORT) || defined(DOT11K)
    if(	priv->pmib->dot11StationConfigEntry.cu_enable ){
        pbuf = construct_BSS_load_ie(priv, pbuf, &frlen);
    }
    #endif



    /*
        2008-12-16, For Buffalo WLI_CB_AG54L 54Mbps NIC interoperability issue.
        This NIC can not connect to our AP when our AP is set to WPA/TKIP encryption.
        This issue can be fixed after move "HT Capability Info" and "Additional HT Info" in front of "WPA" and "WMM".
    */
    if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
        construct_ht_ie(priv, priv->pshare->is_40m_bw, priv->pshare->offset_2nd_chan);
        pbuf = set_ie(pbuf, _HT_CAP_, priv->ht_cap_len, (unsigned char *)&priv->ht_cap_buf, &frlen);
        pbuf = set_ie(pbuf, _HT_IE_, priv->ht_ie_len, (unsigned char *)&priv->ht_ie_buf, &frlen);
    }
// beacon
#ifdef RTK_AC_SUPPORT //for 11ac logo
    if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC) {
#ifdef MCR_WIRELESS_EXTEND
		construct_vht_ie_mcr(priv, priv->pshare->working_channel, pstat);
#else
		construct_vht_ie(priv, priv->pshare->working_channel);
#endif
        pbuf = set_ie(pbuf, EID_VHTCapability, priv->vht_cap_len, (unsigned char *)&priv->vht_cap_buf, &frlen);
        pbuf = set_ie(pbuf, EID_VHTOperation, priv->vht_oper_len, (unsigned char *)&priv->vht_oper_buf, &frlen);
        // 62
        if(priv->pshare->rf_ft_var.lpwrc) {
            char tmp[4];
            pbuf = set_ie(pbuf, _PWR_CONSTRAINT_IE_, 1, &priv->pshare->rf_ft_var.lpwrc, &frlen);
            tmp[1] = tmp[2] = tmp[3] = priv->pshare->rf_ft_var.lpwrc;
            tmp[0] = priv->pshare->CurrentChannelBW;	//	20, 40, 80
            pbuf = set_ie(pbuf, EID_VHTTxPwrEnvelope, tmp[0]+2, tmp, &frlen);
        }
    }
#endif



// 63
    if (GET_ROOT(priv)->pmib->dot11DFSEntry.DFS_detected && priv->pshare->dfsSwitchChannel) {
        static unsigned int set_stop_bcn = 0;

        if (priv->pshare->dfsSwitchChCountDown) {
            unsigned char tmpStr[3];
            tmpStr[0] = 1;	/* channel switch mode */
            tmpStr[1] = priv->pshare->dfsSwitchChannel;	/* new channel number */
            tmpStr[2] = priv->pshare->dfsSwitchChCountDown;	/* channel switch count */
            pbuf = set_ie(pbuf, _CSA_IE_, 3, tmpStr, &frlen);



#if defined(RTK_AC_SUPPORT) //for 11ac logo
            if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC) {
                unsigned char tmp2[15];
                int len = 5;

                tmp2[0] = EID_WIDEBW_CH_SW;
                tmp2[1] = 3;
                tmp2[2] = (priv->pmib->dot11nConfigEntry.dot11nUse40M==2) ? 1 : 0;
                tmp2[3] = get_center_channel(priv, priv->pshare->dfsSwitchChannel, priv->pshare->offset_2nd_chan, 0);
                tmp2[4] = 0;
                pbuf = set_ie(pbuf, EID_CH_SW_WRAPPER, len, tmp2, &frlen);
            }
#endif


            //if (IS_ROOT_INTERFACE(priv))
            if (is_main_AP_interface(priv))
            {
                priv->pshare->dfsSwitchChCountDown--;
                if (set_stop_bcn)
                    set_stop_bcn = 0;
            }
        }
        else {
            //if (IS_ROOT_INTERFACE(priv))
            if (is_main_AP_interface(priv))
            {
                if ((GET_CHIP_VER(priv) == VERSION_8812E) || (GET_CHIP_VER(priv) == VERSION_8881A)|| (GET_CHIP_VER(priv) == VERSION_8814A) || (GET_CHIP_VER(priv) == VERSION_8822B)) {
                    if (GET_CHIP_VER(priv) == VERSION_8881A){
                        PHY_SetBBReg(priv, 0xcb0, 0x000000f0, 4);
                    }
                    SwitchChannel(GET_ROOT(priv));
                    if (GET_CHIP_VER(priv) == VERSION_8881A){
                        delay_us(500);
                        PHY_SetBBReg(priv, 0xcb0, 0x000000f0, 5);
                    }
                }
                else
                {
                    DFS_SwitchChannel(GET_ROOT(priv));
                }
            }
            else {
                if (!set_stop_bcn) {
                    if (IS_HAL_CHIP(priv)) {
                        u8  RegTXPause;
                        GET_HAL_INTERFACE(priv)->GetHwRegHandler(priv, HW_VAR_TXPAUSE, (pu1Byte)&RegTXPause);
                        RegTXPause |= TXPAUSE_BCN_QUEUE_BIT;
                        GET_HAL_INTERFACE(priv)->SetHwRegHandler(priv, HW_VAR_TXPAUSE, (pu1Byte)&RegTXPause);
                    } else if(CONFIG_WLAN_NOT_HAL_EXIST)
                    {//not HAL
                        RTL_W8(TXPAUSE, RTL_R8(TXPAUSE) | STOP_BCN);
                    }
                    set_stop_bcn++;
                }
            }
            return;
        }
    }

#ifdef RTK_AC_SUPPORT //for 11ac logo
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC) {
		//66
		if(priv->pshare->rf_ft_var.opmtest&1) {
			pbuf = set_ie(pbuf, EID_VHTOperatingMode, 1, (unsigned char *)&(priv->pshare->rf_ft_var.oper_mode_field), &frlen);
		}
	}
#endif

#ifdef WIFI_11N_2040_COEXIST
	if ((OPMODE & WIFI_AP_STATE) &&
		((priv->pmib->dot11BssType.net_work_type & (WIRELESS_11N|WIRELESS_11G))==(WIRELESS_11N|WIRELESS_11G)) &&
		COEXIST_ENABLE && priv->pshare->is_40m_bw) {

		extended_cap_ie[0] |=_2040_COEXIST_SUPPORT_ ;// byte0
		construct_obss_scan_para_ie(priv);
		pbuf = set_ie(pbuf, _OBSS_SCAN_PARA_IE_, priv->obss_scan_para_len,
			(unsigned char *)&priv->obss_scan_para_buf, &frlen);

	}
#endif

	if (pmib->dot11RsnIE.rsnielen) {
		memcpy(pbuf, pmib->dot11RsnIE.rsnie, pmib->dot11RsnIE.rsnielen);
		pbuf += pmib->dot11RsnIE.rsnielen;
		frlen += pmib->dot11RsnIE.rsnielen;
	}

#ifdef WIFI_WMM
	//Set WMM Parameter Element
	if (QOS_ENABLE)
		pbuf = set_ie(pbuf, _RSN_IE_1_, _WMM_Para_Element_Length_, GET_WMM_PARA_IE, &frlen);
#endif

#if defined(TV_MODE) || defined(A4_STA)
    i = 0;
#ifdef A4_STA
    if(priv->pshare->rf_ft_var.a4_enable == 2) {
        i |= BIT0;
    }
#endif

#ifdef TV_MODE
    i |= BIT1;
#endif
    if(i)
        pbuf = construct_ecm_tvm_ie(priv, pbuf, &frlen, i);
#endif //defined(TV_MODE) || defined(A4_STA)

	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
		/*
			2008-12-16, For Buffalo WLI_CB_AG54L 54Mbps NIC interoperability issue.
			This NIC can not connect to our AP when our AP is set to WPA/TKIP encryption.
			This issue can be fixed after move "HT Capability Info" and "Additional HT Info" in front of "WPA" and "WMM".
		 */
		//construct_ht_ie(priv, priv->pshare->is_40m_bw, priv->pshare->offset_2nd_chan);
		//pbuf = set_ie(pbuf, _HT_CAP_, priv->ht_cap_len, (unsigned char *)&priv->ht_cap_buf, &frlen);
		//pbuf = set_ie(pbuf, _HT_IE_, priv->ht_ie_len, (unsigned char *)&priv->ht_ie_buf, &frlen);
		//pbuf = construct_ht_ie_old_form(priv, pbuf, &frlen);
	}

	// Realtek proprietary IE
	if (priv->pshare->rtk_ie_len)
		pbuf = set_ie(pbuf, _RSN_IE_1_, priv->pshare->rtk_ie_len, priv->pshare->rtk_ie_buf, &frlen);

		// Customer proprietary IE
		if (priv->pmib->miscEntry.private_ie_len) {
			memcpy(pbuf, pmib->miscEntry.private_ie, pmib->miscEntry.private_ie_len);
			pbuf += pmib->miscEntry.private_ie_len;
			frlen += pmib->miscEntry.private_ie_len;
		}

	if(priv->pmib->miscEntry.stage) {
		unsigned char tmp[10] = { 0x00, 0x0d, 0x02, 0x07, 0x01, 0x00, 0x00 };
		if(priv->pmib->miscEntry.stage<=5)
			tmp[6] = 1<<(8-priv->pmib->miscEntry.stage);
		pbuf = set_ie(pbuf, _RSN_IE_1_, 7, tmp, &frlen);
	}

#ifdef WIFI_SIMPLE_CONFIG
/*modify for WPS2DOTX SUPPORT*/
	if (pmib->wscEntry.wsc_enable && pmib->wscEntry.beacon_ielen
		&& priv->pmib->dot11StationConfigEntry.dot11AclMode!=ACL_allow) {
		memcpy(pbuf, pmib->wscEntry.beacon_ie, pmib->wscEntry.beacon_ielen);
		pbuf += pmib->wscEntry.beacon_ielen;
		frlen += pmib->wscEntry.beacon_ielen;
	}
#endif

#ifdef HS2_SUPPORT
	// support hs2 enable, p2p disable
	if ((OPMODE & WIFI_AP_STATE) && priv->pmib->hs2Entry.hs2_ielen)
	{
		//unsigned char p2ptmpie[]={0x50,0x6f,0x9a,0x09,0x02,0x02,0x00,0x00,0x00};
		unsigned char p2ptmpie[]={0x50,0x6f,0x9a,0x09,0x0a,0x01,0x00,0x05};
		pbuf = set_ie(pbuf, 221, sizeof(p2ptmpie), p2ptmpie, &frlen);
	}
	if ((OPMODE & WIFI_AP_STATE) && priv->pmib->hs2Entry.hs2_ielen)
    {
        pbuf = set_ie(pbuf, _HS2_IE_, priv->pmib->hs2Entry.hs2_ielen, priv->pmib->hs2Entry.hs2_ie, &frlen);
    }
	if ((OPMODE & WIFI_AP_STATE) && priv->pmib->hs2Entry.interworking_ielen)
	{
		unsigned char magicmac[MACADDRLEN]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
		if (!memcmp(magicmac,priv->pmib->hs2Entry.interworking_ie+3,6))	{ // this case is used for HS2 testing (means MAC address of the AP)
			memcpy(priv->pmib->hs2Entry.interworking_ie+3, priv->pmib->dot11OperationEntry.hwaddr, MACADDRLEN);
			pbuf = set_ie(pbuf, _INTERWORKING_IE_, priv->pmib->hs2Entry.interworking_ielen, priv->pmib->hs2Entry.interworking_ie, &frlen);

		}
		else {
			pbuf = set_ie(pbuf, _INTERWORKING_IE_, priv->pmib->hs2Entry.interworking_ielen, priv->pmib->hs2Entry.interworking_ie, &frlen);
		}
	}
	if ((OPMODE & WIFI_AP_STATE) && priv->pmib->hs2Entry.advt_proto_ielen)
	{
		pbuf = set_ie(pbuf, _ADVT_PROTO_IE_, priv->pmib->hs2Entry.advt_proto_ielen, priv->pmib->hs2Entry.advt_proto_ie, &frlen);
	}
	if ((OPMODE & WIFI_AP_STATE) && priv->pmib->hs2Entry.MBSSID_ielen)
	{
		pbuf = set_ie(pbuf, _MUL_BSSID_IE_, priv->pmib->hs2Entry.MBSSID_ielen, priv->pmib->hs2Entry.MBSSID_ie, &frlen);
	}
	if ((OPMODE & WIFI_AP_STATE) && priv->pmib->hs2Entry.roam_ielen && priv->pmib->hs2Entry.roam_enable)
	{
		pbuf = set_ie(pbuf, _ROAM_IE_, priv->pmib->hs2Entry.roam_ielen, priv->pmib->hs2Entry.roam_ie, &frlen);
	}

	if (priv->dtimcount == 0)
    {
		if (priv->timeadvt_dtimcount == 0)
		{
			if ((OPMODE & WIFI_AP_STATE) && priv->pmib->hs2Entry.timeadvt_ielen)
			{
	            pbuf = set_ie(pbuf, _TIMEADVT_IE_, priv->pmib->hs2Entry.timeadvt_ielen, priv->pmib->hs2Entry.timeadvt_ie, &frlen);
			}
			if ((OPMODE & WIFI_AP_STATE) && priv->pmib->hs2Entry.timezone_ielen)
			{
	            pbuf = set_ie(pbuf, _TIMEZONE_IE_, priv->pmib->hs2Entry.timezone_ielen, priv->pmib->hs2Entry.timezone_ie, &frlen);
			}
			priv->timeadvt_dtimcount = priv->pmib->hs2Entry.timeadvt_DTIMIntval-1;
		}
		else
		{
			priv->timeadvt_dtimcount--;
		}
	}
#endif


#ifdef USER_ADDIE
	for (i=0; i<MAX_USER_IE; i++) {
		if (priv->user_ie_list[i].used) {
			memcpy(pbuf, priv->user_ie_list[i].ie, priv->user_ie_list[i].ie_len);
			pbuf += priv->user_ie_list[i].ie_len;
			frlen += priv->user_ie_list[i].ie_len;
		}
	}
#endif

#ifdef TDLS_SUPPORT
	if(priv->pmib->dot11OperationEntry.tdls_prohibited){
		extended_cap_ie[4] |= _TDLS_PROHIBITED_;	// bit 38(tdls_prohibited)
	}else{
		if(priv->pmib->dot11OperationEntry.tdls_cs_prohibited){
			extended_cap_ie[4] |= _TDLS_CS_PROHIBITED_;	// bit 39(tdls_cs_prohibited)
		}
	}
#endif
#ifdef CONFIG_IEEE80211V
	if(WNM_ENABLE)
		extended_cap_ie[2] |= _BSS_TRANSITION_ ; // bit 19
#endif

	#ifdef HS2_SUPPORT		/*process ext cap IE(IE ID=107) for HS2*/
	if (priv->pmib->hs2Entry.interworking_ielen)
	{
		// byte0
		if (priv->proxy_arp){
			extended_cap_ie[1] |=_PROXY_ARP_ ;
		}

		if ((priv->pmib->hs2Entry.timezone_ielen!=0) && (priv->pmib->hs2Entry.timeadvt_ielen)){
			extended_cap_ie[3] |= _UTC_TSF_OFFSET_ ;
		}
		extended_cap_ie[3] |= _INTERWORKING_SUPPORT_ ;
		//HS2_CHECK
		if(priv->pmib->hs2Entry.QoSMap_ielen[0] || priv->pmib->hs2Entry.QoSMap_ielen[1])
			extended_cap_ie[4] |= _QOS_MAP_; // QoS MAP (Bit32)

		extended_cap_ie[5] |= _WNM_NOTIFY_; //WNM notification, Bit46

	}
	#endif

	#ifdef RTK_AC_SUPPORT //for 11ac logo 4.2.55
		if((priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC))
		if((priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _WEP_40_PRIVACY_)
			&& (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _WEP_104_PRIVACY_))
		{
			//41 operting mode
			{
				extended_cap_ie[7] |= 0x40;
			}

		}
	#endif

	/* ext cap IE fill in here! so far (HS2, AC,TDLS ) */
	pbuf = set_ie(pbuf, _EXTENDED_CAP_IE_, 8, extended_cap_ie, &frlen);
	/*===========ext cap IE end=================*/
	/* Insert customized IE start*/
	if(priv->pptyIE) {
		unsigned char content[3+64+1];	//OUI+64-byte-content+1

		memcpy(content, priv->pptyIE->oui, 3);
		strcpy(content+3, priv->pptyIE->content);
		pbuf = set_ie(pbuf, priv->pptyIE->id, priv->pptyIE->length, content, &frlen);
	}
	/* Insert customized IE end*/
/*
	pdesc->Dword0 = set_desc(TX_FirstSeg| TX_LastSeg|  (32)<<TX_OffsetSHIFT | (frlen) << TX_PktSizeSHIFT);
	pdesc->Dword1 = set_desc(0x10 << TX_QSelSHIFT);
//		pdesc->Dword4 = set_desc((0x7 << TX_RaBRSRIDSHIFT) | TX_UseRate);	// need to confirm
	pdesc->Dword4 = set_desc(TX_UseRate);
	pdesc->Dword4 = set_desc(TX_DisDataFB);
	pdesc->Dword7 = set_desc(frlen & TX_TxBufSizeMask);
*/	// by signin_beacon_desc

//#ifdef CONFIG_RTL865X_AC
#if defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD) || defined(CONFIG_RTL8196C_KLD)
	priv->ext_stats.tx_byte_cnt += frlen;
#endif


        // if schedule off, don't send beacon
        if (priv->pmib->miscEntry.func_off)
        {
            if(!((GET_CHIP_VER(priv)== VERSION_8188C) || (GET_CHIP_VER(priv)== VERSION_8192C) || (GET_CHIP_VER(priv)== VERSION_8192D)))
                return;
            else if(!IS_ROOT_INTERFACE(priv))
                return;
        }

	if (!IS_DRV_OPEN(priv))
		return;

//		pdesc->Dword0 |= set_desc(TX_OWN);	// by signin_beacon_desc
	assign_wlanseq(phw, (unsigned char *)priv->beaconbuf, NULL ,pmib
		);
//		pdesc->Dword3 |= set_desc((GetSequence(priv->beaconbuf) & TX_SeqMask)<< TX_SeqSHIFT);	// by signin_beacon_desc
//		rtl_cache_sync_wback(priv, get_desc(pdesc->Dword8), 128*sizeof(unsigned int), PCI_DMA_TODEVICE);	// by signin_beacon_desc



#ifdef CONFIG_OFFLOAD_FUNCTION

	if((priv->offload_function_ctrl&1)) {
    // this section is for AP_OFFLOAD download
		unsigned char *prsp;
		const unsigned char txDesSize = 40;
        unsigned char pkt_offset = 0;
        unsigned int dummy = 0;
        unsigned int probePayloadLen = 0;
        unsigned int TotalPktLen = 0;

		unsigned int len	;
		len = frlen + txDesSize; // add beacon tx desc size ,
		// now len is represent from beacon desc + beacon payload

         dummy = GET_HAL_INTERFACE(priv)->GetRsvdPageLocHandler(priv,len,&pkt_offset);
        // dummy = bcn pkt_len + dummy

        priv->offload_bcn_page = RTL_R8(0x209);
        priv->offload_proc_page = RTL_R8(0x209) + pkt_offset;

#if (IS_RTL8197F_SERIES || IS_RTL8822B_SERIES)
        if ( IS_HARDWARE_TYPE_8197F(priv) || IS_HARDWARE_TYPE_8822B(priv)) { //3081
            priv->offload_bcn_page = RTL_R16(0x204)&0x0fff;
            priv->offload_proc_page = priv->offload_bcn_page + pkt_offset;
        }
#endif

        printk("priv->offload_bcn_page =%x \n",priv->offload_bcn_page);
        printk("priv->offload_proc_page =%x \n",priv->offload_proc_page);
        printk("dummy =%x \n",dummy);

		prsp = (unsigned char *)priv->beaconbuf  + dummy ;
		memset(prsp, 0, WLAN_HDR_A3_LEN);
		probePayloadLen = WLAN_HDR_A3_LEN + fill_probe_rsp_content(priv, prsp, prsp+WLAN_HDR_A3_LEN, SSID, SSID_LEN, 1, 0, 0);
        len = probePayloadLen;

		assign_wlanseq(phw, prsp, NULL ,pmib
		);

        TotalPktLen = probePayloadLen + dummy;


        GET_HAL_INTERFACE(priv)->SetRsvdPageHandler(priv,prsp,priv->beaconbuf,probePayloadLen,TotalPktLen,frlen);
        //delay_us(100);


        // TODO: currently, we do not implement this function, so mark it temporarily
		// 8814 merge issue
        //GET_HAL_INTERFACE(priv)->DownloadRsvdPageHandler(priv,priv->beaconbuf,frlen);
        priv->offload_function_ctrl = 0;
	}
    else
#endif //#ifdef CONFIG_OFFLOAD_FUNCTION
    {
	if (IS_HAL_CHIP(priv)) {
        GET_HAL_INTERFACE(priv)->SigninBeaconTXBDHandler(priv, priv->beaconbuf, frlen);
        GET_HAL_INTERFACE(priv)->SetBeaconDownloadHandler(priv, HW_VAR_BEACON_ENABLE_DOWNLOAD);
	} else if(CONFIG_WLAN_NOT_HAL_EXIST)
    {//not HAL
	    signin_beacon_desc(priv, priv->beaconbuf, frlen);
    }

	// Now we use sw beacon, we need to poll it every time.
	//RTL_W8(PCIE_CTRL_REG, BCNQ_POLL);
    }

}


void construct_ibss_beacon(struct rtl8192cd_priv *priv)
{
    unsigned short	val16;
    unsigned char	val8;
    struct wifi_mib *pmib;
    unsigned char	*bssid;

    unsigned int	frlen=0;
    unsigned char	*pbuf=(unsigned char *)priv->beaconbuf_ibss_vxd;
    unsigned char	*pbssrate=NULL;
    int	bssrate_len;
    struct FWtemplate *txfw;
    struct FWtemplate Temptxfw;

    unsigned int rate;


	pmib = GET_MIB(priv);
	bssid = pmib->dot11StationConfigEntry.dot11Bssid;



    {
        unsigned char tmpbssid[MACADDRLEN];

        unsigned char random;
        int i =0;

        memset(tmpbssid, 0, MACADDRLEN);
        if (!memcmp(BSSID, tmpbssid, MACADDRLEN)) {
            // generate an unique Ibss ssid
            get_random_bytes(&random, 1);
            tmpbssid[0] = 0x02;
            for (i=1; i<MACADDRLEN; i++)
                tmpbssid[i] = GET_MY_HWADDR[i-1] ^ GET_MY_HWADDR[i] ^ random;
            while(1) {
                for (i=0; i<priv->site_survey->count_target; i++) {
                    if (!memcmp(tmpbssid, priv->site_survey->bss_target[i].bssid, MACADDRLEN)) {
                        tmpbssid[5]++;
                        break;
                    }
                }
                if (i == priv->site_survey->count)
                    break;
            }
        }
    }

    memset(pbuf, 0, sizeof(priv->beaconbuf_ibss_vxd));

    txfw = &Temptxfw;
    rate = find_rate(priv, NULL, 0, 1);

    if (is_MCS_rate(rate)) {
        // can we use HT rates for beacon?
        txfw->txRate = rate & 0x7f;
        txfw->txHt = 1;
    }
    else {
        txfw->txRate = get_rate_index_from_ieee_value((UINT8)rate);
        if (priv->pshare->is_40m_bw) {
            if (priv->pshare->offset_2nd_chan == HT_2NDCH_OFFSET_BELOW)
                txfw->txSC = 2;
            else
                txfw->txSC = 1;
        }
    }

    SetFrameSubType(pbuf, WIFI_BEACON);

    memset((void *)GetAddr1Ptr(pbuf), 0xff, 6);
    memcpy((void *)GetAddr2Ptr(pbuf), GET_MY_HWADDR, 6);
    memcpy((void *)GetAddr3Ptr(pbuf), bssid, 6); // (Indeterminable set null mac (all zero)) (Refer: Draft 1.06, Page 12, 7.2.3, Line 21~30)

    pbuf += 24;
    frlen += 24;

    frlen += _TIMESTAMP_;	// for timestamp
    pbuf += _TIMESTAMP_;

    //setup BeaconPeriod...
    if(priv->beacon_period <= 0)
        priv->beacon_period	= pmib->dot11StationConfigEntry.dot11BeaconPeriod;

    if(priv->beacon_period)
        val16 = cpu_to_le16(priv->beacon_period);
    else
        val16 = cpu_to_le16(100);

    pbuf = set_fixed_ie(pbuf, _BEACON_ITERVAL_, (unsigned char *)&val16, &frlen);

    val16 = cpu_to_le16(BIT(1)); //IBSS


    if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm)
        val16 |= cpu_to_le16(BIT(4));

    if (SHORTPREAMBLE)
        val16 |= cpu_to_le16(BIT(5));
#ifdef RTK_AC_SUPPORT //for 11ac logo
    if(priv->pshare->rf_ft_var.lpwrc)
        val16 |= cpu_to_le16(BIT(8));	/* set spectrum mgt */
#endif


    pbuf = set_fixed_ie(pbuf, _CAPABILITY_, (unsigned char *)&val16, &frlen);
    priv->pBeaconCapability = (unsigned short *)(pbuf - _CAPABILITY_);

    if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) && (priv->pmib->dot11ErpInfo.shortSlot))
        SET_SHORTSLOT_IN_BEACON_CAP;
    else
        RESET_SHORTSLOT_IN_BEACON_CAP;

    //set ssid...
    pbuf = set_ie(pbuf, _SSID_IE_, SSID_LEN, SSID, &frlen);

    //supported rates...
    get_bssrate_set(priv, _SUPPORTEDRATES_IE_, &pbssrate, &bssrate_len);
    pbuf = set_ie(pbuf, _SUPPORTEDRATES_IE_, bssrate_len, pbssrate, &frlen);


    val16 = 0;
    pbuf = set_ie(pbuf, _IBSS_PARA_IE_, 2, (unsigned char *)&val16, &frlen);


#if defined(DOT11D) || defined(DOT11H) || defined(DOT11K)
    if(priv->countryTableIdx) {
        pbuf = construct_country_ie(priv, pbuf, &frlen);
    }
#endif

#if defined(DOT11H) || defined(DOT11K)
    if(priv->pmib->dot11hTPCEntry.tpc_enable || priv->pmib->dot11StationConfigEntry.dot11RadioMeasurementActivated) {
        pbuf = set_ie(pbuf, _PWR_CONSTRAINT_IE_, 1, &priv->pshare->rf_ft_var.lpwrc, &frlen);
        pbuf = construct_TPC_report_ie(priv, pbuf, &frlen);
    }
#endif

    if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) {
        // ERP infomation
        unsigned char val8 = 0;
        if (priv->pmib->dot11ErpInfo.protection)
            val8 |= BIT(1);
        if (priv->pmib->dot11ErpInfo.nonErpStaNum)
            val8 |= BIT(0);

        if (!SHORTPREAMBLE || priv->pmib->dot11ErpInfo.longPreambleStaNum)
            val8 |= BIT(2);

        pbuf = set_ie(pbuf, _ERPINFO_IE_, 1, &val8, &frlen);
    }

    // EXT supported rates
    if (get_bssrate_set(priv, _EXT_SUPPORTEDRATES_IE_, &pbssrate, &bssrate_len))
        pbuf = set_ie(pbuf, _EXT_SUPPORTEDRATES_IE_, bssrate_len, pbssrate, &frlen);


    if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
        construct_ht_ie(priv, priv->pshare->is_40m_bw, priv->pshare->offset_2nd_chan);
        pbuf = set_ie(pbuf, _HT_CAP_, priv->ht_cap_len, (unsigned char *)&priv->ht_cap_buf, &frlen);
        pbuf = set_ie(pbuf, _HT_IE_, priv->ht_ie_len, (unsigned char *)&priv->ht_ie_buf, &frlen);
    }
	// beacon
#ifdef RTK_AC_SUPPORT //for 11ac logo
    if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC) {
        // 41
        {
            char tmp[8];
            memset(tmp, 0, 8);
            tmp[7] = 0x40;
            pbuf = set_ie(pbuf, _EXTENDED_CAP_IE_, 8, tmp, &frlen);
        }
        // 60, 61
        construct_vht_ie(priv, priv->pshare->working_channel);
        pbuf = set_ie(pbuf, EID_VHTCapability, priv->vht_cap_len, (unsigned char *)&priv->vht_cap_buf, &frlen);
        pbuf = set_ie(pbuf, EID_VHTOperation, priv->vht_oper_len, (unsigned char *)&priv->vht_oper_buf, &frlen);
        // 62
        if(priv->pshare->rf_ft_var.lpwrc) {
            char tmp[4];
            pbuf = set_ie(pbuf, _PWR_CONSTRAINT_IE_, 1, &priv->pshare->rf_ft_var.lpwrc, &frlen);
            tmp[1] = tmp[2] = tmp[3] = priv->pshare->rf_ft_var.lpwrc;
            tmp[0] = priv->pshare->CurrentChannelBW;	//	20, 40, 80
            pbuf = set_ie(pbuf, EID_VHTTxPwrEnvelope, tmp[0]+2, tmp, &frlen);
        }
    }
#endif

#ifdef RTK_AC_SUPPORT //for 11ac logo
    if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC) {
        //66
        if(priv->pshare->rf_ft_var.opmtest&1) {
            pbuf = set_ie(pbuf, EID_VHTOperatingMode, 1, (unsigned char *)&(priv->pshare->rf_ft_var.oper_mode_field), &frlen);
        }
    }
#endif

    if (pmib->dot11RsnIE.rsnielen) {
        memcpy(pbuf, pmib->dot11RsnIE.rsnie, pmib->dot11RsnIE.rsnielen);
        pbuf += pmib->dot11RsnIE.rsnielen;
        frlen += pmib->dot11RsnIE.rsnielen;
    }

#ifdef WIFI_WMM
    //Set WMM Parameter Element
    if (QOS_ENABLE)
        pbuf = set_ie(pbuf, _RSN_IE_1_, _WMM_Para_Element_Length_, GET_WMM_PARA_IE, &frlen);
#endif

    //if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
    //    pbuf = construct_ht_ie_old_form(priv, pbuf, &frlen);
    //}

    // Realtek proprietary IE
    if (priv->pshare->rtk_ie_len)
        pbuf = set_ie(pbuf, _RSN_IE_1_, priv->pshare->rtk_ie_len, priv->pshare->rtk_ie_buf, &frlen);

    // Customer proprietary IE
    if (priv->pmib->miscEntry.private_ie_len) {
        memcpy(pbuf, pmib->miscEntry.private_ie, pmib->miscEntry.private_ie_len);
        pbuf += pmib->miscEntry.private_ie_len;
        frlen += pmib->miscEntry.private_ie_len;
    }

    priv->beaconbuf_ibss_vxd_len = frlen;

}


void issue_beacon_ibss_vxd(unsigned long task_priv)
{
	struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)task_priv;
	unsigned long flags;
	DECLARE_TXINSN(txinsn);

	if (!(priv->drv_state & DRV_STATE_OPEN))
		return;

	SAVE_INT_AND_CLI(flags);
	SMP_LOCK(flags);

	if(priv->beaconbuf_ibss_vxd_len<=0)
		goto issue_beacon_ibss_done;

	if(!netif_running(priv->dev))
		goto issue_beacon_ibss_done;

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
	txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif
	txinsn.fixed_rate = 1;
	txinsn.phdr = get_wlanhdr_from_poll(priv);
	txinsn.pframe = get_mgtbuf_from_poll(priv);

	if (txinsn.phdr == NULL)
		goto issue_beacon_ibss_fail;

	if (txinsn.pframe == NULL)
		goto issue_beacon_ibss_fail;

//fill MAC header
	memset((void *)(txinsn.phdr), 0, sizeof (struct	wlan_hdr));

	SetFrameSubType(txinsn.phdr, WIFI_BEACON);
	memset((void *)GetAddr1Ptr((txinsn.phdr)), 0xff, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);

//fill Beacon frames
	txinsn.fr_len = (priv->beaconbuf_ibss_vxd_len-24);

	if(txinsn.fr_len > 0)
		memcpy((unsigned char *)txinsn.pframe, (((unsigned char *)priv->beaconbuf_ibss_vxd)+24) , txinsn.fr_len);
	else
		goto issue_beacon_ibss_fail;

	if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS)
		goto issue_beacon_ibss_done;

issue_beacon_ibss_fail:
	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);

issue_beacon_ibss_done:
	RESTORE_INT(flags);
	SMP_UNLOCK(flags);

	mod_timer(&priv->pshare->vxd_ibss_beacon, jiffies + RTL_MILISECONDS_TO_JIFFIES(500));

}




/**
 *	@brief	Initial beacon
 *
  *	Refer wifi.h and 8190mib.h about MIB define.	\n
 *  Refer 802.11 7,3,13 Beacon interval field	\n
 *	- Timestamp \n - Beacon interval \n - Capability \n - SSID \n - Support rate \n - DS Parameter set \n \n
 *	+-------+-------+-------+	\n
 *	| addr1 | addr2 | addr3 |	\n
 *	+-------+-------+-------+	\n
 *
 *	+-----------+-----------------+------------+------+--------------+------------------+	\n
 *	| Timestamp | Beacon interval | Capability | SSID | Support rate | DS Parameter set |	\n
 *	+-----------+-----------------+------------+------+--------------+------------------+	\n
 *	[Note] \n
 *	abridge FH (unused), CF (AP not support PCF), \n
 *	IBSS parameter set (STA), DTIM (AP), ERP ??Ext rate  IE complete in Update beacon.\n
 *	set_desc is important.
 */

void init_beacon(struct rtl8192cd_priv *priv)
{
    unsigned short	val16;
    unsigned char	val8;
    struct wifi_mib *pmib;
    unsigned char	*bssid;
    //		struct tx_desc		*pdesc;
    //		struct rtl8192cd_hw	*phw=GET_HW(priv);
    //		int next_idx = 1;

    unsigned int	frlen=0;
    unsigned char	*pbuf=(unsigned char *)priv->beaconbuf;
    unsigned char	*pbssrate=NULL;
    int	bssrate_len;
    struct FWtemplate *txfw;
    struct FWtemplate Temptxfw;

    unsigned int rate;

	if(IS_VXD_INTERFACE(priv) && (OPMODE & WIFI_ADHOC_STATE))
		return;

    pmib = GET_MIB(priv);
    bssid = pmib->dot11StationConfigEntry.dot11Bssid;

    //memset(pbuf, 0, 128*4);
    memset(pbuf, 0, sizeof(priv->beaconbuf));
    txfw = &Temptxfw;

        rate = find_rate(priv, NULL, 0, 1);

    if (is_MCS_rate(rate)) {
        // can we use HT rates for beacon?
        txfw->txRate = rate - HT_RATE_ID;
        txfw->txHt = 1;
    }
    else {
        txfw->txRate = get_rate_index_from_ieee_value((UINT8)rate);
        if (priv->pshare->is_40m_bw) {
            if (priv->pshare->offset_2nd_chan == HT_2NDCH_OFFSET_BELOW)
                txfw->txSC = 2;
            else
                txfw->txSC = 1;
        }
    }

    SetFrameSubType(pbuf, WIFI_BEACON);

    memset((void *)GetAddr1Ptr(pbuf), 0xff, 6);
    memcpy((void *)GetAddr2Ptr(pbuf), GET_MY_HWADDR, 6);
    memcpy((void *)GetAddr3Ptr(pbuf), bssid, 6); // (Indeterminable set null mac (all zero)) (Refer: Draft 1.06, Page 12, 7.2.3, Line 21~30)

    pbuf += 24;
    frlen += 24;

    frlen += _TIMESTAMP_;	// for timestamp
    pbuf += _TIMESTAMP_;

    //setup BeaconPeriod...
    val16 = cpu_to_le16(pmib->dot11StationConfigEntry.dot11BeaconPeriod);
#ifdef CLIENT_MODE
    if (OPMODE & WIFI_ADHOC_STATE)
        val16 = cpu_to_le16(priv->beacon_period);
#endif
    pbuf = set_fixed_ie(pbuf, _BEACON_ITERVAL_, (unsigned char *)&val16, &frlen);

    {
        if (OPMODE & WIFI_AP_STATE)
            val16 = cpu_to_le16(BIT(0)); //ESS
        else
            val16 = cpu_to_le16(BIT(1)); //IBSS
    }

    if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm)
        val16 |= cpu_to_le16(BIT(4));

    if (SHORTPREAMBLE)
        val16 |= cpu_to_le16(BIT(5));

#ifdef DOT11H
    if(priv->pmib->dot11hTPCEntry.tpc_enable)
        val16 |= cpu_to_le16(BIT(8));	/* set spectrum mgt */
#endif


#if defined(WMM_APSD)
	if (APSD_ENABLE)
		val16 |= cpu_to_le16(BIT(11));
#endif

    pbuf = set_fixed_ie(pbuf, _CAPABILITY_, (unsigned char *)&val16, &frlen);
    priv->pBeaconCapability = (unsigned short *)(pbuf - _CAPABILITY_);

    if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) && (priv->pmib->dot11ErpInfo.shortSlot))
        SET_SHORTSLOT_IN_BEACON_CAP;
    else
        RESET_SHORTSLOT_IN_BEACON_CAP;

    //set ssid...
#ifdef WIFI_SIMPLE_CONFIG
    priv->pbeacon_ssid = pbuf;
#endif
    {
        if (!HIDDEN_AP && pmib->miscEntry.raku_only == 0)
            pbuf = set_ie(pbuf, _SSID_IE_, SSID_LEN, SSID, &frlen);
        else {
            if (HIDDEN_AP == 2) {
                pbuf = set_ie(pbuf, _SSID_IE_, 0, NULL, &frlen);
            } else {
                unsigned char ssidbuf[32];
                memset(ssidbuf, 0, 32);
                pbuf = set_ie(pbuf, _SSID_IE_, SSID_LEN, ssidbuf, &frlen);
            }
        }
    }

	//supported rates...
/*cfg p2p cfg p2p*/
        get_bssrate_set(priv, _SUPPORTEDRATES_IE_, &pbssrate, &bssrate_len);
    pbuf = set_ie(pbuf, _SUPPORTEDRATES_IE_, bssrate_len, pbssrate, &frlen);

    //ds parameter set...
    val8 = pmib->dot11RFEntry.dot11channel;
#if defined(RTK_5G_SUPPORT)
    if ( priv->pmib->dot11RFEntry.phyBandSelect & PHY_BAND_2G)
#endif
        pbuf = set_ie(pbuf, _DSSET_IE_, 1, &val8, &frlen);
    priv->timoffset = frlen;
    //		pdesc = phw->tx_descB;	// by signin_beacon_desc
    // clear all bit
    //		memset(pdesc, 0, 32);	// by signin_beacon_desc

    //		pdesc->Dword4 |= set_desc(0x08 << TX_RtsRateSHIFT);	// by signin_beacon_desc
    //		pdesc->Dword8 = set_desc(get_physical_addr(priv, priv->beaconbuf, 128*sizeof(unsigned int), PCI_DMA_TODEVICE));	// by signin_beacon_desc

    // next pointer should point to a descriptor, david
    //set NextDescAddress
    //		pdesc->Dword10 = set_desc(get_physical_addr(priv, &phw->tx_descB[next_idx], sizeof(struct tx_desc), PCI_DMA_TODEVICE));	// by signin_beacon_desc

    if (priv->pmib->dot11StationConfigEntry.beacon_rate != 0xff) {
        if (priv->pmib->dot11StationConfigEntry.beacon_rate > 11)
            panic_printk("[WARN] Beacon rate is not legacy rate!\n");
        if ((priv->pmib->dot11RFEntry.phyBandSelect & PHY_BAND_5G) &&
                (priv->pmib->dot11StationConfigEntry.beacon_rate < 4)) {
            panic_printk("[WARN] Beacon rate is CCK in 5G! Correct to OFDM rate\n");
            priv->pmib->dot11StationConfigEntry.beacon_rate = 4;
        }
    }

    update_beacon(priv);

    // enable tx bcn
    //#if defined(UNIVERSAL_REPEATER) || defined(MBSSID)
    //		if (IS_ROOT_INTERFACE(priv))
    //#endif
    //			RTL_W8(BCN_CTRL, EN_BCN_FUNCTION);

    if (IS_HAL_CHIP(priv)) {
        GET_HAL_INTERFACE(priv)->TxPollingHandler(priv, TXPOLL_BEACON_QUEUE);
    } else
    {
    // use hw beacon
        RTL_W8(PCIE_CTRL_REG, BCNQ_POLL);
    }

#ifndef DRVMAC_LB
    if (IS_HAL_CHIP(priv)) {
        GET_HAL_INTERFACE(priv)->SetHwRegHandler(priv, HW_VAR_ENABLE_BEACON_DMA, NULL);
    } else if(CONFIG_WLAN_NOT_HAL_EXIST)
    {//not HAL
        RTL_W16(PCIE_CTRL_REG, RTL_R16(PCIE_CTRL_REG)& (~ BCNQSTOP) );
    }
#endif


}


#ifndef CONFIG_RTL_NEW_AUTOCH
static void setChannelScore(int number, unsigned int *val, int min, int max)
{
	int i=0, score;

	if (number > max)
		return;

	*(val + number) += 5;

	if (number > min) {
		for (i=number-1, score=4; i>=min && score; i--, score--) {
			*(val + i) += score;
		}
	}
	if (number < max) {
		for (i=number+1, score=4; i<=max && score; i++, score--) {
			*(val +i) += score;
		}
	}
}
#endif


#if defined(CONFIG_RTL_NEW_AUTOCH) && defined(SS_CH_LOAD_PROC)
static void record_SS_report(struct rtl8192cd_priv *priv)
{
	int i, j;
	priv->ch_ss_rpt_cnt = priv->site_survey->count;
	memset(priv->ch_ss_rpt, 0, (sizeof(struct ss_report)*MAX_BSS_NUM));

	for(i=0; i<priv->site_survey->count ;i++){
		priv->ch_ss_rpt[i].channel = priv->site_survey->bss[i].channel;
		priv->ch_ss_rpt[i].is40M = ((priv->site_survey->bss[i].t_stamp[1] & BIT(1)) ? 1 : 0);
		priv->ch_ss_rpt[i].rssi	= priv->site_survey->bss[i].rssi;
		for (j=0; j<priv->available_chnl_num; j++) {
			if (priv->ch_ss_rpt[i].channel == priv->available_chnl[j]) {
				priv->ch_ss_rpt[i].fa_count = priv->chnl_ss_fa_count[j];
				priv->ch_ss_rpt[i].cca_count = priv->chnl_ss_cca_count[j];
				priv->ch_ss_rpt[i].ch_load = priv->chnl_ss_load[j];
			}
		}
	}
}
#endif

struct ap_info {
	unsigned char ch;
	unsigned char bw;
	unsigned char rssi;
};

#define MASK_CH(_ch, _begin, _end) if ((_ch) >= (_begin) && (_ch) < (_end)) score[_ch] = 0xffffffff;
int find_clean_channel(struct rtl8192cd_priv *priv, unsigned int begin, unsigned int end, unsigned int *score)
{
	struct bss_desc *pBss=NULL;
	int i, j, y, found;
	int ap_count[MAX_CHANNEL_NUM];
	struct ap_info ap_rec[MAX_BSS_NUM];
	unsigned int ap_rec_num = 0;

	memset(ap_count, 0, sizeof(ap_count));
	memset(ap_rec, 0, sizeof(ap_rec));

	for (y=begin; y<end; y++) {
		score[y] = priv->chnl_ss_fa_count[y];
#ifdef _DEBUG_RTL8192CD_
		printk("ch %d: FA: %d\n", y+1, score[y]);
#endif
	}

	for (i=0; i<priv->site_survey->count; i++) {
		pBss = &priv->site_survey->bss[i];

		ap_rec[ap_rec_num].ch = pBss->channel;
		if ((pBss->t_stamp[1] & 0x6) == 0) ap_rec[ap_rec_num].bw = 0;
		else if ((pBss->t_stamp[1] & 0x4) == 0) ap_rec[ap_rec_num].bw = 1;
		else ap_rec[ap_rec_num].bw = 2;
		ap_rec[ap_rec_num++].rssi = pBss->rssi;

		if (pBss->rssi > 15) {
			for (y=begin; y<end; y++) {
				if (pBss->channel == priv->available_chnl[y]) {
					ap_count[y]++;
					if ((pBss->t_stamp[1] & 0x6) == 0) {  // 20M
						for (j=-2; j<=2; j++)
							MASK_CH(y+j, begin, end);
					}
					else if ((pBss->t_stamp[1] & 0x4) == 0) {  // 40M upper
						for (j=-2; j<=6; j++)
							MASK_CH(y+j, begin, end);
					}
					else {  // 40M lower
						for (j=-6; j<=2; j++)
							MASK_CH(y+j, begin, end);
					}
				}
			}
		}
	}

	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)
		&& priv->pshare->is_40m_bw) {
		for (y=begin; y<end; y++)
			if (priv->available_chnl[y] == 14)
				score[y] = 0xffffffff;		// mask chan14
	}

	if (priv->pmib->dot11RFEntry.disable_ch1213) {
		for (y=begin; y<end; y++) {
			int ch = priv->available_chnl[y];
			if ((ch == 12) || (ch == 13))
				score[y] = 0xffffffff;
		}
	}

	if (((priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_GLOBAL) ||
			(priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_WORLD_WIDE)) &&
		 (end >= 11) && (end < 14)) {
		score[13] = 0xffffffff;	// mask chan14
		score[12] = 0xffffffff; // mask chan13
		score[11] = 0xffffffff; // mask chan12
	}

#ifdef _DEBUG_RTL8192CD_
	for (y=begin; y<end; y++) {
		if (score[y] == 0xffffffff)
			printk("ch %d: ap_count: %d, score: 0xffffffff ", y+1, ap_count[y]);
		else
			printk("ch %d: ap_count: %d, score: %d ", y+1, ap_count[y], score[y]);

		for (i=0; i<ap_rec_num; i++) {
			if (priv->available_chnl[y] == ap_rec[i].ch) {
				printk("%s:%d ", ap_rec[i].bw==0?"N":(ap_rec[i].bw==1?"U":"L"), ap_rec[i].rssi);
			}
		}
		printk("\n");
	}
#endif

	for (y=begin; y<end; y++) {
		found = 1;
		if (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_20_40)
			i = 4;
		else
			i = 2;

		for (j=-i; j<=i; j++) {
			if ((y+j >= begin) && (y+j < end)) {
				if (score[y+j] == 0xffffffff) {
					found = 0;
					break;
				}
			}
		}

		if (found)
			return 1;
	}

	return 0;
}

#ifdef NHM_ACS2_SUPPORT
unsigned int CH_20m[MAX_NUM_20M_CH] = {36,40,44,48, 52,56,60,64, 100,104,108,112, 116,120,124,128, 132,136,140,144, 149,153,157,161, 165,169,173,177};
int find_interf_ap_count_acs2(struct rtl8192cd_priv *priv, unsigned int begin, unsigned int end, unsigned int *ap_count)
{
	struct bss_desc *pBss=NULL;
	int i, y;

	if (ap_count == NULL)
	{
		printk("[ACS2] error : ap_count is NULL\n");
		return -1;
	}

	if (priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_2G)
	{
		if (end > 14)
		{
			printk("[ACS2] wrong para => begin: %d, end: %d\n", begin, end);
			return -1;
		}

		for (i=0; i<priv->site_survey->count; i++)
		{
			pBss = &priv->site_survey->bss[i];
			//if (pBss->rssi > 15)
			{
				for (y=begin; y<end; y++)
				{
					if (pBss->channel == priv->available_chnl[y])
					{
						ap_count[y]++;
						if (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_20_40)
						{
							if ((y + 4) < end)
								ap_count[y + 4]++;
							if ((y - 4) > 0)
								ap_count[y - 4]++;
							if ((pBss->t_stamp[1] & 0x6) == 0)  // 20M
							{
								// do nothing
							}
							else if ((pBss->t_stamp[1] & 0x4) == 0)  // 40M upper
							{
								if ((y + 4 + 4) < end)
									ap_count[y + 4 + 4]++;
							}
							else  // 40M lower
							{
								if ((y - 4 - 4) > 0)
									ap_count[y - 4 - 4]++;
							}
						}
						else
						{
							if ((pBss->t_stamp[1] & 0x6) == 0)  // 20M
							{
								// do nothing
							}
							else if ((pBss->t_stamp[1] & 0x4) == 0)  // 40M upper
							{
								if ((y + 4) < end)
									ap_count[y + 4]++;
							}
							else  // 40M lower
							{
								if ((y - 4) > 0)
									ap_count[y - 4]++;
							}
						}
					}
				}
			}
		}

		return 0;
	}
	else if (priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_5G)
	{
		unsigned int ap_count_tmp[MAX_NUM_20M_CH];
		int j, k;

		if (end > MAX_NUM_20M_CH)
		{
			printk("[ACS2] wrong para => begin: %d, end: %d\n", begin, end);
			return -1;
		}

		memset(ap_count_tmp, 0, sizeof(ap_count_tmp));

		for (i = 0; i < priv->site_survey->count; i++)
		{
			pBss = &priv->site_survey->bss[i];
			for (y = 0; y < MAX_NUM_20M_CH; y++)
			{
				if (pBss->channel == CH_20m[y])
				{
					ACSDEBUG("(%02d) CH : %3d, pBss->t_stamp[1] : 0x%04x, ", i, CH_20m[y], pBss->t_stamp[1]);

					ap_count_tmp[y]++;
					if ((pBss->t_stamp[1] & 0x6) == 0)	// 20M
					{
						// do nothing
						ACSDEBUG("BW : 20M\n");
					}
					else if	((pBss->t_stamp[1] & (BSS_BW_MASK << BSS_BW_SHIFT))	== (HT_CHANNEL_WIDTH_80 << BSS_BW_SHIFT))  // 80M
					{
						k = (y / 4) * 4;
						for (j = k; j < (k + 4); j++)
						{
							if ((j != y) && (j < MAX_NUM_20M_CH))
								ap_count_tmp[j]++;
						}
						ACSDEBUG("BW : 80M\n");
					}
					else if ((pBss->t_stamp[1] & 0x4) == 0)  // 40M upper
					{
						if ((y + 1) < MAX_NUM_20M_CH)
							ap_count_tmp[y + 1]++;
						ACSDEBUG("BW : 40M (U)\n");
					}
					else  // 40M lower
					{
						if ((y - 1) >= 0)
							ap_count_tmp[y - 1]++;
						ACSDEBUG("BW : 40M (L)\n");
					}
				}
			}
		}

		for (i = begin; i < end; i++)
		{
			for (y = 0; y < MAX_NUM_20M_CH; y++)
			{
				if (priv->available_chnl[i] == CH_20m[y])
				{
					ap_count[i] = ap_count_tmp[y];
				}
			}
		}

		//for (i = 0; i < MAX_NUM_20M_CH; i++)
		//	printk("CH : %3d, ap_count_tmp : %d\n", CH_20m[i], ap_count_tmp[i]);

		for (i = begin; i < end; i++)
			ACSDEBUG("CH : %3d, ap_interf_count : %d\n", priv->available_chnl[i], ap_count[i]);
	}

	return 0;
}

int find_clean_channel_acs2(struct rtl8192cd_priv *priv, unsigned int begin, unsigned int end, unsigned int *nhm_cnt_exp_sum, unsigned int *ap_count)
{
	struct bss_desc *pBss=NULL;
	int i, j, y, found;
	int ch, idx = -1;
	struct ap_info ap_rec[128];
	unsigned int ap_rec_num = 0;
	unsigned int minScore=0xffffffff;
	unsigned int groupScore=0xffffffff;

	memset(ap_rec, 0, sizeof(ap_rec));

	if ((nhm_cnt_exp_sum == NULL) || (ap_count == NULL))
	{
		printk("[ACS2] error : nhm_cnt_exp_sum or ap_count NULL\n");
		return idx;
	}

	if (priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_2G)
	{
		unsigned int score[14];
		unsigned int score_no_mask[14];

		if (end > 14)
		{
			printk("[ACS2] wrong para => begin: %d, end: %d\n", begin, end);
			return idx;
		}

		memset(score, 0xff , sizeof(score));
		memset(score_no_mask, 0xff , sizeof(score_no_mask));

	#if 1
		for (y=begin; y<end; y++) {
			score[y] = nhm_cnt_exp_sum[y];
			score_no_mask[y] = nhm_cnt_exp_sum[y];
			ACSDEBUG("ch %d: nhm_cnt_exp_sum: %d\n", y+1, score[y]);
		}
	#else
		for (y=begin; y<end; y++) {
			score[y] = priv->chnl_ss_fa_count[y];
			score_no_mask[y] = priv->chnl_ss_fa_count[y];
			ACSDEBUG("ch %d: FA: %d\n", y+1, score[y]);
		}
	#endif

		for (i=0; i<priv->site_survey->count; i++) {
			pBss = &priv->site_survey->bss[i];

			ap_rec[ap_rec_num].ch = pBss->channel;
			if ((pBss->t_stamp[1] & 0x6) == 0) ap_rec[ap_rec_num].bw = 0;
			else if ((pBss->t_stamp[1] & 0x4) == 0) ap_rec[ap_rec_num].bw = 1;
			else ap_rec[ap_rec_num].bw = 2;
			ap_rec[ap_rec_num++].rssi = pBss->rssi;

			if (pBss->rssi > 15) {
				for (y=begin; y<end; y++) {
					if (pBss->channel == priv->available_chnl[y]) {
						ap_count[y]++;
						if ((pBss->t_stamp[1] & 0x6) == 0) {  // 20M
							for (j=-2; j<=2; j++)
								MASK_CH(y+j, begin, end);
						}
						else if ((pBss->t_stamp[1] & 0x4) == 0) {  // 40M upper
							for (j=-2; j<=6; j++)
								MASK_CH(y+j, begin, end);
						}
						else {  // 40M lower
							for (j=-6; j<=2; j++)
								MASK_CH(y+j, begin, end);
						}
					}
				}
			}
		}

	#if 0
		if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)
			&& priv->pshare->is_40m_bw) {
			for (y=begin; y<end; y++)
				if (priv->available_chnl[y] == 14)
					score[y] = 0xffffffff;		// mask chan14
		}

		if (priv->pmib->dot11RFEntry.disable_ch1213) {
			for (y=begin; y<end; y++) {
				ch = priv->available_chnl[y];
				if ((ch == 12) || (ch == 13))
					score[y] = 0xffffffff;
			}
		}

		if (((priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_GLOBAL) ||
				(priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_WORLD_WIDE)) &&
			 (end >= 11) && (end < 14)) {
			score[13] = 0xffffffff;	// mask chan14
			score[12] = 0xffffffff; // mask chan13
			score[11] = 0xffffffff; // mask chan12
		}
	#endif

		for (y=begin; y<end; y++) {
			if (score[y] == 0xffffffff)
				ACSDEBUG("ch %d: ap_count: %d, score: 0xffffffff ", y+1, ap_count[y]);
			else
				ACSDEBUG("ch %d: ap_count: %d, score: %d ", y+1, ap_count[y], score[y]);

			for (i=0; i<ap_rec_num; i++) {
				if (priv->available_chnl[y] == ap_rec[i].ch) {
					ACSDEBUG("%s:%d ", ap_rec[i].bw==0?"N":(ap_rec[i].bw==1?"U":"L"), ap_rec[i].rssi);
				}
			}
			ACSDEBUG("\n");
		}

		for (y=begin; y<end; y++)
		{
			found = 0;

			if (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_20_40)
			{
				ch = priv->available_chnl[y];
				if ((ch == 3) || (ch == 4) || (ch == 8) || (ch == 9)) // checking by center channel
				{
					for (i=-4; i<=4; i++)
					{
						found = 1;
						if ((y+i >= begin) && (y+i < end))
						{
							if (score[y+i] == 0xffffffff)
							{
								found = 0;
								break;
							}
						}
					}

					if (found)
					{
						groupScore = score_no_mask[y-2] + score_no_mask[y+2];
						ACSDEBUG("[Use40M:%d] [CEN-IDX:%d], [SCORE:%d] (checking start)\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, y, groupScore);
						if (groupScore < minScore)
						{
							minScore = groupScore;
							idx = y; // center channel
						}
						ACSDEBUG("[Use40M:%d] [CEN-IDX:%d] (checking end)\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, idx);
					}
				}
			}
			else if (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_20)
			{
				ch = priv->available_chnl[y];
				if ((ch == 1) || (ch== 6) || (ch == 11))
				{
					for (i=-2; i<=2; i++)
					{
						found = 1;
						if ((y+i >= begin) && (y+i < end))
						{
							if (score[y+i] == 0xffffffff)
							{
								found = 0;
								break;
							}
						}
					}

					if (found)
					{
						ACSDEBUG("[Use40M:%d] [CEN-IDX:%d], [SCORE:%d] (checking start)\n",  priv->pmib->dot11nConfigEntry.dot11nUse40M, y, score_no_mask[y]);
						if (score_no_mask[y] < minScore)
						{
							minScore = score_no_mask[y];
							idx = y;
						}
						ACSDEBUG("[Use40M:%d] [CEN-IDX:%d] (checking end)\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, idx);
					}
				}
			}
		}

		if (idx != -1)
		{
			if (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_20_40)
			{
				ch = priv->available_chnl[idx];
				if (ch == 3) // 1-5
				{
					GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_ABOVE;
					priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_ABOVE;
					idx = idx - 2; // control channel : 1
				}
				else if (ch == 4) // 6-2
				{
					GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_BELOW;
					priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_BELOW;
					idx = idx + 2; // control channel : 6
				}
				else if (ch == 8) // 6-10
				{
					GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_ABOVE;
					priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_ABOVE;
					idx = idx - 2; // control channel : 6
				}
				else if (ch ==9) // 11-7
				{
					GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_BELOW;
					priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_BELOW;
					idx = idx + 2; // control channel : 11
				}

				unsigned str[10];
				ACSDEBUG("[Use40M:%d] [CRL-IDX:%d] (Result)\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, idx);
				if (idx == 0)
					memcpy(str,"(1+5)",strlen(str));
				else if ((idx == 5) && (priv->pshare->offset_2nd_chan== HT_2NDCH_OFFSET_BELOW))
					memcpy(str,"(6+2)",strlen(str));
				else if ((idx == 5) && (priv->pshare->offset_2nd_chan== HT_2NDCH_OFFSET_ABOVE))
					memcpy(str,"(6+10)",strlen(str));
				else if (idx == 10)
					memcpy(str,"(11+7)",strlen(str));

				ACSDEBUG(" ==> idx: %d, offset_2nd_chan: %d <1:Below, 2:Above> %s", idx, priv->pshare->offset_2nd_chan, str);
			}
			else
			{
				ACSDEBUG("[Use40M:%d] [CRL-IDX:%d] (Result)\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, idx);
				ACSDEBUG(" ==> idx: %d\n\n", idx);
			}

			return idx;
		}

		/* special case */
		if (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_20_40)
		{
			int ap_15_chk=1, ap_62_chk=1, ap_610_chk=1, ap_117_chk=1;
			for (i=0; i<ap_rec_num; i++)
			{
				ch = ap_rec[i].ch;
				if (ap_15_chk &&
					!(((ap_rec[i].bw==0) && ((ch == 1) || (ch == 5) || (ch >= 9))) ||  // bw : 0(N) 1(U) 2(L)
					((ap_rec[i].bw==1) && (ch == 1)) ||
					((ap_rec[i].bw==2) && (ch == 5)))
					)
				{
					ap_15_chk = 0;
				}

				if (ap_62_chk &&
					!(((ap_rec[i].bw==0) && ((ch == 2) || (ch == 6) || (ch >= 10))) ||
					((ap_rec[i].bw==2) && (ch == 6)) ||
					((ap_rec[i].bw==1) && (ch == 2)) ||
					((ap_rec[i].bw==1) && (ch == 6)))
					)
				{
					ap_62_chk = 0;
				}

				if (ap_610_chk &&
					!(((ap_rec[i].bw==0) && ((ch <= 2) || (ch == 6) || (ch == 10))) ||
					((ap_rec[i].bw==1) && (ch == 6)) ||
					((ap_rec[i].bw==2) && (ch == 10)) ||
					((ap_rec[i].bw==2) && (ch == 6)))
					)
				{
					ap_610_chk = 0;
				}

				if (ap_117_chk &&
					!(((ap_rec[i].bw==0) && ((ch <= 3) || (ch == 11))) ||
					((ap_rec[i].bw==2) && (ch == 11)) ||
					((ap_rec[i].bw==1) && (ch == 7)))
					)
				{
					ap_117_chk = 0;
				}
			}

			ACSDEBUG("[Use40M:%d] [ap_15_chk:%d] [ap_62_chk:%d] [ap_610_chk:%d] [ap_117_chk:%d]\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, ap_15_chk, ap_62_chk, ap_610_chk, ap_117_chk);

			if (ap_15_chk)
			{
				groupScore =  score_no_mask[0] + score_no_mask[4];
				ACSDEBUG("[Use40M:%d] [ap_15_chk:%d] [SCORE:%d]\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, ap_15_chk, groupScore);
				if (groupScore < minScore)
				{
					minScore = groupScore;
					idx = 0; // control channel : 1
					GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_ABOVE;
					priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_ABOVE;
				}
				ACSDEBUG("[Use40M:%d] [ap_15_chk:%d] [CRL-IDX:%d]\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, ap_15_chk, idx);
			}
			if (ap_62_chk)
			{
				groupScore =  score_no_mask[5] + score_no_mask[1];
				ACSDEBUG("[Use40M:%d] [ap_62_chk:%d] [SCORE:%d]\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, ap_62_chk, groupScore);
				if (groupScore < minScore)
				{
					minScore = groupScore;
					idx = 5; // control channel : 6
					GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_BELOW;
					priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_BELOW;
				}
				ACSDEBUG("[Use40M:%d] [ap_62_chk:%d] [CRL-IDX:%d]\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, ap_62_chk, idx);
			}
			if (ap_610_chk)
			{
				groupScore =  score_no_mask[5] + score_no_mask[9];
				ACSDEBUG("[Use40M:%d] [ap_610_chk:%d] [SCORE:%d]\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, ap_610_chk, groupScore);
				if (groupScore < minScore)
				{
					minScore = groupScore;
					idx = 5; // control channel : 6
					GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_ABOVE;
					priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_ABOVE;
				}
				ACSDEBUG("[Use40M:%d] [ap_610_chk:%d] [CRL-IDX:%d]\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, ap_610_chk, idx);
			}
			if (ap_117_chk)
			{
				groupScore =  score_no_mask[10] + score_no_mask[6];
				ACSDEBUG("[Use40M:%d] [ap_117_chk:%d] [SCORE:%d]\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, ap_117_chk, groupScore);
				if (groupScore < minScore)
				{
					minScore = groupScore;
					idx = 10; // control channel : 11
					GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_BELOW;
					priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_BELOW;
				}
				ACSDEBUG("[Use40M:%d] [ap_117_chk:%d] [CRL-IDX:%d]\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, ap_117_chk, idx);
			}

			ACSDEBUG("[Use40M:%d] [CRL-IDX:%d] (Result)\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, idx);
			ACSDEBUG("[ACS2] ==> idx: %d, offset_2nd_chan: %d <1:Below, 2:Above> ", idx, priv->pshare->offset_2nd_chan);
			if (idx == 0)
				ACSDEBUG("(1+5)");
			else if ((idx == 5) && (priv->pshare->offset_2nd_chan== HT_2NDCH_OFFSET_BELOW))
				ACSDEBUG("(6+2)");
			else if ((idx == 5) && (priv->pshare->offset_2nd_chan== HT_2NDCH_OFFSET_ABOVE))
				ACSDEBUG("(6+10)");
			else if (idx == 10)
				ACSDEBUG("(11+7)");
			ACSDEBUG("\n\n");
		}
		else if (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_20)
		{
			int ap_1_chk=1, ap_6_chk=1, ap_11_chk=1;
			for (i=0; i<ap_rec_num; i++)
			{
				ch = ap_rec[i].ch;
				if (ap_1_chk &&
					!(((ap_rec[i].bw==0) && ((ch == 1) || (ch >= 5))) || // bw : 0(N) 1(U) 2(L)
					((ap_rec[i].bw==1) && (ch == 1)) ||
					((ap_rec[i].bw==2) && (ch == 5)) ||
					((ap_rec[i].bw==1) && (ch >= 5)) ||
					((ap_rec[i].bw==2) && (ch >= 9)))
					)
				{
					ap_1_chk = 0;
				}

				if (ap_6_chk &&
					!(((ap_rec[i].bw==0) && ((ch == 6) || (ch <= 2) || (ch >= 10))) ||
					((ap_rec[i].bw==1) && (ch == 6)) ||
					((ap_rec[i].bw==2) && (ch == 10)) ||
					((ap_rec[i].bw==1) && (ch == 2)) ||
					((ap_rec[i].bw==2) && (ch == 6)))
					)
				{
					ap_6_chk = 0;
				}

				if (ap_11_chk &&
					!(((ap_rec[i].bw==0) && ((ch == 11) || (ch <= 7))) ||
					((ap_rec[i].bw==1) && (ch == 7)) ||
					((ap_rec[i].bw==2) && (ch == 11)) ||
					((ap_rec[i].bw==1) && (ch <= 3)) ||
					((ap_rec[i].bw==2) && (ch <= 7)))
					)
				{
					ap_11_chk = 0;
				}
			}

			ACSDEBUG("[Use40M:%d] [ap_1_chk:%d] [ap_6_chk:%d] [ap_11_chk:%d]\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, ap_1_chk, ap_6_chk, ap_11_chk);

			if (ap_1_chk)
			{
				ACSDEBUG("[Use40M:%d] [ap_1_chk:%d] [SCORE:%d]\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, ap_1_chk, score_no_mask[0]);
				if (score_no_mask[0] < minScore)
				{
					minScore = score_no_mask[0];
					idx = 0;
				}
				ACSDEBUG("[Use40M:%d] [ap_1_chk:%d] [CRL-IDX:%d]\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, ap_1_chk, idx);
			}
			if (ap_6_chk)
			{
				ACSDEBUG("[Use40M:%d] [ap_6_chk:%d] [SCORE:%d]\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, ap_6_chk, score_no_mask[5]);
				if (score_no_mask[5] < minScore)
				{
					minScore = score_no_mask[5];
					idx = 5;
				}
				ACSDEBUG("[Use40M:%d] [ap_6_chk:%d] [CRL-IDX:%d]\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, ap_6_chk, idx);
			}
			if (ap_11_chk)
			{
				ACSDEBUG("[Use40M:%d] [ap_11_chk:%d] [SCORE:%d]\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, ap_11_chk, score_no_mask[10]);
				if (score_no_mask[10] < minScore)
				{
					minScore = score_no_mask[10];
					idx = 10;
				}
				ACSDEBUG("[Use40M:%d] [ap_11_chk:%d] [CRL-IDX:%d]\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, ap_11_chk, idx);
			}

			ACSDEBUG("[Use40M:%d] [CRL-IDX:%d] (Result)\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, idx);
			ACSDEBUG("==> idx: %d\n\n", idx);
		}

		return idx;
	}
	else if (priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_5G)
	{
		unsigned int ap_count_tmp[MAX_NUM_20M_CH];
		unsigned int score[MAX_NUM_20M_CH];
		unsigned int score_no_mask[MAX_NUM_20M_CH];

		if (end > MAX_NUM_20M_CH)
		{
			printk("[ACS2] wrong para => begin: %d, end: %d\n", begin, end);
			return idx;
		}

		memset(ap_count_tmp, 0, sizeof(ap_count_tmp));
		memset(score, 0xff , sizeof(score));
		memset(score_no_mask, 0xff , sizeof(score_no_mask));

		for (i = begin; i < end; i++)
		{
			ch = priv->available_chnl[i];
			for (y = 0; y < MAX_NUM_20M_CH; y++)
			{
				if (ch == CH_20m[y])
				{
					score[y] = nhm_cnt_exp_sum[i];
					score_no_mask[y] = nhm_cnt_exp_sum[i];
					printk("ch %3d: nhm_cnt_exp_sum: %d\n", priv->available_chnl[i], score[y]);
				}
			}
		}

#ifdef HUAWEI_CUSTOM_CN_AUTO_AVAIL_CH
		if (priv->pmib->miscEntry.autoch_149to165_enable)
		{
			for (i = begin; i < end; i++)
			{
				ch = priv->available_chnl[i];
				for (y = 0; y < MAX_NUM_20M_CH; y++)
				{
					if (ch == CH_20m[y])
					{
						if (!((ch >= 149) && (ch <= 165)))
							score[y] = 0xffffffff;	/* Only channel 149~165 for old STA compatibility in CN */
					}
				}
			}
		}
#endif
		for (i = 0; i < priv->site_survey->count; i++)
		{
			pBss = &priv->site_survey->bss[i];

			ap_rec[ap_rec_num].ch = pBss->channel;
			if ((pBss->t_stamp[1] & 0x6) == 0) // 20M
				ap_rec[ap_rec_num].bw = 0;
			else if ((pBss->t_stamp[1] & (BSS_BW_MASK << BSS_BW_SHIFT)) == (HT_CHANNEL_WIDTH_80 << BSS_BW_SHIFT)) // 80M
				ap_rec[ap_rec_num].bw = 3;
			else if ((pBss->t_stamp[1] & 0x4) == 0) // 40M upper
				ap_rec[ap_rec_num].bw = 1;
			else //40M lower
				ap_rec[ap_rec_num].bw = 2;
			ap_rec[ap_rec_num++].rssi = pBss->rssi;

			if (pBss->rssi > 15)
			{
				for (y = 0; y < MAX_NUM_20M_CH; y++)
				{
						if (pBss->channel == CH_20m[y])
						{
							ap_count_tmp[y]++;
							score[y] = 0xffffffff;
							if ((pBss->t_stamp[1] & 0x6) == 0)	// 20M
							{
								// do nothing
							}
							else if ((pBss->t_stamp[1] & (BSS_BW_MASK << BSS_BW_SHIFT)) == (HT_CHANNEL_WIDTH_80 << BSS_BW_SHIFT))  // 80M
							{
								int k = (y / 4) * 4;
								for (j = k; j < (k + 4); j++)
								{
									if ((j != y) && (j < MAX_NUM_20M_CH))
										score[j] = 0xffffffff;
								}
							}
							else if ((pBss->t_stamp[1] & 0x4) == 0)  // 40M upper
							{
								if ((y + 1) < MAX_NUM_20M_CH)
									score[y + 1] = 0xffffffff;
							}
							else  // 40M lower
							{
								if ((y - 1) >= 0)
									score[y - 1] = 0xffffffff;
							}
						}
					}
			}
		}

		for (i = begin; i < end; i++)
		{
			for (y = 0; y < MAX_NUM_20M_CH; y++)
			{
				if (priv->available_chnl[i] == CH_20m[y])
				{
					ap_count[i] = ap_count_tmp[y];

					if (score[y] == 0xffffffff)
						ACSDEBUG("ch %3d: ap_count: %d, score: 0xffffffff ", priv->available_chnl[i], ap_count_tmp[y]);
					else
						ACSDEBUG("ch %3d: ap_count: %d, score: %d ", priv->available_chnl[i], ap_count_tmp[y], score[y]);
				}
			}

			for (y = 0; y < ap_rec_num; y++)
			{
				if (priv->available_chnl[i] == ap_rec[y].ch)
					ACSDEBUG("%s:%d ", ap_rec[y].bw==3?"80N":ap_rec[y].bw==0?"20N":(ap_rec[y].bw==1?"40U":"40L"), ap_rec[y].rssi);
			}
			ACSDEBUG("\n");
		}

		for (i = begin; i < end; i++)
		{
			found = 0;
			groupScore = 0xffffffff;

			if (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_80)
			{
				for (y = 0; y < MAX_NUM_20M_CH; y++)
				{
					if (priv->available_chnl[i] == CH_20m[y])
					{
						if(is80MChannel(priv->available_chnl, priv->available_chnl_num, priv->available_chnl[i]))
						{
							int k = (y / 4) * 4;
							for (j = k; j < (k + 4); j++)
							{
								groupScore += score_no_mask[j];
								if ((j >= MAX_NUM_20M_CH) || (score[j] == 0xffffffff))
									break;
							}
							if (j == (k + 4))
								found = 1;
						}
					}
				}
			}
			else if (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_20_40)
			{
				for (y = 0; y < MAX_NUM_20M_CH; y++)
				{
					if (priv->available_chnl[i] == CH_20m[y])
					{
						if(is40MChannel(priv->available_chnl, priv->available_chnl_num, priv->available_chnl[i]))
						{
							int k = (y / 2) * 2;
							for (j = k; j < (k + 2); j++)
							{
								groupScore += score_no_mask[j];
								if ((j >= MAX_NUM_20M_CH) || (score[j] == 0xffffffff))
									break;
							}
							if (j == (k + 2))
								found = 1;
						}
					}
				}
			}
			else
			{
				for (y = 0; y < MAX_NUM_20M_CH; y++)
				{
					if (priv->available_chnl[i] == CH_20m[y])
					{
						groupScore = score_no_mask[y];
						if (score[y] != 0xffffffff)
							found = 1;
					}
				}
			}

			if (found)
			{
				if (groupScore < minScore)
				{
					minScore = groupScore;
					idx = i;
					ACSDEBUG("[Use40M:%d] [CH:%d], [SCORE:%d]\n", priv->pmib->dot11nConfigEntry.dot11nUse40M, priv->available_chnl[i], groupScore);
				}
			}
		}

		ACSDEBUG("[ACS2] ==> idx: %d ", idx);
		if (idx != -1)
			ACSDEBUG("(CH : %d, BW : %dM)\n\n", priv->available_chnl[idx], priv->pmib->dot11nConfigEntry.dot11nUse40M==0?20:(priv->pmib->dot11nConfigEntry.dot11nUse40M*40));
		else
			ACSDEBUG("\n\n");

		return idx;
	}

	return idx;
}

int selectClearChannel_2g_acs2(struct rtl8192cd_priv *priv)
{
	int i, j, idx=0;
	unsigned int y, ch_begin=0, ch_end= priv->available_chnl_num;

	unsigned int ap_count[64];
	unsigned int ap_count_interf[64];
	unsigned int dig_upper_bond;
	int max_DIG_cover_bond;

	unsigned int clit_est[64]; // channel loading & interference time (CLIT) estimation
	unsigned int clm_ratio[64];
	unsigned int clm_ratio_ori[64];

	unsigned int fa_total_cnt;
	unsigned int fa_noise_factor[64];

	unsigned int nhm_noise_ratio[64]; // can't be reduced by IGI upper bond
	unsigned int nhm_cnt_exp_sum[64];

	memset(ap_count, 0, sizeof(ap_count));
	memset(ap_count_interf, 0, sizeof(ap_count_interf));
	memset(nhm_cnt_exp_sum, 0, sizeof(nhm_cnt_exp_sum));

	find_interf_ap_count_acs2(priv, 0, priv->available_chnl_num, ap_count_interf);

	dig_upper_bond = priv->pshare->rf_ft_var.dbg_dig_upper;
	if (priv->pshare->rf_ft_var.dbg_dig_upper < priv->pshare->rf_ft_var.dbg_dig_lower)
		dig_upper_bond = priv->pshare->rf_ft_var.dbg_dig_lower;

	if (priv->pmib->dot11RFEntry.acs2_cca_cap_db > ACS_CCA_CAP_MAX)
			priv->pmib->dot11RFEntry.acs2_cca_cap_db = ACS_CCA_CAP_MAX;

	max_DIG_cover_bond = (dig_upper_bond - priv->pmib->dot11RFEntry.acs2_cca_cap_db);

	for (y=ch_begin; y<ch_end; y++)
	{
		// CLM
		if ((CLM_SAMPLE_NUM * priv->pmib->dot11RFEntry.acs2_round) != 0 )
			clm_ratio_ori[y] = (priv->clm_cnt[y] * 100) / (CLM_SAMPLE_NUM * priv->pmib->dot11RFEntry.acs2_round);
		else
			clm_ratio_ori[y] = 100;

		clm_ratio[y] = clm_ratio_ori[y];
		if (priv->pmib->dot11RFEntry.acs2_clm_weighting_mode == 1)
		{
			if (ap_count_interf[y] != 0)
				clm_ratio[y] = clm_ratio[y] / 2;
		}
		else if (priv->pmib->dot11RFEntry.acs2_clm_weighting_mode == 2)
		{
			if (ap_count_interf[y] != 0)
				clm_ratio[y] = (clm_ratio[y] * ap_count_interf[y])/ (ap_count_interf[y] + 1);
		}

		// FA
		fa_total_cnt = priv->chnl_ss_fa_count[y];
		if (fa_total_cnt <= priv->chnl_ss_cca_count[y])
		{
			if (priv->chnl_ss_cca_count[y] != 0)
				fa_noise_factor[y] = fa_total_cnt * 100 / priv->chnl_ss_cca_count[y];
			else
				fa_noise_factor[y] = 0;
		}
		else
			fa_noise_factor[y] = 100;

		// NHM
		if (max_DIG_cover_bond >= 0x26)
		{
			for (j=0; j<8; j++) // round 1
				nhm_cnt_exp_sum[y] += priv->nhm_cnt_round[0][y][j] << j;

			nhm_noise_ratio[y] = (priv->nhm_cnt_round[0][y][8] * 100) / (255 * priv->pmib->dot11RFEntry.acs2_round);
		}
		else if (max_DIG_cover_bond > 0x20 && max_DIG_cover_bond < 0x26)
		{
			for (j=0; j<7; j++) // round 1
				nhm_cnt_exp_sum[y] += priv->nhm_cnt_round[0][y][j] << j;

			nhm_noise_ratio[y] = (priv->nhm_cnt_round[0][y][7] * 100) / (255 * priv->pmib->dot11RFEntry.acs2_round);
		}
		else
		{
			for (j=0; j<6; j++) // round 1
				nhm_cnt_exp_sum[y] += priv->nhm_cnt_round[0][y][j] << j;

			nhm_noise_ratio[y] = (priv->nhm_cnt_round[0][y][6] * 100) / (255 * priv->pmib->dot11RFEntry.acs2_round);
		}

		// CLIT
		clit_est[y] = clm_ratio[y] + nhm_noise_ratio[y];

		if (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_20_40)
		{
			//if ((y == 4) || (y == 5) || (y == 9) || (y == 10))
			{
				ACSDEBUG("[CH_%d] clm_cnt: [%d], clm_ratio_ori: [%d](ap:%d, m:%d), clm_ratio: [%d], fa_noise_factor: [%d]\n",
					y+1, priv->clm_cnt[y], clm_ratio_ori[y], ap_count_interf[y], priv->pmib->dot11RFEntry.acs2_clm_weighting_mode, clm_ratio[y], fa_noise_factor[y]);
				ACSDEBUG("[CH_%d] cca: [%d], fa: [%d], cck_fa: [%d], ofdm_fa: [%d]\n",
					y+1, priv->chnl_ss_cca_count[y], priv->chnl_ss_fa_count[y], priv->chnl_ss_cck_fa_count[y], priv->chnl_ss_ofdm_fa_count[y]);
				for (j = 0; j < 1; j++)
				{
					if (max_DIG_cover_bond >= 0x26)
					{
						ACSDEBUG("[CH_%d] nhm_cnt_round%d: H<-[ %3d %3d <%3d> %3d %3d %3d %3d %3d %3d %3d %3d]->L, dbg_dig_upper : 0x%x\n",
							y+1, j+1, priv->nhm_cnt_round[j][y][10], priv->nhm_cnt_round[j][y][9], priv->nhm_cnt_round[j][y][8], priv->nhm_cnt_round[j][y][7],
							priv->nhm_cnt_round[j][y][6], priv->nhm_cnt_round[j][y][5], priv->nhm_cnt_round[j][y][4],
							priv->nhm_cnt_round[j][y][3], priv->nhm_cnt_round[j][y][2], priv->nhm_cnt_round[j][y][1],
							priv->nhm_cnt_round[j][y][0], priv->pshare->rf_ft_var.dbg_dig_upper);
					}
					else if (max_DIG_cover_bond > 0x20 && max_DIG_cover_bond < 0x26)
					{
						ACSDEBUG("[CH_%d] nhm_cnt_round%d: H<-[ %3d %3d %3d <%3d> %3d %3d %3d %3d %3d %3d %3d]->L, dbg_dig_upper : 0x%x\n",
							y+1, j+1, priv->nhm_cnt_round[j][y][10], priv->nhm_cnt_round[j][y][9], priv->nhm_cnt_round[j][y][8], priv->nhm_cnt_round[j][y][7],
							priv->nhm_cnt_round[j][y][6], priv->nhm_cnt_round[j][y][5], priv->nhm_cnt_round[j][y][4],
							priv->nhm_cnt_round[j][y][3], priv->nhm_cnt_round[j][y][2], priv->nhm_cnt_round[j][y][1],
							priv->nhm_cnt_round[j][y][0], priv->pshare->rf_ft_var.dbg_dig_upper);
					}
					else
					{
						ACSDEBUG("[CH_%d] nhm_cnt_round%d: H<-[ %3d %3d %3d %3d <%3d> %3d %3d %3d %3d %3d %3d]->L, dbg_dig_upper : 0x%x\n",
							y+1, j+1, priv->nhm_cnt_round[j][y][10], priv->nhm_cnt_round[j][y][9], priv->nhm_cnt_round[j][y][8], priv->nhm_cnt_round[j][y][7],
							priv->nhm_cnt_round[j][y][6], priv->nhm_cnt_round[j][y][5], priv->nhm_cnt_round[j][y][4],
							priv->nhm_cnt_round[j][y][3], priv->nhm_cnt_round[j][y][2], priv->nhm_cnt_round[j][y][1],
							priv->nhm_cnt_round[j][y][0], priv->pshare->rf_ft_var.dbg_dig_upper);
					}
				}
				ACSDEBUG("[CH_%d] nhm_cnt_exp_sum: [%d], nhm_noise_ratio: [%d], clit_est: [%d]\n\n",
					y+1, nhm_cnt_exp_sum[y], nhm_noise_ratio[y], clit_est[y]);
			}
		}
		else if (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_20)
		{
			//if ((y == 0) || (y == 5) || (y == 10))
			{
				ACSDEBUG("[CH_%d] clm_cnt: [%d], clm_ratio_ori: [%d](ap:%d, m:%d), clm_ratio: [%d], fa_noise_factor: [%d]\n",
					y+1, priv->clm_cnt[y], clm_ratio_ori[y], ap_count_interf[y], priv->pmib->dot11RFEntry.acs2_clm_weighting_mode, clm_ratio[y], fa_noise_factor[y]);
				ACSDEBUG("[CH_%d] cca: [%d], fa: [%d], cck_fa: [%d], ofdm_fa: [%d]\n",
					y+1, priv->chnl_ss_cca_count[y], priv->chnl_ss_fa_count[y], priv->chnl_ss_cck_fa_count[y], priv->chnl_ss_ofdm_fa_count[y]);
				for (j = 0; j < 1; j++)
				{
					if (max_DIG_cover_bond >= 0x26)
					{
						ACSDEBUG("[CH_%d] nhm_cnt_round%d: H<-[ %3d %3d <%3d> %3d %3d %3d %3d %3d %3d %3d %3d]->L, dbg_dig_upper : 0x%x\n",
							y+1, j+1, priv->nhm_cnt_round[j][y][10], priv->nhm_cnt_round[j][y][9], priv->nhm_cnt_round[j][y][8], priv->nhm_cnt_round[j][y][7],
							priv->nhm_cnt_round[j][y][6], priv->nhm_cnt_round[j][y][5], priv->nhm_cnt_round[j][y][4],
							priv->nhm_cnt_round[j][y][3], priv->nhm_cnt_round[j][y][2], priv->nhm_cnt_round[j][y][1],
							priv->nhm_cnt_round[j][y][0], priv->pshare->rf_ft_var.dbg_dig_upper);
					}
					else if (max_DIG_cover_bond > 0x20 && max_DIG_cover_bond < 0x26)
					{
						ACSDEBUG("[CH_%d] nhm_cnt_round%d: H<-[ %3d %3d %3d <%3d> %3d %3d %3d %3d %3d %3d %3d]->L, dbg_dig_upper : 0x%x\n",
							y+1, j+1, priv->nhm_cnt_round[j][y][10], priv->nhm_cnt_round[j][y][9], priv->nhm_cnt_round[j][y][8], priv->nhm_cnt_round[j][y][7],
							priv->nhm_cnt_round[j][y][6], priv->nhm_cnt_round[j][y][5], priv->nhm_cnt_round[j][y][4],
							priv->nhm_cnt_round[j][y][3], priv->nhm_cnt_round[j][y][2], priv->nhm_cnt_round[j][y][1],
							priv->nhm_cnt_round[j][y][0], priv->pshare->rf_ft_var.dbg_dig_upper);
					}
					else
					{
						ACSDEBUG("[CH_%d] nhm_cnt_round%d: H<-[ %3d %3d %3d %3d <%3d> %3d %3d %3d %3d %3d %3d]->L, dbg_dig_upper : 0x%x\n",
							y+1, j+1, priv->nhm_cnt_round[j][y][10], priv->nhm_cnt_round[j][y][9], priv->nhm_cnt_round[j][y][8], priv->nhm_cnt_round[j][y][7],
							priv->nhm_cnt_round[j][y][6], priv->nhm_cnt_round[j][y][5], priv->nhm_cnt_round[j][y][4],
							priv->nhm_cnt_round[j][y][3], priv->nhm_cnt_round[j][y][2], priv->nhm_cnt_round[j][y][1],
							priv->nhm_cnt_round[j][y][0], priv->pshare->rf_ft_var.dbg_dig_upper);
					}
				}
				ACSDEBUG("[CH_%d] nhm_cnt_exp_sum: [%d], nhm_noise_ratio: [%d], clit_est: [%d]\n\n",
					y+1, nhm_cnt_exp_sum[y], nhm_noise_ratio[y], clit_est[y]);
			}
		}
	}

	if (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_20_40)
	{
		int ret = -1;

		ret = find_clean_channel_acs2(priv, 0, priv->available_chnl_num, nhm_cnt_exp_sum, ap_count);
		if (priv->pmib->dot11RFEntry.acs2_dis_clean_channel)
		{
			printk("[ACS2] !!!! Ignore clean channel result\n");
			ret = -1;
		}

		if (ret >= 0)
		{
			printk("[ACS2] !!!! Found clean channel, select minimum \"nhm_cnt_exp_sum\" channel\n");
			idx = ret;
		}
		else
		{
			printk("[ACS2] !!!! Not found clean channel, use NHM/CLM algorithm\n");

			printk("\n");
			ACSDEBUG("[ACS2] BW-40M CLM and Two Round NHM Info:\n");

			for (i=0; i<priv->available_chnl_num; i++) // get NHM/CLM info
			{
				int ch = priv->available_chnl[i];
				if ((ch == 5) || (ch == 6) || (ch == 10) || (ch == 11))
				{
					ACSDEBUG("[CH_%d] clm_cnt: [%d], clm_ratio_ori: [%d](ap:%d), clm_ratio: [%d] fa_noise_factor: [%d]\n", i+1, priv->clm_cnt[i], clm_ratio_ori[i], ap_count_interf[i], clm_ratio[i], fa_noise_factor[i]);
					for (j = 0; j < 1; j++)
					{
						if (max_DIG_cover_bond >= 0x26)
						{
							ACSDEBUG("[CH_%d] nhm_cnt_round%d: H<-[ %3d %3d <%3d> %3d %3d %3d %3d %3d %3d %3d %3d]->L, dbg_dig_upper : 0x%x\n",
								i+1, j+1, priv->nhm_cnt_round[j][i][10], priv->nhm_cnt_round[j][i][9], priv->nhm_cnt_round[j][i][8], priv->nhm_cnt_round[j][i][7],
								priv->nhm_cnt_round[j][i][6], priv->nhm_cnt_round[j][i][5], priv->nhm_cnt_round[j][i][4],
								priv->nhm_cnt_round[j][i][3], priv->nhm_cnt_round[j][i][2], priv->nhm_cnt_round[j][i][1],
								priv->nhm_cnt_round[j][i][0], priv->pshare->rf_ft_var.dbg_dig_upper);
						}
						else if (max_DIG_cover_bond > 0x20 && max_DIG_cover_bond < 0x26)
						{
							ACSDEBUG("[CH_%d] nhm_cnt_round%d: H<-[ %3d %3d %3d <%3d> %3d %3d %3d %3d %3d %3d %3d]->L, dbg_dig_upper : 0x%x\n",
								i+1, j+1, priv->nhm_cnt_round[j][i][10], priv->nhm_cnt_round[j][i][9], priv->nhm_cnt_round[j][i][8], priv->nhm_cnt_round[j][i][7],
								priv->nhm_cnt_round[j][i][6], priv->nhm_cnt_round[j][i][5], priv->nhm_cnt_round[j][i][4],
								priv->nhm_cnt_round[j][i][3], priv->nhm_cnt_round[j][i][2], priv->nhm_cnt_round[j][i][1],
								priv->nhm_cnt_round[j][i][0], priv->pshare->rf_ft_var.dbg_dig_upper);
						}
						else
						{
							ACSDEBUG("[CH_%d] nhm_cnt_round%d: H<-[ %3d %3d %3d %3d <%3d> %3d %3d %3d %3d %3d %3d]->L, dbg_dig_upper : 0x%x\n",
								i+1, j+1, priv->nhm_cnt_round[j][i][10], priv->nhm_cnt_round[j][i][9], priv->nhm_cnt_round[j][i][8], priv->nhm_cnt_round[j][i][7],
								priv->nhm_cnt_round[j][i][6], priv->nhm_cnt_round[j][i][5], priv->nhm_cnt_round[j][i][4],
								priv->nhm_cnt_round[j][i][3], priv->nhm_cnt_round[j][i][2], priv->nhm_cnt_round[j][i][1],
								priv->nhm_cnt_round[j][i][0], priv->pshare->rf_ft_var.dbg_dig_upper);
						}
					}
					ACSDEBUG("[CH_%d] nhm_cnt_exp_sum: [%d], nhm_noise_ratio: [%d], clit_est: [%d]\n",
						i+1, nhm_cnt_exp_sum[i], nhm_noise_ratio[i], clit_est[i]);
					ACSDEBUG("\n");
				}
			}

			unsigned int idx_40[4];
			unsigned int clit_est_40[4];
			unsigned int clm_cnt_40[4];
			unsigned int clm_ratio_40[4];
			unsigned int fa_noise_factor_40[4];
			unsigned int nhm_noise_ratio_40[4];
			unsigned int nhm_cnt_exp_sum_40[4];

			for (i=0; i<4; i++) // init value
			{
				idx_40[i] = 0;
				clit_est_40[i] = 200;
				clm_cnt_40[i] = 0xFFFFFFFF;
				clm_ratio_40[i] = 200;
				fa_noise_factor_40[i] = 100;
				nhm_noise_ratio_40[i] = 100;
				nhm_cnt_exp_sum_40[i] = 0xFFFFFFFF;
			}

			for (i=0; i<priv->available_chnl_num; i++) // get NHM/CLM info
			{
				int ch = priv->available_chnl[i];
				if (ch == 5) //1-5
				{
					idx_40[0] = i;
					clit_est_40[0] = clit_est[i];
					clm_cnt_40[0] = priv->clm_cnt[i];
					clm_ratio_40[0] = clm_ratio[i];
					fa_noise_factor_40[0] = fa_noise_factor[i];
					nhm_noise_ratio_40[0] = nhm_noise_ratio[i];
					nhm_cnt_exp_sum_40[0] = nhm_cnt_exp_sum[i];
				}
				if (ch == 6) //6-2
				{
					idx_40[1] = i;
					clit_est_40[1] = clit_est[i];
					clm_cnt_40[1] = priv->clm_cnt[i];
					clm_ratio_40[1] = clm_ratio[i];
					fa_noise_factor_40[1] = fa_noise_factor[i];
					nhm_noise_ratio_40[1] = nhm_noise_ratio[i];
					nhm_cnt_exp_sum_40[1] = nhm_cnt_exp_sum[i];
				}
				if (ch == 10) //6-10
				{
					idx_40[2] = i;
					clit_est_40[2] = clit_est[i];
					clm_cnt_40[2] = priv->clm_cnt[i];
					clm_ratio_40[2] = clm_ratio[i];
					fa_noise_factor_40[2] = fa_noise_factor[i];
					nhm_noise_ratio_40[2] = nhm_noise_ratio[i];
					nhm_cnt_exp_sum_40[2] = nhm_cnt_exp_sum[i];
				}
				if (ch == 11) //11-7
				{
					idx_40[3] = i;
					clit_est_40[3] = clit_est[i];
					clm_cnt_40[3] = priv->clm_cnt[i];
					clm_ratio_40[3] = clm_ratio[i];
					fa_noise_factor_40[3] = fa_noise_factor[i];
					nhm_noise_ratio_40[3] = nhm_noise_ratio[i];
					nhm_cnt_exp_sum_40[3] = nhm_cnt_exp_sum[i];
				}
			}

			ACSDEBUG("\n");
			ACSDEBUG("[ACS2] BW-40M CLM/NHM Checking:\n");
			ACSDEBUG("[ACS2] CLM_SAMPLE_NUM: %d, acs2_round: %d\n", CLM_SAMPLE_NUM, priv->pmib->dot11RFEntry.acs2_round);
			ACSDEBUG("[ACS2] before sorting:\n");
			for (i=0; i<4; i++)
			{
				ACSDEBUG("[ACS2] (%d) idx_40_%d, clit_est_40: %d, clm_cnt_40: %d, clm_ratio_40: %d, fa_noise_factor_40: %d, nhm_noise_ratio_40: %d, nhm_cnt_exp_sum_40: %d\n",
					i, idx_40[i], clit_est_40[i], clm_cnt_40[i], clm_ratio_40[i], fa_noise_factor_40[i], nhm_noise_ratio_40[i], nhm_cnt_exp_sum_40[i]);
			}

			for (i=0; i<4; i++) // high priority with small NHM/CLM value
			{
				unsigned int idx_40_tmp;
				unsigned int clit_est_40_tmp;
				unsigned int clm_cnt_40_tmp;
				unsigned int clm_ratio_40_tmp;
				unsigned int fa_noise_factor_40_tmp;
				unsigned int nhm_noise_ratio_40_tmp;
				unsigned int nhm_cnt_exp_sum_40_tmp;

				for (j=i; j<4; j++)
				{
					if (clit_est_40[j] < clit_est_40[i])
					{
						idx_40_tmp = idx_40[j];
						clit_est_40_tmp = clit_est_40[j];
						clm_cnt_40_tmp = clm_cnt_40[j];
						clm_ratio_40_tmp = clm_ratio_40[j];
						fa_noise_factor_40_tmp = fa_noise_factor_40[j];
						nhm_noise_ratio_40_tmp = nhm_noise_ratio_40[j];
						nhm_cnt_exp_sum_40_tmp = nhm_cnt_exp_sum_40[j];

						idx_40[j] = idx_40[i];
						clit_est_40[j] = clit_est_40[i];
						clm_cnt_40[j] = clm_cnt_40[i];
						clm_ratio_40[j] = clm_ratio_40[i];
						fa_noise_factor_40[j] = fa_noise_factor_40[i];
						nhm_noise_ratio_40[j] = nhm_noise_ratio_40[i];
						nhm_cnt_exp_sum_40[j] = nhm_cnt_exp_sum_40[i];

						idx_40[i] = idx_40_tmp;
						clit_est_40[i] = clit_est_40_tmp;
						clm_cnt_40[i] = clm_cnt_40_tmp;
						clm_ratio_40[i] = clm_ratio_40_tmp;
						fa_noise_factor_40[i] = fa_noise_factor_40_tmp;
						nhm_noise_ratio_40[i] = nhm_noise_ratio_40_tmp;
						nhm_cnt_exp_sum_40[i] = nhm_cnt_exp_sum_40_tmp;
					}
					else if (clit_est_40[j] == clit_est_40[i])
					{
						if (nhm_noise_ratio_40[j] < nhm_noise_ratio_40[i])
						{
							idx_40_tmp = idx_40[j];
							clit_est_40_tmp = clit_est_40[j];
							clm_cnt_40_tmp = clm_cnt_40[j];
							clm_ratio_40_tmp = clm_ratio_40[j];
							fa_noise_factor_40_tmp = fa_noise_factor_40[j];
							nhm_noise_ratio_40_tmp = nhm_noise_ratio_40[j];
							nhm_cnt_exp_sum_40_tmp = nhm_cnt_exp_sum_40[j];

							idx_40[j] = idx_40[i];
							clit_est_40[j] = clit_est_40[i];
							clm_cnt_40[j] = clm_cnt_40[i];
							clm_ratio_40[j] = clm_ratio_40[i];
							fa_noise_factor_40[j] = fa_noise_factor_40[i];
							nhm_noise_ratio_40[j] = nhm_noise_ratio_40[i];
							nhm_cnt_exp_sum_40[j] = nhm_cnt_exp_sum_40[i];

							idx_40[i] = idx_40_tmp;
							clit_est_40[i] = clit_est_40_tmp;
							clm_cnt_40[i] = clm_cnt_40_tmp;
							clm_ratio_40[i] = clm_ratio_40_tmp;
							fa_noise_factor_40[i] = fa_noise_factor_40_tmp;
							nhm_noise_ratio_40[i] = nhm_noise_ratio_40_tmp;
							nhm_cnt_exp_sum_40[i] = nhm_cnt_exp_sum_40_tmp;
						}
						else if (nhm_noise_ratio_40[j] == nhm_noise_ratio_40[i])
						{
							if (fa_noise_factor_40[j] < fa_noise_factor_40[i])
							{
								idx_40_tmp = idx_40[j];
								clit_est_40_tmp = clit_est_40[j];
								clm_cnt_40_tmp = clm_cnt_40[j];
								clm_ratio_40_tmp = clm_ratio_40[j];
								fa_noise_factor_40_tmp = fa_noise_factor_40[j];
								nhm_noise_ratio_40_tmp = nhm_noise_ratio_40[j];
								nhm_cnt_exp_sum_40_tmp = nhm_cnt_exp_sum_40[j];

								idx_40[j] = idx_40[i];
								clit_est_40[j] = clit_est_40[i];
								clm_cnt_40[j] = clm_cnt_40[i];
								clm_ratio_40[j] = clm_ratio_40[i];
								fa_noise_factor_40[j] = fa_noise_factor_40[i];
								nhm_noise_ratio_40[j] = nhm_noise_ratio_40[i];
								nhm_cnt_exp_sum_40[j] = nhm_cnt_exp_sum_40[i];

								idx_40[i] = idx_40_tmp;
								clit_est_40[i] = clit_est_40_tmp;
								clm_cnt_40[i] = clm_cnt_40_tmp;
								clm_ratio_40[i] = clm_ratio_40_tmp;
								fa_noise_factor_40[i] = fa_noise_factor_40_tmp;
								nhm_noise_ratio_40[i] = nhm_noise_ratio_40_tmp;
								nhm_cnt_exp_sum_40[i] = nhm_cnt_exp_sum_40_tmp;
							}
							else if (fa_noise_factor_40[j] == fa_noise_factor_40[i])
							{
								if (nhm_cnt_exp_sum_40[j] < nhm_cnt_exp_sum_40[i])
								{
									idx_40_tmp = idx_40[j];
									clit_est_40_tmp = clit_est_40[j];
									clm_cnt_40_tmp = clm_cnt_40[j];
									clm_ratio_40_tmp = clm_ratio_40[j];
									fa_noise_factor_40_tmp = fa_noise_factor_40[j];
									nhm_noise_ratio_40_tmp = nhm_noise_ratio_40[j];
									nhm_cnt_exp_sum_40_tmp = nhm_cnt_exp_sum_40[j];

									idx_40[j] = idx_40[i];
									clit_est_40[j] = clit_est_40[i];
									clm_cnt_40[j] = clm_cnt_40[i];
									clm_ratio_40[j] = clm_ratio_40[i];
									fa_noise_factor_40[j] = fa_noise_factor_40[i];
									nhm_noise_ratio_40[j] = nhm_noise_ratio_40[i];
									nhm_cnt_exp_sum_40[j] = nhm_cnt_exp_sum_40[i];

									idx_40[i] = idx_40_tmp;
									clit_est_40[i] = clit_est_40_tmp;
									clm_cnt_40[i] = clm_cnt_40_tmp;
									clm_ratio_40[i] = clm_ratio_40_tmp;
									fa_noise_factor_40[i] = fa_noise_factor_40_tmp;
									nhm_noise_ratio_40[i] = nhm_noise_ratio_40_tmp;
									nhm_cnt_exp_sum_40[i] = nhm_cnt_exp_sum_40_tmp;
								}
							}
						}
					}
				}
			}

			ACSDEBUG("\n[ACS2] after sorting:\n");
			for (i=0; i<4; i++)
			{
				ACSDEBUG("[ACS2] (%d) idx_40_%d, clit_est_40: %d, clm_cnt_40: %d, clm_ratio_40: %d, fa_noise_factor_40: %d, nhm_noise_ratio_40: %d, nhm_cnt_exp_sum_40: %d\n",
					i, idx_40[i], clit_est_40[i], clm_cnt_40[i], clm_ratio_40[i], fa_noise_factor_40[i], nhm_noise_ratio_40[i], nhm_cnt_exp_sum_40[i]);
			}

			/* side band */
			idx = idx_40[0];
			int ch = priv->available_chnl[idx];
			if (ch == 5)
			{
				GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_ABOVE;
				priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_ABOVE;
				idx = 0; // ch1
			}
			else if (ch == 6)
			{
				GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_BELOW;
				priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_BELOW;
			}
			else if (ch == 10)
			{
				GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_ABOVE;
				priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_ABOVE;
				idx = 5; // ch6
			}
			else if (ch == 11)
			{
				GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_BELOW;
				priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_BELOW;
			}

			ACSDEBUG("[ACS2] ==> idx: %d, offset_2nd_chan: %d <1:Below, 2:Above> ", idx, priv->pshare->offset_2nd_chan);
			if (idx == 0)
				ACSDEBUG("(1+5)");
			else if ((idx == 5) && (priv->pshare->offset_2nd_chan== 1))
				ACSDEBUG("(6+2)");
			else if ((idx == 5) && (priv->pshare->offset_2nd_chan== 2))
				ACSDEBUG("(6+10)");
			else if (idx == 10)
				ACSDEBUG("(11+7)");
			ACSDEBUG("\n\n");
		}
	}
	else
	{
		int ret = -1;

		ret = find_clean_channel_acs2(priv, 0, priv->available_chnl_num, nhm_cnt_exp_sum, ap_count);
		if (priv->pmib->dot11RFEntry.acs2_dis_clean_channel)
		{
			printk("[ACS2] !!!! Ignore clean channel result\n");
			ret = -1;
		}

		if (ret >= 0)
		{
			printk("[ACS2] !!!! Found clean channel, select minimum \"nhm_cnt_exp_sum\" channel\n");
			idx = ret;
		}
		else
		{
			printk("[ACS2] !!!! Not found clean channel, use NHM/CLM algorithm\n");

			printk("\n");
			ACSDEBUG("[ACS2] BW-20M CLM and Two Round NHM Info:\n");

			for (i=0; i<priv->available_chnl_num; i++) // get NHM/CLM info
			{
				int ch = priv->available_chnl[i];
				if ((ch == 1) || (ch == 6) || (ch == 11))
				{
					ACSDEBUG("[CH_%d] clm_cnt: [%d], clm_ratio_ori: [%d](ap:%d), clm_ratio: [%d], fa_noise_factor: [%d]\n", i+1, priv->clm_cnt[i], clm_ratio_ori[i], ap_count_interf[i], clm_ratio[i], fa_noise_factor[i]);
					for (j = 0; j < 1; j++)
					{
							if (max_DIG_cover_bond >= 0x26)
							{
								ACSDEBUG("[CH_%d] nhm_cnt_round%d: H<-[ %3d %3d <%3d> %3d %3d %3d %3d %3d %3d %3d %3d]->L, dbg_dig_upper : 0x%x\n",
									i+1, j+1, priv->nhm_cnt_round[j][i][10], priv->nhm_cnt_round[j][i][9], priv->nhm_cnt_round[j][i][8], priv->nhm_cnt_round[j][i][7],
									priv->nhm_cnt_round[j][i][6], priv->nhm_cnt_round[j][i][5], priv->nhm_cnt_round[j][i][4],
									priv->nhm_cnt_round[j][i][3], priv->nhm_cnt_round[j][i][2], priv->nhm_cnt_round[j][i][1],
									priv->nhm_cnt_round[j][i][0], priv->pshare->rf_ft_var.dbg_dig_upper);
							}
							else if (max_DIG_cover_bond > 0x20 && max_DIG_cover_bond < 0x26)
							{
								ACSDEBUG("[CH_%d] nhm_cnt_round%d: H<-[ %3d %3d %3d <%3d> %3d %3d %3d %3d %3d %3d %3d]->L, dbg_dig_upper : 0x%x\n",
									i+1, j+1, priv->nhm_cnt_round[j][i][10], priv->nhm_cnt_round[j][i][9], priv->nhm_cnt_round[j][i][8], priv->nhm_cnt_round[j][i][7],
									priv->nhm_cnt_round[j][i][6], priv->nhm_cnt_round[j][i][5], priv->nhm_cnt_round[j][i][4],
									priv->nhm_cnt_round[j][i][3], priv->nhm_cnt_round[j][i][2], priv->nhm_cnt_round[j][i][1],
									priv->nhm_cnt_round[j][i][0], priv->pshare->rf_ft_var.dbg_dig_upper);
							}
							else
							{
								printk("[CH_%d] nhm_cnt_round%d: H<-[ %3d %3d %3d %3d <%3d> %3d %3d %3d %3d %3d %3d]->L, dbg_dig_upper : 0x%x\n",
									i+1, j+1, priv->nhm_cnt_round[j][i][10], priv->nhm_cnt_round[j][i][9], priv->nhm_cnt_round[j][i][8], priv->nhm_cnt_round[j][i][7],
									priv->nhm_cnt_round[j][i][6], priv->nhm_cnt_round[j][i][5], priv->nhm_cnt_round[j][i][4],
									priv->nhm_cnt_round[j][i][3], priv->nhm_cnt_round[j][i][2], priv->nhm_cnt_round[j][i][1],
									priv->nhm_cnt_round[j][i][0], priv->pshare->rf_ft_var.dbg_dig_upper);
							}
					}
					ACSDEBUG("[CH_%d] nhm_cnt_exp_sum: [%d], nhm_noise_ratio: [%d], clit_est: [%d]\n",
						i+1, nhm_cnt_exp_sum[i], nhm_noise_ratio[i], clit_est[i]);
					ACSDEBUG("\n");
				}
			}

			unsigned int idx_20[3];
			unsigned int clit_est_20[3];
			unsigned int clm_cnt_20[3];
			unsigned int clm_ratio_20[3];
			unsigned int fa_noise_factor_20[3];
			unsigned int nhm_noise_ratio_20[3];
			unsigned int nhm_cnt_exp_sum_20[3];

			for (i=0; i<3; i++) // init value
			{
				idx_20[i] = 0;
				clit_est_20[i] = 200;
				clm_cnt_20[i] = 0xFFFFFFFF;
				clm_ratio_20[i] = 200;
				fa_noise_factor_20[i] = 100;
				nhm_noise_ratio_20[i] = 100;
				nhm_cnt_exp_sum_20[i] = 0xFFFFFFFF;
			}

			for (i=0; i<priv->available_chnl_num; i++) // get NHM/CLM info
			{
				int ch = priv->available_chnl[i];
				if (ch == 1)
				{
					idx_20[0] = i;
					clit_est_20[0] = clit_est[i];
					clm_cnt_20[0] = priv->clm_cnt[i];
					clm_ratio_20[0] = clm_ratio[i];
					fa_noise_factor_20[0] = fa_noise_factor[i];
					nhm_noise_ratio_20[0] = nhm_noise_ratio[i];
					nhm_cnt_exp_sum_20[0] = nhm_cnt_exp_sum[i];
				}
				if (ch == 6)
				{
					idx_20[1] = i;
					clit_est_20[1] = clit_est[i];
					clm_cnt_20[1] = priv->clm_cnt[i];
					clm_ratio_20[1] = clm_ratio[i];
					fa_noise_factor_20[1] = fa_noise_factor[i];
					nhm_noise_ratio_20[1] = nhm_noise_ratio[i];
					nhm_cnt_exp_sum_20[1] = nhm_cnt_exp_sum[i];
				}
				if (ch == 11)
				{
					idx_20[2] = i;
					clit_est_20[2] = clit_est[i];
					clm_cnt_20[2] = priv->clm_cnt[i];
					clm_ratio_20[2] = clm_ratio[i];
					fa_noise_factor_20[2] = fa_noise_factor[i];
					nhm_noise_ratio_20[2] = nhm_noise_ratio[i];
					nhm_cnt_exp_sum_20[2] = nhm_cnt_exp_sum[i];
				}
			}

			ACSDEBUG("\n");
			ACSDEBUG("[ACS2] BW-20M CLM/NHM Checking:\n");
			ACSDEBUG("[ACS2] CLM_SAMPLE_NUM: %d, acs2_round: %d\n", CLM_SAMPLE_NUM, priv->pmib->dot11RFEntry.acs2_round);
			ACSDEBUG("[ACS2] before sorting:\n");
			for (i=0; i<3; i++)
			{
				ACSDEBUG("[ACS2] (%d) idx_20_%d, clit_est_20: %d, clm_cnt_20: %d, clm_ratio_20: %d, fa_noise_factor_20: %d, nhm_noise_ratio_20: %d, nhm_cnt_exp_sum_20: %d\n",
					i, idx_20[i], clit_est_20[i], clm_cnt_20[i], clm_ratio_20[i], fa_noise_factor_20[i], nhm_noise_ratio_20[i], nhm_cnt_exp_sum_20[i]);
			}

			for (i=0; i<3; i++) // high priority with small NHM/CLM value
			{
				unsigned int idx_20_tmp;
				unsigned int clit_est_20_tmp;
				unsigned int clm_cnt_20_tmp;
				unsigned int clm_ratio_20_tmp;
				unsigned int fa_noise_factor_20_tmp;
				unsigned int nhm_noise_ratio_20_tmp;
				unsigned int nhm_cnt_exp_sum_20_tmp;

				for (j=i; j<3; j++)
				{
					if (clit_est_20[j] < clit_est_20[i])
					{
						idx_20_tmp = idx_20[j];
						clit_est_20_tmp = clit_est_20[j];
						clm_cnt_20_tmp = clm_cnt_20[j];
						clm_ratio_20_tmp = clm_ratio_20[j];
						fa_noise_factor_20_tmp = fa_noise_factor_20[j];
						nhm_noise_ratio_20_tmp = nhm_noise_ratio_20[j];
						nhm_cnt_exp_sum_20_tmp = nhm_cnt_exp_sum_20[j];

						idx_20[j] = idx_20[i];
						clit_est_20[j] = clit_est_20[i];
						clm_cnt_20[j] = clm_cnt_20[i];
						clm_ratio_20[j] = clm_ratio_20[i];
						fa_noise_factor_20[j] = fa_noise_factor_20[i];
						nhm_noise_ratio_20[j] = nhm_noise_ratio_20[i];
						nhm_cnt_exp_sum_20[j] = nhm_cnt_exp_sum_20[i];

						idx_20[i] = idx_20_tmp;
						clit_est_20[i] = clit_est_20_tmp;
						clm_cnt_20[i] = clm_cnt_20_tmp;
						clm_ratio_20[i] = clm_ratio_20_tmp;
						fa_noise_factor_20[i] = fa_noise_factor_20_tmp;
						nhm_noise_ratio_20[i] = nhm_noise_ratio_20_tmp;
						nhm_cnt_exp_sum_20[i] = nhm_cnt_exp_sum_20_tmp;
					}
					else if (clit_est_20[j] == clit_est_20[i])
					{
						if (nhm_noise_ratio_20[j] < nhm_noise_ratio_20[i])
						{
							idx_20_tmp = idx_20[j];
							clit_est_20_tmp = clit_est_20[j];
							clm_cnt_20_tmp = clm_cnt_20[j];
							clm_ratio_20_tmp = clm_ratio_20[j];
							fa_noise_factor_20_tmp = fa_noise_factor_20[j];
							nhm_noise_ratio_20_tmp = nhm_noise_ratio_20[j];
							nhm_cnt_exp_sum_20_tmp = nhm_cnt_exp_sum_20[j];

							idx_20[j] = idx_20[i];
							clit_est_20[j] = clit_est_20[i];
							clm_cnt_20[j] = clm_cnt_20[i];
							clm_ratio_20[j] = clm_ratio_20[i];
							fa_noise_factor_20[j] = fa_noise_factor_20[i];
							nhm_noise_ratio_20[j] = nhm_noise_ratio_20[i];
							nhm_cnt_exp_sum_20[j] = nhm_cnt_exp_sum_20[i];

							idx_20[i] = idx_20_tmp;
							clit_est_20[i] = clit_est_20_tmp;
							clm_cnt_20[i] = clm_cnt_20_tmp;
							clm_ratio_20[i] = clm_ratio_20_tmp;
							fa_noise_factor_20[i] = fa_noise_factor_20_tmp;
							nhm_noise_ratio_20[i] = nhm_noise_ratio_20_tmp;
							nhm_cnt_exp_sum_20[i] = nhm_cnt_exp_sum_20_tmp;
						}
						else if (nhm_noise_ratio_20[j] == nhm_noise_ratio_20[i])
						{
							if (fa_noise_factor_20[j] < fa_noise_factor_20[i])
							{
								idx_20_tmp = idx_20[j];
								clit_est_20_tmp = clit_est_20[j];
								clm_cnt_20_tmp = clm_cnt_20[j];
								clm_ratio_20_tmp = clm_ratio_20[j];
								fa_noise_factor_20_tmp = fa_noise_factor_20[j];
								nhm_noise_ratio_20_tmp = nhm_noise_ratio_20[j];
								nhm_cnt_exp_sum_20_tmp = nhm_cnt_exp_sum_20[j];

								idx_20[j] = idx_20[i];
								clit_est_20[j] = clit_est_20[i];
								clm_cnt_20[j] = clm_cnt_20[i];
								clm_ratio_20[j] = clm_ratio_20[i];
								fa_noise_factor_20[j] = fa_noise_factor_20[i];
								nhm_noise_ratio_20[j] = nhm_noise_ratio_20[i];
								nhm_cnt_exp_sum_20[j] = nhm_cnt_exp_sum_20[i];

								idx_20[i] = idx_20_tmp;
								clit_est_20[i] = clit_est_20_tmp;
								clm_cnt_20[i] = clm_cnt_20_tmp;
								clm_ratio_20[i] = clm_ratio_20_tmp;
								fa_noise_factor_20[i] = fa_noise_factor_20_tmp;
								nhm_noise_ratio_20[i] = nhm_noise_ratio_20_tmp;
								nhm_cnt_exp_sum_20[i] = nhm_cnt_exp_sum_20_tmp;
							}
							else if (fa_noise_factor_20[j] == fa_noise_factor_20[i])
							{
								if (nhm_cnt_exp_sum_20[j] < nhm_cnt_exp_sum_20[i])
								{
									idx_20_tmp = idx_20[j];
									clit_est_20_tmp = clit_est_20[j];
									clm_cnt_20_tmp = clm_cnt_20[j];
									clm_ratio_20_tmp = clm_ratio_20[j];
									fa_noise_factor_20_tmp = fa_noise_factor_20[j];
									nhm_noise_ratio_20_tmp = nhm_noise_ratio_20[j];
									nhm_cnt_exp_sum_20_tmp = nhm_cnt_exp_sum_20[j];

									idx_20[j] = idx_20[i];
									clit_est_20[j] = clit_est_20[i];
									clm_cnt_20[j] = clm_cnt_20[i];
									clm_ratio_20[j] = clm_ratio_20[i];
									fa_noise_factor_20[j] = fa_noise_factor_20[i];
									nhm_noise_ratio_20[j] = nhm_noise_ratio_20[i];
									nhm_cnt_exp_sum_20[j] = nhm_cnt_exp_sum_20[i];

									idx_20[i] = idx_20_tmp;
									clit_est_20[i] = clit_est_20_tmp;
									clm_cnt_20[i] = clm_cnt_20_tmp;
									clm_ratio_20[i] = clm_ratio_20_tmp;
									fa_noise_factor_20[i] = fa_noise_factor_20_tmp;
									nhm_noise_ratio_20[i] = nhm_noise_ratio_20_tmp;
									nhm_cnt_exp_sum_20[i] = nhm_cnt_exp_sum_20_tmp;
								}
							}
						}
					}
				}
			}

			ACSDEBUG("\n[ACS2] after sorting:\n");
			for (i=0; i<3; i++)
			{
				ACSDEBUG("[ACS2] (%d) idx_20_%d, clit_est_20: %d, clm_cnt_20: %d, clm_ratio_20: %d, fa_noise_factor_20: %d, nhm_noise_ratio_20: %d, nhm_cnt_exp_sum_20: %d\n",
					i, idx_20[i], clit_est_20[i], clm_cnt_20[i], clm_ratio_20[i], fa_noise_factor_20[i], nhm_noise_ratio_20[i], nhm_cnt_exp_sum_20[i]);
			}

			idx = idx_20[0];
			ACSDEBUG("[ACS2] ==> idx: %d\n\n", idx);
		}
	}

	return idx;
}

int selectClearChannel_5g_acs2(struct rtl8192cd_priv *priv)
{
	int i, j, idx=0;
	unsigned int y, ch_begin=0, ch_end= priv->available_chnl_num;

	unsigned int ap_count[64];
	unsigned int ap_count_interf[64];
	unsigned int dig_upper_bond;
	int max_DIG_cover_bond;

	unsigned int clit_est[64]; // channel loading & interference time (CLIT) estimation
	unsigned int clm_ratio[64];
	unsigned int clm_ratio_ori[64];

	unsigned int fa_total_cnt;
	unsigned int fa_noise_factor[64];

	unsigned int nhm_noise_ratio[64]; // can't be reduced by IGI upper bond
	unsigned int nhm_cnt_exp_sum[64];

	memset(ap_count, 0, sizeof(ap_count));
	memset(ap_count_interf, 0, sizeof(ap_count_interf));
	memset(nhm_cnt_exp_sum, 0, sizeof(nhm_cnt_exp_sum));

	// Step1
	find_interf_ap_count_acs2(priv, 0, priv->available_chnl_num, ap_count_interf);

	dig_upper_bond = priv->pshare->rf_ft_var.dbg_dig_upper;
	if (priv->pshare->rf_ft_var.dbg_dig_upper < priv->pshare->rf_ft_var.dbg_dig_lower)
		dig_upper_bond = priv->pshare->rf_ft_var.dbg_dig_lower;

	if (priv->pmib->dot11RFEntry.acs2_cca_cap_db > ACS_CCA_CAP_MAX)
			priv->pmib->dot11RFEntry.acs2_cca_cap_db = ACS_CCA_CAP_MAX;

	max_DIG_cover_bond = (dig_upper_bond - priv->pmib->dot11RFEntry.acs2_cca_cap_db);

	// Step2
	for (y = ch_begin; y < ch_end; y++)
	{
		// CLM
		if ((CLM_SAMPLE_NUM * priv->pmib->dot11RFEntry.acs2_round) != 0 )
			clm_ratio_ori[y] = (priv->clm_cnt[y] * 100) / (CLM_SAMPLE_NUM * priv->pmib->dot11RFEntry.acs2_round);
		else
			clm_ratio_ori[y] = 100;

		clm_ratio[y] = clm_ratio_ori[y];
		if (priv->pmib->dot11RFEntry.acs2_clm_weighting_mode == 1)
		{
			if (ap_count_interf[y] != 0)
				clm_ratio[y] = clm_ratio[y] / 2;
		}
		else if (priv->pmib->dot11RFEntry.acs2_clm_weighting_mode == 2)
		{
			if (ap_count_interf[y] != 0)
				clm_ratio[y] = (clm_ratio[y] * ap_count_interf[y])/ (ap_count_interf[y] + 1);
		}

		// FA
		fa_total_cnt = priv->chnl_ss_fa_count[y];
		if (fa_total_cnt <= priv->chnl_ss_cca_count[y])
		{
			if (priv->chnl_ss_cca_count[y] != 0)
				fa_noise_factor[y] = fa_total_cnt * 100 / priv->chnl_ss_cca_count[y];
			else
				fa_noise_factor[y] = 0;
		}
		else
			fa_noise_factor[y] = 100;

		// NHM
		if (max_DIG_cover_bond >= 0x26)
		{
			for (j = 0; j < 8; j++) // round 1
				nhm_cnt_exp_sum[y] += priv->nhm_cnt_round[0][y][j] << j;

			nhm_noise_ratio[y] = (priv->nhm_cnt_round[0][y][8] * 100) / (255 * priv->pmib->dot11RFEntry.acs2_round);
		}
		else if (max_DIG_cover_bond > 0x20 && max_DIG_cover_bond < 0x26)
		{
			for (j = 0; j < 7; j++) // round 1
				nhm_cnt_exp_sum[y] += priv->nhm_cnt_round[0][y][j] << j;

			nhm_noise_ratio[y] = (priv->nhm_cnt_round[0][y][7] * 100) / (255 * priv->pmib->dot11RFEntry.acs2_round);
		}
		else
		{
			for (j = 0; j < 6; j++) // round 1
				nhm_cnt_exp_sum[y] += priv->nhm_cnt_round[0][y][j] << j;

			nhm_noise_ratio[y] = (priv->nhm_cnt_round[0][y][6] * 100) / (255 * priv->pmib->dot11RFEntry.acs2_round);
		}

		// CLIT
		clit_est[y] = clm_ratio[y] + nhm_noise_ratio[y];

		ACSDEBUG("\n[CH_%d] clm_cnt: [%d], clm_ratio_ori: [%d](ap:%d, m:%d), clm_ratio: [%d], fa_noise_factor: [%d]\n",
			priv->available_chnl[y], priv->clm_cnt[y], clm_ratio_ori[y], ap_count_interf[y], priv->pmib->dot11RFEntry.acs2_clm_weighting_mode, clm_ratio[y], fa_noise_factor[y]);
		ACSDEBUG("[CH_%d] cca: [%d], fa: [%d], cck_fa: [%d], ofdm_fa: [%d]\n",
			priv->available_chnl[y], priv->chnl_ss_cca_count[y], priv->chnl_ss_fa_count[y], priv->chnl_ss_cck_fa_count[y], priv->chnl_ss_ofdm_fa_count[y]);
		for (j = 0; j < 1; j++)
		{
			if (max_DIG_cover_bond >= 0x26)
			{
				ACSDEBUG("[CH_%d] nhm_cnt_round%d: H<-[ %3d %3d <%3d> %3d %3d %3d %3d %3d %3d %3d %3d]->L, dbg_dig_upper : 0x%x\n",
					priv->available_chnl[y], j+1, priv->nhm_cnt_round[j][y][10], priv->nhm_cnt_round[j][y][9], priv->nhm_cnt_round[j][y][8], priv->nhm_cnt_round[j][y][7],
					priv->nhm_cnt_round[j][y][6], priv->nhm_cnt_round[j][y][5], priv->nhm_cnt_round[j][y][4],
					priv->nhm_cnt_round[j][y][3], priv->nhm_cnt_round[j][y][2], priv->nhm_cnt_round[j][y][1],
					priv->nhm_cnt_round[j][y][0], priv->pshare->rf_ft_var.dbg_dig_upper);
			}
			else if (max_DIG_cover_bond > 0x20 && max_DIG_cover_bond < 0x26)
			{
				ACSDEBUG("[CH_%d] nhm_cnt_round%d: H<-[ %3d %3d %3d <%3d> %3d %3d %3d %3d %3d %3d %3d]->L, dbg_dig_upper : 0x%x\n",
					priv->available_chnl[y], j+1, priv->nhm_cnt_round[j][y][10], priv->nhm_cnt_round[j][y][9], priv->nhm_cnt_round[j][y][8], priv->nhm_cnt_round[j][y][7],
					priv->nhm_cnt_round[j][y][6], priv->nhm_cnt_round[j][y][5], priv->nhm_cnt_round[j][y][4],
					priv->nhm_cnt_round[j][y][3], priv->nhm_cnt_round[j][y][2], priv->nhm_cnt_round[j][y][1],
					priv->nhm_cnt_round[j][y][0], priv->pshare->rf_ft_var.dbg_dig_upper);
			}
			else
			{
				ACSDEBUG("[CH_%d] nhm_cnt_round%d: H<-[ %3d %3d %3d %3d <%3d> %3d %3d %3d %3d %3d %3d]->L, dbg_dig_upper : 0x%x\n",
					priv->available_chnl[y], j+1, priv->nhm_cnt_round[j][y][10], priv->nhm_cnt_round[j][y][9], priv->nhm_cnt_round[j][y][8], priv->nhm_cnt_round[j][y][7],
					priv->nhm_cnt_round[j][y][6], priv->nhm_cnt_round[j][y][5], priv->nhm_cnt_round[j][y][4],
					priv->nhm_cnt_round[j][y][3], priv->nhm_cnt_round[j][y][2], priv->nhm_cnt_round[j][y][1],
					priv->nhm_cnt_round[j][y][0], priv->pshare->rf_ft_var.dbg_dig_upper);
			}
		}
		ACSDEBUG("[CH_%d] nhm_cnt_exp_sum: [%d], nhm_noise_ratio: [%d], clit_est: [%d]\n\n",
			priv->available_chnl[y], nhm_cnt_exp_sum[y], nhm_noise_ratio[y], clit_est[y]);
	}

	// Step3
	idx = find_clean_channel_acs2(priv, 0, priv->available_chnl_num, nhm_cnt_exp_sum, ap_count);
	if (priv->pmib->dot11RFEntry.acs2_dis_clean_channel)
	{
		printk("[ACS2] !!!! Ignore clean channel result\n");
		idx = -1;
	}

	if (idx >= 0)
	{
		printk("[ACS2] !!!! Found clean channel\n");
	}
	else
	{
		printk("[ACS2] !!!! Not found clean channel, use NHM/CLM algorithm\n");

		printk("\n");
		ACSDEBUG("[ACS2] CLM and NHM Info:\n");

		unsigned int check_ch_cnt = 0;
		unsigned int idx_info[MAX_NUM_20M_CH];
		unsigned int clit_est_info[MAX_NUM_20M_CH];
		unsigned int clm_cnt_info[MAX_NUM_20M_CH];
		unsigned int clm_ratio_info[MAX_NUM_20M_CH];
		unsigned int fa_noise_factor_info[MAX_NUM_20M_CH];
		unsigned int nhm_noise_ratio_info[MAX_NUM_20M_CH];
		unsigned int nhm_cnt_exp_sum_info[MAX_NUM_20M_CH];

		for (i = 0; i < priv->available_chnl_num; i++) // get NHM/CLM info
		{
			int ch = priv->available_chnl[i];
			ACSDEBUG("[CH_%d] clm_cnt: [%d], clm_ratio_ori: [%d](ap_i:%d, ap:%d), clm_ratio: [%d], fa_noise_factor: [%d]\n", priv->available_chnl[i], priv->clm_cnt[i], clm_ratio_ori[i], ap_count_interf[i], ap_count[i], clm_ratio[i], fa_noise_factor[i]);
			for (j = 0; j < 1; j++)
			{
				if (max_DIG_cover_bond >= 0x26)
				{
					ACSDEBUG("[CH_%d] nhm_cnt_round%d: H<-[ %3d %3d <%3d> %3d %3d %3d %3d %3d %3d %3d %3d]->L, dbg_dig_upper : 0x%x\n",
						priv->available_chnl[i], j+1, priv->nhm_cnt_round[j][i][10], priv->nhm_cnt_round[j][i][9], priv->nhm_cnt_round[j][i][8], priv->nhm_cnt_round[j][i][7],
						priv->nhm_cnt_round[j][i][6], priv->nhm_cnt_round[j][i][5], priv->nhm_cnt_round[j][i][4],
						priv->nhm_cnt_round[j][i][3], priv->nhm_cnt_round[j][i][2], priv->nhm_cnt_round[j][i][1],
						priv->nhm_cnt_round[j][i][0], priv->pshare->rf_ft_var.dbg_dig_upper);
				}
				else if (max_DIG_cover_bond > 0x20 && max_DIG_cover_bond < 0x26)
				{
					ACSDEBUG("[CH_%d] nhm_cnt_round%d: H<-[ %3d %3d %3d <%3d> %3d %3d %3d %3d %3d %3d %3d]->L, dbg_dig_upper : 0x%x\n",
						priv->available_chnl[i], j+1, priv->nhm_cnt_round[j][i][10], priv->nhm_cnt_round[j][i][9], priv->nhm_cnt_round[j][i][8], priv->nhm_cnt_round[j][i][7],
						priv->nhm_cnt_round[j][i][6], priv->nhm_cnt_round[j][i][5], priv->nhm_cnt_round[j][i][4],
						priv->nhm_cnt_round[j][i][3], priv->nhm_cnt_round[j][i][2], priv->nhm_cnt_round[j][i][1],
						priv->nhm_cnt_round[j][i][0], priv->pshare->rf_ft_var.dbg_dig_upper);
				}
				else
				{
					ACSDEBUG("[CH_%d] nhm_cnt_round%d: H<-[ %3d %3d %3d %3d <%3d> %3d %3d %3d %3d %3d %3d]->L, dbg_dig_upper : 0x%x\n",
						priv->available_chnl[i], j+1, priv->nhm_cnt_round[j][i][10], priv->nhm_cnt_round[j][i][9], priv->nhm_cnt_round[j][i][8], priv->nhm_cnt_round[j][i][7],
						priv->nhm_cnt_round[j][i][6], priv->nhm_cnt_round[j][i][5], priv->nhm_cnt_round[j][i][4],
						priv->nhm_cnt_round[j][i][3], priv->nhm_cnt_round[j][i][2], priv->nhm_cnt_round[j][i][1],
						priv->nhm_cnt_round[j][i][0], priv->pshare->rf_ft_var.dbg_dig_upper);
				}
			}
			ACSDEBUG("[CH_%d] nhm_cnt_exp_sum: [%d], nhm_noise_ratio: [%d], clit_est: [%d]\n",
				priv->available_chnl[i], nhm_cnt_exp_sum[i], nhm_noise_ratio[i], clit_est[i]);
			ACSDEBUG("\n");
		}

		for (i = 0; i < MAX_NUM_20M_CH; i++) // init value
		{
			idx_info[i] = 0;
			clit_est_info[i] = 200;
			clm_cnt_info[i] = 0xFFFFFFFF;
			clm_ratio_info[i] = 200;
			fa_noise_factor_info[i] = 100;
			nhm_noise_ratio_info[i] = 100;
			nhm_cnt_exp_sum_info[i] = 0xFFFFFFFF;
		}

		for (i = 0; i < priv->available_chnl_num; i++) // get NHM/CLM info
		{
			int ch = priv->available_chnl[i];
			int sorting_flag = 0;
			if (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_80)
			{
				if (is80MChannel(priv->available_chnl, priv->available_chnl_num, ch))
					sorting_flag = 1;
			}
			else if (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_20_40)
			{
				if (is40MChannel(priv->available_chnl, priv->available_chnl_num, ch))
					sorting_flag = 1;
			}
			else
			{
				sorting_flag = 1;
			}

#ifdef HUAWEI_CUSTOM_CN_AUTO_AVAIL_CH
			if (priv->pmib->miscEntry.autoch_149to165_enable)
			{
				if (!((ch >= 149) && (ch <= 165)))
					sorting_flag = 0;
			}
#endif
			if (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_80)
			{
				if (ap_count[i] == 0)
					sorting_flag = 0;
			}

			if (sorting_flag)
			{
				idx_info[check_ch_cnt] = i;
				clit_est_info[check_ch_cnt] = clit_est[i];
				clm_cnt_info[check_ch_cnt] = priv->clm_cnt[i];
				clm_ratio_info[check_ch_cnt] = clm_ratio[i];
				fa_noise_factor_info[check_ch_cnt] = fa_noise_factor[i];
				nhm_noise_ratio_info[check_ch_cnt] = nhm_noise_ratio[i];
				nhm_cnt_exp_sum_info[check_ch_cnt] = nhm_cnt_exp_sum[i];

				check_ch_cnt++;
			}
		}

		ACSDEBUG("\n");
		ACSDEBUG("[ACS2] CLM/NHM Checking:\n");
		ACSDEBUG("[ACS2] CLM_SAMPLE_NUM: %d, acs2_round: %d\n", CLM_SAMPLE_NUM, priv->pmib->dot11RFEntry.acs2_round);
		ACSDEBUG("[ACS2] before sorting:\n");
		for (i = 0; i < check_ch_cnt; i++)
		{
			ACSDEBUG("[ACS2-%02d] CH: %3d, idx_info_%02d, clit_est_info: %d, clm_cnt_info: %d, clm_ratio_info: %d\n",
				i, priv->available_chnl[idx_info[i]], idx_info[i], clit_est_info[i], clm_cnt_info[i], clm_ratio_info[i]);
			ACSDEBUG("[ACS2-%02d] CH: %3d, fa_noise_factor_info: %d, nhm_noise_ratio_info: %d, nhm_cnt_exp_sum_info: %d\n\n",
				i, priv->available_chnl[idx_info[i]], fa_noise_factor_info[i], nhm_noise_ratio_info[i], nhm_cnt_exp_sum_info[i]);
		}

		if (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_80)
		{
			for (i = 0; i < check_ch_cnt; i++) // high priority with large CLM value
			{
				unsigned int idx_info_tmp;
				unsigned int clm_cnt_info_tmp;

				for (j = i; j < check_ch_cnt; j++)
				{
					if (clm_cnt_info[j] > clm_cnt_info[i])
					{
						idx_info_tmp = idx_info[j];
						clm_cnt_info_tmp = clm_cnt_info[j];

						idx_info[j] = idx_info[i];
						clm_cnt_info[j] = clm_cnt_info[i];

						idx_info[i] = idx_info_tmp;
						clm_cnt_info[i] = clm_cnt_info_tmp;
					}
				}
			}
		}
		else
		{
			for (i = 0; i < check_ch_cnt; i++) // high priority with small NHM/CLM value
			{
				unsigned int idx_info_tmp;
				unsigned int clit_est_info_tmp;
				unsigned int clm_cnt_info_tmp;
				unsigned int clm_ratio_info_tmp;
				unsigned int fa_noise_factor_info_tmp;
				unsigned int nhm_noise_ratio_info_tmp;
				unsigned int nhm_cnt_exp_sum_info_tmp;

				for (j = i; j < check_ch_cnt; j++)
				{
					if (clit_est_info[j] < clit_est_info[i])
					{
						idx_info_tmp = idx_info[j];
						clit_est_info_tmp = clit_est_info[j];
						clm_cnt_info_tmp = clm_cnt_info[j];
						clm_ratio_info_tmp = clm_ratio_info[j];
						fa_noise_factor_info_tmp = fa_noise_factor_info[j];
						nhm_noise_ratio_info_tmp = nhm_noise_ratio_info[j];
						nhm_cnt_exp_sum_info_tmp = nhm_cnt_exp_sum_info[j];

						idx_info[j] = idx_info[i];
						clit_est_info[j] = clit_est_info[i];
						clm_cnt_info[j] = clm_cnt_info[i];
						clm_ratio_info[j] = clm_ratio_info[i];
						fa_noise_factor_info[j] = fa_noise_factor_info[i];
						nhm_noise_ratio_info[j] = nhm_noise_ratio_info[i];
						nhm_cnt_exp_sum_info[j] = nhm_cnt_exp_sum_info[i];

						idx_info[i] = idx_info_tmp;
						clit_est_info[i] = clit_est_info_tmp;
						clm_cnt_info[i] = clm_cnt_info_tmp;
						clm_ratio_info[i] = clm_ratio_info_tmp;
						fa_noise_factor_info[i] = fa_noise_factor_info_tmp;
						nhm_noise_ratio_info[i] = nhm_noise_ratio_info_tmp;
						nhm_cnt_exp_sum_info[i] = nhm_cnt_exp_sum_info_tmp;
					}
					else if (clit_est_info[j] == clit_est_info[i])
					{
						if (nhm_noise_ratio_info[j] < nhm_noise_ratio_info[i])
						{
							idx_info_tmp = idx_info[j];
							clit_est_info_tmp = clit_est_info[j];
							clm_cnt_info_tmp = clm_cnt_info[j];
							clm_ratio_info_tmp = clm_ratio_info[j];
							fa_noise_factor_info_tmp = fa_noise_factor_info[j];
							nhm_noise_ratio_info_tmp = nhm_noise_ratio_info[j];
							nhm_cnt_exp_sum_info_tmp = nhm_cnt_exp_sum_info[j];

							idx_info[j] = idx_info[i];
							clit_est_info[j] = clit_est_info[i];
							clm_cnt_info[j] = clm_cnt_info[i];
							clm_ratio_info[j] = clm_ratio_info[i];
							fa_noise_factor_info[j] = fa_noise_factor_info[i];
							nhm_noise_ratio_info[j] = nhm_noise_ratio_info[i];
							nhm_cnt_exp_sum_info[j] = nhm_cnt_exp_sum_info[i];

							idx_info[i] = idx_info_tmp;
							clit_est_info[i] = clit_est_info_tmp;
							clm_cnt_info[i] = clm_cnt_info_tmp;
							clm_ratio_info[i] = clm_ratio_info_tmp;
							fa_noise_factor_info[i] = fa_noise_factor_info_tmp;
							nhm_noise_ratio_info[i] = nhm_noise_ratio_info_tmp;
							nhm_cnt_exp_sum_info[i] = nhm_cnt_exp_sum_info_tmp;
						}
						else if (nhm_noise_ratio_info[j] == nhm_noise_ratio_info[i])
						{
							if (fa_noise_factor_info[j] < fa_noise_factor_info[i])
							{
								idx_info_tmp = idx_info[j];
								clit_est_info_tmp = clit_est_info[j];
								clm_cnt_info_tmp = clm_cnt_info[j];
								clm_ratio_info_tmp = clm_ratio_info[j];
								fa_noise_factor_info_tmp = fa_noise_factor_info[j];
								nhm_noise_ratio_info_tmp = nhm_noise_ratio_info[j];
								nhm_cnt_exp_sum_info_tmp = nhm_cnt_exp_sum_info[j];

								idx_info[j] = idx_info[i];
								clit_est_info[j] = clit_est_info[i];
								clm_cnt_info[j] = clm_cnt_info[i];
								clm_ratio_info[j] = clm_ratio_info[i];
								fa_noise_factor_info[j] = fa_noise_factor_info[i];
								nhm_noise_ratio_info[j] = nhm_noise_ratio_info[i];
								nhm_cnt_exp_sum_info[j] = nhm_cnt_exp_sum_info[i];

								idx_info[i] = idx_info_tmp;
								clit_est_info[i] = clit_est_info_tmp;
								clm_cnt_info[i] = clm_cnt_info_tmp;
								clm_ratio_info[i] = clm_ratio_info_tmp;
								fa_noise_factor_info[i] = fa_noise_factor_info_tmp;
								nhm_noise_ratio_info[i] = nhm_noise_ratio_info_tmp;
								nhm_cnt_exp_sum_info[i] = nhm_cnt_exp_sum_info_tmp;
							}
							else if (fa_noise_factor_info[j] == fa_noise_factor_info[i])
							{
								if (nhm_cnt_exp_sum_info[j] < nhm_cnt_exp_sum_info[i])
								{
									idx_info_tmp = idx_info[j];
									clit_est_info_tmp = clit_est_info[j];
									clm_cnt_info_tmp = clm_cnt_info[j];
									clm_ratio_info_tmp = clm_ratio_info[j];
									fa_noise_factor_info_tmp = fa_noise_factor_info[j];
									nhm_noise_ratio_info_tmp = nhm_noise_ratio_info[j];
									nhm_cnt_exp_sum_info_tmp = nhm_cnt_exp_sum_info[j];

									idx_info[j] = idx_info[i];
									clit_est_info[j] = clit_est_info[i];
									clm_cnt_info[j] = clm_cnt_info[i];
									clm_ratio_info[j] = clm_ratio_info[i];
									fa_noise_factor_info[j] = fa_noise_factor_info[i];
									nhm_noise_ratio_info[j] = nhm_noise_ratio_info[i];
									nhm_cnt_exp_sum_info[j] = nhm_cnt_exp_sum_info[i];

									idx_info[i] = idx_info_tmp;
									clit_est_info[i] = clit_est_info_tmp;
									clm_cnt_info[i] = clm_cnt_info_tmp;
									clm_ratio_info[i] = clm_ratio_info_tmp;
									fa_noise_factor_info[i] = fa_noise_factor_info_tmp;
									nhm_noise_ratio_info[i] = nhm_noise_ratio_info_tmp;
									nhm_cnt_exp_sum_info[i] = nhm_cnt_exp_sum_info_tmp;
								}
							}
						}
					}
				}
			}
		}

		ACSDEBUG("\n[ACS2] after sorting:\n");
		for (i = 0; i < check_ch_cnt; i++)
		{
			ACSDEBUG("[ACS2-%02d] CH: %3d, idx_info_%02d, clit_est_info: %d, clm_cnt_info: %d, clm_ratio_info: %d\n",
				i, priv->available_chnl[idx_info[i]], idx_info[i], clit_est_info[i], clm_cnt_info[i], clm_ratio_info[i]);
			ACSDEBUG("[ACS2-%02d] CH: %3d, fa_noise_factor_info: %d, nhm_noise_ratio_info: %d, nhm_cnt_exp_sum_info: %d\n\n",
				i, priv->available_chnl[idx_info[i]], fa_noise_factor_info[i], nhm_noise_ratio_info[i], nhm_cnt_exp_sum_info[i]);
		}

		idx = idx_info[0];
	}

	ACSDEBUG("[ACS2] ==> idx: %d ", idx);
	if (idx != -1)
		ACSDEBUG("(CH : %d, BW : %dM)\n\n", priv->available_chnl[idx], priv->pmib->dot11nConfigEntry.dot11nUse40M==0?20:(priv->pmib->dot11nConfigEntry.dot11nUse40M*40));
	else
		ACSDEBUG("\n\n");

	return idx;
}
#endif
int selectClearChannel(struct rtl8192cd_priv *priv)
{
	static unsigned int score2G[MAX_2G_CHANNEL_NUM], score5G[MAX_5G_CHANNEL_NUM];
	unsigned int score[MAX_BSS_NUM], use_nhm = 0;
	unsigned int minScore=0xffffffff;
	unsigned int tmpScore, tmpIdx=0;
	unsigned int traffic_check 			= 0;
	unsigned int fa_count_weighting = 1;
	int i, j, idx=0, idx_2G_end=-1, idx_5G_begin=-1, minChan=0;
	struct bss_desc *pBss=NULL;

	//move here for function use
	unsigned int y, ch_begin=0, ch_end= priv->available_chnl_num;
	unsigned int do_ap_check = 1, ap_ratio = 0;

#ifdef _DEBUG_RTL8192CD_
	char tmpbuf[400];
	int len=0;
#endif

	memset(score2G, '\0', sizeof(score2G));
	memset(score5G, '\0', sizeof(score5G));

	for (i=0; i<priv->available_chnl_num; i++) {
		if (priv->available_chnl[i] <= 14)
			idx_2G_end = i;
		else
			break;
	}

	for (i=0; i<priv->available_chnl_num; i++) {
		if (priv->available_chnl[i] > 14) {
			idx_5G_begin = i;
			break;
		}
	}

#ifdef ACS_DEBUG_INFO//for debug
	for (i=0; i<priv->site_survey->count; i++) {
		int bw;
		pBss = &priv->site_survey->bss[i];
#ifdef RTK_AC_SUPPORT
		if (pBss->t_stamp[1] & (BSS_BW_MASK << BSS_BW_SHIFT)) {
			int tmp = (pBss->t_stamp[1] >> BSS_BW_SHIFT) & BSS_BW_MASK;
			switch (tmp) {
			case 0:
				bw = 20;
				break;
			case 1:
				bw = 40;
				break;
			case 2:
				bw = 80;
				break;
			case 3:
				bw = 160;
				break;
			case 4:
				bw = 10;
				break;
			case 5:
				bw = 5;
				break;
			}
		}
		else
#endif
		if (pBss->t_stamp[1] & BIT(1))
			bw = 40;
		else
			bw = 20;

		printk("[%3d] MAC %02x%02x%02x%02x%02x%02x ch %3d %3dM mode\n",
			i, pBss->bssid[0], pBss->bssid[1], pBss->bssid[2], pBss->bssid[3], pBss->bssid[4], pBss->bssid[5],
			pBss->channel, bw);
	}
#endif

#ifndef CONFIG_RTL_NEW_AUTOCH
	for (i=0; i<priv->site_survey->count; i++) {
		pBss = &priv->site_survey->bss[i];
		for (idx=0; idx<priv->available_chnl_num; idx++) {
			if (pBss->channel == priv->available_chnl[idx]) {
				if (pBss->channel <= 14)
					setChannelScore(idx, score2G, 0, MAX_2G_CHANNEL_NUM-1);
				else
					score5G[idx - idx_5G_begin] += 5;
				break;
			}
		}
	}
#endif

	if (idx_2G_end >= 0)
		for (i=0; i<=idx_2G_end; i++)
			score[i] = score2G[i];
	if (idx_5G_begin >= 0)
		for (i=idx_5G_begin; i<priv->available_chnl_num; i++)
			score[i] = score5G[i - idx_5G_begin];

#ifdef CONFIG_RTL_NEW_AUTOCH
	{
		if (idx_2G_end >= 0)
			ch_end = idx_2G_end+1;
		if (idx_5G_begin >= 0)
			ch_begin = idx_5G_begin;

#ifdef ACS_DEBUG_INFO//for debug
		printk("\n");
		for (y=ch_begin; y<ch_end; y++)
			printk("1. init: chnl[%d] 20M_rx[%d] 40M_rx[%d] fa_cnt[%d] score[%d]\n",
				priv->available_chnl[y],
				priv->chnl_ss_mac_rx_count[y],
				priv->chnl_ss_mac_rx_count_40M[y],
				priv->chnl_ss_fa_count[y],
				score[y]);
		printk("\n");
#endif

		if (priv->pmib->dot11RFEntry.acs_type == 1) {
			unsigned int tmp_score[MAX_BSS_NUM];
			memcpy(tmp_score, score, sizeof(score));
			if (find_clean_channel(priv, ch_begin, ch_end, tmp_score)) {
				//memcpy(score, tmp_score, sizeof(score));
				DEBUG_INFO("!! Found clean channel, select minimum FA channel\n");
				if ( priv->pshare->acs_for_adaptivity_flag ) {
					DEBUG_INFO("!! But acs_for_adaptivity enable, use NHM algorithm\n");
					use_nhm = 1;
				}
			} else {
				DEBUG_INFO("!! Not found clean channel, use NHM algorithm\n");
				use_nhm = 1;
			}
			for (y=ch_begin; y<ch_end; y++) {
				for (i=0; i<=9; i++) {
					unsigned int val32 = priv->nhm_cnt[y][i];
					for (j=0; j<i; j++)
						val32 *= 3;
					score[y] += val32;
				}

#ifdef _DEBUG_RTL8192CD_
				printk("nhm_cnt_%d: H<-[ %3d %3d %3d %3d %3d %3d %3d %3d %3d %3d]->L, score: %d\n",
					y+1, priv->nhm_cnt[y][9], priv->nhm_cnt[y][8], priv->nhm_cnt[y][7],
					priv->nhm_cnt[y][6], priv->nhm_cnt[y][5], priv->nhm_cnt[y][4],
					priv->nhm_cnt[y][3], priv->nhm_cnt[y][2], priv->nhm_cnt[y][1],
					priv->nhm_cnt[y][0], score[y]);
#endif
			}

			if (!use_nhm)
				memcpy(score, tmp_score, sizeof(score));

			goto choose_ch;
		}
#ifdef NHM_ACS2_SUPPORT
		else if (priv->pmib->dot11RFEntry.acs_type==2)
		{
			ACSDEBUG("[CH:%d] [BW:%d] [Use40M:%d] [RF0x18:%d]\n", priv->site_survey->ss_channel, priv->pshare->CurrentChannelBW, priv->pmib->dot11nConfigEntry.dot11nUse40M, PHY_QueryRFReg(priv, RF92CD_PATH_A, 0x18, 0xff, 1));

			RTL_W8(0xa0a, priv->cck_th_backup); // restore "cck th"

			goto choose_ch;
		}
#endif
		/*
		 * 	For each channel, weighting behind channels with MAC RX counter
		 * 	For each channel, weighting the channel with FA counter
		 */
		if (priv->pmib->dot11RFEntry.acs_type != 3) {
		for (y=ch_begin; y<ch_end; y++) {
			score[y] += 8 * priv->chnl_ss_mac_rx_count[y];
			if (priv->chnl_ss_mac_rx_count[y] > 30)
				do_ap_check = 0;
			if( priv->chnl_ss_mac_rx_count[y] > MAC_RX_COUNT_THRESHOLD )
				traffic_check = 1;

#ifdef RTK_5G_SUPPORT
			if (priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_2G)
#endif
			{
				if ((int)(y-4) >= (int)ch_begin)
					score[y-4] += 2 * priv->chnl_ss_mac_rx_count[y];
				if ((int)(y-3) >= (int)ch_begin)
					score[y-3] += 8 * priv->chnl_ss_mac_rx_count[y];
				if ((int)(y-2) >= (int)ch_begin)
					score[y-2] += 8 * priv->chnl_ss_mac_rx_count[y];
				if ((int)(y-1) >= (int)ch_begin)
					score[y-1] += 10 * priv->chnl_ss_mac_rx_count[y];
				if ((int)(y+1) < (int)ch_end)
					score[y+1] += 10 * priv->chnl_ss_mac_rx_count[y];
				if ((int)(y+2) < (int)ch_end)
					score[y+2] += 8 * priv->chnl_ss_mac_rx_count[y];
				if ((int)(y+3) < (int)ch_end)
					score[y+3] += 8 * priv->chnl_ss_mac_rx_count[y];
				if ((int)(y+4) < (int)ch_end)
					score[y+4] += 2 * priv->chnl_ss_mac_rx_count[y];
			}

#ifdef SS_CH_LOAD_PROC
			//this is for CH_LOAD caculation
			if( priv->chnl_ss_cca_count[y] > priv->chnl_ss_fa_count[y])
				priv->chnl_ss_cca_count[y]-= priv->chnl_ss_fa_count[y];
			else
				priv->chnl_ss_cca_count[y] = 0;
#endif
		}

#ifdef ACS_DEBUG_INFO//for debug
		printk("\n");
		for (y=ch_begin; y<ch_end; y++)
			printk("2. after 20M check: chnl[%d] score[%d]\n",priv->available_chnl[y], score[y]);
		printk("\n");
#endif

		for (y=ch_begin; y<ch_end; y++) {
			if (priv->chnl_ss_mac_rx_count_40M[y]) {
				score[y] += 5 * priv->chnl_ss_mac_rx_count_40M[y];
				if (priv->chnl_ss_mac_rx_count_40M[y] > 30)
					do_ap_check = 0;
				if( priv->chnl_ss_mac_rx_count_40M[y] > MAC_RX_COUNT_THRESHOLD )
					traffic_check = 1;

#ifdef RTK_5G_SUPPORT
				if (priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_2G)
#endif
				{
					if ((int)(y-6) >= (int)ch_begin)
						score[y-6] += 1 * priv->chnl_ss_mac_rx_count_40M[y];
					if ((int)(y-5) >= (int)ch_begin)
						score[y-5] += 4 * priv->chnl_ss_mac_rx_count_40M[y];
					if ((int)(y-4) >= (int)ch_begin)
						score[y-4] += 4 * priv->chnl_ss_mac_rx_count_40M[y];
					if ((int)(y-3) >= (int)ch_begin)
						score[y-3] += 5 * priv->chnl_ss_mac_rx_count_40M[y];
					if ((int)(y-2) >= (int)ch_begin)
						score[y-2] += (5 * priv->chnl_ss_mac_rx_count_40M[y])/2;
					if ((int)(y-1) >= (int)ch_begin)
						score[y-1] += 5 * priv->chnl_ss_mac_rx_count_40M[y];
					if ((int)(y+1) < (int)ch_end)
						score[y+1] += 5 * priv->chnl_ss_mac_rx_count_40M[y];
					if ((int)(y+2) < (int)ch_end)
						score[y+2] += (5 * priv->chnl_ss_mac_rx_count_40M[y])/2;
					if ((int)(y+3) < (int)ch_end)
						score[y+3] += 5 * priv->chnl_ss_mac_rx_count_40M[y];
					if ((int)(y+4) < (int)ch_end)
						score[y+4] += 4 * priv->chnl_ss_mac_rx_count_40M[y];
					if ((int)(y+5) < (int)ch_end)
						score[y+5] += 4 * priv->chnl_ss_mac_rx_count_40M[y];
					if ((int)(y+6) < (int)ch_end)
						score[y+6] += 1 * priv->chnl_ss_mac_rx_count_40M[y];
				}
			}
		}

#ifdef ACS_DEBUG_INFO//for debug
		printk("\n");
		for (y=ch_begin; y<ch_end; y++)
			printk("3. after 40M check: chnl[%d] score[%d]\n",priv->available_chnl[y], score[y]);
		printk("\n");
		printk("4. do_ap_check=%d traffic_check=%d\n", do_ap_check, traffic_check);
		printk("\n");
#endif

		if( traffic_check == 0)
			fa_count_weighting = 5;
		else
			fa_count_weighting = 1;

		for (y=ch_begin; y<ch_end; y++) {
			score[y] += fa_count_weighting * priv->chnl_ss_fa_count[y];
		}

#ifdef ACS_DEBUG_INFO//for debug
		printk("\n");
		for (y=ch_begin; y<ch_end; y++)
			printk("5. after fa check: chnl[%d] score[%d]\n",priv->available_chnl[y], score[y]);
		printk("\n");
#endif
		} /* if acs_type != 3 */

		if (do_ap_check) {
			for (i=0; i<priv->site_survey->count; i++) {
				pBss = &priv->site_survey->bss[i];
				for (y=ch_begin; y<ch_end; y++) {
					if (pBss->channel == priv->available_chnl[y]) {
						if (pBss->channel <= 14) {
#ifdef ACS_DEBUG_INFO//for debug
						printk("\n");
						printk("chnl[%d] has ap rssi=%d bw[0x%02x]\n",
							pBss->channel, pBss->rssi, pBss->t_stamp[1]);
						printk("\n");
#endif
							if (pBss->rssi > 60)
								ap_ratio = 4;
							else if (pBss->rssi > 35)
								ap_ratio = 2;
							else
								ap_ratio = 1;

							if ((pBss->t_stamp[1] & 0x6) == 0) {
								score[y] += 50 * ap_ratio;
								if ((int)(y-4) >= (int)ch_begin)
									score[y-4] += 10 * ap_ratio;
								if ((int)(y-3) >= (int)ch_begin)
									score[y-3] += 20 * ap_ratio;
								if ((int)(y-2) >= (int)ch_begin)
									score[y-2] += 30 * ap_ratio;
								if ((int)(y-1) >= (int)ch_begin)
									score[y-1] += 40 * ap_ratio;
								if ((int)(y+1) < (int)ch_end)
									score[y+1] += 40 * ap_ratio;
								if ((int)(y+2) < (int)ch_end)
									score[y+2] += 30 * ap_ratio;
								if ((int)(y+3) < (int)ch_end)
									score[y+3] += 20 * ap_ratio;
								if ((int)(y+4) < (int)ch_end)
									score[y+4] += 10 * ap_ratio;
							}
							else if ((pBss->t_stamp[1] & 0x4) == 0) {
								score[y] += 50 * ap_ratio;
								if ((int)(y-3) >= (int)ch_begin)
									score[y-3] += 20 * ap_ratio;
								if ((int)(y-2) >= (int)ch_begin)
									score[y-2] += 30 * ap_ratio;
								if ((int)(y-1) >= (int)ch_begin)
									score[y-1] += 40 * ap_ratio;
								if ((int)(y+1) < (int)ch_end)
									score[y+1] += 50 * ap_ratio;
								if ((int)(y+2) < (int)ch_end)
									score[y+2] += 50 * ap_ratio;
								if ((int)(y+3) < (int)ch_end)
									score[y+3] += 50 * ap_ratio;
								if ((int)(y+4) < (int)ch_end)
									score[y+4] += 50 * ap_ratio;
								if ((int)(y+5) < (int)ch_end)
									score[y+5] += 40 * ap_ratio;
								if ((int)(y+6) < (int)ch_end)
									score[y+6] += 30 * ap_ratio;
								if ((int)(y+7) < (int)ch_end)
									score[y+7] += 20 * ap_ratio;
							}
							else {
								score[y] += 50 * ap_ratio;
								if ((int)(y-7) >= (int)ch_begin)
									score[y-7] += 20 * ap_ratio;
								if ((int)(y-6) >= (int)ch_begin)
									score[y-6] += 30 * ap_ratio;
								if ((int)(y-5) >= (int)ch_begin)
									score[y-5] += 40 * ap_ratio;
								if ((int)(y-4) >= (int)ch_begin)
									score[y-4] += 50 * ap_ratio;
								if ((int)(y-3) >= (int)ch_begin)
									score[y-3] += 50 * ap_ratio;
								if ((int)(y-2) >= (int)ch_begin)
									score[y-2] += 50 * ap_ratio;
								if ((int)(y-1) >= (int)ch_begin)
									score[y-1] += 50 * ap_ratio;
								if ((int)(y+1) < (int)ch_end)
									score[y+1] += 40 * ap_ratio;
								if ((int)(y+2) < (int)ch_end)
									score[y+2] += 30 * ap_ratio;
								if ((int)(y+3) < (int)ch_end)
									score[y+3] += 20 * ap_ratio;
							}
						}
						else {
							if ((pBss->t_stamp[1] & 0x6) == 0) {
								score[y] += 500;
							}
							else if ((pBss->t_stamp[1] & 0x4) == 0) {
								score[y] += 500;
								if ((int)(y+1) < (int)ch_end)
									score[y+1] += 500;
							}
							else {
								score[y] += 500;
								if ((int)(y-1) >= (int)ch_begin)
									score[y-1] += 500;
							}
						}
						break;
					}
				}
			}
		}

#ifdef ACS_DEBUG_INFO//for debug
		printk("\n");
		for (y=ch_begin; y<ch_end; y++)
			printk("6. after ap check: chnl[%d]:%d\n", priv->available_chnl[y],score[y]);
		printk("\n");
#endif

#ifdef 	SS_CH_LOAD_PROC

		// caculate noise level -- suggested by wilson
		for (y=ch_begin; y<ch_end; y++)  {
			int fa_lv=0, cca_lv=0;
			if (priv->chnl_ss_fa_count[y]>1000) {
				fa_lv = 100;
			} else if (priv->chnl_ss_fa_count[y]>500) {
				fa_lv = 34 * (priv->chnl_ss_fa_count[y]-500) / 500 + 66;
			} else if (priv->chnl_ss_fa_count[y]>200) {
				fa_lv = 33 * (priv->chnl_ss_fa_count[y] - 200) / 300 + 33;
			} else if (priv->chnl_ss_fa_count[y]>100) {
				fa_lv = 18 * (priv->chnl_ss_fa_count[y] - 100) / 100 + 15;
			} else {
				fa_lv = 15 * priv->chnl_ss_fa_count[y] / 100;
			}
			if (priv->chnl_ss_cca_count[y]>400) {
				cca_lv = 100;
			} else if (priv->chnl_ss_cca_count[y]>200) {
				cca_lv = 34 * (priv->chnl_ss_cca_count[y] - 200) / 200 + 66;
			} else if (priv->chnl_ss_cca_count[y]>80) {
				cca_lv = 33 * (priv->chnl_ss_cca_count[y] - 80) / 120 + 33;
			} else if (priv->chnl_ss_cca_count[y]>40) {
				cca_lv = 18 * (priv->chnl_ss_cca_count[y] - 40) / 40 + 15;
			} else {
				cca_lv = 15 * priv->chnl_ss_cca_count[y] / 40;
			}

			priv->chnl_ss_load[y] = (((fa_lv > cca_lv)? fa_lv : cca_lv)*75+((score[y]>100)?100:score[y])*25)/100;

			DEBUG_INFO("ch:%d f=%d , c=%d , fl=%d, cl=%d, sc=%d, cu=%d\n",
					priv->available_chnl[y],
					priv->chnl_ss_fa_count[y],
					priv->chnl_ss_cca_count[y],
					fa_lv,
					cca_lv,
					score[y],
					priv->chnl_ss_load[y]);

		}
#endif
	}
#endif

choose_ch:

	if (priv->pmib->dot11RFEntry.acs_type != 3) {
		// heavy weighted DFS channel
		if (idx_5G_begin >= 0){
			for (i=idx_5G_begin; i<priv->available_chnl_num; i++) {
				if (!priv->pmib->dot11DFSEntry.disable_DFS && is_DFS_channel(priv->available_chnl[i])
				&& (score[i]!= 0xffffffff)){
					score[i] += 1600;
				}
			}
		}
	}


//prevent Auto Channel selecting wrong channel in 40M mode-----------------
	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)
		&& priv->pshare->is_40m_bw) {
#if 0
		if (GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset == 1) {
			//Upper Primary Channel, cannot select the two lowest channels
			if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) {
				score[0] = 0xffffffff;
				score[1] = 0xffffffff;
				score[2] = 0xffffffff;
				score[3] = 0xffffffff;
				score[4] = 0xffffffff;

				score[13] = 0xffffffff;
				score[12] = 0xffffffff;
				score[11] = 0xffffffff;
			}

//			if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11A) {
//				score[idx_5G_begin] = 0xffffffff;
//				score[idx_5G_begin + 1] = 0xffffffff;
//			}
		}
		else if (GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset == 2) {
			//Lower Primary Channel, cannot select the two highest channels
			if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) {
				score[0] = 0xffffffff;
				score[1] = 0xffffffff;
				score[2] = 0xffffffff;

				score[13] = 0xffffffff;
				score[12] = 0xffffffff;
				score[11] = 0xffffffff;
				score[10] = 0xffffffff;
				score[9] = 0xffffffff;
			}

//			if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11A) {
//				score[priv->available_chnl_num - 2] = 0xffffffff;
//				score[priv->available_chnl_num - 1] = 0xffffffff;
//			}
		}
#endif
		for (i=0; i<=idx_2G_end; ++i)
			if (priv->available_chnl[i] == 14)
				score[i] = 0xffffffff;		// mask chan14

#ifdef RTK_5G_SUPPORT
		if (idx_5G_begin >= 0) {
			for (i=idx_5G_begin; i<priv->available_chnl_num; i++) {
				int ch = priv->available_chnl[i];
				if(priv->available_chnl[i] > 144)
					--ch;
				if((ch%4) || ch==140 || ch == 164 )	//mask ch 140, ch 165, ch 184...
					score[i] = 0xffffffff;
			}
		}
#endif


	}

	if (priv->pmib->dot11RFEntry.disable_ch1213) {
		for (i=0; i<=idx_2G_end; ++i) {
			int ch = priv->available_chnl[i];
			if ((ch == 12) || (ch == 13))
				score[i] = 0xffffffff;
		}
	}

	if (((priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_GLOBAL) ||
			(priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_WORLD_WIDE)) &&
		 (idx_2G_end >= 11) && (idx_2G_end < 14)) {
		score[13] = 0xffffffff;	// mask chan14
		score[12] = 0xffffffff; // mask chan13
		score[11] = 0xffffffff; // mask chan12
	}

//------------------------------------------------------------------

#ifdef _DEBUG_RTL8192CD_
	for (i=0; i<priv->available_chnl_num; i++) {
		len += sprintf(tmpbuf+len, "ch%d:%u ", priv->available_chnl[i], score[i]);
	}
	strcat(tmpbuf, "\n");
	panic_printk("%s", tmpbuf);

#endif

	if ( (priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_5G)
		&& (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_80)
		&& (priv->pmib->dot11RFEntry.acs_type != 3))
	{
#ifdef NHM_ACS2_SUPPORT
		if (priv->pmib->dot11RFEntry.acs_type == 2) {
			idx = selectClearChannel_5g_acs2(priv);
		}
		else
#endif
		{
		for (i=0; i<priv->available_chnl_num; i++) {
			if (is80MChannel(priv->available_chnl, priv->available_chnl_num, priv->available_chnl[i])) {
				tmpScore = 0;
				for (j=0; j<4; j++) {
					if ((tmpScore != 0xffffffff) && (score[i+j] != 0xffffffff))
						tmpScore += score[i+j];
					else
						tmpScore = 0xffffffff;
				}
				tmpScore = tmpScore / 4;
				if (minScore > tmpScore) {
					minScore = tmpScore;

					tmpScore = 0xffffffff;
					for (j=0; j<4; j++) {
						if (score[i+j] < tmpScore) {
							tmpScore = score[i+j];
							tmpIdx = i+j;
						}
					}

					idx = tmpIdx;
				}
				i += 3;
			}
		}
		if (minScore == 0xffffffff) {
			// there is no 80M channels
			priv->pshare->is_40m_bw = HT_CHANNEL_WIDTH_20;
			for (i=0; i<priv->available_chnl_num; i++) {
				if (score[i] < minScore) {
					minScore = score[i];
					idx = i;
				}
			}
		}
	}
	}
	else if( (priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_5G)
			&& (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_20_40)
			&& (priv->pmib->dot11RFEntry.acs_type != 3))
 	{
#ifdef NHM_ACS2_SUPPORT
		if (priv->pmib->dot11RFEntry.acs_type == 2) {
			idx = selectClearChannel_5g_acs2(priv);
		}
		else
#endif
 		{
		for (i=0; i<priv->available_chnl_num; i++) {
			if(is40MChannel(priv->available_chnl,priv->available_chnl_num,priv->available_chnl[i])) {
				tmpScore = 0;
				for(j=0;j<2;j++) {
					if ((tmpScore != 0xffffffff) && (score[i+j] != 0xffffffff))
						tmpScore += score[i+j];
					else
						tmpScore = 0xffffffff;
				}
				tmpScore = tmpScore / 2;
				if(minScore > tmpScore) {
					minScore = tmpScore;

					tmpScore = 0xffffffff;
					for (j=0; j<2; j++) {
						if (score[i+j] < tmpScore) {
							tmpScore = score[i+j];
							tmpIdx = i+j;
						}
					}

					idx = tmpIdx;
				}
				i += 1;
			}
		}
		if (minScore == 0xffffffff) {
			// there is no 40M channels
			priv->pshare->is_40m_bw = HT_CHANNEL_WIDTH_20;
			for (i=0; i<priv->available_chnl_num; i++) {
				if (score[i] < minScore) {
					minScore = score[i];
					idx = i;
				}
			}
		}
	}
	}
	else if( (priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_2G)
			&& (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_20_40)
			&& (priv->available_chnl_num >= 8) )
	{
#ifdef NHM_ACS2_SUPPORT
		if (priv->pmib->dot11RFEntry.acs_type == 2) {
			idx = selectClearChannel_2g_acs2(priv);
		}
		else
#endif
	{
		unsigned int groupScore[14];

		memset(groupScore, 0xff , sizeof(groupScore));
		for (i=0; i<priv->available_chnl_num-4; i++) {
			if (score[i] != 0xffffffff && score[i+4] != 0xffffffff) {
				groupScore[i] = score[i] + score[i+4];
				DEBUG_INFO("groupScore, ch %d,%d: %d\n", i+1, i+5, groupScore[i]);
				if (groupScore[i] < minScore) {
					if(priv->pmib->miscEntry.autoch_1611_enable)
					{
						if((priv->available_chnl[i]==1 && (score[i] < score[i+4])) ||
							(priv->available_chnl[i]==2 && (score[i] > score[i+4])) ||
							(priv->available_chnl[i]==6 && (score[i] < score[i+4])) ||
							(priv->available_chnl[i]==7 && (score[i] > score[i+4])))
						{
							minScore = groupScore[i];
							idx = i;
						}
					} else {
						minScore = groupScore[i];
						idx = i;
					}
				}
			}
		}

		if(minScore == 0xffffffff && priv->pmib->miscEntry.autoch_1611_enable){
			for (i=0; i<priv->available_chnl_num; i++) {
				if (score[i] < minScore) {
					if(priv->available_chnl[i]==1 || priv->available_chnl[i]==6 || priv->available_chnl[i]==11)
					{
						minScore = score[i];
						idx = i;
					}
				}
			}

			if(priv->available_chnl[idx] == 1){
				GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_ABOVE;
				priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_ABOVE;
			}else if(priv->available_chnl[idx] == 11){
				GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_BELOW;
				priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_BELOW;
			}else {//channel 6
				if(score[idx-4] > score[idx+4]){
					GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_ABOVE;
					priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_ABOVE;
				}else{
					GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_BELOW;
					priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_BELOW;
				}
			}
		} else {
			if (score[idx] < score[idx+4]) {
				GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_ABOVE;
				priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_ABOVE;
			} else {
				idx = idx + 4;
				GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_BELOW;
				priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_BELOW;
			}
		}
	}
	}
	else
	{
#ifdef NHM_ACS2_SUPPORT
		if (priv->pmib->dot11RFEntry.acs_type == 2)
		{
			if (priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_2G) {
				idx = selectClearChannel_2g_acs2(priv);
			} else {
				idx = selectClearChannel_5g_acs2(priv);
			}
		}
		else
#endif
	{
		for (i=0; i<priv->available_chnl_num; i++) {
			if (score[i] < minScore) {
				if(priv->pmib->miscEntry.autoch_1611_enable)
				{
					if(priv->available_chnl[i]==1 || priv->available_chnl[i]==6 || priv->available_chnl[i]==11)
					{
						minScore = score[i];
						idx = i;
					}
				} else {
					minScore = score[i];
					idx = i;
					}
				}
			}
		}
	}

	if (IS_A_CUT_8881A(priv) &&
		(priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_80)) {
		if ((priv->available_chnl[idx] == 36) ||
			(priv->available_chnl[idx] == 52) ||
			(priv->available_chnl[idx] == 100) ||
			(priv->available_chnl[idx] == 116) ||
			(priv->available_chnl[idx] == 132) ||
			(priv->available_chnl[idx] == 149) ||
			(priv->available_chnl[idx] == 165))
			idx++;
		else if ((priv->available_chnl[idx] == 48) ||
			(priv->available_chnl[idx] == 64) ||
			(priv->available_chnl[idx] == 112) ||
			(priv->available_chnl[idx] == 128) ||
			(priv->available_chnl[idx] == 144) ||
			(priv->available_chnl[idx] == 161) ||
			(priv->available_chnl[idx] == 177))
			idx--;
	}

	minChan = priv->available_chnl[idx];

	// skip channel 14 if don't support ofdm
	if ((priv->pmib->dot11RFEntry.disable_ch14_ofdm) &&
			(minChan == 14)) {
		score[idx] = 0xffffffff;

		minScore = 0xffffffff;
		for (i=0; i<priv->available_chnl_num; i++) {
			if (score[i] < minScore) {
				minScore = score[i];
				idx = i;
			}
		}
		minChan = priv->available_chnl[idx];
	}

#if 0
	//Check if selected channel available for 80M/40M BW or NOT ?
	if(priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_5G)
	{
		if(priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_80)
		{
			if(!is80MChannel(priv->available_chnl,priv->available_chnl_num,minChan))
			{
				//printk("BW=80M, selected channel = %d is unavaliable! reduce to 40M\n", minChan);
				//priv->pmib->dot11nConfigEntry.dot11nUse40M = HT_CHANNEL_WIDTH_20_40;
				priv->pshare->is_40m_bw = HT_CHANNEL_WIDTH_20_40;
			}
		}

		if(priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_20_40)
		{
			if(!is40MChannel(priv->available_chnl,priv->available_chnl_num,minChan))
			{
				//printk("BW=40M, selected channel = %d is unavaliable! reduce to 20M\n", minChan);
				//priv->pmib->dot11nConfigEntry.dot11nUse40M = HT_CHANNEL_WIDTH_20;
				priv->pshare->is_40m_bw = HT_CHANNEL_WIDTH_20;
			}
		}
	}
#endif

#ifdef CONFIG_RTL_NEW_AUTOCH
	RTL_W32(RXERR_RPT, RXERR_RPT_RST);
#endif

// auto adjust contro-sideband
	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)
			&& (priv->pshare->is_40m_bw ==1 || priv->pshare->is_40m_bw ==2)) {

#ifdef RTK_5G_SUPPORT
		if (priv->pmib->dot11RFEntry.phyBandSelect & PHY_BAND_5G) {
			if( (minChan>144) ? ((minChan-1)%8) : (minChan%8)) {
				GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_ABOVE;
				priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_ABOVE;
			} else {
				GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_BELOW;
				priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_BELOW;
			}

		} else
#endif
		{
#if 0
#ifdef CONFIG_RTL_NEW_AUTOCH
			unsigned int ch_max;

			if (priv->available_chnl[idx_2G_end] >= 13)
				ch_max = 13;
			else
				ch_max = priv->available_chnl[idx_2G_end];

			if ((minChan >= 5) && (minChan <= (ch_max-5))) {
				if (score[minChan+4] > score[minChan-4]) { /* what if some channels were cancelled? */
					GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_BELOW;
					priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_BELOW;
				} else {
					GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_ABOVE;
					priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_ABOVE;
				}
			} else
#endif
			{
				if (minChan < 5) {
					GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_ABOVE;
					priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_ABOVE;
				}
				else if (minChan > 7) {
					GET_MIB(priv)->dot11nConfigEntry.dot11n2ndChOffset = HT_2NDCH_OFFSET_BELOW;
					priv->pshare->offset_2nd_chan	= HT_2NDCH_OFFSET_BELOW;
				}
			}
#endif
		}
	}
//-----------------------

#ifdef _DEBUG_RTL8192CD_
	panic_printk("Auto channel choose ch:%d\n", minChan);
#endif
#ifdef ACS_DEBUG_INFO//for debug
	printk("7. minChan:%d 2nd_offset:%d\n", minChan, priv->pshare->offset_2nd_chan);
#endif

	return minChan;
}


#ifdef CONFIG_RTL_WLAN_DOS_FILTER
int issue_disassoc_from_kernel(void *priv, unsigned char *mac)
{
	memcpy(block_mac[block_mac_idx], mac, 6);
	block_mac_idx++;
	block_mac_idx = block_mac_idx % MAX_BLOCK_MAC;

	if (priv != NULL) {
//		issue_disassoc((struct rtl8192cd_priv *)priv, mac, _RSON_UNSPECIFIED_);
		issue_deauth((struct rtl8192cd_priv *)priv, mac, _RSON_UNSPECIFIED_);
		block_sta_time = ((struct rtl8192cd_priv *)priv)->pshare->rf_ft_var.dos_block_time;
		block_priv = (unsigned long)priv;
	}
	return 0;
}
#endif

/**
 *	@brief	issue de-authenticaion
 *
 *	Defragement fail will be de-authentication or STA issue deauthenticaion request
 *
 *	+---------------+-----+----+----+-------+-----+-------------+ \n
 *	| Frame Control | ... | DA | SA | BSSID | ... | Reason Code | \n
 *	+---------------+-----+----+----+-------+-----+-------------+ \n
 */
void issue_deauth(struct rtl8192cd_priv *priv, unsigned char *da, int reason)
{
	struct wifi_mib *pmib;
	unsigned char	*bssid, *pbuf;
	unsigned short  val;
	#if defined(WIFI_WMM)
	int ret;
	#endif
	struct stat_info *pstat = get_stainfo(priv, da);
	DECLARE_TXINSN(txinsn);

	if (!memcmp(da, "\x0\x0\x0\x0\x0\x0", 6))
		return;

	// check if da is legal
	if (da[0] & 0x01
#ifdef CONFIG_IEEE80211W
		&& memcmp(da,"\xff\xff\xff\xff\xff\xff",6)
#endif
		){
		DEBUG_WARN("Send Deauth Req to bad DA %02x%02x%02x%02x%02x%02x", da[0], da[1], da[2], da[3], da[4], da[5]);
		return;
	}

#ifdef TLN_STATS
	stats_conn_rson_counts(priv, reason);
#endif


    if(IS_HAL_CHIP(priv))
    {
        if(pstat && (REMAP_AID(pstat) < 128))
        {
            DEBUG_WARN("%s %d issue_asocrsp, set MACID 0 AID = %x \n",__FUNCTION__,__LINE__,REMAP_AID(pstat));
			if(pstat->txpdrop_flag == 1) {
				GET_HAL_INTERFACE(priv)->UpdateHalMSRRPTHandler(priv, pstat, INCREASE);
				pstat->txpdrop_flag = 0;
			}
            GET_HAL_INTERFACE(priv)->SetMACIDSleepHandler(priv, 0, REMAP_AID(pstat));
			if(priv->pshare->paused_sta_num && pstat->txpause_flag) {
				priv->pshare->paused_sta_num--;
				pstat->txpause_flag =0;
        	}
        }
        else
        {
            DEBUG_WARN(" MACID sleep only support 128 STA \n");
        }
    }
	txinsn.retry = priv->pmib->dot11OperationEntry.dot11ShortRetryLimit;

	pmib= GET_MIB(priv);

	bssid = pmib->dot11StationConfigEntry.dot11Bssid;


	txinsn.q_num = MANAGE_QUE_NUM;
    	txinsn.tx_rate  = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
	txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif
	txinsn.fixed_rate = 1;
	txinsn.fr_type = _PRE_ALLOCMEM_;
#ifdef CONFIG_IEEE80211W
	if(!memcmp(da,"\xff\xff\xff\xff\xff\xff",6)) {
		txinsn.isPMF = 1; //?????
	} else {
		PMFDEBUG("txinsn.isPMF,pstat=%x, da=%02x%02x%02x%02x%02x%02x\n",pstat,da[0],da[1],da[2],da[3],da[4],da[5]);

		if(pstat)
			txinsn.isPMF = pstat->isPMF;
		else
			txinsn.isPMF = 0;

			//else
				//txinsn.isPMF = 0;
	}
		//printk("deauth:txinsn.isPMF=%d\n",txinsn.isPMF);
#endif

	pbuf = txinsn.pframe  = get_mgtbuf_from_poll(priv);

	if (pbuf == NULL)
		goto issue_deauth_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);

	if (txinsn.phdr == NULL)
		goto issue_deauth_fail;

	memset((void *)txinsn.phdr, 0, sizeof(struct  wlan_hdr));

	val = cpu_to_le16(reason);

	pbuf = set_fixed_ie(pbuf, _RSON_CODE_ , (unsigned char *)&val, &txinsn.fr_len);



	SetFrameType((txinsn.phdr),WIFI_MGT_TYPE);
	SetFrameSubType((txinsn.phdr),WIFI_DEAUTH);
#ifdef CONFIG_IEEE80211W
	if (txinsn.isPMF)
		*(unsigned char*)(txinsn.phdr+1) |= BIT(6); // enable privacy
#endif

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), da, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), bssid, MACADDRLEN);

	SNMP_MIB_ASSIGN(dot11DeauthenticateReason, reason);
	SNMP_MIB_COPY(dot11DeauthenticateStation, da, MACADDRLEN);


#if defined(WIFI_WMM)
	ret = check_dz_mgmt(priv, pstat, &txinsn);

	if (ret < 0)
		goto issue_deauth_fail;
	else if (ret==1)
		return;
	else
#endif
	if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS){
#ifdef CONFIG_RTL_WLAN_STATUS
        priv->wlan_status_flag=1;
#endif
		return;
    }

issue_deauth_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
}


void issue_disassoc(struct rtl8192cd_priv *priv, unsigned char *da, int reason)
{
	struct wifi_mib *pmib;
	unsigned char	*bssid, *pbuf;
	unsigned short  val;
	struct stat_info *pstat;
	#if defined(WIFI_WMM)
	int ret;
	#endif
	DECLARE_TXINSN(txinsn);

	// check if da is legal
	if (da[0] & 0x01) {
		DEBUG_WARN("Send Disassoc Req to bad DA %02x%02x%02x%02x%02x%02x", da[0], da[1], da[2], da[3], da[4], da[5]);
		return;
	}
	pstat = get_stainfo(priv, da);

#ifdef TLN_STATS
	stats_conn_rson_counts(priv, reason);
#endif


        if(IS_HAL_CHIP(priv))
        {
            if(pstat && (REMAP_AID(pstat) < 128))
            {
                DEBUG_WARN("%s %d issue disAssoc, set MACID 0 AID = %x \n",__FUNCTION__,__LINE__,REMAP_AID(pstat));
				if(pstat->txpdrop_flag == 1) {
					GET_HAL_INTERFACE(priv)->UpdateHalMSRRPTHandler(priv, pstat, INCREASE);
					pstat->txpdrop_flag = 0;
				}
                GET_HAL_INTERFACE(priv)->SetMACIDSleepHandler(priv, 0 , REMAP_AID(pstat));
				if(priv->pshare->paused_sta_num && pstat->txpause_flag) {
					priv->pshare->paused_sta_num--;
					pstat->txpause_flag =0;
	        	}
            }
            else
            {
                DEBUG_WARN(" MACID sleep only support 128 STA \n");
            }
        }



	txinsn.retry = priv->pmib->dot11OperationEntry.dot11ShortRetryLimit;

	pmib= GET_MIB(priv);

	bssid = pmib->dot11StationConfigEntry.dot11Bssid;

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
#ifdef CONFIG_IEEE80211W
	if(!memcmp(da,"\xff\xff\xff\xff\xff\xff",6)) {
		txinsn.isPMF = 1; //?????
	} else {
		if(pstat)
			txinsn.isPMF = pstat->isPMF;
		else
			txinsn.isPMF = 0;
	}
    PMFDEBUG("deauth:isPMF=[%d]\n",txinsn.isPMF);
#endif

    	txinsn.tx_rate  = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
	txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif
	txinsn.fixed_rate = 1;

	pbuf = txinsn.pframe  = get_mgtbuf_from_poll(priv);

	if (pbuf == NULL)
		goto issue_disassoc_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);

	if (txinsn.phdr == NULL)
		goto issue_disassoc_fail;

	memset((void *)txinsn.phdr, 0, sizeof(struct  wlan_hdr));

	val = cpu_to_le16(reason);

	pbuf = set_fixed_ie(pbuf, _RSON_CODE_, (unsigned char *)&val, &txinsn.fr_len);


	SetFrameType((txinsn.phdr), WIFI_MGT_TYPE);
	SetFrameSubType((txinsn.phdr), WIFI_DISASSOC);

#ifdef CONFIG_IEEE80211W
	if (txinsn.isPMF)
		*(unsigned char*)(txinsn.phdr+1) |= BIT(6); // enable privacy
#endif

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), da, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), bssid, MACADDRLEN);

	SNMP_MIB_ASSIGN(dot11DisassociateReason, reason);
	SNMP_MIB_COPY(dot11DisassociateStation, da, MACADDRLEN);

#if defined(CONFIG_AUTH_RESULT)
	if(priv->authRes == 0 && reason == 3)
	{
		priv->authRes = 25;
		//printk("set disassociate reason to %d\n", authRes);
	}
#endif
#if defined(WIFI_WMM)
	ret = check_dz_mgmt(priv, pstat, &txinsn);

	if (ret < 0)
		goto issue_disassoc_fail;
	else if (ret==1)
		return;
	else
#endif
	if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS){
#ifdef CONFIG_RTL_WLAN_STATUS
        priv->wlan_status_flag=1;
#endif
		return;
    }

issue_disassoc_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
}


// if pstat == NULL, indiate we are station now...
void issue_auth(struct rtl8192cd_priv *priv, struct stat_info *pstat, unsigned short status)
{
	struct wifi_mib *pmib;
	unsigned char	*bssid, *pbuf;
	unsigned short  val;
	int use_shared_key=0;


	DECLARE_TXINSN(txinsn);

	if(priv->pmib->dot1180211AuthEntry.dot11EnablePSK !=0) {
		if(OPMODE & WIFI_AP_STATE) {
			//Reset 4-WAY STATE for some phones' connection issue
			if(pstat)
			pstat->wpa_sta_info->state = PSK_STATE_IDLE;
		}
	}

	txinsn.retry = priv->pmib->dot11OperationEntry.dot11ShortRetryLimit;

	pmib= GET_MIB(priv);

	if (pstat)
		bssid = BSSID;
	else
		bssid = priv->pmib->dot11Bss.bssid;

#if defined(CLIENT_MODE) && defined(INCLUDE_WPA_PSK)
       if (priv->assoc_reject_on && !memcmp(priv->assoc_reject_mac, bssid, MACADDRLEN))
	       	       goto issue_auth_fail;
#endif

	txinsn.q_num = MANAGE_QUE_NUM;
    	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
	txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif
	txinsn.fixed_rate = 1;
	txinsn.fr_type = _PRE_ALLOCMEM_;

	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);

	if (pbuf == NULL)
		goto issue_auth_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);

	if (txinsn.phdr == NULL)
		goto issue_auth_fail;

	memset((void *)(txinsn.phdr), 0, sizeof (struct	wlan_hdr));

	// setting auth algm number
	/* In AP mode,	if auth is set to shared-key, use shared key
	 *		if auth is set to auto, use shared key if client use shared
	 *		otherwise set to open
	 * In client mode, if auth is set to shared-key or auto, and WEP is used,
	 *		use shared key algorithm
	 */
	val = 0;

	{

#ifdef WIFI_SIMPLE_CONFIG
	if (pmib->wscEntry.wsc_enable) {
		if (pstat && (status == _STATS_SUCCESSFUL_) && (pstat->auth_seq == 2) &&
			(pstat->state & WIFI_AUTH_SUCCESS) && (pstat->AuthAlgrthm == 0))
			goto skip_security_check;
		else if ((pstat == NULL) && (AUTH_SEQ == 1))
			goto skip_security_check;
	}
#endif

	if (priv->pmib->dot1180211AuthEntry.dot11AuthAlgrthm > 0) {
		if (pstat) {
			if (priv->pmib->dot1180211AuthEntry.dot11AuthAlgrthm == 1) // shared key
				val = 1;
			else { // auto mode, check client algorithm
				if (pstat && pstat->AuthAlgrthm)
					val = 1;
			}
		}
		else { // client mode, use shared key if WEP is enabled
			if (priv->pmib->dot1180211AuthEntry.dot11AuthAlgrthm == 1) { // shared-key ONLY
				if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_ ||
					priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_)
					val = 1;
			}
			else { // auto-auth
				if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_ ||
					priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_) {
					if (AUTH_SEQ == 1)
						AUTH_MODE_TOGGLE_VAL((AUTH_MODE_TOGGLE ? 0 : 1));

					if (AUTH_MODE_TOGGLE)
						val = 1;
				}
			}
		}

		if (val) {
			val = cpu_to_le16(val);
			use_shared_key = 1;
		}
	}

	if (pstat && (status != _STATS_SUCCESSFUL_))
		val = cpu_to_le16(pstat->AuthAlgrthm);
	}

#ifdef WIFI_SIMPLE_CONFIG
skip_security_check:
#endif

	pbuf = set_fixed_ie(pbuf, _AUTH_ALGM_NUM_, (unsigned char *)&val, &txinsn.fr_len);

	// setting transaction sequence number...
	if (pstat)
		val = cpu_to_le16(pstat->auth_seq);	// Mesh only
	else
		val = cpu_to_le16(AUTH_SEQ);

	pbuf = set_fixed_ie(pbuf, _AUTH_SEQ_NUM_, (unsigned char *)&val, &txinsn.fr_len);

	// setting status code...
	val = cpu_to_le16(status);
	pbuf = set_fixed_ie(pbuf, _STATUS_CODE_, (unsigned char *)&val, &txinsn.fr_len);

	// then checking to see if sending challenging text... (Mesh skip this section)
	if (pstat)
	{
		if ((pstat->auth_seq == 2) && (pstat->state & WIFI_AUTH_STATE1) && use_shared_key)
			pbuf = set_ie(pbuf, _CHLGETXT_IE_, 128, pstat->chg_txt, &txinsn.fr_len);
	}
	else
	{
		if ((AUTH_SEQ == 3) && (OPMODE & WIFI_AUTH_STATE1) && use_shared_key)
		{
			pbuf = set_ie(pbuf, _CHLGETXT_IE_, 128, CHG_TXT, &txinsn.fr_len);
			SetPrivacy(txinsn.phdr);
			DEBUG_INFO("sending a privacy pkt with auth_seq=%d\n", AUTH_SEQ);
		}
	}


	SetFrameSubType((txinsn.phdr), WIFI_AUTH);

	if (pstat)	// for AP mode
	{
		memcpy((void *)GetAddr1Ptr((txinsn.phdr)), pstat->hwaddr, MACADDRLEN);
		memcpy((void *)GetAddr2Ptr((txinsn.phdr)), bssid, MACADDRLEN);
	}
	else
	{
		memcpy((void *)GetAddr1Ptr((txinsn.phdr)), bssid, MACADDRLEN);
		memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	}

		memcpy((void *)GetAddr3Ptr((txinsn.phdr)), bssid, MACADDRLEN);

	if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS)
		return;

issue_auth_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
}

void issue_asocrsp(struct rtl8192cd_priv *priv, unsigned short status, struct stat_info *pstat, int pkt_type)
{
	unsigned short	val;
	struct wifi_mib *pmib;
	unsigned char	*bssid,*pbuf;
	DECLARE_TXINSN(txinsn);

#ifdef CONFIG_IEEE80211W
	unsigned char iebuf[5];
	int timeout;
	unsigned int temp_timeout;
#endif

	/*process all ext cap IE(107)*/
	unsigned char extended_cap_ie[8];
	memset(extended_cap_ie,0,8);

#ifdef TLN_STATS
	stats_conn_status_counts(priv, status);
#endif

	txinsn.retry = priv->pmib->dot11OperationEntry.dot11ShortRetryLimit;

	pmib= GET_MIB(priv);

	bssid = pmib->dot11StationConfigEntry.dot11Bssid;

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
    	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
	txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif
	txinsn.fixed_rate = 1;
	pbuf = txinsn.pframe  = get_mgtbuf_from_poll(priv);


        if(IS_HAL_CHIP(priv))
        {
            if(pstat && (REMAP_AID(pstat) < 128))
            {
                DEBUG_WARN("%s %d issue_asocrsp, set MACID 0 AID = %x \n",__FUNCTION__,__LINE__,REMAP_AID(pstat));
                GET_HAL_INTERFACE(priv)->SetMACIDSleepHandler(priv, 0 ,REMAP_AID(pstat));
				if(pstat->txpdrop_flag == 1) {
					GET_HAL_INTERFACE(priv)->UpdateHalMSRRPTHandler(priv, pstat, INCREASE);
					pstat->txpdrop_flag = 0;
				}
				if(priv->pshare->paused_sta_num && pstat->txpause_flag) {
					priv->pshare->paused_sta_num--;
					pstat->txpause_flag =0;
	        	}
            }
            else
            {
                DEBUG_WARN(" MACID sleep only support 128 STA \n");
            }
        }


	if (pbuf == NULL)
		goto issue_asocrsp_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);

	if (txinsn.phdr == NULL)
		goto issue_asocrsp_fail;

	memset((void *)txinsn.phdr, 0, sizeof(struct  wlan_hdr));

	val = cpu_to_le16(BIT(0));

	if (pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm)
		val |= cpu_to_le16(BIT(4));

	if (SHORTPREAMBLE)
		val |= cpu_to_le16(BIT(5));

	if (priv->pmib->dot11ErpInfo.shortSlot)
		val |= cpu_to_le16(BIT(10));

#ifdef RTK_AC_SUPPORT //for 11ac logo
	if(priv->pshare->rf_ft_var.lpwrc)
		val |= cpu_to_le16(BIT(8));	/* set spectrum mgt */
#endif

#if defined(WMM_APSD)
	if (APSD_ENABLE)
		val |= cpu_to_le16(BIT(11));
#endif

	pbuf = set_fixed_ie(pbuf, _CAPABILITY_, (unsigned char *)&val, &txinsn.fr_len);

	status = cpu_to_le16(status);
	pbuf = set_fixed_ie(pbuf, _STATUS_CODE_, (unsigned char *)&status, &txinsn.fr_len);

	val = cpu_to_le16(pstat->aid | 0xC000);
	pbuf = set_fixed_ie(pbuf, _ASOC_ID_, (unsigned char *)&val, &txinsn.fr_len);

	if (STAT_OPRATE_LEN <= 8)
		pbuf = set_ie(pbuf, _SUPPORTEDRATES_IE_, STAT_OPRATE_LEN, STAT_OPRATE, &txinsn.fr_len);
	else {
		pbuf = set_ie(pbuf, _SUPPORTEDRATES_IE_, 8, STAT_OPRATE, &txinsn.fr_len);
		pbuf = set_ie(pbuf, _EXT_SUPPORTEDRATES_IE_, STAT_OPRATE_LEN-8, STAT_OPRATE+8, &txinsn.fr_len);
	}



#ifdef CONFIG_IEEE80211W
	if(status == cpu_to_le16(_STATS_ASSOC_REJ_TEMP_) && pstat->isPMF) {
		iebuf[0] = ASSOC_COMEBACK_TIME;

		timeout = (int) RTL_JIFFIES_TO_MILISECONDS(pstat->sa_query_end - jiffies);
		if(timeout < 0)
			timeout = 0;
		if(timeout < SA_QUERY_MAX_TO)
			timeout++;
		PMFDEBUG("ASSOC comeback time, timeout=%d\n", timeout);
		temp_timeout = cpu_to_le32(timeout);
		memcpy(&iebuf[1], &temp_timeout, 4);
		pbuf = set_ie(pbuf, EID_TIMEOUT_INTERVAL, 5, iebuf, &txinsn.fr_len);
	}
#endif

#ifdef HS2_SUPPORT
	HS2_DEBUG_INFO("Driver:pmib->hs2Entry.QoSMap_ielen[0]=%d\n\n",pmib->hs2Entry.QoSMap_ielen[0]);
	if(pmib->hs2Entry.QoSMap_ielen[0]) {
		pbuf = set_ie(pbuf, _QOS_MAP_SET_IE_, pmib->hs2Entry.QoSMap_ielen[0], pmib->hs2Entry.QoSMap_ie[0], &txinsn.fr_len);
	}
#endif
#ifdef WIFI_WMM
	//Set WMM Parameter Element
	if ((QOS_ENABLE) && (pstat->QosEnabled))
		pbuf = set_ie(pbuf, _RSN_IE_1_, _WMM_Para_Element_Length_, GET_WMM_PARA_IE, &txinsn.fr_len);
#endif

#ifdef WIFI_SIMPLE_CONFIG
/*modify for WPS2DOTX SUPPORT*/
	if (pmib->wscEntry.wsc_enable && pmib->wscEntry.assoc_ielen
		&& priv->pmib->dot11StationConfigEntry.dot11AclMode!=ACL_allow) {
		memcpy(pbuf, pmib->wscEntry.assoc_ie, pmib->wscEntry.assoc_ielen);
		pbuf += pmib->wscEntry.assoc_ielen;
		txinsn.fr_len += pmib->wscEntry.assoc_ielen;
	}
#endif

	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) && (pstat->ht_cap_len > 0)) {
		if (!should_restrict_Nrate(priv, pstat)) {
			pbuf = set_ie(pbuf, _HT_CAP_, priv->ht_cap_len, (unsigned char *)&priv->ht_cap_buf, &txinsn.fr_len);
			pbuf = set_ie(pbuf, _HT_IE_, priv->ht_ie_len, (unsigned char *)&priv->ht_ie_buf, &txinsn.fr_len);
			//pbuf = construct_ht_ie_old_form(priv, pbuf, &txinsn.fr_len);
		}

#ifdef WIFI_11N_2040_COEXIST
		if (priv->pmib->dot11nConfigEntry.dot11nCoexist && priv->pshare->is_40m_bw &&
			(priv->pmib->dot11BssType.net_work_type & WIRELESS_11G)) {

			extended_cap_ie[0] |=_2040_COEXIST_SUPPORT_ ;// byte0
			construct_obss_scan_para_ie(priv);
			pbuf = set_ie(pbuf, _OBSS_SCAN_PARA_IE_, priv->obss_scan_para_len,
				(unsigned char *)&priv->obss_scan_para_buf, &txinsn.fr_len);

		}
#endif
	}

#ifdef RTK_AC_SUPPORT //for 11ac logo
	if((priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC) && (!should_restrict_Nrate(priv, pstat)))
	if((priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _WEP_40_PRIVACY_)
		&& (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _WEP_104_PRIVACY_))
	{
		//41 operting mode
		{
			extended_cap_ie[7] |= 0x40;
		}

//60, 61
		construct_vht_ie(priv, priv->pshare->working_channel);
		pbuf = set_ie(pbuf, EID_VHTCapability, priv->vht_cap_len, (unsigned char *)&priv->vht_cap_buf, &txinsn.fr_len);
		pbuf = set_ie(pbuf, EID_VHTOperation, priv->vht_oper_len, (unsigned char *)&priv->vht_oper_buf, &txinsn.fr_len);
// 66
		if(priv->pshare->rf_ft_var.opmtest&1)
			pbuf = set_ie(pbuf, EID_VHTOperatingMode, 1, (unsigned char *)&(priv->pshare->rf_ft_var.oper_mode_field), &txinsn.fr_len);

	}
#endif

#if defined(TV_MODE) || defined(A4_STA)
    val = 0;
#ifdef A4_STA
    if(priv->pshare->rf_ft_var.a4_enable == 2) {
        if(pstat->state & WIFI_A4_STA) {
            val |= BIT0;
        }
    }
#endif

#ifdef TV_MODE
    val |= BIT1;
#endif
    if(val)
        pbuf = construct_ecm_tvm_ie(priv, pbuf, &txinsn.fr_len, val);
#endif //defined(TV_MODE) || defined(A4_STA)

	// Realtek proprietary IE
	if (priv->pshare->rtk_ie_len)
		pbuf = set_ie(pbuf, _RSN_IE_1_, priv->pshare->rtk_ie_len, priv->pshare->rtk_ie_buf, &txinsn.fr_len);



	#ifdef HS2_SUPPORT	/*process ext cap IE(IE ID=107) for HS2*/
	if (priv->pmib->hs2Entry.interworking_ielen)
	{
			// byte0
		if (priv->proxy_arp){
			extended_cap_ie[1] |=_PROXY_ARP_ ;
		}

		if ((priv->pmib->hs2Entry.timezone_ielen!=0) && (priv->pmib->hs2Entry.timeadvt_ielen)){
			extended_cap_ie[3] |= _UTC_TSF_OFFSET_ ;
		}
		extended_cap_ie[3] |= _INTERWORKING_SUPPORT_ ;
		//HS2_CHECK
		if(priv->pmib->hs2Entry.QoSMap_ielen[0] || priv->pmib->hs2Entry.QoSMap_ielen[1])
			extended_cap_ie[4] |= _QOS_MAP_; // QoS MAP (Bit32)

		extended_cap_ie[5] |= _WNM_NOTIFY_; //WNM notification, Bit46

	}
	#endif
#ifdef TDLS_SUPPORT
	if(priv->pmib->dot11OperationEntry.tdls_prohibited){
		extended_cap_ie[4] |= _TDLS_PROHIBITED_;	// bit 38
	}else{
		if(priv->pmib->dot11OperationEntry.tdls_cs_prohibited){
			extended_cap_ie[4] |= _TDLS_CS_PROHIBITED_;	// bit 39
		}
	}
#endif
#ifdef CONFIG_IEEE80211V
	if(WNM_ENABLE)
		extended_cap_ie[2] |=_BSS_TRANSITION_ ; // bit 19
#endif
	/*===========ext cap IE all in here! so far (HS2, AC,TDLS )!!=================*/
	pbuf = set_ie(pbuf, _EXTENDED_CAP_IE_, 8, extended_cap_ie, &txinsn.fr_len);
	/*===========ext cap IE all in here!!!=================*/


	if ((pkt_type == WIFI_ASSOCRSP) || (pkt_type == WIFI_REASSOCRSP))
		SetFrameSubType((txinsn.phdr), pkt_type);
	else
		goto issue_asocrsp_fail;

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), pstat->hwaddr, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), bssid, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), bssid, MACADDRLEN);


if((GET_CHIP_VER(priv) != VERSION_8812E) && (GET_CHIP_VER(priv) != VERSION_8881A))
{
	unsigned int tmp_d2c = RTL_R32(0xd2c);

	if(pstat->IOTPeer== HT_IOT_PEER_INTEL)
	{
		tmp_d2c = (tmp_d2c & (~ BIT(11)));
	}
	else
	{
		if(is_intel_connected(priv))
		tmp_d2c = (tmp_d2c & (~ BIT(11)));
		else
			tmp_d2c = (tmp_d2c | BIT(11));
	}

	RTL_W32(0xd2c, tmp_d2c);

#if 1
	tmp_d2c = RTL_R32(0xd2c);

	if(tmp_d2c & BIT(11)) {
		DEBUG_INFO("BIT(11) of 0xd2c = %d, 0xd2c = 0x%x\n", 1, tmp_d2c);
	} else {
		DEBUG_INFO("BIT(11) of 0xd2c = %d, 0xd2c = 0x%x\n", 0, tmp_d2c);
	}
#endif

}

	if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS) {
//#if !defined(CONFIG_RTL865X_KLD) && !defined(CONFIG_RTL8196B_KLD)
#if 0
		if(!SWCRYPTO && !IEEE8021X_FUN &&
			(pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_ ||
			 pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_)) {
			DOT11_SET_KEY Set_Key;
			memcpy(Set_Key.MACAddr, pstat->hwaddr, 6);
			Set_Key.KeyType = DOT11_KeyType_Pairwise;
			Set_Key.EncType = pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm;

			Set_Key.KeyIndex = pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex;
			DOT11_Process_Set_Key(priv->dev, NULL, &Set_Key,
				pmib->dot11DefaultKeysTable.keytype[Set_Key.KeyIndex].skey);
		}
#endif
		return;
	}

issue_asocrsp_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
}


int fill_probe_rsp_content(struct rtl8192cd_priv *priv,
				UINT8 *phdr, UINT8 *pbuf,
				UINT8 *ssid, int ssid_len, int set_privacy, UINT8 is_11s, UINT8 is_11b_only)
{
    unsigned short	val;
    struct wifi_mib *pmib;
    unsigned char	*bssid;
    UINT8	val8;
    unsigned char	*pbssrate=NULL;
    int 	bssrate_len, fr_len=0;



	unsigned char extended_cap_ie[8];	/*process all ext cap IE ;HS2_SUPPORT*/
	memset(extended_cap_ie,0,8);


    pmib= GET_MIB(priv);

    bssid = pmib->dot11StationConfigEntry.dot11Bssid;

    pbuf += _TIMESTAMP_;
    fr_len += _TIMESTAMP_;

    val = cpu_to_le16(pmib->dot11StationConfigEntry.dot11BeaconPeriod);
    pbuf = set_fixed_ie(pbuf,  _BEACON_ITERVAL_ , (unsigned char *)&val, (unsigned int *)&fr_len);

    {
        if (OPMODE & WIFI_AP_STATE)
            val = cpu_to_le16(BIT(0)); //ESS
        else
            val = cpu_to_le16(BIT(1)); //IBSS
    }

    if (pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm && set_privacy)
        val |= cpu_to_le16(BIT(4));


    if (SHORTPREAMBLE)
        val |= cpu_to_le16(BIT(5));

    if (priv->pmib->dot11ErpInfo.shortSlot)
        val |= cpu_to_le16(BIT(10));

#ifdef DOT11H
    if(priv->pmib->dot11hTPCEntry.tpc_enable)
        val |= cpu_to_le16(BIT(8));	/* set spectrum mgt */
#endif


#if defined(WMM_APSD)
	if (APSD_ENABLE)
		val |= cpu_to_le16(BIT(11));
#endif
    pbuf = set_fixed_ie(pbuf, _CAPABILITY_, (unsigned char *)&val, (unsigned int *)&fr_len);

    pbuf = set_ie(pbuf, _SSID_IE_, ssid_len, ssid, (unsigned int *)&fr_len);

        get_bssrate_set(priv, _SUPPORTEDRATES_IE_, &pbssrate, &bssrate_len);
    pbuf = set_ie(pbuf, _SUPPORTEDRATES_IE_, bssrate_len, pbssrate, (unsigned int *)&fr_len);


    {
        val8 = pmib->dot11RFEntry.dot11channel;
    }

#if defined(RTK_5G_SUPPORT)
    if ( priv->pmib->dot11RFEntry.phyBandSelect & PHY_BAND_2G)
#endif
        pbuf = set_ie(pbuf, _DSSET_IE_, 1, &val8 , (unsigned int *)&fr_len);

    if (OPMODE & WIFI_ADHOC_STATE) {
        unsigned short val16 = 0;
        pbuf = set_ie(pbuf, _IBSS_PARA_IE_, 2, (unsigned char *)&val16, (unsigned int *)&fr_len);
    }

#if defined(DOT11D) || defined(DOT11H) || defined(DOT11K)
    if(priv->countryTableIdx) {
        pbuf = construct_country_ie(priv, pbuf, &fr_len);
    }
#endif


#if defined(DOT11H) || defined(DOT11K)
    if(priv->pmib->dot11hTPCEntry.tpc_enable || priv->pmib->dot11StationConfigEntry.dot11RadioMeasurementActivated)
    {
        pbuf = set_ie(pbuf, _PWR_CONSTRAINT_IE_, 1, &priv->pshare->rf_ft_var.lpwrc, &fr_len);
        pbuf = construct_TPC_report_ie(priv, pbuf, &fr_len);
    }
#endif


    if (OPMODE & WIFI_AP_STATE) {
        if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) {
            //ERP infomation.
            val8=0;
            if (priv->pmib->dot11ErpInfo.protection)
                val8 |= BIT(1);
            if (priv->pmib->dot11ErpInfo.nonErpStaNum)
                val8 |= BIT(0);
            pbuf = set_ie(pbuf, _ERPINFO_IE_ , 1 , &val8, (unsigned int *)&fr_len);
        }
    }

    //EXT supported rates.
    if (get_bssrate_set(priv, _EXT_SUPPORTEDRATES_IE_, &pbssrate, &bssrate_len))
        pbuf = set_ie(pbuf, _EXT_SUPPORTEDRATES_IE_ , bssrate_len , pbssrate, (unsigned int *)&fr_len);

    #if defined(HS2_SUPPORT) || defined(DOT11K)
    if(priv->pmib->dot11StationConfigEntry.cu_enable){
        pbuf = construct_BSS_load_ie(priv, pbuf, &fr_len);
    }
    #endif



	/*
		2008-12-16, For Buffalo WLI_CB_AG54L 54Mbps NIC interoperability issue.
		This NIC can not connect to our AP when our AP is set to WPA/TKIP encryption.
		This issue can be fixed after move "HT Capability Info" and "Additional HT Info" in front of "WPA" and "WMM".
	 */
    if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) && (!is_11b_only)) {
        pbuf = set_ie(pbuf, _HT_CAP_, priv->ht_cap_len, (unsigned char *)&priv->ht_cap_buf, (unsigned int *)&fr_len);
        pbuf = set_ie(pbuf, _HT_IE_, priv->ht_ie_len, (unsigned char *)&priv->ht_ie_buf, (unsigned int *)&fr_len);
    }
// probe

#ifdef RTK_AC_SUPPORT //for 11ac logo
	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC) {
		// 41
		extended_cap_ie[7] |= 0x40;
		// 60, 61
		construct_vht_ie(priv, priv->pshare->working_channel);
		pbuf = set_ie(pbuf, EID_VHTCapability, priv->vht_cap_len, (unsigned char *)&priv->vht_cap_buf, &fr_len);
		pbuf = set_ie(pbuf, EID_VHTOperation, priv->vht_oper_len, (unsigned char *)&priv->vht_oper_buf, &fr_len);

		// 62
		if(priv->pshare->rf_ft_var.lpwrc) {
			char tmp[4];
			pbuf = set_ie(pbuf, _PWR_CONSTRAINT_IE_, 1, &priv->pshare->rf_ft_var.lpwrc, &fr_len);
			tmp[1] = tmp[2] = tmp[3] = priv->pshare->rf_ft_var.lpwrc;
			tmp[0] = priv->pshare->CurrentChannelBW;	//	20, 40, 80
			pbuf = set_ie(pbuf, EID_VHTTxPwrEnvelope, tmp[0]+2, tmp, &fr_len);
		}

		//66
		if(priv->pshare->rf_ft_var.opmtest&1)
		pbuf = set_ie(pbuf, EID_VHTOperatingMode, 1, (unsigned char *)&(priv->pshare->rf_ft_var.oper_mode_field), &fr_len);

	}
#endif

#ifdef WIFI_11N_2040_COEXIST
	if ((OPMODE & WIFI_AP_STATE) &&
		(priv->pmib->dot11BssType.net_work_type & (WIRELESS_11N|WIRELESS_11G)) &&
		COEXIST_ENABLE && priv->pshare->is_40m_bw) {

		extended_cap_ie[0] |=_2040_COEXIST_SUPPORT_ ;// byte0
		construct_obss_scan_para_ie(priv);
		pbuf = set_ie(pbuf, _OBSS_SCAN_PARA_IE_, priv->obss_scan_para_len,
			(unsigned char *)&priv->obss_scan_para_buf, (unsigned int *)&fr_len);
	}
#endif

	if (pmib->dot11RsnIE.rsnielen && set_privacy
		)
	{
		memcpy(pbuf, pmib->dot11RsnIE.rsnie, pmib->dot11RsnIE.rsnielen);
		pbuf += pmib->dot11RsnIE.rsnielen;
		fr_len += pmib->dot11RsnIE.rsnielen;
	}

#ifdef WIFI_WMM
	//Set WMM Parameter Element
	if (QOS_ENABLE && (is_11b_only != 0xf))
		pbuf = set_ie(pbuf, _RSN_IE_1_, _WMM_Para_Element_Length_, GET_WMM_PARA_IE, (unsigned int *)&fr_len);
#endif


#if defined(TV_MODE) || defined(A4_STA)
    val8 = 0;
#ifdef A4_STA
    if(priv->pshare->rf_ft_var.a4_enable == 2) {
        val8 |= BIT0;
    }
#endif

#ifdef TV_MODE
    val8 |= BIT1;
#endif
    if(val8)
        pbuf = construct_ecm_tvm_ie(priv, pbuf, &fr_len, val8);
#endif //defined(TV_MODE) || defined(A4_STA)



	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) && (!is_11b_only)) {
		/*
			2008-12-16, For Buffalo WLI_CB_AG54L 54Mbps NIC interoperability issue.
			This NIC can not connect to our AP when our AP is set to WPA/TKIP encryption.
			This issue can be fixed after move "HT Capability Info" and "Additional HT Info" in front of "WPA" and "WMM".
		 */
		//pbuf = set_ie(pbuf, _HT_CAP_, priv->ht_cap_len, (unsigned char *)&priv->ht_cap_buf, &txinsn.fr_len);
		//pbuf = set_ie(pbuf, _HT_IE_, priv->ht_ie_len, (unsigned char *)&priv->ht_ie_buf, &txinsn.fr_len);
		//pbuf = construct_ht_ie_old_form(priv, pbuf, (unsigned int *)&fr_len);
	}


	// Realtek proprietary IE
	if (priv->pshare->rtk_ie_len)
		pbuf = set_ie(pbuf, _RSN_IE_1_, priv->pshare->rtk_ie_len, priv->pshare->rtk_ie_buf, (unsigned int *)&fr_len);

	// Customer proprietary IE
	if (priv->pmib->miscEntry.private_ie_len) {
		memcpy(pbuf, pmib->miscEntry.private_ie, pmib->miscEntry.private_ie_len);
		pbuf += pmib->miscEntry.private_ie_len;
		fr_len += pmib->miscEntry.private_ie_len;
	}
#ifdef USER_ADDIE
{
	int i;
	for (i=0; i<MAX_USER_IE; i++) {
		if (priv->user_ie_list[i].used) {
			memcpy(pbuf, priv->user_ie_list[i].ie, priv->user_ie_list[i].ie_len);
			pbuf += priv->user_ie_list[i].ie_len;
			fr_len += priv->user_ie_list[i].ie_len;
		}
	}
}
#endif

	if(priv->pmib->miscEntry.stage) {
		unsigned char tmp[10] = { 0x00, 0x0d, 0x02, 0x07, 0x01, 0x00, 0x00 };
		if(priv->pmib->miscEntry.stage<=5)
			tmp[6] = 1<<(8-priv->pmib->miscEntry.stage);
		pbuf = set_ie(pbuf, _RSN_IE_1_, 7, tmp, &fr_len);
	}
#ifdef HS2_SUPPORT
	//if support hs2 enable, p2p disable
    if ((OPMODE & WIFI_AP_STATE) && priv->pmib->hs2Entry.hs2_ielen)
    {
        //unsigned char p2ptmpie[]={0x50,0x6f,0x9a,0x09,0x02,0x02,0x00,0x00,0x00};
        unsigned char p2ptmpie[]={0x50,0x6f,0x9a,0x09,0x0a,0x01,0x00,0x01};
        pbuf = set_ie(pbuf, 221, sizeof(p2ptmpie), p2ptmpie, &fr_len);
    }
	//if ((OPMODE & WIFI_AP_STATE) && priv->pmib->hs2Entry.hs2_ielen)
	//{
    //    pbuf = set_ie(pbuf, _HS2_IE_, priv->pmib->hs2Entry.hs2_ielen, priv->pmib->hs2Entry.hs2_ie, &fr_len);
    //}
	if ((OPMODE & WIFI_AP_STATE) && priv->pmib->hs2Entry.hs2_ielen)
    {
        pbuf = set_ie(pbuf, _HS2_IE_, priv->pmib->hs2Entry.hs2_ielen, priv->pmib->hs2Entry.hs2_ie, &fr_len);
    }
	if ((OPMODE & WIFI_AP_STATE) && priv->pmib->hs2Entry.interworking_ielen)
	{
		pbuf = set_ie(pbuf, _INTERWORKING_IE_, priv->pmib->hs2Entry.interworking_ielen, priv->pmib->hs2Entry.interworking_ie, &fr_len);
	}
	if ((OPMODE & WIFI_AP_STATE) && priv->pmib->hs2Entry.advt_proto_ielen)
	{
		pbuf = set_ie(pbuf, _ADVT_PROTO_IE_, priv->pmib->hs2Entry.advt_proto_ielen, priv->pmib->hs2Entry.advt_proto_ie, &fr_len);
	}
	if ((OPMODE & WIFI_AP_STATE) && priv->pmib->hs2Entry.roam_ielen && priv->pmib->hs2Entry.roam_enable)
	{
		pbuf = set_ie(pbuf, _ROAM_IE_, priv->pmib->hs2Entry.roam_ielen, priv->pmib->hs2Entry.roam_ie, &fr_len);
	}
	if ((OPMODE & WIFI_AP_STATE) && priv->pmib->hs2Entry.timeadvt_ielen)
    {
        pbuf = set_ie(pbuf, _TIMEADVT_IE_, priv->pmib->hs2Entry.timeadvt_ielen, priv->pmib->hs2Entry.timeadvt_ie, &fr_len);
    }
	if ((OPMODE & WIFI_AP_STATE) && priv->pmib->hs2Entry.MBSSID_ielen)
    {
      pbuf = set_ie(pbuf, _MUL_BSSID_IE_, priv->pmib->hs2Entry.MBSSID_ielen, priv->pmib->hs2Entry.MBSSID_ie, &fr_len);
    }
    if ((OPMODE & WIFI_AP_STATE) && priv->pmib->hs2Entry.timezone_ielen)
    {
        pbuf = set_ie(pbuf, _TIMEZONE_IE_, priv->pmib->hs2Entry.timezone_ielen, priv->pmib->hs2Entry.timezone_ie, &fr_len);
    }
#endif



#ifdef WIFI_SIMPLE_CONFIG
	{
		if (!priv->pshare->rf_ft_var.NDSi_support
		&& priv->pmib->dot11StationConfigEntry.dot11AclMode!=ACL_allow){
			if (pmib->wscEntry.wsc_enable && pmib->wscEntry.probe_rsp_ielen) {
				memcpy(pbuf, pmib->wscEntry.probe_rsp_ie, pmib->wscEntry.probe_rsp_ielen);
				pbuf += pmib->wscEntry.probe_rsp_ielen;
				fr_len += pmib->wscEntry.probe_rsp_ielen;
			}
		}
	}
#endif


	#ifdef HS2_SUPPORT	/*process ext cap IE(IE ID=107) for HS2*/
	if (priv->pmib->hs2Entry.interworking_ielen)
	{
			// byte0
		if (priv->proxy_arp){
			extended_cap_ie[1] |=_PROXY_ARP_ ;
		}

		if ((priv->pmib->hs2Entry.timezone_ielen!=0) && (priv->pmib->hs2Entry.timeadvt_ielen)){
			extended_cap_ie[3] |= _UTC_TSF_OFFSET_ ;
		}
		extended_cap_ie[3] |= _INTERWORKING_SUPPORT_ ;
			//HS2_CHECK
		if(priv->pmib->hs2Entry.QoSMap_ielen[0] || priv->pmib->hs2Entry.QoSMap_ielen[1])
			extended_cap_ie[4] |= _QOS_MAP_; // QoS MAP (Bit32)

		extended_cap_ie[5] |= _WNM_NOTIFY_; //WNM notification, Bit46
	}
	#endif
#ifdef TDLS_SUPPORT
	if(priv->pmib->dot11OperationEntry.tdls_prohibited){
		extended_cap_ie[4] |= _TDLS_PROHIBITED_;	// bit 38
	}else{
		if(priv->pmib->dot11OperationEntry.tdls_cs_prohibited){
			extended_cap_ie[4] |= _TDLS_CS_PROHIBITED_;	// bit 39
		}
	}
#endif
 #ifdef CONFIG_IEEE80211V
	if(WNM_ENABLE)
		extended_cap_ie[2] |=_BSS_TRANSITION_ ; // bit 19
#endif

	/*===========fill ext cap IE here! so far (HS2, AC,TDLS )*/
	pbuf = set_ie(pbuf, _EXTENDED_CAP_IE_, 8, extended_cap_ie, &fr_len);
	/*===========ext cap IE all in here!!!=================*/


	SetFrameSubType((phdr), WIFI_PROBERSP);
	memcpy((void *)GetAddr2Ptr((phdr)), GET_MY_HWADDR, MACADDRLEN);


	{
		memcpy((void *)GetAddr3Ptr((phdr)), bssid, MACADDRLEN);
	}

	return fr_len;
}


/**
 *	@brief	issue probe response
 *
 *	- Timestamp \n - Beacon interval \n - Capability \n - SSID \n - Support rate \n - DS Parameter set \n \n
 *	+-------+-------+----+----+--------+	\n
 *	| Frame control | DA | SA |	BSS ID |	\n
 *	+-------+-------+----+----+--------+	\n
 *	\n
 *	+-----------+-----------------+------------+------+--------------+------------------+-----------+	\n
 *	| Timestamp | Beacon interval | Capability | SSID | Support rate | DS Parameter set | ERP info.	|	\n
 *	+-----------+-----------------+------------+------+--------------+------------------+-----------+	\n
 *
 *	\param	priv	device info.
 *	\param	da	address
 *	\param	sid	SSID
 *	\param	ssid_len	SSID length
 *	\param 	set_privacy	Use Robust security network
 */
//static 	;  extern for P2P_SUPPORT
void issue_probersp(struct rtl8192cd_priv *priv, unsigned char *da,
				UINT8 *ssid, int ssid_len, int set_privacy, UINT8 is_11b_only)
{
	unsigned int z = 0;
#if defined(WIFI_WMM)
	int ret;
	struct stat_info *pstat = get_stainfo(priv, da);
#endif
	DECLARE_TXINSN(txinsn);

#ifdef SDIO_AP_OFFLOAD
	if (priv->pshare->offload_function_ctrl)
		return;
#endif

//	pmib= GET_MIB(priv);
//	bssid = pmib->dot11StationConfigEntry.dot11Bssid;
	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;


    	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
	txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif

	if (priv->pmib->dot11StationConfigEntry.prsp_rate != 0xff) {
		if (priv->pmib->dot11StationConfigEntry.prsp_rate > 11)
			panic_printk("[WARN] probe rsp rate is not legacy rate!\n");
		if ((priv->pmib->dot11RFEntry.phyBandSelect & PHY_BAND_5G) &&
			(priv->pmib->dot11StationConfigEntry.prsp_rate < 4)) {
			panic_printk("[WARN] probe rsp is CCK in 5G! Correct to OFDM rate\n");
			priv->pmib->dot11StationConfigEntry.prsp_rate = 4;
		}
		txinsn.tx_rate = get_rate_from_bit_value( BIT(priv->pmib->dot11StationConfigEntry.prsp_rate));
	}

	txinsn.fixed_rate = 1;
	txinsn.pframe  = get_mgtbuf_from_poll(priv);

	if (txinsn.pframe == NULL)
		goto issue_probersp_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);

	if (txinsn.phdr == NULL)
		goto issue_probersp_fail;

	memset((void *)(txinsn.phdr), 0, sizeof (struct	wlan_hdr));

	for (z = 0; z < PSP_OUI_NUM; z++) {
		if (is_11b_only && (da[0] == PSP_OUI[z][0]) &&
			(da[1] == PSP_OUI[z][1]) &&
			(da[2] == PSP_OUI[z][2])) {
				is_11b_only = 0xf;
				printMac(da);
				break;
		}
	}



	txinsn.fr_len = fill_probe_rsp_content(priv, txinsn.phdr, txinsn.pframe, ssid, ssid_len, set_privacy, 0, is_11b_only);

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), da, MACADDRLEN);
#if defined(WIFI_WMM)
	ret = check_dz_mgmt(priv, pstat, &txinsn);

	if (ret < 0)
		goto issue_probersp_fail;
	else if (ret==1)
		return;
	else
#endif
	if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS)
		return;

issue_probersp_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
}


/**
 *	@brief	STA issue prob request
 *
 *	+---------------+-----+------+-----------------+--------------------------+	\n
 *	| Frame Control | ... | SSID | Supported Rates | Extended Supported Rates |	\n
 *	+---------------+-----+------+-----------------+--------------------------+	\n
 *	@param	priv	device
 *	@param	ssid	ssid name
 *	@param	ssid_len	ssid length
 */
static void issue_probereq(struct rtl8192cd_priv *priv, unsigned char *ssid, int ssid_len, unsigned char *da)
{

    struct wifi_mib *pmib;
    unsigned char	*hwaddr, *pbuf;
    unsigned char	*pbssrate=NULL;
    int		bssrate_len;
    DECLARE_TXINSN(txinsn);

#ifdef MP_TEST
    if (priv->pshare->rf_ft_var.mp_specific)
        return;
#endif
#ifdef CONFIG_IEEE80211V_CLI
	unsigned char extended_cap_ie[8];
	memset(extended_cap_ie, 0, 8);
#endif

#if	0	//def DFS
	if(under_apmode_repeater(priv))
	{
		unsigned int channel = priv->pmib->dot11RFEntry.dot11channel;
		unsigned char issue_ok = 0;
		unsigned int tmp_opmode =0;
		if(is_DFS_channel(channel))
		{
			if((OPMODE & WIFI_ASOC_STATE) && (OPMODE & WIFI_STATION_STATE))
			{
				issue_ok = 1;
			}

			if(IS_ROOT_INTERFACE(priv))
			{
				tmp_opmode = GET_VXD_PRIV(priv)->pmib->dot11OperationEntry.opmode;

				if((tmp_opmode & WIFI_ASOC_STATE) && (tmp_opmode & WIFI_STATION_STATE))
				{
					issue_ok = 1;
				}
			}

			//printk("DFS Channel=%d, issue_probeReq=%d\n", channel, issue_ok);

			if(issue_ok == 0)
				return;
		}
	}
#endif

    txinsn.retry = priv->pmib->dot11OperationEntry.dot11ShortRetryLimit;

    pmib = GET_MIB(priv);

    hwaddr = pmib->dot11OperationEntry.hwaddr;
    txinsn.q_num = MANAGE_QUE_NUM;
    txinsn.fr_type = _PRE_ALLOCMEM_;

        txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
    txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif
    txinsn.fixed_rate = 1;
    pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);

    if (pbuf == NULL)
        goto issue_probereq_fail;

    txinsn.phdr = get_wlanhdr_from_poll(priv);

    if (txinsn.phdr == NULL)
        goto issue_probereq_fail;

    memset((void *)(txinsn.phdr), 0, sizeof (struct	wlan_hdr));

#ifdef HS2_SUPPORT
#ifdef HS2_CLIENT_TEST
	if ((priv->pshare->rf_ft_var.swq_dbg == 30) || (priv->pshare->rf_ft_var.swq_dbg == 31))
    {
		printk("Probe Request to SSID [Hotspot 2.0]\n");
        strcpy(ssid, "Hotspot 2.0");
        ssid[11] = '\0';
        ssid_len = strlen(ssid);
        pbuf = set_ie(pbuf, _SSID_IE_, ssid_len, ssid, &txinsn.fr_len);
    }
    else if ((priv->pshare->rf_ft_var.swq_dbg == 32) || (priv->pshare->rf_ft_var.swq_dbg == 33) || (priv->pshare->rf_ft_var.swq_dbg == 34) || (priv->pshare->rf_ft_var.swq_dbg == 35) || (priv->pshare->rf_ft_var.swq_dbg == 36) || (priv->pshare->rf_ft_var.swq_dbg == 37) || (priv->pshare->rf_ft_var.swq_dbg == 38) || (priv->pshare->rf_ft_var.swq_dbg == 39))
    {
        pbuf = set_ie(pbuf, _SSID_IE_, 0, ssid, &txinsn.fr_len);
    }
    else
#endif
#endif
	pbuf = set_ie(pbuf, _SSID_IE_, ssid_len, ssid, &txinsn.fr_len);
    /*fill supported rates*/

    {
        get_bssrate_set(priv, _SUPPORTEDRATES_IE_, &pbssrate, &bssrate_len);
    }


    pbuf = set_ie(pbuf, _SUPPORTEDRATES_IE_ , bssrate_len , pbssrate, &txinsn.fr_len);

    if (get_bssrate_set(priv, _EXT_SUPPORTEDRATES_IE_, &pbssrate, &bssrate_len))
        pbuf = set_ie(pbuf, _EXT_SUPPORTEDRATES_IE_ , bssrate_len , pbssrate, &txinsn.fr_len);




#ifdef RTK_AC_SUPPORT	// WDS-VHT support
    if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC) {
        //WDEBUG("construct_vht_ie\n");
        construct_vht_ie(priv, priv->pshare->working_channel);
        pbuf = set_ie(pbuf, EID_VHTCapability, priv->vht_cap_len, (unsigned char *)&priv->vht_cap_buf, &txinsn.fr_len);
        pbuf = set_ie(pbuf, EID_VHTOperation, priv->vht_oper_len, (unsigned char *)&priv->vht_oper_buf, &txinsn.fr_len);
    }
#endif

#ifdef WIFI_SIMPLE_CONFIG
	{
		if (pmib->wscEntry.wsc_enable && pmib->wscEntry.probe_req_ielen) {
			memcpy(pbuf, pmib->wscEntry.probe_req_ie, pmib->wscEntry.probe_req_ielen);
			pbuf += pmib->wscEntry.probe_req_ielen;
			txinsn.fr_len += pmib->wscEntry.probe_req_ielen;
		}
	}
#endif

/*cfg p2p cfg p2p*/
/*cfg p2p cfg p2p*/

#ifdef CONFIG_IEEE80211V_CLI
		if(WNM_ENABLE) {
			extended_cap_ie[2] |= _WNM_BSS_TRANS_SUPPORT_ ;
			pbuf = set_ie(pbuf, _EXTENDED_CAP_IE_, 8, extended_cap_ie, &txinsn.fr_len);
		 }
#endif

#ifdef HS2_SUPPORT
#ifdef HS2_CLIENT_TEST
	if (priv->pshare->rf_ft_var.swq_dbg == 30)
    {
		// HS2.0 AP does not transmit a probe response frame
		// HESSID is wrong.

        unsigned int temp_buf32, buf32 = _INTERWORKING_SUPPORT_BY_DW_, frlen=0;
        unsigned char tmp[]={0x02,0x50,0x6F,0x9A,0x00,0x00,0x01};

        temp_buf32 = cpu_to_le32(buf32);
        pbuf = set_ie(pbuf, _EXTENDED_CAP_IE_, 4, &temp_buf32, &frlen);
        txinsn.fr_len += frlen;

        frlen = 0;
        pbuf = set_ie(pbuf, _INTERWORKING_IE_, 7, tmp, &frlen);
        txinsn.fr_len += frlen;
    }
    else if (priv->pshare->rf_ft_var.swq_dbg == 31)
    {
		// HS2.0 AP does not transmit a probe response frame
		// HESSID is wrong.
        unsigned int temp_buf32, buf32 = _INTERWORKING_SUPPORT_BY_DW_, frlen=0;
        unsigned char tmp[]={0x02,0x00,0x00,0x50,0x6F,0x9A,0x00,0x00,0x01};

        temp_buf32 = cpu_to_le32(buf32);
        pbuf = set_ie(pbuf, _EXTENDED_CAP_IE_, 4, &temp_buf32, &frlen);
        txinsn.fr_len += frlen;

        frlen = 0;
        pbuf = set_ie(pbuf, _INTERWORKING_IE_, 9, tmp, &frlen);
        txinsn.fr_len += frlen;
    }
    else if (priv->pshare->rf_ft_var.swq_dbg == 32)
    {
		// APUT transmits Probe Response Message
        unsigned int temp_buf32, buf32 = _INTERWORKING_SUPPORT_BY_DW_, frlen=0;
        unsigned char tmp[]={0x02,0x00,0x33,0x44,0x55,0x66,0x77}; // HESSID = redir_mac, please refer to next line

		memcpy(&tmp[1], priv->pmib->hs2Entry.redir_mac, 6);

        temp_buf32 = cpu_to_le32(buf32);
        pbuf = set_ie(pbuf, _EXTENDED_CAP_IE_, 4, &temp_buf32, &frlen);
        txinsn.fr_len += frlen;

		frlen = 0;
        pbuf = set_ie(pbuf, _INTERWORKING_IE_, 7, tmp, &frlen);
        txinsn.fr_len += frlen;
    }
    else if (priv->pshare->rf_ft_var.swq_dbg == 33)
    {
		// APUT transmits Probe Response Message
        unsigned int temp_buf32, buf32 = _INTERWORKING_SUPPORT_BY_DW_, frlen=0;
        unsigned char tmp[]={0x02,0x00,0x00,0x00,0x33,0x44,0x55,0x66,0x77}; // HESSID = redir_mac, please refer to next line

		memcpy(&tmp[3], priv->pmib->hs2Entry.redir_mac, 6);

        temp_buf32 = cpu_to_le32(buf32);
        pbuf = set_ie(pbuf, _EXTENDED_CAP_IE_, 4, &temp_buf32, &frlen);
        txinsn.fr_len += frlen;

        frlen = 0;
        pbuf = set_ie(pbuf, _INTERWORKING_IE_, 9, tmp, &frlen);
        txinsn.fr_len += frlen;
    }
    else if (priv->pshare->rf_ft_var.swq_dbg == 34)
    {
        unsigned int temp_buf32, buf32 = _INTERWORKING_SUPPORT_BY_DW_, frlen=0;
        unsigned char tmp[]={0x03}; // HESSID is not present

        temp_buf32 = cpu_to_le32(buf32);
        pbuf = set_ie(pbuf, _EXTENDED_CAP_IE_, 4, &temp_buf32, &frlen);
        txinsn.fr_len += frlen;

        frlen = 0;
        pbuf = set_ie(pbuf, _INTERWORKING_IE_, 1, tmp, &frlen);
        txinsn.fr_len += frlen;
    }
    else if (priv->pshare->rf_ft_var.swq_dbg == 35)
    {
        unsigned int temp_buf32, buf32 = _INTERWORKING_SUPPORT_BY_DW_, frlen=0;
        unsigned char tmp[]={0x03,0x00,0x00}; // HESSID is not present

		temp_buf32 = cpu_to_le32(buf32);
        pbuf = set_ie(pbuf, _EXTENDED_CAP_IE_, 4, &temp_buf32, &frlen);
        txinsn.fr_len += frlen;

        frlen = 0;
        pbuf = set_ie(pbuf, _INTERWORKING_IE_, 3, tmp, &frlen);
        txinsn.fr_len += frlen;
    }
    else if (priv->pshare->rf_ft_var.swq_dbg == 36)
    {
        unsigned int temp_buf32, buf32 = _INTERWORKING_SUPPORT_BY_DW_, frlen=0;
        unsigned char tmp[]={0x02,0xff,0xff,0xff,0xff,0xff,0xff};

        temp_buf32 = cpu_to_le32(buf32);
        pbuf = set_ie(pbuf, _EXTENDED_CAP_IE_, 4, &temp_buf32, &frlen);
        txinsn.fr_len += frlen;

        frlen = 0;
        pbuf = set_ie(pbuf, _INTERWORKING_IE_, 7, tmp, &frlen);
        txinsn.fr_len += frlen;
    }
    else if (priv->pshare->rf_ft_var.swq_dbg == 37)
    {
        unsigned int temp_buf32, buf32 = _INTERWORKING_SUPPORT_BY_DW_, frlen=0;
        unsigned char tmp[]={0x02,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff};

        temp_buf32 = cpu_to_le32(buf32);
        pbuf = set_ie(pbuf, _EXTENDED_CAP_IE_, 4, &temp_buf32, &frlen);
        txinsn.fr_len += frlen;

        frlen = 0;
        pbuf = set_ie(pbuf, _INTERWORKING_IE_, 9, tmp, &frlen);
        txinsn.fr_len += frlen;
    }
	else if (priv->pshare->rf_ft_var.swq_dbg == 38)
    {
        unsigned int temp_buf32, buf32 = _INTERWORKING_SUPPORT_BY_DW_, frlen=0;
        unsigned char tmp[]={0x0f,0xff,0xff,0xff,0xff,0xff,0xff};

        temp_buf32 = cpu_to_le32(buf32);
        pbuf = set_ie(pbuf, _EXTENDED_CAP_IE_, 4, &temp_buf32, &frlen);
        txinsn.fr_len += frlen;

        frlen = 0;
        pbuf = set_ie(pbuf, _INTERWORKING_IE_, 7, tmp, &frlen);
        txinsn.fr_len += frlen;
    }
    else if (priv->pshare->rf_ft_var.swq_dbg == 39)
    {
        unsigned int temp_buf32, buf32 = _INTERWORKING_SUPPORT_BY_DW_, frlen=0;
        unsigned char tmp[]={0x0f,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff};

        temp_buf32 = cpu_to_le32(buf32);
        pbuf = set_ie(pbuf, _EXTENDED_CAP_IE_, 4, &temp_buf32, &frlen);
        txinsn.fr_len += frlen;

        frlen = 0;
        pbuf = set_ie(pbuf, _INTERWORKING_IE_, 9, tmp, &frlen);
        txinsn.fr_len += frlen;
    }
#endif
#endif


#ifdef A4_STA
    if(priv->pshare->rf_ft_var.a4_enable == 2) {
        pbuf = construct_ecm_tvm_ie(priv, pbuf, &txinsn.fr_len, BIT0);
    }
#endif


	SetFrameSubType(txinsn.phdr, WIFI_PROBEREQ);

	if (da)
		memcpy((void *)GetAddr1Ptr((txinsn.phdr)), da, MACADDRLEN); // unicast
	else
		memset((void *)GetAddr1Ptr((txinsn.phdr)), 0xff, MACADDRLEN); // broadcast
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), hwaddr, MACADDRLEN);
	//nctu note
	// spec define ProbeREQ Address 3 is BSSID or wildcard) (Refer: Draft 1.06, Page 12, 7.2.3, Line 27~28)
	memset((void *)GetAddr3Ptr((txinsn.phdr)), 0xff, MACADDRLEN);

	if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS)
        return;

issue_probereq_fail:
    NDEBUG("tx probe_req fail!!\n");
    if(RTL_R8(TXPAUSE)){
        NDEBUG("!!!tx_pause_val[%X]\n",RTL_R8(TXPAUSE));
    }

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
}


#if defined(CLIENT_MODE) && defined(WIFI_11N_2040_COEXIST)
static void issue_coexist_mgt(struct rtl8192cd_priv *priv)
{
	unsigned char	*pbuf;
	unsigned int len = 0, ch_len=0, i=0;
	DECLARE_TXINSN(txinsn);

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
    	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
	txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif
	txinsn.fixed_rate = 1;

	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);
	if (pbuf == NULL)
		goto issue_coexist_mgt_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);
	if (txinsn.phdr == NULL)
		goto issue_coexist_mgt_fail;

	memset((void *)(txinsn.phdr), 0, sizeof(struct wlan_hdr));

	pbuf[0] = _PUBLIC_CATEGORY_ID_;
	pbuf[1] = _2040_COEXIST_ACTION_ID_;
	len+=2;

	pbuf[2] = _2040_BSS_COEXIST_IE_;
	pbuf[3] = 1;
	len+=2;

	if (priv->intolerant_timeout)
		pbuf[4] = _20M_BSS_WIDTH_REQ_;
	else
		pbuf[4] = 0;
	len+=1;

	if (priv->bg_ap_timeout) {
		pbuf[5] = _2040_Intolerant_ChRpt_IE_;
		pbuf[7] = 0; /*set category*/
		for (i=0; i<14; i++) {
			if (priv->bg_ap_timeout_ch[i]) {
				pbuf[8+ch_len] = i+1;/*set channels*/
				ch_len++;
			}
		}
		pbuf[6] = ch_len+1;
		len += (pbuf[6]+2);
	}

	txinsn.fr_len = len;
	SetFrameSubType((txinsn.phdr), WIFI_WMM_ACTION);
	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);

	DEBUG_INFO("Coexist-mgt sent to AP\n");

	if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS)
		return;

issue_coexist_mgt_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
	return;
}
#endif


#ifdef WIFI_WMM

#if 0
void issue_DELBA(struct rtl8192cd_priv *priv, struct stat_info *pstat, unsigned char TID, unsigned char initiator){
	unsigned char	*pbuf;
	unsigned short	delba_para = 0;
	DECLARE_TXINSN(txinsn);

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
	txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif
	txinsn.fixed_rate = 1;

	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);
	if (pbuf == NULL)
		goto issue_DELBA_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);
	if (txinsn.phdr == NULL)
		goto issue_DELBA_fail;

	memset((void *)(txinsn.phdr), 0, sizeof(struct wlan_hdr));

	pbuf[0] = _BLOCK_ACK_CATEGORY_ID_;
	pbuf[1] = _DELBA_ACTION_ID_;
	delba_para = initiator << 11 | TID << 12;	// assign buffer size | assign TID | set Immediate Block Ack
	pbuf[2] = initiator << 3 | TID << 4;
	pbuf[3] = 0;
	pbuf[4] = 38;//reason code
	pbuf[5] = 0;

	/* set the immediate next seq number of the "TID", as Block Ack Starting Seq*/

	txinsn.fr_len = _DELBA_Frame_Length;

	SetFrameSubType((txinsn.phdr), WIFI_WMM_ACTION);
	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), pstat->hwaddr, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);

	DEBUG_INFO("issue_DELBAreq sent to AID %d, token %d TID %d size %d seq %d\n",
		pstat->aid, pstat->dialog_token, TID, max_size, pstat->AC_seq[TID]);

	if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS)
		return;

issue_DELBA_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
	return;
}
#endif

void issue_ADDBAreq(struct rtl8192cd_priv *priv, struct stat_info *pstat, unsigned char TID)
{
	unsigned char	*pbuf;
	unsigned short	ba_para = 0;
#if defined(WIFI_WMM)
	int ret=0;
#endif
	int max_size;
	DECLARE_TXINSN(txinsn);

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;

#ifdef CONFIG_IEEE80211W
	if(pstat)
		txinsn.isPMF = pstat->isPMF;
	else
		txinsn.isPMF = 0;
#endif

    	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
	txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif
	txinsn.fixed_rate = 1;

	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);
	if (pbuf == NULL)
		goto issue_ADDBAreq_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);
	if (txinsn.phdr == NULL)
		goto issue_ADDBAreq_fail;

	memset((void *)(txinsn.phdr), 0, sizeof(struct wlan_hdr));

	if (!(++pstat->dialog_token))	// dialog token set to a non-zero value
		pstat->dialog_token++;

	pbuf[0] = _BLOCK_ACK_CATEGORY_ID_;
	pbuf[1] = _ADDBA_Req_ACTION_ID_;
	pbuf[2] = pstat->dialog_token;

	if (should_restrict_Nrate(priv, pstat))
		max_size = 1;
	else {
		max_size = _ADDBA_Maximum_Buffer_Size_;
	}

	ba_para = (max_size<<6) | (TID<<2) | BIT(1);	// assign buffer size | assign TID | set Immediate Block Ack

#if defined(SUPPORT_RX_AMSDU_AMPDU)
	if(AMSDU_ENABLE >= 2) {
		if (!((GET_CHIP_VER(priv)== VERSION_8814A) && priv->pmib->dot11nConfigEntry.dot11nUse40M == 0))
			ba_para |= BIT(0);			// AMSDU
	}
#endif

	pbuf[3] = ba_para & 0x00ff;
	pbuf[4] = (ba_para & 0xff00) >> 8;

	// set Block Ack Timeout value to zero, to disable the timeout
	pbuf[5] = 0;
	pbuf[6] = 0;

	// set the immediate next seq number of the "TID", as Block Ack Starting Seq
	pbuf[7] = ((pstat->AC_seq[TID] & 0xfff) << 4) & 0x00ff;
	pbuf[8] = (((pstat->AC_seq[TID] & 0xfff) << 4) & 0xff00) >> 8;

	txinsn.fr_len = _ADDBA_Req_Frame_Length_;
	SetFrameSubType((txinsn.phdr), WIFI_WMM_ACTION);
#ifdef CONFIG_IEEE80211W
	if (txinsn.isPMF)
		*(unsigned char*)(txinsn.phdr+1) |= BIT(6); // enable privacy
#endif

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), pstat->hwaddr, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);

	DEBUG_INFO("ADDBA-req sent to AID %d, token %d TID %d size %d seq %d\n",
		pstat->aid, pstat->dialog_token, TID, max_size, pstat->AC_seq[TID]);
	/*
	panic_printk("ADDBA-req sent to AID %d, token %d TID %d size %d seq %d\n",
		pstat->aid, pstat->dialog_token, TID, max_size, pstat->AC_seq[TID]);
	*/
#if defined(WIFI_WMM)
	ret = check_dz_mgmt(priv, pstat, &txinsn);
	if (ret < 0)
		goto issue_ADDBAreq_fail;
	else if (ret==1)
		return;
	else
#endif
	if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS) {
		//pstat->ADDBA_ready++;
		return;
	}

issue_ADDBAreq_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
	return;
}

#ifdef HS2_SUPPORT
int issue_QoS_MAP_Configure(struct rtl8192cd_priv *priv, unsigned char *da, unsigned char indexQoSMAP) // QoS Action Frame (Robust)
{
	unsigned char rnd;
	unsigned char	*pbuf;
	struct stat_info *pstat;
	unsigned char zeromac[6]={0x00,0x00,0x00,0x00,0x00,0x00};

	struct wifi_mib *pmib;
	pmib = GET_MIB(priv);

#if defined(WIFI_WMM)
	int ret;
#endif

	DECLARE_TXINSN(txinsn);

	if(priv->pmib->hs2Entry.QoSMap_ielen[indexQoSMAP] == 0)
		goto issue_QoS_MAP_CONFIGURE_fail;

	HS2_DEBUG_INFO("iface: %s, da:%02x%02x%02x%02x%02x%02x\n",priv->dev->name,da[0],da[1],da[2],da[3],da[4],da[5]);

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;

#ifdef CONFIG_IEEE80211W
		struct stat_info *psta;
		if(!memcmp(da,"\xff\xff\xff\xff\xff\xff",6)) {
			txinsn.isPMF = 1; //?????
		} else {
			psta = get_stainfo(priv,da);
			if(psta)
				txinsn.isPMF = psta->isPMF;
			else
				txinsn.isPMF = 0;
		}
		//printk("deauth:txinsn.isPMF=%d\n",txinsn.isPMF);
#endif
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
	txinsn.lowest_tx_rate = txinsn.tx_rate;
	txinsn.fixed_rate = 1;
	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);
	if (pbuf == NULL)
		goto issue_QoS_MAP_CONFIGURE_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);
	if (txinsn.phdr == NULL)
		goto issue_QoS_MAP_CONFIGURE_fail;

	memset((void *)(txinsn.phdr), 0, sizeof(struct wlan_hdr));

	pbuf[0] = _QOS_CATEGORY_ID_;
	pbuf[1] = _QOS_MAP_CONFIGURE_ID_;
	pbuf[2] = _QOS_MAP_SET_IE_;
	pbuf[3] = priv->pmib->hs2Entry.QoSMap_ielen[indexQoSMAP];

	if(priv->pmib->hs2Entry.QoSMap_ielen[indexQoSMAP] != 0) {
		memcpy(pbuf+4,priv->pmib->hs2Entry.QoSMap_ie[indexQoSMAP],priv->pmib->hs2Entry.QoSMap_ielen[indexQoSMAP]);
		txinsn.fr_len += 4 + priv->pmib->hs2Entry.QoSMap_ielen[indexQoSMAP];
	}


	SetFrameSubType((txinsn.phdr), WIFI_WMM_ACTION);

#ifdef CONFIG_IEEE80211W
	if (txinsn.isPMF)
		*(unsigned char*)(txinsn.phdr+1) |= BIT(6); // enable privacy
#endif

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), da, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
	pstat = get_stainfo(priv, da);

	if (pstat == NULL)
		goto issue_QoS_MAP_CONFIGURE_fail;

	txinsn.pstat = pstat;

#if defined(WIFI_WMM)
	ret = check_dz_mgmt(priv, pstat, &txinsn);
	if (ret < 0)
		goto issue_QoS_MAP_CONFIGURE_fail;
	else if (ret==1) {
		HS2_DEBUG_INFO("issue_QoS_MAP_Configure success\n");
		return SUCCESS;
	}
	else
#endif
	{
		if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS) {
			HS2_DEBUG_INFO("issue_QoS_MAP_Configure success\n");
			return SUCCESS;
		}
	}

issue_QoS_MAP_CONFIGURE_fail:
	HS2_DEBUG_ERR("issue_QoS_MAP_Configure failed\n");

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);

	return FAIL;
}


int issue_WNM_Notify(struct rtl8192cd_priv *priv)
{
	unsigned char rnd;
	unsigned char	*pbuf;
	struct stat_info *pstat;
	unsigned char zeromac[6]={0x00,0x00,0x00,0x00,0x00,0x00};

	struct wifi_mib *pmib;
	pmib = GET_MIB(priv);

#if defined(WIFI_WMM)
	int ret;
#endif


	DECLARE_TXINSN(txinsn);

	HS2_DEBUG_INFO("iface: %s, sta_mac:%02x%02x%02x%02x%02x%02x\n",priv->dev->name,priv->pmib->hs2Entry.sta_mac[0],priv->pmib->hs2Entry.sta_mac[1],priv->pmib->hs2Entry.sta_mac[2],priv->pmib->hs2Entry.sta_mac[3],priv->pmib->hs2Entry.sta_mac[4],priv->pmib->hs2Entry.sta_mac[5]);
	if(!memcmp(priv->pmib->hs2Entry.sta_mac,zeromac,6))  {
		HS2_DEBUG_ERR("sta_mac is zeromac\n");
		goto issue_WNM_NOTIFY_fail;
	}

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;

#ifdef CONFIG_IEEE80211W
		struct stat_info *psta;
		if(!memcmp(priv->pmib->hs2Entry.sta_mac,"\xff\xff\xff\xff\xff\xff",6)) {
			HS2_DEBUG_ERR("WNM_NOTIFY_FAIL: Cannot be multicast da.\n");
			goto issue_WNM_NOTIFY_fail;
		} else {
			psta = get_stainfo(priv,priv->pmib->hs2Entry.sta_mac);
			if(psta)
				txinsn.isPMF = 1;
			else
				goto issue_WNM_NOTIFY_fail;
		}
		//printk("deauth:txinsn.isPMF=%d\n",txinsn.isPMF);
#endif

    if(txinsn.isPMF == 0) {
		HS2_DEBUG_ERR("\n\nWNM_NOTIFY_FAIL: PMF is not Enabled, psta=%x\n\n");
		goto issue_WNM_NOTIFY_fail;
    }
 	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
	txinsn.lowest_tx_rate = txinsn.tx_rate;
	txinsn.fixed_rate = 1;
	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);
	if (pbuf == NULL)
		goto issue_WNM_NOTIFY_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);
	if (txinsn.phdr == NULL)
		goto issue_WNM_NOTIFY_fail;

	memset((void *)(txinsn.phdr), 0, sizeof(struct wlan_hdr));

	pbuf[0] = _WNM_CATEGORY_ID_;
	pbuf[1] = _WNM_NOTIFICATION_ID_;
	pbuf[2] = 4;			//token
	pbuf[3] = 1;				//type
	pbuf[4] = 221;	//Subelement ID
	pbuf[5] = 5 + strlen(priv->pmib->hs2Entry.remedSvrURL);
	pbuf[6] = 0x50;
	pbuf[7] = 0x6F;
	pbuf[8] = 0x9A;
	pbuf[9] = 0x00;	// Subelement Type
	pbuf[10] = strlen(priv->pmib->hs2Entry.remedSvrURL);
	strcpy(pbuf+11,priv->pmib->hs2Entry.remedSvrURL);
	#if 0
	pbuf[11+strlen(priv->pmib->hs2Entry.remedSvrURL)] = priv->pmib->hs2Entry.serverMethod;
	#endif

	SDEBUG("issue_WNM_Notify,remedSvrURL=%s\n",priv->pmib->hs2Entry.remedSvrURL);
#if 0
	SDEBUG("issue_WNM_Notify,serverMethod=%d\n",priv->pmib->hs2Entry.serverMethod);
	txinsn.fr_len += 11 + strlen(priv->pmib->hs2Entry.remedSvrURL)+1;
#else
	txinsn.fr_len += 11 + strlen(priv->pmib->hs2Entry.remedSvrURL);
#endif



	SetFrameSubType((txinsn.phdr), WIFI_WMM_ACTION);
#ifdef CONFIG_IEEE80211W
	if (txinsn.isPMF)
		*(unsigned char*)(txinsn.phdr+1) |= BIT(6); // enable privacy
#endif

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), priv->pmib->hs2Entry.sta_mac, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
	pstat = get_stainfo(priv, priv->pmib->hs2Entry.sta_mac);

	if (pstat == NULL)
		goto issue_WNM_NOTIFY_fail;

	txinsn.pstat = pstat;

#if defined(WIFI_WMM)
	ret = check_dz_mgmt(priv, pstat, &txinsn);
	if (ret < 0)
		goto issue_WNM_NOTIFY_fail;
	else if (ret==1) {
		HS2_DEBUG_INFO("issue_WNM_Notify success\n");
		return SUCCESS;
	}
	else
#endif
	{
		if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS) {
			HS2_DEBUG_INFO("issue_WNM_Notify success\n");
			return SUCCESS;
		}
	}

issue_WNM_NOTIFY_fail:
	PMFDEBUG("issue_WNM_NOTIFY failed\n");

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);

	return FAIL;
}

int issue_WNM_Deauth_Req(struct rtl8192cd_priv *priv, unsigned char *da, unsigned char reason, unsigned short ReAuthDelay, unsigned char *URL)
{
	unsigned char rnd;
	unsigned char	*pbuf;
	struct stat_info *pstat;
	unsigned char zeromac[6]={0x00,0x00,0x00,0x00,0x00,0x00};

	struct wifi_mib *pmib;
	pmib = GET_MIB(priv);

#if defined(WIFI_WMM)
	int ret;
#endif


	DECLARE_TXINSN(txinsn);

	HS2_DEBUG_INFO("iface: %s, da:%02x%02x%02x%02x%02x%02x\n",priv->dev->name,da[0],da[1],da[2],da[3],da[4],da[5]);
	if(!memcmp(da,zeromac,6))  {
		HS2_DEBUG_ERR("da is zeromac\n");
		goto issue_WNM_DEAUTH_fail;
	}

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;

#ifdef CONFIG_IEEE80211W
		struct stat_info *psta;
		if(!memcmp(da,"\xff\xff\xff\xff\xff\xff",6)) {
			txinsn.isPMF = 1; //?????
		} else {
			psta = get_stainfo(priv,da);
			if(psta)
				txinsn.isPMF = psta->isPMF;
			else
				txinsn.isPMF = 0;
		}
		//printk("deauth:txinsn.isPMF=%d\n",txinsn.isPMF);
#endif
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
	txinsn.lowest_tx_rate = txinsn.tx_rate;
	txinsn.fixed_rate = 1;

	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);
	if (pbuf == NULL) {
		HS2_DEBUG_ERR("issue_WNM_DEAUTH_fail\n");
		goto issue_WNM_DEAUTH_fail;
	}


	txinsn.phdr = get_wlanhdr_from_poll(priv);
	if (txinsn.phdr == NULL) {
		HS2_DEBUG_ERR("issue_WNM_DEAUTH_fail\n");
		goto issue_WNM_DEAUTH_fail;
	}

	memset((void *)(txinsn.phdr), 0, sizeof(struct wlan_hdr));

	pbuf[0] = _WNM_CATEGORY_ID_;
	pbuf[1] = _WNM_NOTIFICATION_ID_;
	pbuf[2] = 4;			//token
	pbuf[3] = 1;			//type
	pbuf[4] = 221;			//Subelement ID
	pbuf[5] = 8 + strlen(URL);
	pbuf[6] = 0x50;
	pbuf[7] = 0x6F;
	pbuf[8] = 0x9A;
	pbuf[9] = 0x01;	// Subelement Type
	pbuf[10] = reason;
	pbuf[11] = ReAuthDelay & 0x00ff;
	pbuf[12] = (ReAuthDelay & 0xff00) >> 8;
	pbuf[13] = strlen(URL);	 // Reason URL Length

	strcpy(pbuf+14,URL); // Reason URL
	HS2_DEBUG_INFO("URL=%s\n",URL);
	txinsn.fr_len += 14+strlen(URL);


	SetFrameSubType((txinsn.phdr), WIFI_WMM_ACTION);
#ifdef CONFIG_IEEE80211W
	if (txinsn.isPMF)
		*(unsigned char*)(txinsn.phdr+1) |= BIT(6); // enable privacy
#endif
	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), da, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);

	pstat = get_stainfo(priv, da);
	if (pstat == NULL)
		goto issue_WNM_DEAUTH_fail;

	txinsn.pstat = pstat;

#if defined(WIFI_WMM)
	ret = check_dz_mgmt(priv, pstat, &txinsn);
	if (ret < 0)
		goto issue_WNM_DEAUTH_fail;
	else if (ret==1) {
		return SUCCESS;
	}
	else
#endif
	{
		if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS)
			return SUCCESS;
	}

issue_WNM_DEAUTH_fail:
	HS2_DEBUG_INFO("issue_WNM_DEAUTH failed\n");

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);

	return FAIL;
}
#ifdef HS2_CLIENT_TEST

//issue_GASreq for client test used
int issue_GASreq(struct rtl8192cd_priv *priv, DOT11_HS2_GAS_REQ *gas_req, unsigned short qid)
{
	unsigned char	*pbuf;
	struct stat_info *pstat;
#if defined(WIFI_WMM)
	int ret;
#endif
	DECLARE_TXINSN(txinsn);

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
	txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif
	txinsn.fixed_rate = 1;

	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);

	txinsn.phdr = get_wlanhdr_from_poll(priv);

	memset((void *)(txinsn.phdr), 0, sizeof(struct wlan_hdr));

	if (qid != 10000)
	{
		if (qid < 500)
		{
			if (qid == 0)
				gas_req->Reqlen = 0;
			else if (qid == 270)
				gas_req->Reqlen = 26;
			else if (qid == 271)
				gas_req->Reqlen = 27;
			else if (qid == 272)
				gas_req->Reqlen = 15;
			else
				gas_req->Reqlen = 6;
		}
		else if(qid == 501)
			gas_req->Reqlen = 12;
		else if(qid == 502)
			gas_req->Reqlen = 12;
		else if(qid == 503)
			gas_req->Reqlen = 24;
		else if(qid == 504)
			gas_req->Reqlen = 36;
		else if(qid == 505)
			gas_req->Reqlen = 44;
		else if(qid == 506)
			gas_req->Reqlen = 11;
		else if(qid == 507)
			gas_req->Reqlen = 11;
		else if(qid == 508)
			gas_req->Reqlen = 25;
		else if(qid == 509)
			gas_req->Reqlen = 19;
	}
	else
		gas_req->Reqlen = 10;

	if ((OPMODE & WIFI_ASOC_STATE) == 0)
	{
		//unsigned char tmpmac[]={0x00,0xe0,0x4c,0x09,0x08,0x10};
		//memcpy((GET_MIB(priv))->dot11StationConfigEntry.dot11Bssid, tmpmac, 6);
		memcpy((GET_MIB(priv))->dot11StationConfigEntry.dot11Bssid, priv->pmib->hs2Entry.redir_mac, 6);
	}
	memcpy(gas_req->MACAddr , BSSID, 6);
//	gas_req->MACAddr[1] = 0x33;
//	gas_req->MACAddr[2] = 0x44;
//	gas_req->MACAddr[3] = 0x55;
//	gas_req->MACAddr[4] = 0x66;
//	gas_req->MACAddr[5] = 0x77;

	pbuf[0] = _PUBLIC_CATEGORY_ID_;
	if (qid != 0)
		pbuf[1] = _GAS_INIT_REQ_ACTION_ID_;
	else
		pbuf[1] = _GAS_COMBACK_REQ_ACTION_ID_;

	pbuf[2] = gas_req->Dialog_token;

	if (qid != 0)
	{
		// refer to Fig. 8-354 in IEEE 802.11 - 2012
		pbuf[3] = 108; // element ID: 108 (Advertisement Protocol IE)
		pbuf[4] = 2;  // length = 0x 02
		pbuf[5] = 0x00; // Query Response Length Limit
		pbuf[6] = 0;  // Advertisement Protcol ID = 0x0 (ANQP)
	if (priv->pshare->rf_ft_var.swq_dbg == 12)
		pbuf[6] = 1;	// Advertisement Protcol ID = 0x1 (MIH)
	else
		pbuf[6] = 0;	//  Advertisement Protcol ID = 0x0 (ANQP)
	pbuf[7] = gas_req->Reqlen & 0x00ff;	// Query Request length
	pbuf[8] = (gas_req->Reqlen & 0xff00) >> 8; // Query Request length
	pbuf[9] = 256 & 0x00ff; // Info ID = 256 ( Query List)
	// Query Request fields
	// Info ID = 256 ( Query List)
	pbuf[10] = (256 & 0xff00) >> 8;

	if (qid != 10000)
	{
		if (qid < 500)
		{
			if (qid == 270)
			{
				pbuf[11] = 2 & 0x00ff;			// length
				pbuf[12] = (2 & 0xff00) >> 8;
				pbuf[13] = 261 & 0x00ff;		// InfoID: Roaming list
				pbuf[14] = (261 & 0xff00) >> 8; // InfoID: Roaming list
				pbuf[15] = 0xdd;
				pbuf[16] = 0xdd;
				pbuf[17] = 0x06;
				pbuf[18] = 0x00;
				pbuf[19] = 0x50;
				pbuf[20] = 0x6f;
				pbuf[21] = 0x9a;
				pbuf[22] = 0x11;
				pbuf[23] = 0x04; // WAN Metrics
				pbuf[24] = 0x00;
				pbuf[25] = 0xdd;
				pbuf[26] = 0xdd;
				pbuf[27] = 0x06;
				pbuf[28] = 0x00;
				pbuf[29] = 0x50;
				pbuf[30] = 0x6f;
				pbuf[31] = 0x9a;
				pbuf[32] = 0x11;
				pbuf[33] = 0x07; // Operating Class Indication
				pbuf[34] = 0x00;
				txinsn.fr_len += 35;
			}
			else if (qid == 271)
			{
				pbuf[11] = 2 & 0x00ff;
				pbuf[12] = (2 & 0xff00) >> 8;
				pbuf[13] = 261 & 0x00ff;		// InfoID: Roaming list
				pbuf[14] = (261 & 0xff00) >> 8; // InfoID: Roaming list
				pbuf[15] = 0xdd;
				pbuf[16] = 0xdd;
				pbuf[17] = 0x06;
				pbuf[18] = 0x00;
				pbuf[19] = 0x50;
				pbuf[20] = 0x6f;
				pbuf[21] = 0x9a;
				pbuf[22] = 0x0b;
				pbuf[23] = 0x00; // Subtype = 0 (Reserved)
				pbuf[24] = 0x00;
				pbuf[25] = 0xdd;
				pbuf[26] = 0xdd;
				pbuf[27] = 0x07;
				pbuf[28] = 0x00;
				pbuf[29] = 0x50;
				pbuf[30] = 0x6f;
				pbuf[31] = 0x9a;
				pbuf[32] = 0x11;
				pbuf[33] = 0x01; // HS Query List
				pbuf[34] = 0x00;
				pbuf[35] = 0x07; // Query Operating Class Indication
				txinsn.fr_len += 24;
			}
			else if (qid  == 272) {
				pbuf[11] = 2& 0x00ff;
				pbuf[12] = (2& 0xff00) >> 8;
				pbuf[13] = 261 & 0x00ff;		// InfoID: Roaming list
				pbuf[14] = (261 & 0xff00) >> 8; // InfoID: Roaming list
				pbuf[15] = 0xdd;
				pbuf[16] = 0xdd;
				pbuf[17] = 0x05;
				pbuf[18] = 0x00;
				pbuf[19] = 0x50;
				pbuf[20] = 0x6f;
				pbuf[21] = 0x9a;
				pbuf[22] = 0x0b;
				pbuf[23] = 0x00; // Subtype = 0 (Reserved)
				txinsn.fr_len += 36;
			}
			else
			{
				// ANQP Query List (Fig. 8-403 in IEEE 802.11-2012)
				// length = 2
				pbuf[11] = 2 & 0x00ff;
				pbuf[12] = (2 & 0xff00) >> 8;
				// ANQP Query ID
				pbuf[13] = qid & 0x00ff;
				pbuf[14] = (qid & 0xff00) >> 8;
				txinsn.fr_len += 15;
			}
		}
		else if(qid == 501)
		{
			pbuf[9] = 56797 & 0x00ff;
			pbuf[10] = (56797 & 0xff00) >> 8;
			pbuf[11] = 0x08;
			pbuf[12] = 0;
			//OI
			pbuf[13] =0x50;
			pbuf[14] =0x6f;
			pbuf[15] =0x9a;
			pbuf[16] =0x11;
			pbuf[17] =0x1; // HS query list
			pbuf[18] =0x0;
			//payload
			pbuf[19] =0x2; // HS Capability List
			pbuf[20] =0x3; // Operator Friendly Name
			txinsn.fr_len += 21;
		}
		else if(qid == 502)
		{
			pbuf[9] = 56797 & 0x00ff;
			pbuf[10] = (56797 & 0xff00) >> 8;
			pbuf[11] = 0x08;
			pbuf[12] = 0;
			//OI
			pbuf[13] =0x50;
			pbuf[14] =0x6f;
			pbuf[15] =0x9a;
			pbuf[16] =0x11;
			pbuf[17] =0x1; // HS query list
			pbuf[18] =0x0;
			//payload
			pbuf[19] =0x4;
			pbuf[20] =0x5;
			txinsn.fr_len += 21;
		}
		else if(qid == 505)
		{
			pbuf[9] = 56797 & 0x00ff;
            pbuf[10] = (56797 & 0xff00) >> 8;
            pbuf[11] = 0x08;
            pbuf[12] = 0;
			pbuf[13] =0x50;
            pbuf[14] =0x6f;
            pbuf[15] =0x9a;
			pbuf[16] =0x11;
			pbuf[17] =0x1; // HS query list
			pbuf[18] =0x0;
			pbuf[19] =0x4;
            pbuf[20] =0x5;
			pbuf[21] = 56797 & 0x00ff;
            pbuf[22] = (56797 & 0xff00) >> 8;
            pbuf[23] = 0x06;
            pbuf[24] = 0;
            pbuf[25] =0x50;
            pbuf[26] =0x6f;
            pbuf[27] =0x9a;
            pbuf[28] =0x0b;
            pbuf[29] =0x0; // Reserved
			pbuf[30] =0x0;
			pbuf[31] = 56797 & 0x00ff;
            pbuf[32] = (56797 & 0xff00) >> 8;
            pbuf[33] = 21;
            pbuf[34] = 0;
            pbuf[35] =0x50;
            pbuf[36] =0x6f;
            pbuf[37] =0x9a;
            pbuf[38] =0x11;
            pbuf[39] =0x06; // Subtype: NAI Home Realm Query
			pbuf[40] =0x0;
			pbuf[41] =0x1;
            pbuf[42] =0x0;
            pbuf[43] =0xc;
            pbuf[44] ='e';
            pbuf[45] ='x';
            pbuf[46] ='a';
            pbuf[47] ='m';
            pbuf[48] ='p';
            pbuf[49] ='l';
            pbuf[50] ='e';
            pbuf[51] ='.';
            pbuf[52] ='c';
            pbuf[53] ='o';
            pbuf[54] ='m';

            txinsn.fr_len += 55;

		}
		else if(qid == 503)
		{
			pbuf[9] = 56797 & 0x00ff;
			pbuf[10] = (56797 & 0xff00) >> 8;
			pbuf[11] = 21;
			pbuf[12] = 0;
			//OI
			pbuf[13] =0x50;
			pbuf[14] =0x6f;
			pbuf[15] =0x9a;
			//TYPE
			pbuf[16] =0x11;
			//home realm query
			pbuf[17] =0x6;
			pbuf[18] =0x0;
			//payload
			pbuf[19] =0x1;
			pbuf[20] =0x0;
			pbuf[21] =0xc;
			pbuf[22] ='e';
			pbuf[23] ='x';
			pbuf[24] ='a';
			pbuf[25] ='m';
			pbuf[26] ='p';
			pbuf[27] ='l';
			pbuf[28] ='e';
			pbuf[29] ='.';
			pbuf[30] ='c';
			pbuf[31] ='o';
			pbuf[32] ='m';
			txinsn.fr_len += 33;
		}
		else if (qid == 506)
		{
			pbuf[9] = 56797 & 0x00ff;
            pbuf[10] = (56797 & 0xff00) >> 8;
            pbuf[11] = 7;
            pbuf[12] = 0;
			pbuf[13] =0x50;
            pbuf[14] =0x6f;
            pbuf[15] =0x9a;
			pbuf[16] =0x11;
			pbuf[17] =0x6;
			pbuf[18] =0x0;
			pbuf[19] =0x0;
			txinsn.fr_len += 20;
		}
		else if (qid == 507)
        {
            pbuf[9] = 56797 & 0x00ff;
            pbuf[10] = (56797 & 0xff00) >> 8;
            pbuf[11] = 7;
            pbuf[12] = 0;
            pbuf[13] =0x50;
            pbuf[14] =0x6f;
            pbuf[15] =0x9a;
            pbuf[16] =0x11;
            pbuf[17] =0x1;
            pbuf[18] =0x0;
			pbuf[19] =0x7;
            txinsn.fr_len += 20;
        }
		else if (qid == 504)
		{
			pbuf[9] = 56797 & 0x00ff;
            pbuf[10] = (56797 & 0xff00) >> 8;
            pbuf[11] = 32;
            pbuf[12] = 0;
			pbuf[13] =0x50;
            pbuf[14] =0x6f;
            pbuf[15] =0x9a;
			pbuf[16] =0x11;
			pbuf[17] =0x6;
			pbuf[18] =0x0;
			pbuf[19] =0x2;
			pbuf[20] =0x0;
            pbuf[21] =0x9;
            pbuf[22] ='c';
            pbuf[23] ='i';
            pbuf[24] ='s';
            pbuf[25] ='c';
            pbuf[26] ='o';
            pbuf[27] ='.';
            pbuf[28] ='c';
            pbuf[29] ='o';
            pbuf[30] ='m';

            pbuf[31] =0x0;
            pbuf[32] =0xc;
            pbuf[33] ='e';
            pbuf[34] ='x';
            pbuf[35] ='a';
            pbuf[36] ='m';
            pbuf[37] ='p';
            pbuf[38] ='l';
            pbuf[39] ='e';
            pbuf[40] ='4';
            pbuf[41] ='.';
            pbuf[42] ='c';
            pbuf[43] ='o';
            pbuf[44] ='m';
            txinsn.fr_len += 45;
		}
		else if (qid == 508)
        {
            pbuf[9] = 56797 & 0x00ff;
            pbuf[10] = (56797 & 0xff00) >> 8;
            pbuf[11] = 15;
            pbuf[12] = 0;
            pbuf[13] =0x50;
            pbuf[14] =0x6f;
            pbuf[15] =0x9a;
            pbuf[16] =0x11;
            pbuf[17] =0xa;
            pbuf[18] =0x0;
			pbuf[19] ='1';
			pbuf[20] ='3';
			pbuf[21] ='5';
			pbuf[22] ='7';
			pbuf[23] ='1';
			pbuf[24] ='6';
			pbuf[25] ='1';
			pbuf[26] ='4';
			pbuf[27] ='7';
			pbuf[28] ='5';
			pbuf[29] ='_';
			pbuf[30] ='w';
			pbuf[31] ='i';
			pbuf[32] ='f';
			pbuf[33] ='i';

            txinsn.fr_len += 34;
        }
		else if (qid == 509)
		{

			pbuf[11] = 4 & 0x00ff;			// length
			pbuf[12] = (4 & 0xff00) >> 8;
			pbuf[13] = 260 & 0x00ff;		// InfoID: Network Authentication Type
			pbuf[14] = (260 & 0xff00) >> 8; // InfoID: Network Authentication Type
			pbuf[15] = 263 & 0x00ff;		// InfoID: NAI Realm List
			pbuf[16] = (263 & 0xff00) >> 8; // InfoID: NAI Realm List
			pbuf[17] = 56797 & 0x00ff;
			pbuf[18] = (56797 & 0xff00) >> 8;
			pbuf[19] = 0x07;
			pbuf[20] = 0;
			//OI
			pbuf[21] =0x50;
			pbuf[22] =0x6f;
			pbuf[23] =0x9a;
			pbuf[24] =0x11;
			pbuf[25] =0x1; // HS query list
			pbuf[26] =0x0;
			//payload
			pbuf[27] =0x8; // OSU Providers List
			txinsn.fr_len += 28;
		}
	}
	else
	{
		pbuf[11] = 6 & 0x00ff;
		pbuf[12] = (6 & 0xff00) >> 8;
		pbuf[13] = 263 & 0x00ff;
		pbuf[14] = (263 & 0xff00) >> 8;
		pbuf[15] = 264 & 0x00ff;
		pbuf[16] = (264 & 0xff00) >> 8;
		pbuf[17] = 268 & 0x00ff;
		pbuf[18] = (268 & 0xff00) >> 8;
		txinsn.fr_len += 19;
	}
	}
	else
	{
		txinsn.fr_len += 11;
	}
	SetFrameSubType((txinsn.phdr), WIFI_WMM_ACTION);

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), gas_req->MACAddr, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), gas_req->MACAddr, MACADDRLEN);

	if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS)
		return SUCCESS;
}
#endif

int issue_GASrsp(struct rtl8192cd_priv *priv, DOT11_HS2_GAS_RSP *gas_rsp)
{
	unsigned char	*pbuf;
	struct stat_info *pstat;
#if defined(WIFI_WMM)
	int ret;
#endif
	HS2_DEBUG_INFO("Rsp len=[%d]\n",gas_rsp->Rsplen);
	DECLARE_TXINSN(txinsn);

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
	txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif
	txinsn.fixed_rate = 1;

	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);
	if (pbuf == NULL)
		goto issue_GASrsp_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);
	if (txinsn.phdr == NULL)
		goto issue_GASrsp_fail;

	memset((void *)(txinsn.phdr), 0, sizeof(struct wlan_hdr));

	pbuf[0] = _PUBLIC_CATEGORY_ID_;
	pbuf[1] = gas_rsp->Action;
	pbuf[2] = gas_rsp->Dialog_token;
	pbuf[3] = gas_rsp->StatusCode & 0x00ff;
	pbuf[4] = (gas_rsp->StatusCode & 0xff00) >> 8;

	if (gas_rsp->Action == _GAS_INIT_RSP_ACTION_ID_)
	{
		pbuf[5] = gas_rsp->Comeback_delay & 0x00ff;
		pbuf[6] = (gas_rsp->Comeback_delay & 0xff00) >> 8;
		//advertisement protocol element
		pbuf[7] = 0x6c;
		pbuf[8] = 2;
		pbuf[9] = 0x20;
		pbuf[10] = 0;//gas_rsp->Advt_proto;
		//gas rsp
		pbuf[11] = gas_rsp->Rsplen & 0x00ff;
		pbuf[12] = (gas_rsp->Rsplen & 0xff00) >> 8;
		if (gas_rsp->Rsplen > 0)
			memcpy(&pbuf[13], gas_rsp->Rsp, gas_rsp->Rsplen);

		txinsn.fr_len += 13 +	gas_rsp->Rsplen;
	}
	else if (gas_rsp->Action == _GAS_COMBACK_RSP_ACTION_ID_)
	{
		pbuf[5] = gas_rsp->Rsp_fragment_id;
		pbuf[6] = gas_rsp->Comeback_delay & 0x00ff;
		pbuf[7] = (gas_rsp->Comeback_delay & 0xff00) >> 8;
		//advertisement protocol element
		pbuf[8] = 0x6c;
		pbuf[9] = 2;
		pbuf[10] = 0x20;
		pbuf[11] = 0;//gas_rsp->Advt_proto;
		//gas rsp
		pbuf[12] = gas_rsp->Rsplen & 0x00ff;
		pbuf[13] = (gas_rsp->Rsplen & 0xff00) >> 8;
		if (gas_rsp->Rsplen > 0)
			memcpy(&pbuf[14], gas_rsp->Rsp, gas_rsp->Rsplen);

		txinsn.fr_len += 14 +	gas_rsp->Rsplen;
	}
	else
	{
		//unknown action
		goto issue_GASrsp_fail;
	}

	SetFrameSubType((txinsn.phdr), WIFI_WMM_ACTION);

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), gas_rsp->MACAddr, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);

	pstat = get_stainfo(priv, gas_rsp->MACAddr);

#if defined(WIFI_WMM)
	ret = check_dz_mgmt(priv, pstat, &txinsn);

	if (ret < 0)
		goto issue_GASrsp_fail;
	else if (ret==1)
		return SUCCESS;
	else
#endif
	{
		if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS)
			return SUCCESS;
	}

issue_GASrsp_fail:
	HS2_DEBUG_INFO("issue_GASrsp_fail\n");
	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);

	return FAIL;
}

int issue_BSS_TSM_req(struct rtl8192cd_priv *priv, DOT11_HS2_TSM_REQ *tsm_req)
{
	unsigned char	*pbuf;
	struct stat_info *pstat;
	unsigned int curLen;
#if defined(WIFI_WMM)
	int ret;
#endif

	DECLARE_TXINSN(txinsn);
    HS2DEBUG("\n");

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
	txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif
	txinsn.fixed_rate = 1;
#ifdef CONFIG_IEEE80211W
	struct stat_info *psta;
	if(!memcmp(tsm_req->MACAddr,"\xff\xff\xff\xff\xff\xff",6)) {
		txinsn.isPMF = 1; //?????
	} else {
		psta = get_stainfo(priv,tsm_req->MACAddr);
		if(!psta) {
			printk("STA does not exist\n");
			return 0;
		}
		if(psta)
			txinsn.isPMF = psta->isPMF;
		else
			txinsn.isPMF = 0;
	}
		//printk("deauth:txinsn.isPMF=%d\n",txinsn.isPMF);
#endif

	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);
	if (pbuf == NULL)
		goto issue_BSS_TxMgmt_req_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);
	if (txinsn.phdr == NULL)
		goto issue_BSS_TxMgmt_req_fail;

	memset((void *)(txinsn.phdr), 0, sizeof(struct wlan_hdr));

	pbuf[0] = _WNM_CATEGORY_ID_;
	pbuf[1] = _BSS_TSMREQ_ACTION_ID_;
	pbuf[2] = tsm_req->Dialog_token;			//token
	pbuf[3] = tsm_req->Req_mode;				//request mode
	pbuf[4] = tsm_req->Disassoc_timer & 0xff;	//disassociation timer
	pbuf[5] = (tsm_req->Disassoc_timer & 0xff00) >> 8;
	pbuf[6] = tsm_req->Validity_intval;			//validity interval
	curLen = 7;
	if (tsm_req->term_len != 0) { // BSS Termination Duration field
		memcpy(&pbuf[curLen], tsm_req->terminal_dur, 12);
		curLen += tsm_req->term_len;
	}
	if (tsm_req->url_len != 0) { //Session Information URL field
		pbuf[curLen++] = tsm_req->url_len;
		memcpy(&pbuf[curLen], tsm_req->Session_url, tsm_req->url_len);
		curLen += tsm_req->url_len;
	}
	if (tsm_req->list_len != 0) {
		memcpy(&pbuf[curLen], tsm_req->Candidate_list, tsm_req->list_len);
		curLen += tsm_req->list_len;
	}

	txinsn.fr_len += 7+tsm_req->term_len+1+tsm_req->url_len+tsm_req->list_len;

	SetFrameSubType((txinsn.phdr), WIFI_WMM_ACTION);

#ifdef CONFIG_IEEE80211W
	if (txinsn.isPMF)
		*(unsigned char*)(txinsn.phdr+1) |= BIT(6); // enable privacy
#endif
	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), tsm_req->MACAddr, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);

	pstat = get_stainfo(priv, tsm_req->MACAddr);

#if defined(WIFI_WMM)
	ret = check_dz_mgmt(priv, pstat, &txinsn);

	if (ret < 0)
		goto issue_BSS_TxMgmt_req_fail;
	else if (ret==1)
		return SUCCESS;
	else
#endif
	{
		if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS)
			return SUCCESS;
	}

issue_BSS_TxMgmt_req_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);

	return FAIL;
}

int issue_DLS_rsp(struct rtl8192cd_priv *priv, unsigned short status, unsigned char *da, unsigned char *dest, unsigned char *src)
{
	unsigned char   *pbuf;
    struct stat_info *pstat;
#if defined(WIFI_WMM)
    int ret;
#endif

    DECLARE_TXINSN(txinsn);

    txinsn.q_num = MANAGE_QUE_NUM;
    txinsn.fr_type = _PRE_ALLOCMEM_;
    txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
    txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif
    txinsn.fixed_rate = 1;

    pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);
	if (pbuf == NULL)
        goto issue_DLS_rsp_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);
    if (txinsn.phdr == NULL)
        goto issue_DLS_rsp_fail;

    memset((void *)(txinsn.phdr), 0, sizeof(struct wlan_hdr));

	pbuf[0] = _DLS_CATEGORY_ID_;
    pbuf[1] = _DLS_RSP_ACTION_ID_;
    pbuf[2] = status & 0xff;
    pbuf[3] = (status >> 8) & 0xff;           //status code
	memcpy(&pbuf[4], dest, 6);
	memcpy(&pbuf[10], src, 6);

	txinsn.fr_len += 16;

	pstat = get_stainfo(priv, da);

	SetFrameSubType((txinsn.phdr), WIFI_WMM_ACTION);

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), da, MACADDRLEN);
    memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
    memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);

#if defined(WIFI_WMM)
    ret = check_dz_mgmt(priv, pstat, &txinsn);

    if (ret < 0)
        goto issue_DLS_rsp_fail;
    else if (ret==1)
        return SUCCESS;
    else
#endif
    {
        if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS)
            return SUCCESS;
    }

issue_DLS_rsp_fail:
    if (txinsn.phdr)
        release_wlanhdr_to_poll(priv, txinsn.phdr);
    if (txinsn.pframe)
        release_mgtbuf_to_poll(priv, txinsn.pframe);

    return FAIL;
}

#ifdef HS2_CLIENT_TEST
//issue_TSM Query for client test used
int issue_BSS_TSM_query(struct rtl8192cd_priv *priv, unsigned char *list, unsigned char list_len)
{
	unsigned char	*pbuf;
	struct stat_info *pstat;
#if defined(WIFI_WMM)
	int ret;
#endif
	unsigned char tmpda[]={0x00,0x33,0x44,0x055,0x66,0x77};

	DECLARE_TXINSN(txinsn);

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
	txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif
	txinsn.fixed_rate = 1;

	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);
	if (pbuf == NULL)
		goto issue_BSS_TxMgmt_query_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);
	if (txinsn.phdr == NULL)
		goto issue_BSS_TxMgmt_query_fail;

	memset((void *)(txinsn.phdr), 0, sizeof(struct wlan_hdr));

	pbuf[0] = _WNM_CATEGORY_ID_;
	pbuf[1] = _WNM_TSMQUERY_ACTION_ID_;
	pbuf[2] = 20;			//token
	pbuf[3] = 18;				//request mode

	if (list_len >0 )
		memcpy(&pbuf[4], list, list_len);
	txinsn.fr_len += 4 + list_len;

	SetFrameSubType((txinsn.phdr), WIFI_WMM_ACTION);

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);

	pstat = get_stainfo(priv, BSSID);

#if defined(WIFI_WMM)
	ret = check_dz_mgmt(priv, pstat, &txinsn);

	if (ret < 0)
		goto issue_BSS_TxMgmt_query_fail;
	else if (ret==1)
		return SUCCESS;
	else
#endif
	{
		if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS)
			return SUCCESS;
	}

issue_BSS_TxMgmt_query_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);

	return FAIL;
}

//issue_TSM response for client test used
int issue_BSS_TSM_rsp(struct rtl8192cd_priv *priv, unsigned char *token, unsigned char *list, unsigned char list_len)
{
	unsigned char	*pbuf;
	struct stat_info *pstat;
#if defined(WIFI_WMM)
	int ret;
#endif
	unsigned char tmpda[]={0x00,0x33,0x44,0x055,0x66,0x77};

	DECLARE_TXINSN(txinsn);

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
	txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif
	txinsn.fixed_rate = 1;

	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);
	if (pbuf == NULL)
		goto issue_BSS_TxMgmt_rsp_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);
	if (txinsn.phdr == NULL)
		goto issue_BSS_TxMgmt_rsp_fail;

	memset((void *)(txinsn.phdr), 0, sizeof(struct wlan_hdr));

	pbuf[0] = _WNM_CATEGORY_ID_;
	pbuf[1] = _BSS_TSMRSP_ACTION_ID_;
	pbuf[2] = *token;			//token
	pbuf[3] = 0;				//request mode
	pbuf[4] = 0;

	if (list_len != 0)
		memcpy(&pbuf[5], list, list_len);

	txinsn.fr_len += 5+list_len;

	SetFrameSubType((txinsn.phdr), WIFI_WMM_ACTION);

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);

	pstat = get_stainfo(priv, BSSID);

#if defined(WIFI_WMM)
	ret = check_dz_mgmt(priv, pstat, &txinsn);

	if (ret < 0)
		goto issue_BSS_TxMgmt_rsp_fail;
	else if (ret==1)
		return SUCCESS;
	else
#endif
	{
		if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS)
			return SUCCESS;
	}

issue_BSS_TxMgmt_rsp_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);

	return FAIL;
}
#endif
#endif

#if defined(WIFI_WMM)
#ifdef RTK_AC_SUPPORT //for 11ac logo

void issue_op_mode_notify(struct rtl8192cd_priv *priv, struct stat_info *pstat, char mode)
{
	unsigned char	*pbuf;
	DECLARE_TXINSN(txinsn);

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
    	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
	txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif
	txinsn.fixed_rate = 1;

	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);
	if (pbuf == NULL)
		goto issue_opm_notification_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);
	if (txinsn.phdr == NULL)
		goto issue_opm_notification_fail;

	memset((void *)(txinsn.phdr), 0, sizeof(struct wlan_hdr));

	pbuf[0] = _VHT_ACTION_CATEGORY_ID_;
	pbuf[1] = _VHT_ACTION_OPMNOTIF_ID_;
	pbuf[2] = mode;
	txinsn.fr_len = _OPMNOTIF_Frame_Length_;

	SetFrameSubType((txinsn.phdr), WIFI_WMM_ACTION);
	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), pstat->hwaddr, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);


	if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS) {
		return;
	}

issue_opm_notification_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
	return;
}

#endif

/*-------------------------------------------------------------------------------
	Check if packet should be queued
return value:
-1: fail
1: success
0: no queue
--------------------------------------------------------------------------------*/
int check_dz_mgmt(struct rtl8192cd_priv *priv, struct stat_info *pstat, struct tx_insn* txcfg)
{
	if (pstat && ((pstat->state & (WIFI_SLEEP_STATE | WIFI_ASOC_STATE)) ==
			(WIFI_SLEEP_STATE | WIFI_ASOC_STATE))){
		int ret;
		struct tx_insn *ptx_insn;
		ptx_insn = (struct tx_insn*)kmalloc(sizeof(struct tx_insn), GFP_ATOMIC);

		if (ptx_insn == NULL){
			printk("%s: not enough memory\n", __FUNCTION__);
			return -1;
		}
		memcpy((void *)ptx_insn, (void *)txcfg, sizeof(struct tx_insn));

		//printk("%s %d\n",__FUNCTION__,__LINE__);
		DEBUG_INFO("h= %d t=%d\n", (pstat->MGT_dz_queue->head), (pstat->MGT_dz_queue->tail));
		ret = enque(priv, &(pstat->MGT_dz_queue->head), &(pstat->MGT_dz_queue->tail),
					(unsigned long)(pstat->MGT_dz_queue->ptx_insn), NUM_DZ_MGT_QUEUE, (void *)ptx_insn);

		if (ret == FALSE) {
			kfree(ptx_insn);
			DEBUG_ERR("MGT_dz_queue full!\n");
			return -1;
		}

		return 1; // success
	}else{
		return 0; // no queue
	}
}
#endif


#ifdef CONFIG_IEEE80211W

void stop_sa_query(struct stat_info *pstat)
{
	PMFDEBUG("RX 11W_SA_Rsp , stop send sa query again\n");
	pstat->sa_query_count = 0;
	if (timer_pending(&pstat->SA_timer))
		del_timer(&pstat->SA_timer);
}


int check_sa_query_timeout(struct stat_info *pstat)
{
	if(pstat->sa_query_end <= jiffies) {
		PMFDEBUG("sa query time out\n");
		pstat->sa_query_timed_out = 1;
		pstat->sa_query_count = 0;
		if (timer_pending(&pstat->SA_timer))
			del_timer(&pstat->SA_timer);
		return 1;
	}
	return 0;
}

void rtl8192cd_sa_query_timer(unsigned long task_priv)
{
	struct stat_info        *pstat = (struct stat_info *)task_priv;
	struct rtl8192cd_priv *priv = NULL;
	struct aid_obj *aidobj;

	if(!pstat)
		return ;

	aidobj = container_of(pstat, struct aid_obj, station);
	priv = aidobj->priv;


	if (!(priv->drv_state & DRV_STATE_OPEN))
		return;

    if(pstat->sa_query_count > 0 && check_sa_query_timeout(pstat)) {
		PMFDEBUG("check_sa_query_timeout\n");
		return;
    }
	if(pstat->sa_query_count < SA_QUERY_MAX_NUM) {
		PMFDEBUG("re Send sa query\n");
		pstat->sa_query_count++;
		issue_SA_Query_Req(priv->dev,pstat->hwaddr);
	}
	mod_timer(&pstat->SA_timer, jiffies + RTL_MILISECONDS_TO_JIFFIES(SA_QUERY_RETRY_TO));
}

int issue_SA_Query_Req(struct net_device *dev, unsigned char *da)
{
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);


	struct stat_info *pstat;
	unsigned char	*pbuf;
	int ret;
	DECLARE_TXINSN(txinsn);

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);

	txinsn.fixed_rate = 1;

	pstat = get_stainfo(priv, da);

	if(pstat)
		txinsn.isPMF = pstat->isPMF;
	else
		txinsn.isPMF = 0;

	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);
	if (pbuf == NULL)
		goto issue_SA_Query_Req_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);
	if (txinsn.phdr == NULL)
		goto issue_SA_Query_Req_fail;

	memset((void *)(txinsn.phdr), 0, sizeof(struct wlan_hdr));

	pbuf[0] = _SA_QUERY_CATEGORY_ID_;
	pbuf[1] = _SA_QUERY_REQ_ACTION_ID_;

	get_random_bytes(&pstat->SA_TID[pstat->sa_query_count], sizeof(unsigned short));

	pbuf[2] = pstat->SA_TID[pstat->sa_query_count] & 0xff;
	pbuf[3] = (pstat->SA_TID[pstat->sa_query_count] & 0xff00) >> 8;
#ifdef CONFIG_IEEE80211W_AP_DEBUG
	panic_printk("DA[%02x%02x%02x:%02x%02x%02x] STA_TID=[%02x%02x]\n",
	da[0],da[1],da[2],da[3],da[4],da[5],pbuf[2], pbuf[3]);
#endif
	txinsn.fr_len = 4;

	SetFrameSubType((txinsn.phdr), WIFI_WMM_ACTION);

	if (txinsn.isPMF)
		*(unsigned char*)(txinsn.phdr+1) |= BIT(6); // enable privacy

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), da, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);

	if (pstat == NULL){
		PMFDEBUG("issue_SA_Query_Req_fail\n");
		goto issue_SA_Query_Req_fail;
    }

	txinsn.pstat = pstat;

#if defined(WIFI_WMM)
	ret = check_dz_mgmt(priv, pstat, &txinsn);

	if (ret < 0){
		PMFDEBUG("issue_SA_Query_Req_fail\n");
		goto issue_SA_Query_Req_fail;
	}else if (ret==1) {
		PMFDEBUG("sta go to sleep... Q it\n");
		return SUCCESS;
	}
	else
#endif
	{
		if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS) {

    		PMFDEBUG("SUCCESS!!\n");
			return SUCCESS;
		}
	}

issue_SA_Query_Req_fail:

    PMFDEBUG("issue_SA_Query_Req_fail\n");

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);

	return FAIL;
}


int issue_SA_Query_Rsp(struct net_device *dev, unsigned char *da, unsigned char *trans_id)
{
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);

	struct stat_info *pstat;
	unsigned char	*pbuf;
	int ret;
	DECLARE_TXINSN(txinsn);

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);

	txinsn.fixed_rate = 1;

	pstat = get_stainfo(priv, da);

	if(pstat)
		txinsn.isPMF = pstat->isPMF;
	else
		txinsn.isPMF = 0;

	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);
	if (pbuf == NULL)
		goto issue_SA_Query_Rsp_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);
	if (txinsn.phdr == NULL)
		goto issue_SA_Query_Rsp_fail;

	memset((void *)(txinsn.phdr), 0, sizeof(struct wlan_hdr));

	pbuf[0] = _SA_QUERY_CATEGORY_ID_;
	pbuf[1] = _SA_QUERY_RSP_ACTION_ID_;
	memcpy(pbuf+2,trans_id, 2);
#ifdef CONFIG_IEEE80211W_CLI_DEBUG
	panic_printk("TID= %02x%02x\n", pbuf[2], pbuf[3]);
#endif

	txinsn.fr_len = 4;

	SetFrameSubType((txinsn.phdr), WIFI_WMM_ACTION);

	if (txinsn.isPMF)
		*(unsigned char*)(txinsn.phdr+1) |= BIT(6); // enable privacy

	PMFDEBUG("\n");
	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), da, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
#ifdef CONFIG_IEEE80211W_AP_DEBUG
       panic_printk("DA[%02x%02x%02x:%02x%02x%02x]\n",da[0],da[1],da[2],da[3],da[4],da[5]);
#endif

	if (pstat == NULL)
		goto issue_SA_Query_Rsp_fail;

	txinsn.pstat = pstat;

#if defined(WIFI_WMM)
	ret = check_dz_mgmt(priv, pstat, &txinsn);
	if (ret < 0)
		goto issue_SA_Query_Rsp_fail;
	else if (ret==1) {
		return SUCCESS;
	}
	else
#endif
	{
		if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS)
			return SUCCESS;
	}

issue_SA_Query_Rsp_fail:
	PMFDEBUG("issue_SA_Query_Rsp_fail\n");

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);

	return FAIL;
}
#endif

int issue_ADDBArsp(struct rtl8192cd_priv *priv, unsigned char *da, unsigned char dialog_token,
				unsigned char TID, unsigned short status_code, unsigned short timeout)
{
	unsigned char	*pbuf;
	unsigned short	ba_para = 0;
	struct stat_info *pstat;
	int max_size;
#if defined(WIFI_WMM)
	int ret;
#endif

	DECLARE_TXINSN(txinsn);

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
    	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
	txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif
	txinsn.fixed_rate = 1;

	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);
	if (pbuf == NULL)
		goto issue_ADDBArsp_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);
	if (txinsn.phdr == NULL)
		goto issue_ADDBArsp_fail;

	memset((void *)(txinsn.phdr), 0, sizeof(struct wlan_hdr));

	pbuf[0] = _BLOCK_ACK_CATEGORY_ID_;
	pbuf[1] = _ADDBA_Rsp_ACTION_ID_;
	pbuf[2] = dialog_token;
	pbuf[3] = status_code & 0x00ff;
	pbuf[4] = (status_code & 0xff00) >> 8;

	pstat = get_stainfo(priv, da);

#ifdef CONFIG_IEEE80211W
	 if(pstat)
		 txinsn.isPMF = pstat->isPMF;
	 else
		 txinsn.isPMF = 0;
		 //printk("deauth:txinsn.isPMF=%d\n",txinsn.isPMF);
#endif

	if (priv->pmib->dot11nConfigEntry.dot11nAMPDURevSz){
		max_size = priv->pmib->dot11nConfigEntry.dot11nAMPDURevSz;
	} else if (pstat && should_restrict_Nrate(priv, pstat))
		max_size = 1;
	else {
	{
		if((GET_CHIP_VER(priv)== VERSION_8197F || GET_CHIP_VER(priv)==VERSION_8192E) && pstat && pstat->tx_bw == HT_CHANNEL_WIDTH_20)
			max_size = 16;
		else
#ifdef MULTI_STA_REFINE
		if (priv->pshare->total_assoc_num >10)
			max_size = 	_ADDBA_Maximum_Buffer_Size_>>2;
		else
#endif
			max_size = _ADDBA_Maximum_Buffer_Size_;
	}

	}

	ba_para = (max_size<<6) | (TID<<2) | BIT(1);	// assign buffer size | assign TID | set Immediate Block Ack

#if defined(SUPPORT_RX_AMSDU_AMPDU)
	if(AMSDU_ENABLE >= 2) {
		if (!((GET_CHIP_VER(priv)== VERSION_8814A) && priv->pmib->dot11nConfigEntry.dot11nUse40M == 0))
			ba_para |= BIT(0);
	}
#endif

#ifdef RTK_AC_SUPPORT //for 11ac logo
	if((AC_SIGMA_MODE == AC_SIGMA_APUT) && (AMSDU_ENABLE >= 1))
	ba_para |= BIT(0);
#endif

	pbuf[5] = ba_para & 0x00ff;
	pbuf[6] = (ba_para & 0xff00) >> 8;
	pbuf[7] = timeout & 0x00ff;
	pbuf[8] = (timeout & 0xff00) >> 8;

	txinsn.fr_len += _ADDBA_Rsp_Frame_Length_;

	SetFrameSubType((txinsn.phdr), WIFI_WMM_ACTION);
#ifdef CONFIG_IEEE80211W
	if (txinsn.isPMF)
		*(unsigned char*)(txinsn.phdr+1) |= BIT(6); // enable privacy
#endif
	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), da, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), BSSID, MACADDRLEN);

	DEBUG_INFO("ADDBA-rsp sent to AID %d, token %d TID %d size %d status %d\n",
		get_stainfo(priv, da)->aid, dialog_token, TID, max_size, status_code);
#if defined(WIFI_WMM)
	ret = check_dz_mgmt(priv, pstat, &txinsn);

	if (ret < 0)
		goto issue_ADDBArsp_fail;
	else if (ret==1)
		return SUCCESS;
	else
#endif
	if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS)
		return SUCCESS;

issue_ADDBArsp_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
	return FAIL;
}
#endif


#ifdef RTK_WOW
void issue_rtk_wow(struct rtl8192cd_priv *priv, unsigned char *da)
{
	unsigned char	*pbuf;
	unsigned int i;
	DECLARE_TXINSN(txinsn);

	if (!(OPMODE & WIFI_AP_STATE)) {
		DEBUG_WARN("rtk_wake_up pkt should be sent in AP mode!!\n");
		return;
	}

	txinsn.retry = priv->pmib->dot11OperationEntry.dot11ShortRetryLimit;
	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
    	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
	txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif

	txinsn.fixed_rate = 1;

	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);
	if (pbuf == NULL)
		goto send_rtk_wake_up_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);
	if (txinsn.phdr == NULL)
		goto send_rtk_wake_up_fail;

	memset((void *)(txinsn.phdr), 0, sizeof (struct	wlan_hdr));

	SetFrameSubType(txinsn.phdr, WIFI_DATA);
	SetFrDs(txinsn.phdr);
	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), da, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);

	// sync stream
	memset((void *)pbuf, 0xff, MACADDRLEN);
	pbuf += MACADDRLEN;
	txinsn.fr_len += MACADDRLEN;

	for(i=0; i<16; i++) {
		memcpy((void *)pbuf, da, MACADDRLEN);
		pbuf += MACADDRLEN;
		txinsn.fr_len += MACADDRLEN;
	}

	if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS) {
		DEBUG_INFO("RTK wake up pkt sent\n");
		return;
	}
	else {
		DEBUG_ERR("Fail to send RTK wake up pkt\n");
	}

send_rtk_wake_up_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
}
#endif




/**
 *	@brief	Process Site Survey
 *
 *	set site survery, reauth. , reassoc, idle_timer and proces Site survey \n
 *	PS: ss_timer is site survey timer	\n
 */
void start_clnt_ss(struct rtl8192cd_priv *priv)
{

    unsigned long	flags;

#ifdef SUPPORT_MULTI_PROFILE
    int j;
#endif


/*cfg p2p cfg p2p*/
    /*cfg p2p cfg p2p*/

#ifdef WIFI_WPAS_CLI
    printk("acli: start_ss_t ss_req:%d scanning:%d is_root:%d  \n",
    priv->ss_req_ongoing, priv->pshare->bScanInProcess, IS_ROOT_INTERFACE(priv));
#endif

    if (timer_pending(&priv->ss_timer))
        del_timer(&priv->ss_timer);
#ifdef CLIENT_MODE
    if (PENDING_REAUTH_TIMER)
        DELETE_REAUTH_TIMER;
    if (PENDING_REASSOC_TIMER)
        DELETE_REASSOC_TIMER;
    if (timer_pending(&priv->idle_timer))
        del_timer(&priv->idle_timer);
#endif


#ifdef SDIO_AP_OFFLOAD
	ap_offload_deactivate(priv, OFFLOAD_PROHIBIT_SITE_SURVEY);
#endif

#ifdef _OUTSRC_COEXIST
    if(IS_OUTSRC_CHIP(priv))
#endif
        priv->pshare->bScanInProcess = TRUE;

    OPMODE_VAL(OPMODE & (~WIFI_SITE_MONITOR));

    SAVE_INT_AND_CLI(flags);
    {
        {
        	//brian, only scan all channels one round, because Hostapd will scan 5 times by default
            priv->site_survey_times = SS_COUNT-1;
        }
    }

	if ( !priv->pshare->acs_for_adaptivity_flag )
		RTL_W8(TXPAUSE, RTL_R8(TXPAUSE) | STOP_BCN); //when start ss, disable beacon
    priv->site_survey->ss_channel = priv->available_chnl[0];


    //let vxd can do ss under 5G
    if (!priv->pmib->dot11DFSEntry.disable_DFS && is_DFS_channel(priv->site_survey->ss_channel))
        priv->pmib->dot11DFSEntry.disable_tx = 1;
    else
        priv->pmib->dot11DFSEntry.disable_tx = 0;
		if (IS_ROOT_INTERFACE(priv) ||
			(IS_VXD_INTERFACE(priv) && !GET_ROOT(priv)->pmib->dot11DFSEntry.CAC_ss_counter ))
		{

    if(priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_10)
        priv->pshare->CurrentChannelBW = HT_CHANNEL_WIDTH_10;
    else if(priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_5)
        priv->pshare->CurrentChannelBW = HT_CHANNEL_WIDTH_5;
    else
        priv->pshare->CurrentChannelBW = HT_CHANNEL_WIDTH_20;

#ifdef NHM_ACS2_SUPPORT
	if (((GET_CHIP_VER(priv) == VERSION_8192E)||(GET_CHIP_VER(priv) == VERSION_8197F)) && (priv->auto_channel == 1) && (priv->pmib->dot11RFEntry.acs_type == 2))
	{
		if (priv->pshare->CurrentChannelBW == HT_CHANNEL_WIDTH_20_40)
		{
			priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_ABOVE;
			priv->site_survey->to_scan_40M = 1;
			priv->site_survey->ss_channel = priv->available_chnl[0];
			priv->site_survey->start_ch_40M = priv->available_chnl[0];	//when scan 40M, record start ch
		}
		else if (priv->pshare->CurrentChannelBW == HT_CHANNEL_WIDTH_20)
		{
			priv->site_survey->to_scan_40M = 0;
		}

		ACSDEBUG("[CH:%d] [BW:%d] [Use40M:%d] [2ND-CHAN:%d] [to_scan_40M:%d]\n", priv->site_survey->ss_channel, priv->pshare->CurrentChannelBW, priv->pmib->dot11nConfigEntry.dot11nUse40M, priv->pshare->offset_2nd_chan, priv->site_survey->to_scan_40M);

		SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);
		SwChnl(priv, priv->site_survey->ss_channel, priv->pshare->offset_2nd_chan);
	}
	else
#endif
    {
        SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);
        SwChnl(priv, priv->site_survey->ss_channel, priv->pshare->offset_2nd_chan);
    }

    //by brian, trigger channel load evaluation after channel switched
    start_bbp_ch_load(priv, RTK80211_TIME_TO_SAMPLE_NUM);

    if (IS_VXD_INTERFACE(priv) && priv->pmib->wscEntry.wsc_enable)
        GET_ROOT(priv)->pmib->miscEntry.func_off = 1;
	}
	if (GET_ROOT(priv)->pmib->dot11DFSEntry.CAC_ss_counter){
		GET_ROOT(priv)->pmib->dot11DFSEntry.CAC_ss_counter--;
	}

    {
        priv->site_survey->count = 0;
        priv->site_survey->hidden_ap_found = 0;
        memset((void *)priv->site_survey->bss, 0, sizeof(struct bss_desc)*MAX_BSS_NUM);

#ifdef WIFI_SIMPLE_CONFIG
        if (priv->ss_req_ongoing == 2)
            memset((void *)priv->site_survey->wscie, 0, sizeof(struct wps_ie_info)*MAX_BSS_NUM);
#endif

        if (priv->ss_req_ongoing == 2)
            memset((void *)&priv->site_survey->wpa_ie, 0, sizeof(struct wpa_ie_info)*MAX_BSS_NUM);
        if (priv->ss_req_ongoing == 2)
            memset((void *)&priv->site_survey->rsn_ie, 0, sizeof(struct rsn_ie_info)*MAX_BSS_NUM);
        if (priv->ss_req_ongoing == 2)
            memset((void *)&priv->site_survey->rtk_p2p_ie, 0, sizeof(struct p2p_ie_info)*MAX_BSS_NUM);
    }



        RTL_W8(BCN_CTRL, RTL_R8(BCN_CTRL) | DIS_TSF_UPDATE_N);

#if defined(CLIENT_MODE)
    {
            RTL_W32(RCR, RTL_R32(RCR) & ~RCR_CBSSID_ADHOC);
    }
#endif

    DIG_for_site_survey(priv, TRUE);
    OPMODE_VAL(OPMODE | WIFI_SITE_MONITOR);
    RESTORE_INT(flags);


#ifdef CONFIG_RTL_NEW_AUTOCH
    if (priv->auto_channel == 1) {
        reset_FA_reg(priv);

        if (OPMODE & WIFI_AP_STATE)
            RTL_W32(RXERR_RPT, RXERR_RPT_RST);

		priv->auto_channel_step = 0;
		if (priv->pmib->dot11RFEntry.acs_type == 1) {
			if (IS_OUTSRC_CHIP(priv))
			{
				phydm_AutoChannelSelectSettingAP( ODMPTR, STORE_DEFAULT_NHM_SETTING, priv->auto_channel_step );
				phydm_AutoChannelSelectSettingAP( ODMPTR, ACS_NHM_SETTING, priv->auto_channel_step );
			}
			else
			{
				RTL_W8(0xc50, 0x3e);
				if (get_rf_mimo_mode(priv) != MIMO_1T1R)
					RTL_W8(0xc58, 0x3e);

				RTL_W16(0x896, 0x61a8);
				RTL_W16(0x892, 0xffff);
				RTL_W32(0x898, 0x82786e64);
				RTL_W32(0x89c, 0xffffff8c);
				PHY_SetBBReg(priv, 0xE28, bMaskByte0, 0xff);
				PHY_SetBBReg(priv, 0x890, BIT8|BIT9|BIT10, 3);
			}

			if ( !priv->pshare->acs_for_adaptivity_flag )
				RTL_W8(TXPAUSE, STOP_BCN);

			memset(priv->nhm_cnt, 0, sizeof(priv->nhm_cnt));
			priv->auto_channel_step = 1;
			// trigger NHM
			PHY_SetBBReg(priv, 0x890, BIT1, 0);
			PHY_SetBBReg(priv, 0x890, BIT1, 1);
		}
#ifdef NHM_ACS2_SUPPORT
		else if (priv->pmib->dot11RFEntry.acs_type == 2)
		{
			if ((GET_CHIP_VER(priv) == VERSION_8192E) || (GET_CHIP_VER(priv) == VERSION_8197F)) {
				priv->auto_channel_step = 1;
				priv->acs2_round_cn = 0;

				memset(priv->chnl_ss_fa_count, 0, sizeof(priv->chnl_ss_fa_count));
				memset(priv->chnl_ss_cca_count, 0, sizeof(priv->chnl_ss_cca_count));
				memset(priv->chnl_ss_ofdm_fa_count, 0, sizeof(priv->chnl_ss_ofdm_fa_count));
				memset(priv->chnl_ss_cck_fa_count, 0, sizeof(priv->chnl_ss_cck_fa_count));
				memset(priv->nhm_cnt_round, 0, sizeof(priv->nhm_cnt_round));
				memset(priv->clm_cnt, 0, sizeof(priv->clm_cnt));

				RTL_W8(TXPAUSE, STOP_BCN);

				int max_DIG_cover_bond = 0;
				int acs_IGI = 0; // related to max_DIG_cover_bond
				int dig_upper_bond = priv->pshare->rf_ft_var.dbg_dig_upper;
				int i;

				if (priv->pshare->rf_ft_var.dbg_dig_upper < priv->pshare->rf_ft_var.dbg_dig_lower)
				{
					dig_upper_bond = priv->pshare->rf_ft_var.dbg_dig_lower;
					printk("Caution!! dbg_dig_upper is lower than dbg_dig_lower. Fix dbg_dig_upper to dbg_dig_lower\n");
				}

				if (priv->pmib->dot11RFEntry.acs2_cca_cap_db > ACS_CCA_CAP_MAX)
				{
					priv->pmib->dot11RFEntry.acs2_cca_cap_db = ACS_CCA_CAP_MAX;
					printk("Caution!! acs2_cca_cap_db is larger than ACS_CCA_CAP_MAX. Fix acs2_cca_cap_db to ACS_CCA_CAP_MAX\n");
				}

				max_DIG_cover_bond = (dig_upper_bond - priv->pmib->dot11RFEntry.acs2_cca_cap_db);

				/* NHM */
				RTL_W16(0x896, 0xc350); 						// duration = 200ms, 4us per unit
				RTL_W16(0x892, 0xffff);							// th10,  th9
				//RTL_W32(0x898, 0x403a342e);						// th3, th2, th1, th0
				//RTL_W32(0x89c, 0x58524c46);						// th7, th6, th5, th4
				RTL_W32(0x898, 0xffffffff);						// th3, th2, th1, th0
				RTL_W32(0x89c, 0xffffffff);						// th7, th6, th5, th4

				if (max_DIG_cover_bond >= 0x26)
				{
					acs_IGI = max_DIG_cover_bond - MAX_UP_RESOLUTION;

					for (i = 0; i < 8; i++)
						RTL_W8(0x89f - i, (max_DIG_cover_bond * IGI_TO_NHM_TH_MULTIPLIER) - (MAX_UP_RESOLUTION * i));	//th7 ~ th0
				}
				else if (max_DIG_cover_bond > 0x20 && max_DIG_cover_bond < 0x26)
				{
					acs_IGI = 0x20;

					RTL_W8(0x89e, max_DIG_cover_bond * IGI_TO_NHM_TH_MULTIPLIER); // th6
					for (i = 0; i < 6; i++)
						RTL_W8(0x89d - i, (acs_IGI * IGI_TO_NHM_TH_MULTIPLIER) - (MAX_UP_RESOLUTION * i));	//th5 ~ th0
				}
				else
				{
					acs_IGI = max_DIG_cover_bond;

					for (i = 0; i < 6; i++)
						RTL_W8(0x89d - i, (max_DIG_cover_bond * IGI_TO_NHM_TH_MULTIPLIER) - (MAX_UP_RESOLUTION * i));	//th5 ~ th0
				}

				RTL_W8(0xc50, acs_IGI);
				if (get_rf_mimo_mode(priv) != MIMO_1T1R)
					RTL_W8(0xc58, acs_IGI);

				PHY_SetBBReg(priv, 0xE28, bMaskByte0, 0xff);	// th8
				PHY_SetBBReg(priv, 0x890, BIT8|BIT9|BIT10, 1);	// BIT8=1 "CCX_en", BIT9=1 "disable ignore CCA", BIT10=ignore_TXON
				/* trigger NHM */
				PHY_SetBBReg(priv, 0x890, BIT1, 0);
				PHY_SetBBReg(priv, 0x890, BIT1, 1);

				/* CLM */
				phydm_CLMInit(ODMPTR, CLM_SAMPLE_NUM);
				phydm_CLMtrigger(ODMPTR);
			}
			else if ((GET_CHIP_VER(priv) == VERSION_8812E) || (GET_CHIP_VER(priv) == VERSION_8822B))
			{
				priv->auto_channel_step = 1;
				priv->acs2_round_cn = 0;

				memset(priv->chnl_ss_fa_count, 0, sizeof(priv->chnl_ss_fa_count));
				memset(priv->chnl_ss_cca_count, 0, sizeof(priv->chnl_ss_cca_count));
				memset(priv->chnl_ss_ofdm_fa_count, 0, sizeof(priv->chnl_ss_ofdm_fa_count));
				memset(priv->chnl_ss_cck_fa_count, 0, sizeof(priv->chnl_ss_cck_fa_count));
				memset(priv->nhm_cnt_round, 0, sizeof(priv->nhm_cnt_round));
				memset(priv->clm_cnt, 0, sizeof(priv->clm_cnt));

				RTL_W8(TXPAUSE, STOP_BCN);

				priv->cck_th_backup = RTL_R8(0xa0a); // backup "cck th"
				RTL_W8(0xa0a, 0xcd);

				int max_DIG_cover_bond = 0;
				int acs_IGI = 0; // related to max_DIG_cover_bond
				int dig_upper_bond = priv->pshare->rf_ft_var.dbg_dig_upper;
				int i;

				if (priv->pshare->rf_ft_var.dbg_dig_upper < priv->pshare->rf_ft_var.dbg_dig_lower)
				{
					dig_upper_bond = priv->pshare->rf_ft_var.dbg_dig_lower;
					ACSDEBUG("Caution!! dbg_dig_upper is lower than dbg_dig_lower. Fix dbg_dig_upper to dbg_dig_lower\n");
				}

				if (priv->pmib->dot11RFEntry.acs2_cca_cap_db > ACS_CCA_CAP_MAX)
				{
					priv->pmib->dot11RFEntry.acs2_cca_cap_db = ACS_CCA_CAP_MAX;
					printk("Caution!! acs2_cca_cap_db is larger than ACS_CCA_CAP_MAX. Fix acs2_cca_cap_db to ACS_CCA_CAP_MAX\n");
				}

				max_DIG_cover_bond = (dig_upper_bond - priv->pmib->dot11RFEntry.acs2_cca_cap_db);

				/* NHM */
				RTL_W16(0x992, 0xc350); 						// period = 200ms, 4us per unit
				RTL_W16(0x996, 0xffff); 						// th10,  th9
				RTL_W32(0x998, 0xffffffff); 					// th3, th2, th1, th0
				RTL_W32(0x99c, 0xffffffff); 					// th7, th6, th5, th4

				if (max_DIG_cover_bond >= 0x26)
				{
					acs_IGI = max_DIG_cover_bond - MAX_UP_RESOLUTION;

					for (i = 0; i < 8; i++)
						RTL_W8(0x99f - i, (max_DIG_cover_bond * IGI_TO_NHM_TH_MULTIPLIER) - (MAX_UP_RESOLUTION * i));	//th7 ~ th0
				}
				else if (max_DIG_cover_bond > 0x20 && max_DIG_cover_bond < 0x26)
				{
					acs_IGI = 0x20;

					RTL_W8(0x99e, max_DIG_cover_bond * IGI_TO_NHM_TH_MULTIPLIER); // th6
					for (i = 0; i < 6; i++)
						RTL_W8(0x99d - i, (acs_IGI * IGI_TO_NHM_TH_MULTIPLIER) - (MAX_UP_RESOLUTION * i));	//th5 ~ th0
				}
				else
				{
					acs_IGI = max_DIG_cover_bond;

					for (i = 0; i < 6; i++)
						RTL_W8(0x99d - i, (max_DIG_cover_bond * IGI_TO_NHM_TH_MULTIPLIER) - (MAX_UP_RESOLUTION * i));	//th5 ~ th0
				}

				RTL_W8(0xc50, acs_IGI);
				if (get_rf_mimo_mode(priv) != MIMO_1T1R)
					RTL_W8(0xe50, acs_IGI);

				PHY_SetBBReg(priv, 0x9a0, bMaskByte0, 0xff);	// th8
				PHY_SetBBReg(priv, 0x994, BIT8|BIT9|BIT10, 1);	// BIT8=1 "CCX_en", BIT9=1 "disable ignore CCA", BIT10=ignore_TXON
				/* trigger NHM */
				PHY_SetBBReg(priv, 0x994, BIT1, 0);
				PHY_SetBBReg(priv, 0x994, BIT1, 1);

				/* CLM */
				phydm_CLMInit(ODMPTR, CLM_SAMPLE_NUM);
				phydm_CLMtrigger(ODMPTR);
			}
		}
#endif
    }
#endif




    if( (priv->site_survey->hidden_ap_found == HIDE_AP_FOUND_DO_ACTIVE_SSAN ) ||
        !is_passive_channel(priv, priv->pmib->dot11StationConfigEntry.dot11RegDomain, priv->site_survey->ss_channel))
    {

        {
            /*no assigned SSID*/
            if (priv->ss_ssidlen == 0){

                {
                    //STADEBUG("issue_probereq no assigned SSID\n");
                    if (!priv->auto_channel_step) {
						issue_probereq(priv, NULL, 0, NULL);
					}
                }

            }else{   /*has assigned SSID*/
                #ifdef SUPPORT_MULTI_PROFILE	/*per channel tx multi probe_req by profile_num*/
                if (priv->pmib->ap_profile.enable_profile && priv->pmib->ap_profile.profile_num > 0) {
                    for(j=0;j<priv->pmib->ap_profile.profile_num;j++) {
                        NDEBUG3("issue_probereq,ssid[%s],ch=[%d]\n",priv->pmib->ap_profile.profile[j].ssid,priv->site_survey->ss_channel);
                        issue_probereq(priv, priv->pmib->ap_profile.profile[j].ssid, strlen(priv->pmib->ap_profile.profile[j].ssid), NULL);
                    }
                }
                else
                #endif
                {
                    NDEBUG3("issue_probereq,ssid[%s],ch[%d]\n",priv->ss_ssid,priv->site_survey->ss_channel);
                    issue_probereq(priv, priv->ss_ssid, priv->ss_ssidlen, NULL);
                }

            }
        }
    }



    /*how long stady on current channel -start*/
    if(is_passive_channel(priv, priv->pmib->dot11StationConfigEntry.dot11RegDomain, priv->site_survey->ss_channel))
    {
        if(priv->pmib->miscEntry.passive_ss_int) {
            mod_timer(&priv->ss_timer, jiffies + RTL_MILISECONDS_TO_JIFFIES(priv->pmib->miscEntry.passive_ss_int)
            );
        } else {
            mod_timer(&priv->ss_timer, jiffies + SS_PSSV_TO
            );
        }
    }else
    {
        #ifdef CONFIG_RTL_NEW_AUTOCH
        if (priv->auto_channel == 1){
            	if (priv->auto_channel_step) {
			if (priv->pmib->dot11RFEntry.acs_type == 1)
				mod_timer(&priv->ss_timer, jiffies + SS_AUTO_CHNL_NHM_TO);
#ifdef NHM_ACS2_SUPPORT
			else if (priv->pmib->dot11RFEntry.acs_type == 2)
				mod_timer(&priv->ss_timer, jiffies + SS_AUTO_CHNL_ACS2_TO);
#endif
			else
				printk("Error @ %s : wrong acs_type (%d)\n", __FUNCTION__, priv->pmib->dot11RFEntry.acs_type);
		}
		else {
			mod_timer(&priv->ss_timer, jiffies + SS_AUTO_CHNL_TO
            	);
		}
        }else
        #endif
        {
            mod_timer(&priv->ss_timer, jiffies + SS_TO
            );
        }

	}
    /*how long stady on current channel -end*/


}

static void ProfileSort (struct rtl8192cd_priv *priv, int CompareType , void  *base, int nel, int width)
{
	int wgap, i, j, k;
	unsigned char tmp;

	if ((nel > 1) && (width > 0)) {
		//assert( nel <= ((size_t)(-1)) / width ); /* check for overflow */
		wgap = 0;
		do {
			wgap = 3 * wgap + 1;
		} while (wgap < (nel-1)/3);

        /* From the above, we know that either wgap == 1 < nel or */
		/* ((wgap-1)/3 < (int) ((nel-1)/3) <= (nel-1)/3 ==> wgap <  nel. */

		wgap *= width;			/* So this can not overflow if wnel doesn't. */
		nel *= width;			/* Convert nel to 'wnel' */
		do {
			i = wgap;
			do {
				j = i;
				do {
					register unsigned char *a;
					register unsigned char *b;

					j -= wgap;
					a = (unsigned char *)(j + ((char *)base));
					b = a + wgap;
					if ( compareTpyeByProfile(priv,a, b,CompareType) <= 0 ) {
						break;
					}
					k = width;
					do {
						tmp = *a;
						*a++ = *b;
						*b++ = tmp;
					} while ( --k );
				} while (j >= wgap);
				i += width;
			} while (i < nel);
			wgap = (wgap - width)/3;
		} while (wgap);
	}
}

#ifdef CONFIG_IEEE80211V_CLI
void qsort (void  *base, int nel, int width,
				int (*comp)(const void *, const void *))
#else
static void qsort (void  *base, int nel, int width,
				int (*comp)(const void *, const void *))
#endif
{
	int wgap, i, j, k;
	unsigned char tmp;

	if ((nel > 1) && (width > 0)) {
		//assert( nel <= ((size_t)(-1)) / width ); /* check for overflow */
		wgap = 0;
		do {
			wgap = 3 * wgap + 1;
		} while (wgap < (nel-1)/3);
		/* From the above, we know that either wgap == 1 < nel or */
		/* ((wgap-1)/3 < (int) ((nel-1)/3) <= (nel-1)/3 ==> wgap <  nel. */
		wgap *= width;			/* So this can not overflow if wnel doesn't. */
		nel *= width;			/* Convert nel to 'wnel' */
		do {
			i = wgap;
			do {
				j = i;
				do {
					register unsigned char *a;
					register unsigned char *b;

					j -= wgap;
					a = (unsigned char *)(j + ((char *)base));
					b = a + wgap;
					if ( (*comp)(a, b) <= 0 ) {
						break;
					}
					k = width;
					do {
						tmp = *a;
						*a++ = *b;
						*b++ = tmp;
					} while ( --k );
				} while (j >= wgap);
				i += width;
			} while (i < nel);
			wgap = (wgap - width)/3;
		} while (wgap);
	}
}


static int get_profile_index(struct rtl8192cd_priv *priv ,char* SSID2Search)
{
     int idx=0;
     int len1=0;
     int len2=0;
     len1 = strlen(SSID2Search);


     for(idx=0 ; idx < priv->pmib->ap_profile.profile_num ; idx++){
         len2 = strlen(priv->pmib->ap_profile.profile[idx].ssid);
         if(len1==len2){
            if(!strcmp(priv->pmib->ap_profile.profile[idx].ssid,SSID2Search)){
                return idx;
            }
         }
     }
     return -1;

}
static int compareTpyeByProfile(struct rtl8192cd_priv *priv , const void *entry1, const void *entry2 , int CompareType)
{
    int result1=0;
    int result2=0;
    switch(CompareType){
        case COMPARE_BSS:
                result1=get_profile_index(priv,((struct bss_desc *)entry1)->ssid);
                result2=get_profile_index(priv,((struct bss_desc *)entry2)->ssid);
                break;
        case COMPARE_WSCIE:
                result1=get_profile_index(priv,((struct wps_ie_info *)entry1)->ssid);
                result2=get_profile_index(priv,((struct wps_ie_info *)entry2)->ssid);
                break;
#ifdef WIFI_WPAS
        case COMPARE_WPAIE:
                result1=get_profile_index(priv,((struct wpa_ie_info *)entry1)->ssid);
                result2=get_profile_index(priv,((struct wpa_ie_info *)entry2)->ssid);
                break;
        case COMPARE_RSNIE:
                result1=get_profile_index(priv,((struct rsn_ie_info *)entry1)->ssid);
                result2=get_profile_index(priv,((struct rsn_ie_info *)entry2)->ssid);
                break;
#endif
        default:
            STADEBUG("unknow, check!!!\n\n");
    }

	/*result more small then list at more front*/
    if (  result1 < result2 )
        return -1;

    if (  result1 > result2 )
        return 1;

    return 0;

}

static int compareBSS(const void *entry1, const void *entry2)
{
	if (((struct bss_desc *)entry1)->rssi > ((struct bss_desc *)entry2)->rssi)
		return -1;

	if (((struct bss_desc *)entry1)->rssi < ((struct bss_desc *)entry2)->rssi)
		return 1;

	return 0;
}


#ifdef WIFI_SIMPLE_CONFIG
static int compareBSS_for_2G(const void *entry1, const void *entry2)
{
	if (((struct bss_desc *)entry1)->channel <= 14 && ((struct bss_desc *)entry2)->channel > 14)
		return -1;

	if (((struct bss_desc *)entry1)->channel > 14 && ((struct bss_desc *)entry2)->channel <= 14)
		return 1;

	if (((struct bss_desc *)entry1)->rssi > ((struct bss_desc *)entry2)->rssi)
		return -1;

	if (((struct bss_desc *)entry1)->rssi < ((struct bss_desc *)entry2)->rssi)
		return 1;

	return 0;
}

static int compareWpsIE(const void *entry1, const void *entry2)
{
	if (((struct wps_ie_info *)entry1)->rssi > ((struct wps_ie_info *)entry2)->rssi)
		return -1;

	if (((struct wps_ie_info *)entry1)->rssi < ((struct wps_ie_info *)entry2)->rssi)
		return 1;

	return 0;
}

static int compareWpsIE_for_2G(const void *entry1, const void *entry2)
{
	if (((struct wps_ie_info *)entry1)->chan <= 14 && ((struct wps_ie_info *)entry2)->chan > 14	)
		return -1;

	if (((struct wps_ie_info *)entry1)->chan > 14 && ((struct wps_ie_info *)entry2)->chan <= 14	)
		return 1;

	if (((struct wps_ie_info *)entry1)->rssi > ((struct wps_ie_info *)entry2)->rssi)
		return -1;

	if (((struct wps_ie_info *)entry1)->rssi < ((struct wps_ie_info *)entry2)->rssi)
		return 1;

	return 0;
}
#endif

static int compareWpaIE(const void *entry1, const void *entry2)
{
	if (((struct wpa_ie_info *)entry1)->rssi > ((struct wpa_ie_info *)entry2)->rssi)
		return -1;

	if (((struct wpa_ie_info *)entry1)->rssi < ((struct wpa_ie_info *)entry2)->rssi)
		return 1;

	return 0;
}

static int compareRsnIE(const void *entry1, const void *entry2)
{
	if (((struct rsn_ie_info *)entry1)->rssi > ((struct rsn_ie_info *)entry2)->rssi)
		return -1;

	if (((struct rsn_ie_info *)entry1)->rssi < ((struct rsn_ie_info *)entry2)->rssi)
		return 1;

	return 0;
}

static int compareP2PIE(const void *entry1, const void *entry2)
{
	if (((struct p2p_ie_info *)entry1)->rssi > ((struct p2p_ie_info *)entry2)->rssi)
		return -1;

	if (((struct p2p_ie_info *)entry1)->rssi < ((struct p2p_ie_info *)entry2)->rssi)
		return 1;

	return 0;
}

static void debug_print_bss(struct rtl8192cd_priv *priv)
{
#if 0
	STADEBUG("Got ssid count %d\n", priv->site_survey->count);

	int i;

	panic_printk("Got ssid count %d\n", priv->site_survey->count);
	panic_printk("SSID                 BSSID        ch  prd cap  bsc  oper ss sq bd 40m\n");
	for(i=0; i<priv->site_survey->count; i++)
	{
		char tmpbuf[33];
		UINT8 *mac = priv->site_survey->bss[i].bssid;

		memcpy(tmpbuf, priv->site_survey->bss[i].ssid, priv->site_survey->bss[i].ssidlen);
		if (priv->site_survey->bss[i].ssidlen < 20) {
			memset(tmpbuf+priv->site_survey->bss[i].ssidlen, ' ', 20-priv->site_survey->bss[i].ssidlen);
			tmpbuf[20] = '\0';
		}
		else
			tmpbuf[priv->site_survey->bss[i].ssidlen] = '\0';

		panic_printk("%s %02x%02x%02x%02x%02x%02x %2d %4d %04x %04x %04x %02x %02x %02x %3d\n",
			tmpbuf,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],priv->site_survey->bss[i].channel,
			priv->site_survey->bss[i].beacon_prd,priv->site_survey->bss[i].capability,
			(unsigned short)priv->site_survey->bss[i].basicrate,
			(unsigned short)priv->site_survey->bss[i].supportrate,
			priv->site_survey->bss[i].rssi,priv->site_survey->bss[i].sq,
			priv->site_survey->bss[i].network,
			((priv->site_survey->bss[i].t_stamp[1] & BIT(1)) ? 1 : 0)
			);
	}
    panic_printk("\n\n");
#endif
}


void rtl8192cd_ss_timer(unsigned long task_priv)
{
	struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)task_priv;
	int idx, loop_finish=0;
	int AUTOCH_SS_COUNT=SS_COUNT;//AUTOCH_SS_SPEEDUP
	int i,j;

#ifdef SUPPORT_MULTI_PROFILE
	int jdx;
#endif
	if (!(priv->drv_state & DRV_STATE_OPEN))
		return;

#ifndef WIFI_WPAS_CLI
#endif
/*cfg p2p cfg p2p*/


    SMP_LOCK(flags);

    /*cfg p2p cfg p2p*/

    STADEBUG("rtl8192cd_ss_timer,ss_channel=%d\n",priv->site_survey->ss_channel);



	for (idx=0; idx<priv->available_chnl_num; idx++)
		if (priv->site_survey->ss_channel == priv->available_chnl[idx])
			break;

	//by brian, collect channel statistic here, move from 8192cd_expire_timer
	{
		priv->rtk->survey_info[idx].channel = priv->site_survey->ss_channel;
		unsigned int val = 0;
		val=read_bbp_ch_load(priv);
		if(val) {
			priv->rtk->survey_info[idx].chbusytime = RTK80211_SAMPLE_NUM_TO_TIME(val);
		} else {
			priv->rtk->survey_info[idx].chbusytime = 0;
			NDEBUG("Invalid channel load!\n");
		}

		check_sta_throughput(priv, idx);
		read_noise_report(priv, idx);
	}

#ifdef CONFIG_RTL_NEW_AUTOCH
#ifdef AUTOCH_SS_SPEEDUP
	if (priv->auto_channel == 1)
		if(priv->pmib->miscEntry.autoch_ss_cnt>0)
			AUTOCH_SS_COUNT = priv->pmib->miscEntry.autoch_ss_cnt;
#endif
	if (priv->auto_channel == 1) {
		unsigned int ofdm_ok, cck_ok, ht_ok;
#ifdef NHM_ACS2_SUPPORT
		if (((GET_CHIP_VER(priv) == VERSION_8192E)||(GET_CHIP_VER(priv) == VERSION_8197F)) && (priv->pmib->dot11RFEntry.acs_type == 2) && (priv->auto_channel_step == 1))
		{
			ACSDEBUG("[CH:%d] [BW:%d] [Use40M:%d] [2ND-CHAN:%d] [to_scan_40M:%d] [RF0x18:%d]\n", priv->site_survey->ss_channel, priv->pshare->CurrentChannelBW, priv->pmib->dot11nConfigEntry.dot11nUse40M, priv->pshare->offset_2nd_chan, priv->site_survey->to_scan_40M, PHY_QueryRFReg(priv, RF92CD_PATH_A, 0x18, 0xff, 1));
			priv->acs2_round_cn++;

			hold_CCA_FA_counter(priv);
			_FA_statistic(priv);
			priv->chnl_ss_fa_count[idx] += priv->pshare->FA_total_cnt;
			priv->chnl_ss_cck_fa_count[idx] += priv->pshare->cck_FA_cnt;
			priv->chnl_ss_ofdm_fa_count[idx] += (priv->pshare->FA_total_cnt - priv->pshare->cck_FA_cnt);
			priv->chnl_ss_cca_count[idx] += ((RTL_R8(0xa60)<<8)|RTL_R8(0xa61)) + RTL_R16(0xda0);

			release_CCA_FA_counter(priv);
		}
		else if (((GET_CHIP_VER(priv) == VERSION_8812E)||(GET_CHIP_VER(priv) == VERSION_8822B)) && (priv->pmib->dot11RFEntry.acs_type == 2) && (priv->auto_channel_step == 1))
		{
			ACSDEBUG("[CH:%d] [BW:%d] [Use40M:%d] [2ND-CHAN:%d] [to_scan_40M:%d] [RF0x18:%d] [idx:%d]\n", priv->site_survey->ss_channel, priv->pshare->CurrentChannelBW, priv->pmib->dot11nConfigEntry.dot11nUse40M, priv->pshare->offset_2nd_chan, priv->site_survey->to_scan_40M, PHY_QueryRFReg(priv, RF92CD_PATH_A, 0x18, 0xff, 1), idx);
			priv->acs2_round_cn++;

			odm_FalseAlarmCounterStatistics(ODMPTR);
			priv->chnl_ss_fa_count[idx] = ODMPTR->FalseAlmCnt.Cnt_all;
			priv->chnl_ss_cck_fa_count[idx] += ODMPTR->FalseAlmCnt.Cnt_Cck_fail;
			priv->chnl_ss_ofdm_fa_count[idx] += ODMPTR->FalseAlmCnt.Cnt_Ofdm_fail;
			priv->chnl_ss_cca_count[idx] = ODMPTR->FalseAlmCnt.Cnt_CCA_all;
		}
		else
#endif
		{
		if (!priv->site_survey->to_scan_40M && (priv->auto_channel_step <= 1)) {
#ifdef _OUTSRC_COEXIST
			if (IS_OUTSRC_CHIP(priv))//outsrc chip use odemFAStatistic
#endif
			{
				odm_FalseAlarmCounterStatistics(ODMPTR);
				priv->chnl_ss_fa_count[idx] = ODMPTR->FalseAlmCnt.Cnt_all;
				priv->chnl_ss_cca_count[idx] = ODMPTR->FalseAlmCnt.Cnt_CCA_all;
			}
#if !defined(USE_OUT_SRC) || defined(_OUTSRC_COEXIST)
#ifdef _OUTSRC_COEXIST
			if (!IS_OUTSRC_CHIP(priv))
#endif
			{
				hold_CCA_FA_counter(priv);
				_FA_statistic(priv);
				priv->chnl_ss_fa_count[idx] = priv->pshare->FA_total_cnt;
				priv->chnl_ss_cca_count[idx] = ((RTL_R8(0xa60)<<8)|RTL_R8(0xa61)) + RTL_R16(0xda0);

				release_CCA_FA_counter(priv);
			}
 #endif // !USE_OUT_SRC || _OUTSRC_COEXIST
		}
		}
		RTL_W32(RXERR_RPT, 0 << RXERR_RPT_SEL_SHIFT);
		ofdm_ok = RTL_R16(RXERR_RPT);

		RTL_W32(RXERR_RPT, 3 << RXERR_RPT_SEL_SHIFT);
		cck_ok = RTL_R16(RXERR_RPT);

		RTL_W32(RXERR_RPT, 6 << RXERR_RPT_SEL_SHIFT);
		ht_ok = RTL_R16(RXERR_RPT);

		RTL_W32(RXERR_RPT, RXERR_RPT_RST);

		if (priv->site_survey->to_scan_40M) {
			unsigned int z=0, ch_begin=0, ch_end=priv->available_chnl_num,
				current_ch = PHY_QueryRFReg(priv, RF92CD_PATH_A, 0x18, 0xff, 1);
			int idx_2G_end=-1;
#if defined(RTK_5G_SUPPORT)
			int idx_5G_begin=-1;
			if (priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_5G) {
				for (z=0; z<priv->available_chnl_num; z++) {
					if (priv->available_chnl[z] > 14) {
						idx_5G_begin = z;
						break;
					}
				}
				if (idx_5G_begin >= 0)
					ch_begin = idx_5G_begin;

				for (z=ch_begin; z < ch_end; z++) {
					if ((priv->available_chnl[z] == (current_ch+2)) || (priv->available_chnl[z] == (current_ch-2))) {
						priv->chnl_ss_mac_rx_count_40M[z] = ofdm_ok + cck_ok + ht_ok;
					}
				}
			} else
#endif
			{
				for (z=0; z<priv->available_chnl_num; z++) {
					if (priv->available_chnl[z] <= 14)
						idx_2G_end = z;
					else
						break;
				}
				if (idx_2G_end >= 0)
					ch_end = idx_2G_end+1;

				for (z=ch_begin; z < ch_end; z++) {
					if (priv->available_chnl[z] == current_ch) {
						priv->chnl_ss_mac_rx_count_40M[z] = ofdm_ok + cck_ok + ht_ok;
						break;
					}
				}
			}
		} else {
			priv->chnl_ss_mac_rx_count[idx] = ofdm_ok + cck_ok + ht_ok;
		}

#ifdef NHM_ACS2_SUPPORT
		if (priv->pmib->dot11RFEntry.acs_type == 2)
		{
			unsigned long val32 = 0;
			if ((GET_CHIP_VER(priv) == VERSION_8192E||GET_CHIP_VER(priv) == VERSION_8197F) && priv->auto_channel_step)
			{
				for (i=0; i<20; i++)
				{
					mdelay(1);
					if ( phydm_checkCLMready(ODMPTR))
						break;
				}

				//if (priv->acs2_round_cn > 0 && priv->acs2_round_cn <= priv->pmib->dot11RFEntry.acs2_round)
				{
					priv->nhm_cnt_round[0][idx][10] += RTL_R8(0x8d6);
					priv->nhm_cnt_round[0][idx][9] += RTL_R8(0x8d3);
					priv->nhm_cnt_round[0][idx][8] += RTL_R8(0x8d2);
					val32 = RTL_R32(0x8dc);
					priv->nhm_cnt_round[0][idx][7] += (val32 & 0xff000000) >> 24;
					priv->nhm_cnt_round[0][idx][6] += (val32 & 0x00ff0000) >> 16;
					priv->nhm_cnt_round[0][idx][5] += (val32 & 0x0000ff00) >> 8;
					priv->nhm_cnt_round[0][idx][4] += (val32 & 0x000000ff);
					val32 = RTL_R32(0x8d8);
					priv->nhm_cnt_round[0][idx][3] += (val32 & 0xff000000) >> 24;
					priv->nhm_cnt_round[0][idx][2] += (val32 & 0x00ff0000) >> 16;
					priv->nhm_cnt_round[0][idx][1] += (val32 & 0x0000ff00) >> 8;
					priv->nhm_cnt_round[0][idx][0] += (val32 & 0x000000ff);
				}

				priv->clm_cnt[idx] += phydm_getCLMresult(ODMPTR);

				ACSDEBUG("[ACS2-%s], REG0x898=%x, REG0x8f8=%x, REG0x892=%x, REG0xc50=%x, REG0xc58=%x\n", __FUNCTION__, RTL_R32(0x898), RTL_R16(0x8f8), RTL_R16(0x892), RTL_R8(0xc50), RTL_R8(0xc58));
				ACSDEBUG("[ACS2-%s], REG0x89c=%x, REG0xe28=%x, REG0x890=%x, REG0xa0a=%x\n\n", __FUNCTION__, RTL_R32(0x89c), RTL_R16(0xe28), RTL_R16(0x890), RTL_R8(0xa0a));
			}
			else if ((GET_CHIP_VER(priv) == VERSION_8812E||GET_CHIP_VER(priv) == VERSION_8822B) && priv->auto_channel_step)
			{
				for (i=0; i<20; i++)
				{
					mdelay(1);
					if (phydm_checkCLMready(ODMPTR))
						break;
				}

				//if (priv->acs2_round_cn > 0 && priv->acs2_round_cn <= priv->pmib->dot11RFEntry.acs2_round)
				{
					priv->nhm_cnt_round[0][idx][10] += RTL_R8(0xfb2);
					priv->nhm_cnt_round[0][idx][9] += RTL_R8(0xfb1);
					priv->nhm_cnt_round[0][idx][8] += RTL_R8(0xfb0);
					val32 = RTL_R32(0xfac);
					priv->nhm_cnt_round[0][idx][7] += (val32 & 0xff000000) >> 24;
					priv->nhm_cnt_round[0][idx][6] += (val32 & 0x00ff0000) >> 16;
					priv->nhm_cnt_round[0][idx][5] += (val32 & 0x0000ff00) >> 8;
					priv->nhm_cnt_round[0][idx][4] += (val32 & 0x000000ff);
					val32 = RTL_R32(0xfa8);
					priv->nhm_cnt_round[0][idx][3] += (val32 & 0xff000000) >> 24;
					priv->nhm_cnt_round[0][idx][2] += (val32 & 0x00ff0000) >> 16;
					priv->nhm_cnt_round[0][idx][1] += (val32 & 0x0000ff00) >> 8;
					priv->nhm_cnt_round[0][idx][0] += (val32 & 0x000000ff);
				}

				priv->clm_cnt[idx] += phydm_getCLMresult(ODMPTR);

				ACSDEBUG("[ACS2-%s], REG0x998=%x, REG0x8f8=%x, REG0x996=%x, REG0xc50=%x, REG0xe50=%x\n", __FUNCTION__, RTL_R32(0x998), RTL_R16(0x8f8), RTL_R16(0x996), RTL_R8(0xc50), RTL_R8(0xe50));
				ACSDEBUG("[ACS2-%s], REG0x99c=%x, REG0x9a0=%x, REG0x994=%x, REG0xa0a=%x\n\n", __FUNCTION__, RTL_R32(0x99c), RTL_R16(0x9a0), RTL_R16(0x994), RTL_R8(0xa0a));
			}
		}
		else
#endif
		if (!priv->site_survey->to_scan_40M) {
			if (priv->pmib->dot11RFEntry.acs_type && priv->auto_channel_step) {
				if (IS_OUTSRC_CHIP(priv))
				{
					DEBUG_INFO("phydm_GetNHMStatisticsAP: auto_channel_step=%d, idx=%d\n", priv->auto_channel_step, idx);
		            phydm_GetNHMStatisticsAP( ODMPTR, idx, priv->auto_channel_step );    // @ 2G, Real channel number = idx+1

		            if( (idx == (priv->available_chnl_num - 1)) && (priv->auto_channel_step==2) )
					{
		                for(i=0; i<=10; i++)                  // channel index
		                    for(j=0; j<=9; j++)               // NHM index
		                        priv->nhm_cnt[i][j]=ODMPTR->DM_ACS.NHM_Cnt[i][j];

		                phydm_AutoChannelSelectSettingAP( ODMPTR, RESTORE_DEFAULT_NHM_SETTING, priv->auto_channel_step );
		            }
				}
				else
				{
					unsigned long val32 = 0;
		            for (i=0; i<20; i++)
					{
						mdelay(1);
						if (RTL_R8(0x8b6) & BIT1)
							break;
					}

					if (priv->auto_channel_step == 1) {
						val32 = RTL_R32(0x8dc);
						priv->nhm_cnt[idx][9] = (val32 & 0x0000ff00) >> 8;
						priv->nhm_cnt[idx][8] = (val32 & 0x000000ff);
						val32 = RTL_R32(0x8d8);
						priv->nhm_cnt[idx][7] = (val32 & 0xff000000) >> 24;
						priv->nhm_cnt[idx][6] = (val32 & 0x00ff0000) >> 16;
						priv->nhm_cnt[idx][5] = (val32 & 0x0000ff00) >> 8;
					} else if (priv->auto_channel_step == 2) {
						val32 = RTL_R32(0x8d8);
						priv->nhm_cnt[idx][4] = RTL_R8(0x8dc);
						priv->nhm_cnt[idx][3] = (val32 & 0xff000000) >> 24;
						priv->nhm_cnt[idx][2] = (val32 & 0x00ff0000) >> 16;
						priv->nhm_cnt[idx][1] = (val32 & 0x0000ff00) >> 8;
						priv->nhm_cnt[idx][0] = (val32 & 0x000000ff);
					}
				}
			}
		}
	}
#endif          // #ifdef CONFIG_RTL_NEW_AUTOCH


	int loop_flag = 1;
#ifdef NHM_ACS2_SUPPORT
	if (((GET_CHIP_VER(priv) == VERSION_8192E)||(GET_CHIP_VER(priv) == VERSION_8197F) || (GET_CHIP_VER(priv) == VERSION_8812E)||(GET_CHIP_VER(priv) == VERSION_8822B)) && (priv->auto_channel == 1) && (priv->pmib->dot11RFEntry.acs_type == 2))
	{
		if (priv->acs2_round_cn != priv->pmib->dot11RFEntry.acs2_round)
			loop_flag = 0;
	}
#endif

    if (idx == (priv->available_chnl_num - 1) &&
        priv->site_survey->hidden_ap_found != HIDE_AP_FOUND && loop_flag) {
        loop_finish = 1;
    }
    else {
        if (priv->site_survey->hidden_ap_found != HIDE_AP_FOUND) {
            if(priv->site_survey->defered_ss) {
                priv->site_survey->defered_ss--;
            } else
            {
#ifdef NHM_ACS2_SUPPORT
			if ((GET_CHIP_VER(priv) == VERSION_8192E ||GET_CHIP_VER(priv) == VERSION_8197F)&& (priv->auto_channel == 1) && (priv->pmib->dot11RFEntry.acs_type == 2))
			{
				if (priv->acs2_round_cn == priv->pmib->dot11RFEntry.acs2_round) // for round1 setting
				{
					priv->acs2_round_cn = 0;
					priv->site_survey->ss_channel = priv->available_chnl[idx+1];
					if (priv->site_survey->to_scan_40M && (priv->site_survey->ss_channel >= 5))
						priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_BELOW;
				}

					int i;
					int max_DIG_cover_bond = 0;
					int acs_IGI = 0; // related to max_DIG_cover_bond
					int dig_upper_bond = priv->pshare->rf_ft_var.dbg_dig_upper;

					if (priv->pshare->rf_ft_var.dbg_dig_upper < priv->pshare->rf_ft_var.dbg_dig_lower)
					{
						dig_upper_bond = priv->pshare->rf_ft_var.dbg_dig_lower;
						printk("Caution!! dbg_dig_upper is lower than dbg_dig_lower. Fix dbg_dig_upper to dbg_dig_lower\n");
					}

					if (priv->pmib->dot11RFEntry.acs2_cca_cap_db > ACS_CCA_CAP_MAX)
					{
						priv->pmib->dot11RFEntry.acs2_cca_cap_db = ACS_CCA_CAP_MAX;
						printk("Caution!! acs2_cca_cap_db is larger than ACS_CCA_CAP_MAX. Fix acs2_cca_cap_db to ACS_CCA_CAP_MAX\n");
					}

					max_DIG_cover_bond = (dig_upper_bond - priv->pmib->dot11RFEntry.acs2_cca_cap_db);

					/* NHM */
					RTL_W16(0x896, 0xc350); 						// duration = 200ms, 4us per unit
					RTL_W16(0x892, 0xffff); 						// th10,  th9
					//RTL_W32(0x898, 0x403a342e);						// th3, th2, th1, th0
					//RTL_W32(0x89c, 0x58524c46);						// th7, th6, th5, th4
					RTL_W32(0x898, 0xffffffff); 					// th3, th2, th1, th0
					RTL_W32(0x89c, 0xffffffff); 					// th7, th6, th5, th4
					if (max_DIG_cover_bond >= 0x26)
					{
						acs_IGI = max_DIG_cover_bond - MAX_UP_RESOLUTION;
						for (i = 0; i < 8; i++)
							RTL_W8(0x89f - i, (max_DIG_cover_bond * IGI_TO_NHM_TH_MULTIPLIER) - (MAX_UP_RESOLUTION * i));	//th7 ~ th0
					}
					else if (max_DIG_cover_bond > 0x20 && max_DIG_cover_bond < 0x26)
					{
						acs_IGI = 0x20;
						RTL_W8(0x89e, max_DIG_cover_bond * IGI_TO_NHM_TH_MULTIPLIER); // th6
						for (i = 0; i < 6; i++)
							RTL_W8(0x89d - i, (acs_IGI * IGI_TO_NHM_TH_MULTIPLIER) - (MAX_UP_RESOLUTION * i));	//th5 ~ th0
					}
					else
					{
						acs_IGI = max_DIG_cover_bond;
						for (i = 0; i < 6; i++)
							RTL_W8(0x89d - i, (max_DIG_cover_bond * IGI_TO_NHM_TH_MULTIPLIER) - (MAX_UP_RESOLUTION * i));	//th5 ~ th0
					}

					RTL_W8(0xc50, acs_IGI);
					if (get_rf_mimo_mode(priv) != MIMO_1T1R)
						RTL_W8(0xc58, acs_IGI);

					PHY_SetBBReg(priv, 0xE28, bMaskByte0, 0xff);	// th8
					PHY_SetBBReg(priv, 0x890, BIT8|BIT9|BIT10, 1);	// BIT8=1 "CCX_en", BIT9=1 "disable ignore CCA", BIT10=ignore_TXON
			}
			else if ((GET_CHIP_VER(priv) == VERSION_8812E||GET_CHIP_VER(priv) == VERSION_8822B) && (priv->auto_channel == 1) && (priv->pmib->dot11RFEntry.acs_type == 2))
			{
					if (priv->acs2_round_cn == priv->pmib->dot11RFEntry.acs2_round) // for round1 setting
					{
						priv->acs2_round_cn = 0;
						priv->site_survey->ss_channel = priv->available_chnl[idx+1];
					}

					int i;
					int max_DIG_cover_bond = 0;
					int acs_IGI = 0; // related to max_DIG_cover_bond
					int dig_upper_bond = priv->pshare->rf_ft_var.dbg_dig_upper;

					if (priv->pshare->rf_ft_var.dbg_dig_upper < priv->pshare->rf_ft_var.dbg_dig_lower)
					{
						dig_upper_bond = priv->pshare->rf_ft_var.dbg_dig_lower;
						printk("Caution!! dbg_dig_upper is lower than dbg_dig_lower. Fix dbg_dig_upper to dbg_dig_lower\n");
					}

					if (priv->pmib->dot11RFEntry.acs2_cca_cap_db > ACS_CCA_CAP_MAX)
					{
						priv->pmib->dot11RFEntry.acs2_cca_cap_db = ACS_CCA_CAP_MAX;
						printk("Caution!! acs2_cca_cap_db is larger than ACS_CCA_CAP_MAX. Fix acs2_cca_cap_db to ACS_CCA_CAP_MAX\n");
					}

					max_DIG_cover_bond = (dig_upper_bond - priv->pmib->dot11RFEntry.acs2_cca_cap_db);

					/* NHM */
					RTL_W16(0x992, 0xc350); 						// period = 200ms, 4us per unit
					RTL_W16(0x996, 0xffff); 						// th10,  th9
					RTL_W32(0x998, 0xffffffff); 					// th3, th2, th1, th0
					RTL_W32(0x99c, 0xffffffff); 					// th7, th6, th5, th4

					if (max_DIG_cover_bond >= 0x26)
					{
						acs_IGI = max_DIG_cover_bond - MAX_UP_RESOLUTION;
						for (i = 0; i < 8; i++)
							RTL_W8(0x99f - i, (max_DIG_cover_bond * IGI_TO_NHM_TH_MULTIPLIER) - (MAX_UP_RESOLUTION * i));	//th7 ~ th0
					}
					else if (max_DIG_cover_bond > 0x20 && max_DIG_cover_bond < 0x26)
					{
						acs_IGI = 0x20;
						RTL_W8(0x99e, max_DIG_cover_bond * IGI_TO_NHM_TH_MULTIPLIER); // th6
						for (i = 0; i < 6; i++)
							RTL_W8(0x99d - i, (acs_IGI * IGI_TO_NHM_TH_MULTIPLIER) - (MAX_UP_RESOLUTION * i));	//th5 ~ th0
					}
					else
					{
						acs_IGI = max_DIG_cover_bond;
						for (i = 0; i < 6; i++)
							RTL_W8(0x99d - i, (max_DIG_cover_bond * IGI_TO_NHM_TH_MULTIPLIER) - (MAX_UP_RESOLUTION * i));	//th5 ~ th0
					}

					RTL_W8(0xc50, acs_IGI);
					if (get_rf_mimo_mode(priv) != MIMO_1T1R)
						RTL_W8(0xe50, acs_IGI);
					PHY_SetBBReg(priv, 0x9a0, bMaskByte0, 0xff);	// th8
					PHY_SetBBReg(priv, 0x994, BIT8|BIT9|BIT10, 1);	// BIT8=1 "CCX_en", BIT9=1 "disable ignore CCA", BIT10=ignore_TXON
			}
			else
#endif
		                priv->site_survey->ss_channel = priv->available_chnl[idx+1];

                priv->site_survey->defered_ss = should_defer_ss(priv);
            }


			if(priv->pmib->dot11RFEntry.disable_scan_ch14 && priv->site_survey->ss_channel == 14) {

				if( priv->available_chnl[(priv->available_chnl_num - 1)] == 14)
					loop_finish = 1;
				else {

					for(idx=0; idx<priv->available_chnl_num; idx++){
						if(priv->available_chnl[idx] == 14)
							break;
					}

					priv->site_survey->ss_channel = priv->available_chnl[idx+1];

				}
			}

            priv->site_survey->hidden_ap_found = 0;
        }
        else{
            STADEBUG("HIDE_AP_FOUND_DO_ACTIVE_SSAN\n");
            priv->site_survey->hidden_ap_found = HIDE_AP_FOUND_DO_ACTIVE_SSAN;
        }
        #ifdef CONFIG_RTL_NEW_AUTOCH
        if ((priv->auto_channel == 1) && priv->site_survey->to_scan_40M) {
            #if defined(RTK_5G_SUPPORT)
            if (priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_5G) {
                unsigned int current_ch = PHY_QueryRFReg(priv, RF92CD_PATH_A, 0x18, 0xff, 1);

                if (((priv->site_survey->ss_channel+2) == current_ch) || ((priv->site_survey->ss_channel-2) == current_ch)) {
                    if ((idx+2) >= (priv->available_chnl_num - 1))
                        loop_finish = 1;
                    else
                        priv->site_survey->ss_channel = priv->available_chnl[idx+2];
                }
            } else
            #endif
            {
                if (priv->site_survey->ss_channel == 14)
                    loop_finish = 1;
            }
        }
        #endif
    }

    if (loop_finish) {
        priv->site_survey_times++;
#ifdef SIMPLE_CH_UNI_PROTOCOL
        if(priv->auto_channel == 1 && GET_MIB(priv)->dot1180211sInfo.mesh_enable && priv->site_survey_times <= _11S_SS_COUNT1+_11S_SS_COUNT2 )
        {
            int reorder_chnl = 0;
            if( priv->site_survey_times == _11S_SS_COUNT1+_11S_SS_COUNT2 ) {
                reorder_chnl = 1;
            }
            else if(priv->site_survey_times == _11S_SS_COUNT1) {
                if(priv->mesh_ChannelPrecedence == 0)/*not yet select channel or recieved others channel number*/
                {
                    reorder_chnl = 1; /*for selectClearChannel to collect parameters*/
                }
            }
            else if(priv->site_survey_times == _11S_SS_COUNT1+1) {
                if(priv->mesh_ChannelPrecedence == 0)/*not yet select channel or recieved others channel number*/
                {
                    priv->pmib->dot11RFEntry.dot11channel = selectClearChannel(priv);
                    SET_PSEUDO_RANDOM_NUMBER(priv->mesh_ChannelPrecedence);
                }
            }

            if(reorder_chnl) {
                get_available_channel(priv);
            }
            else {
                for(i=0; i<priv->available_chnl_num; i++)
                {
                    get_random_bytes(&(idx), sizeof(idx));
                    idx %= priv->available_chnl_num;
                    loop_finish = priv->available_chnl[idx];
                    priv->available_chnl[idx] = priv->available_chnl[i];
                    priv->available_chnl[i] = loop_finish;
                }
            }
            priv->site_survey->ss_channel = priv->available_chnl[0];
        }
        else

#endif

// only do multiple scan when site-survey request, david+2006-01-25
//		if (priv->site_survey_times < SS_COUNT)
		if (priv->ss_req_ongoing && priv->site_survey_times < AUTOCH_SS_COUNT) {//AUTOCH_SS_SPEEDUP
// mark by david ---------------------
            #if 0   // mark by david ---------------------
			// scan again
			if (priv->pmib->dot11RFEntry.dot11ch_low != 0)
				priv->site_survey->ss_channel = priv->pmib->dot11RFEntry.dot11ch_low;
			else
            #endif  //------------------------ 2007-04-14

			//if scan 40M, start channel from start_ch_40M
			if(priv->site_survey->to_scan_40M && priv->site_survey->start_ch_40M!=0)
				priv->site_survey->ss_channel = priv->site_survey->start_ch_40M;
			else {
					priv->site_survey->ss_channel = priv->available_chnl[0];
			}

			if(priv->pmib->miscEntry.ss_loop_delay) {
				//STADEBUG("loop delay for %d miliseconads\n",priv->pmib->miscEntry.ss_loop_delay);
				if(timer_pending(&priv->ss_timer))
					del_timer(&priv->ss_timer);
				mod_timer(&priv->ss_timer, jiffies + RTL_MILISECONDS_TO_JIFFIES(priv->pmib->miscEntry.ss_loop_delay));

				if(priv->ss_req_ongoing == SSFROM_REPEATER_VXD) {
					//STADEBUG("RollBack to ROOT's ch[%d] between loop\n",GET_ROOT(priv)->pmib->dot11RFEntry.dot11channel);
					{
						if (!is_DFS_channel(GET_ROOT(priv)->pmib->dot11RFEntry.dot11channel))
							RTL_W8(TXPAUSE, RTL_R8(TXPAUSE)& ~STOP_BCN);
						else
							RTL_W8(TXPAUSE, RTL_R8(TXPAUSE) | STOP_BCN);
						SwBWMode(GET_ROOT(priv), GET_ROOT(priv)->pshare->CurrentChannelBW, GET_ROOT(priv)->pshare->offset_2nd_chan);
						SwChnl(GET_ROOT(priv), GET_ROOT(priv)->pmib->dot11RFEntry.dot11channel, GET_ROOT(priv)->pshare->offset_2nd_chan);
					}
					priv->pmib->dot11DFSEntry.disable_tx = 0;
				}
				return;
			}
		}
#ifdef CONFIG_RTL_NEW_AUTOCH
		else if ((priv->auto_channel == 1) && !priv->site_survey->to_scan_40M) {
			unsigned int z=0, ch_begin=0, ch_end=priv->available_chnl_num;
			int idx_2G_end=-1;
			unsigned int proc_nhm = 0;
#if defined(RTK_5G_SUPPORT)
			int idx_5G_begin=-1;
			if (priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_5G) {
				for (z=0; z<priv->available_chnl_num; z++) {
					if (priv->available_chnl[z] > 14) {
						idx_5G_begin = z;
						break;
					}
				}
				if (idx_5G_begin < 0)
					goto skip_40M_ss;
#ifdef NHM_ACS2_SUPPORT
				if ((GET_CHIP_VER(priv) == VERSION_8812E||GET_CHIP_VER(priv) == VERSION_8822B) && (priv->pmib->dot11RFEntry.acs_type == 2))
				{
					if (priv->auto_channel_step) {
						RTL_W8(TXPAUSE, RTL_R8(TXPAUSE)&~STOP_BCN);
						priv->auto_channel_step = 0;
						goto skip_40M_ss;
					}
				}
#endif
			} else
#endif
			if ((priv->pmib->dot11RFEntry.acs_type==1) && priv->auto_channel_step == 1)
	        {
				if (IS_OUTSRC_CHIP(priv)) {
		            DEBUG_INFO(" phydm_AutoChannelSelectSettingAP: auto_channel_step=%d\n", priv->auto_channel_step);
		            phydm_AutoChannelSelectSettingAP( ODMPTR, ACS_NHM_SETTING, priv->auto_channel_step );
				}
				else
				{
					RTL_W8(0xc50, 0x2a);
					if (get_rf_mimo_mode(priv) != MIMO_1T1R)
						RTL_W8(0xc58, 0x2a);

					RTL_W32(0x898, 0x5a50463c);
					RTL_W32(0x89c, 0xffffff64);
				}
				priv->auto_channel_step = 2;
		        priv->site_survey_times = 0;
		        priv->site_survey->ss_channel = priv->available_chnl[0];
		        proc_nhm = 1;
	        }
	        else
			{
                if (priv->auto_channel_step)        // priv->auto_channel_step = 2
				{
					if ( !priv->pshare->acs_for_adaptivity_flag ) {
                        DEBUG_INFO(" Restart Beacon, auto_channel_step=%d\n", priv->auto_channel_step);
						RTL_W8(TXPAUSE, RTL_R8(TXPAUSE)&~STOP_BCN);
					}
					priv->auto_channel_step = 0;
					goto skip_40M_ss;
				}

				for (z=0; z<priv->available_chnl_num; z++) {
					if (priv->available_chnl[z] < 14)
						idx_2G_end = z;
					else
						break;
				}
				if (idx_2G_end >= 0)
					ch_end = idx_2G_end+1;

				for (z=ch_begin; z < ch_end; z++)
					if ((priv->available_chnl[z] >= 5) && (priv->available_chnl[z] < 14))
						break;
				if (z == ch_end)
					goto skip_40M_ss;
			}

			if (!proc_nhm) {
				priv->site_survey->to_scan_40M++;
				priv->site_survey->ss_channel = priv->available_chnl[z];
				priv->site_survey->start_ch_40M = priv->available_chnl[z];//when scan 40M, record start ch
				//priv->site_survey_times = 0;//Do not reset the ss_time because it will rescan SS_COUNT again //AUTOCH_SS_SPEEDUP
				priv->pshare->CurrentChannelBW = HT_CHANNEL_WIDTH_20_40;
			}
		}
#endif
        else {
#ifdef CONFIG_RTL_NEW_AUTOCH
skip_40M_ss:
            priv->site_survey->to_scan_40M = 0;
#ifdef NHM_ACS2_SUPPORT
		if ((priv->auto_channel == 1) && (priv->pmib->dot11RFEntry.acs_type == 2))
		{
			if (priv->auto_channel_step)
			{
				RTL_W8(TXPAUSE, RTL_R8(TXPAUSE)&~STOP_BCN);
				priv->auto_channel_step = 0;
			}
		}
#endif
#endif
            /*cfg p2p cfg p2p*/
            // scan end
            OPMODE_VAL(OPMODE & ~WIFI_SITE_MONITOR);
            //STADEBUG("End of scan\n");
			if(priv->site_survey->pptyIE) {
				if(priv->site_survey->pptyIE->content) {
					kfree(priv->site_survey->pptyIE->content);
					priv->site_survey->pptyIE->content = NULL;
				}

				kfree(priv->site_survey->pptyIE);
				priv->site_survey->pptyIE = NULL;
			}


            DIG_for_site_survey(priv, FALSE);
			priv->pshare->bScanInProcess = FALSE;
            {
#if 0
                if (OPMODE & WIFI_STATION_STATE) {
                    if (IS_ROOT_INTERFACE(priv) && !netif_running(GET_VXD_PRIV(priv)->dev))
                        RTL_W32(RCR, RTL_R32(RCR) | RCR_CBSSID);
                }
                else
#endif
                if (IS_ROOT_INTERFACE(priv) && !netif_running(GET_VXD_PRIV(priv)->dev))
				if ((IS_VXD_INTERFACE(priv)&& !timer_pending(&GET_ROOT(priv)->ss_timer))
					|| (IS_ROOT_INTERFACE(priv) && !timer_pending(&GET_VXD_PRIV(priv)->ss_timer)))
                    {
					    if ( !priv->pshare->acs_for_adaptivity_flag )
                            RTL_W8(TXPAUSE, RTL_R8(TXPAUSE) & ~STOP_BCN); 	// Re-enable beacon
                    }

				if (OPMODE & WIFI_ADHOC_STATE)
					RTL_W32(RCR, RTL_R32(RCR) | RCR_CBSSID_ADHOC);

                if(IS_ROOT_INTERFACE(priv) && (OPMODE & WIFI_STATION_STATE))
                    RTL_W32(RCR, RTL_R32(RCR) | RCR_CBSSID_ADHOC);
            }



            if (!GET_ROOT(priv)->pmib->dot11DFSEntry.disable_DFS &&
                    (timer_pending(&GET_ROOT(priv)->ch_avail_chk_timer)))
                GET_ROOT(priv)->pmib->dot11DFSEntry.disable_tx = 1;
            else
                GET_ROOT(priv)->pmib->dot11DFSEntry.disable_tx = 0;

			if (priv->ss_req_ongoing
			&& (IS_ROOT_INTERFACE(priv) ||
			(IS_VXD_INTERFACE(priv) && !GET_ROOT(priv)->pmib->dot11DFSEntry.CAC_ss_counter ))
			)
			{
				GET_ROOT(priv)->pshare->CurrentChannelBW = GET_ROOT(priv)->pshare->is_40m_bw;
				STADEBUG("RollBack to ROOT's ch[%d] becoz scan done\n",GET_ROOT(priv)->pmib->dot11RFEntry.dot11channel);
				{
					if (priv->pmib->dot11RFEntry.phyBandSelect & PHY_BAND_2G) {
					 if ((GET_ROOT(priv)->pmib->dot11RFEntry.dot11channel < 5)
						&& GET_ROOT(priv)->pshare->offset_2nd_chan == HT_2NDCH_OFFSET_BELOW){
		        				GET_ROOT(priv)->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_ABOVE;
					}
					else if ((GET_ROOT(priv)->pmib->dot11RFEntry.dot11channel > 9)
						&& (GET_ROOT(priv)->pshare->offset_2nd_chan == HT_2NDCH_OFFSET_ABOVE)){
               						GET_ROOT(priv)->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_BELOW;
					}
					}
					SwBWMode(GET_ROOT(priv), GET_ROOT(priv)->pshare->CurrentChannelBW, GET_ROOT(priv)->pshare->offset_2nd_chan);
					SwChnl(GET_ROOT(priv), GET_ROOT(priv)->pmib->dot11RFEntry.dot11channel, GET_ROOT(priv)->pshare->offset_2nd_chan);
				}
                #if 0//CONFIG_RTL_92D_SUPPORT
				if (GET_CHIP_VER(priv) == VERSION_8192D
					&& IS_ROOT_INTERFACE(priv)
				);
                #endif
			}
			if (IS_VXD_INTERFACE(priv) && priv->pmib->wscEntry.wsc_enable)
				GET_ROOT(priv)->pmib->miscEntry.func_off = 0;





            #ifdef SUPPORT_MULTI_PROFILE
            if( priv->pmib->ap_profile.sortbyprofile && priv->pmib->ap_profile.enable_profile && priv->pmib->ap_profile.profile_num > 0){

                // sort by profile
                ProfileSort(priv,COMPARE_BSS, priv->site_survey->bss, priv->site_survey->count, sizeof(struct bss_desc));
                #ifdef WIFI_SIMPLE_CONFIG
                ProfileSort(priv,COMPARE_WSCIE, priv->site_survey->wscie, priv->site_survey->count, sizeof(struct wps_ie_info));
                #endif
                    ProfileSort(priv,COMPARE_WPAIE, priv->site_survey->wpa_ie, priv->site_survey->count, sizeof(struct wpa_ie_info));
                    ProfileSort(priv,COMPARE_RSNIE, priv->site_survey->rsn_ie, priv->site_survey->count, sizeof(struct rsn_ie_info));
            }else
            #endif
            {
		// sort by rssi
		qsort(priv->site_survey->bss, priv->site_survey->count, sizeof(struct bss_desc), compareBSS);
		#ifdef WIFI_SIMPLE_CONFIG
		qsort(priv->site_survey->wscie, priv->site_survey->count, sizeof(struct wps_ie_info), compareWpsIE);
		if (priv->pshare->rf_ft_var.prefer_2g) { // prefer 2G
			qsort(priv->site_survey->bss, priv->site_survey->count, sizeof(struct bss_desc), compareBSS_for_2G);
			qsort(priv->site_survey->wscie, priv->site_survey->count, sizeof(struct wps_ie_info), compareWpsIE_for_2G);
		}
		#endif
		qsort(priv->site_survey->wpa_ie, priv->site_survey->count, sizeof(struct wpa_ie_info), compareWpaIE);
		qsort(priv->site_survey->rsn_ie, priv->site_survey->count, sizeof(struct rsn_ie_info), compareRsnIE);
            }

    		debug_print_bss(priv);

			SMP_UNLOCK(flags);
			NDEBUG2("end of scan,==>realtek_cfg80211_inform_ss_result\n");
			realtek_cfg80211_inform_ss_result(priv);

            /*cfg p2p note:  priv-> ss_req_ongoing be set to 0 in event_indicate_cfg80211()*/
            {
				//if(priv->site_survey->count) always report scan done, maight be SSID not found
					event_indicate_cfg80211(priv, NULL, CFG80211_SCAN_DONE, NULL);
            }

			SMP_LOCK(flags);
            if (priv->auto_channel == 1) {
                #ifdef SIMPLE_CH_UNI_PROTOCOL
                if(GET_MIB(priv)->dot1180211sInfo.mesh_enable) {
                    get_available_channel(priv);
                }
                if(!GET_MIB(priv)->dot1180211sInfo.mesh_enable)
                #endif
                {
    				priv->pmib->dot11RFEntry.dot11channel = selectClearChannel(priv);
    				DEBUG_INFO("auto channel select ch %d\n", priv->pmib->dot11RFEntry.dot11channel);
                    #if defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD) || defined(CONFIG_RTL8196C_KLD)
    				LOG_START_MSG();
                    #endif

					// Adaptivity mode 2/3 will use nhm for detect environment, so NHM threshold should be recovered after NHM-ACS done
					if (priv->pmib->dot11RFEntry.acs_type && priv->pshare->rf_ft_var.adaptivity_enable >= 2)
					{
						if ( IS_OUTSRC_CHIP(priv) )
							Phydm_NHMCounterStatisticsInit(ODMPTR);
						else
						{
							rtl8192cd_NHMBBInit(priv);
						}
					}

					if ( priv->pshare->acs_for_adaptivity_flag ) {
					    priv->pshare->acs_for_adaptivity_flag = FALSE;
					    RTL_W8(TXPAUSE, priv->pshare->reg_tapause_bak);
					}
                }
				if (IS_ROOT_INTERFACE(priv))
				{
				 	if(!priv->pmib->dot11DFSEntry.disable_DFS
					&& is_DFS_channel(priv->pmib->dot11RFEntry.dot11channel) && (OPMODE & WIFI_AP_STATE)) {
						if (timer_pending(&priv->DFS_timer))
							del_timer(&priv->DFS_timer);

						if (timer_pending(&priv->ch_avail_chk_timer))
							del_timer(&priv->ch_avail_chk_timer);

						if (timer_pending(&priv->dfs_det_chk_timer))
							del_timer(&priv->dfs_det_chk_timer);

						init_timer(&priv->ch_avail_chk_timer);
						priv->ch_avail_chk_timer.data = (unsigned long) priv;
						priv->ch_avail_chk_timer.function = rtl8192cd_ch_avail_chk_timer;

						if ((priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_ETSI) &&
							(IS_METEOROLOGY_CHANNEL(priv->pmib->dot11RFEntry.dot11channel)))
							mod_timer(&priv->ch_avail_chk_timer, jiffies + CH_AVAIL_CHK_TO_CE);
						else
							mod_timer(&priv->ch_avail_chk_timer, jiffies + CH_AVAIL_CHK_TO);

						init_timer(&priv->DFS_timer);
						priv->DFS_timer.data = (unsigned long) priv;
						priv->DFS_timer.function = rtl8192cd_DFS_timer;

						/* DFS activated after 5 sec; prevent switching channel due to DFS false alarm */
						mod_timer(&priv->DFS_timer, jiffies + RTL_SECONDS_TO_JIFFIES(5));

						init_timer(&priv->dfs_det_chk_timer);
						priv->dfs_det_chk_timer.data = (unsigned long) priv;
						priv->dfs_det_chk_timer.function = rtl8192cd_dfs_det_chk_timer;

						mod_timer(&priv->dfs_det_chk_timer, jiffies + RTL_MILISECONDS_TO_JIFFIES(priv->pshare->rf_ft_var.dfs_det_period*10));

						DFS_SetReg(priv);

						if (!priv->pmib->dot11DFSEntry.CAC_enable) {
							del_timer_sync(&priv->ch_avail_chk_timer);
							mod_timer(&priv->ch_avail_chk_timer, jiffies + RTL_MILISECONDS_TO_JIFFIES(200));
						}
				 	}


					/* disable all of the transmissions during channel availability check */
					priv->pmib->dot11DFSEntry.disable_tx = 0;
					if (!priv->pmib->dot11DFSEntry.disable_DFS &&
					is_DFS_channel(priv->pmib->dot11RFEntry.dot11channel) && (OPMODE & WIFI_AP_STATE)){
						priv->pmib->dot11DFSEntry.disable_tx = 1;
					}
				}



				if (OPMODE & WIFI_AP_STATE)
					priv->auto_channel = 0;
				else
					priv->auto_channel = 2;

				priv->pshare->CurrentChannelBW = priv->pshare->is_40m_bw;
				{
					SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);
					SwChnl(priv, priv->pmib->dot11RFEntry.dot11channel, priv->pshare->offset_2nd_chan);
				}

				priv->ht_cap_len = 0;	// re-construct HT IE
				init_beacon(priv);
#ifdef SIMPLE_CH_UNI_PROTOCOL
				STADEBUG("scan finish, sw ch to (#%d), init beacon\n", priv->pmib->dot11RFEntry.dot11channel);
#endif
				if (GET_ROOT(priv)->pmib->miscEntry.vap_enable) {
					for (i=0; i<RTL8192CD_NUM_VWLAN; i++) {
						priv->pvap_priv[i]->pmib->dot11RFEntry.dot11channel = priv->pmib->dot11RFEntry.dot11channel;
						priv->pvap_priv[i]->ht_cap_len = 0;	// re-construct HT IE

						if (IS_DRV_OPEN(priv->pvap_priv[i]))
							init_beacon(priv->pvap_priv[i]);
					}
				}

#ifdef CLIENT_MODE
#ifdef HS2_CLIENT_TEST
				JOIN_RES = STATE_Sta_Ibss_Idle;
#else
				if (JOIN_RES == STATE_Sta_Ibss_Idle) {
					RTL_W32(CR, (RTL_R32(CR) & ~(NETYPE_Mask << NETYPE_SHIFT)) | ((NETYPE_ADHOC & NETYPE_Mask) << NETYPE_SHIFT));
					mod_timer(&priv->idle_timer, jiffies + RTL_SECONDS_TO_JIFFIES(5));
				}
#endif
#endif



				if (priv->ss_req_ongoing) {
					priv->site_survey->count_backup = priv->site_survey->count;
					memcpy(priv->site_survey->bss_backup, priv->site_survey->bss, sizeof(struct bss_desc)*priv->site_survey->count);
					priv->ss_req_ongoing = 0;
				}

#if defined(CONFIG_RTL_NEW_AUTOCH) && defined(SS_CH_LOAD_PROC)
				record_SS_report(priv);
#endif

			}
			// backup the bss database
#ifdef WIFI_WPAS_CLI
			else if (SSFROM_WPAS == priv->ss_req_ongoing) {
				printk("cliW: scan ind to => sp:%d ss:%d\n",
					priv->pshare->bScanInProcess, priv->ss_req_ongoing);

				/* report scan list to wpa_supplicant using ap scan */
				GET_VXD_PRIV(priv)->site_survey->count_backup = priv->site_survey->count;
				memcpy(GET_VXD_PRIV(priv)->site_survey->bss_backup, priv->site_survey->bss,
					sizeof(struct bss_desc)*priv->site_survey->count);
#ifdef WIFI_SIMPLE_CONFIG
				memcpy(GET_VXD_PRIV(priv)->site_survey->wscie, priv->site_survey->wscie,
					sizeof(struct wps_ie_info)*priv->site_survey->count);
#endif
				GET_VXD_PRIV(priv)->ss_req_ongoing = 0;
				priv->ss_req_ongoing = 0;
				printk("cliW: scan done 4 ss_req_ongoing = 0\n");
				event_indicate_wpas(GET_VXD_PRIV(priv), NULL, WPAS_SCAN_DONE, NULL);
			}
#endif // WIFI_WPAS_CLI
			else if (priv->ss_req_ongoing) {
				{
					priv->site_survey->count_backup = priv->site_survey->count;
					memcpy(priv->site_survey->bss_backup, priv->site_survey->bss, sizeof(struct bss_desc)*priv->site_survey->count);
				}

/*cfg p2p;remove*/
/*cfg p2p;remove*/
#ifdef CLIENT_MODE
				if(priv->ss_req_ongoing != SSFROM_WSC)
				{
    				if (JOIN_RES == STATE_Sta_Ibss_Idle) {
                        STADEBUG("start_clnt_lookup(RESCAN)\n");
    					start_clnt_lookup(priv, RESCAN);
    				}
    				else if (JOIN_RES == STATE_Sta_Auth_Success){
    					start_clnt_assoc(priv);
    				}
    				else if (JOIN_RES == STATE_Sta_Roaming_Scan) {
                        STADEBUG("start_clnt_lookup(RESCAN)\n");
    					start_clnt_lookup(priv, RESCAN);
    				}
    				else if(JOIN_RES == STATE_Sta_No_Bss) {
                        STADEBUG("start_clnt_lookup(RESCAN)\n");
    					JOIN_RES_VAL(STATE_Sta_Roaming_Scan);
    					start_clnt_lookup(priv, RESCAN);
    				}

				}
#endif

				if (priv->ss_req_ongoing == SSFROM_REPEATER_VXD) {
					// VXD Interface
#ifdef SUPPORT_MULTI_PROFILE
					if (GET_MIB(priv)->ap_profile.enable_profile &&
							GET_MIB(priv)->ap_profile.profile_num > 0) {
						SSID2SCAN_LEN = strlen(GET_MIB(priv)->ap_profile.profile[priv->profile_idx].ssid);
						memcpy(SSID2SCAN, GET_MIB(priv)->ap_profile.profile[priv->profile_idx].ssid, SSID2SCAN_LEN);
					}
					else
#endif
					{
						SSID2SCAN_LEN = GET_MIB(priv)->dot11StationConfigEntry.dot11SSIDtoScanLen;
						memcpy(SSID2SCAN, GET_MIB(priv)->dot11StationConfigEntry.dot11SSIDtoScan, SSID2SCAN_LEN);
					}
					priv->site_survey->count_target = priv->site_survey->count;
					memcpy(priv->site_survey->bss_target, priv->site_survey->bss, sizeof(struct bss_desc)*priv->site_survey->count);
					priv->join_index = -1;
					priv->join_res = STATE_Sta_Min;

					start_clnt_lookup(priv, DONTRESCAN);
				}

				priv->ss_req_ongoing = 0;
                //STADEBUG("set priv->ss_req_ongoing to 0\n\n\n");

#ifdef WIFI_WPAS
				event_indicate_wpas(priv, NULL, WPAS_SCAN_DONE, NULL);
#endif
			}
#ifdef CLIENT_MODE
			else if (OPMODE & (WIFI_STATION_STATE | WIFI_ADHOC_STATE)) {
				priv->site_survey->count_target = priv->site_survey->count;
				memcpy(priv->site_survey->bss_target, priv->site_survey->bss, sizeof(struct bss_desc)*priv->site_survey->count);
				priv->join_index = -1;
#ifdef HS2_CLIENT_TEST
#else
				if (JOIN_RES == STATE_Sta_Roaming_Scan){
                    STADEBUG("start_clnt_lookup(DONTRESCAN)\n");
					start_clnt_lookup(priv, DONTRESCAN);
                }

#endif
			}
#endif
			else {
				DEBUG_ERR("Faulty scanning\n");
			}

#ifdef CONFIG_RTL_COMAPI_WLTOOLS
            wake_up_interruptible(&priv->ss_wait);
#endif

#ifdef CHECK_BEACON_HANGUP
            priv->pshare->beacon_wait_cnt = 2;
#endif
            SMP_UNLOCK(flags);
            /*cfg p2p cfg p2p*/
            NDEBUG2("endofscan\n");
            /*cfg p2p cfg p2p*/
#ifdef SDIO_AP_OFFLOAD
			ap_offload_activate(priv, OFFLOAD_PROHIBIT_SITE_SURVEY);
#endif
            return;
        }
    }



   /*switch channel; now, change RF channel... start*/


	// now, change RF channel...
    if (!priv->pmib->dot11DFSEntry.disable_DFS && is_DFS_channel(priv->site_survey->ss_channel)){
        priv->pmib->dot11DFSEntry.disable_tx = 1;
    }else{
        priv->pmib->dot11DFSEntry.disable_tx = 0;
    }


    #ifdef CONFIG_RTL_NEW_AUTOCH
    if (priv->site_survey->to_scan_40M) {
        #if defined(RTK_5G_SUPPORT)
        if (GET_ROOT(priv)->pmib->dot11RFEntry.phyBandSelect & PHY_BAND_5G) {
            if((priv->site_survey->ss_channel>144) ? ((priv->site_survey->ss_channel-1)%8) : (priv->site_survey->ss_channel%8)) {
                //STADEBUG("SwChnl=%d,2ndoffsetCh=%d\n",priv->site_survey->ss_channel,HT_2NDCH_OFFSET_ABOVE);
                {
                    SwChnl(GET_ROOT(priv), priv->site_survey->ss_channel, HT_2NDCH_OFFSET_ABOVE);
                    SwBWMode(GET_ROOT(priv), priv->pshare->CurrentChannelBW, HT_2NDCH_OFFSET_ABOVE);
                }
            } else {
                //STADEBUG("SwChnl=%d,2ndoffsetCh=%d\n",priv->site_survey->ss_channel,HT_2NDCH_OFFSET_BELOW);
                {
                    SwChnl(GET_ROOT(priv), priv->site_survey->ss_channel, HT_2NDCH_OFFSET_BELOW);
                    SwBWMode(GET_ROOT(priv), priv->pshare->CurrentChannelBW, HT_2NDCH_OFFSET_BELOW);
                }
            }

        } else
        #endif
        {
            /* set channel >= 5 for algo requirement */
            //STADEBUG("SwChnl=%d,2ndoffsetCh=%d\n",priv->site_survey->ss_channel,HT_2NDCH_OFFSET_BELOW);
            {
		if (priv->site_survey->ss_channel >= 5)
		{
                	SwChnl(GET_ROOT(priv), priv->site_survey->ss_channel, HT_2NDCH_OFFSET_BELOW);
                	SwBWMode(GET_ROOT(priv), priv->pshare->CurrentChannelBW, HT_2NDCH_OFFSET_BELOW);
		}
		else
		{
			SwChnl(GET_ROOT(priv), priv->site_survey->ss_channel, HT_2NDCH_OFFSET_ABOVE);
			SwBWMode(GET_ROOT(priv), priv->pshare->CurrentChannelBW, HT_2NDCH_OFFSET_ABOVE);
		}
            }
        }

	if (priv->auto_channel == 1) {
		reset_FA_reg(priv);
#ifdef NHM_ACS2_SUPPORT
		if ((GET_CHIP_VER(priv) == VERSION_8192E||GET_CHIP_VER(priv) == VERSION_8197F) && (priv->pmib->dot11RFEntry.acs_type == 2)
					  && priv->auto_channel_step) { // 40M
			PHY_SetBBReg(priv, 0x890, BIT1, 0);
			PHY_SetBBReg(priv, 0x890, BIT1, 1);

			phydm_CLMtrigger(ODMPTR);
		}
#endif
        }
    }
    else
    #endif
    {
		if(IS_ROOT_INTERFACE(priv) ||
		(IS_VXD_INTERFACE(priv) && !GET_ROOT(priv)->pmib->dot11DFSEntry.CAC_ss_counter))
		{
	        if(priv->site_survey->defered_ss) {
	            //STADEBUG("Between B53&B56, stay AP's ch for %d miliseconds\n",priv->pmib->miscEntry.ss_delay);
	            //STADEBUG("SwChnl[%d],2ndCh[%d]\n",GET_ROOT(priv)->pmib->dot11RFEntry.dot11channel,GET_ROOT(priv)->pshare->offset_2nd_chan);
	            {
				if (!is_DFS_channel(GET_ROOT(priv)->pmib->dot11RFEntry.dot11channel))
					RTL_W8(TXPAUSE, RTL_R8(TXPAUSE)&~STOP_BCN);
				else
					RTL_W8(TXPAUSE, RTL_R8(TXPAUSE) | STOP_BCN);
	                SwBWMode(GET_ROOT(priv), GET_ROOT(priv)->pshare->CurrentChannelBW, GET_ROOT(priv)->pshare->offset_2nd_chan);
	                SwChnl(GET_ROOT(priv), GET_ROOT(priv)->pmib->dot11RFEntry.dot11channel, GET_ROOT(priv)->pshare->offset_2nd_chan);
	            }
	            priv->pmib->dot11DFSEntry.disable_tx = 0;

	        } else
	        {
				{
					if ( !priv->pshare->acs_for_adaptivity_flag )
					{
						if (!is_DFS_channel(priv->site_survey->ss_channel))
							RTL_W8(TXPAUSE, RTL_R8(TXPAUSE)&~STOP_BCN);
						else
							RTL_W8(TXPAUSE, RTL_R8(TXPAUSE) | STOP_BCN);
					}

				}
				SwChnl(GET_ROOT(priv), priv->site_survey->ss_channel, priv->pshare->offset_2nd_chan);

	            #ifdef CONFIG_RTL_NEW_AUTOCH
	            if (priv->auto_channel == 1) {
	                reset_FA_reg(priv);

			if ((GET_CHIP_VER(priv) == VERSION_8192E||GET_CHIP_VER(priv) == VERSION_8197F) && priv->pmib->dot11RFEntry.acs_type && priv->auto_channel_step) {
	                	PHY_SetBBReg(priv, 0x890, BIT1, 0);
	        	        PHY_SetBBReg(priv, 0x890, BIT1, 1);
#ifdef NHM_ACS2_SUPPORT
				if (priv->pmib->dot11RFEntry.acs_type == 2)
					phydm_CLMtrigger(ODMPTR);
#endif
	                }
#ifdef NHM_ACS2_SUPPORT
			else if ((GET_CHIP_VER(priv) == VERSION_8812E||GET_CHIP_VER(priv) == VERSION_8822B) && (priv->pmib->dot11RFEntry.acs_type==2) && priv->auto_channel_step) {
				PHY_SetBBReg(priv, 0x994, BIT1, 0);
				PHY_SetBBReg(priv, 0x994, BIT1, 1);

				phydm_CLMtrigger(ODMPTR);
			}
#endif
	            }
	            #endif
	        }
	    }
	}
	/*switch channel-end*/
	//by brian, trigger channel load evaluation after channel switched
	start_bbp_ch_load(priv, RTK80211_TIME_TO_SAMPLE_NUM);
    /*TX probe_request -- start*/


    if (priv->site_survey->hidden_ap_found == HIDE_AP_FOUND_DO_ACTIVE_SSAN ||
        !is_passive_channel(priv , priv->pmib->dot11StationConfigEntry.dot11RegDomain, priv->site_survey->ss_channel))
    {

        #ifdef SIMPLE_CH_UNI_PROTOCOL
        if(GET_MIB(priv)->dot1180211sInfo.mesh_enable)
            issue_probereq_MP(priv, "MESH-SCAN", 9, NULL, TRUE);
        else
        #endif
        if (priv->ss_ssidlen == 0){

            {
                if (!priv->auto_channel_step) {
                    issue_probereq(priv, NULL, 0, NULL);
                }
            }
        }else{
            #ifdef SUPPORT_MULTI_PROFILE	/*send multi probe_req by profile_num*/
            if (priv->pmib->ap_profile.enable_profile && priv->pmib->ap_profile.profile_num > 0) {
                for(jdx=0;jdx<priv->pmib->ap_profile.profile_num;jdx++) {
                    STADEBUG("issue_probereq[%s],ch=[%d]\n",priv->pmib->ap_profile.profile[jdx].ssid , priv->site_survey->ss_channel);
                    issue_probereq(priv, priv->pmib->ap_profile.profile[jdx].ssid, strlen(priv->pmib->ap_profile.profile[jdx].ssid), NULL);
                }
            }
            else
            #endif
            {
                STADEBUG("issue_probereq,ssid[%s],ch[%d]\n",priv->ss_ssid,priv->site_survey->ss_channel);
                issue_probereq(priv, priv->ss_ssid, priv->ss_ssidlen, NULL);
            }

        }
    }

    /*TX probe_request -- End*/
    SMP_UNLOCK(flags);


	/*now, scheduling next ss_timer ; the time util to next timer executed is how long DUT stady in this channel*/
	// now, start another timer again.

    if(is_passive_channel(priv , priv->pmib->dot11StationConfigEntry.dot11RegDomain, priv->site_survey->ss_channel))
    {
        if(priv->site_survey->defered_ss) {
            if(priv->pmib->miscEntry.ss_delay) {
                mod_timer(&priv->ss_timer, jiffies + RTL_MILISECONDS_TO_JIFFIES(priv->pmib->miscEntry.ss_delay));
            } else {
                if(priv->pmib->miscEntry.passive_ss_int) {
                    mod_timer(&priv->ss_timer, jiffies + RTL_MILISECONDS_TO_JIFFIES(priv->pmib->miscEntry.passive_ss_int));
                } else {
                    mod_timer(&priv->ss_timer, jiffies + SS_PSSV_TO);
                }
            }
        }
        else {
            if(priv->pmib->miscEntry.passive_ss_int) {
                mod_timer(&priv->ss_timer, jiffies + RTL_MILISECONDS_TO_JIFFIES(priv->pmib->miscEntry.passive_ss_int));
            } else {
                mod_timer(&priv->ss_timer, jiffies + SS_PSSV_TO);
            }
        }


    }
    else

    {
        #ifdef CONFIG_RTL_NEW_AUTOCH
        if (priv->auto_channel == 1){
            #ifdef AUTOCH_SS_SPEEDUP
            if(priv->pmib->miscEntry.autoch_ss_to != 0)
                mod_timer(&priv->ss_timer, jiffies + RTL_MILISECONDS_TO_JIFFIES(priv->pmib->miscEntry.autoch_ss_to));
            else
            #endif
            if (priv->auto_channel_step)
		{
			if (priv->pmib->dot11RFEntry.acs_type == 1)
				mod_timer(&priv->ss_timer, jiffies + SS_AUTO_CHNL_NHM_TO);
#ifdef NHM_ACS2_SUPPORT
			else if (priv->pmib->dot11RFEntry.acs_type == 2)
				mod_timer(&priv->ss_timer, jiffies + SS_AUTO_CHNL_ACS2_TO);
#endif
			else
				printk("Error @ %s : wrong acs_type (%d)\n", __FUNCTION__, priv->pmib->dot11RFEntry.acs_type);
		}
            else
                mod_timer(&priv->ss_timer, jiffies + SS_AUTO_CHNL_TO);
        }else
        #endif
        {
            {
    	    	mod_timer(&priv->ss_timer, jiffies + SS_TO);
            }
        }
    }
}


/**
 *	@brief  get WPA/WPA2 information
 *
 *	use 1 timestamp (32-bit variable) to carry WPA/WPA2 info \n
 *	1st 16-bit:                 WPA \n
 *  |          auth       |              unicast cipher              |              multicast cipher            |	\n
 *     15    14    13   12      11      10     9     8       7      6      5       4      3     2       1      0	\n
 *	+-----+-----+----+-----+--------+------+-----+------+-------+-----+--------+------+-----+------+-------+-----+	\n
 *	| Rsv | PSK | 1X | Rsv | WEP104 | CCMP | Rsv | TKIP | WEP40 | Grp | WEP104 | CCMP | Rsv | TKIP | WEP40 | Grp |	\n
 *	+-----+-----+----+-----+--------+------+-----+------+-------+-----+--------+------+-----+------+-------+-----+	\n
 *	2nd 16-bit:                 WPA2 \n
 *            auth       |              unicast cipher              |              multicast cipher            |	\n
 *	  15    14    13   12      11      10     9     8       7      6      5       4      3     2       1      0		\n
 *  +-----+-----+----+-----+--------+------+-----+------+-------+-----+--------+------+-----+------+-------+-----+	\n
 *	| Rsv | PSK | 1X | Rsv | WEP104 | CCMP | Rsv | TKIP | WEP40 | Grp | WEP104 | CCMP | Rsv | TKIP | WEP40 | Grp |	\n
 *  +-----+-----+----+-----+--------+------+-----+------+-------+-----+--------+------+-----+------+-------+-----+	\n
 */
static void get_security_info(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo, int index)
{
	int i, len, result;
	unsigned char *p, *pframe, *p_uni, *p_auth, val;
	unsigned short num;
	unsigned char OUI1[] = {0x00, 0x50, 0xf2};
	unsigned char OUI2[] = {0x00, 0x0f, 0xac};
#ifdef  CONFIG_IEEE80211W_CLI
	unsigned short	rsnie_cap;
#endif

	//WPS2DOTX
	unsigned char *awPtr  =  (unsigned char *)&priv->site_survey->wscie[index].data;
	int foundtimes = 0;
	unsigned char *ptmp = NULL;
	unsigned int lentmp=0;
	unsigned int totallen=0;
	//WPS2DOTX

	pframe = get_pframe(pfrinfo);
	priv->site_survey->bss[index].t_stamp[0] = 0;

	p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_;
	len = 0;
	result = 0;
	do {
		p = get_ie(p, _RSN_IE_1_, &len,
			pfrinfo->pktlen - (p - pframe));
		if ((p != NULL) && (len > 18))
		{
			if (memcmp((p + 2), OUI1, 3))
				goto next_tag;
			if (*(p + 5) != 0x01)
				goto next_tag;
			if (memcmp((p + 8), OUI1, 3))
				goto next_tag;
			val = *(p + 11);
			priv->site_survey->bss[index].t_stamp[0] |= BIT(val);
			p_uni = p + 12;
			memcpy(&num, p_uni, 2);
			num = le16_to_cpu(num);
			for (i=0; i<num; i++) {
				if (memcmp((p_uni + 2 + 4 * i), OUI1, 3))
					goto next_tag;
				val = *(p_uni + 2 + 4 * i + 3);
				priv->site_survey->bss[index].t_stamp[0] |= (BIT(val) << 6);
			}
			p_auth = p_uni + 2 + 4 * num;
			memcpy(&num, p_auth, 2);
			num = le16_to_cpu(num);
			for (i=0; i<num; i++) {
				if (memcmp((p_auth + 2 + 4 * i), OUI1, 3))
					goto next_tag;
				val = *(p_auth + 2 + 4 * i + 3);
				priv->site_survey->bss[index].t_stamp[0] |= (BIT(val) << 12);
			}
			result = 1;
		}
next_tag:
		if (p != NULL)
			p = p + 2 + len;
	} while ((p != NULL) && (result != 1));

	if (result != 1)
	{
		priv->site_survey->bss[index].t_stamp[0] = 0;
	}

	p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_;
	len = 0;
	result = 0;
	do {
		p = get_ie(p, _RSN_IE_2_, &len,
			pfrinfo->pktlen - (p - pframe));
		if ((p != NULL) && (len > 12))
		{
			if (memcmp((p + 4), OUI2, 3))
				goto next_id;
			val = *(p + 7);
			priv->site_survey->bss[index].t_stamp[0] |= (BIT(val) << 16);
			p_uni = p + 8;
			memcpy(&num, p_uni, 2);
			num = le16_to_cpu(num);
			for (i=0; i<num; i++) {
				if (memcmp((p_uni + 2 + 4 * i), OUI2, 3))
					goto next_id;
				val = *(p_uni + 2 + 4 * i + 3);
				priv->site_survey->bss[index].t_stamp[0] |= (BIT(val) << 22);
			}
			p_auth = p_uni + 2 + 4 * num;
			memcpy(&num, p_auth, 2);
			num = le16_to_cpu(num);

#ifdef CONFIG_IEEE80211W_CLI
			priv->bss_support_akmp = 0;
#endif

			for (i=0; i<num; i++) {
				if (memcmp((p_auth + 2 + 4 * i), OUI2, 3))
					goto next_id;
				val = *(p_auth + 2 + 4 * i + 3);
				priv->site_survey->bss[index].t_stamp[0] |= (BIT(val) << 28);

#ifdef CONFIG_IEEE80211W_CLI
				priv->bss_support_akmp |= BIT(val);
#endif
			}

#ifdef CONFIG_IEEE80211W_CLI
			memcpy(&rsnie_cap, (p_auth + 2 + 4 * num), 2);
			priv->wpa_global_info->rsnie_cap = le16_to_cpu(rsnie_cap);

			if((priv->wpa_global_info->rsnie_cap & BIT(6)) && (priv->wpa_global_info->rsnie_cap & BIT(7)))
				priv->site_survey->bss[index].t_stamp[1] |= PMF_REQ;
			else if(priv->wpa_global_info->rsnie_cap & BIT(7))
				priv->site_survey->bss[index].t_stamp[1] |= PMF_CAP;
			else
				priv->site_survey->bss[index].t_stamp[1] |= PMF_NONE;
#endif

			result = 1;
		}
next_id:
		if (p != NULL)
			p = p + 2 + len;
	} while ((p != NULL) && (result != 1));

	if (result != 1)
	{
		priv->site_survey->bss[index].t_stamp[0] &= 0x0000ffff;
	}


#ifdef WIFI_SIMPLE_CONFIG
/* WPS2DOTX*/
#if defined(WIFI_WPAS_CLI)
	if ((OPMODE & WIFI_STATION_STATE) || (SSFROM_WPAS == priv->ss_req_ongoing))
#else
	if (OPMODE & WIFI_STATION_STATE)
#endif
	{
		//ptmp = pframe + WLAN_HDR_A3_LEN + _PROBEREQ_IE_OFFSET_;
		ptmp = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_;

		for (;;)
		{
			ptmp = get_ie(ptmp, _WPS_IE_, (int *)&lentmp,pfrinfo->pktlen - (ptmp - pframe));

			if (ptmp != NULL) {
				if (!memcmp(ptmp+2, WSC_IE_OUI, 4)) {
					foundtimes ++;
					if(foundtimes ==1){
						memcpy(awPtr , ptmp ,lentmp + 2);
						awPtr+= (lentmp + 2);
						totallen += (lentmp + 2);
					}else{
						memcpy(awPtr , ptmp+2+4 ,lentmp-4);
						awPtr+= (lentmp-4);
						totallen += (lentmp-4);
					}
				}
			}
			else{
				break;
			}

			ptmp = ptmp + lentmp + 2;
		}
		if(foundtimes){
        	/*cfg p2p cfg p2p*/
			/*Save WSC IE*/
			priv->site_survey->wscie[index].wps_ie_len = totallen;
			//debug_out("WSC_IE",priv->site_survey.ie[index].data,totallen);
			//get the first wps ie to see the
			//search_wsc_tag(unsigned char *data, unsigned short id, int len, &target_len);
			priv->site_survey->bss[index].t_stamp[1] |= BIT(8);  //  set t_stamp[1] bit 8 when AP supports WPS
		} else {
			awPtr[0]='\0';
			priv->site_survey->bss[index].t_stamp[1] &= ~BIT(8);  // clear t_stamp[1] bit 8 when AP not supports WPS(do not have wps IE)
		}
	}

#endif
/* WPS2DOTX*/
}


#ifdef	CONFIG_IEEE80211W_CLI

unsigned char SHA256_AKM_SUITE[] = {0x00, 0x0F, 0xAC, 0x06};
unsigned char Doll1X_SHA256_AKM_SUITE[] = {0x00, 0x0F, 0xAC, 0x05};
unsigned char Doll1X_AKM_SUITE[] = {0x00, 0x0F, 0xAC, 0x01};


void add_sha256_akm(struct rtl8192cd_priv *priv)
{
	WPA_GLOBAL_INFO *pGblInfo = priv->wpa_global_info;
	unsigned char AKM_buff[KEY_AKM_LEN];

	OCTET_STRING AKM;
	memcpy(AKM_buff, SHA256_AKM_SUITE, KEY_AKM_LEN);
	AKM.Octet = AKM_buff;
	AKM.Length = KEY_AKM_LEN;
	Message_setSha256AKM(pGblInfo->AuthInfoElement, AKM);
}
#endif

/**
 *	@brief	After site survey, collect BSS information to site_survey->bss[index]
 *
 *	The function can find site
 *  Later finish site survey, call the function get BSS informat.
 */
int collect_bss_info(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
	int i, index, len=0;
	unsigned char *addr, *p, *pframe, *sa, channel=0;
	UINT32	basicrate=0, supportrate=0, hiddenAP=0;
	UINT16	val16;
	struct wifi_mib *pmib;
	DOT11_WPA_MULTICAST_CIPHER wpaMulticastCipher;
	unsigned char OUI1[] = {0x00, 0x50, 0xf2, 0x01};
	DOT11_WPA2_MULTICAST_CIPHER wpa2MulticastCipher;
	unsigned char OUI2[] = {0x00, 0x0f, 0xac};



#ifdef SUPPORT_MULTI_PROFILE
	int jdx=0;
	int found=0;
	int ssid_len=0;
#endif

	pframe = get_pframe(pfrinfo);
		addr = GetAddr3Ptr(pframe);

	sa = GetAddr2Ptr(pframe);
	pmib = GET_MIB(priv);

	if(priv->site_survey->pptyIE) {
		unsigned char str_content[65] = {0};

		p = get_oui(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, priv->site_survey->pptyIE->oui, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);

		if(len > 3) {
			memcpy(str_content, p+2+3, len-3);
			str_content[len-3] = '\0';

			//p+2 is the IE content
			if(p && strcmp(priv->site_survey->pptyIE->content,str_content))
				return 0;
		} else
			return 0;
	}
	/*cfg p2p cfg p2p*/

#ifdef WIFI_11N_2040_COEXIST
	if (priv->pmib->dot11nConfigEntry.dot11nCoexist &&
		(priv->pmib->dot11BssType.net_work_type & (WIRELESS_11N|WIRELESS_11G)) && (
#ifdef CLIENT_MODE
		(OPMODE & WIFI_STATION_STATE) ||
#endif
		priv->pshare->is_40m_bw)) {
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_CAP_,
			&len, pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p == NULL) {
			p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _DSSET_IE_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
			if (p != NULL)
				channel = *(p+2);
			if (OPMODE & WIFI_AP_STATE) {
				if (channel && (channel <= 14) && (priv->pmib->dot11nConfigEntry.dot11nCoexist_ch_chk ?
				 (channel != priv->pmib->dot11RFEntry.dot11channel) : 1)) {

					if(!priv->bg_ap_timeout) {
						priv->bg_ap_timeout = 60;
						update_RAMask_to_FW(priv, 1);
						SetTxPowerLevel(priv, channel);
					}

					//priv->bg_ap_timeout = 60;
				}
			}
#ifdef CLIENT_MODE
			else if ((OPMODE & WIFI_STATION_STATE) && priv->coexist_connection) {
				if (channel && (channel <= 14)) {
#if 0
//#if defined(CONFIG_RTL_8812_SUPPORT) || defined(CONFIG_WLAN_HAL)
					if(!priv->bg_ap_timeout) {
						priv->bg_ap_timeout = 180;
						update_RAMask_to_FW(priv, 1);
					}
#endif
					priv->bg_ap_timeout = 180;
					priv->bg_ap_timeout_ch[channel-1] = 180;
					channel = 0;
				}
			}
#endif
		}
#ifdef CLIENT_MODE
		else if ((OPMODE & WIFI_STATION_STATE) && priv->coexist_connection) {
			/*
			 *	check if there is any 40M intolerant field set by other 11n AP
			 */
			struct ht_cap_elmt *ht_cap=(struct ht_cap_elmt *)(p+2);
			if (cpu_to_le16(ht_cap->ht_cap_info) & _HTCAP_40M_INTOLERANT_)
				priv->intolerant_timeout = 180;
		}
#endif
	}
#endif

	if (priv->site_survey->count >= MAX_BSS_NUM)
		return 0;

	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _DSSET_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	if (p != NULL)
		channel = *(p+2);
	else {
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_IE_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p !=  NULL)
			channel = *(p+2);
		else {
			if (priv->site_survey->ss_channel > 14 && !priv->site_survey->defered_ss)
				channel = priv->site_survey->ss_channel;
			else {
				DEBUG_INFO("Beacon/Probe rsp doesn't carry channel info\n");
				return SUCCESS;
			}
		}
	}


	for(i=0; i<priv->site_survey->count; i++) {
		if (!memcmp((void *)addr, priv->site_survey->bss[i].bssid, MACADDRLEN)) {
#if defined(CLIENT_MODE) && defined(WIFI_WMM) && defined(WMM_APSD)  //  WMM STA
			if ((OPMODE & WIFI_STATION_STATE) && QOS_ENABLE && APSD_ENABLE
				&& (channel == priv->site_survey->bss[i].channel)) {  // get WMM IE / WMM Parameter IE
				p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_;
				for (;;) {
					p = get_ie(p, _RSN_IE_1_, &len,
						pfrinfo->pktlen - (p - pframe));
					if (p != NULL) {
						if (!memcmp(p+2, WMM_PARA_IE, 6)) {
							priv->site_survey->bss[i].t_stamp[1] |= BIT(0);  //  set t_stamp[1] bit 0 when AP supports WMM

							if (*(p+8) & BIT(7))
								priv->site_survey->bss[i].t_stamp[1] |= BIT(3);  //  set t_stamp[1] bit 3 when AP supports UAPSD
							else
								priv->site_survey->bss[i].t_stamp[1] &= ~(BIT(3));  //  reset t_stamp[1] bit 3 when AP not support UAPSD
							break;
						}
					} else {
						priv->site_survey->bss[i].t_stamp[1] &= ~(BIT(0)|BIT(3));  //  reset t_stamp[1] bit 0 when AP not support WMM & UAPSD
						break;
					}
					p = p + len + 2;
				}
			}
#endif

			if ((unsigned char)pfrinfo->rssi > priv->site_survey->bss[i].rssi) {
				priv->site_survey->bss[i].rssi = (unsigned char)pfrinfo->rssi;
				#ifdef WIFI_SIMPLE_CONFIG
				priv->site_survey->wscie[i].rssi = priv->site_survey->bss[i].rssi;
				#endif

					priv->site_survey->wpa_ie[i].rssi = priv->site_survey->bss[i].rssi;
					priv->site_survey->rsn_ie[i].rssi = priv->site_survey->bss[i].rssi;
					priv->site_survey->rtk_p2p_ie[i].rssi = priv->site_survey->bss[i].rssi;

				if (channel == priv->site_survey->bss[i].channel) {
					if ((unsigned char)pfrinfo->sq > priv->site_survey->bss[i].sq)
						priv->site_survey->bss[i].sq = (unsigned char)pfrinfo->sq;
				} else {
					priv->site_survey->bss[i].channel = channel;
					priv->site_survey->bss[i].sq = (unsigned char)pfrinfo->sq;
				}
			}
			return SUCCESS;
		}
	}

	// checking SSID
	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _SSID_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);

	if ((p == NULL) ||		// NULL AP case 1
		(len == 0) ||		// NULL AP case 2
		(*(p+2) == '\0'))	// NULL AP case 3 (like 8181/8186)
	{
		if (priv->ss_req_ongoing && pmib->miscEntry.show_hidden_bss)
			hiddenAP = 1;
		else if (priv->auto_channel == 1)
			hiddenAP = 1;
		else {
#ifdef CLIENT_MODE
			if ((OPMODE & WIFI_STATION_STATE) &&
		    //!priv->ss_req_ongoing &&	//20131218 , mark it for hidden + dfs ch AP connection
			//		!priv->auto_channel &&
			is_passive_channel(priv , priv->pmib->dot11StationConfigEntry.dot11RegDomain, priv->site_survey->ss_channel))
			{
                /*For eg: ch60 now and hidden_ap_found=1 ; next time ss_timer will keep at ch60=> issue probe_req; next time ss_timer change to ch64 and on going...
				hidden_ap_found status machine as below
                            1) 0->1,1st ch60
                            next time
                            2)1->2, 2nd ch 60, and don't 2->1(chk in here) else will bring loop,ch 60,60,60,60,60......util hidden AP gone
                            */
                if(	priv->site_survey->hidden_ap_found != HIDE_AP_FOUND_DO_ACTIVE_SSAN){
	                //STADEBUG("hidden_ap_found=1\n");
					priv->site_survey->hidden_ap_found = HIDE_AP_FOUND;
				}
			}
#endif
			DEBUG_INFO("drop beacon/probersp due to null ssid\n");
			return 0;
		}
	}

	// if scan specific SSID
	if (priv->ss_ssidlen > 0)
    {
        /*when multiProfile enable,we send probe_req to all dev on profiles list , so chk if ssid match with ssid on profiles list*/
#ifdef SUPPORT_MULTI_PROFILE
		if (priv->pmib->ap_profile.enable_profile && priv->pmib->ap_profile.profile_num > 0) {
			found = 0;
			for(jdx=0;jdx<priv->pmib->ap_profile.profile_num;jdx++) {
				ssid_len = strlen(priv->pmib->ap_profile.profile[jdx].ssid);
				if ((ssid_len == len) && !memcmp(priv->pmib->ap_profile.profile[jdx].ssid, p+2, len)) {
					//STADEBUG("Found ssid=%s,jdx=%d\n", priv->pmib->ap_profile.profile[jdx].ssid , jdx);
					found = 1;
                    break;
                }
			}

			if(found == 0)
				return 0;
		}
		else
#endif
		if ((priv->ss_ssidlen != len) || memcmp(priv->ss_ssid, p+2, len)){
			/*cfg p2p cfg p2p*/
			{
                NDEBUG("ignore!\n");
    			return 0;
            }
        }
	}

#ifdef CLIENT_MODE
// mantis#2523
	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _SSID_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);

	if( p && (SSID_LEN == len) && !memcmp(SSID, p+2, len)) {
		memcpy(priv->rx_timestamp, pframe+WLAN_HDR_A3_LEN, 8);
	}
#endif

	//printk("priv->ss_ssid = %s, priv->ss_ssidlen=%d\n", priv->ss_ssid, priv->ss_ssidlen);

	// if scan specific SSID && WPA2 enabled
	if (priv->ss_ssidlen > 0) {
		// search WPA2 IE
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _RSN_IE_2_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p != NULL) {
			// RSN IE
			// 0	1	23	4567
			// ID	Len	Versin	GroupCipherSuite
#ifdef	CONFIG_IEEE80211W_CLI
			if((2 == WPA_GET_LE16(p+14))){ //AKM num >= 2
				if(!memcmp((p+20), SHA256_AKM_SUITE, sizeof(SHA256_AKM_SUITE))){
					priv->bss_support_sha256 = TRUE;
					add_sha256_akm(priv);
				}
			}else if(!memcmp((p+16), SHA256_AKM_SUITE, sizeof(SHA256_AKM_SUITE))){
				priv->bss_support_sha256 = TRUE;
				add_sha256_akm(priv);
			}else
				priv->bss_support_sha256 = FALSE;
#endif

			if ((len > 7) && (pmib->dot11RsnIE.rsnie[0] == _RSN_IE_2_) &&
					(pmib->dot11RsnIE.rsnie[7] != *(p+7)) &&
					!memcmp((p + 4), OUI2, 3)) {
				// set WPA2 Multicast Cipher as same as AP's
				//printk("WPA2 Multicast Cipher = %d\n", *(p+7));
				pmib->dot11RsnIE.rsnie[7] = *(p+7);
			}
#ifndef WITHOUT_ENQUEUE
			wpa2MulticastCipher.EventId = DOT11_EVENT_WPA2_MULTICAST_CIPHER;
			wpa2MulticastCipher.IsMoreEvent = 0;
			wpa2MulticastCipher.MulticastCipher = *(p+7);
			DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&wpa2MulticastCipher,
						sizeof(DOT11_WPA2_MULTICAST_CIPHER));
#endif
			event_indicate(priv, NULL, -1);

		}
	}

	// david, reported multicast cipher suite for WPA
	// if scan specific SSID && WPA2 enabled
	if (priv->ss_ssidlen > 0) {
		// search WPA IE, should skip that not real RSNIE (eg. Asus WL500g-Deluxe)
		p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_;
		len = 0;
		do {
			p = get_ie(p, _RSN_IE_1_, &len,
				pfrinfo->pktlen - (p - pframe));
			if (p != NULL) {
				if ((len > 11) && (pmib->dot11RsnIE.rsnie[0] == _RSN_IE_1_) &&
						(pmib->dot11RsnIE.rsnie[11] != *(p+11)) &&
						!memcmp((p + 2), OUI1, 4)) {
					// set WPA Multicast Cipher as same as AP's
					pmib->dot11RsnIE.rsnie[11] = *(p+11);

#ifndef WITHOUT_ENQUEUE
					wpaMulticastCipher.EventId = DOT11_EVENT_WPA_MULTICAST_CIPHER;
					wpaMulticastCipher.IsMoreEvent = 0;
					wpaMulticastCipher.MulticastCipher = *(p+11);
					DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&wpaMulticastCipher,
							sizeof(DOT11_WPA_MULTICAST_CIPHER));
#endif
					event_indicate(priv, NULL, -1);
				}
			}
			if (p != NULL)
				p = p + 2 + len;
		} while (p != NULL);
	}

	for(i=0; i<priv->available_chnl_num; i++) {
		if (channel == priv->available_chnl[i])
			break;
	}
	if (i == priv->available_chnl_num)	// receive the adjacent channel that is not our domain
		return 0;

	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _SUPPORTEDRATES_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	if (p != NULL) {
		for(i=0; i<len; i++) {
			if (p[2+i] & 0x80)
				basicrate |= get_bit_value_from_ieee_value(p[2+i] & 0x7f);
			supportrate |= get_bit_value_from_ieee_value(p[2+i] & 0x7f);
		}
	}

	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _EXT_SUPPORTEDRATES_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	if (p != NULL) {
		for(i=0; i<len; i++) {
			if (p[2+i] & 0x80)
				basicrate |= get_bit_value_from_ieee_value(p[2+i] & 0x7f);
			supportrate |= get_bit_value_from_ieee_value(p[2+i] & 0x7f);
		}
	}

	if (channel <= 14)
	{
		if (!(pmib->dot11BssType.net_work_type & WIRELESS_11B)){
			if (((basicrate & 0xff0) == 0) && ((supportrate & 0xff0) == 0)){
				return 0;
			}
		}

		if (!(pmib->dot11BssType.net_work_type & WIRELESS_11G)){
			if (((basicrate & 0xf) == 0) && ((supportrate & 0xf) == 0)){
				return 0;
			}
		}
	}


	/*
	 * okay, recording this bss...
	 */
	index = priv->site_survey->count;
	priv->site_survey->count++;

	memcpy(priv->site_survey->bss[index].bssid, addr, MACADDRLEN);

	if (hiddenAP) {
		priv->site_survey->bss[index].ssidlen = 0;
		memset((void *)(priv->site_survey->bss[index].ssid),0, 32);
	}
	else {
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _SSID_IE_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		priv->site_survey->bss[index].ssidlen = len;
		memcpy((void *)(priv->site_survey->bss[index].ssid), (void *)(p+2), len);
        /*add for sorting by profile */
        #ifdef WIFI_SIMPLE_CONFIG
		memcpy((void *)(priv->site_survey->wscie[index].ssid), (void *)(p+2), len);
        #endif

			memcpy((void *)(priv->site_survey->wpa_ie[index].ssid), (void *)(p+2), len);
			memcpy((void *)(priv->site_survey->rsn_ie[index].ssid), (void *)(p+2), len);
			memcpy((void *)(priv->site_survey->rtk_p2p_ie[index].ssid), (void *)(p+2), len);
	}

	// we use t_stamp to carry other info so don't get timestamp here
#if 0
	memcpy(&val32, (pframe + WLAN_HDR_A3_LEN), 4);
	priv->site_survey->bss[index].t_stamp[0] = le32_to_cpu(val32);

	memcpy(&val32, (pframe + WLAN_HDR_A3_LEN + 4), 4);
	priv->site_survey->bss[index].t_stamp[1] = le32_to_cpu(val32);
#endif

	memcpy(&val16, (pframe + WLAN_HDR_A3_LEN + 8 ), 2);
	priv->site_survey->bss[index].beacon_prd = le16_to_cpu(val16);

	memcpy(&val16, (pframe + WLAN_HDR_A3_LEN + 8 + 2), 2);
	priv->site_survey->bss[index].capability = le16_to_cpu(val16);

	if ((priv->site_survey->bss[index].capability & BIT(0)) &&
		!(priv->site_survey->bss[index].capability & BIT(1)))
		priv->site_survey->bss[index].bsstype = WIFI_AP_STATE;
	else if (!(priv->site_survey->bss[index].capability & BIT(0)) &&
		(priv->site_survey->bss[index].capability & BIT(1)))
		priv->site_survey->bss[index].bsstype = WIFI_ADHOC_STATE;
	else
		priv->site_survey->bss[index].bsstype = 0;

	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _TIM_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	if (p != NULL)
		priv->site_survey->bss[index].dtim_prd = *(p+3);

	priv->site_survey->bss[index].channel = channel;
	priv->site_survey->bss[index].basicrate = basicrate;
	priv->site_survey->bss[index].supportrate = supportrate;

	memcpy(priv->site_survey->bss[index].bdsa, sa, MACADDRLEN);

	priv->site_survey->bss[index].rssi = (unsigned char)pfrinfo->rssi;
	priv->site_survey->bss[index].sq = (unsigned char)pfrinfo->sq;

#ifdef WIFI_SIMPLE_CONFIG
	priv->site_survey->wscie[index].rssi = priv->site_survey->bss[index].rssi;
	priv->site_survey->wscie[index].chan = priv->site_survey->bss[index].channel;
#endif
		priv->site_survey->wpa_ie[index].rssi = priv->site_survey->bss[index].rssi;
		priv->site_survey->rsn_ie[index].rssi = priv->site_survey->bss[index].rssi;
		priv->site_survey->rtk_p2p_ie[index].rssi = priv->site_survey->bss[index].rssi;

	if (channel >= 36)
		priv->site_survey->bss[index].network |= WIRELESS_11A;
	else {
		if ((basicrate & 0xff0) || (supportrate & 0xff0))
			priv->site_survey->bss[index].network |= WIRELESS_11G;
		if ((basicrate & 0xf) || (supportrate & 0xf))
			priv->site_survey->bss[index].network |= WIRELESS_11B;
	}

	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_CAP_, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	if (p !=  NULL) {
		struct ht_cap_elmt *ht_cap=(struct ht_cap_elmt *)(p+2);
		if (cpu_to_le16(ht_cap->ht_cap_info) & _HTCAP_SUPPORT_CH_WDTH_)
			priv->site_survey->bss[index].t_stamp[1] |= BIT(1);
		else
			priv->site_survey->bss[index].t_stamp[1] &= ~(BIT(1));
		priv->site_survey->bss[index].network |= WIRELESS_11N;
		memcpy(&priv->site_survey->bss[index].ht_cap, ht_cap, 26);
	} else {
		priv->site_survey->bss[index].t_stamp[1] &= ~(BIT(1));
	}
#ifdef RTK_AC_SUPPORT
	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, EID_VHTCapability, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	if ((p !=  NULL) && (len <= sizeof(struct vht_cap_elmt))) {
		priv->site_survey->bss[index].network |= WIRELESS_11AC;
	}

	//Check if 80M AP
	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, EID_VHTOperation, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);

	priv->site_survey->bss[index].t_stamp[1] &= ~(BSS_BW_MASK << BSS_BW_SHIFT);

	if ((p !=  NULL) && (len <= sizeof(struct vht_oper_elmt))) {
		if (p[2] == 1) {
			priv->site_survey->bss[index].t_stamp[1] |= (HT_CHANNEL_WIDTH_AC_80 << BSS_BW_SHIFT);
		}
	}
#endif
	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_IE_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);

	if (p !=  NULL) {
		struct ht_info_elmt *ht_info=(struct ht_info_elmt *)(p+2);
		if (!(ht_info->info0 & _HTIE_STA_CH_WDTH_))
			priv->site_survey->bss[index].t_stamp[1] &= ~(BIT(1)|BIT(2));
		else {
			if ((ht_info->info0 & _HTIE_2NDCH_OFFSET_BL_) == _HTIE_2NDCH_OFFSET_NO_)
				priv->site_survey->bss[index].t_stamp[1] &= ~(BIT(1)|BIT(2));
			else
			if ((ht_info->info0 & _HTIE_2NDCH_OFFSET_BL_) == _HTIE_2NDCH_OFFSET_BL_)
				priv->site_survey->bss[index].t_stamp[1] |= BIT(2);
			else
				priv->site_survey->bss[index].t_stamp[1] &= ~(BIT(2));
		}
		memcpy(&priv->site_survey->bss[index].ht_info, ht_info, 22);
	}
	else
		priv->site_survey->bss[index].t_stamp[1] &= ~(BIT(1)|BIT(2));

	// get WPA/WPA2 information
	get_security_info(priv, pfrinfo, index);

	// Parse Multi Stage Element
	p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_; len = 0;
	for (;;)
	{
		unsigned char oui[] = { 0x00, 0x0d, 0x02 };
		unsigned char oui_type =7, ver1 = 0x01, ver2 = 0x00;
		p = get_ie(p, _RSN_IE_1_, &len,
			pfrinfo->pktlen - (p - pframe));
		if (p != NULL) {
			if (!memcmp(p+2, oui, 3) && (*(p+2+3) == oui_type)) {
				if ( (*(p+2+4) == ver1) && (*(p+2+5) == ver2))
				{
					switch(*(p+2+6)) {
						case 0x80: priv->site_survey->bss[index].stage = 1;
									break;
						case 0x40: priv->site_survey->bss[index].stage = 2;
									break;
						case 0x20: priv->site_survey->bss[index].stage = 3;
									break;
						case 0x10: priv->site_survey->bss[index].stage = 4;
									break;
						case 0x08: priv->site_survey->bss[index].stage = 5;
									break;
						default:
							priv->site_survey->bss[index].stage = 0;
					};
				} else {
					priv->site_survey->bss[index].stage = 0;
				}
				break;
			}
		}
		else
			break;
		p = p + len + 2;
	}

			/*cfg p2p*/

#ifdef WIFI_WMM  //  WMM STA
	if (QOS_ENABLE) {  // get WMM IE / WMM Parameter IE
		p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_;
		for (;;) {
			p = get_ie(p, _RSN_IE_1_, &len,
				pfrinfo->pktlen - (p - pframe));
			if (p != NULL) {
				if ((!memcmp(p+2, WMM_IE, 6)) || (!memcmp(p+2, WMM_PARA_IE, 6))) {
					priv->site_survey->bss[index].t_stamp[1] |= BIT(0);  //  set t_stamp[1] bit 0 when AP supports WMM
#if defined(CLIENT_MODE) && defined(WMM_APSD)
					if ((OPMODE & WIFI_STATION_STATE) && APSD_ENABLE) {
						if (!memcmp(p+2, WMM_PARA_IE, 6)) {
							if (*(p+8) & BIT(7))
								priv->site_survey->bss[index].t_stamp[1] |= BIT(3);  //  set t_stamp[1] bit 3 when AP supports UAPSD
							else
								priv->site_survey->bss[index].t_stamp[1] &= ~(BIT(3));  //  reset t_stamp[1] bit 3 when AP not support UAPSD
							break;
						} else {
							priv->site_survey->bss[index].t_stamp[1] &= ~(BIT(3));  //  reset t_stamp[1] bit 3 when AP not support UAPSD
						}
					} else
#endif
						break;
				}
			} else {
				priv->site_survey->bss[index].t_stamp[1] &= ~(BIT(0));  //  reset t_stamp[1] bit 0 when AP not support WMM
#if defined(CLIENT_MODE) && defined(WMM_APSD)
				if ((OPMODE & WIFI_STATION_STATE) && APSD_ENABLE)
					priv->site_survey->bss[index].t_stamp[1] &= ~(BIT(3));  //  reset t_stamp[1] bit 3 when AP not support UAPSD
#endif
				break;
			}
			p = p + len + 2;
		}
	}
#endif

#ifdef CONFIG_IEEE80211V_CLI
	if(WNM_ENABLE) {
		unsigned char ext_cap[8];
		memset(ext_cap, 0, 8);
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _EXTENDED_CAP_IE_, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if(p != NULL) {
			memcpy(ext_cap, p+2, 8);
			if(ext_cap[2] & _WNM_BSS_TRANS_SUPPORT_) {
				priv->site_survey->bss[index].t_stamp[1] |= BSS_TRANS_SUPPORT;
			}
		}
	}
#endif

			priv->site_survey->bss[index].rsn_ie_len = 0;			/*cfg p2p cfg p2p*/
			priv->site_survey->bss[index].wpa_ie_len = 0;			/*cfg p2p cfg p2p*/

			p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_;
			p = get_ie(p, _RSN_IE_2_, &len,
				pfrinfo->pktlen - (p - pframe));
			if ((p != NULL) && (len > 7)) {

                priv->site_survey->bss[index].rsn_ie_len = len + 2;			/*cfg p2p cfg p2p*/
                memcpy(priv->site_survey->bss[index].rsn_ie , p, len + 2);			/*cfg p2p cfg p2p*/

			}

			len = 0;
			p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_;
			do {
				p = get_ie(p, _RSN_IE_1_, &len,
					pfrinfo->pktlen - (p - pframe));
				if ((p != NULL) && (len > 11) 			/*cfg p2p cfg p2p*/
					 && (!memcmp((p + 2), OUI1, 4)) ) {
                    priv->site_survey->bss[index].wpa_ie_len = len+2;
					memcpy(priv->site_survey->bss[index].wpa_ie, p, len + 2);
				}			/*cfg p2p cfg p2p*/
				if (p != NULL)
					p = p + 2 + len;
			} while (p != NULL);





	return SUCCESS;
}


void assign_tx_rate(struct rtl8192cd_priv *priv, struct stat_info *pstat, struct rx_frinfo *pfrinfo)
{
	int tx_rate=0;
	UINT8 rate;
	int auto_rate;

	{
		auto_rate = priv->pmib->dot11StationConfigEntry.autoRate;
		tx_rate = priv->pmib->dot11StationConfigEntry.fixedTxRate;
	}

	if (auto_rate ||
#ifdef RTK_AC_SUPPORT
		( is_fixedVHTTxRate(priv, pstat) && !(pstat->vht_cap_len)) ||
#endif
		( should_restrict_Nrate(priv, pstat) && is_fixedMCSTxRate(priv, pstat))) {
#if 0
		// if auto rate, select highest or lowest rate depending on rssi
		if (pfrinfo && pfrinfo->rssi > 30)
			pstat->current_tx_rate = find_rate(priv, pstat, 1, 0);
		else
			pstat->current_tx_rate = find_rate(priv, pstat, 0, 0);
#endif
		pstat->current_tx_rate = find_rate(priv, pstat, 1, 0);

	}
	else {
		// see if current fixed tx rate of mib is existed in supported rates set
		rate = get_rate_from_bit_value(tx_rate);
		if (match_supp_rate(pstat->bssrateset, pstat->bssratelen, rate))
			tx_rate = (int)rate;
		if (tx_rate == 0) // if not found, use highest supported rate for current tx rate
			tx_rate = find_rate(priv, pstat, 1, 0);

		if((GET_CHIP_VER(priv)== VERSION_8812E)||(GET_CHIP_VER(priv)== VERSION_8881A)||(GET_CHIP_VER(priv)== VERSION_8814A) || (GET_CHIP_VER(priv)== VERSION_8723B) || (GET_CHIP_VER(priv)== VERSION_8822B)){
			pstat->current_tx_rate = rate;
		}else
		{
			pstat->current_tx_rate = tx_rate;
		}
// ToDo:
// fixed 2T rate, but STA is 1R...
	}

	if ((pstat->MIMO_ps & _HT_MIMO_PS_STATIC_) && is_2T_rate(pstat->current_tx_rate)) {
#ifdef RTK_AC_SUPPORT
		if( is_VHT_rate(pstat->current_tx_rate)){
			pstat->current_tx_rate = _NSS1_MCS9_RATE_;
		}else
#endif
		{
			pstat->current_tx_rate = _MCS7_RATE_;	// when HT MIMO Static power save is set and rate > MCS7, fix rate to MCS7
		}
	}


	if (pfrinfo)
		pstat->rssi = pfrinfo->rssi;	// give the initial value to pstat->rssi

	pstat->ht_current_tx_info = 0;
	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) && pstat->ht_cap_len) {
		if (priv->pshare->is_40m_bw && ((pstat->tx_bw == HT_CHANNEL_WIDTH_20_40)||(pstat->tx_bw == HT_CHANNEL_WIDTH_80))) {
			pstat->ht_current_tx_info |= TX_USE_40M_MODE;
			if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor40M &&
				(pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SHORTGI_40M_)))
				pstat->ht_current_tx_info |= TX_USE_SHORT_GI;
		}
		else {
			if (priv->pmib->dot11nConfigEntry.dot11nShortGIfor20M &&
				(pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SHORTGI_20M_)))
				pstat->ht_current_tx_info |= TX_USE_SHORT_GI;
		}
	}

	if (priv->pshare->rf_ft_var.rssi_dump && pfrinfo)
	{
#ifdef  RTK_AC_SUPPORT
		if(is_VHT_rate(pstat->current_tx_rate))
			printk("[%d] rssi=%d%% assign rate %s%d %d\n", pstat->aid, pfrinfo->rssi,
			"VHT NSS", (((pstat->current_tx_rate-VHT_RATE_ID)/10)+1), ((pstat->current_tx_rate-VHT_RATE_ID)%10));
		else
#endif
		printk("[%d] rssi=%d%% assign rate %s%d\n", pstat->aid, pfrinfo->rssi,
			is_MCS_rate(pstat->current_tx_rate)? "MCS" : "",
			is_MCS_rate(pstat->current_tx_rate)? (pstat->current_tx_rate-HT_RATE_ID) : pstat->current_tx_rate/2);
	}
}


// Assign aggregation method automatically.
// We according to the following rule:
// 1. Rtl8190: AMPDU
// 2. Broadcom: AMSDU
// 3. Station who supports only 4K AMSDU receiving: AMPDU
// 4. Others: AMSDU

#if defined(RTK_AC_SUPPORT) && defined(SUPPORT_TX_AMSDU)
unsigned int amsduUpperTP(struct rtl8192cd_priv *priv, struct stat_info *pstat)
{
	unsigned int baseTP = 433*priv->pshare->rf_ft_var.amsdu_th/100;
	u1Byte              rf_mimo_mode    = get_rf_mimo_mode(priv);
	u1Byte				mimo_mode;

	if (get_rf_NTx(pstat->sta_mimo_mode) > get_rf_NTx(rf_mimo_mode))
		mimo_mode = rf_mimo_mode;
	else
		mimo_mode = pstat->sta_mimo_mode;

	if(mimo_mode == MIMO_4T4R)
		return 4*baseTP;
	else if(mimo_mode == MIMO_3T3R)
		return 3*baseTP;
	else if(mimo_mode == MIMO_2T2R)
		return 2*baseTP;
	else if(mimo_mode == MIMO_1T1R)
		return baseTP;

}

unsigned int amsduLowerTP(struct rtl8192cd_priv *priv, struct stat_info *pstat)
{
	unsigned int baseTP = 433*priv->pshare->rf_ft_var.amsdu_th2/100;
	u1Byte              rf_mimo_mode    = get_rf_mimo_mode(priv);
	u1Byte				mimo_mode;

	if (get_rf_NTx(pstat->sta_mimo_mode) > get_rf_NTx(rf_mimo_mode))
		mimo_mode = rf_mimo_mode;
	else
		mimo_mode = pstat->sta_mimo_mode;

	if(mimo_mode == MIMO_4T4R)
		return 4*baseTP;
	else if(mimo_mode == MIMO_3T3R)
		return 3*baseTP;
	else if(mimo_mode == MIMO_2T2R)
		return 2*baseTP;
	else if(mimo_mode == MIMO_1T1R)
		return baseTP;

}
#endif

void assign_aggre_mthod(struct rtl8192cd_priv *priv, struct stat_info *pstat)
{
	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) && pstat->ht_cap_len) {
#ifdef RTK_AC_SUPPORT //for 11ac logo
		if ((AMPDU_ENABLE) && (AMSDU_ENABLE >= 2) ) {
#ifdef SUPPORT_TX_AMSDU
			if(pstat->vht_cap_len && pstat->AMSDU_AMPDU_support) {
				if(priv->pshare->iot_mode_enable == 0) {
					pstat->aggre_mthd = AGGRE_MTHD_MPDU;
				} else {
#if (MU_BEAMFORMING_SUPPORT == 1)
					if(pstat->muFlagForAMSDU) {
						pstat->aggre_mthd = AGGRE_MTHD_MPDU;
						pstat->muFlagForAMSDU = 0;
					} else
#endif
					if((pstat->aggre_mthd != AGGRE_MTHD_MPDU_AMSDU) && ((pstat->tx_avarage >> 17) >= amsduUpperTP(priv, pstat))) {
						//printk("TP:%d Mbps --> amsdu mode\n", (pstat->tx_avarage >> 17));
						pstat->aggre_mthd = AGGRE_MTHD_MPDU_AMSDU;
						if(GET_CHIP_VER(priv) == VERSION_8822B) {
							pstat->maxAggNum = (pstat->maxAggNumOrig >> 1);
						}
					} else if((pstat->aggre_mthd == AGGRE_MTHD_MPDU_AMSDU) && (((pstat->tx_avarage >> 17) < amsduLowerTP(priv, pstat)))) {
						//printk("TP:%d Mbps --> ampdu mode\n", (pstat->tx_avarage >> 17));
						pstat->aggre_mthd = AGGRE_MTHD_MPDU;
						if(GET_CHIP_VER(priv) == VERSION_8822B) {
							pstat->maxAggNum = pstat->maxAggNumOrig;
						}
					}
				}
			}
			else
#endif
				pstat->aggre_mthd = AGGRE_MTHD_MPDU;
		}
		else
#endif // RTK_AC_SUPPORT

		if ((AMPDU_ENABLE == 1) || (AMSDU_ENABLE == 1))		// auto assignment
			pstat->aggre_mthd = AGGRE_MTHD_MPDU;
		else if ((AMPDU_ENABLE >= 2) && (AMSDU_ENABLE == 0))
			pstat->aggre_mthd = AGGRE_MTHD_MPDU;
		else if ((AMPDU_ENABLE == 0) && (AMSDU_ENABLE >= 2))
			pstat->aggre_mthd = AGGRE_MTHD_MSDU;									//5.2.38
		else
			pstat->aggre_mthd = AGGRE_MTHD_NONE;
	}
	else
		pstat->aggre_mthd = AGGRE_MTHD_NONE;

	if (should_restrict_Nrate(priv, pstat) && (pstat->aggre_mthd != AGGRE_MTHD_NONE))
		pstat->aggre_mthd = AGGRE_MTHD_NONE;

// Client mode IOT issue, Button 2009.07.17
// we won't restrict N rate with 8190
#ifdef CLIENT_MODE
	if(OPMODE & WIFI_STATION_STATE)
	{
		if((pstat->IOTPeer !=HT_IOT_PEER_REALTEK_92SE) && pstat->is_realtek_sta && pstat->is_legacy_encrpt)
			pstat->aggre_mthd = AGGRE_MTHD_NONE;
	}
#endif

//	if(pstat->sta_in_firmware != 1 && priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _NO_PRIVACY_)
//		pstat->aggre_mthd = AGGRE_MTHD_NONE;
}


void assign_aggre_size(struct rtl8192cd_priv *priv, struct stat_info *pstat)
{
    int sta_mimo_mode;
    if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) && pstat->ht_cap_len) {
        if ((priv->pmib->dot11nConfigEntry.dot11nAMPDUSendSz == 8) ||
            (priv->pmib->dot11nConfigEntry.dot11nAMPDUSendSz == 16) ||
            (priv->pmib->dot11nConfigEntry.dot11nAMPDUSendSz == 32)) {
            if (priv->pmib->dot11nConfigEntry.dot11nAMPDUSendSz == 8)
                pstat->diffAmpduSz = 0x44444444;
            else if (priv->pmib->dot11nConfigEntry.dot11nAMPDUSendSz == 16)
                pstat->diffAmpduSz = 0x88888888;
            else
                pstat->diffAmpduSz = 0xffffffff;
        } else {
            unsigned int ampdu_para = pstat->ht_cap_buf.ampdu_para & 0x03;
            pstat->diffAmpduSz = RTL_R32(AGGLEN_LMT);
            if ((!ampdu_para) || (ampdu_para == 1)) {
                if ((pstat->diffAmpduSz & 0xf) > 4*(ampdu_para+1))
                    pstat->diffAmpduSz = (pstat->diffAmpduSz & ~0xf) | 0x4*(ampdu_para+1);
                if (((pstat->diffAmpduSz & 0xf0) >> 4) > 4*(ampdu_para+1))
                    pstat->diffAmpduSz = (pstat->diffAmpduSz & ~0xf0) | 0x40*(ampdu_para+1);
                if (((pstat->diffAmpduSz & 0xf00) >> 8) > 4*(ampdu_para+1))
                    pstat->diffAmpduSz = (pstat->diffAmpduSz & ~0xf00) | 0x400*(ampdu_para+1);
                if (((pstat->diffAmpduSz & 0xf000) >> 12) > 4*(ampdu_para+1))
                    pstat->diffAmpduSz = (pstat->diffAmpduSz & ~0xf000) | 0x4000*(ampdu_para+1);
                if (((pstat->diffAmpduSz & 0xf0000) >> 16) > 4*(ampdu_para+1))
                    pstat->diffAmpduSz = (pstat->diffAmpduSz & ~0xf0000) | 0x40000*(ampdu_para+1);
                if (((pstat->diffAmpduSz & 0xf00000) >> 20) > 4*(ampdu_para+1))
                    pstat->diffAmpduSz = (pstat->diffAmpduSz & ~0xf00000) | 0x400000*(ampdu_para+1);
                if (((pstat->diffAmpduSz & 0xf000000) >> 24) > 4*(ampdu_para+1))
                    pstat->diffAmpduSz = (pstat->diffAmpduSz & ~0xf000000) | 0x4000000*(ampdu_para+1);
                if (((pstat->diffAmpduSz & 0xf0000000) >> 28) > 4*(ampdu_para+1))
                    pstat->diffAmpduSz = (pstat->diffAmpduSz & ~0xf0000000) | 0x40000000*(ampdu_para+1);
            }
        }
        DEBUG_INFO("assign aggregation size: %d\n", 8<<(pstat->ht_cap_buf.ampdu_para & 0x03));
    }

    //if (pstat->ht_cap_len)
    //	pstat->maxAggNum = ((1<<(pstat->ht_cap_buf.ampdu_para & 0x03))*5);
    if (pstat->ht_cap_len) {
        if((pstat->ht_cap_buf.ampdu_para & 0x03) == 3) {

                pstat->maxAggNum = 42;

        }
        else if((pstat->ht_cap_buf.ampdu_para & 0x03) == 2)
            pstat->maxAggNum = 21;
        else if((pstat->ht_cap_buf.ampdu_para & 0x03) == 1)
            pstat->maxAggNum = 10;
        else
            pstat->maxAggNum = 5;
    }
#ifdef RTK_AC_SUPPORT
    if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC) && (pstat->vht_cap_len)) {
        pstat->maxAggNum = ((1<<((cpu_to_le32(pstat->vht_cap_buf.vht_cap_info) & 0x3800000)>>MAX_RXAMPDU_FACTOR_S))*5);

    }

#endif

    if(pstat->maxAggNum >= 0x3F)
        pstat->maxAggNum = 0x3F;

	pstat->maxAggNumOrig= pstat->maxAggNum;
}

#ifdef SUPPORT_MONITOR
void rtl8192cd_chan_switch_timer(unsigned long task_priv)
{
	struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)task_priv;

	if (!(priv->drv_state & DRV_STATE_OPEN))
		return;
	if(priv->is_monitor_mode==TRUE)
	{
		if(((priv->chan_num%priv->available_chnl_num)==0)&&(priv->chan_num>0))
			priv->chan_num = 0;
		else
			priv->chan_num++;

		priv->pshare->CurrentChannelBW = HT_CHANNEL_WIDTH_20;
		SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);
		SwChnl(priv, priv->available_chnl[priv->chan_num], priv->pshare->offset_2nd_chan);
		mod_timer(&priv->chan_switch_timer, jiffies + RTL_MILISECONDS_TO_JIFFIES(priv->pmib->miscEntry.chan_switch_time));
	}
}
#endif

#ifndef USE_WEP_DEFAULT_KEY
void set_keymapping_wep(struct rtl8192cd_priv *priv, struct stat_info *pstat)
{
	struct wifi_mib	*pmib = GET_MIB(priv);

//	if ((GET_ROOT(priv)->pmib->dot11OperationEntry.opmode & WIFI_AP_STATE) &&
	if (!SWCRYPTO && !IEEE8021X_FUN &&
		((pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_) ||
		 (pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_)))
	{
		pstat->dot11KeyMapping.dot11Privacy = pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm;
		pstat->keyid = pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex;
		if (pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_) {
			pstat->dot11KeyMapping.dot11EncryptKey.dot11TTKeyLen = 5;
			memcpy(pstat->dot11KeyMapping.dot11EncryptKey.dot11TTKey.skey,
				   pmib->dot11DefaultKeysTable.keytype[pstat->keyid].skey, 5);
		}
		else {
			pstat->dot11KeyMapping.dot11EncryptKey.dot11TTKeyLen = 13;
			memcpy(pstat->dot11KeyMapping.dot11EncryptKey.dot11TTKey.skey,
				   pmib->dot11DefaultKeysTable.keytype[pstat->keyid].skey, 13);
		}

		DEBUG_INFO("going to set %s unicast key for sta %02X%02X%02X%02X%02X%02X, id=%d\n",
			(pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_)?"WEP40":"WEP104",
			pstat->hwaddr[0], pstat->hwaddr[1], pstat->hwaddr[2],
			pstat->hwaddr[3], pstat->hwaddr[4], pstat->hwaddr[5], pstat->keyid);
		if (!SWCRYPTO) {
			int retVal=0;

#ifdef USE_WEP_4_KEYS
			{
				if(priv->pshare->total_cam_entry - priv->pshare->CamEntryOccupied >=4) {
					int keyid=0;
					for(;keyid<4; keyid++) {
						if (CamDeleteOneEntry(priv, pstat->hwaddr, keyid, 0)) {
							priv->pshare->CamEntryOccupied--;
						}
						if (CamAddOneEntry(priv, pstat->hwaddr, keyid,
							pstat->dot11KeyMapping.dot11Privacy<<2, 0,
							pmib->dot11DefaultKeysTable.keytype[keyid].skey)) {
							priv->pshare->CamEntryOccupied++;
							retVal ++;
						}
					}
				}
				if( retVal ==4) {
					pstat->dot11KeyMapping.keyInCam = TRUE;
				} else {
					int keyid=0;
					for(;keyid<4; keyid++) {
						if (CamDeleteOneEntry(priv, pstat->hwaddr, keyid, 0))
							priv->pshare->CamEntryOccupied--;
					}
					pstat->dot11KeyMapping.keyInCam = FALSE;
				}
			}
			if (pstat->aggre_mthd != AGGRE_MTHD_NONE)
				pstat->aggre_mthd = AGGRE_MTHD_NONE;
#else
			retVal = CamDeleteOneEntry(priv, pstat->hwaddr, pstat->keyid, 0);
			if (retVal) {
				priv->pshare->CamEntryOccupied--;
				pstat->dot11KeyMapping.keyInCam = FALSE;
			}
			retVal = CamAddOneEntry(priv, pstat->hwaddr, pstat->keyid,
				pstat->dot11KeyMapping.dot11Privacy<<2, 0, pstat->dot11KeyMapping.dot11EncryptKey.dot11TTKey.skey);
			if (retVal) {
				priv->pshare->CamEntryOccupied++;
				pstat->dot11KeyMapping.keyInCam = TRUE;
			}
			else {
				if (pstat->aggre_mthd != AGGRE_MTHD_NONE)
					pstat->aggre_mthd = AGGRE_MTHD_NONE;
			}
#endif
		}
	}
}
#endif


/*-----------------------------------------------------------------------------
OnAssocReg:
	--> Reply DeAuth or AssocRsp
Capability Info, Listen Interval, SSID, SupportedRates
------------------------------------------------------------------------------*/
unsigned int OnAssocReq(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
	struct wifi_mib		*pmib;
	struct stat_info	*pstat;
	unsigned char		*pframe, *p;
	unsigned char		rsnie_hdr[4]={0x00, 0x50, 0xf2, 0x01};
#ifdef RTL_WPA2
	unsigned char		rsnie_hdr_wpa2[2]={0x01, 0x00};
#endif
#ifdef HS2_SUPPORT
	unsigned char		rsnie_hdr_OSEN[4]={0x50, 0x6F, 0x9A, 0x12};
#endif
	int		len;
	unsigned long		flags;
	DOT11_ASSOCIATION_IND     Association_Ind;
	DOT11_REASSOCIATION_IND   Reassociation_Ind;
	unsigned char		supportRate[32];
	int					supportRateNum;
	unsigned int		status = _STATS_SUCCESSFUL_;
	unsigned short		frame_type, ie_offset=0, val16;
	unsigned int z = 0;

	pmib = GET_MIB(priv);
	pframe = get_pframe(pfrinfo);
	pstat = get_stainfo(priv, GetAddr2Ptr(pframe));

	if (!(OPMODE & WIFI_AP_STATE))
		return FAIL;


	if (pmib->miscEntry.func_off || pmib->miscEntry.raku_only)
		return FAIL;
#ifdef STA_ROAMING_CHECK
    if (pfrinfo->rssi < priv->pmib->dot11StationConfigEntry.staAssociateRSSIThreshold + priv->pmib->dot11StationConfigEntry.staRoamingRSSIGap) {
#else
    if (pfrinfo->rssi < priv->pmib->dot11StationConfigEntry.staAssociateRSSIThreshold) {
#endif
#ifdef STA_ASSOC_STATISTIC
		add_reject_sta(priv,GetAddr2Ptr(pframe), pfrinfo->rssi);
#endif
		return FAIL;
	}
	if (priv->pshare->rf_ft_var.dfs_det_period)
		priv->det_asoc_clear = 500 / priv->pshare->rf_ft_var.dfs_det_period;
	else
		priv->det_asoc_clear = 50;



//Ignore AssocReq during 4-WAY Handshake for some phones' connection issue
if((pstat) && (pstat->wpa_sta_info->state == PSK_STATE_PTKINITNEGOTIATING))
	return;

	frame_type = GetFrameSubType(pframe);

	if (frame_type == WIFI_ASSOCREQ)
		ie_offset = _ASOCREQ_IE_OFFSET_;
	else // WIFI_REASSOCREQ
		ie_offset = _REASOCREQ_IE_OFFSET_;

	if (pstat == (struct stat_info *)NULL)
	{
		status = _RSON_CLS2_;
		goto asoc_class2_error;
	}

	// check if this stat has been successfully authenticated/assocated
	if (!((pstat->state) & WIFI_AUTH_SUCCESS))
	{
		status = _RSON_CLS2_;
		goto asoc_class2_error;
	}

	if (priv->assoc_reject_on)
	{
		status = _STATS_OTHER_;
		goto OnAssocReqFail;
	}
#ifdef CONFIG_IEEE80211W
	if(((pstat->state) & WIFI_ASOC_STATE) &&
		pstat->isPMF &&
		!pstat->sa_query_timed_out &&
		pstat->sa_query_count) 	{
		check_sa_query_timeout(pstat);
	}

	if(((pstat->state) & WIFI_ASOC_STATE) &&
		pstat->isPMF &&
		!pstat->sa_query_timed_out) {

		status = _STATS_ASSOC_REJ_TEMP_;
		if(pstat->sa_query_count == 0) {
			pstat->sa_query_start = jiffies;
			pstat->sa_query_end = jiffies + RTL_MILISECONDS_TO_JIFFIES(SA_QUERY_MAX_TO);
		}
		if (frame_type == WIFI_ASSOCREQ)
			issue_asocrsp(priv, status, pstat, WIFI_ASSOCRSP);
		else
			issue_asocrsp(priv, status, pstat, WIFI_REASSOCRSP);

		if(pstat->sa_query_count == 0) {
			//PMFDEBUG("sa_query_end=%lu, sa_query_start=%lu\n", pstat->sa_query_end, pstat->sa_query_start);
			pstat->sa_query_count++;
			issue_SA_Query_Req(priv->dev,pstat->hwaddr);


			if(timer_pending(&pstat->SA_timer))
				del_timer(&pstat->SA_timer);

			pstat->SA_timer.data = (unsigned long) pstat;
			pstat->SA_timer.function = rtl8192cd_sa_query_timer;
			mod_timer(&pstat->SA_timer, jiffies + RTL_MILISECONDS_TO_JIFFIES(SA_QUERY_RETRY_TO));

		}
		return FAIL;
	}
	pstat->sa_query_timed_out = 0;
#endif
#ifdef CONFIG_RTL_WLAN_DOS_FILTER
	if (block_sta_time)
	{
		int i;
		for (i=0; i<MAX_BLOCK_MAC;i++)
		{
			if (memcmp(pstat->hwaddr, block_mac[i], 6) == 0)
			{
				status = _STATS_OTHER_;
				goto OnAssocReqFail;
			}
		}
	}
#endif

#ifdef CONFIG_IEEE80211V
	if(WNM_ENABLE) {
		set_staBssTransCap(pstat, pframe, (pfrinfo->pktlen - WLAN_HDR_A3_LEN - ie_offset), ie_offset);
	}
#endif




	/* Rate adpative algorithm */
	if (pstat->check_init_tx_rate)
		pstat->check_init_tx_rate = 0;

	// now we should check all the fields...

	// checking SSID
	p = get_ie(pframe + WLAN_HDR_A3_LEN + ie_offset, _SSID_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - ie_offset);

	if (p == NULL)
	{
		status = _STATS_FAILURE_;
		goto OnAssocReqFail;
	}

	if (len == 0) // broadcast ssid, however it is not allowed in assocreq
		status = _STATS_FAILURE_;
	else
	{
		// check if ssid match
		if (memcmp((void *)(p+2), SSID, SSID_LEN))
			status = _STATS_FAILURE_;

		if (len != SSID_LEN)
			status = _STATS_FAILURE_;
	}

	// check if the supported is ok
	p = get_ie(pframe + WLAN_HDR_A3_LEN + ie_offset, _SUPPORTEDRATES_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - ie_offset);

	if (len > 8)
		status = _STATS_RATE_FAIL_;
	else if (p == NULL) {
		DEBUG_WARN("Rx a sta assoc-req which supported rate is empty!\n");
		// use our own rate set as statoin used
		memcpy(supportRate, AP_BSSRATE, AP_BSSRATE_LEN);
		supportRateNum = AP_BSSRATE_LEN;
	}
	else {
		memcpy(supportRate, p+2, len);
		supportRateNum = len;

		p = get_ie(pframe + WLAN_HDR_A3_LEN + ie_offset, _EXT_SUPPORTEDRATES_IE_ , &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - ie_offset);
		if ((p !=  NULL) && (len <= 8)) {
			memcpy(supportRate+supportRateNum, p+2, len);
			supportRateNum += len;
		}
	}




#if 0
	{
	if (check_basic_rate(priv, supportRate, supportRateNum) == FAIL) {		// check basic rate. jimmylin 2004/12/02
		DEBUG_WARN("Rx a sta assoc-req which basic rates not match! %02X%02X%02X%02X%02X%02X\n",
			pstat->hwaddr[0],pstat->hwaddr[1],pstat->hwaddr[2],pstat->hwaddr[3],pstat->hwaddr[4],pstat->hwaddr[5]);
		if (priv->pmib->dot11OperationEntry.wifi_specific) {
			status = _STATS_RATE_FAIL_;
			goto OnAssocReqFail;
		}
	}
	}
#endif
	get_matched_rate(priv, supportRate, &supportRateNum, 0);
	update_support_rate(pstat, supportRate, supportRateNum);

	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) &&
		!isErpSta(pstat) &&
		(priv->pmib->dot11StationConfigEntry.legacySTADeny & WIRELESS_11B)) {
		status = _STATS_RATE_FAIL_;
		goto OnAssocReqFail;
	}

	val16 = cpu_to_le16(*(unsigned short*)((unsigned long)pframe + WLAN_HDR_A3_LEN));
	if (!(val16 & BIT(5))) // NOT use short preamble
		pstat->useShortPreamble = 0;
	else
		pstat->useShortPreamble = 1;

	pstat->state |= WIFI_ASOC_STATE;

	if (status != _STATS_SUCCESSFUL_)
		goto OnAssocReqFail;


    	// now the station is qualified to join our BSS...

#ifdef WIFI_WMM
	// check if there is WMM IE
	if (QOS_ENABLE) {
		p = pframe + WLAN_HDR_A3_LEN + ie_offset; len = 0;
		for (;;) {
			p = get_ie(p, _RSN_IE_1_, &len,
				pfrinfo->pktlen - (p - pframe));
			if (p != NULL) {
				if (!memcmp(p+2, WMM_IE, 6)) {
					pstat->QosEnabled = 1;
#ifdef WMM_APSD
					if (APSD_ENABLE)
						pstat->apsd_bitmap = *(p+8) & 0x0f;		// get QSTA APSD bitmap
#endif
					break;
				}
			}
			else {
				pstat->QosEnabled = 0;
#ifdef WMM_APSD
				pstat->apsd_bitmap = 0;
#endif
				break;
			}
			p = p + len + 2;
		}
	}
	else {
		pstat->QosEnabled = 0;
#ifdef WMM_APSD
		pstat->apsd_bitmap = 0;
#endif
	}
#endif
#ifdef RTK_AC_SUPPORT
	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC)
	{
		p = get_ie(pframe + WLAN_HDR_A3_LEN + ie_offset, EID_VHTCapability, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - ie_offset);

		if ((p !=  NULL) && (len <= sizeof(struct vht_cap_elmt))) {
			pstat->vht_cap_len = len;
			memcpy((unsigned char *)&pstat->vht_cap_buf, p+2, len);
			/* For debugging
			SDEBUG("Receive vht_cap len = %d \n",len);
			if (pstat->vht_cap_buf.vht_cap_info & cpu_to_le32(_VHTCAP_RX_STBC_CAP_)) {
				SDEBUG("STA support RX STBC\n");
			}
			if (pstat->vht_cap_buf.vht_cap_info & cpu_to_le32(_VHTCAP_RX_LDPC_CAP_)) {
				SDEBUG("STA support RX LDPC\n");
			}
			*/
		}
#if 1//for 2.4G VHT IE
		else
		{
			if(priv->pmib->dot11RFEntry.phyBandSelect==PHY_BAND_2G)
			{
				unsigned char vht_ie_id[] = {0x00, 0x90, 0x4c};
				p = pframe + WLAN_HDR_A3_LEN + ie_offset;
				len = 0;

				for (;;)
				{
					//printk("\nOUI limit=%d\n", pfrinfo->pktlen - (p - pframe));
					p = get_ie(p, _RSN_IE_1_, &len, pfrinfo->pktlen - (p - pframe));
					if (p != NULL) {
						#if 0//debug
						int i;
						for(i=0; i<len+2; i++){
							if((i%8)==0)
								panic_printk("\n");
							panic_printk("%02x ", *(p+i));
						}
						panic_printk("\nlen=%d vht_len=%d\n",len, sizeof(struct vht_cap_elmt));
						#endif
						// Bcom VHT IE
						// {0xdd, 0x13} RSN_IE_1-221, length
						// {0x00, 0x90, 0x4c} oui // {0x04 0x08 } unknow
						// {0xbf, 0x0c} element id, length
						if (!memcmp(p+2, vht_ie_id, 3) && (*(p+7) == 0xbf) && ((*(p+8)) <= sizeof(struct vht_cap_elmt))) {
							pstat->vht_cap_len = *(p+8);
							memcpy((unsigned char *)&pstat->vht_cap_buf, p+9, pstat->vht_cap_len);
							//panic_printk("\n get vht ie in OUI!!! len=%d\n\n", pstat->vht_cap_len);
							break;
						}
					}
					else
						break;
					p = p + len + 2;
				}
			}
		}
#endif
		p = get_ie(pframe + WLAN_HDR_A3_LEN + ie_offset, EID_VHTOperation, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - ie_offset);

		if ((p !=  NULL) && (len <= sizeof(struct vht_oper_elmt))) {
			pstat->vht_oper_len = len;
			memcpy((unsigned char *)&pstat->vht_oper_buf, p+2, len);
//			SDEBUG("Receive vht_oper len = %d \n",len);
		}
	}
#endif
// ====2011-0926 ;roll back ; ht issue
#if 1
	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
		p = get_ie(pframe + WLAN_HDR_A3_LEN + ie_offset, _HT_CAP_, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - ie_offset);
		if ((p !=  NULL) && (len <= sizeof(struct ht_cap_elmt))) {
			pstat->ht_cap_len = len;
			memcpy((unsigned char *)&pstat->ht_cap_buf, p+2, len);
		}
		else {
			unsigned char old_ht_ie_id[] = {0x00, 0x90, 0x4c};
			p = pframe + WLAN_HDR_A3_LEN + ie_offset; len = 0;
			for (;;)
			{
				p = get_ie(p, _RSN_IE_1_, &len,
					pfrinfo->pktlen - (p - pframe));
				if (p != NULL) {
					if (!memcmp(p+2, old_ht_ie_id, 3) && (*(p+5) == 0x33) && ((len - 4) <= sizeof(struct ht_cap_elmt))) {
						pstat->ht_cap_len = len - 4;
						memcpy((unsigned char *)&pstat->ht_cap_buf, p+6, pstat->ht_cap_len);
						break;
					}
				}
				else
					break;

				p = p + len + 2;
			}
		}

		//AC mode only, deny N mode STA
#ifdef RTK_AC_SUPPORT
		if (!pstat->vht_cap_len && (priv->pmib->dot11StationConfigEntry.legacySTADeny & (WIRELESS_11N ))) {
			DEBUG_ERR("AC mode only, deny non-AC STA association!\n");
			status = _STATS_RATE_FAIL_;
			goto OnAssocReqFail;
		}
#endif
		if (pstat->ht_cap_len) {
			// below is the process to check HT MIMO power save
			unsigned char mimo_ps = ((cpu_to_le16(pstat->ht_cap_buf.ht_cap_info)) >> 2)&0x0003;
			pstat->MIMO_ps = 0;
			if (!mimo_ps)
				pstat->MIMO_ps |= _HT_MIMO_PS_STATIC_;
			else if (mimo_ps == 1)
				pstat->MIMO_ps |= _HT_MIMO_PS_DYNAMIC_;
			if (cpu_to_le16(pstat->ht_cap_buf.ht_cap_info) & _HTCAP_AMSDU_LEN_8K_) {
				pstat->is_8k_amsdu = 1;
				pstat->amsdu_level = 7935 - sizeof(struct wlan_hdr);
			}
			else {
				pstat->is_8k_amsdu = 0;
				pstat->amsdu_level = 3839 - sizeof(struct wlan_hdr);
			}

			if (pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_))
				pstat->tx_bw = HT_CHANNEL_WIDTH_20_40;
#ifdef RTK_AC_SUPPORT
			if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC) && (pstat->vht_cap_len))
			{
				switch(cpu_to_le32(pstat->vht_cap_buf.vht_cap_info) & 0x3) {
					default:
					case 0:
						pstat->is_8k_amsdu = 0;
						pstat->amsdu_level = 3895 - sizeof(struct wlan_hdr);
						break;
					case 1:
						pstat->is_8k_amsdu = 1;
						pstat->amsdu_level = 7991 - sizeof(struct wlan_hdr);
						break;
					case 2:
						pstat->is_8k_amsdu = 1;
						pstat->amsdu_level = 11454 - sizeof(struct wlan_hdr);
						break;
				}
// force 4k
//				pstat->is_8k_amsdu = 0;
//				pstat->amsdu_level = 3895 - sizeof(struct wlan_hdr);
			}
#endif
		}
		else {
			if(!priv->pmib->wscEntry.wsc_enable){
				if (priv->pmib->dot11StationConfigEntry.legacySTADeny & (WIRELESS_11G | WIRELESS_11A)) {
					DEBUG_ERR("Deny legacy STA association!\n");
					status = _STATS_RATE_FAIL_;
					goto OnAssocReqFail;
				}

			}
		}
#ifdef RTK_AC_SUPPORT
		if((priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC)) {
			if (pstat->vht_cap_len && (priv->vht_oper_buf.vht_oper_info[0] == 1))
				pstat->tx_bw = HT_CHANNEL_WIDTH_80;
			p = get_ie(pframe + WLAN_HDR_A3_LEN + ie_offset, EID_VHTOperatingMode, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - ie_offset);
			if ((p !=  NULL) && (len == 1)) {
				// check self capability....
					if((p[2] &3) <= priv->pshare->CurrentChannelBW) {
						pstat->tx_bw = p[2] &3;
                   }
					pstat->nss = ((p[2]>>4)&0x7)+1;
					//printk("receive opering mode data = %x \n", p[2]);
			}


		}
#endif
		pstat->tx_bw_bak = pstat->tx_bw;
		pstat->shrink_ac_bw = (pstat->tx_bw)? pstat->tx_bw<<2:2;
		pstat->shrink_ac_bw_bak = (priv->pshare->is_40m_bw)? priv->pshare->is_40m_bw<<2:2;
	}
#endif
// ====2011-0926 ; ht issue



#ifdef WIFI_WMM
	if (QOS_ENABLE) {
		if ((pstat->QosEnabled == 0) && pstat->ht_cap_len) {
			DEBUG_INFO("STA supports HT but doesn't support WMM, force WMM supported\n");
			pstat->QosEnabled = 1;
		}
	}
#endif




	// Realtek proprietary IE
	pstat->is_realtek_sta = FALSE;
	pstat->IOTPeer = HT_IOT_PEER_UNKNOWN;
	p = pframe + WLAN_HDR_A3_LEN + ie_offset; len = 0;
	for (;;)
	{
		p = get_ie(p, _RSN_IE_1_, &len,
			pfrinfo->pktlen - (p - pframe));
		if (p != NULL) {
			if (!memcmp(p+2, Realtek_OUI, 3) && *(p+2+3) == 2) { /*found realtek out and type == 2*/
                pstat->is_realtek_sta = TRUE;
                pstat->IOTPeer = HT_IOT_PEER_REALTEK;

                if (*(p+2+3+2) & RTK_CAP_IE_AP_CLIENT)
                    pstat->IOTPeer = HT_IOT_PEER_RTK_APCLIENT;

                if(*(p+2+3+2) & RTK_CAP_IE_WLAN_8192SE)
                    pstat->IOTPeer = HT_IOT_PEER_REALTEK_92SE;

                if (*(p+2+3+2) & RTK_CAP_IE_USE_AMPDU)
                    pstat->is_forced_ampdu = TRUE;
                else
                    pstat->is_forced_ampdu = FALSE;
#ifdef RTK_WOW
                if (*(p+2+3+2) & RTK_CAP_IE_USE_WOW)
                    pstat->IOTPeer = HT_IOT_PEER_REALTEK_WOW;
#endif
                if (*(p+2+3+2) & RTK_CAP_IE_WLAN_88C92C)
                    pstat->IOTPeer = HT_IOT_PEER_REALTEK_81XX;

                if (*(p+2+3+3) & ( RTK_CAP_IE_8812_BCUT | RTK_CAP_IE_8812_CCUT))
                    pstat->IOTPeer = HT_IOT_PEER_REALTEK_8812;
				break;
			}
		}
		else
			break;

		p = p + len + 2;
	}

	// identify if this is Broadcom sta
	p = pframe + WLAN_HDR_A3_LEN + ie_offset; len = 0;

	for (;;)
	{
		unsigned char Broadcom_OUI1[]={0x00, 0x05, 0xb5};
		unsigned char Broadcom_OUI2[]={0x00, 0x0a, 0xf7};
		unsigned char Broadcom_OUI3[]={0x00, 0x10, 0x18};

		p = get_ie(p, _RSN_IE_1_, &len,
			pfrinfo->pktlen - (p - pframe));
		if (p != NULL) {
			if (!memcmp(p+2, Broadcom_OUI1, 3) ||
				!memcmp(p+2, Broadcom_OUI2, 3) ||
				!memcmp(p+2, Broadcom_OUI3, 3)) {

				pstat->IOTPeer= HT_IOT_PEER_BROADCOM;

				break;
			}
		}
		else
			break;

		p = p + len + 2;
	}

	// identify if this is ralink sta
	p = pframe + WLAN_HDR_A3_LEN + ie_offset; len = 0;

#if 0
//#if !defined(USE_OUT_SRC) || defined(_OUTSRC_COEXIST)
#ifdef _OUTSRC_COEXIST
	if(!IS_OUTSRC_CHIP(priv))
#endif
	pstat->is_ralink_sta = FALSE;
#endif
	for (;;)
	{
		unsigned char Ralink_OUI1[]={0x00, 0x0c, 0x43};

		p = get_ie(p, _RSN_IE_1_, &len,
			pfrinfo->pktlen - (p - pframe));
		if (p != NULL) {
			if (!memcmp(p+2, Ralink_OUI1, 3)) {

				pstat->IOTPeer= HT_IOT_PEER_RALINK;

				break;
			}
		}
		else
			break;

		p = p + len + 2;
	}

	for (z = 0; z < HTC_OUI_NUM; z++) {
		if ((pstat->hwaddr[0] == HTC_OUI[z][0]) &&
			(pstat->hwaddr[1] == HTC_OUI[z][1]) &&
			(pstat->hwaddr[2] == HTC_OUI[z][2])) {

			pstat->IOTPeer = HT_IOT_PEER_HTC;

			break;
		}
	}

	if (!pstat->is_realtek_sta && (pstat->IOTPeer!=HT_IOT_PEER_BROADCOM) && pstat->IOTPeer!=HT_IOT_PEER_RALINK && pstat->IOTPeer!=HT_IOT_PEER_HTC)
	{
		//unsigned int z = 0;

		for (z = 0; z < INTEL_OUI_NUM; z++) {
			if ((pstat->hwaddr[0] == INTEL_OUI[z][0]) &&
				(pstat->hwaddr[1] == INTEL_OUI[z][1]) &&
				(pstat->hwaddr[2] == INTEL_OUI[z][2])) {

				pstat->IOTPeer = HT_IOT_PEER_INTEL;
				pstat->no_rts = 1;
				break;
			}
		}

	}

#ifdef MCR_WIRELESS_EXTEND
	if ((GET_CHIP_VER(priv)==VERSION_8812E) || (GET_CHIP_VER(priv)==VERSION_8192E) || (GET_CHIP_VER(priv)==VERSION_8814A)) {
		if (!memcmp(pstat->hwaddr, "\x00\x01\x02\x03\x04\x05", MACADDRLEN)) {
			pstat->IOTPeer = HT_IOT_PEER_CMW;
			priv->pshare->cmw_link = 1;
			priv->pshare->rf_ft_var.tx_pwr_ctrl = 0;
		}
	}
#endif

#ifdef A4_STA
    if(priv->pshare->rf_ft_var.a4_enable) {
        if(priv->pshare->rf_ft_var.a4_enable == 2) {
            if(0 < parse_a4_ie(priv, pframe + WLAN_HDR_A3_LEN + ie_offset, pfrinfo->pktlen - (WLAN_HDR_A3_LEN + ie_offset))) {
                add_a4_client(priv, pstat);
            }
        }
        a4_sta_update(GET_ROOT(priv), NULL, pstat->hwaddr);
    }
#endif

#ifdef TV_MODE
    if(priv->tv_mode_status & BIT1){ /*tv mode is auto*/
        if(0 < parse_tv_mode_ie(priv, pframe + WLAN_HDR_A3_LEN + ie_offset, pfrinfo->pktlen - (WLAN_HDR_A3_LEN + ie_offset))) {
            pstat->tv_auto_support = 1;
            priv->tv_mode_status |= BIT0; /* set tv mode to auto(enable)*/
        }
    }
#endif

	SAVE_INT_AND_CLI(flags);
	auth_list_del(priv, pstat);
	if (asoc_list_add(priv, pstat))
	{
		pstat->expire_to = priv->expire_to;
		//printk("wlan%d pstat->asoc_list = %p priv->asoc_list=%p\n",priv->pshare->wlandev_idx, pstat->asoc_list, priv->asoc_list);
		cnt_assoc_num(priv, pstat, INCREASE, (char *)__FUNCTION__);
		check_sta_characteristic(priv, pstat, INCREASE);
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)
			construct_ht_ie(priv, priv->pshare->is_40m_bw, priv->pshare->offset_2nd_chan);
	}
	RESTORE_INT(flags);

	assign_tx_rate(priv, pstat, pfrinfo);

#if defined(WIFI_11N_2040_COEXIST_EXT)
	update_40m_staMap(priv, pstat, 0);
	checkBandwidth(priv);
	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) && (priv->ht_cap_len == 0))
		construct_ht_ie(priv, priv->pshare->is_40m_bw, priv->pshare->offset_2nd_chan);
#endif

	if (IS_HAL_CHIP(priv)) {
		//panic_printk("%s %d UpdateRAMask\n", __FUNCTION__, __LINE__);
		GET_HAL_INTERFACE(priv)->UpdateHalRAMaskHandler(priv, pstat, 3);
		pstat->H2C_rssi_rpt = 1;
		ODM_RAPostActionOnAssoc(ODMPTR);
		pstat->H2C_rssi_rpt = 0;
		pstat->ratr_idx_init = pstat->ratr_idx;
		//phydm_ra_dynamic_rate_id_on_assoc(ODMPTR, pstat->WirelessMode, pstat->ratr_idx_init);

	} else
	{
	}
	assign_aggre_mthod(priv, pstat);
	assign_aggre_size(priv, pstat);


	// Customer proprietary IE
	if (priv->pmib->miscEntry.private_ie_len) {
		p = get_ie(pframe + WLAN_HDR_A3_LEN + ie_offset, priv->pmib->miscEntry.private_ie[0], &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - ie_offset);
		if (p) {
			memcpy(pstat->private_ie, p, len + 2);
			pstat->private_ie_len = len + 2;
		}
	}

	DEBUG_INFO("%s %02X%02X%02X%02X%02X%02X\n",
		(frame_type == WIFI_ASSOCREQ)? "OnAssocReq" : "OnReAssocReq",
		pstat->hwaddr[0],pstat->hwaddr[1],pstat->hwaddr[2],pstat->hwaddr[3],pstat->hwaddr[4],pstat->hwaddr[5]);

	/* 1. If 802.1x enabled, get RSN IE (if exists) and indicate ASSOIC_IND event
	 * 2. Set dot118021xAlgrthm, dot11PrivacyAlgrthm in pstat
	 */
	if (IEEE8021X_FUN || IAPP_ENABLE || priv->pmib->wscEntry.wsc_enable)
	{
		p = pframe + WLAN_HDR_A3_LEN + ie_offset; len = 0;
		for(;;)
		{
#ifdef RTL_WPA2
			char tmpbuf[128];
			int buf_len=0;
			p = get_rsn_ie(priv, p, &len,
				pfrinfo->pktlen - (p - pframe));

			buf_len = sprintf(tmpbuf, "RSNIE len = %d, p = %s", len, (p==NULL? "NULL":"non-NULL"));
			if (p != NULL)
				buf_len += sprintf(tmpbuf+buf_len, ", ID = %02X\n", *(unsigned char *)p);
			else
				buf_len += sprintf(tmpbuf+buf_len, "\n");
			DEBUG_INFO("%s", tmpbuf);
#else
			p = get_ie(p, _RSN_IE_1_, &len,
				pfrinfo->pktlen - (p - pframe));
#endif
			/*cfg p2p cfg p2p
			if (p == NULL)
#ifdef WIFI_HAPD
			{
				memset(pstat->wpa_ie, 0, 256);
				break;
			}
#else
				break;
#endif
			*/
			if (p == NULL){
                memset(pstat->wpa_ie, 0, 6);
                break;
            }

			/*cfg p2p cfg p2p*/
			if((*(unsigned char *)p == _RSN_IE_1_)&& (len >= 4))
				{
					pstat->wpa_sta_info->RSNEnabled = BIT(0);
					memcpy(pstat->wpa_ie, p, len+2);
				}
			else if((*(unsigned char *)p == _RSN_IE_2_) && (len >= 2))
				{
					pstat->wpa_sta_info->RSNEnabled = BIT(1);
					memcpy(pstat->wpa_ie, p, len+2);
				}

#ifdef RTL_WPA2
			if ((*(unsigned char *)p == _RSN_IE_1_) && (len >= 4) && (!memcmp((void *)(p + 2), (void *)rsnie_hdr, 4))) {
#ifdef TLN_STATS
				pstat->enterpise_wpa_info = STATS_ETP_WPA;
#endif
				break;
}

			if ((*(unsigned char *)p == _RSN_IE_2_) && (len >= 2) && (!memcmp((void *)(p + 2), (void *)rsnie_hdr_wpa2, 2))) {
#ifdef TLN_STATS
				pstat->enterpise_wpa_info = STATS_ETP_WPA2;
#endif
				break;
			}

            #ifdef HS2_SUPPORT
            if ((*(unsigned char *)p == _RSN_IE_1_) && (len >= 4) && (!memcmp((void *)(p + 2), (void *)rsnie_hdr_OSEN, 4))) {
                HS2DEBUG("found OSEN IE in Assoc Req\n");
                #ifdef TLN_STATS
                pstat->enterpise_wpa_info = STATS_ETP_WPA2;
                #endif
                break;
            }
            #endif // HS2_SUPPORT

#else
			if ((len >= 4) && (!memcmp((void *)(p + 2), (void *)rsnie_hdr, 4))) {
#ifdef TLN_STATS
				pstat->enterpise_wpa_info = STATS_ETP_WPA;
#endif
				break;
			}
#endif

			p = p + len + 2;
		}

#ifdef WIFI_SIMPLE_CONFIG
/* WPS2DOTX   -start*/
		if (priv->pmib->wscEntry.wsc_enable & 2) { // work as AP (not registrar)
			unsigned char *ptmp;
			unsigned char *TagPtr=NULL;
			int IS_V2=0;
			int Taglen = 0;
			int Taglent2 = 0;
			unsigned int lentmp = 0;
			unsigned char passWscIE=0;
			unsigned char both_band_cred = 0;
			DOT11_WSC_ASSOC_IND wsc_Association_Ind;

		//================both_band_credential====================
		if(priv->pmib->wscEntry.both_band_multicredential){
			ptmp = pframe + WLAN_HDR_A3_LEN + ie_offset;
			for (;;)
			{
				ptmp = get_ie(ptmp, 221, (int *)&lentmp , pfrinfo->pktlen - (ptmp - pframe));
				if (ptmp != NULL) {
					if (!memcmp(ptmp+2, NEC_OUI, 3) && ptmp[5] == 0x06 && (ptmp[8] & BIT7)) {
						both_band_cred = 1;
						break;
					}
				}
				else{
					break;
				}
				ptmp = ptmp + lentmp + 2;
			}
		}
		//==================================================

			ptmp = pframe + WLAN_HDR_A3_LEN + ie_offset;

			for (;;)
			{
				ptmp = get_ie(ptmp, _WPS_IE_, (int *)&lentmp , pfrinfo->pktlen - (ptmp - pframe));
				if (ptmp != NULL) {
					if ((!memcmp(ptmp+2, WSC_IE_OUI, 4)) && ((lentmp + 2) <= (MIN_NUM(PROBEIELEN,256)))) {//256 is size of pstat->wps_ie
#if (defined(RTK_NL80211) || defined(WIFI_HAPD)) && !defined(HAPD_DRV_PSK_WPS)
						//printk("copy wps_ie \n");
						memcpy(pstat->wps_ie, ptmp, lentmp+2);
#endif

						TagPtr = search_wsc_tag(ptmp+2+4, TAG_REQUEST_TYPE, lentmp-4, &Taglen);
						if (TagPtr && (*TagPtr <= MAX_REQUEST_TYPE_NUM)) {
							SME_DEBUG("found WSC_IE TAG_REQUEST_TYPE=%d (from %02x%02x%02x:%02x%02x%02x)\n",
								*TagPtr , pstat->hwaddr[0], pstat->hwaddr[1], pstat->hwaddr[2],
								 pstat->hwaddr[3], pstat->hwaddr[4], pstat->hwaddr[5]);
							passWscIE = 1;
						}


						TagPtr = search_wsc_tag(ptmp+2+4, TAG_VENDOR_EXT, lentmp-4, &Taglen);
						if (TagPtr != NULL)	{
							if(!memcmp(TagPtr , WSC_VENDOR_OUI ,3 )){
								SME_DEBUG("Found WFA-vendor OUI!!\n");
								TagPtr = search_VendorExt_tag(TagPtr ,VENDOR_VERSION2 , Taglen , &Taglent2);
								if(TagPtr){
									IS_V2 = 1;
									SME_DEBUG("sme Rev version2(0x%x) ProReq\n",TagPtr[0]);
								}
							}
						}


						break;
					} else {
						if (TagPtr !=NULL){
							DEBUG_INFO("Found WSC_IE TAG_REQUEST_TYPE=%d", *TagPtr);
						}else{
							DEBUG_INFO("Found WSC_IE");
						}
							DEBUG_INFO(" from %02x%02x%02x:%02x%02x%02x, but the length(%d) of WSC_IE may be bigger than %d, Parse next WSC_IE\n",
							pstat->hwaddr[0], pstat->hwaddr[1], pstat->hwaddr[2], pstat->hwaddr[3],
							pstat->hwaddr[4], pstat->hwaddr[5],	lentmp + 2, (MIN_NUM(PROBEIELEN,256)) );
					}
				}
				else{
#if (defined(RTK_NL80211) || defined(WIFI_HAPD)) && !defined(HAPD_DRV_PSK_WPS)
					memset(pstat->wps_ie, 0, 256);
#endif
					break;
					}

				ptmp = ptmp + lentmp + 2;
			}

			memset(&wsc_Association_Ind, 0, sizeof(DOT11_WSC_ASSOC_IND));
			wsc_Association_Ind.EventId = DOT11_EVENT_WSC_ASSOC_REQ_IE_IND;
			memcpy((void *)wsc_Association_Ind.MACAddr, (void *)GetAddr2Ptr(pframe), MACADDRLEN);
			if (passWscIE) {
				if(both_band_cred){
					//indicates wscd for both_band_credential
					memcpy((void *)wsc_Association_Ind.AssocIE, "\x10\xFF\x00\x01\x01", 5); // T(0x10ff)L(1)V(1)
					memcpy((void *)wsc_Association_Ind.AssocIE + 5, (void *)(ptmp), wsc_Association_Ind.AssocIELen);
					panic_printk("[%s]%d both_band_cred_ind =1 \n",__FUNCTION__,__LINE__);
				}else{
					memcpy((void *)wsc_Association_Ind.AssocIE, (void *)(ptmp), wsc_Association_Ind.AssocIELen);
				}
				wsc_Association_Ind.wscIE_included = 1;
				wsc_Association_Ind.AssocIELen = lentmp + 2;

			}
			else {
				/*modify for WPS2DOTX SUPPORT*/
				if(IS_V2==0)
				{
					/*when sta is wps1.1 case then should be run below path*/
					if (IEEE8021X_FUN &&
						(pstat->AuthAlgrthm == _NO_PRIVACY_) && // authentication is open
						(p == NULL)) // No SSN or RSN IE
					{
						wsc_Association_Ind.wscIE_included = 1; //treat this case as WSC IE included
						SME_DEBUG("Association : auth open & no SSN or RSN IE , for wps1.1 case\n");
					}
				}
			}

             /*   wscIE_included :
                  case 1:  make sure STA include WSC IE
                  case 2:  because auth == open & no SSN or RSN IE ;so we
                           treat this case as WSC IE included
             */
			/*modify for WPS2DOTX SUPPORT*/
			if ((wsc_Association_Ind.wscIE_included == 1) || !IEEE8021X_FUN){
#ifdef INCLUDE_WPS

				wps_NonQueue_indicate_evt(priv ,
					(UINT8 *)&wsc_Association_Ind,sizeof(DOT11_WSC_ASSOC_IND));
#else

				DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&wsc_Association_Ind,
						sizeof(DOT11_WSC_ASSOC_IND));
#endif
			}
			/*modify for WPS2DOTX SUPPORT*/
			if (wsc_Association_Ind.wscIE_included == 1) {
				pstat->state |= WIFI_WPS_JOIN;
				goto OnAssocReqSuccess;
			}
// Brad add for DWA-652 WPS interoperability 2008/03/13--------
			if ((pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_ ||
     				pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_) &&
     				!IEEE8021X_FUN)
				pstat->state |= WIFI_WPS_JOIN;
//------------------------- end

		}
/* WPS2DOTX   -end*/
#endif

// ====2011-0926 ;roll back ; ht issue
#if 1
	if(priv->pmib->wscEntry.wsc_enable) {
		if (!pstat->ht_cap_len && (priv->pmib->dot11StationConfigEntry.legacySTADeny & (WIRELESS_11G | WIRELESS_11A))) {
			DEBUG_ERR("Deny legacy STA association!\n");
			status = _STATS_RATE_FAIL_;
			SAVE_INT_AND_CLI(flags);
			asoc_list_del(priv, pstat);
			cnt_assoc_num(priv, pstat, DECREASE, (char *)__FUNCTION__);
			check_sta_characteristic(priv, pstat, DECREASE);
			RESTORE_INT(flags);
			goto OnAssocReqFail;
		}
	}
#endif
// ====2011-0926 end


	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
		(pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _NO_PRIVACY_))
	{
		int mask_mcs_rate = 0;
		if 	((pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_) ||
			 (pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_))
			mask_mcs_rate = 2;
		else {
			if (p == NULL)
				mask_mcs_rate = 1;
			else {
				if (*p == _RSN_IE_1_) {
					if (is_support_wpa_aes(priv,  p, len+2) != 1)
						mask_mcs_rate = 1;
				}
				else if (*p == _RSN_IE_2_) {
					if (is_support_wpa2_aes(priv,  p, len+2) != 1)
						mask_mcs_rate = 1;
				}
				else
						mask_mcs_rate = 1;
			}
		}

		if (mask_mcs_rate) {
			pstat->is_legacy_encrpt = mask_mcs_rate;
			assign_tx_rate(priv, pstat, pfrinfo);
			if (IS_HAL_CHIP(priv)) {
				//panic_printk("%s %d UpdateRAMask\n", __FUNCTION__, __LINE__);
				GET_HAL_INTERFACE(priv)->UpdateHalRAMaskHandler(priv, pstat, 3);
				pstat->H2C_rssi_rpt = 1;
				ODM_RAPostActionOnAssoc(ODMPTR);
				pstat->H2C_rssi_rpt = 0;
				pstat->ratr_idx_init = pstat->ratr_idx;
				//phydm_ra_dynamic_rate_id_on_assoc(ODMPTR, pstat->WirelessMode, pstat->ratr_idx_init);
			} else
			{
			}
			assign_aggre_mthod(priv, pstat);
		}
	}

#ifndef WITHOUT_ENQUEUE
		if (frame_type == WIFI_ASSOCREQ)
		{
			memcpy((void *)Association_Ind.MACAddr, (void *)GetAddr2Ptr(pframe), MACADDRLEN);
			Association_Ind.EventId = DOT11_EVENT_ASSOCIATION_IND;
			Association_Ind.IsMoreEvent = 0;
			if (p == NULL)
				Association_Ind.RSNIELen = 0;
			else
			{
				DEBUG_INFO("assoc indication rsnie len=%d\n", len);
#ifdef RTL_WPA2
				// inlcude ID and Length
				Association_Ind.RSNIELen = len + 2;
				memcpy((void *)Association_Ind.RSNIE, (void *)(p), Association_Ind.RSNIELen);
#else
				Association_Ind.RSNIELen = len;
				memcpy((void *)Association_Ind.RSNIE, (void *)(p + 2), len);
#endif
			}
			// indicate if 11n sta associated
			Association_Ind.RSNIE[MAXRSNIELEN-1] = ((pstat->ht_cap_len==0) ? 0 : 1);

			DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&Association_Ind,
						sizeof(DOT11_ASSOCIATION_IND));
		}
		else
		{
			memcpy((void *)Reassociation_Ind.MACAddr, (void *)GetAddr2Ptr(pframe), MACADDRLEN);
			Reassociation_Ind.EventId = DOT11_EVENT_REASSOCIATION_IND;
			Reassociation_Ind.IsMoreEvent = 0;
			if (p == NULL)
				Reassociation_Ind.RSNIELen = 0;
			else
			{
				DEBUG_INFO("assoc indication rsnie len=%d\n", len);
#ifdef RTL_WPA2
				// inlcude ID and Length
				Reassociation_Ind.RSNIELen = len + 2;
				memcpy((void *)Reassociation_Ind.RSNIE, (void *)(p), Reassociation_Ind.RSNIELen);
#else
				Reassociation_Ind.RSNIELen = len;
				memcpy((void *)Reassociation_Ind.RSNIE, (void *)(p + 2), len);
#endif
			}
			memcpy((void *)Reassociation_Ind.OldAPaddr,
				(void *)(pframe + WLAN_HDR_A3_LEN + _CAPABILITY_ + _LISTEN_INTERVAL_), MACADDRLEN);

			// indicate if 11n sta associated
			Reassociation_Ind.RSNIE[MAXRSNIELEN-1] = ((pstat->ht_cap_len==0) ? 0 : 1);

			DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&Reassociation_Ind,
						sizeof(DOT11_REASSOCIATION_IND));
		}
#endif // WITHOUT_ENQUEUE

	//printk("pstat=0x%x at %d: %02x %02x %02x\n", pstat, __LINE__, pstat->wpa_ie[0], pstat->wpa_ie[1], pstat->wpa_ie[2]);
	//event_indicate_cfg80211(priv, GetAddr2Ptr(pframe), CFG80211_NEW_STA, pstat);

		{
			int id;
			unsigned char *pIE;
			int ie_len;

			LOG_MSG("A wireless client is associated - %02X:%02X:%02X:%02X:%02X:%02X\n",
				*GetAddr2Ptr(pframe), *(GetAddr2Ptr(pframe)+1), *(GetAddr2Ptr(pframe)+2),
				*(GetAddr2Ptr(pframe+3)), *(GetAddr2Ptr(pframe)+4), *(GetAddr2Ptr(pframe)+5));

			if (frame_type == WIFI_ASSOCREQ)
				id = DOT11_EVENT_ASSOCIATION_IND;
			else
				id = DOT11_EVENT_REASSOCIATION_IND;

#ifdef RTL_WPA2
			ie_len = len + 2;
			pIE = p;
#else
			ie_len = len;
			pIE = p + 2;
#endif
			psk_indicate_evt(priv, id, GetAddr2Ptr(pframe), pIE, ie_len);
		}

		//printk("pstat=0x%x at %d: %02x %02x %02x\n", pstat, __LINE__, pstat->wpa_ie[0], pstat->wpa_ie[1], pstat->wpa_ie[2]);
		//event_indicate_cfg80211(priv, GetAddr2Ptr(pframe), CFG80211_NEW_STA, pstat);
	}
#ifdef HS2_SUPPORT
	calcu_sta_v6ip(pstat);
#endif

//#ifndef INCLUDE_WPA_PSK
#if   defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD) || defined(CONFIG_RTL8196C_KLD)
	if (!IEEE8021X_FUN &&
			!(priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _TKIP_PRIVACY_ ||
			 priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _CCMP_PRIVACY_))
			LOG_MSG_NOTICE("Wireless PC connected;note:%02x-%02x-%02x-%02x-%02x-%02x;\n",
				*GetAddr2Ptr(pframe), *(GetAddr2Ptr(pframe)+1), *(GetAddr2Ptr(pframe)+2),
				*(GetAddr2Ptr(pframe+3)), *(GetAddr2Ptr(pframe)+4), *(GetAddr2Ptr(pframe)+5));
#else
	LOG_MSG("A wireless client is associated - %02X:%02X:%02X:%02X:%02X:%02X\n",
			*GetAddr2Ptr(pframe), *(GetAddr2Ptr(pframe)+1), *(GetAddr2Ptr(pframe)+2),
			*(GetAddr2Ptr(pframe+3)), *(GetAddr2Ptr(pframe)+4), *(GetAddr2Ptr(pframe)+5));
#endif
//#endif


#ifdef WIFI_SIMPLE_CONFIG
OnAssocReqSuccess:
#endif

#ifdef RTK_WLAN_EVENT_INDICATE
	rtk_wlan_event_indicate(priv->dev->name, WIFI_CONNECT_SUCCESS,  pstat->hwaddr, 0);
#endif
#ifdef STA_ASSOC_STATISTIC
	add_sta_assoc_status(priv, pstat->hwaddr, pstat->rssi, 0);
#endif
	if (frame_type == WIFI_ASSOCREQ)
		issue_asocrsp(priv, status, pstat, WIFI_ASSOCRSP);
	else
		issue_asocrsp(priv, status, pstat, WIFI_REASSOCRSP);

#if (BEAMFORMING_SUPPORT == 1)
	if (priv->pshare->WlanSupportAbility & WLAN_BEAMFORMING_SUPPORT)
	if((priv->pmib->dot11RFEntry.txbf == 1) &&
		((pstat->ht_cap_len && (pstat->ht_cap_buf.txbf_cap))
#ifdef RTK_AC_SUPPORT
		||(pstat->vht_cap_len && (cpu_to_le32(pstat->vht_cap_buf.vht_cap_info) & (BIT(SU_BFEE_S)|BIT(SU_BFER_S))))
#endif
		)) {
			Beamforming_Enter(priv, pstat);
	}
#endif

//#ifdef BR_SHORTCUT
#if 0
	clear_shortcut_cache();
#endif

	update_fwtbl_asoclst(priv, pstat);

/*update mesh proxy table*/
	/*cfg p2p cfg p2p*/
	//NDEBUG("pstat=0x%x: %02x %02x %02x\n", pstat, pstat->wpa_ie[0], pstat->wpa_ie[1], pstat->wpa_ie[2]);
	event_indicate_cfg80211(priv, GetAddr2Ptr(pframe), CFG80211_NEW_STA, pstat);

#ifndef USE_WEP_DEFAULT_KEY
	set_keymapping_wep(priv, pstat);
#endif



	return SUCCESS;

asoc_class2_error:

	issue_deauth(priv,	(void *)GetAddr2Ptr(pframe), status);
	if (pstat){
		free_stainfo(priv, pstat);
	}
	return FAIL;

OnAssocReqFail:

	if (frame_type == WIFI_ASSOCREQ)
		issue_asocrsp(priv, status, pstat, WIFI_ASSOCRSP);
	else
		issue_asocrsp(priv, status, pstat, WIFI_REASSOCRSP);
	return FAIL;
}


unsigned int OnProbeReq(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
	struct wifi_mib	*pmib;
	unsigned char	*pframe, *p;
	unsigned char	*ssidptrx=NULL;
	unsigned int	len;
	unsigned char	*bssid, is_11b_only=0;
    #if	defined(WIFI_SIMPLE_CONFIG) || defined(P2P_SUPPORT)
	static	unsigned char tmp_assem_wscie[512];
	unsigned char *awPtr = tmp_assem_wscie ;
	unsigned int foundtimes=0;
	int lenx =	0;
	unsigned char *ptmp;
	unsigned int lentmp;
    #endif

	bssid  = BSSID;
	pmib   = GET_MIB(priv);
	pframe = get_pframe(pfrinfo);

	if (!IS_DRV_OPEN(priv))
		return FAIL;


#ifdef SIMPLE_CH_UNI_PROTOCOL
    if(!pfrinfo->is_11s) /*do not drop Progreq from other mesh node*/
#endif
    {
        if (priv->auto_channel == 1)
            return FAIL;
    }


		{
			if (!((OPMODE & WIFI_AP_STATE) || (OPMODE & WIFI_ADHOC_STATE))
#ifdef MP_TEST
			|| priv->pshare->rf_ft_var.mp_specific
#endif
			)
				return FAIL;

		}

		if (pfrinfo->rssi < priv->pmib->dot11StationConfigEntry.staAssociateRSSIThreshold){
			return FAIL;
		}

	if (pmib->miscEntry.func_off || pmib->miscEntry.raku_only)
		return FAIL;
#ifdef CLIENT_MODE
	if ((OPMODE & WIFI_ADHOC_STATE) &&
			(!priv->ibss_tx_beacon || (OPMODE & WIFI_SITE_MONITOR)))
		return FAIL;
#endif
	if(priv->pmib->dot11StationConfigEntry.probe_info_enable){
		if (check_probe_sta_rssi_valid(priv,GetAddr2Ptr(pframe),pfrinfo->rssi))
			add_probe_req_sta(priv,GetAddr2Ptr(pframe),pfrinfo->rssi);
	}

	if ( (priv->pmib->dot11StationConfigEntry.disable_prsp)
		|| (pfrinfo->rssi < priv->pmib->dot11StationConfigEntry.staAssociateRSSIThreshold)	)
		return FAIL;

    /*PSP IOT*/
	p = get_ie(pframe + WLAN_HDR_A3_LEN + _PROBEREQ_IE_OFFSET_, _EXT_SUPPORTEDRATES_IE_, (int *)&len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _PROBEREQ_IE_OFFSET_);
	if (p == NULL) {
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _PROBEREQ_IE_OFFSET_, _SUPPORTEDRATES_IE_,  (int *)&len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _PROBEREQ_IE_OFFSET_);
		if( (p == NULL) || ( len<=4))
			is_11b_only = 1;
	}
    /*PSP IOT*/

    #ifdef WIFI_SIMPLE_CONFIG
	if (priv->pmib->wscEntry.wsc_enable & 2) { // work as AP (not registrar)
		ptmp = pframe + WLAN_HDR_A3_LEN + _PROBEREQ_IE_OFFSET_; lentmp = 0;
		for (;;)
		{
			ptmp = get_ie(ptmp, _WPS_IE_, (int *)&lentmp,
			pfrinfo->pktlen - (ptmp - pframe));
			if (ptmp != NULL) {
				if (!memcmp(ptmp+2, WSC_IE_OUI, 4)) {
					foundtimes ++;
					if(foundtimes ==1){
						if ( (lentmp + 2 ) > PROBEIELEN)
						{
							DEBUG_WARN("[%s] WPS_IE length is too big =%d\n", __FUNCTION__, (lentmp+2));
							foundtimes--;
							break;
						} else {
							memcpy(awPtr , ptmp ,lentmp + 2);
							awPtr+= (lentmp + 2);
							lenx += (lentmp + 2);
						}
					}else{
					    if ( (lenx + lentmp-4 ) > PROBEIELEN)
						{
							DEBUG_WARN("[%s] Total length of several WPS_IE is too big =%d, do not include the last WSC IEs\n", __FUNCTION__, (lenx+lentmp-4));
							foundtimes--;
							break;
						} else {
							memcpy(awPtr , ptmp+2+4 ,lentmp-4);
							awPtr+= (lentmp-4);
							lenx += (lentmp-4);
						}
					}
				}
			}
			else{
				break;
			}

			ptmp = ptmp + lentmp + 2;
		}
		if(foundtimes){
			lenx = (int)(((unsigned long)awPtr)-((unsigned long)tmp_assem_wscie));
			if(foundtimes>1){
				tmp_assem_wscie[1] = lenx-2;
				//debug_out("ReAss probe_Req wsc_ie ",tmp_assem_wscie,lenx);
			}
			wsc_forward_probe_request(priv, pframe, tmp_assem_wscie, lenx);
		}else{	//
			if( search_wsc_pbc_probe_sta(priv, (unsigned char *)GetAddr2Ptr(pframe))==1){
                DOT11_WSC_PIN_IND wsc_ind;
                wsc_ind.EventId = DOT11_EVENT_WSC_RM_PBC_STA ;
                wsc_ind.IsMoreEvent = 0;
				memcpy(wsc_ind.code,(unsigned char *)GetAddr2Ptr(pframe),6);
                DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (unsigned char*)&wsc_ind, sizeof(DOT11_WSC_PIN_IND));
                event_indicate(priv, NULL, -1);
			}
		}
	}
    /* WPS2DOTX   */

    #ifdef HS2_SUPPORT
	if (priv->pmib->hs2Entry.interworking_ielen)
	{
		unsigned char *ptmp;
		unsigned int  hs_len=0;
		ptmp = get_ie(pframe + WLAN_HDR_A3_LEN + _PROBEREQ_IE_OFFSET_, _EXTENDED_CAP_IE_, (int *)&hs_len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _PROBEREQ_IE_OFFSET_);

		if (ptmp)
		{
			unsigned char tmp[12];

			if (hs_len >= 4)
			{
				memcpy(tmp, ptmp+2, hs_len);

				if (tmp[3] & _INTERWORKING_SUPPORT_)	/*interworking capability bit ; check bit 31(byte3 bit(7));*/
				{
				    hs_len=0;
					ptmp = get_ie(pframe + WLAN_HDR_A3_LEN + _PROBEREQ_IE_OFFSET_, _INTERWORKING_IE_, (int *)&hs_len,
							pfrinfo->pktlen - WLAN_HDR_A3_LEN - _PROBEREQ_IE_OFFSET_);

					if (ptmp && hs_len)
					{
						memcpy(tmp, ptmp+2, hs_len);
						//printk("Check Interworking element, hs_len=%d\n",hs_len);
						//check ant match or not
						if ((tmp[0] & 0x0f) != (priv->pmib->hs2Entry.interworking_ie[0] & 0x0f) && ((tmp[0] & 0x0f) != 15)) //access network type not match
						{
							//printk("ant (access network type) not match\n");
							goto OnProbeReqFail;
						}

						hs_len -= 1;
						if (hs_len > 1)
						{
							//printk("Case hs_len = %d\n",hs_len);
							if (hs_len == 8)
							{
								if (memcmp(tmp+3, priv->pmib->hs2Entry.interworking_ie+3, 6) && memcmp(tmp+3, "\xff\xff\xff\xff\xff\xff", 6)) //hessid not match
								{
									//printk("no match hessid1, %02x%02x%02x%02x%02x%02x\n",tmp[3],tmp[4],tmp[5],tmp[6],tmp[7],tmp[8]);
									goto OnProbeReqFail;
								}
							}
							else if (hs_len == 6)
							{
								if (memcmp(tmp+1, priv->pmib->hs2Entry.interworking_ie+3, 6) && memcmp(tmp+1, "\xff\xff\xff\xff\xff\xff", 6)) //hessid not match
								{
									//printk("no match hessid2, %02x%02x%02x%02x%02x%02x\n",tmp[1],tmp[2],tmp[3],tmp[4],tmp[5],tmp[6]);
									goto OnProbeReqFail;
								}
							}
							else {
								goto OnProbeReqFail;
							}
						}
					}
					else{
						HS2DEBUG("enable interworking bit, but no interworking ie!!\n");
                    }
				}
			}
		}
	}
    #endif
    #endif


	ssidptrx = get_ie(pframe + WLAN_HDR_A3_LEN + _PROBEREQ_IE_OFFSET_, _SSID_IE_, (int *)&len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _PROBEREQ_IE_OFFSET_);

	if (ssidptrx == NULL)
		goto OnProbeReqFail;

	if (len == 0) {
		if (HIDDEN_AP || TAKEOVER_HIDDEN_AP)
			goto OnProbeReqFail;
		else
			goto send_rsp;
	}

	if ((len != SSID_LEN) ||
			memcmp((void *)(ssidptrx+2), (void *)SSID, SSID_LEN)) {
		if ((len == 3) &&
				((*(ssidptrx+2) == 'A') || (*(ssidptrx+2) == 'a')) &&
				((*(ssidptrx+3) == 'N') || (*(ssidptrx+3) == 'n')) &&
				((*(ssidptrx+4) == 'Y') || (*(ssidptrx+4) == 'y'))) {
			if (pmib->dot11OperationEntry.deny_any)
				goto OnProbeReqFail;
			else
				if (HIDDEN_AP || TAKEOVER_HIDDEN_AP)
					goto OnProbeReqFail;
				else
					goto send_rsp;
		}
		else
			goto OnProbeReqFail;
	}

send_rsp:



    {
#if 0
		if(priv->pmib->dot11StationConfigEntry.probe_info_enable){
			if (check_probe_sta_rssi_valid(priv,GetAddr2Ptr(pframe),pfrinfo->rssi))
				add_probe_req_sta(priv,GetAddr2Ptr(pframe),pfrinfo->rssi);
		}
#endif
        issue_probersp(priv, GetAddr2Ptr(pframe), SSID, SSID_LEN, 1, is_11b_only);
    }
	return SUCCESS;

OnProbeReqFail:

	return FAIL;
}


unsigned int OnProbeRsp(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{



// ==== modified by GANTOE for site survey 2008/12/25 ====
    if (OPMODE & WIFI_SITE_MONITOR)
    {

        {
            collect_bss_info(priv, pfrinfo);

	}
    }



	return SUCCESS;
}

unsigned int OnBeacon(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
    int i, len;
    unsigned char *p, *pframe, channel;

    if (OPMODE & WIFI_SITE_MONITOR)
    {


            collect_bss_info(priv, pfrinfo);
        return SUCCESS;
    }


    pframe = get_pframe(pfrinfo);

    p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _DSSET_IE_, &len,
        pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
    if (p != NULL)
        channel = *(p+2);
    else
        channel = priv->pmib->dot11RFEntry.dot11channel;

    // If used as AP in G mode, need monitor other 11B AP beacon to enable
    // protection mechanism
    if ((OPMODE & WIFI_AP_STATE) &&
        (priv->pmib->dot11BssType.net_work_type & (WIRELESS_11G|WIRELESS_11A)) &&
        (channel == priv->pmib->dot11RFEntry.dot11channel))
    {
        // look for ERP rate. if no ERP rate existed, thought it is a legacy AP
        unsigned char supportedRates[32];
        int supplen=0, legacy=1;

        p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_,
            _SUPPORTEDRATES_IE_, &len,
            pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
        if (p) {
            if (len>8)
                len=8;
            memcpy(&supportedRates[supplen], p+2, len);
            supplen += len;
        }

        p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_,
                _EXT_SUPPORTEDRATES_IE_, &len,
                pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
        if (p) {
            if (len>8)
                len=8;
            memcpy(&supportedRates[supplen], p+2, len);
            supplen += len;
        }


        for (i=0; i<supplen; i++) {
            if (!is_CCK_rate(supportedRates[i]&0x7f)) {
                legacy = 0;
                break;
            }
        }

        // look for ERP IE and check non ERP present
        if (legacy == 0) {
            p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _ERPINFO_IE_,
                    &len, pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
            if (p && (*(p+2) & BIT(0)))
                legacy = 1;
        }

        if (legacy) {
            if (!priv->pmib->dot11StationConfigEntry.olbcDetectDisabled &&
                    priv->pmib->dot11ErpInfo.olbcDetected==0) {
                priv->pmib->dot11ErpInfo.olbcDetected = 1;
                check_protection_shortslot(priv);
                DEBUG_INFO("OLBC detected\n");
            }
            if (priv->pmib->dot11ErpInfo.olbcDetected)
                priv->pmib->dot11ErpInfo.olbcExpired = DEFAULT_OLBC_EXPIRE;
        }
    }

    if ((OPMODE & WIFI_AP_STATE) &&
            (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)
            //		&& (channel == priv->pmib->dot11RFEntry.dot11channel)
    )
    {
        if (!priv->pmib->dot11StationConfigEntry.protectionDisabled &&
        !priv->pmib->dot11StationConfigEntry.olbcDetectDisabled) {
            p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_CAP_,
                    &len, pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
            if (p == NULL)
                priv->ht_legacy_obss_to = 60;
        }

        if (!priv->pmib->dot11StationConfigEntry.protectionDisabled &&
            !priv->pmib->dot11StationConfigEntry.nmlscDetectDisabled) {

            p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_IE_, &len,
                    pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
            if (p !=  NULL) {
                struct ht_info_elmt *ht_info=(struct ht_info_elmt *)(p+2);
                if (len) {
                    unsigned int prot_mode =  (cpu_to_le16(ht_info->info1) & 0x03);
                    if (prot_mode == _HTIE_OP_MODE3_)
                        priv->ht_nomember_legacy_sta_to= 60;
                }
            }
        }
    }

#ifdef WIFI_11N_2040_COEXIST
    if (priv->pmib->dot11nConfigEntry.dot11nCoexist && (OPMODE & WIFI_AP_STATE) &&
            (priv->pmib->dot11BssType.net_work_type & (WIRELESS_11N|WIRELESS_11G)) &&
            priv->pshare->is_40m_bw) {
        p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_CAP_,
            &len, pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
        if ((p == NULL) && bg_ap_rssi_chk(priv, pfrinfo, channel)) {

            if (channel && (priv->pmib->dot11nConfigEntry.dot11nCoexist_ch_chk ?
                (channel != priv->pmib->dot11RFEntry.dot11channel) :1))
            {
                if(!priv->bg_ap_timeout) {
                    priv->bg_ap_timeout = 60;
                    update_RAMask_to_FW(priv, 1);
                    SetTxPowerLevel(priv, channel);
                }
                //priv->bg_ap_timeout = 60;
            }
        }
    }
#endif



#ifdef COCHANNEL_RTS
	if ((OPMODE & WIFI_AP_STATE) && (channel == priv->pmib->dot11RFEntry.dot11channel))
	{
		priv->cochannel_to = 5;
	}
#endif

	return SUCCESS;
}


unsigned int OnDisassoc(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned char *pframe;
	struct  stat_info   *pstat;
	unsigned char *sa;
	unsigned short reason;
	DOT11_DISASSOCIATION_IND Disassociation_Ind;
	unsigned long flags;

	pframe = get_pframe(pfrinfo);
	sa = GetAddr2Ptr(pframe);
	pstat = get_stainfo(priv, sa);

	if (pstat == NULL)
		return 0;


#ifdef CONFIG_IEEE80211W
	if(pstat->isPMF) {
		pstat->isPMF = 0; // transmit unprotected mgmt frame
	}
#endif

#ifdef CONFIG_IEEE80211V
	if(WNM_ENABLE) {
		reset_staBssTransStatus(pstat);
	}
#endif

#ifdef RTK_WOW
	if (pstat->is_rtk_wow_sta)
		return 0;
#endif


	reason = cpu_to_le16(*(unsigned short *)((unsigned long)pframe + WLAN_HDR_A3_LEN ));
	DEBUG_INFO("receiving disassoc from station %02X%02X%02X%02X%02X%02X reason %d\n",
		pstat->hwaddr[0], pstat->hwaddr[1], pstat->hwaddr[2],
		pstat->hwaddr[3], pstat->hwaddr[4], pstat->hwaddr[5], reason);


	SAVE_INT_AND_CLI(flags);

	if (asoc_list_del(priv, pstat))
	{
		if (pstat->expire_to > 0)
		{
			cnt_assoc_num(priv, pstat, DECREASE, (char *)__FUNCTION__);
			check_sta_characteristic(priv, pstat, DECREASE);
		}

        if(IS_HAL_CHIP(priv))
        {
            if(pstat && (REMAP_AID(pstat) < 128))
            {
                DEBUG_WARN("%s %d OnDisassoc, set MACID 0 AID = %x \n",__FUNCTION__,__LINE__,REMAP_AID(pstat));
				if(pstat->txpdrop_flag == 1) {
					GET_HAL_INTERFACE(priv)->UpdateHalMSRRPTHandler(priv, pstat, INCREASE);
					pstat->txpdrop_flag = 0;
				}
                GET_HAL_INTERFACE(priv)->SetMACIDSleepHandler(priv, 0 , REMAP_AID(pstat));
				if(priv->pshare->paused_sta_num && pstat->txpause_flag) {
					priv->pshare->paused_sta_num--;
					pstat->txpause_flag =0;
	        	}
            }
            else
            {
                DEBUG_WARN(" MACID sleep only support 128 STA \n");
            }
        }

	}

	// Need change state back to autehnticated
	if (IEEE8021X_FUN)
	{
	}

	release_stainfo(priv, pstat);
	init_stainfo(priv, pstat);
	pstat->state |= WIFI_AUTH_SUCCESS;
	pstat->expire_to = priv->assoc_to;
	auth_list_add(priv, pstat);

	RESTORE_INT(flags);

	if (IEEE8021X_FUN)
	{
		psk_indicate_evt(priv, DOT11_EVENT_DISASSOCIATION_IND, sa, NULL, 0);
	}

	LOG_MSG("A wireless client is disassociated - %02X:%02X:%02X:%02X:%02X:%02X\n",
		*sa, *(sa+1), *(sa+2), *(sa+3), *(sa+4), *(sa+5));
	event_indicate_cfg80211(priv, sa, CFG80211_DEL_STA, NULL);
	event_indicate(priv, sa, 2);


	return SUCCESS;
}


unsigned int OnAuth(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned int		privacy,seq, len;
#if 1//!defined(SMP_SYNC) || (defined(CONFIG_USB_HCI) || defined(CONFIG_SDIO_HCI))
	unsigned long		flags=0;
#endif
	struct list_head	*phead, *plist;
	struct wlan_acl_node *paclnode;
	unsigned int		acl_mode;

	struct wifi_mib		*pmib;
	struct stat_info	*pstat=NULL;
	unsigned char		*pframe, *sa, *p;
	unsigned int		res=FAIL;
	UINT16				algorithm;
	int					status, alloc_pstat=0;
	struct ac_log_info	*log_info;


	pmib = GET_MIB(priv);

	acl_mode = priv->pmib->dot11StationConfigEntry.dot11AclMode;
	pframe = get_pframe(pfrinfo);
	sa = GetAddr2Ptr(pframe);
	pstat = get_stainfo(priv, sa);

	if (!IS_DRV_OPEN(priv))
		return FAIL;
	if (!(OPMODE & WIFI_AP_STATE))
		return FAIL;


	if (pmib->miscEntry.func_off || pmib->miscEntry.raku_only)
		return FAIL;

	if (pfrinfo->rssi < priv->pmib->dot11StationConfigEntry.staAssociateRSSIThreshold) {
#ifdef STA_ASSOC_STATISTIC
		add_reject_sta(priv,GetAddr2Ptr(pframe), pfrinfo->rssi);
#endif
		return FAIL;
	}


	privacy = priv->pmib->dot1180211AuthEntry.dot11AuthAlgrthm;

	seq = cpu_to_le16(*(unsigned short *)((unsigned long)pframe + WLAN_HDR_A3_LEN + 2));

	algorithm = cpu_to_le16(*(unsigned short *)((unsigned long)pframe + WLAN_HDR_A3_LEN));

	{
	if (GetPrivacy(pframe))
	{
		int use_keymapping=0;
		status = wep_decrypt(priv, pfrinfo, pfrinfo->pktlen,
			priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm, use_keymapping);

		if (status == FALSE)
		{
			SAVE_INT_AND_CLI(flags);
#ifdef RTK_WLAN_EVENT_INDICATE
			rtk_wlan_event_indicate(priv->dev->name, WIFI_CONNECT_FAIL, sa, _STATS_CHALLENGE_FAIL_);
#endif
#ifdef PROC_STA_CONN_FAIL_INFO
			{
				int i;
				for (i=0; i<64; i++) {
					if (!priv->sta_conn_fail[i].used) {
						priv->sta_conn_fail[i].used = TRUE;
						priv->sta_conn_fail[i].error_state = _STATS_CHALLENGE_FAIL_;
						memcpy(priv->sta_conn_fail[i].addr, sa, MACADDRLEN);
						break;
					}
					else {
						if (!memcmp(priv->sta_conn_fail[i].addr, sa, MACADDRLEN)) {
							priv->sta_conn_fail[i].error_state = _STATS_CHALLENGE_FAIL_;
							break;
						}
					}
				}
			}
#endif
#ifdef STA_ASSOC_STATISTIC
			add_sta_assoc_status(priv, pstat->hwaddr, pstat->rssi, _STATS_CHALLENGE_FAIL_);
#endif
			DEBUG_ERR("wep-decrypt a Auth frame error!\n");
			status = _STATS_CHALLENGE_FAIL_;
			goto auth_fail;
		}

		seq = cpu_to_le16(*(unsigned short *)((unsigned long)pframe + WLAN_HDR_A3_LEN + 4 + 2));
		algorithm = cpu_to_le16(*(unsigned short *)((unsigned long)pframe + WLAN_HDR_A3_LEN + 4));
	}
#ifdef WIFI_SIMPLE_CONFIG
	else {
		if (pmib->wscEntry.wsc_enable && (seq == 1) && (algorithm == 0))
			privacy = 0;
	}
#endif

	DEBUG_INFO("auth alg=%x, seq=%X\n", algorithm, seq);

	if (privacy == 2 &&
			priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _WEP_40_PRIVACY_ &&
			priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _WEP_104_PRIVACY_)
		privacy = 0;

	if ((algorithm == 1 && privacy == 0) ||	// rx a shared-key auth but shared not enabled
		(algorithm == 0 && privacy == 1) )	// rx a open-system auth but shared-key is enabled
	{
		SAVE_INT_AND_CLI(flags);
		DEBUG_ERR("auth rejected due to bad alg [alg=%d, auth_mib=%d] %02X%02X%02X%02X%02X%02X\n",
			algorithm, privacy, sa[0], sa[1], sa[2], sa[3], sa[4], sa[5]);
#ifdef RTK_WLAN_EVENT_INDICATE
		rtk_wlan_event_indicate(priv->dev->name, WIFI_CONNECT_FAIL, sa, _STATS_NO_SUPP_ALG_);
#endif
#ifdef PROC_STA_CONN_FAIL_INFO
		{
			int i;
			for (i=0; i<64; i++) {
				if (!priv->sta_conn_fail[i].used) {
					priv->sta_conn_fail[i].used = TRUE;
					priv->sta_conn_fail[i].error_state = _STATS_NO_SUPP_ALG_;
					memcpy(priv->sta_conn_fail[i].addr, sa, MACADDRLEN);
					break;
				}
				else {
					if (!memcmp(priv->sta_conn_fail[i].addr, sa, MACADDRLEN)) {
						priv->sta_conn_fail[i].error_state = _STATS_NO_SUPP_ALG_;
						break;
					}
				}
			}
		}
#endif
#ifdef STA_ASSOC_STATISTIC
		add_sta_assoc_status(priv, pstat->hwaddr, pstat->rssi, _STATS_NO_SUPP_ALG_);
#endif
		status = _STATS_NO_SUPP_ALG_;
		goto auth_fail;
	}

#ifdef WIFI_SIMPLE_CONFIG
	wsc_disconn_list_update(priv, sa);
#endif

	// STA ACL check;nctu note
	SAVE_INT_AND_CLI(flags);
	SMP_LOCK_ACL(flags);

	phead = &priv->wlan_acl_list;
	plist = phead->next;
	//check sa
	if (acl_mode == 1)		// 1: positive check, only those on acl_list can be connected.
		res = FAIL;
	else
		res = SUCCESS;

	while(plist != phead)
	{
		paclnode = list_entry(plist, struct wlan_acl_node, list);
		plist = plist->next;
		if (!memcmp((void *)sa, paclnode->addr, 6)) {
			if (paclnode->mode & 2) { // deny
				res = FAIL;
				break;
			}
			else {
				res = SUCCESS;
				break;
			}
		}
	}


	RESTORE_INT(flags);
	SMP_UNLOCK_ACL(flags);



	if (res != SUCCESS) {
		DEBUG_ERR("auth abort because ACL!\n");

		log_info = aclog_lookfor_entry(priv, sa);
		if (log_info) {
			aclog_update_entry(log_info, sa);

			if (log_info->cur_cnt == 1) { // first time trigger
#if   defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD) || defined(CONFIG_RTL8196C_KLD)
				LOG_MSG_DROP("Unauthorized wireless PC try to connect;note:%02x:%02x:%02x:%02x:%02x:%02x;\n",
					*sa, *(sa+1), *(sa+2), *(sa+3), *(sa+4), *(sa+5));
#else
				LOG_MSG("A wireless client was rejected due to access control - %02X:%02X:%02X:%02X:%02X:%02X\n",
					*sa, *(sa+1), *(sa+2), *(sa+3), *(sa+4), *(sa+5));
#endif
				log_info->last_cnt = log_info->cur_cnt;

				if (priv->acLogCountdown == 0)
					priv->acLogCountdown = AC_LOG_TIME;
			}
		}
#ifdef RTK_WLAN_EVENT_INDICATE
		rtk_wlan_event_indicate(priv->dev->name, WIFI_CONNECT_FAIL, sa, _STATS_OTHER_);
#endif
#ifdef PROC_STA_CONN_FAIL_INFO
		{
			int i;
			for (i=0; i<64; i++) {
				if (!priv->sta_conn_fail[i].used) {
					priv->sta_conn_fail[i].used = TRUE;
					priv->sta_conn_fail[i].error_state = _STATS_OTHER_;
					memcpy(priv->sta_conn_fail[i].addr, sa, MACADDRLEN);
					break;
				}
				else {
					if (!memcmp(priv->sta_conn_fail[i].addr, sa, MACADDRLEN)) {
						priv->sta_conn_fail[i].error_state = _STATS_OTHER_;
						break;
					}
				}
			}
		}
#endif
#ifdef STA_ASSOC_STATISTIC
		add_sta_assoc_status(priv, pstat->hwaddr, pstat->rssi, _STATS_OTHER_);
#endif
#ifdef ERR_ACCESS_CNTR
		{
			int i = 0, found = 0;
			for (i=0; i<MAX_ERR_ACCESS_CNTR; i++) {
				if (priv->err_ac_list[i].used) {
					if (!memcmp(sa, priv->err_ac_list[i].mac, MACADDRLEN)) {
						priv->err_ac_list[i].num++;
						found++;
						break;
					}
				}
				else
					break;
			}
			if (!found && (i != MAX_ERR_ACCESS_CNTR)) {
				priv->err_ac_list[i].used = TRUE;
				memcpy(priv->err_ac_list[i].mac, sa, MACADDRLEN);
				priv->err_ac_list[i].num++;
			}
		}
#endif

		return FAIL;
	}

	if (priv->pmib->dot11StationConfigEntry.supportedStaNum) {
		if (!pstat && priv->assoc_num >= priv->pmib->dot11StationConfigEntry.supportedStaNum) {
			DEBUG_ERR("Exceed the upper limit of supported clients...\n");
			status = _STATS_UNABLE_HANDLE_STA_;
			goto auth_fail;
		}
	}
	if (priv->pshare->rf_ft_var.dynamic_max_num_stat) {
		if (!pstat && priv->pshare->total_assoc_num >= priv->pshare->rf_ft_var.dynamic_max_num_stat) {
			DEBUG_ERR("Exceed the maximum total supported clients...\n");
			status = _STATS_UNABLE_HANDLE_STA_;
			goto auth_fail;
		}
	}
	}	//if MESH is enable here is end of (FALSE == isMeshMP)


#ifdef STA_CONTROL && STA_CONTROL_ALGO != STA_CONTROL_ALGO3
    if(priv->stactrl.stactrl_status && priv->stactrl.stactrl_prefer == 0) {  /*band steering on and at non-prefer band*/
        unsigned char req_status = stactrl_check_request(priv, GetAddr2Ptr(pframe), WIFI_AUTH, pfrinfo->rssi);
        if(req_status == 1 || req_status == 2)
            return FAIL;
    }
#endif


	if (pstat == NULL) {
		struct rtl8192cd_priv *priv_del = NULL;
		struct stat_info *pstat_del = NULL;
		int i;
		for(i=0; i<NUM_STAT; i++) {
			if (priv->pshare->aidarray[i] && (priv->pshare->aidarray[i]->used == TRUE) &&
				!memcmp(priv->pshare->aidarray[i]->station.hwaddr, sa, MACADDRLEN) &&
				(GET_MIB(priv->pshare->aidarray[i]->priv)->dot11OperationEntry.opmode & WIFI_AP_STATE)
				) {
				priv_del = priv->pshare->aidarray[i]->priv;
				pstat_del = &priv->pshare->aidarray[i]->station;
				break;
			}
		}
		if (priv_del)
			del_station(priv_del, pstat_del, 0);
	}

	// (below, share with Mesh) due to the free_statinfo in AUTH_TO, we should enter critical section here!
	SAVE_INT_AND_CLI(flags);

	if (pstat == NULL)	// STA only, other one, Don't detect peer MP myself, But peer MP detect me and send Auth request.;nctu note
	{

		// allocate a new one
		DEBUG_INFO("going to alloc stainfo for sa=%02X%02X%02X%02X%02X%02X\n",  sa[0],sa[1],sa[2],sa[3],sa[4],sa[5]);
		pstat = alloc_stainfo(priv, sa, -1);

		if (pstat == NULL)
		{
			DEBUG_ERR("Exceed the upper limit of supported clients...\n");
			status = _STATS_UNABLE_HANDLE_STA_;
			goto auth_fail;
		}
		pstat->state = WIFI_AUTH_NULL;
		pstat->auth_seq = 0;	// clear in alloc_stainfo;nctu note
		pstat->tpcache_mgt = GetTupleCache(pframe);
	}
#ifdef CONFIG_IEEE80211W
	else if(pstat->isPMF)
	{
		pstat->auth_seq = seq + 1;
		goto auth_success;
	}
#endif
	else
	{	// close exist connection.;nctu note
		if (asoc_list_del(priv, pstat))
		{

			if (pstat->expire_to > 0)
			{
				cnt_assoc_num(priv, pstat, DECREASE, (char *)__FUNCTION__);
				check_sta_characteristic(priv, pstat, DECREASE);
			}
		}
		if (seq==1) {
#ifdef  SUPPORT_TX_MCAST2UNI
			int ipmc_num;
			struct ip_mcast_info ipmc[MAX_IP_MC_ENTRY];

			ipmc_num = pstat->ipmc_num;
			if (ipmc_num)
				memcpy(ipmc, pstat->ipmc, MAX_IP_MC_ENTRY * sizeof(struct ip_mcast_info));
#endif
			release_stainfo(priv, pstat);
			init_stainfo(priv, pstat);

			pstat->tpcache_mgt = GetTupleCache(pframe);
#ifdef  SUPPORT_TX_MCAST2UNI
			if (ipmc_num) {
				pstat->ipmc_num = ipmc_num;
				memcpy(pstat->ipmc, ipmc, MAX_IP_MC_ENTRY * sizeof(struct ip_mcast_info));
			}
#endif


		}
	}


	auth_list_add(priv, pstat);

	if (pstat->auth_seq == 0)
		pstat->expire_to = priv->auth_to;

	// Authentication Sequence (STA only)
	if ((pstat->auth_seq + 1) != seq)
	{
		DEBUG_ERR("(1)auth rejected because out of seq [rx_seq=%d, exp_seq=%d]!\n",
			seq, pstat->auth_seq+1);
		status = _STATS_OUT_OF_AUTH_SEQ_;
		goto auth_fail;
	}

	if (algorithm == 0 && (privacy == 0 || privacy == 2)) // STA only open auth
	{
		if (seq == 1)
		{

			pstat->state &= ~WIFI_AUTH_NULL;
			pstat->state |= WIFI_AUTH_SUCCESS;
			pstat->expire_to = priv->assoc_to;

			pstat->AuthAlgrthm = algorithm;


		}
		else
		{
			DEBUG_ERR("(2)auth rejected because out of seq [rx_seq=%d, exp_seq=%d]!\n",
				seq, pstat->auth_seq+1);
			status = _STATS_OUT_OF_AUTH_SEQ_;
			goto auth_fail;
		}
	}
	else // shared system or auto authentication (STA only).
	{
		if (seq == 1)
		{
			//prepare for the challenging txt...
			get_random_bytes((void *)pstat->chg_txt, 128);
			pstat->state &= ~WIFI_AUTH_NULL;
			pstat->state |= WIFI_AUTH_STATE1;
			pstat->AuthAlgrthm = algorithm;
			pstat->auth_seq = 2;
		}
		else if (seq == 3)
		{
			//checking for challenging txt...
			p = get_ie(pframe + WLAN_HDR_A3_LEN + 4 + _AUTH_IE_OFFSET_, _CHLGETXT_IE_, (int *)&len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _AUTH_IE_OFFSET_ - 4);
			if ((p != NULL) && !memcmp((void *)(p + 2),pstat->chg_txt, 128))
			{
				pstat->state &= (~WIFI_AUTH_STATE1);
				pstat->state |= WIFI_AUTH_SUCCESS;
				// challenging txt is correct...
				pstat->expire_to = priv->assoc_to;
			}
			else
			{
				DEBUG_ERR("auth rejected because challenge failure!\n");
				status = _STATS_CHALLENGE_FAIL_;
#ifdef RTK_WLAN_EVENT_INDICATE
				rtk_wlan_event_indicate(priv->dev->name, WIFI_CONNECT_FAIL, sa, _STATS_CHALLENGE_FAIL_);
#endif
#ifdef PROC_STA_CONN_FAIL_INFO
				{
					int i;
					for (i=0; i<64; i++) {
						if (!priv->sta_conn_fail[i].used) {
							priv->sta_conn_fail[i].used = TRUE;
							priv->sta_conn_fail[i].error_state = _STATS_CHALLENGE_FAIL_;
							memcpy(priv->sta_conn_fail[i].addr, sa, MACADDRLEN);
							break;
						}
						else {
							if (!memcmp(priv->sta_conn_fail[i].addr, sa, MACADDRLEN)) {
								priv->sta_conn_fail[i].error_state = _STATS_CHALLENGE_FAIL_;
								break;
							}
						}
					}
				}
#endif
#ifdef STA_ASSOC_STATISTIC
				add_sta_assoc_status(priv, pstat->hwaddr, pstat->rssi, _STATS_CHALLENGE_FAIL_);
#endif
#ifdef ERR_ACCESS_CNTR
				{
					int i = 0, found = 0;
					for (i=0; i<MAX_ERR_ACCESS_CNTR; i++) {
						if (priv->err_ac_list[i].used) {
							if (!memcmp(sa, priv->err_ac_list[i].mac, MACADDRLEN)) {
								priv->err_ac_list[i].num++;
								found++;
								break;
							}
						}
						else
							break;
					}
					if (!found && (i != MAX_ERR_ACCESS_CNTR)) {
						priv->err_ac_list[i].used = TRUE;
						memcpy(priv->err_ac_list[i].mac, sa, MACADDRLEN);
						priv->err_ac_list[i].num++;
					}
				}
#endif
				goto auth_fail;
			}
		}
		else
		{
			DEBUG_ERR("(3)auth rejected because out of seq [rx_seq=%d, exp_seq=%d]!\n",
				seq, pstat->auth_seq+1);
			status = _STATS_OUT_OF_AUTH_SEQ_;
			goto auth_fail;
		}
	}

	// Now, we are going to issue_auth...

	pstat->auth_seq = seq + 1;
	issue_auth(priv, pstat, (unsigned short)(_STATS_SUCCESSFUL_));

	if (pstat->state & WIFI_AUTH_SUCCESS)	// STA valid
		pstat->auth_seq = 0;

	RESTORE_INT(flags);


	return SUCCESS;
auth_success:

	issue_auth(priv, pstat, (unsigned short)(_STATS_SUCCESSFUL_));

	if (pstat->state & WIFI_AUTH_SUCCESS)	// STA valid
		pstat->auth_seq = 0;

	RESTORE_INT(flags);


	return SUCCESS;

auth_fail:

	if ((OPMODE & WIFI_AP_STATE) && (pstat == NULL)) {
		pstat = (struct stat_info *)kmalloc(sizeof(struct stat_info), GFP_ATOMIC);
		if (pstat == NULL) {
			RESTORE_INT(flags);
			return FAIL;
		}

		alloc_pstat = 1;
		memset(pstat, 0, sizeof(struct stat_info));

		pstat->auth_seq = 2;
		memcpy(pstat->hwaddr, sa, 6);
		pstat->AuthAlgrthm = algorithm;
	}
	else {
		alloc_pstat = 0;
		pstat->auth_seq = seq + 1;
	}

	issue_auth(priv, pstat, (unsigned short)status);
#ifdef TLN_STATS
	stats_conn_status_counts(priv, status);
#endif

	if (alloc_pstat)
		kfree(pstat);

	SNMP_MIB_ASSIGN(dot11AuthenticateFailStatus, status);
	SNMP_MIB_COPY(dot11AuthenticateFailStation, sa, MACADDRLEN);

	RESTORE_INT(flags);

	return FAIL;
}



/**
 *	@brief	AP recived De-Authentication
 *
 */
unsigned int OnDeAuth(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned char *pframe;
	struct  stat_info   *pstat;
	unsigned char *sa;
	unsigned short reason;
	DOT11_DISASSOCIATION_IND Disassociation_Ind;
	unsigned long flags;

	pframe = get_pframe(pfrinfo);
	sa = GetAddr2Ptr(pframe);
	pstat = get_stainfo(priv, sa);

	if (pstat == NULL)
		return 0;


#ifdef CONFIG_IEEE80211W
	if(pstat->isPMF) {
		pstat->isPMF = 0;
	}
#endif

#ifdef CONFIG_IEEE80211V
	if(WNM_ENABLE) {
		reset_staBssTransStatus(pstat);
	}
#endif

#ifdef RTK_WOW
	if (pstat->is_rtk_wow_sta)
		return 0;
#endif


	reason = cpu_to_le16(*(unsigned short *)((unsigned long)pframe + WLAN_HDR_A3_LEN ));
	DEBUG_INFO("receiving deauth from station %02X%02X%02X%02X%02X%02X reason %d\n",
		pstat->hwaddr[0], pstat->hwaddr[1], pstat->hwaddr[2],
		pstat->hwaddr[3], pstat->hwaddr[4], pstat->hwaddr[5], reason);


	SAVE_INT_AND_CLI(flags);
	if (asoc_list_del(priv, pstat))
	{
		if (pstat->expire_to > 0)
		{
			cnt_assoc_num(priv, pstat, DECREASE, (char *)__FUNCTION__);
			check_sta_characteristic(priv, pstat, DECREASE);

		}

        if(IS_HAL_CHIP(priv))
        {
            if(pstat && (REMAP_AID(pstat) < 128))
            {
                DEBUG_WARN("%s %d OnDeAuth, set MACID 0 AID = %x \n",__FUNCTION__,__LINE__,REMAP_AID(pstat));
				if(pstat->txpdrop_flag == 1) {
					GET_HAL_INTERFACE(priv)->UpdateHalMSRRPTHandler(priv, pstat, INCREASE);
					pstat->txpdrop_flag = 0;
				}
                GET_HAL_INTERFACE(priv)->SetMACIDSleepHandler(priv, 0, REMAP_AID(pstat));
				if(priv->pshare->paused_sta_num && pstat->txpause_flag) {
					priv->pshare->paused_sta_num--;
					pstat->txpause_flag =0;
	        	}
            }
            else
            {
                DEBUG_WARN(" MACID sleep only support 128 STA \n");
            }
        }

	}
	RESTORE_INT(flags);

#if (BEAMFORMING_SUPPORT == 1)
	if (priv->pmib->dot11RFEntry.txbf == 1 && (priv->pshare->WlanSupportAbility & WLAN_BEAMFORMING_SUPPORT))
	{
            PRT_BEAMFORMING_INFO 	pBeamformingInfo = &(priv->pshare->BeamformingInfo);
		ODM_RT_TRACE(ODMPTR, PHYDM_COMP_TXBF, ODM_DBG_LOUD, ("%s,\n", __FUNCTION__));

            pBeamformingInfo->CurDelBFerBFeeEntrySel = BFerBFeeEntry;
		if(Beamforming_DeInitEntry(priv, pstat->hwaddr))
			Beamforming_Notify(priv);
	}
#endif


	free_stainfo(priv, pstat);

	LOG_MSG("A wireless client is deauthenticated - %02X:%02X:%02X:%02X:%02X:%02X\n",
		*sa, *(sa+1), *(sa+2), *(sa+3), *(sa+4), *(sa+5));

	if (IEEE8021X_FUN)
	{
		psk_indicate_evt(priv, DOT11_EVENT_DISASSOCIATION_IND, sa, NULL, 0);
	}

	event_indicate_cfg80211(priv, sa, CFG80211_DEL_STA, NULL);
	event_indicate(priv, sa, 2);

	return SUCCESS;
}


unsigned int OnWmmAction(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
#if defined(P2P_SUPPORT) || defined(HS2_SUPPORT)
	int needRdyAssoc=1;
#endif

#ifdef WIFI_WMM
	if (QOS_ENABLE
	) {
		unsigned char *sa = pfrinfo->sa;
		unsigned char *da = pfrinfo->da;
		struct stat_info *pstat = get_stainfo(priv, sa);
		unsigned char *pframe=NULL;
		unsigned char Category_field=0, Action_field=0, previous_mimo_ps=0;
		unsigned char TID=0;
		unsigned short blockAck_para=0, status_code=_STATS_SUCCESSFUL_, timeout=0, reason_code, max_size, start_sequence;
#ifdef TX_SHORTCUT
		unsigned int do_tx_slowpath = 0;
#endif
#ifdef HS2_SUPPORT
		DOT11_HS2_GAS_REQ   gas_req, gas_req2;
#endif

		// Reply in B/G mode to fix IOT issue with D-Link DWA-642
#if 0
		if (!(priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) ||
				(pstat && pstat->ht_cap_len == 0)) {
			DEBUG_ERR("Drop Action frame!\n");
			return SUCCESS;
		}
#endif

		pframe = get_pframe(pfrinfo) + WLAN_HDR_A3_LEN;	//start of action frame content
		Category_field = pframe[0];
		Action_field = pframe[1];
#ifdef HS2_SUPPORT
        if((Category_field==_PUBLIC_CATEGORY_ID_) && ((Action_field==_GAS_INIT_REQ_ACTION_ID_)||(Action_field==_GAS_COMBACK_REQ_ACTION_ID_)||(Action_field==_GAS_INIT_RSP_ACTION_ID_)||(Action_field==_GAS_COMBACK_RSP_ACTION_ID_)))
        {
			needRdyAssoc=0;
        }
        if ((Category_field==_DLS_CATEGORY_ID_) && (Action_field==_DLS_REQ_ACTION_ID_))
			needRdyAssoc=0;
#endif



		if ((!IS_MCAST(da)) && (pstat
#if defined(P2P_SUPPORT) || defined(HS2_SUPPORT)
			|| needRdyAssoc==0
#endif
			))
		{

			switch (Category_field) {
#ifdef DOT11H
                case _SPECTRUM_MANAGEMENT_CATEGORY_ID_:
                    switch (Action_field) {
                        case _TPC_REQEST_ACTION_ID_:
                            if(priv->pmib->dot11hTPCEntry.tpc_enable)
                                issue_TPC_report(priv, sa, pframe[2]);
                            break;
                        case _TPC_REPORT_ACTION_ID_:
                            break;
                        default:
                            break;
                    }
                    break;
#endif
				case _BLOCK_ACK_CATEGORY_ID_:
					switch (Action_field) {
						case _ADDBA_Req_ACTION_ID_:
							blockAck_para = pframe[3] | (pframe[4] << 8);
							start_sequence = (pframe[7] | (pframe[8] << 8))>> 4;
							timeout = 0; //pframe[5] | (pframe[6] << 8);
							pstat->AMSDU_AMPDU_support = blockAck_para & 0x1;
							TID = (blockAck_para>>2)&0x000f;
							max_size = (blockAck_para&0xffc0)>>6;
							DEBUG_INFO("ADDBA-req recv fr AID %d, token %d TID %d size %d timeout %d\n",
								pstat->aid, pframe[2], TID, max_size, timeout);
							if (!(blockAck_para & BIT(1)) || (pstat->ht_cap_len == 0)
							|| (pstat->sta_in_firmware != 1 && priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _NO_PRIVACY_)
							|| priv->pmib->dot11nConfigEntry.dot11nAddBAreject
							){	// 0=delayed BA, 1=immediate BA
								status_code = _STATS_REQ_DECLINED_;
							}else{
								pstat->ADDBA_req_num[TID] = 0;
							}

#ifdef DZ_ADDBA_RSP
							if (pstat && ((pstat->state & (WIFI_SLEEP_STATE | WIFI_ASOC_STATE)) ==
											(WIFI_SLEEP_STATE | WIFI_ASOC_STATE))) {
								pstat->dz_addba.used = 1;
								pstat->dz_addba.dialog_token = pframe[2];
								pstat->dz_addba.TID = TID;
								pstat->dz_addba.status_code = status_code;
								pstat->dz_addba.timeout = timeout;
							}
							else
#endif
							{
								if (!issue_ADDBArsp(priv, sa, pframe[2], TID, status_code, timeout)) {
									DEBUG_ERR("issue ADDBA-rsp failed\n");
								} else {
									if(start_sequence == 0) {
										int i=0;

										//panic_printk("reset rc/tpcache after addbaReq from STA\n");
										//STA try to reset start sequence of AMPDU, make prior packets dequed.
										reorder_ctrl_consumeQ(priv, pstat, TID, 4);
										//reset sequence cache
										for(i=0;i<TUPLE_WINDOW;i++)
											pstat->tpcache[TID][i] = 0xffff;
									}
								}
							}
							break;
						case _DELBA_ACTION_ID_:
							TID = (pframe[3] & 0xf0) >> 4;
							pstat->ADDBA_ready[TID] = 0;
							pstat->ADDBA_req_num[TID] = 0;
							pstat->ADDBA_sent[TID] = 0;
							reason_code = pframe[4] | (pframe[5] << 8);
							DEBUG_INFO("DELBA recv from AID %d, TID %d reason %d\n", pstat->aid, TID, reason_code);
#ifdef TX_SHORTCUT
							do_tx_slowpath++;
#endif
							break;
						case _ADDBA_Rsp_ACTION_ID_:
							blockAck_para = pframe[5] | (pframe[6] << 8);
							status_code = pframe[3] | (pframe[4] << 8);
							pstat->AMSDU_AMPDU_support = blockAck_para & 0x1;
							TID = (blockAck_para>>2)&0x000f;
							max_size = (blockAck_para&0xffc0)>>6;
							pstat->StaRxBuf = max_size;
							if (status_code != _STATS_SUCCESSFUL_) {
								pstat->ADDBA_ready[TID] = 0;
							} else {
								DEBUG_INFO("%s %d increase ADDBA_ready, clear ADDBA_sent\n",__func__,__LINE__);
								pstat->ADDBA_ready[TID]++;
								pstat->ADDBA_sent[TID] = 0;
							}
							pstat->ADDBA_req_num[TID] = 0;
#ifdef TX_SHORTCUT
							do_tx_slowpath++;
#endif
							DEBUG_INFO("ADDBA-rsp recv fr AID %d, token %d TID %d size %d status %d\n",
								pstat->aid, pframe[2], TID, max_size, status_code);
							break;
						default:
							DEBUG_ERR("Error BA Action frame is received\n");
							goto error_frame;
							break;
					}
					break;

#if	defined(WIFI_11N_2040_COEXIST) || defined(P2P_SUPPORT) || defined(HS2_SUPPORT)
				case _PUBLIC_CATEGORY_ID_:
					switch (Action_field) {
						case _2040_COEXIST_ACTION_ID_:
							if (COEXIST_ENABLE) {
								if (!(OPMODE & WIFI_AP_STATE)) {
									DEBUG_WARN("Ignored Public Action frame received since this is not an AP\n");
									break;
								}
								if (!priv->pshare->is_40m_bw) {
									DEBUG_WARN("Ignored Public Action frame received since AP is 20m mode\n");
									break;
								}
								if (!(priv->pmib->dot11BssType.net_work_type & (WIRELESS_11N|WIRELESS_11G))) {
									DEBUG_WARN("Ignored Public Action frame received since AP is not 2.4G band\n");
									break;
								}
								if (pframe[2] == _2040_BSS_COEXIST_IE_) {
									if (pframe[4] & (_40M_INTOLERANT_ |_20M_BSS_WIDTH_REQ_)) {
										if (pframe[4] & _40M_INTOLERANT_) {
											DEBUG_INFO("Public Action frame: force 20m by 40m intolerant\n");
										} else {
											DEBUG_INFO("Public Action frame: force 20m by 20m bss width req\n");
										}

										setSTABitMap(&priv->switch_20_sta, pstat->aid);


										update_RAMask_to_FW(priv, 1);
										SetTxPowerLevel(priv, priv->pmib->dot11RFEntry.dot11channel);
									} else {
										if ((pframe[2+pframe[3]+2]) && (pframe[2+pframe[3]+2] == _2040_Intolerant_ChRpt_IE_)) {
											int ch_idx, ch_len= pframe[2+pframe[3]+2+1]-1, ch;
											DEBUG_INFO("Public Action frame: force 20m by channel report\n");
											for(ch_idx=0; ch_idx < ch_len; ch_idx++) {
												ch = pframe[2+pframe[3]+2+3 + ch_idx];
												if( ch && (ch<=14) && (priv->pmib->dot11nConfigEntry.dot11nCoexist_ch_chk?
												(ch != priv->pmib->dot11RFEntry.dot11channel):1) )
												{
													setSTABitMap(&priv->switch_20_sta, pstat->aid);

													update_RAMask_to_FW(priv, 1);
													SetTxPowerLevel(priv, priv->pmib->dot11RFEntry.dot11channel);
													break;
												}
											}
										} else {
											DEBUG_INFO("Public Action frame: cancel force 20m\n");
#if 0

										    clearSTABitMap(&priv->switch_20_sta, pstat->aid);

#if defined(WIFI_11N_2040_COEXIST_EXT)
											clearSTABitMap(&priv->pshare->_40m_staMap, pstat->aid);
#endif
											update_RAMask_to_FW(priv, 0);
#endif
										}
									}

#ifdef TX_SHORTCUT
									do_tx_slowpath++;
#endif
								} else {
									DEBUG_ERR("Error Public Action frame received\n");
								}
							} else {
								DEBUG_WARN("Public Action frame received but func off\n");
							}
							break;
#ifdef HS2_SUPPORT
                        case _GAS_INIT_REQ_ACTION_ID_:
                            //HS2DEBUG("Receive GAS Init REQ\n");
                            if (priv->pmib->hs2Entry.interworking_ielen)
                            {
                                int tmplen = pframe[4];
                                gas_req.Reqlen = (pframe[5+tmplen+1] << 8) | pframe[4+tmplen+1];
                                memcpy((void *)gas_req.MACAddr, (void *)sa, MACADDRLEN);
                                gas_req.EventId = DOT11_EVENT_GAS_INIT_REQ;
                                gas_req.IsMoreEvent = 0;
                                gas_req.Dialog_token = pframe[2];
                                gas_req.Advt_proto = pframe[4+tmplen];

                                memcpy(gas_req.Req, &pframe[5+tmplen+2], gas_req.Reqlen); //copy ANQP content

                                DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&gas_req,
                                        sizeof(DOT11_HS2_GAS_REQ));

                                event_indicate(priv, sa, 1);
                            }

                            break;
                        case _GAS_COMBACK_REQ_ACTION_ID_:
                            if (priv->pmib->hs2Entry.interworking_ielen)
                            {
                                memcpy((void *)gas_req.MACAddr, (void *)sa, MACADDRLEN);
                                HS2_DEBUG_INFO("GAS comback REQ Action frame\n");
                                gas_req.EventId = DOT11_EVENT_GAS_COMEBACK_REQ;
                                gas_req.Dialog_token = pframe[2];

                                DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&gas_req,
                                        sizeof(DOT11_HS2_GAS_REQ));

                                event_indicate(priv, sa, 1);
                            }
                            break;
#ifdef HS2_CLIENT_TEST
                        case _GAS_INIT_RSP_ACTION_ID_:
                            {
                                HS2DEBUG("GAS initial RSP Action frame\n");
                                if ((pframe[5] != 0) || (testflg == 1))
                                {
                                    memcpy((void *)gas_req.MACAddr, (void *)sa, MACADDRLEN);
                                    gas_req.Dialog_token = pframe[2];
                                    issue_GASreq(priv, &gas_req, 0);
                                    testflg = 0;
                                }
                            }
                            break;
                        case _GAS_COMBACK_RSP_ACTION_ID_:
                            {
                                HS2DEBUG("GAS combo RSP Action frame\n");
                                if ((pframe[5] & 0x80 )!= 0)
                                {
                                    memcpy((void *)gas_req.MACAddr, (void *)sa, MACADDRLEN);
                                    gas_req.Dialog_token = pframe[2];
                                    issue_GASreq(priv, &gas_req, 0);
                                }
                            }
                            break;
#endif
#endif

						default:
							DEBUG_INFO("Public Action frame received but not support yet\n");
							goto error_frame;
							break;
					}
					break;
#endif
#ifdef CONFIG_IEEE80211W
				case _SA_QUERY_CATEGORY_ID_:
    				PMFDEBUG("RX _SA_QUERY_CATEGORY_ID_\n");
					if (Action_field == _SA_QUERY_REQ_ACTION_ID_) {
						PMFDEBUG("RX 11W_SA_Req\n");
						issue_SA_Query_Rsp(priv->dev, sa, pframe+2);
						//if (IEEE8021X_FUN && priv->pmib->dot1180211AuthEntry.dot11EnablePSK) {
						//	PMFDEBUG("recv SA req..2\n");
						//	psk_indicate_evt(priv, DOT11_EVENT_SA_QUERY, GetAddr2Ptr(pframe), pframe, 0);
						//}
					} else if (Action_field == _SA_QUERY_RSP_ACTION_ID_) {
						int idx;
						unsigned short trans_id = (unsigned short) (pframe[3] << 8) + pframe[2];
						for (idx = 1; idx <= pstat->sa_query_count; idx++) {
							if (pstat->SA_TID[idx] == trans_id)
								break;
						}
						if (idx > pstat->sa_query_count) { // No match
							PMFDEBUG("RX 11W_SA_Rsp but comeback timeout\n");
							return SUCCESS;
						}
						PMFDEBUG("RX 11W_SA_Rsp\n");
						stop_sa_query(pstat);
					}
					break;
#endif // CONFIG_IEEE80211W
#ifdef CONFIG_IEEE80211V
				case _WNM_CATEGORY_ID_:
					if(WNM_ENABLE && pstat->bssTransSupport)
						WNM_ActionHandler(priv, pstat, pframe, (pfrinfo->pktlen - WLAN_HDR_A3_LEN));
					else
						DOT11VDEBUG("Warning: !WNM_ENABLE or !bssTransSupport. \n");
				break;
#endif
#ifdef RTK_AC_SUPPORT
				case _VHT_ACTION_CATEGORY_ID_:
					if (Action_field == _VHT_ACTION_OPMNOTIF_ID_) {
//						int nss = ((pframe[4]>>4)&0x7);
						if((pframe[2] &3) <= priv->pmib->dot11nConfigEntry.dot11nUse40M)
							pstat->tx_bw = pframe[2] &3;
						pstat->nss = ((pframe[2]>>4)&0x7)+1;
//						pstat->vht_cap_buf.vht_support_mcs[0] |=   cpu_to_le32(0xffff);
//						pstat->vht_cap_buf.vht_support_mcs[0] &= ~ cpu_to_le32((1<<((nss+1)<<1))-1);
//						panic_printk("Action 21, operating mode:%d, %d", pstat->tx_bw, pstat->nss);
						if (IS_HAL_CHIP(priv)){
							GET_HAL_INTERFACE(priv)->UpdateHalRAMaskHandler(priv, pstat, 3);
							pstat->H2C_rssi_rpt = 1;
							ODM_RAPostActionOnAssoc(ODMPTR);
							pstat->H2C_rssi_rpt = 0;
						}


					}
#if (MU_BEAMFORMING_SUPPORT == 1)
					else if (Action_field == _VHT_ACTION_GROUPID_ID_){
						if(GET_CHIP_VER(priv) == VERSION_8822B){
							int i;
							PRT_BEAMFORMING_INFO pBeamformingInfo = &(priv->pshare->BeamformingInfo);
							PRT_BEAMFORMER_ENTRY pBeamformerEntry = NULL;
							if (pBeamformingInfo->mu_ap_index < BEAMFORMER_ENTRY_NUM){
								pBeamformerEntry = &pBeamformingInfo->BeamformerEntry[pBeamformingInfo->mu_ap_index];
								//parse GID management frame (set group id and its corresponding position)
								for (i=0;i<8;i++){
									pBeamformerEntry->gid_valid[i] = pframe[i+2];
								}
								for (i=0;i<16;i++){
									pBeamformerEntry->user_position[i] = pframe[i+2+8];
								}
								HalTxbf8822B_ConfigGtab(priv);
							}

						}
					}
#endif

					break;
#endif


				case _HT_CATEGORY_ID_:
					if (Action_field == _HT_MIMO_PS_ACTION_ID_) {
						previous_mimo_ps = pstat->MIMO_ps;
						pstat->MIMO_ps = 0;
						if (pframe[2] & BIT(0)) {
							if (pframe[2] & BIT(1))
								pstat->MIMO_ps|=_HT_MIMO_PS_DYNAMIC_;
							else
								pstat->MIMO_ps|=_HT_MIMO_PS_STATIC_;
						}
						if ((previous_mimo_ps|pstat->MIMO_ps)&_HT_MIMO_PS_STATIC_) {
							assign_tx_rate(priv, pstat, pfrinfo);
							if (IS_HAL_CHIP(priv)) {
								GET_HAL_INTERFACE(priv)->UpdateHalRAMaskHandler(priv, pstat, 3);
							} else
							{
							}
						}
#ifdef TX_SHORTCUT
						if ((previous_mimo_ps|pstat->MIMO_ps)&_HT_MIMO_PS_DYNAMIC_)
							do_tx_slowpath++;
#endif
						check_NAV_prot_len(priv, pstat, 0);
					} else {
						DEBUG_INFO("HT Action Frame is received but not support yet\n");
					}
					break;
#ifdef HS2_SUPPORT
            case _WNM_CATEGORY_ID_:
                switch (Action_field) {
                    case _WNM_TSMQUERY_ACTION_ID_:
                        {
                            DOT11_HS2_TSM_REQ tsm_req;
                            int can_list = 0;
                            int payload = pfrinfo->pktlen - WLAN_HDR_A3_LEN;
                            tsm_req.Dialog_token = pframe[2];

                            HS2_DEBUG_INFO("got TSM QUERY frame\n");
                            if (payload > 4)
                            {
                                if (payload-4 > 100)
                                {
                                    tsm_req.list_len = 100;
                                    tsm_req.url_len = 0;
                                    HS2_DEBUG_ERR("payload too long!!\n");
                                    memcpy(tsm_req.Candidate_list, &pframe[4], 100);
                                    can_list = 1;
                                }
                                else
                                {
                                    tsm_req.list_len = payload-4;
                                    tsm_req.url_len = 0;
                                    memcpy(tsm_req.Candidate_list, &pframe[4], payload-4);
                                    can_list = 1;
                                }
                            }
                            else
                            {
                                tsm_req.url_len = 0;
                                tsm_req.list_len = 0;
                                can_list = 0;
                            }
                            memcpy(tsm_req.MACAddr, get_pframe(pfrinfo) + WLAN_ADDR_LEN + 4, MACADDRLEN);
                            tsm_req.Req_mode = priv->pmib->hs2Entry.reqmode | (can_list & 0x01);
                            tsm_req.Disassoc_timer = priv->pmib->hs2Entry.disassoc_timer;
                            if (can_list != 0)
                                tsm_req.Validity_intval = 200;
                            else
                                tsm_req.Validity_intval = priv->pmib->hs2Entry.validity_intval;
                            issue_BSS_TSM_req(priv, &tsm_req);
                        }
                        break;
                    case _BSS_TSMRSP_ACTION_ID_:
                        HS2_DEBUG_INFO("got TSM RSP frame\n");
                        HS2_DEBUG_INFO("token=%d\n", pframe[2]);
                        HS2_DEBUG_INFO("status code=%d\n", pframe[3]);
                        break;
#ifdef HS2_CLIENT_TEST
                    case _BSS_TSMREQ_ACTION_ID_:
                        printk("got TSM REQ frame\n");
                        if (pfrinfo->pktlen - WLAN_HDR_A3_LEN > 7)
                            issue_BSS_TSM_rsp(priv, &pframe[2], &pframe[7], pfrinfo->pktlen - WLAN_HDR_A3_LEN-7);
                        else
                            issue_BSS_TSM_rsp(priv, &pframe[2], NULL, 0);
                        break;
#endif
                }
                break;
            case _DLS_CATEGORY_ID_:
                printk("dls..action=%d\n", Action_field);
                switch (Action_field) {
                    case _DLS_REQ_ACTION_ID_:
                        printk("recv DLS frame\n");

                        issue_DLS_rsp(priv, 48, sa, &pframe[2], &pframe[8]); // status = 48 means that Direct link is not allowed in the BSS by policy)
                    break;
                };
                break;
#endif // HS2_SUPPORT
#ifdef TDLS_SUPPORT
			case _TDLS_CATEGORY_ID_:
				printk("TDLS..action=%d, tdls_prohibited = %d\n", Action_field, priv->pmib->dot11OperationEntry.tdls_prohibited);
             break;
#endif
				default:
					DEBUG_INFO("Action Frame is received but not support yet\n");
					break;
			}
#ifdef TX_SHORTCUT
			if (do_tx_slowpath) {
				/* let the first tx packet go through normal path and set fw properly */
				if (!priv->pmib->dot11OperationEntry.disable_txsc) {
					int i;
					for (i=0; i<TX_SC_ENTRY_NUM; i++) {
                        if (IS_HAL_CHIP(priv)) {
                            GET_HAL_INTERFACE(priv)->SetShortCutTxBuffSizeHandler(priv, pstat->tx_sc_ent[i].hal_hw_desc, 0);
                        } else if(CONFIG_WLAN_NOT_HAL_EXIST)
                        {//not HAL
						pstat->tx_sc_ent[i].hwdesc1.Dword7 &= set_desc(~TX_TxBufSizeMask);
#ifdef TX_SCATTER
						pstat->tx_sc_ent[i].has_desc3 = 0;
#endif
						 }
					}
				}
			}
#endif
		} else {
			if (IS_MCAST(da)) {
				//DEBUG_ERR("Error Broadcast or Multicast Action Frame is received\n");
			}
			else {
				//DEBUG_ERR("Action Frame is received from non-associated station\n");
			}
		}
	}
error_frame:
#endif
	return SUCCESS;
}


unsigned int DoReserved(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
	return SUCCESS;
}


#ifdef CLIENT_MODE
void update_bss(struct Dot11StationConfigEntry *dst, struct bss_desc *src)
{
	memcpy((void *)dst->dot11Bssid, (void *)src->bssid, MACADDRLEN);
	memset((void *)dst->dot11DesiredSSID, 0, sizeof(dst->dot11DesiredSSID));
	memcpy((void *)dst->dot11DesiredSSID, (void *)src->ssid, src->ssidlen);
	dst->dot11DesiredSSIDLen = src->ssidlen;
}



/**
 *	@brief	Authenticat success, Join a BSS
 *
 *	Set BSSID to hardware, Join BSS complete
 */
void join_bss(struct rtl8192cd_priv *priv)
{
	unsigned short	val16;
	unsigned long	val32;

	memcpy((void *)&val32, BSSID, 4);
	memcpy((void *)&val16, BSSID+4, 2);

#ifdef SDIO_2_PORT
	if (IS_VXD_INTERFACE(priv)) {
		RTL_W32(BSSIDR1, cpu_to_le32(val32));
		RTL_W16((BSSIDR1 + 4), cpu_to_le16(val16));
	} else
#endif
	{
		RTL_W32(BSSIDR, cpu_to_le32(val32));
		RTL_W16((BSSIDR + 4), cpu_to_le16(val16));
	}
}


/**
 *	@brief	issue Association Request
 *
 *	STA find compatible network, and authenticate success, use this function send association request. \n
 *	+---------------+----+----+-------+------------+-----------------+------+-----------------+	\n
 *	| Frame Control | DA | SA | BSSID | Capability | Listen Interval | SSID | Supported Rates |	\n
 *	+---------------+----+----+-------+------------+-----------------+------+-----------------+	\n
 *	\n
 *	+--------------------+---------------------+-----------------+	\n
 *  | Ext. Support Rates | Realtek proprietary | RSN information |	\n
 *	+--------------------+---------------------+-----------------+	\n
 *
 *	PS: Reassociation Frame Body have Current AP Address field, But not implement.
 */
unsigned int issue_assocreq(struct rtl8192cd_priv *priv)
{
	unsigned short	val;
	struct wifi_mib *pmib;
	unsigned char	*bssid, *pbuf;
	unsigned char	*pbssrate=NULL;
	int		bssrate_len;
	unsigned char	supportRateSet[32];
	int		i, j, idx=0, supportRateSetLen=0, match=0;
	unsigned int	retval=0;

	unsigned char extended_cap_ie[8];
	memset(extended_cap_ie, 0, 8);

#ifdef WIFI_WMM
	int		k;
#endif
#if 0 //def SUPPORT_MULTI_PROFILE
    int found=0;
    int jdx=0;
    int ssid_len=0;
#endif

	DECLARE_TXINSN(txinsn);

	txinsn.retry = priv->pmib->dot11OperationEntry.dot11ShortRetryLimit;

	pmib= GET_MIB(priv);

	bssid = pmib->dot11Bss.bssid;

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.fr_type = _PRE_ALLOCMEM_;
    	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
	txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif
	txinsn.fixed_rate = 1;
	pbuf = txinsn.pframe = get_mgtbuf_from_poll(priv);

	if (pbuf == NULL)
		goto issue_assocreq_fail;

	txinsn.phdr = get_wlanhdr_from_poll(priv);

	if (txinsn.phdr == NULL)
		goto issue_assocreq_fail;
	memset((void *)txinsn.phdr, 0, sizeof(struct  wlan_hdr));

	val = cpu_to_le16(pmib->dot11Bss.capability);

	if (pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm)
		val |= cpu_to_le16(BIT(4));

	if (SHORTPREAMBLE)
		val |= cpu_to_le16(BIT(5));

#ifdef DOT11H
    if(priv->pmib->dot11hTPCEntry.tpc_enable)
        val |= cpu_to_le16(BIT(8));	/* set spectrum mgt */
#endif


	pbuf = set_fixed_ie(pbuf, _CAPABILITY_, (unsigned char *)&val, &txinsn.fr_len);

	val	 = cpu_to_le16(3);
	pbuf = set_fixed_ie(pbuf, _LISTEN_INTERVAL_, (unsigned char *)&val, &txinsn.fr_len);

	pbuf = set_ie(pbuf, _SSID_IE_, pmib->dot11Bss.ssidlen, pmib->dot11Bss.ssid, &txinsn.fr_len);

	if (pmib->dot11Bss.supportrate == 0)
	{
		// AP don't contain rate info in beacon/probe response
		// Use our rate in asoc req
		get_bssrate_set(priv, _SUPPORTEDRATES_IE_, &pbssrate, &bssrate_len);
		pbuf = set_ie(pbuf, _SUPPORTEDRATES_IE_, bssrate_len, pbssrate, &txinsn.fr_len);

		//EXT supported rates.
		if (get_bssrate_set(priv, _EXT_SUPPORTEDRATES_IE_, &pbssrate, &bssrate_len))
			pbuf = set_ie(pbuf, _EXT_SUPPORTEDRATES_IE_, bssrate_len, pbssrate, &txinsn.fr_len);
	}
	else
	{
		// See if there is any mutual supported rate
		for (i=0; dot11_rate_table[i]; i++) {
			int bit_mask = 1 << i;
			if (pmib->dot11Bss.supportrate & bit_mask) {
				val = dot11_rate_table[i];
				for (j=0; j<AP_BSSRATE_LEN; j++) {
					if (val == (AP_BSSRATE[j] & 0x7f)) {
						match = 1;
						break;
					}
				}
				if (match)
					break;
			}
		}

		// If no supported rates match, assoc fail!
		if (!match) {
			DEBUG_ERR("Supported rate mismatch!\n");
			retval = 1;
			goto issue_assocreq_fail;
		}

		// Use AP's rate info in asoc req
		for (i=0; dot11_rate_table[i]; i++) {
			int bit_mask = 1 << i;
			if (pmib->dot11Bss.supportrate & bit_mask) {
				val = dot11_rate_table[i];
				if (((pmib->dot11BssType.net_work_type == WIRELESS_11B) && is_CCK_rate(val)) ||
					(pmib->dot11BssType.net_work_type != WIRELESS_11B)) {
					if (pmib->dot11Bss.basicrate & bit_mask)
						val |= 0x80;

					supportRateSet[idx] = val;
					supportRateSetLen++;
					idx++;
				}
			}
		}

		if (supportRateSetLen == 0) {
			retval = 1;
			goto issue_assocreq_fail;
		}
		else if (supportRateSetLen <= 8)
			pbuf = set_ie(pbuf, _SUPPORTEDRATES_IE_ , supportRateSetLen , supportRateSet, &txinsn.fr_len);
		else {
			pbuf = set_ie(pbuf, _SUPPORTEDRATES_IE_, 8, supportRateSet, &txinsn.fr_len);
			pbuf = set_ie(pbuf, _EXT_SUPPORTEDRATES_IE_, supportRateSetLen-8, &supportRateSet[8], &txinsn.fr_len);
		}
	}


#if defined(DOT11H) || defined(DOT11K)
    if(pmib->dot11hTPCEntry.tpc_enable || pmib->dot11StationConfigEntry.dot11RadioMeasurementActivated)
    {
        pbuf = construct_power_capability_ie(priv, pbuf, &txinsn.fr_len);
    }

#ifdef DOT11H
    if(pmib->dot11hTPCEntry.tpc_enable)
    {
        pbuf = construct_supported_channel_ie(priv, pbuf, &txinsn.fr_len);
    }
#endif

#endif
       /*RSN IE*/
#ifdef WIFI_SIMPLE_CONFIG
		if (!(pmib->wscEntry.wsc_enable && pmib->wscEntry.assoc_ielen))
#endif
		{
#if 0
//#ifdef SUPPORT_MULTI_PROFILE
            /*echo profile maintain self RNSIE,when issue_assoc chk and try to use itself RSNIE ,
                            the fail case is when RTK AP use mixed mode we use wrong macst cipher type*/
            if ( priv->pmib->ap_profile.enable_profile && priv->pmib->ap_profile.profile_num > 0) {

                int RSNIE_LEN=0;
                unsigned char RSNIE[128];
                for(jdx=0 ; jdx<priv->pmib->ap_profile.profile_num ; jdx++) {
    				ssid_len = strlen(priv->pmib->ap_profile.profile[jdx].ssid);
                    ///STADEBUG("be chk ssid=%s,idx=%d\n", priv->pmib->ap_profile.profile[jdx].ssid,jdx);
                    if((ssid_len == pmib->dot11Bss.ssidlen) && !memcmp(priv->pmib->ap_profile.profile[jdx].ssid , pmib->dot11Bss.ssid,ssid_len ))
                    {
                        ///STADEBUG("Found;my[%s],target[%s]\n", priv->pmib->ap_profile.profile[jdx].ssid,pmib->dot11Bss.ssid);
                        priv->profile_idx = jdx;
                        ///STADEBUG("call switch_profile(%d)\n",priv->profile_idx);
						switch_profile(priv, priv->profile_idx);

                        if(priv->pmib->ap_profile.profile[jdx].MulticastCipher){

                            priv->wpa_global_info->MulticastCipher=priv->pmib->ap_profile.profile[jdx].MulticastCipher;
                            ///STADEBUG("MulticastCipher=%d\n", priv->wpa_global_info->MulticastCipher);
                            ///STADEBUG("dot11EnablePSK=[%d]\n", priv->pmib->dot1180211AuthEntry.dot11EnablePSK);
                            ConstructIE(priv, RSNIE,&RSNIE_LEN);
                            ///STADEBUG("AuthInfoElement len=%d\n", RSNIE_LEN);
                            if(RSNIE_LEN){
                                memcpy(pbuf, RSNIE,RSNIE_LEN);
                                pbuf += RSNIE_LEN;
                                txinsn.fr_len += RSNIE_LEN;
                                STADEBUG("Use profile's RSNIE[%s]\n",  priv->pmib->ap_profile.profile[jdx].ssid);
                            }

                        }
                        else{
                            ///STADEBUG("NO RSNIE\n");
                        }
                        break;
                    }

                }


            }
            else
#endif
            {
                //STADEBUG("normal case [ssid=%s]\n", pmib->dot11Bss.ssid);
    			if (pmib->dot11RsnIE.rsnielen && priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm) {
    				memcpy(pbuf, pmib->dot11RsnIE.rsnie, pmib->dot11RsnIE.rsnielen);
    				pbuf += pmib->dot11RsnIE.rsnielen;
    				txinsn.fr_len += pmib->dot11RsnIE.rsnielen;
    			}
    		}
	}

	if ((QOS_ENABLE) || (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)) {
		int count=0;
		struct bss_desc	*bss=NULL;

		if (priv->site_survey->count) {
			count = priv->site_survey->count;
			bss = priv->site_survey->bss;
		}
		else if (priv->site_survey->count_backup) {
			count = priv->site_survey->count_backup;
			bss = priv->site_survey->bss_backup;
		}

		for(k=0; k<count; k++) {
			if (!memcmp((void *)bssid, bss[k].bssid, MACADDRLEN)) {

#ifdef WIFI_WMM
				//  AP supports WMM when t_stamp[1] bit 0 is set
				if ((QOS_ENABLE) && (bss[k].t_stamp[1] & BIT(0))) {
#ifdef WMM_APSD
					if (APSD_ENABLE) {
						if (bss[k].t_stamp[1] & BIT(3))
							priv->uapsd_assoc++;
						else
							priv->uapsd_assoc = 0;

						init_WMM_Para_Element(priv, priv->pmib->dot11QosEntry.WMM_IE);
					}
#endif
					pbuf = set_ie(pbuf, _RSN_IE_1_, _WMM_IE_Length_, GET_WMM_IE, &txinsn.fr_len);
				}
#endif
				if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
						(bss[k].network & WIRELESS_11N)) {

					int is_40m_bw, offset_chan;
					if (!IS_ROOT_INTERFACE(priv) && !GET_ROOT(priv)->pmib->dot11nConfigEntry.dot11nUse40M)
						is_40m_bw=0;
					else
						is_40m_bw = (bss[k].t_stamp[1] & BIT(1)) ? 1 : 0;

					if (is_40m_bw) {
						if (bss[k].t_stamp[1] & BIT(2))
							offset_chan = 1;
						else
							offset_chan = 2;
					}
					else
						offset_chan = 0;

					priv->ht_cap_len = 0;	// re-construct HT IE
					construct_ht_ie(priv, is_40m_bw, offset_chan);
					pbuf = set_ie(pbuf, _HT_CAP_, priv->ht_cap_len, (unsigned char *)&priv->ht_cap_buf, &txinsn.fr_len);
#ifdef RTK_AC_SUPPORT
					if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC) {
						construct_vht_ie(priv, priv->pshare->working_channel);
						pbuf = set_ie(pbuf, EID_VHTCapability, priv->vht_cap_len, (unsigned char *)&priv->vht_cap_buf, &txinsn.fr_len);
						pbuf = set_ie(pbuf, EID_VHTOperation, priv->vht_oper_len, (unsigned char *)&priv->vht_oper_buf, &txinsn.fr_len);
						if(priv->pshare->rf_ft_var.opmtest&1) {
							extended_cap_ie[7] = 0x40;
						}
					}
#endif
		#ifdef CONFIG_IEEE80211V_CLI
					if(WNM_ENABLE) {
						extended_cap_ie[2] |= _WNM_BSS_TRANS_SUPPORT_ ;
					}
		#endif
					pbuf = set_ie(pbuf, _EXTENDED_CAP_IE_, 8, extended_cap_ie, &txinsn.fr_len);
				}
				break;
			}
		}
	}

#ifdef WIFI_SIMPLE_CONFIG

	if (pmib->wscEntry.wsc_enable && pmib->wscEntry.assoc_ielen) {
		memcpy(pbuf, pmib->wscEntry.assoc_ie, pmib->wscEntry.assoc_ielen);
		pbuf += pmib->wscEntry.assoc_ielen;
		txinsn.fr_len += pmib->wscEntry.assoc_ielen;
	}
	if (priv->pmib->wscEntry.both_band_multicredential) {
		pbuf = set_ie(pbuf, 221, 7, "\x00\x0D\x02\x06\x01\02\01", &txinsn.fr_len);
	}
#endif

#ifdef A4_STA
    if(priv->pshare->rf_ft_var.a4_enable == 2) {
        pbuf = construct_ecm_tvm_ie(priv, pbuf, &txinsn.fr_len, BIT0);
    }
#endif

#ifdef TV_MODE
    if(priv->tv_mode_status > 0) {
        pbuf = construct_tv_mode_ie(priv, pbuf, &txinsn.fr_len);
    }
#endif


	// Realtek proprietary IE
	if (priv->pshare->rtk_ie_len)
		pbuf = set_ie(pbuf, _RSN_IE_1_, priv->pshare->rtk_ie_len, priv->pshare->rtk_ie_buf, &txinsn.fr_len);

	// Customer proprietary IE
	if (priv->pmib->miscEntry.private_ie_len) {
		memcpy(pbuf, pmib->miscEntry.private_ie, pmib->miscEntry.private_ie_len);
		pbuf += pmib->miscEntry.private_ie_len;
		txinsn.fr_len += pmib->miscEntry.private_ie_len;
	}

	SetFrameSubType((txinsn.phdr), WIFI_ASSOCREQ);

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), bssid, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), GET_MY_HWADDR, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), bssid, MACADDRLEN);

	if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS)
	{
		unsigned char *assocreq_ie = txinsn.pframe + 4; //ignore fix ies
		int assocreq_ie_len = (txinsn.fr_len-4);

		//printk("AssocReq Len = %d\n", assocreq_ie_len);
		if(assocreq_ie_len > MAX_ASSOC_REQ_LEN)
		{
			printk("AssocReq Len too LONG !!\n");
			memcpy(priv->rtk->clnt_info.assoc_req, assocreq_ie, MAX_ASSOC_REQ_LEN);
			priv->rtk->clnt_info.assoc_req_len = MAX_ASSOC_REQ_LEN;
		}
		else
		{
			memcpy(priv->rtk->clnt_info.assoc_req, assocreq_ie, assocreq_ie_len);
			priv->rtk->clnt_info.assoc_req_len = assocreq_ie_len;
		}
		return retval;
	}

issue_assocreq_fail:

	DEBUG_ERR("sending assoc req fail!\n");

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
	if (txinsn.pframe)
		release_mgtbuf_to_poll(priv, txinsn.pframe);
	return retval;
}

void ap_sync_chan_to_bss(struct rtl8192cd_priv *priv, int bss_channel, int bss_bw, int bss_offset)
{
    int i, j;
    struct rtl8192cd_priv *vap_priv;
    struct stat_info * pstat;
    STADEBUG("===>\n");
    for(j=0; j<NUM_STAT; j++)
    {
        if (priv->pshare->aidarray[j] && (priv->pshare->aidarray[j]->used == TRUE)
        ) {
            if (priv != priv->pshare->aidarray[j]->priv)
                continue;

            pstat = &(priv->pshare->aidarray[j]->station);
            {
                issue_deauth(priv, pstat->hwaddr, _RSON_DEAUTH_STA_LEAVING_);
            }
        }
    }
    delay_ms(10);

    //sync channel
    priv->pmib->dot11RFEntry.dot11channel = bss_channel;

    if(bss_channel <= 14)
		priv->pmib->dot11RFEntry.phyBandSelect = PHY_BAND_2G;
	else
		priv->pmib->dot11RFEntry.phyBandSelect = PHY_BAND_5G;

    //sync bw
    priv->pmib->dot11nConfigEntry.dot11nUse40M = bss_bw;
    priv->pshare->CurrentChannelBW = bss_bw;
    priv->pshare->is_40m_bw = bss_bw;

    //sync offset
    priv->pmib->dot11nConfigEntry.dot11n2ndChOffset = bss_offset;
    priv->pshare->offset_2nd_chan =  bss_offset;

    //regen ht ie
    priv->ht_cap_len = 0;
    priv->ht_ie_len = 0;
	if(!priv->pmib->dot11DFSEntry.disable_DFS && is_DFS_channel(priv->pmib->dot11RFEntry.dot11channel)) {
		if (timer_pending(&priv->DFS_timer))
			del_timer(&priv->DFS_timer);

		if (timer_pending(&priv->ch_avail_chk_timer))
			del_timer(&priv->ch_avail_chk_timer);

		if (timer_pending(&priv->dfs_det_chk_timer))
			del_timer(&priv->dfs_det_chk_timer);

		init_timer(&priv->ch_avail_chk_timer);
		priv->ch_avail_chk_timer.data = (unsigned long) priv;
		priv->ch_avail_chk_timer.function = rtl8192cd_ch_avail_chk_timer;

		if ((priv->pmib->dot11StationConfigEntry.dot11RegDomain == DOMAIN_ETSI) &&
			(IS_METEOROLOGY_CHANNEL(priv->pmib->dot11RFEntry.dot11channel)))
			mod_timer(&priv->ch_avail_chk_timer, jiffies + CH_AVAIL_CHK_TO_CE);
		else
			mod_timer(&priv->ch_avail_chk_timer, jiffies + CH_AVAIL_CHK_TO);

		init_timer(&priv->DFS_timer);
		priv->DFS_timer.data = (unsigned long) priv;
		priv->DFS_timer.function = rtl8192cd_DFS_timer;


		/* DFS activated after 5 sec; prevent switching channel due to DFS false alarm */
		mod_timer(&priv->DFS_timer, jiffies + RTL_SECONDS_TO_JIFFIES(5));

		init_timer(&priv->dfs_det_chk_timer);
		priv->dfs_det_chk_timer.data = (unsigned long) priv;
		priv->dfs_det_chk_timer.function = rtl8192cd_dfs_det_chk_timer;

		mod_timer(&priv->dfs_det_chk_timer, jiffies + RTL_MILISECONDS_TO_JIFFIES(priv->pshare->rf_ft_var.dfs_det_period*10));

		DFS_SetReg(priv);

		if (!priv->pmib->dot11DFSEntry.CAC_enable) {
			del_timer_sync(&priv->ch_avail_chk_timer);
			mod_timer(&priv->ch_avail_chk_timer, jiffies + RTL_MILISECONDS_TO_JIFFIES(200));
		}
	}
    /*The  beacon IE content before TIM(TLV) need call init_beacon for update ;
           for example channle info(Direct Sequence Parameter Set)
           else content can update by update_beacon()
        */
    init_beacon(priv);

	if (priv->pmib->miscEntry.vap_enable) {
		for (i=0; i<RTL8192CD_NUM_VWLAN; i++) {
			vap_priv = priv->pvap_priv[i];
			if (vap_priv && IS_DRV_OPEN(vap_priv)) {
				if ((GET_MIB(vap_priv))->dot11OperationEntry.opmode & WIFI_AP_STATE)
				{
				for(j=0; j<NUM_STAT; j++)
				{
					if (priv->pshare->aidarray[j] && (priv->pshare->aidarray[j]->used == TRUE)) {
						if (vap_priv != priv->pshare->aidarray[j]->priv)
							continue;
						issue_deauth(vap_priv, priv->pshare->aidarray[j]->station.hwaddr, _RSON_DEAUTH_STA_LEAVING_);
					}
				}
				delay_ms(10);
				}

				vap_priv->pmib->dot11RFEntry.dot11channel = bss_channel;           //sync channel
				vap_priv->pmib->dot11nConfigEntry.dot11nUse40M = bss_bw;           //sync bandwidth
				vap_priv->pmib->dot11nConfigEntry.dot11n2ndChOffset = bss_offset;  //sync 2nd ch offset

				vap_priv->ht_cap_len = 0;                //regen ht ie
				vap_priv->ht_ie_len = 0;
				init_beacon(vap_priv);
			}
		}
	}

}

void clnt_switch_chan_to_bss(struct rtl8192cd_priv *priv)
{
	int bss_channel, bss_bw, bss_offset = 0;

    STADEBUG("===>\n");
	//sync channel
	bss_channel = priv->pmib->dot11Bss.channel;

	if(bss_channel <= 14)
		priv->pmib->dot11RFEntry.phyBandSelect = PHY_BAND_2G;
	else
		priv->pmib->dot11RFEntry.phyBandSelect = PHY_BAND_5G;

	//sync bw & offset
	bss_bw = HT_CHANNEL_WIDTH_20_40;

#ifdef RTK_AC_SUPPORT
	if(GET_CHIP_VER(priv)==VERSION_8812E || GET_CHIP_VER(priv)==VERSION_8881A || GET_CHIP_VER(priv)==VERSION_8814A || GET_CHIP_VER(priv)==VERSION_8822B){
		if((priv->pmib->dot11Bss.t_stamp[1] & (BSS_BW_MASK << BSS_BW_SHIFT))
			== (HT_CHANNEL_WIDTH_80 << BSS_BW_SHIFT))
			bss_bw = HT_CHANNEL_WIDTH_80;
	}
#endif

	if ((priv->pmib->dot11Bss.t_stamp[1] & (BIT(1) | BIT(2))) == (BIT(1) | BIT(2)))
		bss_offset = HT_2NDCH_OFFSET_BELOW;
	else if ((priv->pmib->dot11Bss.t_stamp[1] & (BIT(1) | BIT(2))) == BIT(1))
		bss_offset = HT_2NDCH_OFFSET_ABOVE;
	else {
		if (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_10)
			bss_bw = HT_CHANNEL_WIDTH_10;
		else if (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_5)
			bss_bw = HT_CHANNEL_WIDTH_5;
		else
			bss_bw = HT_CHANNEL_WIDTH_20;

		bss_offset = HT_2NDCH_OFFSET_DONTCARE;
	}


	//sync channel
	priv->pmib->dot11RFEntry.dot11channel = bss_channel;

	//sync bw
	priv->pmib->dot11nConfigEntry.dot11nUse40M = bss_bw;
	priv->pshare->CurrentChannelBW = bss_bw;
	priv->pshare->is_40m_bw = bss_bw;

	//sync offset
	priv->pmib->dot11nConfigEntry.dot11n2ndChOffset = bss_offset;
	priv->pshare->offset_2nd_chan =  bss_offset;

    /*3.4.6,wlanx-vax interface also can support STA mode*/
	if( !IS_ROOT_INTERFACE(priv)
	)
	{
		struct rtl8192cd_priv *priv_root = GET_ROOT(priv);
//#if defined(SMART_REPEATER_MODE) && !defined(RTK_NL80211)
#if 0	//removed to prevent system hang-up after repeater follow remote AP to switch channel
		priv->pshare->switch_chan_rp = bss_channel;
		priv->pshare->band_width_rp = bss_bw;
		priv->pshare->switch_2ndchoff_rp = bss_offset;
#endif
		if(IS_DRV_OPEN(priv_root) || (priv_root->pmib->miscEntry.vap_enable))
		if ((priv_root->pmib->dot11RFEntry.dot11channel != bss_channel) ||
			(priv_root->pmib->dot11nConfigEntry.dot11nUse40M != bss_bw) ||
			(priv_root->pmib->dot11nConfigEntry.dot11n2ndChOffset != bss_offset)  ) {
				ap_sync_chan_to_bss(priv_root, bss_channel, bss_bw, bss_offset);
		}
//#if defined(SMART_REPEATER_MODE) && !defined(RTK_NL80211)
#if 0//removed to prevent system hang-up after repeater follow remote AP to switch channel
		priv->pshare->switch_chan_rp = 0;
#endif
	}

	// when STA want to connect target AP make sure TX_PAUSE don't pause packet
	priv->site_survey->target_ap_found=1;
	SwBWMode(priv, bss_bw, bss_offset);
	SwChnl(priv, bss_channel, bss_offset);
	priv->site_survey->target_ap_found=0;
}


/**
 *	@brief	STA Authentication
 *
 *	STA process Authentication Request first step.
 */
void start_clnt_auth(struct rtl8192cd_priv *priv)
{
	unsigned long flags;


	SAVE_INT_AND_CLI(flags);

	OPMODE_VAL(OPMODE & ~(WIFI_AUTH_SUCCESS | WIFI_AUTH_STATE1 | WIFI_ASOC_STATE));
	OPMODE_VAL(OPMODE | WIFI_AUTH_NULL);
	REAUTH_COUNT_VAL(0);
	REASSOC_COUNT_VAL(0);
	AUTH_SEQ_VAL(1);

	if (PENDING_REAUTH_TIMER)
		DELETE_REAUTH_TIMER;
	if (PENDING_REASSOC_TIMER)
		DELETE_REASSOC_TIMER;



	{

#if 0 // do not switch to bw=20 when issue auth
		priv->pshare->CurrentChannelBW = HT_CHANNEL_WIDTH_20;
		SwBWMode(priv, priv->pshare->CurrentChannelBW, 0);

		SwChnl(priv, priv->pmib->dot11Bss.channel, 0);
#else
		clnt_switch_chan_to_bss(priv); // Eric
#endif

	}


	MOD_REAUTH_TIMER(REAUTH_TO);

	{
		DEBUG_INFO("start sending auth req\n");
		//STADEBUG("tx auth req\n");
		issue_auth(priv, NULL, 0);
	}



	RESTORE_INT(flags);
}


/**
 *	@brief	client (STA) association
 *
 *	PS: clnt is client.
 */
void start_clnt_assoc(struct rtl8192cd_priv *priv)
{
	unsigned long flags;

	// now auth has succedded...let's perform assoc
	SAVE_INT_AND_CLI(flags);

	OPMODE_VAL(OPMODE & (~ (WIFI_AUTH_NULL | WIFI_AUTH_STATE1 | WIFI_ASOC_STATE)));
	OPMODE_VAL(OPMODE | (WIFI_AUTH_SUCCESS));
	JOIN_RES_VAL(STATE_Sta_Auth_Success);
	REAUTH_COUNT_VAL(0);
	REASSOC_COUNT_VAL(0);

	if (PENDING_REAUTH_TIMER)
		DELETE_REAUTH_TIMER;
	if (PENDING_REASSOC_TIMER)
		DELETE_REASSOC_TIMER;

	DEBUG_INFO("start sending assoc req\n");
	if (issue_assocreq(priv) == 0) {
		MOD_REASSOC_TIMER(REASSOC_TO);
		RESTORE_INT(flags);
	}
	else {
		RESTORE_INT(flags);
#ifdef WIFI_WPAS_CLI
		event_indicate_wpas(priv, NULL, WPAS_DISCON, NULL);
#else
		STADEBUG("start_clnt_lookup(DONTRESCAN)\n");
		start_clnt_lookup(priv, DONTRESCAN);
#endif
	}
}


void clean_for_join(struct rtl8192cd_priv *priv)
{
	int i;
	unsigned long flags;
			/*cfg p2p cfg p2p ; remove*/
	SAVE_INT_AND_CLI(flags);

	for(i=0; i<NUM_STAT; i++) {
		if (priv->pshare->aidarray[i] && (priv->pshare->aidarray[i]->used == TRUE)) {
			if (priv != priv->pshare->aidarray[i]->priv)
				continue;
			if ((free_stainfo(priv, &(priv->pshare->aidarray[i]->station))) == FALSE)
				DEBUG_ERR("free station %d fails\n", i);
		}
	}

	priv->assoc_num = 0;

	memset(BSSID, 0, MACADDRLEN);
    /*cfg p2p cfg p2p ; remove*/
    OPMODE_VAL(OPMODE & (WIFI_STATION_STATE | WIFI_ADHOC_STATE));

	/*cfg p2p cfg p2p ; remove*/

	//P2P_DEBUG("\n\n\n");

	if ((OPMODE & WIFI_STATION_STATE) &&
			((priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _TKIP_PRIVACY_) ||
			 (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _CCMP_PRIVACY_) ||
			 (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_WPA_MIXED_PRIVACY_))) {
		memset(&(priv->pmib->dot11GroupKeysTable), 0, sizeof(struct Dot11KeyMappingsEntry));
		if (IS_ROOT_INTERFACE(priv))
			CamResetAllEntry(priv);
	}

	if (IEEE8021X_FUN)
		priv->pmib->dot118021xAuthEntry.dot118021xcontrolport =
			priv->pmib->dot118021xAuthEntry.dot118021xDefaultPort;
	else
		priv->pmib->dot118021xAuthEntry.dot118021xcontrolport = 1;

	if (priv->pmib->dot11BssType.net_work_type & (WIRELESS_11G|WIRELESS_11A)) {
		if (OPMODE & WIFI_ADHOC_STATE) {
			priv->pmib->dot11ErpInfo.nonErpStaNum = 0;
			check_protection_shortslot(priv);
			priv->pmib->dot11ErpInfo.longPreambleStaNum = 0;
		}
	}

	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)
		priv->ht_legacy_sta_num = 0;

	JOIN_RES_VAL(STATE_Sta_No_Bss);
	priv->link_status = 0;
	//netif_stop_queue(priv->dev);		// don't start/stop queue dynamically
	priv->rxBeaconNumInPeriod = 0;
	memset(priv->rxBeaconCntArray, 0, sizeof(priv->rxBeaconCntArray));
	priv->rxBeaconCntArrayIdx = 0;
	priv->rxBeaconCntArrayWindow = 0;
	priv->rxBeaconPercentage = 0;
	priv->rxDataNumInPeriod = 0;
	memset(priv->rxDataCntArray, 0, sizeof(priv->rxDataCntArray));
	priv->rxMlcstDataNumInPeriod = 0;
//	priv->rxDataNumInPeriod_pre = 0;
//	priv->rxMlcstDataNumInPeriod_pre = 0;
	RESTORE_INT(flags);
}

unsigned int mod64(unsigned int A1, unsigned int A2, unsigned int b)
{
	unsigned int r;
	r = A1%b;
	r = (r<<12) | ((A2>>20)&0x0fff);
	r %=b;
	r = (r<<12) | ((A2>>8)&0x0fff);
	r %=b;
	r = (r<<8) | (A2&0xff);
	r %=b;
//	DEBUG_INFO("A1=%u, A2=%u, b=%u, r=%u\n", A1, A2, b, r);
	return r;
}

void updateTSF(struct rtl8192cd_priv *priv)
{
	UINT64 tsf;
	unsigned int r ;

	if (priv->beacon_period == 0)
		return;

	tsf = *((UINT64*)priv->rx_timestamp);
	tsf = le64_to_cpu(tsf);
	if( tsf > 1024) {
		r = mod64(tsf>>32, tsf&0xffffffff, priv->beacon_period*1024);
		tsf = tsf -r -1024;
		priv->prev_tsf = tsf;
		RTL_W8(TXPAUSE, RTL_R8(TXPAUSE) | BIT(6));

#ifdef SDIO_2_PORT
		if (IS_VXD_INTERFACE(priv)) {
			RTL_W8(DUAL_TSF_RST, BIT(1));
			RTL_W8(BCN_CTRL1, RTL_R8(BCN_CTRL1) & ~ (EN_BCN_FUNCTION));
		} else

#endif
		{
			RTL_W8(DUAL_TSF_RST, BIT(0));
			RTL_W8(BCN_CTRL, RTL_R8(BCN_CTRL) & ~ (EN_BCN_FUNCTION));
		}

		RTL_W32(TSFTR, (unsigned int)(tsf&0xffffffff));
		RTL_W32(TSFTR+4, (unsigned int)(tsf>>32));

#ifdef SDIO_2_PORT
		if (IS_VXD_INTERFACE(priv)) {
			RTL_W8(BCN_CTRL1, RTL_R8(BCN_CTRL1) | DIS_ATIM);
		} else
#endif
		{
			RTL_W8(BCN_CTRL, RTL_R8(BCN_CTRL) | EN_BCN_FUNCTION);

			if(OPMODE & WIFI_STATION_STATE)
				RTL_W8(BCN_CTRL, RTL_R8(BCN_CTRL) | DIS_ATIM);
		}
		RTL_W8(TXPAUSE, RTL_R8(TXPAUSE) ^ BIT(6));
	}
}


/**
 *	@brief	STA join BSS
 *	Join a BSS, In function, emit join request, Before association must be Authentication. \n
 *	[NOTE] TPT information element
 */
void start_clnt_join(struct rtl8192cd_priv *priv)
{
	struct wifi_mib *pmib = GET_MIB(priv);
	unsigned char null_mac[]={0,0,0,0,0,0};
	unsigned char random;
	int i;

	if (priv->pmib->dot11DFSEntry.disable_tx)
		priv->pmib->dot11DFSEntry.disable_tx = 0;



#ifdef WIFI_SIMPLE_CONFIG
	if (priv->pmib->wscEntry.wsc_enable == 1) { //wps client mode
		if (priv->wps_issue_join_req)
			priv->wps_issue_join_req = 0;
		else {
			priv->recover_join_req = 1;
			return;
		}
	}
#endif

// stop ss_timer before join ------------------------
	if (timer_pending(&priv->ss_timer))
		del_timer(&priv->ss_timer);
//------------------------------- david+2007-03-10

	// if found bss
		if (memcmp(pmib->dot11Bss.bssid, null_mac, MACADDRLEN))
		{
			priv->beacon_period = pmib->dot11Bss.beacon_prd;
			if (pmib->dot11Bss.bsstype & WIFI_AP_STATE)
			{
#ifdef WIFI_SIMPLE_CONFIG
			if (priv->pmib->wscEntry.wsc_enable == 1) //wps client mode
				priv->recover_join_req = 1;
#endif
			clean_for_join(priv);

		/*cfg p2p cfg p2p ; remove*/
			OPMODE_VAL(WIFI_STATION_STATE);
		/*cfg p2p cfg p2p ; remove*/

			if (IS_ROOT_INTERFACE(priv))
			{
				RTL_W32(CR, (RTL_R32(CR) & ~(NETYPE_Mask << NETYPE_SHIFT)) | ((NETYPE_INFRA & NETYPE_Mask) << NETYPE_SHIFT));
				updateTSF(priv);
					RTL_W8(BCN_CTRL, RTL_R8(BCN_CTRL) & ~(DIS_TSF_UPDATE_N | DIS_SUB_STATE_N));

			}
#ifdef SDIO_2_PORT
			if (IS_VXD_INTERFACE(priv)) {
				RTL_W32(CR, (RTL_R32(CR) & ~(NETYPE_Mask << NETYPE_SHIFT1)) | ((NETYPE_INFRA & NETYPE_Mask) << NETYPE_SHIFT1));
				updateTSF(priv);
				RTL_W8(BCN_CTRL1, RTL_R8(BCN_CTRL1) & ~(DIS_TSF_UPDATE_N | DIS_SUB_STATE_N));
				RTL_W32(RCR, RTL_R32(RCR) & ~RCR_CBSSID); // acli
			}
#endif
		{
			start_clnt_auth(priv);
		}
			return;
		}
		else if (pmib->dot11Bss.bsstype == WIFI_ADHOC_STATE)
		{
			clean_for_join(priv);
			OPMODE_VAL(WIFI_ADHOC_STATE);
			update_bss(&pmib->dot11StationConfigEntry, &pmib->dot11Bss);
			pmib->dot11RFEntry.dot11channel = pmib->dot11Bss.channel;

			if (pmib->dot11BssType.net_work_type & WIRELESS_11N) {
				if (priv->pmib->dot11nConfigEntry.dot11nUse40M) {
					if (pmib->dot11Bss.t_stamp[1] & BIT(1))
						priv->pshare->is_40m_bw	= 1;
					else
						priv->pshare->is_40m_bw	= 0;

					if (priv->pshare->is_40m_bw) {
						if (pmib->dot11Bss.t_stamp[1] & BIT(2))
							priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_BELOW;
						else
							priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_ABOVE;
					}
					else
						priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_DONTCARE;
				}
				else
					priv->pshare->is_40m_bw	= 0;

				priv->ht_cap_len = 0;
				priv->ht_ie_len = 0;

				priv->pshare->CurrentChannelBW = priv->pshare->is_40m_bw;
				SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);
			}
#ifdef RTK_AC_SUPPORT		//ADHOC-VHT support
			if (pmib->dot11BssType.net_work_type & WIRELESS_11AC)
			{
				if (priv->pmib->dot11nConfigEntry.dot11nUse40M == 2) {
					priv->pshare->is_40m_bw = HT_CHANNEL_WIDTH_80;
					priv->pshare->CurrentChannelBW = priv->pshare->is_40m_bw;
				}
				else if (priv->pmib->dot11nConfigEntry.dot11nUse40M == 1){
					if (pmib->dot11Bss.t_stamp[1] & BIT(1))
						priv->pshare->is_40m_bw = HT_CHANNEL_WIDTH_20_40;

					else
						priv->pshare->is_40m_bw = HT_CHANNEL_WIDTH_20;

					if (priv->pshare->is_40m_bw) {
						if (pmib->dot11Bss.t_stamp[1] & BIT(2))
							priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_BELOW;
						else
							priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_ABOVE;
					}
					else
						priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_DONTCARE;
				}
				else
					priv->pshare->is_40m_bw = 0;

				if (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_5)
					priv->pshare->CurrentChannelBW = HT_CHANNEL_WIDTH_5;
				else if (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_10)
					priv->pshare->CurrentChannelBW = HT_CHANNEL_WIDTH_10;
				else
					priv->pshare->CurrentChannelBW = priv->pshare->is_40m_bw;
				SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);
		}

#endif
			SwChnl(priv, pmib->dot11Bss.channel, priv->pshare->offset_2nd_chan);
			DEBUG_INFO("Join IBSS: chan=%d, 40M=%d, offset=%d\n", pmib->dot11Bss.channel,
				priv->pshare->is_40m_bw, priv->pshare->offset_2nd_chan);

			join_bss(priv);
			RTL_W32(CR, (RTL_R32(CR) & ~(NETYPE_Mask << NETYPE_SHIFT)) | ((NETYPE_ADHOC & NETYPE_Mask) << NETYPE_SHIFT));
			updateTSF(priv);
				RTL_W8(BCN_CTRL, RTL_R8(BCN_CTRL) & ~(DIS_TSF_UPDATE_N));

			JOIN_REQ_ONGOING_VAL(0);
			init_beacon(priv);
			JOIN_RES_VAL(STATE_Sta_Ibss_Active);

			if(IS_VXD_INTERFACE(priv))
				construct_ibss_beacon(priv);

			DEBUG_INFO("Join IBSS - %02X:%02X:%02X:%02X:%02X:%02X\n",
				BSSID[0], BSSID[1], BSSID[2], BSSID[3], BSSID[4], BSSID[5]);
			LOG_MSG("Join IBSS - %02X:%02X:%02X:%02X:%02X:%02X\n",
				BSSID[0], BSSID[1], BSSID[2], BSSID[3], BSSID[4], BSSID[5]);
			return;
		}
		else
			return;
	}

	// not found
	//if (OPMODE & WIFI_STATION_STATE) orig
	if (OPMODE & WIFI_STATION_STATE)
	{
		clean_for_join(priv);
		if (IS_ROOT_INTERFACE(priv))
			RTL_W32(CR, (RTL_R32(CR) & ~(NETYPE_Mask << NETYPE_SHIFT)) | ((NETYPE_NOLINK & NETYPE_Mask) << NETYPE_SHIFT));
#ifdef SDIO_2_PORT
		if (IS_VXD_INTERFACE(priv))
			RTL_W32(CR, (RTL_R32(CR) & ~(NETYPE_Mask << NETYPE_SHIFT1)) | ((NETYPE_NOLINK & NETYPE_Mask) << NETYPE_SHIFT1));
#endif
		JOIN_RES_VAL(STATE_Sta_No_Bss);
		JOIN_REQ_ONGOING_VAL(0);
#ifdef WIFI_WPAS_CLI
		WARN_ON("pmib->dot11Bss.bssid is NULL");
#else
		STADEBUG("start_clnt_lookup(RESCAN)\n");
		start_clnt_lookup(priv, RESCAN);
#endif
		return;
	}
	else if (OPMODE & WIFI_ADHOC_STATE)
	{
		unsigned char tmpbssid[MACADDRLEN];
		int start_period;

		memset(tmpbssid, 0, MACADDRLEN);
		if (!memcmp(BSSID, tmpbssid, MACADDRLEN)) {
			// generate an unique Ibss ssid
			get_random_bytes(&random, 1);
			tmpbssid[0] = 0x02;
			for (i=1; i<MACADDRLEN; i++)
				tmpbssid[i] = GET_MY_HWADDR[i-1] ^ GET_MY_HWADDR[i] ^ random;
			while(1) {
				for (i=0; i<priv->site_survey->count_target; i++) {
					if (!memcmp(tmpbssid, priv->site_survey->bss_target[i].bssid, MACADDRLEN)) {
						tmpbssid[5]++;
						break;
					}
				}
				if (i == priv->site_survey->count)
					break;
			}

			clean_for_join(priv);
			memcpy(BSSID, tmpbssid, MACADDRLEN);
			if (SSID_LEN == 0) {
				SSID_LEN = pmib->dot11StationConfigEntry.dot11DefaultSSIDLen;
				memcpy(SSID, pmib->dot11StationConfigEntry.dot11DefaultSSID, SSID_LEN);
			}

			pmib->dot11Bss.channel = pmib->dot11RFEntry.dot11channel;

			if (pmib->dot11BssType.net_work_type & WIRELESS_11N) {
				priv->pshare->is_40m_bw = priv->pmib->dot11nConfigEntry.dot11nUse40M;
				if (priv->pshare->is_40m_bw)
					priv->pshare->offset_2nd_chan = priv->pmib->dot11nConfigEntry.dot11n2ndChOffset;
				else
					priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_DONTCARE;

				if (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_5)
					priv->pshare->CurrentChannelBW = HT_CHANNEL_WIDTH_5;
				else if (priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_10)
					priv->pshare->CurrentChannelBW = HT_CHANNEL_WIDTH_10;
				else
				priv->pshare->CurrentChannelBW = priv->pshare->is_40m_bw;
				SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);
			}

			SwChnl(priv, pmib->dot11Bss.channel, priv->pshare->offset_2nd_chan);
			DEBUG_INFO("Start IBSS: chan=%d, 40M=%d, offset=%d\n", pmib->dot11Bss.channel,
				priv->pshare->is_40m_bw, priv->pshare->offset_2nd_chan);
			DEBUG_INFO("Start IBSS - %02X:%02X:%02X:%02X:%02X:%02X\n",
				BSSID[0], BSSID[1], BSSID[2], BSSID[3], BSSID[4], BSSID[5]);

			join_bss(priv);

			if(IS_VXD_INTERFACE(priv))
				construct_ibss_beacon(priv);

			RTL_W32(CR, (RTL_R32(CR) & ~(NETYPE_Mask << NETYPE_SHIFT)) | ((NETYPE_ADHOC & NETYPE_Mask) << NETYPE_SHIFT));
			updateTSF(priv);
				RTL_W8(BCN_CTRL, RTL_R8(BCN_CTRL) & ~(DIS_TSF_UPDATE_N));


			priv->beacon_period = pmib->dot11StationConfigEntry.dot11BeaconPeriod;
			JOIN_RES_VAL(STATE_Sta_Ibss_Idle);
			JOIN_REQ_ONGOING_VAL(0);
			if (priv->auto_channel) {
				priv->auto_channel = 1;
				priv->ss_ssidlen = 0;
				DEBUG_INFO("start_clnt_ss, trigger by %s, ss_ssidlen=0\n", (char *)__FUNCTION__);
				RTL_W32(CR, (RTL_R32(CR) & ~(NETYPE_Mask << NETYPE_SHIFT)) | ((NETYPE_NOLINK & NETYPE_Mask) << NETYPE_SHIFT));
				start_clnt_ss(priv);
				return;
			}
			else
				init_beacon(priv);

			JOIN_RES_VAL(STATE_Sta_Ibss_Idle);
		}
		else {
			pmib->dot11Bss.channel = pmib->dot11RFEntry.dot11channel;

			if (pmib->dot11BssType.net_work_type & WIRELESS_11N) {
				priv->pshare->CurrentChannelBW = priv->pshare->is_40m_bw;
				SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);
			}
			SwChnl(priv, pmib->dot11Bss.channel, priv->pshare->offset_2nd_chan);
			RTL_W32(CR, (RTL_R32(CR) & ~(NETYPE_Mask << NETYPE_SHIFT)) | ((NETYPE_ADHOC & NETYPE_Mask) << NETYPE_SHIFT));

			DEBUG_INFO("Start IBSS: chan=%d, 40M=%d, offset=%d\n", pmib->dot11Bss.channel,
				priv->pshare->is_40m_bw, priv->pshare->offset_2nd_chan);
			DEBUG_INFO("Start IBSS - %02X:%02X:%02X:%02X:%02X:%02X\n",
				BSSID[0], BSSID[1], BSSID[2], BSSID[3], BSSID[4], BSSID[5]);
			JOIN_RES_VAL(STATE_Sta_Ibss_Idle);
		}

		// start for more than scanning period, including random backoff
		start_period = UINT32_DIFF(jiffies, priv->jiffies_pre) / HZ + 1;
		get_random_bytes(&random, 1);
		start_period += (random % 5);
		mod_timer(&priv->idle_timer, jiffies + RTL_SECONDS_TO_JIFFIES(start_period));

		LOG_MSG("Start IBSS - %02X:%02X:%02X:%02X:%02X:%02X\n",
			BSSID[0], BSSID[1], BSSID[2], BSSID[3], BSSID[4], BSSID[5]);

		return;
	}
	else
		return;
}


int check_bss_networktype(struct rtl8192cd_priv * priv, struct bss_desc *bss_target)
{
	int result;

	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
		(priv->pmib->dot11StationConfigEntry.legacySTADeny & WIRELESS_11G) &&
		!(bss_target->network & WIRELESS_11N))
		result = FAIL;
	else if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
		(priv->pmib->dot11StationConfigEntry.legacySTADeny & WIRELESS_11B) &&
		(bss_target->network == WIRELESS_11B))
		result = FAIL;
	else if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) &&
		(priv->pmib->dot11StationConfigEntry.legacySTADeny & WIRELESS_11B) &&
		(bss_target->network == WIRELESS_11B))
		result = FAIL;
	else if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC) &&
		(priv->pmib->dot11StationConfigEntry.legacySTADeny & WIRELESS_11N) &&
		!(bss_target->network & WIRELESS_11AC))
		result = FAIL;
	else if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC) &&
		(priv->pmib->dot11StationConfigEntry.legacySTADeny & WIRELESS_11A) &&
		(bss_target->network == WIRELESS_11A))
		result = FAIL;
	else if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
		(priv->pmib->dot11StationConfigEntry.legacySTADeny & WIRELESS_11A) &&
		(bss_target->network == WIRELESS_11A))
		result = FAIL;
	else
		result = SUCCESS;

	if (result == FAIL) {
		DEBUG_ERR("Deny connect to a legacy AP!\n");
	}

	return result;
}



#if 1   //def SMART_REPEATER_MODE
int check_ap_security(struct rtl8192cd_priv *priv, struct bss_desc *bss)

{
#ifdef SUPPORT_MULTI_PROFILE
	if (GET_MIB(priv)->ap_profile.enable_profile &&
			GET_MIB(priv)->ap_profile.profile_num > 0) {

		if ((GET_MIB(priv)->ap_profile.profile[priv->profile_idx].encryption==0) && (bss->capability&BIT(4)))
			return 0;
		else if ((GET_MIB(priv)->ap_profile.profile[priv->profile_idx].encryption == 1) ||
			(GET_MIB(priv)->ap_profile.profile[priv->profile_idx].encryption == 2)) {
			if ((bss->capability&BIT(4))==0)
				return 0;
			else if (bss->t_stamp[0]!=0)
				return 0;
		}
		else if ((GET_MIB(priv)->ap_profile.profile[priv->profile_idx].encryption == 3) ||
			(GET_MIB(priv)->ap_profile.profile[priv->profile_idx].encryption == 4) ||
			(GET_MIB(priv)->ap_profile.profile[priv->profile_idx].encryption == 6)) {
			if ((bss->capability&BIT(4))==0)
				return 0;
			else if (bss->t_stamp[0]==0)
				return 0;
		}

		if (check_bss_networktype(priv, bss))
			return 1;
	}
	else
#endif
	{
		if ((GET_MIB(priv)->dot1180211AuthEntry.dot11PrivacyAlgrthm==_NO_PRIVACY_) && (bss->capability&BIT(4)))
			return 0;
		else if ((GET_MIB(priv)->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_) ||
			(GET_MIB(priv)->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_)) {
			if ((bss->capability&BIT(4))==0)
				return 0;
			else if (bss->t_stamp[0]!=0)
				return 0;
		}
		else if ((GET_MIB(priv)->dot1180211AuthEntry.dot11PrivacyAlgrthm == _TKIP_PRIVACY_) ||
			(GET_MIB(priv)->dot1180211AuthEntry.dot11PrivacyAlgrthm == _CCMP_PRIVACY_)) {
			if ((bss->capability&BIT(4))==0)
				return 0;
			else if (bss->t_stamp[0]==0)
				return 0;
		}

		if (check_bss_networktype(priv, bss))
			return 1;
	}
	return 0;
}
#endif


#ifdef SUPPORT_MULTI_PROFILE
void  switch_profile(struct rtl8192cd_priv *priv, int idx)
{
	struct ap_profile *profile;
	int key_len;

	if (idx > priv->pmib->ap_profile.profile_num) {
		panic_printk("Invalid profile idx (%d), reset to 0.\n", idx);
		idx = 0;
	}

	profile = &priv->pmib->ap_profile.profile[idx];

	SSID2SCAN_LEN = strlen(profile->ssid);
	SSID_LEN = strlen(profile->ssid);
	memcpy(SSID2SCAN, profile->ssid, SSID2SCAN_LEN);
	memcpy(SSID, profile->ssid, SSID_LEN);

	if(OPMODE & WIFI_ADHOC_STATE)
		OPMODE_VAL(WIFI_ADHOC_STATE);
	else
		OPMODE_VAL(WIFI_STATION_STATE);

	priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = _NO_PRIVACY_;
	priv->pmib->dot1180211AuthEntry.dot11EnablePSK= 0;
	priv->pmib->dot1180211AuthEntry.dot11AuthAlgrthm = profile->auth_type;

	if (profile->encryption == 1 || profile->encryption == 2) {
		if (profile->encryption == 1) {
			priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = _WEP_40_PRIVACY_;
			key_len = 5;
		}
		else {
			priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = _WEP_104_PRIVACY_;
			key_len = 13;
		}
		memcpy(&priv->pmib->dot11DefaultKeysTable.keytype[0], profile->wep_key1, key_len);
		memcpy(&priv->pmib->dot11DefaultKeysTable.keytype[1], profile->wep_key2, key_len);
		memcpy(&priv->pmib->dot11DefaultKeysTable.keytype[2], profile->wep_key3, key_len);
		memcpy(&priv->pmib->dot11DefaultKeysTable.keytype[3], profile->wep_key4, key_len);
		priv->pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex = profile->wep_default_key;

		priv->pmib->dot11GroupKeysTable.dot11Privacy = priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm;
		memcpy(&priv->pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKey.skey,
							&priv->pmib->dot11DefaultKeysTable.keytype[0].skey[0], key_len);
		priv->pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKeyLen = key_len;
		priv->pmib->dot11GroupKeysTable.keyid = priv->pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex;
		priv->pmib->dot11GroupKeysTable.keyInCam = 0;
	}
	else if (profile->encryption == 3 || profile->encryption == 4) {
		priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = _CCMP_PRIVACY_;
		if (profile->encryption == 3) {
			priv->pmib->dot1180211AuthEntry.dot11EnablePSK = PSK_WPA;
			priv->pmib->dot1180211AuthEntry.dot11WPACipher = profile->wpa_cipher;
		}
		else {
			priv->pmib->dot1180211AuthEntry.dot11EnablePSK = PSK_WPA2;
			priv->pmib->dot1180211AuthEntry.dot11WPA2Cipher = profile->wpa_cipher;
		}
		strcpy(priv->pmib->dot1180211AuthEntry.dot11PassPhrase, profile->wpa_psk);
	}
	else if (profile->encryption == 6) {
#ifdef SUPPORT_CLIENT_MIXED_SECURITY
        priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = 2;
        priv->pmib->dot1180211AuthEntry.dot11EnablePSK = 3;
        priv->pmib->dot1180211AuthEntry.dot11WPACipher = 10;
        priv->pmib->dot1180211AuthEntry.dot11WPA2Cipher = 10;
#else
        printk("Do not support WPA-MIXED, set to WPA2 by default!");
        priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = _CCMP_PRIVACY_;
        priv->pmib->dot1180211AuthEntry.dot11EnablePSK = PSK_WPA2;
        priv->pmib->dot1180211AuthEntry.dot11WPA2Cipher = 8;
#endif
        strcpy(priv->pmib->dot1180211AuthEntry.dot11PassPhrase, profile->wpa_psk);
	}
	if(priv->pmib->dot1180211AuthEntry.dot11AuthAlgrthm == 1
		&& priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm!=_WEP_40_PRIVACY_
		&& priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm!=_WEP_104_PRIVACY_){//radius
		priv->pmib->dot1180211AuthEntry.dot11AuthAlgrthm = 2;
		priv->pmib->dot1180211AuthEntry.dot11EnablePSK = 0;
		priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm = _IEEE8021X_PSK_;
		priv->pmib->dot118021xAuthEntry.dot118021xAlgrthm= 1;
	}
	else{
		if (priv->pmib->dot1180211AuthEntry.dot11EnablePSK) {
#if (defined(INCLUDE_WPA_PSK) && !defined(WIFI_HAPD) && !defined(RTK_NL80211)) || defined(HAPD_DRV_PSK_WPS)
			psk_init(priv);
#endif
			priv->pmib->dot118021xAuthEntry.dot118021xAlgrthm = 1;
		} else {
			priv->pmib->dot11RsnIE.rsnielen = 0;
			priv->pmib->dot118021xAuthEntry.dot118021xAlgrthm = 0;
		}
	}

	if (should_forbid_Nmode(priv)) {
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
			if (!priv->mask_n_band)
				priv->mask_n_band = (priv->pmib->dot11BssType.net_work_type & (WIRELESS_11N | WIRELESS_11AC));
			priv->pmib->dot11BssType.net_work_type &= ~(WIRELESS_11N | WIRELESS_11AC);
		}
	}
	else {
		if (priv->mask_n_band) {
			priv->pmib->dot11BssType.net_work_type |= priv->mask_n_band;
			priv->mask_n_band = 0;
		}
	}
}
#endif /* SUPPORT_MULTI_PROFILE */


unsigned int get_ava_2ndchoff(struct rtl8192cd_priv *priv, unsigned int channel, unsigned int bandwidth)
{

	int chan_offset = 0;

	if(bandwidth == HT_CHANNEL_WIDTH_20)
		return HT_2NDCH_OFFSET_DONTCARE;

	if(channel >=34){
		if((channel>144) ? ((channel-1)%8) : (channel%8)) {
			chan_offset = HT_2NDCH_OFFSET_ABOVE;
		} else {
			chan_offset = HT_2NDCH_OFFSET_BELOW;
		}
	}
	else
	{
		if(channel < 5)
			chan_offset = HT_2NDCH_OFFSET_ABOVE;
		else if(channel > 9)
			chan_offset = HT_2NDCH_OFFSET_BELOW;
		else
		{
			chan_offset = priv->pmib->dot11nConfigEntry.dot11n2ndChOffset;
			if(chan_offset == HT_2NDCH_OFFSET_DONTCARE)
				chan_offset = HT_2NDCH_OFFSET_BELOW;
		}
	}

	return chan_offset;

}


/**
 *	@brief	STA don't how to do
 *	popen:Maybe process client lookup and auth and assoc by IOCTL trigger
 *
 *	[Important]
 *	Exceed Authentication times, process this function.
 *	@param	rescan	: process rescan.
 */
void start_clnt_lookup(struct rtl8192cd_priv *priv, int rescan)
{
	struct wifi_mib *pmib = GET_MIB(priv);
	unsigned char null_mac[]={0,0,0,0,0,0};
	char tmpbuf[33];
	int i;
    int tmplen;
#ifdef SUPPORT_MULTI_PROFILE
	int j1, j2;
	int found = 0;
#endif


#ifdef WIFI_WPAS_CLI
	printk("cliW: no lookup for wpa supplicant\n");
	return;
#endif



	if(IS_VXD_INTERFACE(priv) && (OPMODE & WIFI_ADHOC_STATE))
		printk("DO RESCAN for VXD ADHOC!! \n");
	else
	if(!IS_ROOT_INTERFACE(priv) && rescan)
    {
		SDEBUG("STA(non-root)don't SS immediately wait next time by timer\n");
        set_vxd_rescan(priv,rescan);  /*when rescan==2 means Roaming or AP's CH/2ndch/BW be changed*/
		return;
	}



    //if (rescan || ((priv->site_survey->count_target > 0) && ((priv->join_index+1) >= priv->site_survey->count_target)))
	if (rescan || ((priv->site_survey->count_target > 0) && ((priv->join_index+1) > priv->site_survey->count_target)))
	{
		JOIN_RES_VAL(STATE_Sta_Roaming_Scan);
		if (OPMODE & WIFI_SITE_MONITOR) // if scanning, scan later
			return;

#if	0	//def SUPPORT_MULTI_PROFILE
		if (priv->pmib->ap_profile.enable_profile && priv->pmib->ap_profile.profile_num > 0) {

			switch_profile(priv, 0);
		}
#endif

		priv->ss_ssidlen = SSID2SCAN_LEN;
		memcpy(priv->ss_ssid, SSID2SCAN, SSID2SCAN_LEN);
		DEBUG_INFO("start_clnt_ss, trigger by %s, ss_ssidlen=%d, rescan=%d\n", (char *)__FUNCTION__, priv->ss_ssidlen, rescan);
		priv->jiffies_pre = jiffies;

#ifdef WIFI_WPAS_CLI
		event_indicate_wpas(priv, NULL, WPAS_DISCON, NULL);
#else
		if((OPMODE & WIFI_ADHOC_STATE) && rescan)
			RTL_W8(TXPAUSE, RTL_R8(TXPAUSE) | STOP_BCN); //when start Ad-hoc ss, disable beacon
		start_clnt_ss(priv);
#endif
		return;
	}

	{
		memset(&pmib->dot11Bss, 0, sizeof(struct bss_desc));
	}


	if (SSID2SCAN_LEN > 0
#ifdef SUPPORT_MULTI_PROFILE
		|| (priv->pmib->ap_profile.enable_profile && priv->pmib->ap_profile.profile_num > 0)
#endif
		)
	{
		for (i=priv->join_index+1; i<priv->site_survey->count_target; i++)
		{
			// check SSID
#ifdef SUPPORT_MULTI_PROFILE

			if (priv->pmib->ap_profile.enable_profile && priv->pmib->ap_profile.profile_num > 0) {
				int idx3,tmpidx;
				int pidx = priv->profile_idx;
				found = 0;
				for(j2=0;j2<priv->pmib->ap_profile.profile_num;j2++)
				{
					j1 = (pidx + j2) % priv->pmib->ap_profile.profile_num;

						if(strlen(priv->pmib->ap_profile.profile[j1].ssid)==0){
							continue;
					}
					tmplen = strlen(priv->pmib->ap_profile.profile[j1].ssid);
					memcpy(SSID2SCAN, priv->pmib->ap_profile.profile[j1].ssid, tmplen);
                    SSID2SCAN[tmplen]='\0';
                    SSID2SCAN_LEN = tmplen;

					//STADEBUG("Profile.ssid=%s, target.ssid=%s,rssi=[%d]\n",priv->pmib->ap_profile.profile[j1].ssid, priv->site_survey->bss_target[i].ssid,priv->site_survey->bss_target[i].rssi);
					if((priv->pmib->miscEntry.stage == 0) ||
						((priv->site_survey->bss_target[i].stage + 1) == priv->pmib->miscEntry.stage))
					if ((priv->site_survey->bss_target[i].ssidlen == SSID2SCAN_LEN) &&
					(!memcmp(SSID2SCAN, priv->site_survey->bss_target[i].ssid, SSID2SCAN_LEN))
					) {
						priv->profile_idx = j1;
                		//STADEBUG("  found[%s];switch_profile(%d)\n",SSID2SCAN ,priv->profile_idx);
						switch_profile(priv, priv->profile_idx);

						if(check_ap_security(priv, &priv->site_survey->bss_target[i])) {
							syncMulticastCipher(priv, &priv->site_survey->bss_target[i]);
							found = 1;
							break;
						}
					}
				}
				tmpidx = priv->profile_idx;
				for(idx3=0;idx3<priv->pmib->ap_profile.profile_num;idx3++){
		    			tmpidx++;
		    			tmpidx%=priv->pmib->ap_profile.profile_num;
		    			if(strlen(priv->pmib->ap_profile.profile[tmpidx].ssid)==0){
		    			}else{
						priv->profile_idx=tmpidx;
						break;
		    			}
				}
				STADEBUG("next to search profile_idx[%d]\n",priv->profile_idx);
			}
			else
			if((priv->pmib->miscEntry.stage == 0) ||
				((priv->site_survey->bss_target[i].stage + 1) == priv->pmib->miscEntry.stage))
			{
				if ((priv->site_survey->bss_target[i].ssidlen == SSID2SCAN_LEN) &&
				(!memcmp(SSID2SCAN, priv->site_survey->bss_target[i].ssid, SSID2SCAN_LEN)))
				{
					found = 1;
				}
				else
					found = 0;
			}
#endif

#ifdef SUPPORT_MULTI_PROFILE
			if (found)
#else
			if((priv->pmib->miscEntry.stage == 0) ||
				((priv->site_survey->bss_target[i].stage + 1) == priv->pmib->miscEntry.stage))
			if ((priv->site_survey->bss_target[i].ssidlen == SSID2SCAN_LEN) &&
					(!memcmp(SSID2SCAN, priv->site_survey->bss_target[i].ssid, SSID2SCAN_LEN)))
#endif
			{
#ifdef SUPPORT_CLIENT_MIXED_SECURITY
                choose_cipher(priv, &priv->site_survey->bss_target[i]);
#endif
				syncMulticastCipher(priv, &priv->site_survey->bss_target[i]);
				// check BSSID
				if (!memcmp(pmib->dot11StationConfigEntry.dot11DesiredBssid, null_mac, MACADDRLEN) ||
					!memcmp(priv->site_survey->bss_target[i].bssid, pmib->dot11StationConfigEntry.dot11DesiredBssid, MACADDRLEN)
					|| (priv->ss_req_ongoing == SSFROM_REPEATER_VXD)
					)
				{
					// check BSS type
					if (((OPMODE & WIFI_STATION_STATE) && (priv->site_survey->bss_target[i].bsstype == WIFI_AP_STATE)) ||
						((OPMODE & WIFI_ADHOC_STATE) && (priv->site_survey->bss_target[i].bsstype == WIFI_ADHOC_STATE))
						|| ((priv->ss_req_ongoing == SSFROM_REPEATER_VXD) && (priv->site_survey->bss_target[i].bsstype == WIFI_AP_STATE))
						)
					{


						//check encryption ; if security setting no match with mine  use next site_survey->bss_target
						if ((priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm==_NO_PRIVACY_) && (priv->site_survey->bss_target[i].capability&BIT(4)))
							continue;
						else if ((priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_) ||
							(priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_)) {
							if ((priv->site_survey->bss_target[i].capability&BIT(4))==0)
								continue;
							else if (priv->site_survey->bss_target[i].t_stamp[0]!=0)
								continue;
						}
						else if ((priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _TKIP_PRIVACY_) ||
							(priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _CCMP_PRIVACY_)) {
							if ((priv->site_survey->bss_target[i].capability&BIT(4))==0)
								continue;
							else if (priv->site_survey->bss_target[i].t_stamp[0]==0)
								continue;
						}
						if ((OPMODE & WIFI_ADHOC_STATE) && (priv->site_survey->bss_target[i].bsstype == WIFI_ADHOC_STATE)){
							if (priv->pmib->dot1180211AuthEntry.dot11EnablePSK){
								if (priv->pmib->dot1180211AuthEntry.dot11EnablePSK==1){
									if (!((priv->site_survey->bss_target[i].t_stamp[0] & (BIT(2)|BIT(4)|BIT(8)|BIT(10))))) {
										continue;
									}
								}
								else if (priv->pmib->dot1180211AuthEntry.dot11EnablePSK==2){
									if (!((priv->site_survey->bss_target[i].t_stamp[0] & (BIT(18)|BIT(20)|BIT(24)|BIT(26))))){
										continue;
									}
								}
							}
						}

			#ifdef CONFIG_IEEE80211W_CLI
						if(!priv->bss_support_pmf
							&& (priv->pmib->dot1180211AuthEntry.dot11IEEE80211W==MGMT_FRAME_PROTECTION_REQUIRED)
							&& (priv->pmib->dot1180211AuthEntry.dot11EnablePSK & PSK_WPA2)){
								PMFDEBUG("(%s)line=%d, AP NOT SUPPORT PMF, but CLI is PMF requested\n", __FUNCTION__, __LINE__);
								PMFDEBUG("(%s)line=%d, dot11IEEE80211W = %d\n", __FUNCTION__, __LINE__,priv->pmib->dot1180211AuthEntry.dot11IEEE80211W);
							continue ;
						}
			#endif
						//check encryption ; if security setting no match with mine  use next site_survey->bss_target
						{
							// check network type
							if (check_bss_networktype(priv, &(priv->site_survey->bss_target[i])))
							{
                                //under multi-repeater case when some STA has connect , the other one don't connect to diff channel AP ; skip this
	                            if(IS_VAP_INTERFACE(priv) && multiRepeater_startlookup_chk(priv,i) ){
                                    STADEBUG("RP1 rdy connected and RP2's target ch/2nd ch not match\n");
                                    continue;
                                }
								memcpy(tmpbuf, SSID2SCAN, SSID2SCAN_LEN);
								tmpbuf[SSID2SCAN_LEN] = '\0';
								DEBUG_INFO("found desired bss [%s], start to join\n", tmpbuf);
								STADEBUG("(1)found desired bss [%s] start to join,\n ch=%d,2ndch=%d , i=%d\n",tmpbuf ,priv->pshare->switch_chan_rp ,priv->pshare->switch_2ndchoff_rp,i );


								memcpy(&pmib->dot11Bss, &(priv->site_survey->bss_target[i]), sizeof(struct bss_desc));
								break;
							}
						}
					}
				}
			}
		}
		priv->join_index = i;
	}
	else
	{
		for (i=priv->join_index+1; i<priv->site_survey->count_target; i++)
		{
			if((priv->pmib->miscEntry.stage == 0) ||
				((priv->site_survey->bss_target[i].stage + 1) == priv->pmib->miscEntry.stage))
			// check BSSID
			if (!memcmp(pmib->dot11StationConfigEntry.dot11DesiredBssid, null_mac, MACADDRLEN) ||
				!memcmp(priv->site_survey->bss_target[i].bssid, pmib->dot11StationConfigEntry.dot11DesiredBssid, MACADDRLEN))
			{
#ifdef SUPPORT_CLIENT_MIXED_SECURITY
				choose_cipher(priv, &priv->site_survey->bss_target[i]);
#endif
				syncMulticastCipher(priv, &priv->site_survey->bss_target[i]);
				// check BSS type
				if (((OPMODE & WIFI_STATION_STATE) && (priv->site_survey->bss_target[i].bsstype == WIFI_AP_STATE)) ||
					((OPMODE & WIFI_ADHOC_STATE) && (priv->site_survey->bss_target[i].bsstype == WIFI_ADHOC_STATE)))
				{
					// check encryption
					if (((priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm) && (priv->site_survey->bss_target[i].capability&BIT(4))) ||
						((priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm==0) && ((priv->site_survey->bss_target[i].capability&BIT(4))==0)))
					{
						if ((OPMODE & WIFI_ADHOC_STATE) && (priv->site_survey->bss_target[i].bsstype == WIFI_ADHOC_STATE)){
							if (priv->pmib->dot1180211AuthEntry.dot11EnablePSK){
								if (priv->pmib->dot1180211AuthEntry.dot11EnablePSK==1){
									if (!((priv->site_survey->bss_target[i].t_stamp[0] & (BIT(2)|BIT(4)|BIT(8)|BIT(10))))) {
										continue;
									}
								}
								else if (priv->pmib->dot1180211AuthEntry.dot11EnablePSK==2){
									if (!((priv->site_survey->bss_target[i].t_stamp[0] & (BIT(18)|BIT(20)|BIT(24)|BIT(26))))){
										continue;
									}
								}
							}
						}
						// check network type
						if (check_bss_networktype(priv, &(priv->site_survey->bss_target[i])))
						{
                            #if 0   //def UNIVERSAL_REPEATER
							// if this is vxd interface, and chan of found AP is
							if ((GET_ROOT_PRIV(priv)->pmib->dot11RFEntry.dot11channel!= priv->site_survey->bss_target[i].channel)){
                                SDEBUG("if this is vxd interface, and chan of found AP is different with root interface AP, skip it\n");
								continue;
                            }
                            #endif
							memcpy(tmpbuf, priv->site_survey->bss_target[i].ssid, priv->site_survey->bss_target[i].ssidlen);
							tmpbuf[priv->site_survey->bss_target[i].ssidlen] = '\0';
							DEBUG_INFO("found desired bss [%s], start to join\n", tmpbuf);
							STADEBUG("(2)found desired bss [%s], start to join\n\n", tmpbuf);

							memcpy(&pmib->dot11Bss, &(priv->site_survey->bss_target[i]), sizeof(struct bss_desc));

							break;
						}
					}
				}
			}
		}
		priv->join_index = i;
	}

#ifdef WIFI_WPAS
	if(priv->wpas_manual_assoc == 0)
#endif

//#ifdef SMART_REPEATER_MODE;20130725 remove
//	if (priv->ss_req_ongoing != 3)
//#endif
		start_clnt_join(priv);
}


void calculate_rx_beacon(struct rtl8192cd_priv *priv)
{
	int window_top;
	unsigned int rx_beacon_delta, expect_num, decision_period, rx_data_delta;
	int pre_index, deltaB;

	if ((((OPMODE & (WIFI_STATION_STATE | WIFI_ASOC_STATE)) == (WIFI_STATION_STATE | WIFI_ASOC_STATE)) ||
		((OPMODE & WIFI_ADHOC_STATE) &&
				((JOIN_RES == STATE_Sta_Ibss_Active) || (JOIN_RES == STATE_Sta_Ibss_Idle)))) &&
		!priv->ss_req_ongoing)
	{
		if (OPMODE & WIFI_ADHOC_STATE)
			decision_period = ROAMING_DECISION_PERIOD_ADHOC;
		else
			decision_period = ROAMING_DECISION_PERIOD_INFRA;

		if (priv->rxBeaconCntArrayIdx > 0)
			pre_index = priv->rxBeaconCntArrayIdx - 1;
		else
			pre_index = decision_period - 1;

		priv->rxBeaconCntArray[priv->rxBeaconCntArrayIdx] = priv->rxBeaconNumInPeriod;
		priv->rxDataCntArray[priv->rxBeaconCntArrayIdx] = priv->rxDataNumInPeriod;
		if (priv->rxBeaconCntArrayWindow < decision_period)
			priv->rxBeaconCntArrayWindow++;
		else
		{
			window_top = priv->rxBeaconCntArrayIdx + 1;
			if (window_top == decision_period)
				window_top = 0;

			rx_beacon_delta = UINT32_DIFF(priv->rxBeaconCntArray[priv->rxBeaconCntArrayIdx],
				priv->rxBeaconCntArray[window_top]);

			rx_data_delta = UINT32_DIFF(priv->rxDataCntArray[priv->rxBeaconCntArrayIdx],
				priv->rxDataCntArray[window_top]);

			expect_num = (decision_period * 1000) / priv->beacon_period;
			priv->rxBeaconPercentage = (rx_beacon_delta * 100) / expect_num;

			if ((OPMODE & (WIFI_STATION_STATE | WIFI_ASOC_STATE)) == (WIFI_STATION_STATE | WIFI_ASOC_STATE)) {
				deltaB = priv->rxBeaconCntArray[priv->rxBeaconCntArrayIdx] - priv->rxBeaconCntArray[pre_index];
				if (deltaB == 0) {
					priv->rxBeaconMissConti++;
					if (priv->rxBeaconMissConti > 2) {
						struct stat_info *pstat = get_stainfo(priv, priv->pmib->dot11StationConfigEntry.dot11Bssid);
						issue_probereq(priv, priv->pmib->dot11StationConfigEntry.dot11DesiredSSID,
							priv->pmib->dot11StationConfigEntry.dot11DesiredSSIDLen, priv->pmib->dot11StationConfigEntry.dot11Bssid);
						if (pstat->WirelessMode & (WIRELESS_MODE_N_24G | WIRELESS_MODE_N_5G | WIRELESS_MODE_AC_24G | WIRELESS_MODE_AC_5G))
							issue_ADDBAreq(priv, pstat, 0);
					}
				}
				else
					priv->rxBeaconMissConti = 0;
			}

			//DEBUG_INFO("Rx beacon percentage=%d%%, delta=%d, cnt=%d\n", priv->rxBeaconPercentage,
			//	rx_beacon_delta, priv->rxBeaconCntArray[priv->rxBeaconCntArrayIdx]);
#ifdef CLIENT_MODE
			if (OPMODE & WIFI_STATION_STATE)
			{
				// when fast-roaming is enabled, trigger roaming while (david+2006-01-25):
				//	- no any beacon frame received in last one sec (under beacon interval is <= 200ms)
				//  - rx beacon is less than FAST_ROAMING_THRESHOLD
				int offset, fast_roaming_triggered=0;
				if (priv->pmib->dot11StationConfigEntry.fastRoaming) {
					if (priv->beacon_period <= 200) {
						if (priv->rxBeaconCntArrayIdx == 0)
							offset = priv->rxBeaconNumInPeriod - priv->rxBeaconCntArray[decision_period];
						else
							offset = priv->rxBeaconNumInPeriod - priv->rxBeaconCntArray[priv->rxBeaconCntArrayIdx-1];
						if (offset == 0)
							fast_roaming_triggered = 1;
					}
					if (!fast_roaming_triggered && priv->rxBeaconPercentage < FAST_ROAMING_THRESHOLD)
						fast_roaming_triggered = 1;
				}
				if ((priv->rxBeaconPercentage < ROAMING_THRESHOLD || fast_roaming_triggered) && !rx_data_delta) {
					DEBUG_INFO("Roaming...\n");
					LOG_MSG("Roaming...\n");
			#ifdef CONFIG_IEEE80211V_CLI
					if(WNM_ENABLE) {
						memset(priv->pmib->dot11StationConfigEntry.dot11DesiredBssid, 0, MACADDRLEN);
						DOT11VDEBUG("Reset dot11DesiredBssid!!\n");//if target AP is disappeared
					}
			#endif
#if defined(CONFIG_RTL8196B_TR) || defined(CONFIG_RTL865X_SC) || defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD) || defined(CONFIG_RTL8196C_KLD) || defined(CONFIG_RTL8196C_EC)
					LOG_MSG_NOTICE("Roaming...;note:\n");
#endif
					OPMODE_VAL(OPMODE & ~(WIFI_AUTH_SUCCESS | WIFI_ASOC_STATE));
					disable_vxd_ap(GET_VXD_PRIV(priv));

					JOIN_RES_VAL(STATE_Sta_No_Bss);


#if defined(WIFI_WPAS_CLI)
					event_indicate_wpas(priv, NULL, WPAS_DISCON, NULL);
#else
					event_indicate_cfg80211(priv, NULL, CFG80211_DISCONNECTED, NULL);

#endif
					#ifdef CONFIG_RTL_WLAN_STATUS
					//panic_printk("%s:%d\n",__FUNCTION__,__LINE__);
					priv->wlan_status_flag=1;
					#endif

				}
			}
			else
			{
				if ((rx_beacon_delta == 0) && (rx_data_delta == 0)) {
					if (JOIN_RES == STATE_Sta_Ibss_Active)
					{
						DEBUG_INFO("Searching IBSS...\n");
						LOG_MSG("Searching IBSS...\n");
						RTL_W32(CR, (RTL_R32(CR) & ~(NETYPE_Mask << NETYPE_SHIFT)) | ((NETYPE_NOLINK & NETYPE_Mask) << NETYPE_SHIFT));
						JOIN_RES_VAL(STATE_Sta_Ibss_Idle);
						start_clnt_lookup(priv, RESCAN);
					}
				}
			}
#endif // CLIENT_MODE
		}

		priv->rxBeaconCntArrayIdx++;
		if (priv->rxBeaconCntArrayIdx == decision_period)
			priv->rxBeaconCntArrayIdx = 0;
	}
}


#ifdef HS2_SUPPORT
void rtl8192cd_disassoc_timer(unsigned long task_priv)
{
	struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)task_priv;
	unsigned char zeromac[6]={0x00,0x00,0x00,0x00,0x00,0x00};
    #if 1 // for debug
    unsigned char* DA=NULL;
    DA = priv->pmib->hs2Entry.sta_mac;
	HS2DEBUG("issue deauth to [%02X%02X%02X:%02X%02X%02X]\n\n",DA[0],DA[1],DA[2],DA[3],DA[4],DA[5]);
    #endif
	if(memcmp(priv->pmib->hs2Entry.sta_mac,zeromac,6))  {
		issue_disassoc(priv,priv->pmib->hs2Entry.sta_mac,_RSON_DISAOC_STA_LEAVING_);
	}

	if (timer_pending(&priv->disassoc_timer))
		del_timer_sync(&priv->disassoc_timer);

	//memset(priv->pmib->hs2Entry.sta_mac, 0, 6);
}

#endif
void rtl8192cd_reauth_timer(unsigned long task_priv)
{
	struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)task_priv;
	unsigned long flags = 0;
	if (!(priv->drv_state & DRV_STATE_OPEN))
		return;

	SAVE_INT_AND_CLI(flags);
	SMP_LOCK(flags);


	REAUTH_COUNT_VAL(REAUTH_COUNT+1);
	if (REAUTH_COUNT > REAUTH_LIMIT)
	{
		DEBUG_WARN("Client Auth time-out!\n");
		RESTORE_INT(flags);
		SMP_UNLOCK(flags);
#ifdef WIFI_WPAS_CLI
		event_indicate_wpas(priv, NULL, WPAS_DISCON, NULL);
#else
		start_clnt_lookup(priv, DONTRESCAN);
#endif
		return;
	}

	if (OPMODE & WIFI_AUTH_SUCCESS)
	{
		RESTORE_INT(flags);
		SMP_UNLOCK(flags);
		return;
	}

	AUTH_SEQ_VAL(1);
	OPMODE_VAL(OPMODE & ~(WIFI_AUTH_STATE1));
	OPMODE_VAL(OPMODE | WIFI_AUTH_NULL);

	DEBUG_INFO("auth timeout, sending auth req again\n");
	issue_auth(priv, NULL, 0);

	mod_timer(&priv->reauth_timer, jiffies + REAUTH_TO);

	RESTORE_INT(flags);
	SMP_UNLOCK(flags);
}


void rtl8192cd_reassoc_timer(unsigned long task_priv)
{
	struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)task_priv;
	unsigned long flags = 0;
	if (!(priv->drv_state & DRV_STATE_OPEN))
		return;

	SAVE_INT_AND_CLI(flags);
	SMP_LOCK(flags);


	REASSOC_COUNT_VAL(REASSOC_COUNT+1);
	if (REASSOC_COUNT > REASSOC_LIMIT)
	{
		DEBUG_WARN("Client Assoc time-out!\n");
		RESTORE_INT(flags);
		SMP_UNLOCK(flags);
#ifdef WIFI_WPAS_CLI
		event_indicate_wpas(priv, NULL, WPAS_DISCON, NULL);
#else
		STADEBUG("Client Assoc time-out!; start_clnt_lookup(DONTRESCAN)\n");
		start_clnt_lookup(priv, DONTRESCAN);
#endif
		return;
	}

	if (OPMODE & WIFI_ASOC_STATE)
	{
		RESTORE_INT(flags);
		SMP_UNLOCK(flags);
		return;
	}

	DEBUG_INFO("assoc timeout, sending assoc req again\n");
	issue_assocreq(priv);

	MOD_REASSOC_TIMER(REASSOC_TO);

	RESTORE_INT(flags);
	SMP_UNLOCK(flags);
}


void rtl8192cd_idle_timer(unsigned long task_priv)
{
	struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)task_priv;

	SMP_LOCK(flags);
	if (!(priv->drv_state & DRV_STATE_OPEN)) {
		SMP_UNLOCK(flags);
		return;
	}

	RTL_W32(CR, (RTL_R32(CR) & ~(NETYPE_Mask << NETYPE_SHIFT)) | ((NETYPE_NOLINK & NETYPE_Mask) << NETYPE_SHIFT));
	LOG_MSG("Searching IBSS...\n");
	start_clnt_lookup(priv, RESCAN);
	SMP_UNLOCK(flags);
}


#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
__NOMIPS16
#endif
unsigned int OnAssocRsp(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned long	flags;
	struct wifi_mib	*pmib;
	struct stat_info *pstat;
	unsigned char	*pframe, *p;
	DOT11_ASSOCIATION_IND	Association_Ind;
	unsigned char	supportRate[32];
	int		supportRateNum;
	UINT16	val;
	int		len;

#ifdef CONFIG_IEEE80211V_CLI
	unsigned char ext_cap[8];
#endif

	if (!(OPMODE & WIFI_STATION_STATE))
		return SUCCESS;

	if (memcmp(GET_MY_HWADDR, pfrinfo->da, MACADDRLEN))
		return SUCCESS;

	if (OPMODE & WIFI_SITE_MONITOR)
		return SUCCESS;

	if (OPMODE & WIFI_ASOC_STATE)
		return SUCCESS;

	pmib = GET_MIB(priv);
	pframe = get_pframe(pfrinfo);
	DEBUG_INFO("got assoc response  (OPMODE %x seq %d)\n", OPMODE, GetSequence(pframe));

	// checking status
	val = cpu_to_le16(*(unsigned short*)((unsigned long)pframe + WLAN_HDR_A3_LEN + 2));

	if (val) {
		DEBUG_ERR("assoc reject, status: %d\n", val);
		goto assoc_rejected;
	}

	AID_VAL(cpu_to_le16(*(unsigned short*)((unsigned long)pframe + WLAN_HDR_A3_LEN + 4)) & 0x3fff);

	pstat = get_stainfo(priv, pfrinfo->sa);
	if (pstat == NULL) {
		pstat = alloc_stainfo(priv, pfrinfo->sa, -1);
		if (pstat == NULL) {
			DEBUG_ERR("Exceed the upper limit of supported clients...\n");
			goto assoc_rejected;
		}
	}
	else {
		cnt_assoc_num(priv, pstat, DECREASE, (char *)__FUNCTION__);
		release_stainfo(priv, pstat);
		init_stainfo(priv, pstat);
	}
	pstat->tpcache_mgt = GetTupleCache(pframe);


            if(IS_HAL_CHIP(priv))
            {
                if(pstat && (REMAP_AID(pstat) < 128))
                {
                    DEBUG_WARN("%s %d OnAssocRsp, set MACID 0 AID = %x \n",__FUNCTION__,__LINE__,REMAP_AID(pstat));
					if(pstat->txpdrop_flag == 1) {
						GET_HAL_INTERFACE(priv)->UpdateHalMSRRPTHandler(priv, pstat, INCREASE);
						pstat->txpdrop_flag = 0;
					}
                    GET_HAL_INTERFACE(priv)->SetMACIDSleepHandler(priv, 0, REMAP_AID(pstat));
					if(priv->pshare->paused_sta_num && pstat->txpause_flag) {
						priv->pshare->paused_sta_num--;
						pstat->txpause_flag =0;
		        	}
                }
                else
                {
                    DEBUG_WARN(" MACID sleep only support 128 STA \n");
                }
            }


	// Realtek proprietary IE
	pstat->is_realtek_sta = FALSE;
	pstat->IOTPeer = HT_IOT_PEER_UNKNOWN;
	p = pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_; len = 0;
	for (;;) {
		p = get_ie(p, _RSN_IE_1_, &len,
		pfrinfo->pktlen - (p - pframe));
		if (p != NULL) {
			if (!memcmp(p+2, Realtek_OUI, 3) && *(p+2+3) == 2) { /*found realtek out and type == 2*/
				pstat->is_realtek_sta = TRUE;
				pstat->IOTPeer = HT_IOT_PEER_REALTEK;

				if(*(p+2+3+2) & RTK_CAP_IE_WLAN_8192SE)
					pstat->IOTPeer = HT_IOT_PEER_REALTEK_92SE;

				if(*(p+2+3+2) & RTK_CAP_IE_WLAN_88C92C)
					pstat->IOTPeer = HT_IOT_PEER_REALTEK_81XX;

				if (*(p+2+3+3) & ( RTK_CAP_IE_8812_BCUT | RTK_CAP_IE_8812_CCUT))
					pstat->IOTPeer = HT_IOT_PEER_REALTEK_8812;
				break;
			}
		}
		else
			break;
		p = p + len + 2;
	}

	// identify if this is Broadcom sta
	p = pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_; len = 0;

	for (;;)
	{
		unsigned char Broadcom_OUI1[]={0x00, 0x05, 0xb5};
		unsigned char Broadcom_OUI2[]={0x00, 0x0a, 0xf7};
		unsigned char Broadcom_OUI3[]={0x00, 0x10, 0x18};

		p = get_ie(p, _RSN_IE_1_, &len,
				pfrinfo->pktlen - (p - pframe));
		if (p != NULL) {
			if (!memcmp(p+2, Broadcom_OUI1, 3) ||
					!memcmp(p+2, Broadcom_OUI2, 3) ||
					!memcmp(p+2, Broadcom_OUI3, 3)) {

				pstat->IOTPeer = HT_IOT_PEER_BROADCOM;

				break;
			}
		}
		else
			break;

		p = p + len + 2;
	}

	// identify if this is ralink sta
	p = pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_; len = 0;

	for (;;)
	{
		unsigned char Ralink_OUI1[]={0x00, 0x0c, 0x43};

		p = get_ie(p, _RSN_IE_1_, &len,
			pfrinfo->pktlen - (p - pframe));
		if (p != NULL) {
			if (!memcmp(p+2, Ralink_OUI1, 3)) {

				pstat->IOTPeer= HT_IOT_PEER_RALINK;

				break;
			}
		}
		else
			break;
		p = p + len + 2;
	}

	if(!pstat->is_realtek_sta && pstat->IOTPeer != HT_IOT_PEER_BROADCOM && pstat->IOTPeer != HT_IOT_PEER_RALINK)

	{
		unsigned int z = 0;
		for (z = 0; z < INTEL_OUI_NUM; z++) {
			if ((pstat->hwaddr[0] == INTEL_OUI[z][0]) &&
				(pstat->hwaddr[1] == INTEL_OUI[z][1]) &&
				(pstat->hwaddr[2] == INTEL_OUI[z][2])) {

				pstat->IOTPeer= HT_IOT_PEER_INTEL;
					pstat->no_rts = 1;
				break;
			}
		}

	}

#ifdef A4_STA
    if(priv->pshare->rf_ft_var.a4_enable == 2) {
        if(0 < parse_a4_ie(priv, pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_,
            pfrinfo->pktlen - (WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_))) {
            pstat->state |= WIFI_A4_STA;
        }
    }
    else if(priv->pshare->rf_ft_var.a4_enable == 1) {
        pstat->state |= WIFI_A4_STA;
    }
#endif

	// get rates
	p = get_ie(pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_, _SUPPORTEDRATES_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _ASOCRSP_IE_OFFSET_);
	if ((p == NULL) || (len > 32)){
		free_stainfo(priv, pstat);
		return FAIL;
	}
	memcpy(supportRate, p+2, len);
	supportRateNum = len;
	p = get_ie(pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_, _EXT_SUPPORTEDRATES_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _ASOCRSP_IE_OFFSET_);
	if ((p !=  NULL) && (len <= 8)) {
		memcpy(supportRate+supportRateNum, p+2, len);
		supportRateNum += len;
	}

	// other capabilities
	memcpy(&val, (pframe + WLAN_HDR_A3_LEN), 2);
	val = le16_to_cpu(val);
	if (val & BIT(5)) {
		// set preamble according to AP
		RTL_W8(RRSR+2, RTL_R8(RRSR+2) | BIT(7));
		pstat->useShortPreamble = 1;
	}
	else {
		// set preamble according to AP
		RTL_W8(RRSR+2, RTL_R8(RRSR+2) & ~BIT(7));
		pstat->useShortPreamble = 0;
	}

	if ((priv->pshare->curr_band == BAND_2G) && (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G))
	{
		if (val & BIT(10)) {
			priv->pmib->dot11ErpInfo.shortSlot = 1;
			set_slot_time(priv, priv->pmib->dot11ErpInfo.shortSlot);
		}
		else {
			priv->pmib->dot11ErpInfo.shortSlot = 0;
			set_slot_time(priv, priv->pmib->dot11ErpInfo.shortSlot);
		}

		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _ERPINFO_IE_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);

		if (p && (*(p+2) & BIT(1)))	// use Protection
			priv->pmib->dot11ErpInfo.protection = 1;
		else
			priv->pmib->dot11ErpInfo.protection = 0;

		if (p && (*(p+2) & BIT(2)))	// use long preamble
			priv->pmib->dot11ErpInfo.longPreambleStaNum = 1;
		else
			priv->pmib->dot11ErpInfo.longPreambleStaNum = 0;
	}

	// set associated and add to association list
	pstat->state |= (WIFI_ASOC_STATE | WIFI_AUTH_SUCCESS);

#ifdef WIFI_WMM  //  WMM STA
	if (QOS_ENABLE) {
		int i;
		p = pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_;
		for (;;) {
			p = get_ie(p, _RSN_IE_1_, &len,
				pfrinfo->pktlen - (p - pframe));
			if (p != NULL) {
				if (!memcmp(p+2, WMM_PARA_IE, 6)) {
					pstat->QosEnabled = 1;
//capture the EDCA para
					p += 10;  // start of EDCA parameters
					for (i = 0; i <4; i++) {
						process_WMM_para_ie(priv, p);  //get the info
						p += 4;
					}
					DEBUG_INFO("BE: ACM %d, AIFSN %d, ECWmin %d, ECWmax %d, TXOP %d\n",
						GET_STA_AC_BE_PARA.ACM, GET_STA_AC_BE_PARA.AIFSN,
						GET_STA_AC_BE_PARA.ECWmin, GET_STA_AC_BE_PARA.ECWmax,
						GET_STA_AC_BE_PARA.TXOPlimit);
					DEBUG_INFO("VO: ACM %d, AIFSN %d, ECWmin %d, ECWmax %d, TXOP %d\n",
						GET_STA_AC_VO_PARA.ACM, GET_STA_AC_VO_PARA.AIFSN,
						GET_STA_AC_VO_PARA.ECWmin, GET_STA_AC_VO_PARA.ECWmax,
						GET_STA_AC_VO_PARA.TXOPlimit);
					DEBUG_INFO("VI: ACM %d, AIFSN %d, ECWmin %d, ECWmax %d, TXOP %d\n",
						GET_STA_AC_VI_PARA.ACM, GET_STA_AC_VI_PARA.AIFSN,
						GET_STA_AC_VI_PARA.ECWmin, GET_STA_AC_VI_PARA.ECWmax,
						GET_STA_AC_VI_PARA.TXOPlimit);
					DEBUG_INFO("BK: ACM %d, AIFSN %d, ECWmin %d, ECWmax %d, TXOP %d\n",
						GET_STA_AC_BK_PARA.ACM, GET_STA_AC_BK_PARA.AIFSN,
						GET_STA_AC_BK_PARA.ECWmin, GET_STA_AC_BK_PARA.ECWmax,
						GET_STA_AC_BK_PARA.TXOPlimit);

					if (IS_ROOT_INTERFACE(priv))
					{
						SAVE_INT_AND_CLI(flags);
						sta_config_EDCA_para(priv);
						RESTORE_INT(flags);
					}
					break;
				}
			}
			else {
				pstat->QosEnabled = 0;
				break;
			}
			p = p + len + 2;
		}
	}
	else {
		pstat->QosEnabled = 0;
	}
#endif




	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) && priv->ht_cap_len) {
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_, _HT_CAP_, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _ASOCRSP_IE_OFFSET_);
		if ((p !=  NULL) && (len <= sizeof(struct ht_cap_elmt))) {
			pstat->ht_cap_len = len;
			memcpy((unsigned char *)&pstat->ht_cap_buf, p+2, len);
		} else {
			pstat->ht_cap_len = 0;
			memset((unsigned char *)&pstat->ht_cap_buf, 0, sizeof(struct ht_cap_elmt));
		}

		p = get_ie(pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_, _HT_IE_, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _ASOCRSP_IE_OFFSET_);
		if ((p !=  NULL) && (len <= sizeof(struct ht_info_elmt))) {
			pstat->ht_ie_len = len;
			memcpy((unsigned char *)&pstat->ht_ie_buf, p+2, len);

			priv->ht_protection = 0;
			if (!priv->pmib->dot11StationConfigEntry.protectionDisabled && pstat->ht_ie_len) {
				unsigned int prot_mode =  (cpu_to_le16(pstat->ht_ie_buf.info1) & 0x03);
				if (prot_mode == _HTIE_OP_MODE1_ || prot_mode == _HTIE_OP_MODE3_)
					priv->ht_protection = 1;
			}
		} else {
			pstat->ht_ie_len = 0;
		}

		if (pstat->ht_cap_len) {
			if (cpu_to_le16(pstat->ht_cap_buf.ht_cap_info) & _HTCAP_AMSDU_LEN_8K_) {
				pstat->is_8k_amsdu = 1;
				pstat->amsdu_level = 7935 - sizeof(struct wlan_hdr);
			} else {
				pstat->is_8k_amsdu = 0;
				pstat->amsdu_level = 3839 - sizeof(struct wlan_hdr);
			}

#ifdef WIFI_11N_2040_COEXIST
			priv->coexist_connection = 0;

			if (priv->pmib->dot11nConfigEntry.dot11nCoexist &&
				(priv->pmib->dot11BssType.net_work_type & WIRELESS_11G)) {
				p = get_ie(pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_, _EXTENDED_CAP_IE_, &len,
					pfrinfo->pktlen - WLAN_HDR_A3_LEN - _ASOCRSP_IE_OFFSET_);

				if (p != NULL) {
					if (*(p+2) & _2040_COEXIST_SUPPORT_)
						priv->coexist_connection = 1;
				}
			}
#endif
		}
	}

#ifdef CONFIG_IEEE80211V_CLI
	if(WNM_ENABLE)  {
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_, _EXTENDED_CAP_IE_, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _ASOCRSP_IE_OFFSET_);
		if (p != NULL) {
			memcpy(ext_cap, p+2, 8);
			if(ext_cap[2] & _WNM_BSS_TRANS_SUPPORT_) {
				pstat->bssTransSupport = TRUE;
				reset_staBssTransStatus(pstat);
			}
		}
	}
#endif

//8812_client add pstat vht ie
#ifdef RTK_AC_SUPPORT
	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC)
	{
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_, EID_VHTCapability, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _ASOCRSP_IE_OFFSET_);

		if ((p !=  NULL) && (len <= sizeof(struct vht_cap_elmt))) {
			pstat->vht_cap_len = len;
			memcpy((unsigned char *)&pstat->vht_cap_buf, p+2, len);
			//printk("receive vht_cap len = %d \n", len);
		}
#if 1//for 2.4G VHT IE
		else
		{
			if(priv->pmib->dot11RFEntry.phyBandSelect==PHY_BAND_2G)
			{
				unsigned char vht_ie_id[] = {0x00, 0x90, 0x4c};
				p = pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_;
				len = 0;

				for (;;)
				{
					//printk("\nOUI limit=%d\n", pfrinfo->pktlen - (p - pframe));
					p = get_ie(p, _RSN_IE_1_, &len, pfrinfo->pktlen - (p - pframe));
					if (p != NULL) {
				#if 0//debug
						int i;
						for(i=0; i<len+2; i++){
							if((i%8)==0)
								panic_printk("\n");
							panic_printk("%02x ", *(p+i));
						}
						panic_printk("\nlen=%d vht_len=%d\n",len, sizeof(struct vht_cap_elmt));
				#endif
						// Bcom VHT IE
						// {0xdd, 0x13} RSN_IE_1-221, length
						// {0x00, 0x90, 0x4c} oui // {0x04 0x08 } unknow
						// {0xbf, 0x0c} element id, length
						if (!memcmp(p+2, vht_ie_id, 3) && (*(p+7) == 0xbf) && ((*(p+8)) <= sizeof(struct vht_cap_elmt))) {
							pstat->vht_cap_len = *(p+8);
							memcpy((unsigned char *)&pstat->vht_cap_buf, p+9, pstat->vht_cap_len);
							//panic_printk("\n get vht ie in OUI!!! len=%d\n\n", pstat->vht_cap_len);
							break;
						}
					}
					else
						break;
					p = p + len + 2;
				}
			}
		}
#endif

		p = get_ie(pframe + WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_, EID_VHTOperation, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _ASOCRSP_IE_OFFSET_);

		if ((p !=  NULL) && (len <= sizeof(struct vht_oper_elmt))) {
			pstat->vht_oper_len = len;
			memcpy((unsigned char *)&pstat->vht_oper_buf, p+2, len);
			//printk("receive vht_oper len = %d \n", len);
		}
	}
#endif

#ifdef WIFI_WMM  //  WMM STA
	if (QOS_ENABLE) {
		if ((pstat->QosEnabled == 0) && pstat->ht_cap_len) {
			DEBUG_INFO("AP supports HT but doesn't support WMM, use default WMM value\n");
			pstat->QosEnabled = 1;
			default_WMM_para(priv);
			if (IS_ROOT_INTERFACE(priv))
			{
				SAVE_INT_AND_CLI(flags);
				sta_config_EDCA_para(priv);
				RESTORE_INT(flags);
			}
		}
	}
#endif

//Client mode IOT issue, Button 2009.07.17
		if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) &&
			(pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm != _NO_PRIVACY_)
			)
		{
			pstat->is_legacy_encrpt = 0;
			if ((pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_)  ||
				(pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_ ))
				pstat->is_legacy_encrpt = 2;
			else if (pmib->dot11RsnIE.rsnielen) {
				if (pmib->dot11RsnIE.rsnie[0] == _RSN_IE_1_) {
					if (is_support_wpa_aes(priv, pmib->dot11RsnIE.rsnie, pmib->dot11RsnIE.rsnielen) != 1)
						pstat->is_legacy_encrpt = 1;
				}
				else {
					if (is_support_wpa2_aes(priv, pmib->dot11RsnIE.rsnie, pmib->dot11RsnIE.rsnielen) != 1)
						pstat->is_legacy_encrpt = 1;
				}
			}
		}

        priv->pshare->AP_BW = -1;
        if (IS_VXD_INTERFACE(priv)) {
                if(GET_ROOT(priv)->pmib->dot11nConfigEntry.dot11nUse40M) {
                        if((pstat->ht_cap_len > 0) && (pstat->ht_ie_len > 0) &&
                        (pstat->ht_ie_buf.info0 & _HTIE_STA_CH_WDTH_) &&
                        (pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_))) {
                                priv->pshare->is_40m_bw = 1;
                        }
                }
        }

        if (pstat->ht_cap_len) {
                if (pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_))
                        pstat->tx_bw = HT_CHANNEL_WIDTH_20_40;
                else
                        pstat->tx_bw = HT_CHANNEL_WIDTH_20;
        }

	get_matched_rate(priv, supportRate, &supportRateNum, 1);
	update_support_rate(pstat, supportRate, supportRateNum);
	assign_tx_rate(priv, pstat, pfrinfo);
	assign_aggre_mthod(priv, pstat);
	assign_aggre_size(priv, pstat);

#if defined(INCLUDE_WPA_PSK) && !defined(WIFI_WPAS_CLI)
	if (IEEE8021X_FUN && priv->pmib->dot1180211AuthEntry.dot11EnablePSK) {
		if (psk_indicate_evt(priv, DOT11_EVENT_ASSOCIATION_IND, GetAddr2Ptr(pframe), NULL, 0) < 0){
			STADEBUG("assoc_rejected\n");
			goto assoc_rejected;
		}
	}
#endif

	SAVE_INT_AND_CLI(flags);

	pstat->expire_to = priv->expire_to;
	asoc_list_add(priv, pstat);
	cnt_assoc_num(priv, pstat, INCREASE, (char *)__FUNCTION__);

	if (!IEEE8021X_FUN &&
			!(priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _TKIP_PRIVACY_ ||
			 priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _CCMP_PRIVACY_)) {
#if defined(CONFIG_RTL8196B_TR) || defined(CONFIG_RTL865X_SC) || defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD) || defined(CONFIG_RTL8196C_KLD) || defined(CONFIG_RTL8196C_EC)
		LOG_MSG_NOTICE("Connected to AP;note:%02x-%02x-%02x-%02x-%02x-%02x;\n",
				*GetAddr2Ptr(pframe), *(GetAddr2Ptr(pframe)+1), *(GetAddr2Ptr(pframe)+2),
				*(GetAddr2Ptr(pframe+3)), *(GetAddr2Ptr(pframe)+4), *(GetAddr2Ptr(pframe)+5));
#else
	LOG_MSG("Associate to AP successfully - %02X:%02X:%02X:%02X:%02X:%02X\n",
		*GetAddr2Ptr(pframe), *(GetAddr2Ptr(pframe)+1), *(GetAddr2Ptr(pframe)+2),
		*(GetAddr2Ptr(pframe+3)), *(GetAddr2Ptr(pframe)+4), *(GetAddr2Ptr(pframe)+5));
#endif
#ifdef RTK_WLAN_EVENT_INDICATE
		rtk_wlan_event_indicate(priv->dev->name, WIFI_CONNECT_SUCCESS, pstat->hwaddr, 0);
#endif
	}

	// now we have successfully join the give bss...
	if (PENDING_REAUTH_TIMER)
		DELETE_REAUTH_TIMER;
	if (PENDING_REASSOC_TIMER)
		DELETE_REASSOC_TIMER;

	// clear cached Dev

	RESTORE_INT(flags);

	OPMODE_VAL(OPMODE | WIFI_ASOC_STATE);
	update_bss(&priv->pmib->dot11StationConfigEntry, &priv->pmib->dot11Bss);
	priv->pmib->dot11RFEntry.dot11channel = priv->pmib->dot11Bss.channel;
	if (IS_ROOT_INTERFACE(priv))
		join_bss(priv);

	JOIN_RES_VAL(STATE_Sta_Bss);
	JOIN_REQ_ONGOING_VAL(0);
#ifndef WITHOUT_ENQUEUE
	if (priv->pmib->dot118021xAuthEntry.dot118021xAlgrthm
#ifdef WIFI_SIMPLE_CONFIG
		&& !(priv->pmib->wscEntry.wsc_enable)
#endif
		)
	{
		memcpy((void *)Association_Ind.MACAddr, (void *)GetAddr2Ptr(pframe), MACADDRLEN);
		Association_Ind.EventId = DOT11_EVENT_ASSOCIATION_IND;
		Association_Ind.IsMoreEvent = 0;
		Association_Ind.RSNIELen = 0;
		DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&Association_Ind,
					sizeof(DOT11_ASSOCIATION_IND));

		//event_indicate_cfg80211(priv, GetAddr2Ptr(pframe), CFG80211_NEW_STA, NULL);
	}
#endif // WITHOUT_ENQUEUE

#ifdef WIFI_SIMPLE_CONFIG
	if (priv->pmib->wscEntry.wsc_enable) {
		DOT11_WSC_ASSOC_IND wsc_Association_Ind;

		memset(&wsc_Association_Ind, 0, sizeof(DOT11_WSC_ASSOC_IND));
		wsc_Association_Ind.EventId = DOT11_EVENT_WSC_ASSOC_REQ_IE_IND;
		memcpy((void *)wsc_Association_Ind.MACAddr, (void *)GetAddr2Ptr(pframe), MACADDRLEN);
#ifdef INCLUDE_WPS
		wps_NonQueue_indicate_evt(priv ,(UINT8 *)&wsc_Association_Ind,
			sizeof(DOT11_WSC_ASSOC_IND));
#else
		DOT11_EnQueue((unsigned long)priv, priv->pevent_queue, (UINT8 *)&wsc_Association_Ind,
			sizeof(DOT11_WSC_ASSOC_IND));

		//event_indicate_cfg80211(priv, GetAddr2Ptr(pframe), CFG80211_NEW_STA, NULL);


#endif
		pstat->state |= WIFI_WPS_JOIN;
	}
#endif

#if 0
	// Get operating bands
	//    |  B |  G | BG  <= AP
	//  B |  B |  x |  B
	//  G |  x |  G |  G
	// BG |  B |  G | BG
	if ((priv->pshare->curr_band == WIRELESS_11A) ||
		(priv->pshare->curr_band == WIRELESS_11B))
		priv->oper_band = priv->pshare->curr_band;
	else {			// curr_band == WIRELESS_11G
		if (!(priv->pmib->dot11BssType.net_work_type & WIRELESS_11B) ||
			!is_CCK_rate(pstat->bssrateset[0] & 0x7f))
			priv->oper_band = WIRELESS_11G;
		else if (is_CCK_rate(pstat->bssrateset[pstat->bssratelen-1] & 0x7f))
			priv->oper_band = WIRELESS_11B;
		else
			priv->oper_band = WIRELESS_11B | WIRELESS_11G;
	}
#endif

	DEBUG_INFO("assoc successful!\n");
	STADEBUG("Assoc successful!\n");


	//if(under_apmode_repeater(priv))
	{
		RTL_W8(TXPAUSE, 0x0);
	}

	if ((OPMODE & WIFI_STATION_STATE)
		&& IS_ROOT_INTERFACE(priv)
	)
		priv->up_flag = 1;

//#ifdef BR_SHORTCUT
#if 0
	clear_shortcut_cache();
#endif


	if (IS_ROOT_INTERFACE(priv))
	{
		if ((pstat->ht_cap_len > 0) && (pstat->ht_ie_len > 0) &&
			(pstat->ht_ie_buf.info0 & _HTIE_STA_CH_WDTH_) &&
			(pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_))) {
			priv->pshare->is_40m_bw = 1;
			if ((pstat->ht_ie_buf.info0 & _HTIE_2NDCH_OFFSET_BL_) == _HTIE_2NDCH_OFFSET_BL_)
				priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_BELOW;
			else
				priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_ABOVE;

			if (priv->pshare->is_40m_bw == 1) {
				if (priv->pshare->offset_2nd_chan == HT_2NDCH_OFFSET_ABOVE) {
					int i, channel = priv->pmib->dot11Bss.channel + 4;
					for (i=0; i<priv->available_chnl_num; i++) {
						if (channel == priv->available_chnl[i])
							break;
					}
					if (i == priv->available_chnl_num) {
						priv->pshare->is_40m_bw = 0;
						priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_DONTCARE;
						DEBUG_INFO("AP is 40M (ch%d-ch%d) but not fit region domain, sw back to 20M\n", priv->pmib->dot11Bss.channel, channel);
					}
				}
			}


#ifdef RTK_AC_SUPPORT
//8812_client , check ap support 80m ??
			if((priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC)) {
				if (pstat->vht_cap_len && (pstat->vht_oper_buf.vht_oper_info[0] == 1)) {
					pstat->tx_bw = HT_CHANNEL_WIDTH_80;
					priv->pshare->is_40m_bw = HT_CHANNEL_WIDTH_80;
				}
			}

			//printk("vht_oper_info[0] = 0x%x\n", pstat->vht_oper_buf.vht_oper_info[0]);
			//printk("vht_cap_len=%d, is_40m_bw=%d\n", pstat->vht_cap_len, priv->pshare->is_40m_bw);
#endif
			priv->pshare->CurrentChannelBW = priv->pshare->is_40m_bw;
			SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);
			SwChnl(priv, priv->pmib->dot11Bss.channel, priv->pshare->offset_2nd_chan);


			DEBUG_INFO("%s: set chan=%d, 40M=%d, offset_2nd_chan=%d\n",
				__FUNCTION__,
				priv->pmib->dot11Bss.channel,
				priv->pshare->is_40m_bw,  priv->pshare->offset_2nd_chan);
		}
		else {
			priv->pshare->is_40m_bw = 0;
			priv->pshare->offset_2nd_chan = HT_2NDCH_OFFSET_DONTCARE;

//_TXPWR_REDEFINE
			if(priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_5)
				priv->pshare->CurrentChannelBW = HT_CHANNEL_WIDTH_5;
			else if(priv->pmib->dot11nConfigEntry.dot11nUse40M == HT_CHANNEL_WIDTH_10)
				priv->pshare->CurrentChannelBW = HT_CHANNEL_WIDTH_10;
			else
				priv->pshare->CurrentChannelBW = priv->pshare->is_40m_bw;
			SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);
			SwChnl(priv, priv->pmib->dot11Bss.channel, priv->pshare->offset_2nd_chan);

		}
	}

//8812_client , check ap support 80m ??
#ifdef RTK_AC_SUPPORT
	if((priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC)) {
		if (pstat->vht_cap_len && (pstat->vht_oper_buf.vht_oper_info[0] == 1)) {
			pstat->tx_bw = HT_CHANNEL_WIDTH_80;
			priv->pshare->is_40m_bw = HT_CHANNEL_WIDTH_80;
		}
	}
#endif
#if 0	//(BEAMFORMING_SUPPORT == 1)

		//panic_printk("%s, %x\n", __FUNCTION__, cpu_to_le32(pstat->vht_cap_buf.vht_cap_info));
			if((priv->pshare->WlanSupportAbility & WLAN_BEAMFORMING_SUPPORT)
				&& (priv->pmib->dot11RFEntry.txbf == 1) && ((pstat->ht_cap_len && (pstat->ht_cap_buf.txbf_cap))
#ifdef RTK_AC_SUPPORT
				|| (pstat->vht_cap_len && (cpu_to_le32(pstat->vht_cap_buf.vht_cap_info) & (BIT(SU_BFEE_S)|BIT(SU_BFER_S))))
//				BeamformingControl(priv, pstat->hwaddr, pstat->aid, 0, pstat->tx_bw);
#endif
			)) {
					Beamforming_Enter(priv,pstat);
	}
#endif

	if (IS_ROOT_INTERFACE(priv)) {
		if (!(priv->pmib->ethBrExtInfo.macclone_enable && !priv->macclone_completed))
		{
			if (
				netif_running(GET_VXD_PRIV(priv)->dev))
				enable_vxd_ap(GET_VXD_PRIV(priv));
		}
	}

#ifndef USE_WEP_DEFAULT_KEY
	set_keymapping_wep(priv, pstat);
#endif

	if (IS_HAL_CHIP(priv)) {
		GET_HAL_INTERFACE(priv)->UpdateHalRAMaskHandler(priv, pstat, 3);
	} else
	{
	}

{
	// get _ASOC_ID_
	  unsigned short *tmp = (unsigned short *) (pframe + WLAN_HDR_A3_LEN + _CAPABILITY_ + _STATUS_CODE_);
	  pstat->assocID = le16_to_cpu(*tmp) & 0x3fff;

}


#if (BEAMFORMING_SUPPORT == 1)
	if((priv->pshare->WlanSupportAbility & WLAN_BEAMFORMING_SUPPORT)
		&& (priv->pmib->dot11RFEntry.txbf == 1) && ((pstat->ht_cap_len && (pstat->ht_cap_buf.txbf_cap))
#ifdef RTK_AC_SUPPORT
		|| (pstat->vht_cap_len && (cpu_to_le32(pstat->vht_cap_buf.vht_cap_info) & (BIT(SU_BFEE_S)|BIT(SU_BFER_S))))
//				BeamformingControl(priv, pstat->hwaddr, pstat->aid, 0, pstat->tx_bw);
#endif
	)) {
			Beamforming_Enter(priv,pstat);
	}
#endif


#ifdef WIFI_WPAS
	//printk("_Eric WPAS_REGISTERED at %s %d\n", __FUNCTION__, __LINE__);
	event_indicate_wpas(priv, GetAddr2Ptr(pframe), WPAS_REGISTERED, NULL);
#endif
#if 1 //wrt_clnt
	{
		unsigned char *assocrsp_ie = pfrinfo->pskb->data + (WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_);
		int assocrsp_ie_len = pfrinfo->pktlen - (WLAN_HDR_A3_LEN + _ASOCRSP_IE_OFFSET_);

		if(assocrsp_ie_len > 0)
		{
			//printk("AssocRsp Len = %d\n", assocrsp_ie_len);
			if(assocrsp_ie_len > MAX_ASSOC_RSP_LEN)
			{
				printk("AssocRsp Len too LONG !!\n");
				memcpy(priv->rtk->clnt_info.assoc_rsp, assocrsp_ie, MAX_ASSOC_RSP_LEN);
				priv->rtk->clnt_info.assoc_rsp_len = MAX_ASSOC_RSP_LEN;
			}
			else
			{
				memcpy(priv->rtk->clnt_info.assoc_rsp, assocrsp_ie, assocrsp_ie_len);
				priv->rtk->clnt_info.assoc_rsp_len = assocrsp_ie_len;
			}

		}
		else
			printk(" !! Error AssocRsp Len = %d\n", assocrsp_ie_len);
	}
#endif
	event_indicate_cfg80211(priv, GetAddr2Ptr(pframe), CFG80211_CONNECT_RESULT, NULL);




#ifdef CLIENT_MODE

	if ((OPMODE & WIFI_STATION_STATE) && pstat->IOTPeer == HT_IOT_PEER_BROADCOM)
	{
		if (priv->pmib->dot11RFEntry.phyBandSelect & PHY_BAND_5G) {
			RTL_W8(0x51a, 0x0f);
		}
	}
#endif


	return SUCCESS;

assoc_rejected:

#ifndef WIFI_WPAS_CLI
	JOIN_RES_VAL(STATE_Sta_No_Bss);
	JOIN_REQ_ONGOING_VAL(0);

	if (PENDING_REASSOC_TIMER)
		DELETE_REASSOC_TIMER;

	STADEBUG("assoc_rejected ; start_clnt_lookup(DONTRESCAN)\n");
	start_clnt_lookup(priv, DONTRESCAN);

	disable_vxd_ap(GET_VXD_PRIV(priv));
#endif // !WIFI_WPAS_CLI

	return FAIL;
}


/**
 *	@brief	STA in Infra-structure mode Beacon process.
 */
unsigned int OnBeaconClnt_Bss(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned char *bssid;
	struct stat_info *pstat;
	unsigned char *p, *pframe;
	int len;
	unsigned short val16;
#if defined(WIFI_11N_2040_COEXIST) || defined(COCHANNEL_RTS)
	unsigned int channel=0;
#endif
#ifdef WIFI_WMM
	unsigned int i, vo_txop=0, vi_txop=0, be_txop=0, bk_txop=0;
	unsigned long flags;
#endif
	int htcap_chwd_cur = 0;

	pframe = get_pframe(pfrinfo);
	bssid = GetAddr3Ptr(pframe);

	memcpy(&val16, (pframe + WLAN_HDR_A3_LEN + 8 + 2), 2);
	val16 = le16_to_cpu(val16);
	if (!(val16 & BIT(0)) || (val16 & BIT(1)))
		return SUCCESS;

#ifdef WIFI_11N_2040_COEXIST
	if ((!IS_BSSID(priv, bssid)) &&
		priv->pmib->dot11nConfigEntry.dot11nCoexist &&
		priv->coexist_connection &&
		(priv->pmib->dot11BssType.net_work_type & (WIRELESS_11N|WIRELESS_11G))) {
		/*
		 *	check if there is any bg AP around
		 */
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_CAP_,
			&len, pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p == NULL) {
			p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _DSSET_IE_, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
			if (p != NULL)
				channel = *(p+2);
			if (channel && (channel <= 14)) {
#if 0
//#if defined(CONFIG_RTL_8812_SUPPORT) || defined(CONFIG_WLAN_HAL)
				if(!priv->bg_ap_timeout) {
					priv->bg_ap_timeout = 180;
					priv->bg_ap_timeout_ch[channel-1] = 180;
					update_RAMask_to_FW(priv, 1);
				}
#endif
				priv->bg_ap_timeout = 180;
				priv->bg_ap_timeout_ch[channel-1] = 180;
				channel = 0;
			}
		} else {
			/*
			 *	check if there is any 40M intolerant field set by other 11n AP
			 */
			struct ht_cap_elmt *ht_cap=(struct ht_cap_elmt *)(p+2);
			if (cpu_to_le16(ht_cap->ht_cap_info) & _HTCAP_40M_INTOLERANT_)
				priv->intolerant_timeout = 180;
		}
	}
#endif

	if (!IS_BSSID(priv, bssid))
	{
#ifdef COCHANNEL_RTS
	    p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _DSSET_IE_, &len,
		    pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	    if (p != NULL)
		    channel = *(p+2);
	    else
		    channel = priv->pmib->dot11RFEntry.dot11channel;
	    if (channel == priv->pmib->dot11RFEntry.dot11channel)
		    priv->cochannel_to = 5;
#endif
		return SUCCESS;
	}

	// this is our AP
	pstat = get_stainfo(priv, bssid);
	if (pstat == NULL) {
		DEBUG_ERR("Can't find our AP\n");
		return FAIL;
	}

	if(priv->pmib->dot11StationConfigEntry.bcastSSID_inherit && IS_VXD_INTERFACE(priv))
		isHiddenAP(pframe,pfrinfo,pstat,priv);

#ifdef RTK_5G_SUPPORT
	if (priv->pmib->dot11RFEntry.phyBandSelect & PHY_BAND_5G) {
		priv->rxBeaconNumInPeriod++;
	} else
#endif
	{
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _DSSET_IE_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p != NULL) {
			if (priv->pmib->dot11Bss.channel == *(p+2)) {
				p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _SSID_IE_, &len,
					pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
				if (!(p && (len > 0) && *(p+2) &&
					memcmp(priv->pmib->dot11Bss.ssid, p+2, priv->pmib->dot11Bss.ssidlen)))
				priv->rxBeaconNumInPeriod++;
			}
		}
	}

	if (priv->ps_state) {
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _TIM_IE_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p != NULL) {
			if (isOurFrameBuffred(p, _AID) == TRUE) {
#if defined(WIFI_WMM) && defined(WMM_APSD)
				if (QOS_ENABLE && APSD_ENABLE && priv->uapsd_assoc) {
					if (!(priv->pmib->dot11QosEntry.UAPSD_AC_BE &&
						priv->pmib->dot11QosEntry.UAPSD_AC_BK &&
						priv->pmib->dot11QosEntry.UAPSD_AC_VI &&
						priv->pmib->dot11QosEntry.UAPSD_AC_VO))
						issue_PsPoll(priv);
				} else
#endif
				{
					issue_PsPoll(priv);
				}
			}

		}
	}

	if (val16 & BIT(5))
		pstat->useShortPreamble = 1;
	else
		pstat->useShortPreamble = 0;

	if ((priv->pshare->curr_band == BAND_2G) && (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G))
	{
		if (val16 & BIT(10)) {
			if (priv->pmib->dot11ErpInfo.shortSlot == 0) {
				priv->pmib->dot11ErpInfo.shortSlot = 1;
				set_slot_time(priv, priv->pmib->dot11ErpInfo.shortSlot);
			}
		}
		else {
			if (priv->pmib->dot11ErpInfo.shortSlot == 1) {
				priv->pmib->dot11ErpInfo.shortSlot = 0;
				set_slot_time(priv, priv->pmib->dot11ErpInfo.shortSlot);
			}
		}

		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _ERPINFO_IE_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);

		if (p && (*(p+2) & BIT(1)))	// use Protection
			priv->pmib->dot11ErpInfo.protection = 1;
		else
			priv->pmib->dot11ErpInfo.protection = 0;

		if (p && (*(p+2) & BIT(2)))	// use long preamble
			priv->pmib->dot11ErpInfo.longPreambleStaNum = 1;
		else
			priv->pmib->dot11ErpInfo.longPreambleStaNum = 0;
	}

	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_CAP_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	if (p !=  NULL) {
		struct ht_cap_elmt *ht_cap = (struct ht_cap_elmt *)(p+2);
		if (OPMODE & WIFI_ASOC_STATE) {
			if ((ht_cap->ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_)))
				htcap_chwd_cur = 1;
		}
	}
	/*
	 *	Update HT Operation IE of AP for protection and coexist infomation
	 */

	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) && pstat->ht_cap_len) {
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_IE_, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p !=  NULL) {
			int htcap_chwd_offset = 0;
			if (OPMODE & WIFI_ASOC_STATE) {
				if ((p[3]& _HTIE_2NDCH_OFFSET_BL_) == _HTIE_2NDCH_OFFSET_BL_)
					htcap_chwd_offset = HT_2NDCH_OFFSET_BELOW;
				else if ((p[3] & _HTIE_2NDCH_OFFSET_BL_) == _HTIE_2NDCH_OFFSET_AB_)
					htcap_chwd_offset = HT_2NDCH_OFFSET_ABOVE;
				else
					htcap_chwd_offset = 0;
				if(htcap_chwd_cur ==0)
					htcap_chwd_offset = 0;

				if (priv->pmib->dot11nConfigEntry.dot11n2ndChOffset != htcap_chwd_offset) {
                    STADEBUG("my dot11n2ndChOffset[%d],AP's offset= [%d]\n\n",priv->pmib->dot11nConfigEntry.dot11n2ndChOffset, htcap_chwd_offset);
					priv->pmib->dot11nConfigEntry.dot11n2ndChOffset = htcap_chwd_offset;
                    #if 0   //def UNIVERSAL_REPEATER
					if (IS_VXD_INTERFACE(priv)) {
						OPMODE_VAL(WIFI_AUTH_SUCCESS | WIFI_ASOC_STATE);
						JOIN_RES_VAL(STATE_Sta_No_Bss);
						DEBUG_INFO("%s: AP has changed 2nd ch offset, reconnect...\n", __FUNCTION__);
						return SUCCESS;
					}
					else
                    #endif
                    #if 0//defined(UNIVERSAL_REPEATER) || defined(MBSSID)
					if (IS_ROOT_INTERFACE(priv))
                    #endif
					{   //either root or non-root goto reconn
						DEBUG_INFO("%s: AP has changed 2nd ch offset, reconnect...\n", __FUNCTION__);
//						goto ReConn;
						priv->pmib->dot11Bss.t_stamp[1] &= ~(BIT(1) | BIT(2));
						if(htcap_chwd_offset == HT_2NDCH_OFFSET_BELOW)
							priv->pmib->dot11Bss.t_stamp[1] |= (BIT(1) | BIT(2));
						else if(htcap_chwd_offset == HT_2NDCH_OFFSET_ABOVE)
							priv->pmib->dot11Bss.t_stamp[1] |= (BIT(1));
						clnt_switch_chan_to_bss(priv);
						update_RAMask_to_FW(priv, 1);
						SetTxPowerLevel(priv, priv->pmib->dot11RFEntry.dot11channel);
					}
				}
			}

			pstat->ht_ie_len = len;
			memcpy((unsigned char *)&pstat->ht_ie_buf, p+2, len);

			priv->ht_protection = 0;
			if (!priv->pmib->dot11StationConfigEntry.protectionDisabled && pstat->ht_ie_len) {
				unsigned int prot_mode =  (cpu_to_le16(pstat->ht_ie_buf.info1) & 0x03);
				if (prot_mode == _HTIE_OP_MODE1_ || prot_mode == _HTIE_OP_MODE3_)
					priv->ht_protection = 1;
			}
		}
#if 0

		//if ((priv->beacon_period > 200) || ((priv->rxBeaconNumInPeriod % 3) == 0)) {
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_CAP_, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p !=  NULL) {
			struct ht_cap_elmt *ht_cap = (struct ht_cap_elmt *)(p+2);
			int htcap_chwd_cur = 0;

			if (OPMODE & WIFI_ASOC_STATE) {
				if ((ht_cap->ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_)))
					htcap_chwd_cur = 1;

				if (priv->pshare->AP_BW < 0)
					priv->pshare->AP_BW = htcap_chwd_cur;
				else {
					if (priv->pshare->AP_BW != htcap_chwd_cur) {
						DEBUG_INFO("%s: AP has changed BW, reconnect...\n", __FUNCTION__);
						goto ReConn;
					}
				}
			}
		} else {
			DEBUG_INFO("%s: AP HT capability missing, reconnect...\n", __FUNCTION__);
			goto ReConn;
		}
		//}
#endif
	}

	/*
	 *	Update TXOP from Beacon every 3 seconds
	 */
#ifdef WIFI_WMM
	if (QOS_ENABLE && pstat->QosEnabled && !(priv->up_time % 3) &&
		IS_ROOT_INTERFACE(priv) &&
		priv->pmib->dot11OperationEntry.wifi_specific) {
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _RSN_IE_1_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);

		if (p != NULL) {
			if (!memcmp(p+2, WMM_PARA_IE, 6)) {
				/* save previous edca value */
				vo_txop = (unsigned short)(GET_STA_AC_VO_PARA.TXOPlimit);
				vi_txop = (unsigned short)(GET_STA_AC_VI_PARA.TXOPlimit);
				be_txop = (unsigned short)(GET_STA_AC_BE_PARA.TXOPlimit);
				bk_txop = (unsigned short)(GET_STA_AC_BK_PARA.TXOPlimit);

				/* capture the EDCA para */
				p += 10;  /* start of EDCA parameters at here*/
				for (i = 0; i <4; i++) {
					process_WMM_para_ie(priv, p);  /* get the info */
					p += 4;
				}

				/* check whether if TXOP is different from previous settings */
				if ((vo_txop != (unsigned short)(GET_STA_AC_VO_PARA.TXOPlimit)) ||
					(vi_txop != (unsigned short)(GET_STA_AC_VI_PARA.TXOPlimit)) ||
					(be_txop != (unsigned short)(GET_STA_AC_BE_PARA.TXOPlimit)) ||
					(bk_txop != (unsigned short)(GET_STA_AC_BK_PARA.TXOPlimit))) {
					SAVE_INT_AND_CLI(flags);
					sta_config_EDCA_para(priv);
					RESTORE_INT(flags);
					DEBUG_INFO("Client mode EDCA updated from beacon\n");
					DEBUG_INFO("BE: ACM %d, AIFSN %d, ECWmin %d, ECWmax %d, TXOP %d\n",
						GET_STA_AC_BE_PARA.ACM, GET_STA_AC_BE_PARA.AIFSN,
						GET_STA_AC_BE_PARA.ECWmin, GET_STA_AC_BE_PARA.ECWmax,
						GET_STA_AC_BE_PARA.TXOPlimit);
					DEBUG_INFO("VO: ACM %d, AIFSN %d, ECWmin %d, ECWmax %d, TXOP %d\n",
						GET_STA_AC_VO_PARA.ACM, GET_STA_AC_VO_PARA.AIFSN,
						GET_STA_AC_VO_PARA.ECWmin, GET_STA_AC_VO_PARA.ECWmax,
						GET_STA_AC_VO_PARA.TXOPlimit);
					DEBUG_INFO("VI: ACM %d, AIFSN %d, ECWmin %d, ECWmax %d, TXOP %d\n",
						GET_STA_AC_VI_PARA.ACM, GET_STA_AC_VI_PARA.AIFSN,
						GET_STA_AC_VI_PARA.ECWmin, GET_STA_AC_VI_PARA.ECWmax,
						GET_STA_AC_VI_PARA.TXOPlimit);
					DEBUG_INFO("BK: ACM %d, AIFSN %d, ECWmin %d, ECWmax %d, TXOP %d\n",
						GET_STA_AC_BK_PARA.ACM, GET_STA_AC_BK_PARA.AIFSN,
						GET_STA_AC_BK_PARA.ECWmin, GET_STA_AC_BK_PARA.ECWmax,
						GET_STA_AC_BK_PARA.TXOPlimit);
				}
			}
		}
	}
#endif

	// Realtek proprietary IE
    pstat->is_realtek_sta = FALSE;
	p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_; len = 0;
	for (;;)
	{
		p = get_ie(p, _RSN_IE_1_, &len,
			pfrinfo->pktlen - (p - pframe));
		if (p != NULL) {
			if (!memcmp(p+2, Realtek_OUI, 3) && *(p+2+3) == 2) { /*found realtek out and type == 2*/
				pstat->is_realtek_sta = TRUE;
				break;
			}
		}
		else
			break;

		p = p + len + 2;
	}

	// Customer proprietary IE
	if (priv->pmib->miscEntry.private_ie_len) {
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, priv->pmib->miscEntry.private_ie[0], &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p) {
			memcpy(pstat->private_ie, p, len + 2);
			pstat->private_ie_len = len + 2;
		}
	}

	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _CSA_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	if (p!=NULL){
		DEBUG_INFO("Associated AP notified to do DFS\n");
		if(IS_ROOT_INTERFACE(priv)) {
			// channel switch mode
			priv->pshare->dfsSwitchChannel = (unsigned int)*(p+3);
			priv->pshare->dfsSwitchChCountDown =(unsigned int)*(p+4);
			DEBUG_INFO("CSA Detected mode=%d, channel=%d, countdown=%d\n",*(p+2), priv->pshare->dfsSwitchChannel, priv->pshare->dfsSwitchChCountDown);
			if (priv->pshare->dfsSwitchChCountDown <= 5) {
				if (timer_pending(&priv->dfs_cntdwn_timer))
					del_timer(&priv->dfs_cntdwn_timer);

				DFS_SwChnl_clnt(priv);
				priv->pshare->dfsSwCh_ongoing = 1;
				mod_timer(&priv->dfs_cntdwn_timer, jiffies + RTL_SECONDS_TO_JIFFIES(2));
			}
		} else {
			if(!GET_ROOT(priv)->pmib->dot11DFSEntry.DFS_detected) {
				GET_ROOT(priv)->pshare->dfsSwitchChannel = (unsigned int)*(p+3);
				GET_ROOT(priv)->pshare->dfsSwitchChCountDown =(unsigned int)*(p+4);
				GET_ROOT(priv)->pmib->dot11DFSEntry.DFS_detected = 1;
				DEBUG_INFO("Asscociated AP detected CSA , channel=%d, countdown=%d\n", GET_ROOT(priv)->pshare->dfsSwitchChannel, GET_ROOT(priv)->pshare->dfsSwitchChCountDown);
		}
	}
}

#ifdef RTK_AC_SUPPORT
	if((priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC)) {
		unsigned char tx_bw_bak = pstat->tx_bw;
		unsigned char nss_bak = pstat->nss;
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, EID_VHTOperatingMode, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if ((p !=  NULL) && (len == 1)) {
			if((p[2] &3) <= priv->pmib->dot11nConfigEntry.dot11nUse40M) {
				pstat->tx_bw = p[2] &3;
			}
			pstat->nss = ((p[2]>>4)&0x7)+1;
			panic_printk("[%s %d]receive opering mode data = %02X , pstat->nss  = %d\n",__FUNCTION__,__LINE__ ,p[2], pstat->nss);
		}
		if(tx_bw_bak != pstat->tx_bw || nss_bak != pstat->nss) {
			if (IS_HAL_CHIP(priv)){
				GET_HAL_INTERFACE(priv)->UpdateHalRAMaskHandler(priv, pstat, 3);
			}
		}
	}
#endif


	return SUCCESS;

#if 0
ReConn:

	OPMODE_VAL(OPMODE & ~(WIFI_AUTH_SUCCESS | WIFI_ASOC_STATE));
#ifdef CONFIG_RTL_WLAN_STATUS
	priv->wlan_status_flag=1;
#endif
	disable_vxd_ap(GET_VXD_PRIV(priv));
	JOIN_RES_VAL(STATE_Sta_No_Bss);
	STADEBUG("start_clnt_lookup(RESCAN_ROAMING)\n");
	start_clnt_lookup(priv, RESCAN_ROAMING);
	return SUCCESS;
#endif
}


/**
 *	@brief	STA in ad hoc mode Beacon process.
 */
int OnBeaconClnt_Ibss(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned char *bssid, *bdsa;
	struct stat_info *pstat;
	unsigned char *p, *pframe, channel;
	int len;
	unsigned char supportRate[32];
	int supportRateNum;
	unsigned short val16;
	unsigned long flags;

	pframe = get_pframe(pfrinfo);

	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _DSSET_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	if(p == NULL)
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	if (p != NULL)
		channel = *(p+2);
	else
		channel = priv->pmib->dot11RFEntry.dot11channel;

	/*
	 * check if OLBC exist
	 */
	if ((priv->pmib->dot11BssType.net_work_type & WIRELESS_11G) &&
		(channel == priv->pmib->dot11RFEntry.dot11channel))
	{
		// look for ERP rate. if no ERP rate existed, thought it is a legacy AP
		unsigned char supportedRates[32];
		int supplen=0, legacy=1, i;

		pframe = get_pframe(pfrinfo);
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _SUPPORTEDRATES_IE_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p) {
			if (len>8)
				len=8;
			memcpy(&supportedRates[supplen], p+2, len);
			supplen += len;
		}

		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _EXT_SUPPORTEDRATES_IE_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p) {
			if (len>8)
				len=8;
			memcpy(&supportedRates[supplen], p+2, len);
			supplen += len;
		}

		for (i=0; i<supplen; i++) {
			if (!is_CCK_rate(supportedRates[i]&0x7f)) {
				legacy = 0;
				break;
			}
		}

		// look for ERP IE and check non ERP present
		if (legacy == 0) {
			p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _ERPINFO_IE_, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
			if (p && (*(p+2) & BIT(0)))
				legacy = 1;
		}

		if (legacy) {
			if (!priv->pmib->dot11StationConfigEntry.olbcDetectDisabled &&
							priv->pmib->dot11ErpInfo.olbcDetected==0) {
				priv->pmib->dot11ErpInfo.olbcDetected = 1;
				check_protection_shortslot(priv);
				DEBUG_INFO("OLBC detected\n");
			}
			if (priv->pmib->dot11ErpInfo.olbcDetected)
				priv->pmib->dot11ErpInfo.olbcExpired = DEFAULT_OLBC_EXPIRE;
		}
	}


// mantis#2523
	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _SSID_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	if ( p && (SSID_LEN == len) && !memcmp(SSID, p+2, len)) {
		memcpy(priv->rx_timestamp, pframe+WLAN_HDR_A3_LEN, 8);
	}

	/*
	 * add into sta table and calculate beacon
	 */
	bssid = GetAddr3Ptr(pframe);
	bdsa = GetAddr2Ptr(pframe);

	if (!IS_BSSID(priv, bssid))
	{
#ifdef COCHANNEL_RTS
	    p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _DSSET_IE_, &len,
		    pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	    if (p != NULL)
		    channel = *(p+2);
	    else
		    channel = priv->pmib->dot11RFEntry.dot11channel;
	    if (channel == priv->pmib->dot11RFEntry.dot11channel)
		    priv->cochannel_to = 5;
#endif
		return SUCCESS;
	}

	memcpy(&val16, (pframe + WLAN_HDR_A3_LEN + 8 + 2), 2);
	val16 = le16_to_cpu(val16);
	if ((val16 & BIT(0)) || !(val16 & BIT(1)))
		return SUCCESS;

	// this is our peers
	pstat = get_stainfo(priv, bdsa);

	if (pstat == NULL) {
		DEBUG_INFO("Add IBSS sta, %02x:%02x:%02x:%02x:%02x:%02x!\n",
			bdsa[0],bdsa[1], bdsa[2],bdsa[3],bdsa[4],bdsa[5]);

		pstat = alloc_stainfo(priv, bdsa, -1);
		if (pstat == NULL)
			return SUCCESS;

		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _SUPPORTEDRATES_IE_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p == NULL) {
			free_stainfo(priv, pstat);
			return SUCCESS;
		}
		memcpy(supportRate, p+2, len);
		supportRateNum = len;
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _EXT_SUPPORTEDRATES_IE_, &len,
			pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
		if (p !=  NULL) {
			memcpy(supportRate+supportRateNum, p+2, len);
			supportRateNum += len;
		}

#ifdef WIFI_WMM
		// check if there is WMM IE
		if (QOS_ENABLE) {
			p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_; len = 0;
			for (;;) {
				p = get_ie(p, _RSN_IE_1_, &len,
					pfrinfo->pktlen - (p - pframe));
				if (p != NULL) {
					if (!memcmp(p+2, WMM_IE, 6)) {
						pstat->QosEnabled = 1;
#ifdef WMM_APSD
						if (APSD_ENABLE)
							pstat->apsd_bitmap = *(p+8) & 0x0f;		// get QSTA APSD bitmap
#endif
						break;
					}
				}
				else {
					pstat->QosEnabled = 0;
#ifdef WMM_APSD
					pstat->apsd_bitmap = 0;
#endif
					break;
				}
				p = p + len + 2;
			}
		}
		else {
			pstat->QosEnabled = 0;
#ifdef WMM_APSD
			pstat->apsd_bitmap = 0;
#endif
		}
#endif

		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N) {
			p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_CAP_, &len,
					pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
			if (p !=  NULL) {
				unsigned char mimo_ps;
				pstat->ht_cap_len = len;
				memcpy((unsigned char *)&pstat->ht_cap_buf, p+2, len);
				// below is the process to check HT MIMO power save
				mimo_ps = ((cpu_to_le16(pstat->ht_cap_buf.ht_cap_info)) >> 2)&0x0003;
				pstat->MIMO_ps = 0;
				if (!mimo_ps)
					pstat->MIMO_ps |= _HT_MIMO_PS_STATIC_;
				else if (mimo_ps == 1)
					pstat->MIMO_ps |= _HT_MIMO_PS_DYNAMIC_;

				check_NAV_prot_len(priv, pstat, 0);

				if (cpu_to_le16(pstat->ht_cap_buf.ht_cap_info) & _HTCAP_AMSDU_LEN_8K_) {
					pstat->is_8k_amsdu = 1;
					pstat->amsdu_level = 7935 - sizeof(struct wlan_hdr);
 				} else {
					pstat->is_8k_amsdu = 0;
					pstat->amsdu_level = 3839 - sizeof(struct wlan_hdr);
				}

				if (pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_))
					pstat->tx_bw = HT_CHANNEL_WIDTH_20_40;
				else
					pstat->tx_bw = HT_CHANNEL_WIDTH_20;
			}
			else {
				pstat->ht_cap_len = 0;
				memset((unsigned char *)&pstat->ht_cap_buf, 0, sizeof(struct ht_cap_elmt));
			}

			p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_IE_, &len,
					pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
			if (p !=  NULL) {
				pstat->ht_ie_len = len;
				memcpy((unsigned char *)&pstat->ht_ie_buf, p+2, len);
			}
			else
				pstat->ht_ie_len = 0;
		}
#ifdef RTK_AC_SUPPORT		//ADHOC-VHT support
		if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC)
		{
			p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, EID_VHTCapability, &len,
					pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
			if ((p !=  NULL) && (len <= sizeof(struct vht_cap_elmt))) {
				pstat->vht_cap_len = len;
				memcpy((unsigned char *)&pstat->vht_cap_buf, p+2, len);
			}
			p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, EID_VHTOperation, &len,
					pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
			if ((p !=  NULL) && (len <= sizeof(struct vht_oper_elmt))) {
				pstat->vht_oper_len = len;
				memcpy((unsigned char *)&pstat->vht_oper_buf, p+2, len);
			}
			if(pstat->vht_cap_len){
				switch(cpu_to_le32(pstat->vht_cap_buf.vht_cap_info) & 0x3) {
					default:
					case 0:
						pstat->is_8k_amsdu = 0;
						pstat->amsdu_level = 3895 - sizeof(struct wlan_hdr);
						break;
					case 1:
						pstat->is_8k_amsdu = 1;
						pstat->amsdu_level = 7991 - sizeof(struct wlan_hdr);
						break;
					case 2:
						pstat->is_8k_amsdu = 1;
						pstat->amsdu_level = 11454 - sizeof(struct wlan_hdr);
						break;
				}
				if ((priv->vht_oper_buf.vht_oper_info[0] == 1) && (pstat->vht_oper_buf.vht_oper_info[0] == 1)) {
					pstat->tx_bw = HT_CHANNEL_WIDTH_80;
					priv->pshare->is_40m_bw = HT_CHANNEL_WIDTH_80;
				}
			}
			p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, EID_VHTOperatingMode, &len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
			if ((p !=  NULL) && (len == 1)) {
					if((p[2] &3) <= priv->pmib->dot11nConfigEntry.dot11nUse40M)
						pstat->tx_bw = p[2] &3;
					pstat->nss = ((p[2]>>4)&0x7)+1;
			}
		}
#endif

		// Realtek proprietary IE
		pstat->is_realtek_sta = FALSE;
		p = pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_; len = 0;
		for (;;)
		{
			p = get_ie(p, _RSN_IE_1_, &len,
								pfrinfo->pktlen - (p - pframe));

#ifdef WIFI_WPAS
			if (p != NULL) {
				if((*(unsigned char *)p == _RSN_IE_1_)&& (len >= 4))
				{
					WPAS_ASSOCIATION_INFO Assoc_Info;

					memset((void *)&Assoc_Info, 0, sizeof(struct _WPAS_ASSOCIATION_INFO));
					Assoc_Info.ReqIELen = p[1]+ 2;
					memcpy(Assoc_Info.ReqIE, p, Assoc_Info.ReqIELen);
					event_indicate_wpas(priv, NULL, WPAS_ASSOC_INFO, (UINT8 *)&Assoc_Info);
				}
			}
#endif

			if (p != NULL) {
				if (!memcmp(p+2, Realtek_OUI, 3) && *(p+2+3) == 2) { /*found realtek out and type == 2*/
					pstat->is_realtek_sta = TRUE;
					break;
				}
			}
			else
				break;

			p = p + len + 2;
		}

		if ((priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _TKIP_PRIVACY_) ||
			(priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _CCMP_PRIVACY_) ||
			(priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_) ||
			(priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_) ) {
			DOT11_SET_KEY Set_Key;
			memcpy(Set_Key.MACAddr, pstat->hwaddr, 6);
			Set_Key.KeyType = DOT11_KeyType_Pairwise;
			if (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_ ||
					priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_) {
				Set_Key.EncType = priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm;
				Set_Key.KeyIndex = priv->pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex;
				DOT11_Process_Set_Key(priv->dev, NULL, &Set_Key,
				priv->pmib->dot11DefaultKeysTable.keytype[Set_Key.KeyIndex].skey);
			}
			else {
				Set_Key.EncType = (unsigned char)priv->pmib->dot11GroupKeysTable.dot11Privacy;
				Set_Key.KeyIndex = priv->pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex;
				DOT11_Process_Set_Key(priv->dev, NULL, &Set_Key,
				priv->pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TMicKey1.skey);
			}
		}

		get_matched_rate(priv, supportRate, &supportRateNum, 0);
		update_support_rate(pstat, supportRate, supportRateNum);

		assign_tx_rate(priv, pstat, pfrinfo);
		assign_aggre_mthod(priv, pstat);
		assign_aggre_size(priv, pstat);

		val16 = cpu_to_le16(*(unsigned short*)((unsigned long)pframe + WLAN_HDR_A3_LEN + 8 + 2));
		if (!(val16 & BIT(5))) // NOT use short preamble
			pstat->useShortPreamble = 0;
		else
			pstat->useShortPreamble = 1;

		pstat->state |= (WIFI_ASOC_STATE | WIFI_AUTH_SUCCESS);

		SAVE_INT_AND_CLI(flags);
		pstat->expire_to = priv->expire_to;
		asoc_list_add(priv, pstat);
		cnt_assoc_num(priv, pstat, INCREASE, (char *)__FUNCTION__);
		check_sta_characteristic(priv, pstat, INCREASE);
		RESTORE_INT(flags);

		event_indicate_cfg80211(priv, NULL, CFG80211_IBSS_JOINED, NULL);

		LOG_MSG("An IBSS client is detected - %02X:%02X:%02X:%02X:%02X:%02X\n",
			*GetAddr2Ptr(pframe), *(GetAddr2Ptr(pframe)+1), *(GetAddr2Ptr(pframe)+2),
			*(GetAddr2Ptr(pframe+3)), *(GetAddr2Ptr(pframe)+4), *(GetAddr2Ptr(pframe)+5));

		if (IS_HAL_CHIP(priv)) {
			GET_HAL_INTERFACE(priv)->UpdateHalRAMaskHandler(priv, pstat, 3);
		} else
		{
		}
	}

	if (timer_pending(&priv->idle_timer))
		del_timer(&priv->idle_timer);

	p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _DSSET_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	if(p == NULL)
		p = get_ie(pframe + WLAN_HDR_A3_LEN + _BEACON_IE_OFFSET_, _HT_IE_, &len,
		pfrinfo->pktlen - WLAN_HDR_A3_LEN - _BEACON_IE_OFFSET_);
	if (p != NULL) {
		if (priv->pmib->dot11Bss.channel	== *(p+2)) {
			pstat->beacon_num++;
			priv->rxBeaconNumInPeriod++;
			JOIN_RES_VAL(STATE_Sta_Ibss_Active);
		}
	}
	return SUCCESS;
}


/**
 *	@brief	STA recived Beacon process
 */
unsigned int OnBeaconClnt(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
    int ret = SUCCESS;

    // Site survey and collect information
    if (OPMODE & WIFI_SITE_MONITOR) {
        {
            collect_bss_info(priv, pfrinfo);
        }
        return SUCCESS;
    }

	// Infra client mode, check beacon info
	if ((OPMODE & (WIFI_STATION_STATE | WIFI_ASOC_STATE)) ==
		(WIFI_STATION_STATE | WIFI_ASOC_STATE))
		ret = OnBeaconClnt_Bss(priv, pfrinfo);
#if defined(WIFI_WMM) && defined(WMM_APSD) && !defined(WIFI_WPAS_CLI)
	else if (QOS_ENABLE && APSD_ENABLE && (OPMODE & WIFI_STATION_STATE) && !(OPMODE & WIFI_ASOC_STATE))
		collect_bss_info(priv, pfrinfo);
#endif

	// Ad-hoc client mode, check peer's beacon
	if ((OPMODE & WIFI_ADHOC_STATE) &&
		((JOIN_RES == STATE_Sta_Ibss_Active) || (JOIN_RES == STATE_Sta_Ibss_Idle)))
		ret = OnBeaconClnt_Ibss(priv, pfrinfo);

	return ret;
}


/**
 *	@brief	STA recived ATIM
 *
 *	STA only.
 */
unsigned int OnATIM(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
	return SUCCESS;
}


unsigned int OnDisassocClnt(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned long link_time=0;
    struct stat_info *pstat=NULL;
	unsigned char *pframe = get_pframe(pfrinfo);
	unsigned char *bssid = GetAddr3Ptr(pframe);
	unsigned short val16;
	if (!(OPMODE & WIFI_STATION_STATE))
		return SUCCESS;

	if (memcmp(GET_MY_HWADDR, pfrinfo->da, MACADDRLEN)
#ifdef CONFIG_IEEE80211W_CLI
		&& !IS_BCAST2(pfrinfo->da) //BIP da: 0xFFFFFFFFFFFF
#endif
		)
		return SUCCESS;

	if (!memcmp(BSSID, bssid, MACADDRLEN)) {
		memcpy(&val16, (pframe + WLAN_HDR_A3_LEN), 2);
        val16 = le16_to_cpu(val16);
		DEBUG_INFO("recv Disassociation, reason: %d\n", val16);
#if defined(CONFIG_AUTH_RESULT)
		if(priv->authRes != 14 && priv->authRes != 2 && (priv->authRes != le16_to_cpu(val16)))
		{
			priv->authRes = le16_to_cpu(val16);
			//printk("set deauth reason to %d\n", authRes);
		}
#endif
        if(ACTIVE_ID == 0) {
		priv->dot114WayStatus = val16;
        }

        pstat = get_stainfo(priv, bssid);
        if (pstat == NULL) {
            link_time = 0;
        }else{
            link_time = pstat->link_time;

			event_indicate_cfg80211(priv, NULL, CFG80211_DISCONNECTED, NULL);
        }

		OPMODE_VAL(OPMODE & ~(WIFI_AUTH_SUCCESS | WIFI_ASOC_STATE));
		JOIN_RES_VAL(STATE_Sta_No_Bss);

#ifdef WIFI_WPAS_CLI
		event_indicate_wpas(priv, NULL, WPAS_DISCON, NULL);
#else // !WIFI_WPAS_CLI
        if(GET_MIB(priv)->ap_profile.enable_profile && GET_MIB(priv)->ap_profile.profile_num > 0)
        {
            if((priv->site_survey->count_target > 0) && ((priv->join_index+1) > priv->site_survey->count_target)){
            	STADEBUG("start_clnt_lookup(RESCAN)\n");
                start_clnt_lookup(priv, RESCAN);
            }else{
            	STADEBUG("start_clnt_lookup(DONTRESCAN)\n");
    		    start_clnt_lookup(priv, DONTRESCAN);
            }
        }else
		if (link_time > priv->expire_to) {	// if link time exceeds timeout, site survey again
			STADEBUG("start_clnt_lookup(RESCAN)\n");
		} else{
			STADEBUG("start_clnt_lookup(DONTRESCAN)\n");
		}
#endif // WIFI_WPAS_CLI
#if defined(CONFIG_RTL8196B_TR) || defined(CONFIG_RTL865X_SC) || defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD) || defined(CONFIG_RTL8196C_KLD) || defined(CONFIG_RTL8196C_EC)
		LOG_MSG_NOTICE("Disassociated by AP;note:%02x-%02x-%02x-%02x-%02x-%02x;\n",
				*GetAddr2Ptr(pframe), *(GetAddr2Ptr(pframe)+1), *(GetAddr2Ptr(pframe)+2),
				*(GetAddr2Ptr(pframe+3)), *(GetAddr2Ptr(pframe)+4), *(GetAddr2Ptr(pframe)+5));
#else
		LOG_MSG("Disassociated by AP - %02X:%02X:%02X:%02X:%02X:%02X\n",
			*GetAddr2Ptr(pframe), *(GetAddr2Ptr(pframe)+1), *(GetAddr2Ptr(pframe)+2),
			*(GetAddr2Ptr(pframe+3)), *(GetAddr2Ptr(pframe)+4), *(GetAddr2Ptr(pframe)+5));
#endif
#ifdef RTK_WLAN_EVENT_INDICATE
		rtk_wlan_event_indicate(priv->dev->name, WIFI_DISCONNECT, bssid, 0);
#endif
		disable_vxd_ap(GET_VXD_PRIV(priv));
#ifdef CONFIG_RTL_WLAN_STATUS
		priv->wlan_status_flag=1;
#endif
	}

	return SUCCESS;
}


/**
 *	@brief	STA recived authentication
 *	AP and STA authentication each other.
 */
unsigned int OnAuthClnt(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned int	privacy, seq, len, status, algthm, offset, go2asoc=0;
	unsigned long	flags;
	struct wifi_mib	*pmib;
	unsigned char	*pframe, *p;

	if (!(OPMODE & WIFI_STATION_STATE))
		return SUCCESS;

	if (memcmp(GET_MY_HWADDR, pfrinfo->da, MACADDRLEN))
		return SUCCESS;

	if (OPMODE & WIFI_SITE_MONITOR)
		return SUCCESS;

#if defined(CLIENT_MODE) && defined(INCLUDE_WPA_PSK)
       if (priv->assoc_reject_on && !memcmp(priv->assoc_reject_mac, pfrinfo->sa, MACADDRLEN))
		return SUCCESS;
#endif

	DEBUG_INFO("got auth response\n");
	pmib = GET_MIB(priv);
	pframe = get_pframe(pfrinfo);

	privacy = priv->pmib->dot1180211AuthEntry.dot11AuthAlgrthm;

	if (GetPrivacy(pframe))
		offset = 4;
	else
		offset = 0;

	algthm 	= cpu_to_le16(*(unsigned short *)((unsigned long)pframe + WLAN_HDR_A3_LEN + offset));
	seq 	= cpu_to_le16(*(unsigned short *)((unsigned long)pframe + WLAN_HDR_A3_LEN + offset + 2));
	status 	= cpu_to_le16(*(unsigned short *)((unsigned long)pframe + WLAN_HDR_A3_LEN + offset + 4));

	if (status != 0)
	{
		DEBUG_ERR("clnt auth fail, status: %d\n", status);
#if defined(CONFIG_AUTH_RESULT)
		priv->authRes = status;
#endif
		goto authclnt_err_end;
	}

	if (seq == 2)
	{
#ifdef WIFI_SIMPLE_CONFIG
		if (pmib->wscEntry.wsc_enable && algthm == 0)
			privacy = 0;
#endif

		if ((privacy == 1) || // legacy shared system
			((privacy == 2) && (AUTH_MODE_TOGGLE) && // auto and use shared-key currently
			 (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_ ||
			  priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_)))
		{
			p = get_ie(pframe + WLAN_HDR_A3_LEN + _AUTH_IE_OFFSET_, _CHLGETXT_IE_, (int *)&len,
				pfrinfo->pktlen - WLAN_HDR_A3_LEN - _AUTH_IE_OFFSET_);

			if (p == NULL) {
				DEBUG_ERR("no challenge text?\n");
				goto authclnt_fail;
			}

			DEBUG_INFO("auth chlgetxt len =%d\n", len);
			memcpy((void *)CHG_TXT, (void *)(p+2), len);
			SAVE_INT_AND_CLI(flags);
			AUTH_SEQ_VAL(3);
			OPMODE_VAL(OPMODE & (~ WIFI_AUTH_NULL));
			OPMODE_VAL(OPMODE | (WIFI_AUTH_STATE1));
			RESTORE_INT(flags);
			issue_auth(priv, NULL, 0);
			return SUCCESS;
		}
		else // open system
			go2asoc = 1;
	}
	else if (seq == 4)
	{
		if (privacy)
			go2asoc = 1;
		else
		{
			// this is illegal
			DEBUG_ERR("no privacy but auth seq=4?\n");
			goto authclnt_fail;
		}
	}
	else
	{
		// this is also illegal
		DEBUG_ERR("clnt auth failed due to illegal seq=%x\n", seq);
		goto authclnt_fail;
	}

	if (go2asoc)
	{
		DEBUG_INFO("auth successful!\n");
		start_clnt_assoc(priv);
		return SUCCESS;
	}

authclnt_fail:

	REAUTH_COUNT_VAL(REAUTH_COUNT+1);
	if (REAUTH_COUNT < REAUTH_LIMIT)
		return FAIL;

authclnt_err_end:

	if ((priv->pmib->dot1180211AuthEntry.dot11AuthAlgrthm == 2) &&
		((priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_40_PRIVACY_) ||
		 (priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm == _WEP_104_PRIVACY_)) &&
		(AUTH_MODE_RETRY == 0)) {
		// auto-auth mode, retry another auth method
		AUTH_MODE_RETRY_VAL(AUTH_MODE_RETRY+1);

		start_clnt_auth(priv);
		return SUCCESS;
	}
	else {
#ifndef WIFI_WPAS_CLI
		JOIN_RES_VAL(STATE_Sta_No_Bss);
		JOIN_REQ_ONGOING_VAL(0);

		if (PENDING_REAUTH_TIMER)
			DELETE_REAUTH_TIMER;

		start_clnt_lookup(priv, DONTRESCAN);
#endif // !WIFI_WPAS_CLI
		return FAIL;
	}
}


/**
 *	@brief	Client/STA De authentication
 *	First DeAuthClnt, Second OnDeAuthClnt
 */
unsigned int OnDeAuthClnt(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
	unsigned long flags;
	unsigned long link_time;
	unsigned char *pframe = get_pframe(pfrinfo);
	unsigned char *bssid = GetAddr3Ptr(pframe);
	struct stat_info *pstat;
	unsigned short val16;

#ifdef CONFIG_IEEE80211W_CLI
	unsigned int	status = _STATS_SUCCESSFUL_;
	unsigned short	frame_type;
#endif

	if (!(OPMODE & WIFI_STATION_STATE))
		return SUCCESS;


	if (memcmp(GET_MY_HWADDR, pfrinfo->da, MACADDRLEN)
#ifdef CONFIG_IEEE80211W_CLI
		&& !IS_BCAST2(pfrinfo->da) //BIP da: 0xFFFFFFFFFFFF
#endif
	)
		return SUCCESS;

	if (memcmp(GetAddr2Ptr(pframe), bssid, MACADDRLEN))
		return SUCCESS;

	if (!memcmp(priv->pmib->dot11Bss.bssid, bssid, MACADDRLEN)) {
		memcpy(&val16, (pframe + WLAN_HDR_A3_LEN), 2);
		DEBUG_INFO("recv Deauthentication, reason: %d\n", le16_to_cpu(val16));
#if defined(CONFIG_AUTH_RESULT)
		if(priv->authRes != 14 && priv->authRes != 2 && (priv->authRes != le16_to_cpu(val16)))
		{
			priv->authRes = le16_to_cpu(val16);
			//printk("set deauth reason to %d\n", authRes);
		}

#endif
			priv->dot114WayStatus = le16_to_cpu(val16);

		pstat = get_stainfo(priv, bssid);
		if (pstat == NULL) { // how come?
// Start scan again ----------------------
//			return FAIL;
			link_time = 0; // get next bss info
			goto do_scan;
//--------------------- david+2007-03-10
		}

		{
		link_time = pstat->link_time;

		event_indicate_cfg80211(priv, NULL, CFG80211_DISCONNECTED, NULL);

		SAVE_INT_AND_CLI(flags);
		if (asoc_list_del(priv, pstat)) {
			cnt_assoc_num(priv, pstat, DECREASE, (char *)__FUNCTION__);

		}
		RESTORE_INT(flags);

		free_stainfo(priv, pstat);
		}
do_scan:
		OPMODE_VAL(OPMODE & ~(WIFI_AUTH_SUCCESS | WIFI_ASOC_STATE));
		JOIN_RES_VAL(STATE_Sta_No_Bss);

// Delete timer --------------------------------------
		if (PENDING_REAUTH_TIMER)
			DELETE_REAUTH_TIMER;

		if (PENDING_REASSOC_TIMER)
			DELETE_REASSOC_TIMER;
//---------------------------------- david+2007-03-10

#ifdef WIFI_WPAS_CLI
		event_indicate_wpas(priv, NULL, WPAS_DISCON, NULL);
#else
		event_indicate_cfg80211(priv, NULL, CFG80211_DISCONNECTED, NULL);
#endif
#if defined(CONFIG_RTL8196B_TR) || defined(CONFIG_RTL865X_SC) || defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD) || defined(CONFIG_RTL8196C_KLD) || defined(CONFIG_RTL8196C_EC)
		LOG_MSG_NOTICE("Deauthenticated by AP;note:%02x-%02x-%02x-%02x-%02x-%02x;\n",
				*GetAddr2Ptr(pframe), *(GetAddr2Ptr(pframe)+1), *(GetAddr2Ptr(pframe)+2),
				*(GetAddr2Ptr(pframe+3)), *(GetAddr2Ptr(pframe)+4), *(GetAddr2Ptr(pframe)+5));
#else
		LOG_MSG("Deauthenticated by AP - %02X:%02X:%02X:%02X:%02X:%02X\n",
			*GetAddr2Ptr(pframe), *(GetAddr2Ptr(pframe)+1), *(GetAddr2Ptr(pframe)+2),
			*(GetAddr2Ptr(pframe+3)), *(GetAddr2Ptr(pframe)+4), *(GetAddr2Ptr(pframe)+5));
#endif
#ifdef RTK_WLAN_EVENT_INDICATE
		rtk_wlan_event_indicate(priv->dev->name, WIFI_DISCONNECT, bssid, 0);
#endif
		disable_vxd_ap(GET_VXD_PRIV(priv));
#ifdef CONFIG_RTL_WLAN_STATUS
	priv->wlan_status_flag=1;
#endif
	}

	return SUCCESS;
}


void issue_PwrMgt_NullData(struct rtl8192cd_priv *priv)
{
	struct wifi_mib *pmib;
	unsigned char *hwaddr;
	DECLARE_TXINSN(txinsn);

	pmib = GET_MIB(priv);
	txinsn.retry = pmib->dot11OperationEntry.dot11ShortRetryLimit;
	hwaddr = pmib->dot11OperationEntry.hwaddr;

	txinsn.q_num = MANAGE_QUE_NUM;
    	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
	txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif
	txinsn.fixed_rate = 1;
	txinsn.fr_type = _PRE_ALLOCHDR_;
	txinsn.phdr = get_wlanhdr_from_poll(priv);
	txinsn.pframe = NULL;

	if (txinsn.phdr == NULL)
		goto send_fail;

	memset((void *)(txinsn.phdr), 0, sizeof (struct	wlan_hdr));

	SetFrameSubType(txinsn.phdr, WIFI_DATA_NULL);
	SetToDs(txinsn.phdr);
	if (priv->ps_state)
		SetPwrMgt(txinsn.phdr);
	else
		ClearPwrMgt(txinsn.phdr);

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), hwaddr, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)),  BSSID, MACADDRLEN);
	txinsn.hdr_len = WLAN_HDR_A3_LEN;

	if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS)
		return;

send_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
}


void issue_PsPoll(struct rtl8192cd_priv *priv)
{
	struct wifi_mib *pmib;
	unsigned char *hwaddr;
	DECLARE_TXINSN(txinsn);

	pmib = GET_MIB(priv);
	txinsn.retry = pmib->dot11OperationEntry.dot11ShortRetryLimit;
	hwaddr = GET_MY_HWADDR;

	txinsn.q_num = MANAGE_QUE_NUM;
    	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
	txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif
	txinsn.fixed_rate = 1;
	txinsn.fr_type = _PRE_ALLOCHDR_;
	txinsn.phdr = get_wlanhdr_from_poll(priv);
	txinsn.pframe = NULL;

	if (txinsn.phdr == NULL)
		goto send_fail;

	memset((void *)(txinsn.phdr), 0, sizeof (struct	wlan_hdr));

	SetFrameSubType(txinsn.phdr, WIFI_PSPOLL);
	SetPwrMgt(txinsn.phdr);
	SetPsPollAid(txinsn.phdr, _AID);

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), hwaddr, MACADDRLEN);
	txinsn.hdr_len = WLAN_HDR_PSPOLL;

	if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS)
		return;

send_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
}

void issue_NullData(struct rtl8192cd_priv *priv, unsigned char *da)
{
#ifdef WIFI_WMM
	unsigned char tempQosControl[2];
#endif
	unsigned char *hwaddr;
	DECLARE_TXINSN(txinsn);

	txinsn.retry = priv->pmib->dot11OperationEntry.dot11ShortRetryLimit;

	hwaddr = GET_MY_HWADDR;

	txinsn.q_num = MANAGE_QUE_NUM;
	txinsn.tx_rate = find_rate(priv, NULL, 0, 1);
#ifndef TX_LOWESTRATE
	txinsn.lowest_tx_rate = txinsn.tx_rate;
#endif
	txinsn.fixed_rate = 1;
	txinsn.phdr = get_wlanhdr_from_poll(priv);
	txinsn.pframe = NULL;

	if (txinsn.phdr == NULL)
		goto send_qos_null_fail;

	memset((void *)(txinsn.phdr), 0, sizeof (struct	wlan_hdr));

	SetToDs(txinsn.phdr);
#ifdef WIFI_WMM
	if ((QOS_ENABLE) && (priv->pmib->dot11Bss.t_stamp[1] & BIT(0)))
	{
		SetFrameSubType(txinsn.phdr, BIT(7) | WIFI_DATA_NULL);
		txinsn.hdr_len = WLAN_HDR_A3_QOS_LEN;
		memset(tempQosControl, 0, 2);
		tempQosControl[0] = 0x07;		//set priority to VO
		tempQosControl[0] |= BIT(4);	//set EOSP
		memcpy((void *)GetQosControl((txinsn.phdr)), tempQosControl, 2);

	}
	else // WIFI_DATA_NULL
#endif
	{
		SetFrameSubType(txinsn.phdr, WIFI_DATA_NULL);
		txinsn.hdr_len = WLAN_HDR_A3_LEN;
	}

	memcpy((void *)GetAddr1Ptr((txinsn.phdr)), BSSID, MACADDRLEN);
	memcpy((void *)GetAddr2Ptr((txinsn.phdr)), hwaddr, MACADDRLEN);
	memcpy((void *)GetAddr3Ptr((txinsn.phdr)), da, MACADDRLEN);

	if ((rtl8192cd_firetx(priv, &txinsn)) == SUCCESS)
		return;

send_qos_null_fail:

	if (txinsn.phdr)
		release_wlanhdr_to_poll(priv, txinsn.phdr);
}


unsigned int isOurFrameBuffred(unsigned char* tim, unsigned int aid)
{
	unsigned int numSta;

	numSta = (*(tim + 4) & 0xFE) * 8;
	if (!((aid < numSta) || (aid >= (numSta + (*(tim + 1)-3)*8)))) {
		unsigned int offset;
		unsigned int offset_byte;
		unsigned int offset_bit;
		unsigned char *PartialBitmap = tim + 5;
		unsigned int result;

		offset = aid - numSta;
		offset_byte = offset / 8;
		offset_bit  = offset % 8;
		result = PartialBitmap[offset_byte] & (1 << offset_bit);

		return (result) ? TRUE : FALSE;
	}

	return FALSE;
}

void start_repeater_ss(struct rtl8192cd_priv *priv)
{
	unsigned long flags = 0;
	int xtimeout = 0; //ecos have a "timeout" function. Rename the variable.
    int channel_tmp1;
    int i=0;
    if(!netif_running(priv->dev)){
        return;
    }

    if(IS_VXD_INTERFACE(priv) && !(priv->drv_state & DRV_STATE_VXD_INIT) )
	  {
        return;
    }

	//STADEBUG("count_target=%d;join_index=%d ;ongoing=%d\n",priv->site_survey->count_target,priv->join_index,priv->ss_req_ongoing);

	SAVE_INT_AND_CLI(flags);
	SMP_LOCK(flags);

	if (  !(GET_MIB(priv)->dot11OperationEntry.opmode & WIFI_ASOC_STATE)) {
		if (!netif_running(priv->dev) || priv->ss_req_ongoing) {
			xtimeout = RTL_SECONDS_TO_JIFFIES(1);
			goto out;
		}
		if (!netif_running(GET_ROOT(priv)->dev) || GET_ROOT(priv)->ss_req_ongoing) {

			xtimeout = RTL_SECONDS_TO_JIFFIES(1);
			goto out;
		}
		if (GET_MIB(priv)->wscEntry.wsc_enable) {
			goto out;
		}

		if(timer_pending(&GET_ROOT(priv)->ch_avail_chk_timer)){
			xtimeout = RTL_SECONDS_TO_JIFFIES(3);
			goto out;
		}
#ifdef SUPPORT_MULTI_PROFILE
		/*change because check_vxd_ap_timer hook on vxd now ; before it hook on root interface jw*/
		if (GET_MIB(priv)->ap_profile.enable_profile && GET_MIB(priv)->ap_profile.profile_num > 0) {
			priv->ss_ssidlen = strlen(GET_MIB(priv)->ap_profile.profile[priv->profile_idx].ssid);
			memcpy(priv->ss_ssid, GET_MIB(priv)->ap_profile.profile[priv->profile_idx].ssid, priv->ss_ssidlen);
		}
		else
#endif
		{
			priv->ss_ssidlen = GET_MIB(priv)->dot11StationConfigEntry.dot11SSIDtoScanLen;
			memcpy(priv->ss_ssid, GET_MIB(priv)->dot11StationConfigEntry.dot11SSIDtoScan, priv->ss_ssidlen);
		}


		priv->pshare->switch_chan_rp = 0;
        //STADEBUG("  Call start_clnt_ss\n\n");

        /*if the other STA is connected just scan the same ch with it*/
        if(IS_VAP_INTERFACE(priv))
        {
            /*multiRepeater_connection_status return if other STA connected(return it's channel) or not*/
            channel_tmp1=multiRepeater_connection_status( priv );
            if(channel_tmp1==0){
                //STADEBUG("\n");
                /*restore available_chnl_num,available_chnl*/
                if(priv->MultiSTA_available_backup==1){
                    //STADEBUG("\n");
                    for(i=0;i<priv->MultiSTA_available_chnl_num;i++)
                      priv->available_chnl[i] =  priv->MultiSTA_available_chnl[i];

                    priv->available_chnl_num =  priv->MultiSTA_available_chnl_num;

                    priv->MultiSTA_available_backup = 0;
                }

            }else{
                //STADEBUG("channel_tmp1=[%d]\n",channel_tmp1);

                /*backup available_chnl_num,available_chnl*/
                if(priv->MultiSTA_available_backup==0){
                    STADEBUG("\n");
                    for(i=0;i<priv->available_chnl_num;i++)
                      priv->MultiSTA_available_chnl[i] =  priv->available_chnl[i];

                      priv->MultiSTA_available_chnl_num=priv->available_chnl_num;
                      priv->MultiSTA_available_backup=1;
                }

                /*only scan the channel the same with the other STA*/
                priv->available_chnl[0]=channel_tmp1;
                priv->available_chnl_num=1;
            }
        }
        /*if the other STA is connected just scan the same ch with it*/

 #ifdef SUPPORT_MULTI_PROFILE
	/* 1) AP disappear,2)AP's ch/2nd ch offset/band width be changed , under these cases do RESCAN instead of DONTRESCAN*/
	if(priv->rescantype==RESCAN_ROAMING){
		priv->ss_req_ongoing = SSFROM_REPEATER_VXD;     // 3 for vxd SiteSurvey case
		priv->rescantype=0;
		start_clnt_ss(priv);
	}else
	if((priv->site_survey->count_target == 0) ||
		((priv->site_survey->count_target > 0) && ((priv->join_index+1) > priv->site_survey->count_target))){

    		priv->ss_req_ongoing = SSFROM_REPEATER_VXD;     // 3 for vxd SiteSurvey case
            STADEBUG("  start_clnt_ss()\n");
    		start_clnt_ss(priv);
        }else{
            STADEBUG("  start_clnt_lookup(DONTRESCAN)\n");
            start_clnt_lookup(priv, DONTRESCAN);
        }
#else
        priv->ss_req_ongoing = SSFROM_REPEATER_VXD;     // 3 for vxd SiteSurvey case
		start_clnt_ss(priv);
#endif
		xtimeout = 0;
	}
#if 0 // Dont run ss_timer when vxd is associated.
    else{

        /*now vxd STA under WIFI_ASOC_STATE mode ; just hook a timer for check myself status*/
		xtimeout = CHECK_VXD_AP_TIMEOUT;
    }
#endif
out:
	if (xtimeout){
        //STADEBUG("timeout=%d\n",timeout);
		mod_timer(&priv->ss_timer, jiffies + xtimeout);
     }
	RESTORE_INT(flags);
	SMP_UNLOCK(flags);
	if (IS_VXD_INTERFACE(priv) && priv->pmib->wscEntry.wsc_enable)
	{
		GET_ROOT(priv)->pmib->miscEntry.func_off = 0;
	}
}
/*
 * reschedule STA( not root interface) to do Scan
 * SMP_LOCK before call set_vxd_rescan()
 */
void set_vxd_rescan(struct rtl8192cd_priv *priv,int rescantype)
{
    /*rescantype://
            RESCAN_BY_NEXTTIME by get_ss_level
            RESCAN_ROAMING:
            because            1) AP disappear,2)AP's ch/2nd ch offset/band width be changed , under these cases do RESCAN
        */
	unsigned long flags = 0;
	if(IS_ROOT_INTERFACE(priv)){
		return;
	}

	if( OPMODE & WIFI_ASOC_STATE){
		return;
	}
	#if defined(DEBUG_NL80211)
	NLMSG("%s %d[%s]Ignore invocation of set_vxd_rescan\n",__func__,__LINE__,priv->dev->name);
	#endif
	return;

	SAVE_INT_AND_CLI(flags);
	//SMP_LOCK(flags);
	if (timer_pending(&priv->ss_timer)) {
		SMP_UNLOCK(flags);
		del_timer_sync(&priv->ss_timer);
		SMP_LOCK(flags);
	}

    if(rescantype==RESCAN_ROAMING){
        STADEBUG("1sec\n");
        mod_timer(&priv->ss_timer, jiffies + RTL_SECONDS_TO_JIFFIES(1));
    }else{
          /*    SS_LV_WSTA = 0,                has STA connect to root AP/VAP
                          SS_LV_WOSTA = 1,              no STA connect to root AP/VAP
                          SS_LV_ROOTFUNCOFF = 2,  only root AP and it's func_off=1    */
       	 switch(get_ss_level(priv)) {
	    	case SS_LV_WSTA:
                 /*SS_LV_WSTA = 0,                has STA connect to root AP/VAP*/
                STADEBUG("60secs\n");
#if defined(CONFIG_RTK_BTCONFIG)
				if(priv->btconfig_timeout > 0)
				{
					priv->btconfig_timeout --;
					mod_timer(&priv->ss_timer, jiffies + RTL_SECONDS_TO_JIFFIES(1));
				}
				else if(priv->up_time < 120)
				{
	    			mod_timer(&priv->ss_timer, jiffies + RTL_SECONDS_TO_JIFFIES(5));
				}
				else
				{
	    			mod_timer(&priv->ss_timer, jiffies + CHECK_VXD_AP_TIMEOUT);
				}
#else
	    		mod_timer(&priv->ss_timer, jiffies + CHECK_VXD_AP_TIMEOUT);
#endif
		    	break;
    		case SS_LV_ROOTFUNCOFF:
		        STADEBUG("1sec\n");
		    	mod_timer(&priv->ss_timer, jiffies + RTL_SECONDS_TO_JIFFIES(1));
			    break;
    		default:
				// No STA connected to AP,and AP hasn't func_off,wait a while let DUT's AP can be connected.
#if defined(CONFIG_RTK_BTCONFIG)
				if(priv->btconfig_timeout > 0)
				{
					priv->btconfig_timeout --;
					mod_timer(&priv->ss_timer, jiffies + RTL_SECONDS_TO_JIFFIES(1));
				}
				else
				{
					mod_timer(&priv->ss_timer, jiffies + CHECK_VXD_RUN_DELAY);
				}
#else
				mod_timer(&priv->ss_timer, jiffies + CHECK_VXD_RUN_DELAY);
#endif
         }
    }

	RESTORE_INT(flags);
	//SMP_UNLOCK(flags);
}

#if 0
void sync_channel_2ndch_bw(struct rtl8192cd_priv *priv)
{
    STADEBUG("\n");
    if(priv->pmib->dot11RFEntry.dot11channel != priv->pshare->switch_chan_rp){
        STADEBUG("ch;My[%d],RAP[%d]\n",priv->pmib->dot11RFEntry.dot11channel , priv->pshare->switch_chan_rp);
        priv->pmib->dot11RFEntry.dot11channel = priv->pshare->switch_chan_rp;
    }
    if(priv->pmib->dot11nConfigEntry.dot11n2ndChOffset != priv->pshare->switch_2ndchoff_rp){
        STADEBUG("2nd ch;My[%d],RAP[%d]\n",priv->pmib->dot11nConfigEntry.dot11n2ndChOffset , priv->pshare->switch_2ndchoff_rp);
        priv->pmib->dot11nConfigEntry.dot11n2ndChOffset = priv->pshare->switch_2ndchoff_rp;
    }
    if(priv->pmib->dot11nConfigEntry.dot11nUse40M != priv->pshare->band_width_rp){
        STADEBUG("\n");
        priv->pmib->dot11nConfigEntry.dot11nUse40M = priv->pshare->band_width_rp;
    }
}
#endif /* SMART_REPEATER_MODE */

#endif // CLIENT_MODE

// A dedicated function to check link status
int chklink_wkstaQ(struct rtl8192cd_priv *priv)
{
	int link_status=0;
	if (OPMODE & WIFI_AP_STATE)
	{
		if (priv->assoc_num > 0)
			link_status = 1;
		else
			link_status = 0;
	}
#ifdef CLIENT_MODE
	else if (OPMODE & WIFI_STATION_STATE)
	{
		if (OPMODE & WIFI_ASOC_STATE) {
			link_status = 1;
		}
		else {
			link_status = 0;
		}
	}
	else if ((OPMODE & WIFI_ADHOC_STATE) &&
		((JOIN_RES == STATE_Sta_Ibss_Active) || (JOIN_RES == STATE_Sta_Ibss_Idle)))
	{
		if (priv->rxBeaconCntArrayWindow < ROAMING_DECISION_PERIOD_ADHOC) {
			if (priv->rxBeaconCntArrayWindow) {
				if (priv->rxBeaconCntArray[priv->rxBeaconCntArrayIdx-1] > 0) {
					link_status = 1;
				}
			}
		}
		else {
			if (priv->rxBeaconPercentage)
				link_status = 1;
			else
				link_status = 0;
		}
	}
#endif
	else
	{
		link_status = 0;
	}

	return link_status;
}


/*cfg p2p cfg p2p rm*/
/*under multi-repeater case when some STA has connect ,
 the other STA don't connect to Remote AP that has diff channel or diff 2nd ch  skip this*/
int multiRepeater_startlookup_chk(struct rtl8192cd_priv *priv , int db_idx)
{

    struct rtl8192cd_priv *priv_root=NULL;
    struct rtl8192cd_priv *priv_other_va=NULL;
    int target_2ndch=0;
    if(!IS_VAP_INTERFACE(priv))
        return 0;
    if((OPMODE & WIFI_STATION_STATE)==0)
        return 0;

    priv_root = GET_ROOT(priv);

    if(priv_root==NULL)
        return 0;

    /*now we default use wlan0-va1 and wlan0-va2 as STA under multiRepeater mode*/
    if(!strcmp(priv->dev->name,"wlan0-va1") || !strcmp(priv->dev->name,"wlan1-va1")){
        priv_other_va=priv_root->pvap_priv[2];
    }else if(!strcmp(priv->dev->name,"wlan0-va2") || !strcmp(priv->dev->name,"wlan1-va2")){
        priv_other_va=priv_root->pvap_priv[1];
    }

    if(priv_other_va && priv_other_va->pmib->dot11OperationEntry.opmode & WIFI_ASOC_STATE){

         /*get target Remote AP's 2nd ch off set*/
    	if ((priv->site_survey->bss_target[db_idx].t_stamp[1] & (BIT(1) | BIT(2))) == (BIT(1) | BIT(2)))
    		target_2ndch = HT_2NDCH_OFFSET_BELOW;
    	else if ((priv->site_survey->bss_target[db_idx].t_stamp[1] & (BIT(1) | BIT(2))) == BIT(1))
    		target_2ndch = HT_2NDCH_OFFSET_ABOVE;
    	else {
    		target_2ndch = HT_2NDCH_OFFSET_DONTCARE;
    	}

        /*check ch and 2nd ch off set*/
        if( (priv_other_va->pmib->dot11RFEntry.dot11channel != priv->site_survey->bss_target[db_idx].channel)||
            ( priv_other_va->pmib->dot11nConfigEntry.dot11n2ndChOffset != target_2ndch))
        {
 		   /*
            if(   priv_other_va->pmib->dot11nConfigEntry.dot11n2ndChOffset != target_2ndch){

                STADEBUG("2nd ch no match other's[%d],target[%d]\n",priv_other_va->pmib->dot11nConfigEntry.dot11n2ndChOffset,target_2ndch);
            }
			*/
                return 1;
        }
    }
    else{
        return 0;
    }

    STADEBUG("\n");
    return 0;

}

int multiRepeater_connection_status(struct rtl8192cd_priv *priv)
{
    struct rtl8192cd_priv *priv_root=NULL;
    struct rtl8192cd_priv *priv_other_va=NULL;

    if(!IS_VAP_INTERFACE(priv))
        return 0;
    if((OPMODE & WIFI_STATION_STATE)==0)
        return 0;

    priv_root = GET_ROOT(priv);

    if(priv_root==NULL)
        return 0;

	//now we default use wlan0-va1 and wlan0-va2 as STA under multiRepeater mode
    if(!strcmp(priv->dev->name,"wlan0-va1") || !strcmp(priv->dev->name,"wlan1-va1")){
        priv_other_va=priv_root->pvap_priv[2];
    }else if(!strcmp(priv->dev->name,"wlan0-va2") || !strcmp(priv->dev->name,"wlan1-va2")){
        priv_other_va=priv_root->pvap_priv[1];
    }

    if(priv_other_va && priv_other_va->pmib->dot11OperationEntry.opmode & WIFI_ASOC_STATE){
        return priv_other_va->pmib->dot11RFEntry.dot11channel;
    }
    else{
        return 0;
    }
}

extern u2Byte dB_Invert_Table[8][12];
static u4Byte rtl8192cd_convertto_db(u4Byte Value)
{
	u1Byte i;
	u1Byte j;
	u4Byte dB;

	Value = Value & 0xFFFF;

	for (i=0;i<8;i++)
	{
		if (Value <= dB_Invert_Table[i][11])
			break;
	}

	if (i >= 8)
		return (96);	// maximum 96 dB

	for (j=0;j<12;j++)
	{
		if (Value <= dB_Invert_Table[i][j])
			break;
	}
	dB = i*12 + j + 1;

	return (dB);
}

static int rtl8192cd_get_psd_data(struct rtl8192cd_priv *priv, unsigned int point)
{
	u4Byte psd_val;

	//2.4G
	if (GET_CHIP_VER(priv)==VERSION_8192E)
	{
		psd_val = RTL_R32(0x808);

		// FFT sample points will be adjusted in "acs_query_psd"
		// set which fft pts we calculate
		psd_val &= 0xFFFFFC00;
		psd_val |= point;
		RTL_W32(0x808, psd_val);

		// Reg 808[22] = 0 ( default )
		// set Reg08[22] = 1, 0->1, PSD activate
		psd_val |= BIT22;
		RTL_W32(0x808, psd_val);

		delay_ms(1);
		psd_val &= ~(BIT22);
		// set Reg08[22] = 0
		RTL_W32(0x808, psd_val);

		psd_val = RTL_R32(0x8B4);
		psd_val &= 0x0000FFFF;
	}
	else if (GET_CHIP_VER(priv)==VERSION_8812E)
	{
		IN PDM_ODM_T pDM_Odm = ODMPTR;

		//Set DCO frequency index, offset=(40MHz/SamplePts)*point
		ODM_SetBBReg(pDM_Odm, 0x910, 0x3FF, point);

		//Start PSD calculation, Reg808[22]=0->1
		ODM_SetBBReg(pDM_Odm, 0x910, BIT22, 1);

		//Need to wait for HW PSD report
		delay_us(150);

		ODM_SetBBReg(pDM_Odm, 0x910, BIT22, 0);

		//Read PSD report, Reg8B4[15:0]
		psd_val = (int)ODM_GetBBReg(pDM_Odm,0xf44, bMaskDWord) & 0x0000FFFF;
		psd_val = (int)(rtl8192cd_convertto_db((u4Byte)psd_val));
	}

	return psd_val;
}

static int rtl8192cd_query_psd_2g(struct rtl8192cd_priv *priv, unsigned int * data, int fft_pts)
{
	unsigned int 	regc70, regc7c, reg800, regc14, regc1c, reg522, reg88c, reg804, reg808, regc50;
	int 			regval;
	unsigned int 	psd_pts=0, psd_start=0, psd_stop=0, psd_data=0, i, j;

	//pre seeting for PSD start ---
	regc50 = RTL_R32(0xc50);	// initial gain

	//store init value
	regc70 = RTL_R32(0xc70);	// AGC
	regc7c = RTL_R32(0xc7c);	// AAGC
	reg800 = RTL_R32(0x800);	// CCK
	regc14 = RTL_R32(0xc14);	// IQ matrix A

	// 2013-07-08 Jeffery modified
	regc1c = RTL_R32(0xc1c);	// IQ matrix B

	reg522 = RTL_R8(0x522);	// MAC queen
	reg88c = RTL_R32(0x88c);	// 3-wire
	reg804 = RTL_R32(0x804);	// PSD ant select
	reg808 = RTL_R32(0x808);	// PSD setting

	//CCK off
	regval = RTL_R32(0x800);
	regval = regval & (~BIT24);//808[24]=0
	// regval = regval & 0xFEFFFFFF;//808[24]=0
	RTL_W32(0x800, regval);

	// CCA off path A & B, set IQ matrix = 0
	RTL_W32(0xc14, 0x0);
	RTL_W32(0xc1c, 0x0);

	//TX off, MAC queen
	RTL_W8(0x522, 0xFF);

	//2 set IGI before 3-wire off

	// caution ! mib uses decimal value
	RTL_W8(0xc50, 0x30);		// default IGI = -30 dBm
	// RTL_W8(0xc50, 0x32);

	//3-wire off
	regval = RTL_R32(0x88c);
	regval = regval | (BIT23|BIT22|BIT21|BIT20) ;
	RTL_W32(0x88c, regval) ;

	//set PSD path  a, b = 0, 1
	//path = 0;
	regval = RTL_R32(0x804);
	regval = regval & (~(BIT4|BIT5));
	// regval = regval & 0xFFFFFFCF;
	RTL_W32(0x804, regval);


	regval = RTL_R32(0x808);

	//FFT pts 128, 256, 512, 1024 = 0, 1, 2, 3
#if 1
	if(fft_pts == 128)
		regval = regval & 0xFFFF3FFF;
	else if(fft_pts == 256)
		regval = regval & 0xFFFF7FFF;
	else if(fft_pts == 512)
		regval = regval & 0xFFFFBFFF;
	//else if(fft_pts == 1024)
	//	regval = regval & 0xFFFFFFFF;
	else//default 128
	{
		regval = regval & 0xFFFF3FFF;
		fft_pts = 128;
	}

	//set psd pts
	psd_start = fft_pts/2;
	psd_stop = psd_start+fft_pts;
	psd_pts = fft_pts;
#else
	//default set PSD pts 128
	printk("[2G] Now we only support PSD to scan 128 points, you set PSD pts:%d\n",fft_pts);
	regval = regval & 0xFFFF3FFF;
	psd_start = 64;
	psd_stop = 192;
	psd_pts = 128;
#endif

	RTL_W32(0x808, regval);


	regval = RTL_R32(0x808);
	regval = regval & ( (~BIT12)|(~BIT13) );
	RTL_W32(0x808, regval);
	//pre setting for PSD end  ---

	//Get PSD Data
	i = psd_start;
	j = 0;
	while(i<psd_stop)
	{
		if( i>= psd_pts)
			psd_data = rtl8192cd_get_psd_data(priv,(i-psd_pts));
		else
			psd_data = rtl8192cd_get_psd_data(priv,i);
		data[j] = psd_data;
		i++;j++;
	}

	//rollback settings start ---
	RTL_W32(0xc70, regc70);
	RTL_W32(0xc7c, regc7c);
	RTL_W32(0x800, reg800);
	RTL_W32(0xc14, regc14);
	RTL_W32(0xc1c, regc1c);
	RTL_W8(0x522, reg522);
	RTL_W32(0x88c, reg88c);
	RTL_W32(0x804, reg804);
	RTL_W32(0x808, reg808);
	RTL_W32(0xc50, regc50);
	//rollback settings start ---

	return psd_pts;
}

static int rtl8192cd_query_psd_5g(struct rtl8192cd_priv *priv, unsigned int * data, int fft_pts)
{
	IN PDM_ODM_T pDM_Odm = ODMPTR;

	unsigned int 	psd_pts=0, psd_start=0, psd_stop=0, psd_data=0, i, j;
	int 			psd_pts_idx, initial_gain = 0x3e, initial_gain_org;


	//pre seeting for PSD start ---
	// Turn off CCK
	ODM_SetBBReg(pDM_Odm, 0x808, BIT28, 0);   //808[28]

	// Turn off TX
	// Pause TX Queue
	if (!priv->pmib->dot11DFSEntry.disable_tx)
		ODM_Write1Byte(pDM_Odm, 0x522, 0xFF); //REG_TXPAUSE set 0xff

	// Turn off CCA
	ODM_SetBBReg(pDM_Odm, 0x838, BIT3, 0x1); //838[3] set 1

	// PHYTXON while loop
	i = 0;
	while (ODM_GetBBReg(pDM_Odm, 0xfa0, BIT18)) {
		i++;
		if (i > 1000000) {
			panic_printk("Wait in %s() more than %d times!\n", __FUNCTION__, i);
			break;
		}
	}

	// backup IGI_origin , set IGI = 0x3e;
	pDM_Odm->bDMInitialGainEnable = FALSE; // setmib dig_enable 0;
	initial_gain_org = ODM_Read1Byte(pDM_Odm, 0xc50);
	ODM_Write_DIG(pDM_Odm, initial_gain);

	delay_us(100);

	// Turn off 3-wire
	ODM_SetBBReg(pDM_Odm, 0xC00, BIT1|BIT0, 0x0); //c00[1:0] set 0

	// pts value = 128=0, 256=1, 512=2, 1024=3
#if 1
	if(fft_pts == 128)
		psd_pts_idx = 0;
	else if(fft_pts == 256)
		psd_pts_idx = 1;
	else if(fft_pts == 512)
		psd_pts_idx = 2;
	//else if(fft_pts == 1024)
	//	psd_pts_idx = 3;
	else//default 128
	{
		psd_pts_idx = 0;
		fft_pts = 128;
	}
	ODM_SetBBReg(pDM_Odm, 0x910, BIT14|BIT15, psd_pts_idx); //910[15:14] set 0, 128 points
	//set psd pts
	psd_start = fft_pts/2;
	psd_stop = psd_start+fft_pts;
	psd_pts = fft_pts;
#else
	printk("[5G] Now we only support PSD to scan 128 points, you set PSD pts:%d\n",fft_pts);
	ODM_SetBBReg(pDM_Odm, 0x910, BIT14|BIT15, 0x0); //910[15:14] set 0, 128 points
	psd_start = 64;
	psd_stop = 192;
	psd_pts = 128;
#endif
	//pre seeting for PSD end ---


	//Get PDS DATA
	i = psd_start;
	j = 0;
	while(i<psd_stop)
	{
		if( i>= psd_pts)
			psd_data = rtl8192cd_get_psd_data(priv,(i-psd_pts));
		else
			psd_data = rtl8192cd_get_psd_data(priv,i);
		data[j] = psd_data;
		i++;j++;
	}


	//rollback settings start ---
	// CCK on
	ODM_SetBBReg(pDM_Odm, 0x808, BIT28, 1); //808[28]

	// Turn on TX
	// Resume TX Queue
	if (!priv->pmib->dot11DFSEntry.disable_tx)
		ODM_Write1Byte(pDM_Odm, 0x522, 0x00); //REG_TXPAUSE set 0x0

	// Turn on 3-wire
	ODM_SetBBReg(pDM_Odm, 0xc00, BIT1|BIT0, 0x3); //c00[1:0] set 3

	// Restore Current Settings
	// Resume DIG
	pDM_Odm->bDMInitialGainEnable = TRUE; // setmib dig_enable 1;
	ODM_Write_DIG(pDM_Odm, initial_gain_org); // set IGI=IGI_origin

	//Turn on CCA
	ODM_SetBBReg(pDM_Odm, 0x838, BIT3, 0); //838[3] set 0
	//rollback settings end ---

	return psd_pts;
}

static int rtl8192cd_query_psd_cfg80211(struct rtl8192cd_priv *priv, int chnl, int bw, int fft_pts)
{
	unsigned int psd_fft_info[1024];//128, 256, 512, 1024
	unsigned int backup_chnl = priv->pmib->dot11RFEntry.dot11channel;
	unsigned int backup_2ndch = priv->pshare->offset_2nd_chan;
	unsigned int backup_bw = priv->pshare->CurrentChannelBW;
	int i, scan_pts=0;

	if (!netif_running(priv->dev)) {
		printk("\nFail: interface not opened\n");
		return -1;
	}

	//check channel
	if(priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_2G)
	{
		if(chnl>14 || chnl<1)
		{
			printk("\nFail: channel %d is not in 2.4G\n", chnl);
			return -1;
		}
	}
	else if(priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_5G)
	{
		if(chnl<14)
		{
			printk("\nFail: channel %d is not in 5G\n", chnl);
			return -1;
		}
	}

	memset(psd_fft_info, 0x0, sizeof(psd_fft_info));

	//set chnl
	SwChnl(priv, chnl, HT_2NDCH_OFFSET_BELOW);
	//set BW, PSD always scan 40M, is this necessary ???
	SwBWMode(priv, HT_CHANNEL_WIDTH_20_40, HT_2NDCH_OFFSET_BELOW);


	//query psd data
	if(priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_2G)
	{
		//printk("Scan 2G PSD in CH[%d] cur_CH[%d] fft_pts[%d]\n", chnl, backup_chnl,fft_pts);
		scan_pts = rtl8192cd_query_psd_2g(priv, psd_fft_info, fft_pts);
	}
	else if(priv->pmib->dot11RFEntry.phyBandSelect == PHY_BAND_5G)
	{
		//printk("Sacn 5G PSD in CH[%d] cur_CH[%d] fft_pts[%d]\n", chnl, backup_chnl, fft_pts);
		scan_pts = rtl8192cd_query_psd_5g(priv, psd_fft_info, fft_pts);
	}

	//rollback chnl and bw
	priv->pmib->dot11RFEntry.dot11channel = backup_chnl;
	priv->pshare->offset_2nd_chan = backup_2ndch;
	priv->pshare->CurrentChannelBW = backup_bw;
	SwChnl(priv,priv->pmib->dot11RFEntry.dot11channel, priv->pshare->offset_2nd_chan);
	SwBWMode(priv, priv->pshare->CurrentChannelBW, priv->pshare->offset_2nd_chan);

	priv->rtk->psd_fft_info[0]=chnl;
	priv->rtk->psd_fft_info[1]=40;//bw;
	priv->rtk->psd_fft_info[2]=scan_pts;
	memcpy(priv->rtk->psd_fft_info+16, psd_fft_info, sizeof(psd_fft_info));

	//for debug
#if 0
	printk("\n PDS Result:\n");
	for(i=0;i<128;i++)
	{
		if(i%16==0)
			printk("\n");
		printk("%3x ", psd_fft_info[i]);
	}
	printk("\n");
#endif

	return 0;
}
