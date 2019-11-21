/*
 *  Handle routines for proc file system
 *
 *  $Id: 8192cd_proc.c,v 1.34.2.15 2011/01/06 07:50:09 button Exp $
 *
 *  Copyright (c) 2009 Realtek Semiconductor Corp.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#define _8192CD_PROC_C_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/netdevice.h>
#include <linux/compiler.h>
#include <linux/init.h>

#include "./8192cd_cfg.h"
#include "./8192cd.h"
#include "./ieee802_mib.h"
#include "./8192cd_cfg80211.h"
#include "./8192cd_headers.h"
#include "./WlanHAL/HalMac88XX/halmac_reg2.h"

#if defined(_INCLUDE_PROC_FS_) || defined(__ECOS)
#include <asm/uaccess.h>

#define SET_DEFAULT_SIZE_FOR_LUNA_SLAVE(_p_)

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,9,0)
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,19)
static inline struct inode *file_inode(struct file *f)
{
	return f->f_path.dentry->d_inode;
}
#else // <= Linux 2.6.19
static inline struct inode *file_inode(struct file *f)
{
	return f->f_dentry->d_inode;
}
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
void *PDE_DATA(const struct inode *inode)
{
	return PDE(inode)->data;
}
#endif

#define RTK_DECLARE_READ_PROC_FOPS(read_proc) \
	int read_proc##_open(struct inode *inode, struct file *file) \
	{ \
			return(single_open(file, read_proc, PDE_DATA(file_inode(file)))); \
	} \
	struct file_operations read_proc##_fops = { \
			.open			= read_proc##_open, \
			.read			= seq_read, \
			.llseek 		= seq_lseek, \
			.release		= single_release, \
	}

#define RTK_DECLARE_WRITE_PROC_FOPS(write_proc) \
	static ssize_t write_proc##_write(struct file * file, const char __user * userbuf, \
		     size_t count, loff_t * off) \
	{ \
		return write_proc(file, userbuf,count, PDE_DATA(file_inode(file))); \
	} \
	struct file_operations write_proc##_fops = { \
			.write			= write_proc##_write, \
	}


#define RTK_DECLARE_READ_WRITE_PROC_FOPS(read_proc,write_proc) \
	static ssize_t read_proc##_write(struct file * file, const char __user * userbuf, \
		     size_t count, loff_t * off) \
	{ \
		return write_proc(file, userbuf,count, PDE_DATA(file_inode(file))); \
	} \
	int read_proc##_open(struct inode *inode, struct file *file) \
	{ \
			return(single_open(file, read_proc, PDE_DATA(file_inode(file)))); \
	} \
	struct file_operations read_proc##_fops = { \
			.open			= read_proc##_open, \
			.read			= seq_read, \
			.write			= read_proc##_write, \
			.llseek 		= seq_lseek, \
			.release		= single_release, \
	}


#define RTK_CREATE_PROC_ENTRY(name) \
{ \
		proc_create_data(name, 0644, rtl8192cd_proc_root, &rtl8192cd_proc_##name##_fops, NULL); \
}

#define RTK_CREATE_PROC_READ_ENTRY(p, name, func) \
{ \
		p = proc_create_data(name, 0644, rtl8192cd_proc_root, &func##_fops, (void *)dev); \
		SET_DEFAULT_SIZE_FOR_LUNA_SLAVE(p); \
}

#define RTK_CREATE_PROC_READ_WRITE_ENTRY(p, name, func, write_func) \
{ \
		p = proc_create_data(name, 0644, rtl8192cd_proc_root, &func##_fops, (void *)dev); \
		SET_DEFAULT_SIZE_FOR_LUNA_SLAVE(p); \
}

#define RTK_CREATE_PROC_WRITE_ENTRY(p, name, write_func) \
{ \
		p = proc_create_data(name, 0644, rtl8192cd_proc_root, &write_func##_fops, (void *)dev); \
		SET_DEFAULT_SIZE_FOR_LUNA_SLAVE(p); \
}






const unsigned char* MCS_DATA_RATEStr[2][2][24] =
{
	{{"6.5", "13", "19.5", "26", "39", "52", "58.5", "65",
	  "13", "26", "39" ,"52", "78", "104", "117", "130",
	  "19.5", "39", "58.5", "78" ,"117", "156", "175.5", "195"},		// Long GI, 20MHz

	 {"7.2", "14.4", "21.7", "28.9", "43.3", "57.8", "65", "72.2",
	  "14.4", "28.9", "43.3", "57.8", "86.7", "115.6", "130", "144.5",
	  "21.7", "43.3", "65", "86.7", "130", "173.3", "195", "216.7"}	},	// Short GI, 20MHz

	{{"13.5", "27", "40.5", "54", "81", "108", "121.5", "135",
	  "27", "54", "81", "108", "162", "216", "243", "270",
	  "40.5", "81", "121.5", "162", "243", "324", "364.5", "405"},		// Long GI, 40MHz

	 {"15", "30", "45", "60", "90", "120", "135", "150",
	  "30", "60", "90", "120", "180", "240", "270", "300",
	  "45", "90", "135", "180", "270", "360", "405", "450"}	}			// Short GI, 40MHz
};


#ifdef  RTK_AC_SUPPORT

extern const u2Byte VHT_MCS_DATA_RATE[3][2][30];
int query_vht_rate(struct stat_info *pstat)
{
	int txrate = pstat->current_tx_rate;
	if(is_MCS_rate(txrate)) {
		unsigned char sg = (pstat->ht_current_tx_info & TX_USE_SHORT_GI) ? 1 : 0;
		if(is_VHT_rate(txrate)) {
			txrate = VHT_MCS_DATA_RATE[MIN_NUM(pstat->tx_bw, 2)][sg][(pstat->current_tx_rate - VHT_RATE_ID)];
		} else {
			char index = pstat->current_tx_rate&0xf;
			txrate=  VHT_MCS_DATA_RATE[MIN_NUM(pstat->tx_bw, 1)][sg][(index <8) ? index :(index+2)];
		}
	}
	return (txrate>>1);
}

#if (MU_BEAMFORMING_SUPPORT == 1)
int query_mu_vht_rate(struct stat_info *pstat)
{
	int txrate = (pstat->mu_rate & 0x7f) - 44 + 0xa0;
	if(is_MCS_rate(txrate)) {
		unsigned char sg = (pstat->mu_rate & 0x80) ? 1:0;
		if(is_VHT_rate(txrate)) {
			txrate = VHT_MCS_DATA_RATE[MIN_NUM(pstat->tx_bw, 2)][sg][(txrate - VHT_RATE_ID)];
		} else {
			char index = pstat->current_tx_rate&0xf;
			txrate=  VHT_MCS_DATA_RATE[MIN_NUM(pstat->tx_bw, 1)][sg][(index <8) ? index :(index+2)];
		}
	}
	return (txrate>>1);
}
#endif

int query_vht_rx_rate(struct stat_info *pstat)
{
	int rxrate = pstat->rx_rate;
	if(is_MCS_rate(rxrate)) {
		unsigned char sg = (pstat->rx_splcp) ? 1 : 0;
		if(is_VHT_rate(rxrate)) {
			rxrate = VHT_MCS_DATA_RATE[MIN_NUM(pstat->rx_bw, 2)][sg][(pstat->rx_rate - VHT_RATE_ID)];
		} else {
			char index = pstat->rx_rate &0xf;
			rxrate=  VHT_MCS_DATA_RATE[MIN_NUM(pstat->rx_bw, 1)][sg][(index <8) ? index :(index+2)];
		}
	}
	return (rxrate>>1);
}

#endif

static int rtl8192cd_proc_mib_staconfig(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();

	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);

	int pos = 0, i;
	unsigned char tmpbuf[100];

	PRINT_ONE("  Dot11StationConfigEntry...", "%s", 1);
	PRINT_ARRAY_ARG("    dot11Bssid: ",
			priv->pmib->dot11StationConfigEntry.dot11Bssid, "%02x", 6);

	memcpy(tmpbuf, priv->pmib->dot11StationConfigEntry.dot11DesiredSSID, priv->pmib->dot11StationConfigEntry.dot11DesiredSSIDLen);
	tmpbuf[priv->pmib->dot11StationConfigEntry.dot11DesiredSSIDLen] = '\0';
	PRINT_ONE("    dot11DesiredSSID:(Len ", "%s", 0);
	PRINT_ONE(priv->pmib->dot11StationConfigEntry.dot11DesiredSSIDLen, "%d) ", 0);
	PRINT_ONE(tmpbuf, "%s", 1);

	memcpy(tmpbuf, priv->pmib->dot11StationConfigEntry.dot11DefaultSSID, priv->pmib->dot11StationConfigEntry.dot11DefaultSSIDLen);
	tmpbuf[priv->pmib->dot11StationConfigEntry.dot11DefaultSSIDLen] = '\0';
	PRINT_ONE("    dot11DefaultSSID:(Len ", "%s", 0);
	PRINT_ONE(priv->pmib->dot11StationConfigEntry.dot11DefaultSSIDLen, "%d) ", 0);
	PRINT_ONE(tmpbuf, "%s", 1);

	memcpy(tmpbuf, priv->pmib->dot11StationConfigEntry.dot11SSIDtoScan, priv->pmib->dot11StationConfigEntry.dot11SSIDtoScanLen);
	tmpbuf[priv->pmib->dot11StationConfigEntry.dot11SSIDtoScanLen] = '\0';
	PRINT_ONE("    dot11SSIDtoScan:(Len ", "%s", 0);
	PRINT_ONE(priv->pmib->dot11StationConfigEntry.dot11SSIDtoScanLen, "%d) ", 0);
	PRINT_ONE(tmpbuf, "%s", 1);

	PRINT_ARRAY_ARG("    dot11DesiredBssid: ",
			priv->pmib->dot11StationConfigEntry.dot11DesiredBssid, "%02x", 6);
	PRINT_ARRAY_ARG("    dot11OperationalRateSet: ",
			priv->pmib->dot11StationConfigEntry.dot11OperationalRateSet, "%02x",
			priv->pmib->dot11StationConfigEntry.dot11OperationalRateSetLen);
	PRINT_SINGL_ARG("    dot11OperationalRateSetLen: ",
			priv->pmib->dot11StationConfigEntry.dot11OperationalRateSetLen, "%d");
	PRINT_SINGL_ARG("    dot11BeaconPeriod: ",
			priv->pmib->dot11StationConfigEntry.dot11BeaconPeriod, "%d");
	PRINT_SINGL_ARG("    dot11DTIMPeriod: ",
			priv->pmib->dot11StationConfigEntry.dot11DTIMPeriod, "%d");
	PRINT_SINGL_ARG("    dot11swcrypto: ",
			priv->pmib->dot11StationConfigEntry.dot11swcrypto, "%d");
	PRINT_SINGL_ARG("    dot11AclMode: ",
			priv->pmib->dot11StationConfigEntry.dot11AclMode, "%d");
	PRINT_SINGL_ARG("    dot11AclNum: ",
			priv->pmib->dot11StationConfigEntry.dot11AclNum, "%d");

	for (i=0; i<priv->pmib->dot11StationConfigEntry.dot11AclNum; i++) {
		sprintf(tmpbuf, "    dot11AclAddr[%d]: ", i);
		PRINT_ARRAY_ARG(tmpbuf,	priv->pmib->dot11StationConfigEntry.dot11AclAddr[i], "%02x", 6);
	}
#ifdef D_ACL
	if(priv->pmib->dot11StationConfigEntry.dot11AclMode) {
		struct list_head	*phead, *plist;
		struct wlan_acl_node	*paclnode;
		phead = &priv->wlan_acl_list;
		plist = phead->next;
		if (!list_empty(&priv->wlan_acl_list))
			PRINT_ONE("    Access control list...", "%s", 1);
		while(plist != phead)		{
			paclnode = list_entry(plist, struct wlan_acl_node, list);
			plist = plist->next;
			PRINT_ONE("	 hwaddr: ", "%s", 0);
			PRINT_ARRAY( paclnode->addr, "%02x", MACADDRLEN, 0);
			PRINT_ONE((paclnode->mode== ACL_allow)?"allow":((paclnode->mode== ACL_deny)?"deny":"invalid"), "(%s)", 1);
		}
	}
#endif

	PRINT_SINGL_ARG("    dot11SupportedRates: ",
			priv->pmib->dot11StationConfigEntry.dot11SupportedRates, "0x%x");
	PRINT_SINGL_ARG("    dot11BasicRates: ",
			priv->pmib->dot11StationConfigEntry.dot11BasicRates, "0x%x");
	PRINT_SINGL_ARG("    dot11RegDomain: ",
			priv->pmib->dot11StationConfigEntry.dot11RegDomain, "%d");
	PRINT_SINGL_ARG("    autoRate: ",
			priv->pmib->dot11StationConfigEntry.autoRate, "%d");
	PRINT_SINGL_ARG("    fixedTxRate: ",
			priv->pmib->dot11StationConfigEntry.fixedTxRate, "0x%x");
#ifdef RTK_AC_SUPPORT  //vht rate
	PRINT_SINGL_ARG("    dot11Supported VHT Rates: ",
			priv->pmib->dot11acConfigEntry.dot11SupportedVHT, "%x");
	PRINT_SINGL_ARG("    dot11 VHT tx rate map: ",
			priv->pmib->dot11acConfigEntry.dot11VHT_TxMap, "%x");
#endif
	PRINT_SINGL_ARG("    swTkipMic: ",
			priv->pmib->dot11StationConfigEntry.swTkipMic, "%d");
	PRINT_SINGL_ARG("    protectionDisabled: ",
			priv->pmib->dot11StationConfigEntry.protectionDisabled, "%d");
	PRINT_SINGL_ARG("    olbcDetectDisabled: ",
			priv->pmib->dot11StationConfigEntry.olbcDetectDisabled, "%d");
	PRINT_SINGL_ARG("    nmlscDetectDisabled: ",
			priv->pmib->dot11StationConfigEntry.nmlscDetectDisabled, "%d");
	PRINT_SINGL_ARG("    legacySTADeny: ",
			priv->pmib->dot11StationConfigEntry.legacySTADeny, "%d");
#ifdef CLIENT_MODE
	PRINT_SINGL_ARG("    fastRoaming: ",
			priv->pmib->dot11StationConfigEntry.fastRoaming, "%d");
#endif
	PRINT_SINGL_ARG("    lowestMlcstRate: ",
			priv->pmib->dot11StationConfigEntry.lowestMlcstRate, "%d");
	PRINT_SINGL_ARG("    supportedStaNum: ",
			priv->pmib->dot11StationConfigEntry.supportedStaNum, "%d");
	PRINT_SINGL_ARG("    totalMaxStaNum: ",
			priv->pshare->rf_ft_var.dynamic_max_num_stat, "%d");
	PRINT_SINGL_ARG("    Support Broadcast SSID inheritance: ", priv->pmib->dot11StationConfigEntry.bcastSSID_inherit, "%d");
	PRINT_SINGL_ARG("    Inherited Broadcast SSID: ", (priv->take_over_hidden)? 0:1, "%d");

	PRINT_SINGL_ARG("    BeaconRate: ",
			priv->pmib->dot11StationConfigEntry.beacon_rate, "%d");

	if(priv->pptyIE && priv->pptyIE->content) {
		unsigned char cie_str[100]={0}, cie_content[65]={0};

		memcpy(cie_content,priv->pptyIE->content,priv->pptyIE->length);
		cie_content[priv->pptyIE->length] = '\0';
		sprintf(cie_str,"[%d]%02x%02x%02x-%s",priv->pptyIE->id,priv->pptyIE->oui[0],priv->pptyIE->oui[1],priv->pptyIE->oui[2],cie_content);
		PRINT_SINGL_ARG("    Beacon with customized IE: ", cie_str, "%s");
	}

	return pos;
}

static int rtl8192cd_proc_mib_auth(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;

	PRINT_ONE("  Dot1180211AuthEntry...", "%s", 1);
	PRINT_SINGL_ARG("    dot11AuthAlgrthm: ",
			priv->pmib->dot1180211AuthEntry.dot11AuthAlgrthm, "%d");
	PRINT_SINGL_ARG("    dot11PrivacyAlgrthm: ",
			priv->pmib->dot1180211AuthEntry.dot11PrivacyAlgrthm, "%d");
	PRINT_SINGL_ARG("    dot11PrivacyKeyIndex: ",
			priv->pmib->dot1180211AuthEntry.dot11PrivacyKeyIndex, "%d");
	PRINT_SINGL_ARG("    dot11PrivacyKeyLen: ",
			priv->pmib->dot1180211AuthEntry.dot11PrivacyKeyLen, "%d");
	PRINT_SINGL_ARG("    dot11EnablePSK: ",
			priv->pmib->dot1180211AuthEntry.dot11EnablePSK, "%d");
	PRINT_SINGL_ARG("    dot11WPACipher: ",
			priv->pmib->dot1180211AuthEntry.dot11WPACipher, "%d");
#ifdef RTL_WPA2
	PRINT_SINGL_ARG("    dot11WPA2Cipher: ",
			priv->pmib->dot1180211AuthEntry.dot11WPA2Cipher, "%d");
#endif
#ifdef CONFIG_IEEE80211W
	PRINT_SINGL_ARG("    dot11IEEE80211W: ",
			priv->pmib->dot1180211AuthEntry.dot11IEEE80211W, "%d");
	PRINT_SINGL_ARG("    dot11EnableSHA256: ",
			priv->pmib->dot1180211AuthEntry.dot11EnableSHA256, "%d");
#endif
	PRINT_SINGL_ARG("    dot11PassPhrase: ",
			priv->pmib->dot1180211AuthEntry.dot11PassPhrase, "%s");
	PRINT_SINGL_ARG("    dot11PassPhraseGuest: ",
			priv->pmib->dot1180211AuthEntry.dot11PassPhraseGuest, "%s");
	PRINT_SINGL_ARG("    dot11GKRekeyTime: ",
			priv->pmib->dot1180211AuthEntry.dot11GKRekeyTime, "%ld");
	PRINT_SINGL_ARG("    dot11UKRekeyTime: ",
			priv->pmib->dot1180211AuthEntry.dot11UKRekeyTime, "%ld");

#ifdef CLIENT_MODE
    if (OPMODE & WIFI_STATION_STATE) {
        PRINT_SINGL_ARG("    4-way status: ",
            priv->dot114WayStatus, "%d");
		PRINT_SINGL_ARG("    4-way finished: ",
            priv->is_4Way_finished, "%d");
    }
#endif

	PRINT_ONE("  Dot118021xAuthEntry...", "%s", 1);
	PRINT_SINGL_ARG("    dot118021xAlgrthm: ",
			priv->pmib->dot118021xAuthEntry.dot118021xAlgrthm, "%d");
	PRINT_SINGL_ARG("    dot118021xDefaultPort: ",
			priv->pmib->dot118021xAuthEntry.dot118021xDefaultPort, "%d");
	PRINT_SINGL_ARG("    dot118021xcontrolport: ",
			priv->pmib->dot118021xAuthEntry.dot118021xcontrolport, "%d");
	PRINT_ONE("  RADIUS Accounting...", "%s", 1);
	PRINT_SINGL_ARG("    Enabled: ",priv->pmib->dot118021xAuthEntry.acct_enabled, "%d");
	PRINT_SINGL_ARG("    Idle period to leave STA: ",priv->pmib->dot118021xAuthEntry.acct_timeout_period, "%lu min(s)");
	PRINT_SINGL_ARG("    Idle throughput to leave STA: ", priv->pmib->dot118021xAuthEntry.acct_timeout_throughput, "%d Kbpm");

	PRINT_ONE("  Dot11RsnIE...", "%s", 1);
	PRINT_ARRAY_ARG("    rsnie: ",
			priv->pmib->dot11RsnIE.rsnie, "%02x", priv->pmib->dot11RsnIE.rsnielen);
	PRINT_SINGL_ARG("    rsnielen: ", priv->pmib->dot11RsnIE.rsnielen, "%d");


	return pos;
}

static int rtl8192cd_proc_mib_dkeytbl(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;

	PRINT_ONE("  Dot11DefaultKeysTable...", "%s", 1);
	PRINT_ARRAY_ARG("    keytype[0].skey: ",
			priv->pmib->dot11DefaultKeysTable.keytype[0].skey, "%02x", 16);
	PRINT_ARRAY_ARG("    keytype[1].skey: ",
			priv->pmib->dot11DefaultKeysTable.keytype[1].skey, "%02x", 16);
	PRINT_ARRAY_ARG("    keytype[2].skey: ",
			priv->pmib->dot11DefaultKeysTable.keytype[2].skey, "%02x", 16);
	PRINT_ARRAY_ARG("    keytype[3].skey: ",
			priv->pmib->dot11DefaultKeysTable.keytype[3].skey, "%02x", 16);
	return pos;
}

static int rtl8192cd_proc_mib_gkeytbl(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;
	unsigned char *ptr;

	PRINT_ONE("  Dot11GroupKeysTable...", "%s", 1);
	{
		PRINT_SINGL_ARG("    dot11Privacy: ",
				priv->pmib->dot11GroupKeysTable.dot11Privacy, "%d");
		PRINT_SINGL_ARG("    keyInCam: ", (priv->pmib->dot11GroupKeysTable.keyInCam? "yes" : "no"), "%s");
		PRINT_SINGL_ARG("    dot11EncryptKey.dot11TTKeyLen: ",
				priv->pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKeyLen, "%d");
		PRINT_SINGL_ARG("    dot11EncryptKey.dot11TMicKeyLen: ",
				priv->pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TMicKeyLen, "%d");
		PRINT_ARRAY_ARG("    dot11EncryptKey.dot11TTKey.skey: ",
				priv->pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TTKey.skey, "%02x", 16);
		PRINT_ARRAY_ARG("    dot11EncryptKey.dot11TMicKey1.skey: ",
				priv->pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TMicKey1.skey, "%02x", 16);
		PRINT_ARRAY_ARG("    dot11EncryptKey.dot11TMicKey2.skey: ",
				priv->pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TMicKey2.skey, "%02x", 16);
		ptr = (unsigned char *)&priv->pmib->dot11GroupKeysTable.dot11EncryptKey.dot11TXPN48.val48;
		PRINT_ARRAY_ARG("    dot11EncryptKey.dot11TXPN48.val48: ", ptr, "%02x", 8);
		ptr = (unsigned char *)&priv->pmib->dot11GroupKeysTable.dot11EncryptKey.dot11RXPN48.val48;
		PRINT_ARRAY_ARG("    dot11EncryptKey.dot11RXPN48.val48: ", ptr, "%02x", 8);


		PRINT_SINGL_ARG("    dot11EncryptKey2.dot11TTKeyLen: ",
				priv->pmib->dot11GroupKeysTable.dot11EncryptKey2.dot11TTKeyLen, "%d");
		PRINT_SINGL_ARG("    dot11EncryptKey2.dot11TMicKeyLen: ",
				priv->pmib->dot11GroupKeysTable.dot11EncryptKey2.dot11TMicKeyLen, "%d");
		PRINT_ARRAY_ARG("    dot11EncryptKey2.dot11TTKey.skey: ",
				priv->pmib->dot11GroupKeysTable.dot11EncryptKey2.dot11TTKey.skey, "%02x", 16);
		PRINT_ARRAY_ARG("    dot11EncryptKey2.dot11TMicKey1.skey: ",
				priv->pmib->dot11GroupKeysTable.dot11EncryptKey2.dot11TMicKey1.skey, "%02x", 16);
		PRINT_ARRAY_ARG("    dot11EncryptKey2.dot11TMicKey2.skey: ",
				priv->pmib->dot11GroupKeysTable.dot11EncryptKey2.dot11TMicKey2.skey, "%02x", 16);
				ptr = (unsigned char *)&priv->pmib->dot11GroupKeysTable.dot11EncryptKey2.dot11TXPN48.val48;
		PRINT_ARRAY_ARG("    dot11EncryptKey2.dot11TXPN48.val48: ", ptr, "%02x", 8);
				ptr = (unsigned char *)&priv->pmib->dot11GroupKeysTable.dot11EncryptKey2.dot11RXPN48.val48;
		PRINT_ARRAY_ARG("    dot11EncryptKey2.dot11RXPN48.val48: ", ptr, "%02x", 8);
	}

	return pos;
}

static int rtl8192cd_proc_mib_operation(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;
	char tmpbuf[20];
	int idx = 0;

	PRINT_ONE("  Dot11OperationEntry...", "%s", 1);
	PRINT_ARRAY_ARG("    hwaddr: ",	priv->pmib->dot11OperationEntry.hwaddr, "%02x", 6);
	PRINT_SINGL_ARG("    opmode: ", priv->pmib->dot11OperationEntry.opmode, "0x%x");
	PRINT_SINGL_ARG("    vap_init_seq: ", priv->vap_init_seq, "0x%x");
	PRINT_SINGL_ARG("    hiddenAP: ", priv->pmib->dot11OperationEntry.hiddenAP, "%d");
	PRINT_SINGL_ARG("    dot11RTSThreshold: ", priv->pmib->dot11OperationEntry.dot11RTSThreshold, "%d");
	PRINT_SINGL_ARG("    dot11FragmentationThreshold: ", priv->pmib->dot11OperationEntry.dot11FragmentationThreshold, "%d");
	PRINT_SINGL_ARG("    dot11ShortRetryLimit: ", priv->pmib->dot11OperationEntry.dot11ShortRetryLimit, "%d");
	PRINT_SINGL_ARG("    dot11LongRetryLimit: ", priv->pmib->dot11OperationEntry.dot11LongRetryLimit, "%d");
	PRINT_SINGL_ARG("    expiretime: ", priv->pmib->dot11OperationEntry.expiretime, "%d");
	PRINT_SINGL_ARG("    led_type: ", priv->pmib->dot11OperationEntry.ledtype, "%d");
#ifdef RTL8190_SWGPIO_LED
	PRINT_SINGL_ARG("    led_route: ", priv->pmib->dot11OperationEntry.ledroute, "0x%x");
#endif
	PRINT_SINGL_ARG("    iapp_enable: ", priv->pmib->dot11OperationEntry.iapp_enable, "%d");
	PRINT_SINGL_ARG("    block_relay: ", priv->pmib->dot11OperationEntry.block_relay, "%d");
	PRINT_SINGL_ARG("    deny_any: ", priv->pmib->dot11OperationEntry.deny_any, "%d");
	PRINT_SINGL_ARG("    crc_log: ", priv->pmib->dot11OperationEntry.crc_log, "%d");
	PRINT_SINGL_ARG("    wifi_specific: ", priv->pmib->dot11OperationEntry.wifi_specific, "%d");
#ifdef WIFI_WMM
	PRINT_SINGL_ARG("    qos_enable: ", priv->pmib->dot11QosEntry.dot11QosEnable, "%d");
#ifdef WMM_APSD
	PRINT_SINGL_ARG("    apsd_enable: ", priv->pmib->dot11QosEntry.dot11QosAPSD, "%d");
#ifdef CLIENT_MODE
	if ((OPMODE & WIFI_STATION_STATE) && QOS_ENABLE && APSD_ENABLE) {
		PRINT_SINGL_ARG("        uapsd_assoc: ", priv->uapsd_assoc, "%d");
		PRINT_SINGL_ARG("        UAPSD_AC_VO: ", priv->pmib->dot11QosEntry.UAPSD_AC_VO, "%d");
		PRINT_SINGL_ARG("        UAPSD_AC_VI: ", priv->pmib->dot11QosEntry.UAPSD_AC_VI, "%d");
		PRINT_SINGL_ARG("        UAPSD_AC_BE: ", priv->pmib->dot11QosEntry.UAPSD_AC_BE, "%d");
		PRINT_SINGL_ARG("        UAPSD_AC_BK: ", priv->pmib->dot11QosEntry.UAPSD_AC_BK, "%d");
	}
#endif
#endif
#endif

	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11A)
		tmpbuf[idx++] = 'A';
	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11B)
		tmpbuf[idx++] = 'B';
	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11G)
		tmpbuf[idx++] = 'G';
	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11N)
		tmpbuf[idx++] = 'N';
#ifdef RTK_AC_SUPPORT
	if (priv->pmib->dot11BssType.net_work_type & WIRELESS_11AC) {
		tmpbuf[idx++] = 'A';
		tmpbuf[idx++] = 'C';
	}
#endif

	tmpbuf[idx] = '\0';
	PRINT_SINGL_ARG("    net_work_type: ", tmpbuf, "%s");

#ifdef TX_SHORTCUT
	PRINT_SINGL_ARG("    disable_txsc: ", priv->pmib->dot11OperationEntry.disable_txsc, "%d");
#endif

#ifdef RX_SHORTCUT
	PRINT_SINGL_ARG("    disable_rxsc: ", priv->pmib->dot11OperationEntry.disable_rxsc, "%d");
#endif

#ifdef TX_SHORTCUT
#ifdef SUPPORT_TX_AMSDU_SHORTCUT
	PRINT_SINGL_ARG("    disable_amsdu_txsc: ", priv->pmib->dot11OperationEntry.disable_amsdu_txsc, "%d");
#endif
#endif


	PRINT_SINGL_ARG("    guest_access: ", priv->pmib->dot11OperationEntry.guest_access, "%d");
	PRINT_SINGL_ARG("    tdls_prohibited:  ", priv->pmib->dot11OperationEntry.tdls_prohibited, "%d");
	PRINT_SINGL_ARG("    tdls_cs_prohibited:  ", priv->pmib->dot11OperationEntry.tdls_cs_prohibited, "%d");
#ifdef CONFIG_IEEE80211V
	if(WNM_ENABLE) {
		PRINT_SINGL_ARG("    BssTransEnable:  ", 	priv->pmib->wnmEntry.dot11vBssTransEnable, "%d");
   		PRINT_SINGL_ARG("    BssReqMode:  ", 		priv->pmib->wnmEntry.dot11vReqMode, "%d");
		PRINT_SINGL_ARG("    BssDiassocImminent:  ", priv->pmib->wnmEntry.dot11vDiassocImminent, "%d");
		PRINT_SINGL_ARG("    BssDiassocDeadline:  ", priv->pmib->wnmEntry.dot11vDiassocDeadline, "%d");
	}
#endif
	return pos;
}

#if (BEAMFORMING_SUPPORT == 1)
static int rtl8192cd_proc_mib_txbf(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;
	int idx;
	PRT_BEAMFORMING_INFO 		pBeamInfo = &(priv->pshare->BeamformingInfo);
	PRT_BEAMFORMING_ENTRY		pEntry = NULL;
	PRT_BEAMFORMER_ENTRY		pTXBFerEntry = NULL;
	struct aid_obj *aidobj;
	PRINT_ONE("  TXBFee Entry...", "%s", 1);
	#if (MU_BEAMFORMING_SUPPORT == 1)
	PRINT_SINGL_ARG("    beamformee_su_reg_maping: ", pBeamInfo->beamformee_su_reg_maping, "%x");
	PRINT_SINGL_ARG("    beamformer_su_cnt: ", pBeamInfo->beamformer_su_cnt, "%d");
	PRINT_SINGL_ARG("    beamformee_su_cnt: ", pBeamInfo->beamformee_su_cnt, "%d");

	if(priv->pmib->dot11RFEntry.txbf_mu) {
		PRINT_SINGL_ARG("    beamformee_mu_reg_maping: ", pBeamInfo->beamformee_mu_reg_maping, "%x");
		PRINT_SINGL_ARG("    beamformer_mu_cnt: ", pBeamInfo->beamformer_mu_cnt, "%d");
		PRINT_SINGL_ARG("    beamformee_mu_cnt: ", pBeamInfo->beamformee_mu_cnt, "%d");
	}
	#endif
	for(idx = 0; idx < BEAMFORMEE_ENTRY_NUM; idx++) {
		pEntry = &(pBeamInfo->BeamformeeEntry[idx]);
		PRINT_ONE(idx,  " %d: txbfee entry...", 1);
		PRINT_SINGL_ARG("    bUsed: ", pEntry->bUsed, "%x");
		if(pEntry->bUsed && pEntry->pSTA) {
			aidobj = container_of(pEntry->pSTA, struct aid_obj, station);
			priv = aidobj->priv;
			PRINT_ONE(aidobj->priv->dev->name, "    Interface: %s\n" , 0);
		}
		PRINT_ARRAY_ARG("    MacAddr: ", pEntry->MacAddr, "%02x", MACADDRLEN);
		PRINT_SINGL_ARG("    MacId: ", pEntry->MacId, "%d");
		PRINT_SINGL_ARG("    bSound: ", pEntry->bSound, "%x");
		PRINT_SINGL_ARG("    BeamformEntryCap: ", pEntry->BeamformEntryCap, "%x");
		#if (MU_BEAMFORMING_SUPPORT == 1)
		if(priv->pmib->dot11RFEntry.txbf_mu) {
			PRINT_SINGL_ARG("    is_mu_sta: ", pEntry->is_mu_sta, "%d");
			if(pEntry->is_mu_sta) {
				PRINT_SINGL_ARG("    mu_reg_index: ", pEntry->mu_reg_index, "%d");
			} else {
				PRINT_SINGL_ARG("    su_reg_index: ", pEntry->su_reg_index, "%d");
			}
		}
		else
		#endif
		{
			PRINT_SINGL_ARG("    su_reg_index: ", pEntry->su_reg_index, "%d");
		}
	}
	PRINT_ONE("  TXBFer Entry...", "%s", 1);
	for(idx = 0; idx < BEAMFORMER_ENTRY_NUM; idx++) {
		pTXBFerEntry = &(pBeamInfo->BeamformerEntry[idx]);
		PRINT_ONE(idx,  " %d: txbfer entry...", 1);
		PRINT_SINGL_ARG("    bUsed: ", pTXBFerEntry->bUsed, "%x");
		PRINT_ARRAY_ARG("    MacAddr: ", pTXBFerEntry->MacAddr, "%02x", MACADDRLEN);
		PRINT_SINGL_ARG("    BeamformEntryCap: ", pTXBFerEntry->BeamformEntryCap, "%x");
		#if (MU_BEAMFORMING_SUPPORT == 1)
		if(priv->pmib->dot11RFEntry.txbf_mu) {
			PRINT_SINGL_ARG("    is_mu_ap: ", pTXBFerEntry->is_mu_ap, "%x");
			if(!pTXBFerEntry->is_mu_ap) {
				PRINT_SINGL_ARG("    su_reg_index: ", pTXBFerEntry->su_reg_index, "%d");
			}
		}
		else
		#endif
		{
			PRINT_SINGL_ARG("    su_reg_index: ", pTXBFerEntry->su_reg_index, "%d");
		}
	}
	return pos;
}
#endif

struct timer_list *get_channel_timer(struct rtl8192cd_priv *priv, int channel)
{
	switch(channel) {
		case 52:
			return &priv->ch52_timer;
			break;
		case 56:
			return &priv->ch56_timer;
			break;
		case 60:
			return &priv->ch60_timer;
			break;
		case 64:
			return &priv->ch64_timer;
			break;
		case 100:
			return &priv->ch100_timer;
			break;
		case 104:
			return &priv->ch104_timer;
			break;
		case 108:
			return &priv->ch108_timer;
			break;
		case 112:
			return &priv->ch112_timer;
			break;
		case 116:
			return &priv->ch116_timer;
			break;
		case 120:
			return &priv->ch120_timer;
			break;
		case 124:
			return &priv->ch124_timer;
			break;
		case 128:
			return &priv->ch128_timer;
			break;
		case 132:
			return &priv->ch132_timer;
			break;
		case 136:
			return &priv->ch136_timer;
			break;
		case 140:
			return &priv->ch140_timer;
			break;
		case 144:
			return &priv->ch144_timer;
			break;
		default:
			DEBUG_ERR("DFS_timer: Channel match none!\n");
			break;
	}
}


static int rtl8192cd_proc_mib_DFS(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;
	int i;
	struct timer_list *ch_timer;
	unsigned long rem_time;

	PRINT_ONE("  Dot11DFSEntry...", "%s", 1);

	PRINT_SINGL_ARG("    DFS version: ", get_DFS_version(), "%s");
	PRINT_SINGL_ARG("    disable_DFS: ", priv->pmib->dot11DFSEntry.disable_DFS, "%d");
	PRINT_SINGL_ARG("    DFS_timeout: ", priv->pmib->dot11DFSEntry.DFS_timeout, "%d");
	PRINT_SINGL_ARG("    DFS_detected: ", priv->pmib->dot11DFSEntry.DFS_detected, "%d");
	PRINT_SINGL_ARG("    NOP_timeout: ", priv->pmib->dot11DFSEntry.NOP_timeout, "%d");
	PRINT_SINGL_ARG("    DFS_TXPAUSE_timeout: ", priv->pmib->dot11DFSEntry.DFS_TXPAUSE_timeout, "%d");
	PRINT_SINGL_ARG("    disable_tx: ", priv->pmib->dot11DFSEntry.disable_tx, "%d");
	PRINT_SINGL_ARG("    CAC_enable: ", priv->pmib->dot11DFSEntry.CAC_enable, "%d");
	PRINT_ONE("    NOP channels...", "%s", 1);
	PRINT_ONE("      ", "%s", 0);
	for (i=0; i<priv->NOP_chnl_num; i++) {
		PRINT_ONE(priv->NOP_chnl[i], "[%d]:", 0);
		ch_timer = get_channel_timer(priv, priv->NOP_chnl[i]);
		rem_time = RTL_JIFFIES_TO_SECOND(TSF_DIFF(ch_timer->expires, jiffies));
		if (rem_time > 0) {
			PRINT_ONE(rem_time, "%dsec ", 0);
		}
		else {
			PRINT_ONE("<1sec ", "%s" , 0);
		}
	}
	PRINT_ONE(" ", "%s", 1);
	if(priv->available_chnl_num) {
	  	PRINT_ARRAY_ARG("    AVAIL_CH: ", priv->available_chnl, "%d ", priv->available_chnl_num);
 	} else {
  		PRINT_ONE("  AVAIL_CH: None", "%s", 1);
 	}

	return pos;
}


static int rtl8192cd_proc_mib_rf_ac(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;

	PRINT_ONE("  Dot11ACEntry...", "%s", 1);

	PRINT_ARRAY_ARG("	 pwrdiff_5G_20BW1S_OFDM1T_A: ", priv->pmib->dot11RFEntry.pwrdiff_5G_20BW1S_OFDM1T_A, "%02x", MAX_5G_CHANNEL_NUM);
	PRINT_ARRAY_ARG("	 pwrdiff_5G_40BW2S_20BW2S_A: ", priv->pmib->dot11RFEntry.pwrdiff_5G_40BW2S_20BW2S_A, "%02x", MAX_5G_CHANNEL_NUM);
	PRINT_ARRAY_ARG("	 pwrdiff_5G_80BW1S_160BW1S_A: ", priv->pmib->dot11RFEntry.pwrdiff_5G_80BW1S_160BW1S_A, "%02x", MAX_5G_CHANNEL_NUM);
	PRINT_ARRAY_ARG("	 pwrdiff_5G_80BW2S_160BW2S_A: ", priv->pmib->dot11RFEntry.pwrdiff_5G_80BW2S_160BW2S_A, "%02x", MAX_5G_CHANNEL_NUM);


	PRINT_ARRAY_ARG("	 pwrdiff_5G_20BW1S_OFDM1T_B: ", priv->pmib->dot11RFEntry.pwrdiff_5G_20BW1S_OFDM1T_B, "%02x", MAX_5G_CHANNEL_NUM);
	PRINT_ARRAY_ARG("	 pwrdiff_5G_40BW2S_20BW2S_B: ", priv->pmib->dot11RFEntry.pwrdiff_5G_40BW2S_20BW2S_B, "%02x", MAX_5G_CHANNEL_NUM);

	PRINT_ARRAY_ARG("	 pwrdiff_5G_80BW1S_160BW1S_B: ", priv->pmib->dot11RFEntry.pwrdiff_5G_80BW1S_160BW1S_B, "%02x", MAX_5G_CHANNEL_NUM);
	PRINT_ARRAY_ARG("	 pwrdiff_5G_80BW2S_160BW2S_B: ", priv->pmib->dot11RFEntry.pwrdiff_5G_80BW2S_160BW2S_B, "%02x", MAX_5G_CHANNEL_NUM);


	PRINT_ARRAY_ARG("	 pwrdiff_20BW1S_OFDM1T_A: ", priv->pmib->dot11RFEntry.pwrdiff_20BW1S_OFDM1T_A, "%02x", MAX_2G_CHANNEL_NUM);
	PRINT_ARRAY_ARG("	 pwrdiff_40BW2S_20BW2S_A: ", priv->pmib->dot11RFEntry.pwrdiff_40BW2S_20BW2S_A, "%02x", MAX_2G_CHANNEL_NUM);
	PRINT_ARRAY_ARG("	 pwrdiff_20BW1S_OFDM1T_B: ", priv->pmib->dot11RFEntry.pwrdiff_20BW1S_OFDM1T_B, "%02x", MAX_2G_CHANNEL_NUM);
	PRINT_ARRAY_ARG("	 pwrdiff_40BW2S_20BW2S_B: ", priv->pmib->dot11RFEntry.pwrdiff_40BW2S_20BW2S_B, "%02x", MAX_2G_CHANNEL_NUM);

	return pos;
}

#ifdef BT_COEXIST
static int rtl8192cd_proc_bt_coexist(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;
	extern int c2h_bt_cnt;
	extern int bt_state;

	PRINT_ONE("  BT Coexist Info:", "%s", 1);
	PRINT_SINGL_ARG("    btc            : ", priv->pshare->rf_ft_var.btc, "%u");
	PRINT_SINGL_ARG("    bt_state       : ", bt_state, "%d");
	PRINT_SINGL_ARG("    c2h_bt_cnt     : ", c2h_bt_cnt, "%d");
	PRINT_SINGL_ARG("    0x40           : ", PHY_QueryBBReg(priv, 0x40, 0xffffffff), "%x");
	PRINT_SINGL_ARG("    0x4f           : ", PHY_QueryBBReg(priv, 0x4c,  0xff000000), "%x");
	PRINT_SINGL_ARG("    0x522          : ", PHY_QueryBBReg(priv, 0x520, 0x00ff0000), "%x");
	PRINT_SINGL_ARG("    0x550          : ", PHY_QueryBBReg(priv, 0x550, 0xffffffff), "%x");
	PRINT_SINGL_ARG("    0x6c0   BT Slot: ", PHY_QueryBBReg(priv, 0x6c0, 0xffffffff), "%x");
	PRINT_SINGL_ARG("    0x6c4 Wifi Slot: ", PHY_QueryBBReg(priv, 0x6c4, 0xffffffff), "%x");
	PRINT_SINGL_ARG("    0x6c8          : ", PHY_QueryBBReg(priv, 0x6c8, 0xffffffff), "%x");
	PRINT_SINGL_ARG("    0x770   (hp rx): ", PHY_QueryBBReg(priv, 0x770, 0xffff0000), "%d");
	PRINT_SINGL_ARG("    0x770   (hp tx): ", PHY_QueryBBReg(priv, 0x770, 0x0000ffff), "%d");
	PRINT_SINGL_ARG("    0x774   (lp rx): ", PHY_QueryBBReg(priv, 0x774, 0xffff0000), "%d");
	PRINT_SINGL_ARG("    0x774   (lp tx): ", PHY_QueryBBReg(priv, 0x774, 0x0000ffff), "%d");
	PRINT_SINGL_ARG("    0x778          : ", PHY_QueryBBReg(priv, 0x778, 0xffffffff), "%x");
	PRINT_SINGL_ARG("    0x92c BT/Wifi Switch: ", PHY_QueryBBReg(priv, 0x92c, 0xffffffff), "%x");
	PRINT_SINGL_ARG("    0x930  HW/SW Control: ", PHY_QueryBBReg(priv, 0x930, 0xffffffff), "%x");
	PRINT_SINGL_ARG("    0xc50          : ", PHY_QueryBBReg(priv, 0xc50, 0xffffffff), "%x");
	PRINT_SINGL_ARG("    0xc58          : ", PHY_QueryBBReg(priv, 0xc58, 0xffffffff), "%x");

	return pos;
}
#endif

static int rtl8192cd_proc_wmm(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;
	int rf_path;
	unsigned char tmpbuf[16];
	panic_printk("	  Cwin	Cmax  AIFS	TXOP\n");
	panic_printk("BK  %3d	%3d   %3d	%3d (%x)\n",PHY_QueryBBReg(priv, 0x50c, 0xf00),PHY_QueryBBReg(priv, 0x50c, 0xf000),PHY_QueryBBReg(priv, 0x50c, bMaskByte0),PHY_QueryBBReg(priv, 0x50c, 0x7ff0000),PHY_QueryBBReg(priv, 0x50c, bMaskDWord));
	panic_printk("BE  %3d	%3d   %3d	%3d (%x)\n",PHY_QueryBBReg(priv, 0x508, 0xf00),PHY_QueryBBReg(priv, 0x508, 0xf000),PHY_QueryBBReg(priv, 0x508, bMaskByte0),PHY_QueryBBReg(priv, 0x508, 0x7ff0000),PHY_QueryBBReg(priv, 0x508, bMaskDWord));
	panic_printk("VI  %3d	%3d   %3d	%3d (%x)\n",PHY_QueryBBReg(priv, 0x504, 0xf00),PHY_QueryBBReg(priv, 0x504, 0xf000),PHY_QueryBBReg(priv, 0x504, bMaskByte0),PHY_QueryBBReg(priv, 0x504, 0x7ff0000),PHY_QueryBBReg(priv, 0x504, bMaskDWord));
	panic_printk("VO  %3d	%3d   %3d	%3d (%x)\n\n",PHY_QueryBBReg(priv, 0x500, 0xf00),PHY_QueryBBReg(priv, 0x500, 0xf000),PHY_QueryBBReg(priv, 0x500, bMaskByte0),PHY_QueryBBReg(priv, 0x500, 0x7ff0000),PHY_QueryBBReg(priv, 0x500, bMaskDWord));

	panic_printk("High queue page count 0x230: %x,\n", PHY_QueryBBReg(priv, 0x230, bMaskDWord));
	panic_printk("Low queue page count 0x234: %x,\n", PHY_QueryBBReg(priv, 0x234, bMaskDWord));
	panic_printk("Normal queue page count 0x238: %x,\n", PHY_QueryBBReg(priv, 0x238, bMaskDWord));
	panic_printk("Extra queue page count 0x23c: %x,\n", PHY_QueryBBReg(priv, 0x23c, bMaskDWord));
	panic_printk("Public queue page count 0x240: %x,\n", PHY_QueryBBReg(priv, 0x240, bMaskDWord));
	panic_printk("Queue mapping 0x10c: %x,\n", PHY_QueryBBReg(priv, 0x10c, bMaskDWord));

#ifdef WIFI_WMM
	panic_printk("VO_pkt_count: %u,\n",priv->pshare->phw->VO_pkt_count);
	panic_printk("VI_pkt_count: %u,\n",priv->pshare->phw->VI_pkt_count);
	panic_printk("BE_pkt_count: %u,\n",priv->pshare->phw->BE_pkt_count);
	panic_printk("BK_pkt_count: %u,\n\n",priv->pshare->phw->BK_pkt_count);

	panic_printk("iot_mode_VO_exist: %u,\n",priv->pshare->iot_mode_VO_exist);
	panic_printk("iot_mode_VI_exist: %u,\n",priv->pshare->iot_mode_VI_exist);
	panic_printk("iot_mode_BK_exist: %u,\n",priv->pshare->iot_mode_BK_exist);
	panic_printk("iot_mode_BE_exist: %u,\n\n",priv->pshare->iot_mode_BE_exist);

	panic_printk("VI_rx_pkt_count: %u,\n",priv->pshare->phw->VI_rx_pkt_count);
	panic_printk("wifi_beq_iot: %u,\n",priv->pshare->rf_ft_var.wifi_beq_iot);
#endif
	panic_printk("txop_enlarge: %u,\n",priv->pshare->txop_enlarge);
	panic_printk("txop_enlarge_lower: %u,\n",priv->pshare->rf_ft_var.txop_enlarge_lower);
	panic_printk("txop_enlarge_upper: %u,\n",priv->pshare->rf_ft_var.txop_enlarge_upper);
#ifdef LOW_TP_TXOP
	panic_printk("BE_cwmax_enhance: %u,\n\n",priv->pshare->BE_cwmax_enhance);
#endif
	panic_printk("wifi_specific: %u,\n",priv->pmib->dot11OperationEntry.wifi_specific);

	PRINT_SINGL_ARG("	 VO drop: ", priv->pshare->phw->VO_droppkt_count, "%d");
	PRINT_SINGL_ARG("	 VI drop: ", priv->pshare->phw->VI_droppkt_count, "%d");
	PRINT_SINGL_ARG("	 BE drop: ", priv->pshare->phw->BE_droppkt_count, "%d");
	PRINT_SINGL_ARG("	 BK drop: ", priv->pshare->phw->BK_droppkt_count, "%d");


	return pos;
}


#ifdef IDLE_NOISE_LEVEL
static int rtl8192cd_proc_mib_noise_level(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;
	unsigned tmpstr[32];

	signed short noise_level=0;
	memset(tmpstr,0,32);
	noise_level=ODM_InbandNoise_Monitor(ODMPTR,TRUE,0x1E,10);
	sprintf(tmpstr,"[%d]",noise_level);
	//GDEBUG("noise_level=[%d]\n",noise_level);
	PRINT_SINGL_ARG("noise level:", tmpstr, "%s");

	return pos;
}
#endif
static int rtl8192cd_proc_mib_rf(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;
	int rf_path;
	unsigned char tmpbuf[16];

		rf_path = 2;

	PRINT_ONE("  Dot11RFEntry...", "%s", 1);
	PRINT_SINGL_ARG("    dot11channel: ", priv->pmib->dot11RFEntry.dot11channel, "%d");
	PRINT_SINGL_ARG("    dot11ch_low: ", priv->pmib->dot11RFEntry.dot11ch_low, "%d");
	PRINT_SINGL_ARG("    dot11ch_hi: ", priv->pmib->dot11RFEntry.dot11ch_hi, "%d");



//	if ((GET_CHIP_VER(priv)==VERSION_8192D) && (priv->pmib->dot11RFEntry.phyBandSelect & PHY_BAND_5G)) {
	if ((GET_CHIP_VER(priv)==VERSION_8192D)||(GET_CHIP_VER(priv)==VERSION_8812E)||(GET_CHIP_VER(priv)==VERSION_8881A)||(GET_CHIP_VER(priv)==VERSION_8814A)||(GET_CHIP_VER(priv)==VERSION_8822B)){
		PRINT_ARRAY_ARG("    pwrlevel5GHT40_1S_A: ", priv->pmib->dot11RFEntry.pwrlevel5GHT40_1S_A, "%02x", MAX_5G_CHANNEL_NUM);
		PRINT_ARRAY_ARG("    pwrlevel5GHT40_1S_B: ", priv->pmib->dot11RFEntry.pwrlevel5GHT40_1S_B, "%02x", MAX_5G_CHANNEL_NUM);
		if(GET_CHIP_VER(priv)!=VERSION_8812E && GET_CHIP_VER(priv) != VERSION_8814A) {
			PRINT_ARRAY_ARG("    pwrdiff5GHT40_2S: ", priv->pmib->dot11RFEntry.pwrdiff5GHT40_2S, "%02x", MAX_5G_CHANNEL_NUM);
			PRINT_ARRAY_ARG("    pwrdiff5GHT20: ", priv->pmib->dot11RFEntry.pwrdiff5GHT20, "%02x", MAX_5G_CHANNEL_NUM);
			PRINT_ARRAY_ARG("    pwrdiff5GOFDM: ", priv->pmib->dot11RFEntry.pwrdiff5GOFDM, "%02x", MAX_5G_CHANNEL_NUM);
		}
	}
	//} else
	{
		PRINT_ARRAY_ARG("    pwrlevelCCK_A: ", priv->pmib->dot11RFEntry.pwrlevelCCK_A, "%02x", MAX_2G_CHANNEL_NUM);
		PRINT_ARRAY_ARG("    pwrlevelCCK_B: ", priv->pmib->dot11RFEntry.pwrlevelCCK_B, "%02x", MAX_2G_CHANNEL_NUM);
		PRINT_ARRAY_ARG("    pwrlevelHT40_1S_A: ", priv->pmib->dot11RFEntry.pwrlevelHT40_1S_A, "%02x", MAX_2G_CHANNEL_NUM);
		PRINT_ARRAY_ARG("    pwrlevelHT40_1S_B: ", priv->pmib->dot11RFEntry.pwrlevelHT40_1S_B, "%02x", MAX_2G_CHANNEL_NUM);
		if(GET_CHIP_VER(priv)!=VERSION_8812E) {
			PRINT_ARRAY_ARG("    pwrdiffHT40_2S: ", priv->pmib->dot11RFEntry.pwrdiffHT40_2S, "%02x", MAX_2G_CHANNEL_NUM);
			PRINT_ARRAY_ARG("    pwrdiffHT20: ", priv->pmib->dot11RFEntry.pwrdiffHT20, "%02x", MAX_2G_CHANNEL_NUM);
			PRINT_ARRAY_ARG("    pwrdiffOFDM: ", priv->pmib->dot11RFEntry.pwrdiffOFDM, "%02x", MAX_2G_CHANNEL_NUM);
		}
	}

#if 0
		PRINT_ONE("  <Power by rate >", "%s", 1);
		PRINT_ARRAY_ARG("	 CCKTxAgc_A : ", (priv->pshare->phw->CCKTxAgc_A), "%d ", 4);
		PRINT_ARRAY_ARG("	 OFDMTxAgcOffset_A : ", (priv->pshare->phw->OFDMTxAgcOffset_A), "%d ", 8);
		PRINT_ARRAY_ARG("	 MCSTxAgcOffset_A : ", (priv->pshare->phw->MCSTxAgcOffset_A), "%d ", 16);
		PRINT_ARRAY_ARG("	 VHTTxAgcOffset_A : ", (priv->pshare->phw->VHTTxAgcOffset_A), "%d ", 20);

		PRINT_ARRAY_ARG("	 CCKTxAgc_B : ", (priv->pshare->phw->CCKTxAgc_B), "%d ", 4);
		PRINT_ARRAY_ARG("	 OFDMTxAgcOffset_B : ", (priv->pshare->phw->OFDMTxAgcOffset_B), "%d ", 8);
		PRINT_ARRAY_ARG("	 MCSTxAgcOffset_B : ", (priv->pshare->phw->MCSTxAgcOffset_B), "%d ", 16);
		PRINT_ARRAY_ARG("	 VHTTxAgcOffset_B : ", (priv->pshare->phw->VHTTxAgcOffset_B), "%d ", 20);
#endif

	PRINT_SINGL_ARG("    TxPowerRate : ", (priv->rtk->pwr_rate), "%d(percent)");

#ifdef TXPWR_LMT
	PRINT_SINGL_ARG("    disable_txpwrlmt : ", (priv->pshare->rf_ft_var.disable_txpwrlmt), "%d");
	PRINT_SINGL_ARG("    txpwr_lmt_index : ", (priv->pmib->dot11StationConfigEntry.txpwr_lmt_index), "%d");
	PRINT_SINGL_ARG("    regdomain : ", (priv->pmib->dot11StationConfigEntry.dot11RegDomain), "%d");
	PRINT_SINGL_ARG("    txpwr_lmt_CCK : ", (priv->pshare->txpwr_lmt_CCK), "%d");
	PRINT_SINGL_ARG("    txpwr_lmt_OFDM : ", (priv->pshare->txpwr_lmt_OFDM), "%d");
	PRINT_SINGL_ARG("    txpwr_lmt_HT1S : ", (priv->pshare->txpwr_lmt_HT1S), "%d");
	PRINT_SINGL_ARG("    txpwr_lmt_HT2S : ", (priv->pshare->txpwr_lmt_HT2S), "%d");
#if defined(TXPWR_LMT_8814A)
	if((GET_CHIP_VER(priv)== VERSION_8814A)){
		PRINT_SINGL_ARG("    txpwr_lmt_HT3S : ", (priv->pshare->txpwr_lmt_HT3S), "%d");
		PRINT_SINGL_ARG("    txpwr_lmt_HT4S : ", (priv->pshare->txpwr_lmt_HT4S), "%d");
	}
#endif
#ifdef TXPWR_LMT_NEWFILE
	if((GET_CHIP_VER(priv)== VERSION_8812E) ||
		(GET_CHIP_VER(priv)== VERSION_8188E) ||
		(IS_HAL_CHIP(priv))){
#if defined(TXPWR_LMT_8812) || defined(TXPWR_LMT_8881A) || defined(TXPWR_LMT_8814A) || defined(TXPWR_LMT_8822B)
			PRINT_SINGL_ARG("    txpwr_lmt_VHT1S : ", (priv->pshare->txpwr_lmt_VHT1S), "%d");
			PRINT_SINGL_ARG("    txpwr_lmt_VHT2S : ", (priv->pshare->txpwr_lmt_VHT2S), "%d");
#endif
#if defined(TXPWR_LMT_8814A)
			if((GET_CHIP_VER(priv)== VERSION_8814A)){
				PRINT_SINGL_ARG("    txpwr_lmt_VHT3S : ", (priv->pshare->txpwr_lmt_VHT3S), "%d");
				PRINT_SINGL_ARG("    txpwr_lmt_VHT4S : ", (priv->pshare->txpwr_lmt_VHT4S), "%d");
			}
#endif
#ifdef BEAMFORMING_AUTO
			if(priv->pshare->rf_ft_var.txbf_pwrlmt == TXBF_TXPWRLMT_AUTO) {
				PRINT_SINGL_ARG("	 txpwr_lmt_TXBF_HT1S : ", (priv->pshare->txpwr_lmt_TXBF_HT1S), "%d");
				PRINT_SINGL_ARG("	 txpwr_lmt_TXBF_HT2S : ", (priv->pshare->txpwr_lmt_TXBF_HT2S), "%d");
#if defined(TXPWR_LMT_8814A)
				if((GET_CHIP_VER(priv)== VERSION_8814A)){
					PRINT_SINGL_ARG("	 txpwr_lmt_TXBF_HT3S : ", (priv->pshare->txpwr_lmt_TXBF_HT3S), "%d");
					PRINT_SINGL_ARG("	 txpwr_lmt_TXBF_HT4S : ", (priv->pshare->txpwr_lmt_TXBF_HT4S), "%d");
				}
#endif
#if defined(TXPWR_LMT_8812) || defined(TXPWR_LMT_8881A) || defined(TXPWR_LMT_8814A)
				PRINT_SINGL_ARG("    txpwr_lmt_TXBF_VHT1S : ", (priv->pshare->txpwr_lmt_TXBF_VHT1S), "%d");
				PRINT_SINGL_ARG("    txpwr_lmt_TXBF_VHT2S : ", (priv->pshare->txpwr_lmt_TXBF_VHT2S), "%d");
#endif
#if defined(TXPWR_LMT_8814A)
				if((GET_CHIP_VER(priv)== VERSION_8814A)){
					PRINT_SINGL_ARG("    txpwr_lmt_TXBF_VHT3S : ", (priv->pshare->txpwr_lmt_TXBF_VHT3S), "%d");
					PRINT_SINGL_ARG("    txpwr_lmt_TXBF_VHT4S : ", (priv->pshare->txpwr_lmt_TXBF_VHT4S), "%d");
				}
#endif
			}
#endif // BEAMFORMING_AUTO

			PRINT_ARRAY_ARG("    tgpwr_CCK_new : ", (priv->pshare->tgpwr_CCK_new), "%d ", rf_path);
			PRINT_ARRAY_ARG("    tgpwr_OFDM_new : ", (priv->pshare->tgpwr_OFDM_new), "%d ", rf_path);
			PRINT_ARRAY_ARG("    tgpwr_HT1S_new : ", (priv->pshare->tgpwr_HT1S_new), "%d ", rf_path);
			PRINT_ARRAY_ARG("    tgpwr_HT2S_new : ", (priv->pshare->tgpwr_HT2S_new), "%d ", rf_path);
#if defined(TXPWR_LMT_8814A)
	if((GET_CHIP_VER(priv)== VERSION_8814A)){
			PRINT_ARRAY_ARG("    tgpwr_HT3S_new : ", (priv->pshare->tgpwr_HT3S_new), "%d ", rf_path);
			PRINT_ARRAY_ARG("    tgpwr_HT4S_new : ", (priv->pshare->tgpwr_HT4S_new), "%d ", rf_path);
	}
#endif

#if defined(TXPWR_LMT_8812) || defined(TXPWR_LMT_8881A) || defined(TXPWR_LMT_8814A) || defined(TXPWR_LMT_8822B)
			PRINT_ARRAY_ARG("    tgpwr_VHT1S_new : ", (priv->pshare->tgpwr_VHT1S_new), "%d ", rf_path);
			PRINT_ARRAY_ARG("    tgpwr_VHT2S_new : ", (priv->pshare->tgpwr_VHT2S_new), "%d ", rf_path);
#endif
#if defined(TXPWR_LMT_8814A)
	if((GET_CHIP_VER(priv)== VERSION_8814A)){
			PRINT_ARRAY_ARG("    tgpwr_VHT3S_new : ", (priv->pshare->tgpwr_VHT3S_new), "%d ", rf_path);
			PRINT_ARRAY_ARG("    tgpwr_VHT4S_new : ", (priv->pshare->tgpwr_VHT4S_new), "%d ", rf_path);
	}
#endif

	} else
#endif
	{
	PRINT_SINGL_ARG("    target_CCK : ", (priv->pshare->tgpwr_CCK), "%d");
	PRINT_SINGL_ARG("    target_OFDM : ", (priv->pshare->tgpwr_OFDM), "%d");
	PRINT_SINGL_ARG("    target_HT1S : ", (priv->pshare->tgpwr_HT1S), "%d");
	PRINT_SINGL_ARG("    target_HT2S : ", (priv->pshare->tgpwr_HT2S), "%d");
	}
#endif
#ifdef POWER_PERCENT_ADJUSTMENT
	PRINT_SINGL_ARG("    power_percent: ", priv->pmib->dot11RFEntry.power_percent, "%d");
#endif
	PRINT_SINGL_ARG("    shortpreamble: ", priv->pmib->dot11RFEntry.shortpreamble, "%d");
	PRINT_SINGL_ARG("    trswitch: ", priv->pmib->dot11RFEntry.trswitch, "%d");
	PRINT_SINGL_ARG("    disable_ch14_ofdm: ", priv->pmib->dot11RFEntry.disable_ch14_ofdm, "%d");
	PRINT_SINGL_ARG("    xcap: ", priv->pmib->dot11RFEntry.xcap, "%d");
	PRINT_SINGL_ARG("    share_xcap: ", priv->pmib->dot11RFEntry.share_xcap, "%d");
	PRINT_SINGL_ARG("    tssi1: ", priv->pmib->dot11RFEntry.tssi1, "%d");
	PRINT_SINGL_ARG("    tssi2: ", priv->pmib->dot11RFEntry.tssi2, "%d");
	PRINT_SINGL_ARG("    ther: ", priv->pmib->dot11RFEntry.ther, "%d");
	if(netif_running(dev))
	{
		PRINT_SINGL_ARG("    current thermal: ", PHY_QueryRFReg(priv, RF_PATH_A, 0x42, 0xfc00, 1), "%d");
	}
	else
	{
		PRINT_SINGL_ARG("    current thermal: ", 0, "%d");
	}
#ifdef THER_TRIM
	PRINT_SINGL_ARG("    ther_trim_enable: ", priv->pshare->rf_ft_var.ther_trim_enable, "%d");
	PRINT_SINGL_ARG("    ther_trim_val: ", priv->pshare->rf_ft_var.ther_trim_val, "%d");
#endif
	switch (priv->pshare->phw->MIMO_TR_hw_support) {
	case MIMO_1T2R:
		sprintf(tmpbuf, "1T2R");
		break;
	case MIMO_1T1R:
		sprintf(tmpbuf, "1T1R");
		break;
	case MIMO_2T2R:
		sprintf(tmpbuf, "2T2R");
		break;
	case MIMO_3T3R:
		sprintf(tmpbuf, "3T3R");
		break;
	case MIMO_4T4R:
		sprintf(tmpbuf, "4T4R");
		break;
	default:
		sprintf(tmpbuf, "2T4R");
		break;
	}
	PRINT_SINGL_ARG("    MIMO_TR_hw_support: ", tmpbuf, "%s");

	switch (priv->pmib->dot11RFEntry.MIMO_TR_mode) {
	case MIMO_1T2R:
		sprintf(tmpbuf, "1T2R");
		break;
	case MIMO_1T1R:
		sprintf(tmpbuf, "1T1R");
		break;
	case MIMO_2T2R:
		sprintf(tmpbuf, "2T2R");
		break;
	case MIMO_3T3R:
		sprintf(tmpbuf, "3T3R");
		break;
	case MIMO_4T4R:
		sprintf(tmpbuf, "4T4R");
		break;
	default:
		sprintf(tmpbuf, "2T4R");
		break;
	}
	PRINT_SINGL_ARG("    MIMO_TR_mode: ", tmpbuf, "%s");
	PRINT_SINGL_ARG("    TXPowerOffset:     ", priv->pshare->phw->TXPowerOffset, "%u");
#ifdef RF_MIMO_SWITCH
	PRINT_SINGL_ARG("    RF status: ", priv->pshare->rf_status, "%d");
#endif





        if (GET_CHIP_VER(priv) == VERSION_8822B) {
            if (_GET_HAL_DATA(priv)->bTestChip)
    		{
    		    sprintf(tmpbuf, "RTL8822B Test");
    		} else {
    			sprintf(tmpbuf, "RTL8822B MP");
    		}
        }

    if (GET_CHIP_VER(priv) == VERSION_8197F) {
        if (_GET_HAL_DATA(priv)->bTestChip)
        {
            sprintf(tmpbuf, "RTL8197F Test");
        } else {
            sprintf(tmpbuf, "RTL8197F");
            extern unsigned int rtl819x_bond_option(void);
            if(rtl819x_bond_option() == 1)
                strcat(tmpbuf, "B");
            else if(rtl819x_bond_option() == 2)
                strcat(tmpbuf, "N");
            else if(rtl819x_bond_option() == 3)
                strcat(tmpbuf, "S");
            strcat(tmpbuf, " MP");

            if(_GET_HAL_DATA(priv)->cutVersion == ODM_CUT_A)
                strcat(tmpbuf, " A");
            else if(_GET_HAL_DATA(priv)->cutVersion == ODM_CUT_B)
                strcat(tmpbuf, " B");
            else if(_GET_HAL_DATA(priv)->cutVersion == ODM_CUT_C)
                strcat(tmpbuf, " C");
            else if(_GET_HAL_DATA(priv)->cutVersion == ODM_CUT_D)
                strcat(tmpbuf, " D");
            else
                strcat(tmpbuf, " O");//others
        }
    }

	if(IS_UMC_A_CUT(priv))
		strcat(tmpbuf, "u");

	PRINT_SINGL_ARG("    chipVersion: ", tmpbuf, "%s");

#ifdef POWER_TRIM
	PRINT_SINGL_ARG("    kfree_enable:", priv->pmib->dot11RFEntry.kfree_enable, "%d");
	PRINT_SINGL_ARG("    kfree_value: 0x", priv->pshare->kfree_value, "%x");
#endif
#if defined (HW_ANT_SWITCH) && !defined(CONFIG_RTL_92C_SUPPORT) && !defined(CONFIG_RTL_92D_SUPPORT)
	PRINT_SINGL_ARG("    Antdiv_Type : ", ((priv->pshare->_dmODM.AntDivType==CGCS_RX_HW_ANTDIV) ? "RX_ANTDIV" : ((priv->pshare->_dmODM.AntDivType==CG_TRX_HW_ANTDIV)?"TRX_ANTDIV":"NONE")), "%s");
	PRINT_SINGL_ARG("    Antdiv switch : ", ((priv->pshare->_dmODM.antdiv_select==0) ? "Auto" : ((priv->pshare->_dmODM.antdiv_select==1)?"A1":"A2")), "%s");
#endif

#ifdef SW_ANT_SWITCH
	PRINT_SINGL_ARG("    SW Ant switch enable: ", (SW_DIV_ENABLE ? "enable" : "disable"), "%s");
	PRINT_SINGL_ARG("    SW Diversity Antenna : ", priv->pshare->DM_SWAT_Table.CurAntenna, "%d");
#endif

	PRINT_SINGL_ARG("    tx2path: ", priv->pmib->dot11RFEntry.tx2path, "%d");
	PRINT_SINGL_ARG("    txbf: ", priv->pmib->dot11RFEntry.txbf, "%d");

#ifdef BEAMFORMING_AUTO
	if(priv->pshare->rf_ft_var.txbf_pwrlmt == TXBF_TXPWRLMT_AUTO) {
		PRINT_SINGL_ARG("    txbferVHT2TX: ", priv->pshare->txbferVHT2TX, "%d");
		PRINT_SINGL_ARG("    txbferHT2TX: ", priv->pshare->txbferHT2TX, "%d");
		PRINT_SINGL_ARG("    txbf2TXbackoff: ", priv->pshare->txbf2TXbackoff, "%d");
	}
#endif

#ifdef RTL8192D_INT_PA
	PRINT_SINGL_ARG("    use_intpa92d: ", priv->pshare->rf_ft_var.use_intpa92d, "%d");
#endif
#ifdef	HIGH_POWER_EXT_PA
	PRINT_SINGL_ARG("    use_ext_pa: ", priv->pshare->rf_ft_var.use_ext_pa, "%d");
#endif
#ifdef HIGH_POWER_EXT_LNA
	PRINT_SINGL_ARG("    use_ext_lna: ", priv->pshare->rf_ft_var.use_ext_lna, "%d");
#endif
	PRINT_SINGL_ARG("    rfe_type: ", priv->pmib->dot11RFEntry.rfe_type, "%u");
	PRINT_SINGL_ARG("    pa_type: ", priv->pmib->dot11RFEntry.pa_type, "%d");
	PRINT_SINGL_ARG("    acs_type: ", priv->pmib->dot11RFEntry.acs_type, "%d");

#if	(defined(CONFIG_SLOT_0_8192EE) && defined(CONFIG_SLOT_0_EXT_LNA))||(defined(CONFIG_SLOT_1_8192EE) && defined(CONFIG_SLOT_1_EXT_LNA))
	PRINT_SINGL_ARG("    lna_type: ", priv->pshare->rf_ft_var.lna_type, "%d");
#endif

#ifdef CONFIG_8881A_HP
	PRINT_SINGL_ARG("    hp_8881a: ", priv->pshare->rf_ft_var.hp_8881a, "%u");
#endif

#ifdef CONFIG_8881A_2LAYER
	PRINT_SINGL_ARG("    use_8881a_2layer: ", priv->pshare->rf_ft_var.use_8881a_2layer, "%u");
#endif

	PRINT_SINGL_ARG("    txpwr_reduction: ", priv->pmib->dot11RFEntry.txpwr_reduction, "%d");
	return pos;
}

#ifdef THERMAL_CONTROL
static int rtl8192cd_proc_thermal_control(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	struct list_head *phead, *plist;
	struct stat_info *pstat;
	unsigned long flags=0;
	int num=1;

	int pos = 0;
	int rf_path;
	unsigned char tmpbuf[16];
	PRINT_ONE("[THERMAL CONTROL]", "%s", 1);
	PRINT_SINGL_ARG("    calibrated ther: ", priv->pmib->dot11RFEntry.ther, "%d");
	PRINT_SINGL_ARG("    current thermal: ", PHY_QueryRFReg(priv, RF_PATH_A, 0x42, 0xfc00, 1), "%d");
	PRINT_SINGL_ARG("	 del_ther: ", priv->pshare->rf_ft_var.del_ther, "%d");
	PRINT_SINGL_ARG("	 ther_hi : ", priv->pshare->rf_ft_var.ther_hi, "%u");
	PRINT_SINGL_ARG("	 ther_low : ", priv->pshare->rf_ft_var.ther_low, "%u");
	PRINT_SINGL_ARG("	 man: ", priv->pshare->rf_ft_var.man, "%d");
	PRINT_SINGL_ARG("	 ther_dm: ", priv->pshare->rf_ft_var.ther_dm, "%d");
	PRINT_SINGL_ARG("	 ther_dm_period: ", priv->pshare->rf_ft_var.ther_dm_period, "%d");
	PRINT_ONE("[STATE]", "%s", 1);
	PRINT_SINGL_ARG("	 state: ", priv->pshare->rf_ft_var.state==0?"Initial State":"Thermal Control State", "%s");
	PRINT_SINGL_ARG("	 monitor_time: ", priv->pshare->rf_ft_var.monitor_time, "%d");
	PRINT_SINGL_ARG("	 countdown: ", priv->pshare->rf_ft_var.countdown, "%d");
	PRINT_SINGL_ARG("	 ther_drop: ", priv->pshare->rf_ft_var.ther_drop, "%d");
	PRINT_ONE("[Tx Power]", "%s", 1);
	PRINT_SINGL_ARG("	 low_power : ", priv->pshare->rf_ft_var.low_power, "%u");
	PRINT_SINGL_ARG("	 power_desc : ", priv->pshare->rf_ft_var.power_desc, "%u");
	PRINT_SINGL_ARG("	 current_power_desc : ", priv->pshare->rf_ft_var.current_power_desc, "%u");
	PRINT_SINGL_ARG("	 current power: ", (priv->pshare->rf_ft_var.low_power==1)?"dynamically per STA":((priv->pshare->rf_ft_var.low_power==2)?((priv->pshare->rf_ft_var.current_power_desc==3)?"-11dB":((priv->pshare->rf_ft_var.current_power_desc==2)?"-7dB":((priv->pshare->rf_ft_var.current_power_desc==1)?"-3dB":((priv->pshare->rf_ft_var.current_power_desc==0)?"0dB":"unknown")))):(priv->pshare->rf_ft_var.low_power==0)?"diable power degradation":"unknown"), "%s");
	PRINT_ONE("	 [Note] low_power =0: as default, 1: dynamic, 2: decided by power_desc", "%s", 1);

	PRINT_ONE("[Tx Path]", "%s", 1);
	PRINT_SINGL_ARG("	 path: ", (priv->pshare->rf_ft_var.path==1)?"1T":((priv->pshare->rf_ft_var.path==2)?"2T":((priv->pshare->rf_ft_var.path==0)?"tx2path":"unknown")), "%s");
	PRINT_SINGL_ARG("	 path : ", priv->pshare->rf_ft_var.path, "%u");
	PRINT_SINGL_ARG("	 current_path: ", priv->pshare->rf_ft_var.current_path, "%u");
	PRINT_SINGL_ARG("	 path_select: ", priv->pshare->rf_ft_var.path_select, "%u");
	PRINT_SINGL_ARG("	 chosen_path: ", priv->pshare->rf_ft_var.chosen_path, "%u");
	PRINT_ONE("	 [Note] path = 0:as default, 1: 1T, 2: 2T", "%s", 1);

	PRINT_ONE("[Tx Duty Cycle]", "%s", 1);
	PRINT_SINGL_ARG("	 txduty: ", priv->pshare->rf_ft_var.txduty, "%u");
	PRINT_SINGL_ARG("	 txduty_level: ", priv->pshare->rf_ft_var.txduty_level, "%u");
	PRINT_SINGL_ARG("	 limit percent (pa): ", priv->pshare->rf_ft_var.pa, "%u");
	PRINT_SINGL_ARG("	 limit_90pa: ", priv->pshare->rf_ft_var.limit_90pa, "%u");
	PRINT_SINGL_ARG("	 sta_bwc_to: ", priv->pshare->rf_ft_var.sta_bwc_to, "%u");

	SAVE_INT_AND_CLI(flags);
	SMP_LOCK_ASOC_LIST(flags);

	PRINT_SINGL_ARG("[STA Info] -- active: ", priv->assoc_num, "%d");

	phead = &priv->asoc_list;
	if (!netif_running(dev) || list_empty(phead)) {
		goto _ret;
	}

	plist = phead->next;
	while (plist != phead) {
		pstat = list_entry(plist, struct stat_info, asoc_list);

#ifdef THERMAL_CONTROL
		PRINT_ONE(num,	" %d: stat_info...", 1);

		PRINT_SINGL_ARG("	  power: ", ((pstat->power==3)?"-11dB":((pstat->power==2)?"-7dB":((pstat->power==1)?"-3dB":((pstat->power==0)?"0dB":"unknown")))), "%s");

		PRINT_SINGL_ARG("	  got_limit_tp: ", pstat->got_limit_tp, "%u");
		PRINT_SINGL_ARG("	  tx_tp_base (Kbps): ", pstat->tx_tp_base , "%d");
		PRINT_SINGL_ARG("	  tx_tp_limit (Kbps): ", pstat->tx_tp_limit , "%d");
		PRINT_SINGL_ARG("	  bwcthrd_tx (Kbps): ", pstat->sta_bwcthrd_tx , "%d");
		PRINT_SINGL_ARG("	  tx_tp_base (Mbps): ", (pstat->tx_tp_base)/1024 , "%d");
		PRINT_SINGL_ARG("	  tx_tp_limit (Mbps): ", (pstat->tx_tp_limit)/1024 , "%d");
#ifdef RTK_STA_BWC
		PRINT_SINGL_ARG("	  bwcthrd_tx (Mbps): ", (pstat->sta_bwcthrd_tx)/1024 , "%d");
		PRINT_SINGL_ARG("	  bwc_drop_cnt : ", pstat->sta_bwcdrop_cnt , "%d");
#endif
#endif
		CHECK_LEN;
		plist = plist->next;
		num++;
	}

_ret:
	SMP_UNLOCK_ASOC_LIST(flags);
	RESTORE_INT(flags);


	return pos;
}
#endif

static int rtl8192cd_proc_mib_bssdesc(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;
	unsigned char tmpbuf[33];

	PRINT_ONE("  bss_desc...", "%s", 1);
	PRINT_ARRAY_ARG("    bssid: ", priv->pmib->dot11Bss.bssid, "%02x", MACADDRLEN);

	memcpy(tmpbuf, priv->pmib->dot11Bss.ssid, priv->pmib->dot11Bss.ssidlen);
	tmpbuf[priv->pmib->dot11Bss.ssidlen] = '\0';
	PRINT_SINGL_ARG("    ssid: ", tmpbuf, "%s");

	PRINT_SINGL_ARG("    ssidlen: ", priv->pmib->dot11Bss.ssidlen, "%d");
	PRINT_SINGL_ARG("    bsstype: ", priv->pmib->dot11Bss.bsstype, "%x");
	PRINT_SINGL_ARG("    beacon_prd: ", priv->pmib->dot11Bss.beacon_prd, "%d");
	PRINT_SINGL_ARG("    dtim_prd: ", priv->pmib->dot11Bss.dtim_prd, "%d");
#ifdef CLIENT_MODE
	if (OPMODE & WIFI_STATION_STATE)
		PRINT_SINGL_ARG("    client mode aid: ", _AID, "%d");
#endif
	PRINT_ARRAY_ARG("    t_stamp(hex): ", priv->pmib->dot11Bss.t_stamp, "%08x", 2);
	PRINT_SINGL_ARG("    ibss_par.atim_win: ", priv->pmib->dot11Bss.ibss_par.atim_win, "%d");
	PRINT_SINGL_ARG("    capability(hex): ", priv->pmib->dot11Bss.capability, "%02x");
	PRINT_SINGL_ARG("    channel: ", priv->pmib->dot11Bss.channel, "%d");
	PRINT_SINGL_ARG("    basicrate(hex): ", priv->pmib->dot11Bss.basicrate, "%x");
	PRINT_SINGL_ARG("    supportrate(hex): ", priv->pmib->dot11Bss.supportrate, "%x");
	PRINT_ARRAY_ARG("    bdsa: ", priv->pmib->dot11Bss.bdsa, "%02x", MACADDRLEN);
	PRINT_SINGL_ARG("    rssi: ", priv->pmib->dot11Bss.rssi, "%d");
	PRINT_SINGL_ARG("    sq: ", priv->pmib->dot11Bss.sq, "%d");

	return pos;
}
#ifdef CONFIG_RTL_WLAN_DIAGNOSTIC
static int rtl8192cd_proc_diagnostic(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;
	PRINT_ONE(diag_log_buff, "%s", 1);
	return pos;
}
#endif
static int rtl8192cd_proc_mib_erp(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;

	PRINT_ONE("  ERP info...", "%s", 1);
	PRINT_SINGL_ARG("    protection: ", priv->pmib->dot11ErpInfo.protection, "%d");
	PRINT_SINGL_ARG("    nonErpStaNum: ", priv->pmib->dot11ErpInfo.nonErpStaNum, "%d");
	PRINT_SINGL_ARG("    olbcDetected: ", priv->pmib->dot11ErpInfo.olbcDetected, "%d");
	PRINT_SINGL_ARG("    olbcExpired: ", priv->pmib->dot11ErpInfo.olbcExpired, "%d");
	PRINT_SINGL_ARG("    shortSlot: ", priv->pmib->dot11ErpInfo.shortSlot, "%d");
	PRINT_SINGL_ARG("    ctsToSelf: ", priv->pmib->dot11ErpInfo.ctsToSelf, "%d");
	PRINT_SINGL_ARG("    longPreambleStaNum: ", priv->pmib->dot11ErpInfo.longPreambleStaNum, "%d");

	return pos;
}

static int rtl8192cd_proc_cam_info(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0, i;
	unsigned char TempOutputMac[6];
	unsigned char TempOutputKey[16];
	unsigned short TempOutputCfg=0;

	if (!(priv->drv_state & DRV_STATE_OPEN))
		return 0;

	PRINT_ONE("  CAM info...", "%s", 1);
	PRINT_ONE("    CAM occupied: ", "%s", 0);
	PRINT_ONE(priv->pshare->CamEntryOccupied, "%d", 1);
	for (i=0; i < priv->pshare->total_cam_entry; i++)
	{
		PRINT_ONE("    Entry", "%s", 0);
		PRINT_ONE(i, " %2d:", 0);
		CAM_read_entry(priv,i,TempOutputMac,TempOutputKey,&TempOutputCfg);
		PRINT_ARRAY_ARG(" MAC addr: ", TempOutputMac, "%02x", 6);
		PRINT_SINGL_ARG("              Config: ", TempOutputCfg, "%x");
		PRINT_ARRAY_ARG("              Key: ", TempOutputKey, "%02x", 16);
	}

	return pos;
}

static int rtl8192cd_proc_probe_info(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
    struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);

	int pos = 0, i;

	PRINT_ONE("  Probe request info...", "%s", 1);
	PRINT_ONE("    Entry occupied:", "%s", 0);
	PRINT_ONE(priv->ProbeReqEntryOccupied, "%d", 1);

	for (i=0; i<priv->ProbeReqEntryOccupied; i++)
	{
		PRINT_ONE("    Entry", "%s", 0);
		PRINT_ONE(priv->probe_sta[i].Entry, " %2d:", 1);
		PRINT_ARRAY_ARG("	    MAC addr: ", priv->probe_sta[i].addr, "%02x", MACADDRLEN);
		PRINT_SINGL_ARG("	    Signal strength: ",100-priv->probe_sta[i].rssi, "-%2d dB");
	}
	for (i=0; i<MAX_PROBE_REQ_STA; i++)
	{
		priv->probe_sta[i].used = 0;
			priv->ProbeReqEntryOccupied = 0;
	}
	return pos;
}

#ifdef STA_ASSOC_STATISTIC
int rtl8192cd_proc_reject_assoc_info(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
    struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0, i;

	PRINT_ONE("  Reject STA info...", "%s", 1);
	PRINT_ONE("  	Entry occupied:", "%s", 0);
	PRINT_ONE(priv->RejectAssocEntryOccupied, "%d", 1);

	for (i=0; i<priv->RejectAssocEntryOccupied; i++)
		{
			PRINT_ONE("    Entry", "%s", 0);
			PRINT_ONE(priv->reject_sta[i].Entry, " %2d:", 1);
			PRINT_ARRAY_ARG("	    MAC addr: ", priv->reject_sta[i].addr, "%02x", MACADDRLEN);
			PRINT_SINGL_ARG("	    RSSI: ", priv->reject_sta[i].rssi, "%02d");
		}
	for (i=0; i<MAX_PROBE_REQ_STA; i++)
		{
			priv->reject_sta[i].used = 0;
			priv->RejectAssocEntryOccupied = 0;
		}
	return pos;
}

int rtl8192cd_proc_rm_assoc_info(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
    struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0, i;

	PRINT_ONE("  Removed STA info...", "%s", 1);
	PRINT_ONE("  	Entry occupied:", "%s", 0);
	PRINT_ONE(priv->RemoveAssocEntryOccupied, "%d", 1);

	for (i=0; i<priv->RemoveAssocEntryOccupied; i++)
		{
			PRINT_ONE("    Entry", "%s", 0);
			PRINT_ONE(priv->removed_sta[i].Entry, " %2d:", 1);
			PRINT_ARRAY_ARG("	    MAC addr: ", priv->removed_sta[i].addr, "%02x", MACADDRLEN);
			PRINT_SINGL_ARG("	    RSSI: ",priv->removed_sta[i].rssi, "%02d");
		}
	for (i=0; i<MAX_PROBE_REQ_STA; i++)
		{
			priv->removed_sta[i].used = 0;
			priv->RemoveAssocEntryOccupied = 0;
		}
	return pos;
}


int rtl8192cd_proc_assoc_status_info(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
    struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0, i;

	PRINT_ONE("  STA assoc status...", "%s", 1);
	PRINT_ONE("  	Entry occupied:", "%s", 0);
	PRINT_ONE(priv->AssocStatusEntryOccupied, "%d", 1);

	for (i=0; i<priv->AssocStatusEntryOccupied; i++)
	{
		PRINT_ONE("    Entry", "%s", 0);
		PRINT_ONE(priv->assoc_sta[i].Entry, " %2d:", 1);
		PRINT_ARRAY_ARG("	    MAC addr: ", priv->assoc_sta[i].addr, "%02x", MACADDRLEN);
		PRINT_SINGL_ARG("	    RSSI: ", priv->assoc_sta[i].rssi, "%02d");
		PRINT_SINGL_ARG("	    status: ", priv->assoc_sta[i].status, "%02d");
	}
	for (i=0; i<MAX_PROBE_REQ_STA; i++)
	{
			priv->assoc_sta[i].used = 0;
			priv->AssocStatusEntryOccupied = 0;
	}
	return pos;
}
#endif

#ifdef PROC_STA_CONN_FAIL_INFO
static int rtl8192cd_proc_sta_conn_fail(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
    struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);

	int pos = 0, i;

	PRINT_ONE("  STA conn fail info...", "%s", 1);

	for (i=0; i<64; i++) {
		if (priv->sta_conn_fail[i].used) {
			PRINT_ARRAY_ARG("	    MAC addr: ", priv->sta_conn_fail[i].addr, "%02x", MACADDRLEN);
			PRINT_SINGL_ARG("	    Error state: ", priv->sta_conn_fail[i].error_state, "%d");
		}
	}
	memset(priv->sta_conn_fail, 0, sizeof(struct sta_conn_fail_info) * 64);

	return pos;
}
#endif

#ifdef STA_RATE_STATISTIC
const u2Byte CCK_OFDM[12] = {1, 2, 5, 11, /*CCK*/
							6, 9, 12, 18, 24, 36, 48, 54/*OFDM*/};
static int rtl8192cd_proc_sta_rateinfo(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int len = 0, rc=1, j;
	int size, num=0;
	struct list_head *phead, *plist;
	struct stat_info *pstat;

	unsigned long flags=0;


	SAVE_INT_AND_CLI(flags);
    SMP_LOCK_ASOC_LIST(flags);

	phead = &priv->asoc_list;
    if (!(priv->drv_state & DRV_STATE_OPEN) || list_empty(phead)) {
        goto _ret;
    }

	PRINT_ONE("STA Rate Info...", "%s", 1);

	plist = phead->next;
	while (plist != phead) {
		pstat = list_entry(plist, struct stat_info, asoc_list);
		PRINT_ARRAY_ARG("  STA: ", pstat->hwaddr, "%02x", MACADDRLEN);

		PRINT_ONE("  Tx Data Rate ", "%s", 1);
		for(j=0; j<STA_RATE_NUM; j++){
			if(pstat->txrate_stat[j]!=0){
				if(j<CCK_OFDM_RATE_NUM){//CCK:0~3, OFDM:4~11
					if(j<4){
						PRINT_ONE("    CCK ", "%s", 0);
						PRINT_ONE(CCK_OFDM[j], "%d: ", 0);
						PRINT_ONE(pstat->txrate_stat[j], "%d", 1);
					}else{
						PRINT_ONE("    OFDM ", "%s", 0);
						PRINT_ONE(CCK_OFDM[j], "%d: ", 0);
						PRINT_ONE(pstat->txrate_stat[j], "%d", 1);
					}
				}else if(j>=CCK_OFDM_RATE_NUM && j<(CCK_OFDM_RATE_NUM+HT_RATE_NUM)){//MCS0~15 MCS16~31
					PRINT_ONE("    HT MCS", "%s", 0);
					PRINT_ONE((j-CCK_OFDM_RATE_NUM), "%d: ", 0);
					PRINT_ONE(pstat->txrate_stat[j], "%d", 1);
				}else{//NSS1 MCS0~9, NSS2 MCS0~9, NSS3 MCS0~9
					PRINT_ONE("    VHT NSS", "%s", 0);
					PRINT_ONE((j-(CCK_OFDM_RATE_NUM+HT_RATE_NUM))/10+1, "%d MCS", 0);
					PRINT_ONE((j-(CCK_OFDM_RATE_NUM+HT_RATE_NUM))%10, "%d: ", 0);
					PRINT_ONE(pstat->txrate_stat[j], "%d", 1);
				}
			}
		}

		PRINT_ONE("  Rx Data Rate ", "%s", 1);
		for(j=0; j<STA_RATE_NUM; j++){
			if(pstat->rxrate_stat[j]!=0){
				if(j<CCK_OFDM_RATE_NUM){//CCK:0~3, OFDM:4~11
					if(j<4){
						PRINT_ONE("    CCK ", "%s", 0);
						PRINT_ONE(CCK_OFDM[j], "%d: ", 0);
						PRINT_ONE(pstat->rxrate_stat[j], "%d", 1);
					}else{
						PRINT_ONE("    OFDM ", "%s", 0);
						PRINT_ONE(CCK_OFDM[j], "%d: ", 0);
						PRINT_ONE(pstat->rxrate_stat[j], "%d", 1);
					}
				}else if(j>=CCK_OFDM_RATE_NUM && j<(CCK_OFDM_RATE_NUM+HT_RATE_NUM)){//MCS0~15 MCS16~31
					PRINT_ONE("    HT MCS", "%s", 0);
					PRINT_ONE((j-CCK_OFDM_RATE_NUM), "%d: ", 0);
					PRINT_ONE(pstat->rxrate_stat[j], "%d", 1);
				}else{//NSS1 MCS0~9, NSS2 MCS0~9, NSS3 MCS0~9
					PRINT_ONE("    VHT NSS", "%s", 0);
					PRINT_ONE((j-(CCK_OFDM_RATE_NUM+HT_RATE_NUM))/10+1, "%d MCS", 0);
					PRINT_ONE((j-(CCK_OFDM_RATE_NUM+HT_RATE_NUM))%10, "%d: ", 0);
					PRINT_ONE(pstat->rxrate_stat[j], "%d", 1);
				}
			}
		}

		memset(pstat->txrate_stat, 0x0, sizeof(pstat->txrate_stat));
		memset(pstat->rxrate_stat, 0x0, sizeof(pstat->rxrate_stat));

		num++;
		plist = plist->next;
	}

_ret:

	SMP_UNLOCK_ASOC_LIST(flags);
	RESTORE_INT(flags);

	return len;
}
#endif


#ifdef HS2_SUPPORT
static int rtl8192cd_proc_mib_hs2(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0, i;

	PRINT_ONE("  HS2 info...", "%s", 1);
	PRINT_SINGL_ARG("    hs_enable: ", priv->pmib->hs2Entry.hs_enable, "%d");

	PRINT_ARRAY_ARG("    hs2_ie: ",
			priv->pmib->hs2Entry.hs2_ie, "%02x", priv->pmib->hs2Entry.hs2_ielen);
	PRINT_SINGL_ARG("    hs2_ielen: ", priv->pmib->hs2Entry.hs2_ielen, "%d");

	PRINT_SINGL_ARG("    proxy arp:	", priv->proxy_arp, "%d");
	PRINT_SINGL_ARG("    dgaf_disable:	",priv->dgaf_disable, "%d");
	PRINT_SINGL_ARG("    OSU_Present:	",priv->OSU_Present, "%d");

	PRINT_ARRAY_ARG("    interworking_ie: ",
			priv->pmib->hs2Entry.interworking_ie, "%02x", priv->pmib->hs2Entry.interworking_ielen);
	PRINT_SINGL_ARG("    interworking_ielen: ", priv->pmib->hs2Entry.interworking_ielen, "%d");

	PRINT_ARRAY_ARG("    QoSMap_ie[0]: ",
			priv->pmib->hs2Entry.QoSMap_ie[0], "%02x", priv->pmib->hs2Entry.QoSMap_ielen[0]);
	PRINT_SINGL_ARG("    QoSMap_ielen[0]: ", priv->pmib->hs2Entry.QoSMap_ielen[0], "%d");
	PRINT_ARRAY_ARG("    QoSMap_ie[1]: ",
			priv->pmib->hs2Entry.QoSMap_ie[1], "%02x", priv->pmib->hs2Entry.QoSMap_ielen[1]);
	PRINT_SINGL_ARG("    QoSMap_ielen[1]: ", priv->pmib->hs2Entry.QoSMap_ielen[1], "%d");

	PRINT_ARRAY_ARG("    advt_proto_ie: ",
			priv->pmib->hs2Entry.advt_proto_ie, "%02x", priv->pmib->hs2Entry.advt_proto_ielen);
	PRINT_SINGL_ARG("    advt_proto_ielen: ", priv->pmib->hs2Entry.advt_proto_ielen, "%d");

	PRINT_ARRAY_ARG("    roam_ie: ",
			priv->pmib->hs2Entry.roam_ie, "%02x", priv->pmib->hs2Entry.roam_ielen);
	PRINT_SINGL_ARG("    roam_ielen: ", priv->pmib->hs2Entry.roam_ielen, "%d");

	PRINT_ARRAY_ARG("    timeadvt_ie: ",
			priv->pmib->hs2Entry.timeadvt_ie, "%02x", priv->pmib->hs2Entry.timeadvt_ielen);
	PRINT_SINGL_ARG("    timeadvt_ielen: ", priv->pmib->hs2Entry.timeadvt_ielen, "%d");

	PRINT_ARRAY_ARG("    timezone_ie: ",
			priv->pmib->hs2Entry.timezone_ie, "%02x", priv->pmib->hs2Entry.timezone_ielen);
	PRINT_SINGL_ARG("    timezone_ielen: ", priv->pmib->hs2Entry.timezone_ielen, "%d");

	PRINT_ARRAY_ARG("    MBSSID_ie: ",
			priv->pmib->hs2Entry.MBSSID_ie, "%02x", priv->pmib->hs2Entry.MBSSID_ielen);
	PRINT_SINGL_ARG("    MBSSID_ielen: ", priv->pmib->hs2Entry.MBSSID_ielen, "%d");

	PRINT_ARRAY_ARG("    MBSSID_ie: ",
			priv->pmib->hs2Entry.MBSSID_ie, "%02x", priv->pmib->hs2Entry.MBSSID_ielen);
	PRINT_SINGL_ARG("    MBSSID_ielen: ", priv->pmib->hs2Entry.MBSSID_ielen, "%d");

	PRINT_SINGL_ARG("    remedSvrURL: ", priv->pmib->hs2Entry.remedSvrURL, "%s");
	PRINT_SINGL_ARG("    SessionInfoURL: ", priv->pmib->hs2Entry.SessionInfoURL, "%s");

	PRINT_ARRAY_ARG("    bssload_ie: ",
			priv->pmib->hs2Entry.bssload_ie, "%02x", 5);
	PRINT_SINGL_ARG("    ICMPv4ECHO: ", priv->pmib->hs2Entry.ICMPv4ECHO, "%d");
	PRINT_SINGL_ARG("    block_relay: ", priv->pmib->dot11OperationEntry.block_relay, "%d");

	PRINT_SINGL_ARG("    80211W: ", priv->pmib->dot1180211AuthEntry.dot11IEEE80211W, "%d");
	PRINT_SINGL_ARG("    SHA256: ", priv->pmib->dot1180211AuthEntry.dot11EnableSHA256, "%d");

	return pos;
}
#endif // HS2_SUPPORT




#ifdef CONFIG_IEEE80211V
int rtl8192cd_proc_transition_read(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;
	int i,j;
	struct stat_info *pstat;

	if((OPMODE & WIFI_AP_STATE) == 0) {
	    panic_printk("\nwarning: invalid command!\n");
	    return pos;
	}

	PRINT_ONE(" --Target Transition List  -- ", "%s", 1);
	j = 1;
	for (i = 0 ; i < MAX_TRANS_LIST_NUM; i++)
	{
	    if((priv->transition_list_bitmask[i>>3] & (1<<(i&7))) == 0)
		continue;

	    pstat = get_stainfo(priv, priv->transition_list[i].addr);
	    if(pstat) {
		    PRINT_ONE(j, "  [%d]", 0);
	   	    PRINT_ARRAY_ARG("STA:", priv->transition_list[i].addr, "%02x", MACADDRLEN);
		    PRINT_ONE("    BSS Trans Rejection Count:", "%s", 0);
	           PRINT_ONE(pstat->bssTransRejectionCount, "%d", 1);
	           PRINT_ONE("    BSS Trans Trans Expired Time:", "%s", 0);
	           PRINT_ONE(pstat->bssTransExpiredTime, "%d", 1);
	    }
	    j++;
	}

    	return pos;
}

#define TRANS_LIST_PROC_LEN	50
int rtl8192cd_proc_transition_write(struct file *file, const char *buffer,
        unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	unsigned char error_code = 0;
	char * tokptr;
	int command = 0;
	int empty_slot;
	int i;
	char tmp[TRANS_LIST_PROC_LEN];
	char *tmpptr;
	int oldest_idx;
	struct target_transition_list list;

	if((OPMODE & WIFI_AP_STATE) == 0) {
	    error_code = 1;
	    goto end;
	}
	if (count < 2 || count >= TRANS_LIST_PROC_LEN) {
	    return -EFAULT;
	}

	if (buffer == NULL || copy_from_user(tmp, buffer, count))
	    return -EFAULT;

	tmp[count] = 0;
	tmpptr = tmp;
	tmpptr = strsep((char **)&tmpptr, "\n");
	tokptr = strsep((char **)&tmpptr, " ");
	if(!memcmp(tokptr, "add", 3))
		command = 1;
	else if (!memcmp(tokptr, "delall", 6))
	   	command = 3;
	else if(!memcmp(tokptr, "del", 3))
       	command = 2;

	if(command)
	{
	    if(command == 1 || command == 2) {
	        tokptr = strsep((char **)&tmpptr," ");
	        if(tokptr)
	            get_array_val(list.addr, tokptr, 12);
	        else {
	            error_code = 1;
	            goto end;
	        }
	    }

	    if(command == 1)   /*add*/
	    {
			for(i = 0, empty_slot = -1; i < MAX_TRANS_LIST_NUM; i++)
		       {
		            if((priv->transition_list_bitmask[i>>3] & (1<<(i&7))) == 0) {
		            	if(empty_slot == -1)
		            		empty_slot = i;
		            }else if(0 == memcmp(list.addr, priv->transition_list[i].addr, MACADDRLEN)) {
		            	break;
		            }
		        }

		       if(i == MAX_TRANS_LIST_NUM && empty_slot != -1) {/*not found, and has empty slot*/
		        	i = empty_slot;
		        }
			memcpy(&priv->transition_list[i], &list, sizeof(struct target_transition_list));
		     	priv->transition_list_bitmask[i>>3] |= (1<<(i&7));
	    }
	    else if(command == 3)   /*delete all*/
	    {
	        	memset(priv->transition_list_bitmask, 0x00, sizeof(priv->transition_list_bitmask));
	    }
	   else if(command == 2)  /*delete*/
	   {
			for (i = 0 ; i < MAX_TRANS_LIST_NUM; i++) {
			        if((priv->transition_list_bitmask[i>>3] & (1<<(i&7))) == 0)
			        	continue;

			        if(0 == memcmp(list.addr, priv->transition_list[i].addr, MACADDRLEN)) {
			        	priv->transition_list_bitmask[i>>3] &= ~(1<<(i&7));
			        	break;
			        }
			}
	    }
	}
	else {
		    error_code = 1;
		    goto end;
	}

	end:
	if(error_code == 1)
	    panic_printk("\nwarning: invalid command!\n");
	else if(error_code == 2)
	    panic_printk("\nwarning: neighbor report table full!\n");
	return count;
}

#endif


static int rtl8192cd_proc_mib_brext(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;

	PRINT_ONE("  BR Ext info...", "%s", 1);
	PRINT_SINGL_ARG("    nat25_disable: ", priv->pmib->ethBrExtInfo.nat25_disable, "%d");
	PRINT_SINGL_ARG("    macclone_enable: ", priv->pmib->ethBrExtInfo.macclone_enable, "%d");
	PRINT_SINGL_ARG("    dhcp_bcst_disable: ", priv->pmib->ethBrExtInfo.dhcp_bcst_disable, "%d");
	PRINT_SINGL_ARG("    addPPPoETag: ", priv->pmib->ethBrExtInfo.addPPPoETag, "%d");
	PRINT_SINGL_ARG("    nat25sc_disable: ", priv->pmib->ethBrExtInfo.nat25sc_disable, "%d");
	PRINT_ARRAY_ARG("    ukpro_mac: ", priv->ukpro_mac, "%02x", MACADDRLEN);

	return pos;
}


static int rtl8192cd_proc_nat25filter_read(struct seq_file *s, void *data)
{
    struct net_device *dev = PROC_GET_DEV();
    struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
    int pos = 0;


    int i;
    unsigned short ethtype;
    unsigned char ipproto;

    PRINT_ONE(" -- NAT25 Filter info -- ", "%s", 1);
    PRINT_SINGL_ARG("    nat25_filter: ", priv->nat25_filter, "%d");

    PRINT_ONE("    Ethernet Type List:", "%s", 1);
    for (i = 0 ; i < NAT25_FILTER_ETH_NUM; i++) {
        ethtype = priv->nat25_filter_ethlist[i];
        if(ethtype == 0xFFFF)
            break;
        PRINT_ONE(ethtype,  "\t%04X", ((i % 4) == 3)?1:0);
    }

    PRINT_ONE("", "%s", 1);
    PRINT_ONE("    IP Protocol List:", "%s", 1);
    for (i = 0 ; i < NAT25_FILTER_IPPROTO_NUM; i++) {
        ipproto = priv->nat25_filter_ipprotolist[i];
        if(ipproto == 0xFF)
            break;
        PRINT_ONE(ipproto,  "\t%02X", ((i % 4) == 3)?1:0);
    }
    PRINT_ONE("", "%s", 1);
    return pos;


}

static int rtl8192cd_proc_nat25filter_write(struct file *file, const char *buffer,
				unsigned long count, void *data)
{

    struct net_device *dev = (struct net_device *)data;
    struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
    unsigned char error_code = 0;
    char * tokptr;
    int command = 0;
    int type = 0;
    int val;
    int i;

    tokptr = strsep((char **)&buffer, " ");
    if(!memcmp(tokptr, "add", 3))
        command = 1;
    else if(!memcmp(tokptr, "del", 3))
        command = 2;

    if(command) {
        tokptr = strsep((char **)&buffer," ");
        if(!memcmp(tokptr, "ethtype", 7))
            type = 1;
        else if(!memcmp(tokptr, "ipproto", 7))
            type = 2;
        else {
            error_code = 1;
            goto end;
        }
        tokptr = strsep((char **)&buffer," ");
        val = _atoi(tokptr, 16);
        if(val > 0) {
            if(type == 1) {/*ethtype*/
                if(command == 1) { /*add*/
                    for(i = 0; i < NAT25_FILTER_ETH_NUM; i++) {
                        if(priv->nat25_filter_ethlist[i] == 0xFFFF ||
                            priv->nat25_filter_ethlist[i] == val) {
                            break;
                        }
                    }
                    if(i < NAT25_FILTER_ETH_NUM) { /*found*/
                        priv->nat25_filter_ethlist[i] = val;
                    }
                    else {
                        error_code = 2;
                        goto end;
                    }
                }
                else if(command == 2) {/*delete*/
                    for(i = 0; i < NAT25_FILTER_ETH_NUM; i++) {
                        if(priv->nat25_filter_ethlist[i] == val) {
                            for(;i < NAT25_FILTER_ETH_NUM-1;i++) {
                                priv->nat25_filter_ethlist[i] = priv->nat25_filter_ethlist[i+1];
                            }
                            priv->nat25_filter_ethlist[NAT25_FILTER_ETH_NUM-1] = 0xFFFF;
                            break;
                        }
                    }
                }

            }
            else {/*ipproto*/
                if(command == 1) { /*add*/
                    for(i = 0; i < NAT25_FILTER_IPPROTO_NUM; i++) {
                        if(priv->nat25_filter_ipprotolist[i] == 0xFF ||
                            priv->nat25_filter_ipprotolist[i] == val) {
                            break;
                        }
                    }

                    if(i < NAT25_FILTER_IPPROTO_NUM) { /*found*/
                        priv->nat25_filter_ipprotolist[i] = val;
                    }
                    else {
                        error_code = 2;
                        goto end;
                    }
                }
                else if(command == 2) {/*delete*/
                    for(i = 0; i < NAT25_FILTER_IPPROTO_NUM; i++) {
                        if(priv->nat25_filter_ipprotolist[i] == val) {
                            for(;i < NAT25_FILTER_IPPROTO_NUM-1;i++) {
                                priv->nat25_filter_ipprotolist[i] = priv->nat25_filter_ipprotolist[i+1];
                            }
                            priv->nat25_filter_ipprotolist[NAT25_FILTER_IPPROTO_NUM-1] = 0xFF;
                            break;
                        }
                    }
                }
            }
        }
        else {
            error_code = 1;
            goto end;
        }
    }
    else {
        val = tokptr[0] - '0';
        if(0 <= val && val <= 2) {
            priv->nat25_filter = val;
        }
        else {
            error_code = 1;
            goto end;
        }
    }

end:
    if(error_code == 1) {
        panic_printk("\nwarning: unknown comman!\n");
    }
    else if(error_code == 2) {
        panic_printk("\nwarning: filter list is full!\n");
    }
    return count;

}


static int rtl8192cd_proc_txdesc_info(struct seq_file *s, void *data)
{
    struct net_device *dev = PROC_GET_DEV();
    struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
    struct rtl8192cd_hw *phw;
    unsigned long *txdescptr;
    unsigned int q_num = priv->txdesc_num;

    int i, len = 0;

    phw = GET_HW(priv);
    seq_printf(s, "  Tx queue %d descriptor ..........\n", q_num);
    if (IS_HAL_CHIP(priv)) {
        GET_HAL_INTERFACE(priv)->DumpTxBDescTestHandler(priv,
            s,
            q_num);
    } else if(CONFIG_WLAN_NOT_HAL_EXIST)
    {//not HAL
        if (get_txdesc(phw, q_num)) {
            seq_printf(s, "  tx_desc%d/physical: 0x%.8lx/0x%.8lx\n", q_num, (unsigned long)get_txdesc(phw, q_num),
                    *(unsigned long *)(((unsigned long)&phw->tx_ring0_addr)+sizeof(unsigned long)*q_num));
            seq_printf(s, "  head/tail: %3d/%-3d  DW0      DW1      DW2      DW3      DW4      DW5\n",
                    get_txhead(phw, q_num), get_txtail(phw, q_num));
            for (i=0; i<CURRENT_NUM_TX_DESC; i++) {
                txdescptr = (unsigned long *)(get_txdesc(phw, q_num) + i);
                seq_printf(s, "%d[%3d]: %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x %.8x\n", q_num, i,
                        (UINT)get_desc(txdescptr[0]), (UINT)get_desc(txdescptr[1]),
                        (UINT)get_desc(txdescptr[2]), (UINT)get_desc(txdescptr[3]),
                        (UINT)get_desc(txdescptr[4]), (UINT)get_desc(txdescptr[5]),
                        (UINT)get_desc(txdescptr[6]), (UINT)get_desc(txdescptr[7]),
                        (UINT)get_desc(txdescptr[8]), (UINT)get_desc(txdescptr[9])
                    );
            }
        }
    }


    return len;

}


#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
__NOMIPS16
#endif
static int rtl8192cd_proc_txdesc_idx_write(struct file *file, const char *buffer,
		unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	char tmp[32];

	if (count < 2)
		return -EFAULT;

	if (buffer && !copy_from_user(tmp, buffer, 32)) {
		int num = sscanf(tmp, "%d", &priv->txdesc_num);

		if (num != 1)
			panic_printk("Invalid tx desc number!\n");
		else if (priv->txdesc_num > 5) {
			panic_printk("Invalid tx desc number!\n");
			priv->txdesc_num = 0;
		}
		else
			panic_printk("Ready to dump tx desc %d\n", priv->txdesc_num);
	}
	return count;
}

static int rtl8192cd_proc_rxdesc_info(struct seq_file *s, void *data)
{
    struct net_device *dev = PROC_GET_DEV();
    struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
    struct rtl8192cd_hw *phw;
    unsigned long *rxdescptr;

    int i, len = 0;

    if (IS_HAL_CHIP(priv)) {
        DumpRxBDesc88XX(priv,
            s,
            0);
        return 0;
    } else
    {
        phw = GET_HW(priv);
        seq_printf(s, "  Rx queue descriptor ..........\n");
        if(phw->rx_descL){
            seq_printf(s, "  rx_descL/physical: 0x%.8lx/0x%.8lx\n", (unsigned long)phw->rx_descL, phw->rx_ring_addr);
            #ifdef DELAY_REFILL_RX_BUF
            seq_printf(s, "  cur_rx/cur_rx_refill: %d/%d\n", phw->cur_rx, phw->cur_rx_refill);
            #else
            seq_printf(s, "  cur_rx: %d\n", phw->cur_rx);
            #endif
            for(i=0; i<NUM_RX_DESC_IF(priv); i++) {
                rxdescptr = (unsigned long *)(phw->rx_descL+i);
                seq_printf(s, "	   rxdesc[%02d]: 0x%.8x 0x%.8x 0x%.8x 0x%.8x 0x%.8x 0x%.8x 0x%.8x 0x%.8x\n", i,
                    (UINT)get_desc(rxdescptr[0]), (UINT)get_desc(rxdescptr[1]),
                    (UINT)get_desc(rxdescptr[2]), (UINT)get_desc(rxdescptr[3]),
                    (UINT)get_desc(rxdescptr[4]), (UINT)get_desc(rxdescptr[5]),
                    (UINT)get_desc(rxdescptr[6]), (UINT)get_desc(rxdescptr[7]));
            }
        }

        return len;
    }
}

static int rtl8192cd_proc_desc_info(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	struct rtl8192cd_hw *phw = GET_HW(priv);
	int pos = 0;


	if (IS_HAL_CHIP(priv)) {

		PHCI_RX_DMA_MANAGER_88XX    prx_dma = (PHCI_RX_DMA_MANAGER_88XX)(_GET_HAL_DATA(priv)->PRxDMA88XX);
		PHCI_TX_DMA_MANAGER_88XX    ptx_dma = (PHCI_TX_DMA_MANAGER_88XX)(_GET_HAL_DATA(priv)->PTxDMA88XX);
		PHCI_RX_DMA_QUEUE_STRUCT_88XX	rx_q   = &(prx_dma->rx_queue[0]);
		PHCI_TX_DMA_QUEUE_STRUCT_88XX	tx_q   = &(ptx_dma->tx_queue[MGNT_QUEUE]);

		PRINT_ONE("  descriptor info...", "%s", 1);
		PRINT_ONE("    RX queue:", "%s", 1);
		PRINT_ONE(" 	 RDSAR: ", "%s", 0);
		PRINT_ONE((UINT)RTL_R32(RX_DESA), "0x%.8x", 0);

		PRINT_ONE("  hwIdx/hostIdx/curHostIdx: ", "%s", 0);
		PRINT_ONE((UINT)rx_q->hw_idx, "%d/", 0);
		PRINT_ONE((UINT)rx_q->host_idx, "%d/", 0);
		PRINT_ONE((UINT)rx_q->cur_host_idx, "%d", 1);

		PRINT_ONE("    queue 0:", "%s", 1);
		PRINT_ONE(" 	 TMGDA: ", "%s", 0);
		PRINT_ONE((UINT)RTL_R32(REG_MGQ_TXBD_DESA), "0x%.8x", 0);
		PRINT_ONE("  hwIdx/hostIdx: ", "%s", 0);
		PRINT_ONE((UINT)tx_q->hw_idx, "%d/", 0);
		PRINT_ONE((UINT)tx_q->host_idx, "%d", 1);

		tx_q   = &(ptx_dma->tx_queue[BK_QUEUE]);
		PRINT_ONE("    queue 1:", "%s", 1);
		PRINT_ONE(" 	 TBKDA: ", "%s", 0);
		PRINT_ONE((UINT)RTL_R32(REG_BKQ_TXBD_DESA), "0x%.8x", 0);
		PRINT_ONE("  hwIdx/hostIdx: ", "%s", 0);
		PRINT_ONE((UINT)tx_q->hw_idx, "%d/", 0);
		PRINT_ONE((UINT)tx_q->host_idx, "%d", 1);

		tx_q   = &(ptx_dma->tx_queue[BE_QUEUE]);
		PRINT_ONE("    queue 2:", "%s", 1);
		PRINT_ONE(" 	 TBEDA: ", "%s", 0);
		PRINT_ONE((UINT)RTL_R32(REG_BEQ_TXBD_DESA), "0x%.8x", 0);
		PRINT_ONE("  hwIdx/hostIdx: ", "%s", 0);
		PRINT_ONE((UINT)tx_q->hw_idx, "%d/", 0);
		PRINT_ONE((UINT)tx_q->host_idx, "%d", 1);

		tx_q   = &(ptx_dma->tx_queue[VI_QUEUE]);
		PRINT_ONE("    queue 3:", "%s", 1);
		PRINT_ONE(" 	 TVIDA: ", "%s", 0);
		PRINT_ONE((UINT)RTL_R32(REG_VIQ_TXBD_DESA), "0x%.8x", 0);
		PRINT_ONE("  hwIdx/hostIdx: ", "%s", 0);
		PRINT_ONE((UINT)tx_q->hw_idx, "%d/", 0);
		PRINT_ONE((UINT)tx_q->host_idx, "%d", 1);

		tx_q   = &(ptx_dma->tx_queue[VO_QUEUE]);
		PRINT_ONE("    queue 4:", "%s", 1);
		PRINT_ONE(" 	 TVODA: ", "%s", 0);
		PRINT_ONE((UINT)RTL_R32(REG_VOQ_TXBD_DESA), "0x%.8x", 0);
		PRINT_ONE("  hwIdx/hostIdx: ", "%s", 0);
		PRINT_ONE((UINT)tx_q->hw_idx, "%d/", 0);
		PRINT_ONE((UINT)tx_q->host_idx, "%d", 1);

		tx_q   = &(ptx_dma->tx_queue[HIGH_QUEUE]);
		PRINT_ONE("    queue 5:", "%s", 1);
		PRINT_ONE(" 	 TH0DA: ", "%s", 0);
		PRINT_ONE((UINT)RTL_R32(REG_HI0Q_TXBD_DESA), "0x%.8x", 0);
		PRINT_ONE("  hwIdx/hostIdx: ", "%s", 0);
		PRINT_ONE((UINT)tx_q->hw_idx, "%d/", 0);
		PRINT_ONE((UINT)tx_q->host_idx, "%d", 1);

		tx_q   = &(ptx_dma->tx_queue[HIGH_QUEUE1]);
		PRINT_ONE("    queue 6:", "%s", 1);
		PRINT_ONE(" 	 TH1DA: ", "%s", 0);
		PRINT_ONE((UINT)RTL_R32(REG_HI1Q_TXBD_DESA), "0x%.8x", 0);
		PRINT_ONE("  hwIdx/hostIdx: ", "%s", 0);
		PRINT_ONE((UINT)tx_q->hw_idx, "%d/", 0);
		PRINT_ONE((UINT)tx_q->host_idx, "%d", 1);

		tx_q   = &(ptx_dma->tx_queue[HIGH_QUEUE2]);
		PRINT_ONE("    queue 7:", "%s", 1);
		PRINT_ONE(" 	 TH2DA: ", "%s", 0);
		PRINT_ONE((UINT)RTL_R32(REG_HI2Q_TXBD_DESA), "0x%.8x", 0);
		PRINT_ONE("  hwIdx/hostIdx: ", "%s", 0);
		PRINT_ONE((UINT)tx_q->hw_idx, "%d/", 0);
		PRINT_ONE((UINT)tx_q->host_idx, "%d", 1);

		tx_q   = &(ptx_dma->tx_queue[HIGH_QUEUE3]);
		PRINT_ONE("    queue 8:", "%s", 1);
		PRINT_ONE(" 	 TH3DA: ", "%s", 0);
		PRINT_ONE((UINT)RTL_R32(REG_HI3Q_TXBD_DESA), "0x%.8x", 0);
		PRINT_ONE("  hwIdx/hostIdx: ", "%s", 0);
		PRINT_ONE((UINT)tx_q->hw_idx, "%d/", 0);
		PRINT_ONE((UINT)tx_q->host_idx, "%d", 1);

		tx_q   = &(ptx_dma->tx_queue[HIGH_QUEUE4]);
		PRINT_ONE("    queue 9:", "%s", 1);
		PRINT_ONE(" 	 TH4DA: ", "%s", 0);
		PRINT_ONE((UINT)RTL_R32(REG_HI4Q_TXBD_DESA), "0x%.8x", 0);
		PRINT_ONE("  hwIdx/hostIdx: ", "%s", 0);
		PRINT_ONE((UINT)tx_q->hw_idx, "%d/", 0);
		PRINT_ONE((UINT)tx_q->host_idx, "%d", 1);

		tx_q   = &(ptx_dma->tx_queue[HIGH_QUEUE5]);
		PRINT_ONE("    queue 10:", "%s", 1);
		PRINT_ONE(" 	 TH5DA: ", "%s", 0);
		PRINT_ONE((UINT)RTL_R32(REG_HI5Q_TXBD_DESA), "0x%.8x", 0);
		PRINT_ONE("  hwIdx/hostIdx: ", "%s", 0);
		PRINT_ONE((UINT)tx_q->hw_idx, "%d/", 0);
		PRINT_ONE((UINT)tx_q->host_idx, "%d", 1);

		tx_q   = &(ptx_dma->tx_queue[HIGH_QUEUE6]);
		PRINT_ONE("    queue 11:", "%s", 1);
		PRINT_ONE(" 	 TH6DA: ", "%s", 0);
		PRINT_ONE((UINT)RTL_R32(REG_HI6Q_TXBD_DESA), "0x%.8x", 0);
		PRINT_ONE("  hwIdx/hostIdx: ", "%s", 0);
		PRINT_ONE((UINT)tx_q->hw_idx, "%d/", 0);
		PRINT_ONE((UINT)tx_q->host_idx, "%d", 1);

		tx_q   = &(ptx_dma->tx_queue[HIGH_QUEUE7]);
		PRINT_ONE("    queue 12:", "%s", 1);
		PRINT_ONE(" 	 TH7DA: ", "%s", 0);
		PRINT_ONE((UINT)RTL_R32(REG_HI7Q_TXBD_DESA), "0x%.8x", 0);
		PRINT_ONE("  hwIdx/hostIdx: ", "%s", 0);
		PRINT_ONE((UINT)tx_q->hw_idx, "%d/", 0);
		PRINT_ONE((UINT)tx_q->host_idx, "%d", 1);

	} else if(CONFIG_WLAN_NOT_HAL_EXIST)
	{//not HAL
	PRINT_ONE("  descriptor info...", "%s", 1);
	PRINT_ONE("    RX queue:", "%s", 1);
	PRINT_ONE("      rx_descL/physical: ", "%s", 0);
	PRINT_ONE((unsigned long)phw->rx_descL, "0x%.8lx/", 0);
	PRINT_ONE(phw->rx_ring_addr, "0x%.8lx", 1);
	PRINT_ONE("      RDSAR: ", "%s", 0);
	PRINT_ONE((UINT)RTL_R32(RX_DESA), "0x%.8x", 0);

#ifdef DELAY_REFILL_RX_BUF
	PRINT_ONE("  cur_rx/cur_rx_refill: ", "%s", 0);
	PRINT_ONE((UINT)phw->cur_rx, "%d/", 0);
	PRINT_ONE((UINT)phw->cur_rx_refill, "%d", 1);
#else
	PRINT_ONE("  cur_rx: ", "%s", 0);
	PRINT_ONE((UINT)phw->cur_rx, "%d", 1);
#endif

	PRINT_ONE("    queue 0:", "%s", 1);
	PRINT_ONE("      tx_desc0/physical: ", "%s", 0);
	PRINT_ONE((unsigned long)phw->tx_desc0, "0x%.8lx/", 0);
	PRINT_ONE(phw->tx_ring0_addr, "0x%.8lx", 1);
	PRINT_ONE("      TMGDA: ", "%s", 0);
	PRINT_ONE((UINT)RTL_R32(MGQ_DESA), "0x%.8x", 0);
	PRINT_ONE("  head/tail: ", "%s", 0);
	PRINT_ONE((UINT)phw->txhead0, "%d/", 0);
	PRINT_ONE((UINT)phw->txtail0, "%d", 1);

	PRINT_ONE("    queue 1:", "%s", 1);
	PRINT_ONE("      tx_desc1/physical: ", "%s", 0);
	PRINT_ONE((unsigned long)phw->tx_desc1, "0x%.8lx/", 0);
	PRINT_ONE(phw->tx_ring1_addr, "0x%.8lx", 1);
	PRINT_ONE("      TBKDA: ", "%s", 0);
	PRINT_ONE((UINT)RTL_R32(BKQ_DESA), "0x%.8x", 0);
	PRINT_ONE("  head/tail: ", "%s", 0);
	PRINT_ONE((UINT)phw->txhead1, "%d/", 0);
	PRINT_ONE((UINT)phw->txtail1, "%d", 1);

	PRINT_ONE("    queue 2:", "%s", 1);
	PRINT_ONE("      tx_desc2/physical: ", "%s", 0);
	PRINT_ONE((unsigned long)phw->tx_desc2, "0x%.8lx/", 0);
	PRINT_ONE(phw->tx_ring2_addr, "0x%.8lx", 1);
	PRINT_ONE("      TBEDA: ", "%s", 0);
	PRINT_ONE((UINT)RTL_R32(BEQ_DESA), "0x%.8x", 0);
	PRINT_ONE("  head/tail: ", "%s", 0);
	PRINT_ONE((UINT)phw->txhead2, "%d/", 0);
	PRINT_ONE((UINT)phw->txtail2, "%d", 1);

	PRINT_ONE("    queue 3:", "%s", 1);
	PRINT_ONE("      tx_desc3/physical: ", "%s", 0);
	PRINT_ONE((unsigned long)phw->tx_desc3, "0x%.8lx/", 0);
	PRINT_ONE(phw->tx_ring3_addr, "0x%.8lx", 1);
	PRINT_ONE("      TLPDA: ", "%s", 0);
	PRINT_ONE((UINT)RTL_R32(VIQ_DESA), "0x%.8x", 0);
	PRINT_ONE("  head/tail: ", "%s", 0);
	PRINT_ONE((UINT)phw->txhead3, "%d/", 0);
	PRINT_ONE((UINT)phw->txtail3, "%d", 1);

	PRINT_ONE("    queue 4:", "%s", 1);
	PRINT_ONE("      tx_desc4/physical: ", "%s", 0);
	PRINT_ONE((unsigned long)phw->tx_desc4, "0x%.8lx/", 0);
	PRINT_ONE(phw->tx_ring4_addr, "0x%.8lx", 1);
	PRINT_ONE("      TNPDA: ", "%s", 0);
	PRINT_ONE((UINT)RTL_R32(VOQ_DESA), "0x%.8x", 0);
	PRINT_ONE("  head/tail: ", "%s", 0);
	PRINT_ONE((UINT)phw->txhead4, "%d/", 0);
	PRINT_ONE((UINT)phw->txtail4, "%d", 1);

	PRINT_ONE("    queue 5:", "%s", 1);
	PRINT_ONE("      tx_desc5/physical: ", "%s", 0);
	PRINT_ONE((unsigned long)phw->tx_desc5, "0x%.8lx/", 0);
	PRINT_ONE(phw->tx_ring5_addr, "0x%.8lx", 1);
	PRINT_ONE("      THPDA: ", "%s", 0);
	PRINT_ONE((UINT)RTL_R32(HQ_DESA), "0x%.8x", 0);
	PRINT_ONE("  head/tail: ", "%s", 0);
	PRINT_ONE((UINT)phw->txhead5, "%d/", 0);
	PRINT_ONE((UINT)phw->txtail5, "%d", 1);
	}
#if 0
	PRINT_ONE("    RX cmd queue:", "%s", 1);
	PRINT_ONE("      rxcmd_desc/physical: ", "%s", 0);
	PRINT_ONE((UINT)phw->rxcmd_desc, "0x%.8x/", 0);
	PRINT_ONE((UINT)phw->rxcmd_ring_addr, "0x%.8x", 1);
	PRINT_ONE("      RCDSA: ", "%s", 0);
	PRINT_ONE((UINT)RTL_R32(_RCDSA_), "0x%.8x", 0);
	PRINT_ONE("  cur_rx: ", "%s", 0);
	PRINT_ONE((UINT)phw->cur_rxcmd, "%d", 1);

	PRINT_ONE("    TX cmd queue:", "%s", 1);
	PRINT_ONE("      txcmd_desc/physical: ", "%s", 0);
	PRINT_ONE((UINT)phw->txcmd_desc, "0x%.8x/", 0);
	PRINT_ONE((UINT)phw->txcmd_ring_addr, "0x%.8x", 1);
	PRINT_ONE("      TCDA:  ", "%s", 0);
	PRINT_ONE((UINT)RTL_R32(_TCDA_), "0x%.8x", 0);
	PRINT_ONE("  head/tail: ", "%s", 0);
	PRINT_ONE((UINT)phw->txcmdhead, "%d/", 0);
	PRINT_ONE((UINT)phw->txcmdtail, "%d", 1);
#endif
	return pos;
}




static int rtl8192cd_proc_buf_info(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;

	PRINT_ONE("  buf info...", "%s", 1);
	PRINT_ONE("    hdr poll:", "%s", 1);
	PRINT_ONE("      head: ", "%s", 0);
	PRINT_ONE((unsigned long)&priv->pshare->wlan_hdrlist, "0x%.8lx", 0);
	PRINT_ONE("    count: ", "%s", 0);
	PRINT_ONE(priv->pshare->pwlan_hdr_poll->count, "%d", 1);

	PRINT_ONE("    hdrllc poll:", "%s", 1);
	PRINT_ONE("      head: ", "%s", 0);
	PRINT_ONE((unsigned long)&priv->pshare->wlanllc_hdrlist, "0x%.8lx", 0);
	PRINT_ONE("    count: ", "%s", 0);
	PRINT_ONE(priv->pshare->pwlanllc_hdr_poll->count, "%d", 1);

	PRINT_ONE("    mgmtbuf poll:", "%s", 1);
	PRINT_ONE("      head: ", "%s", 0);
	PRINT_ONE((unsigned long)&priv->pshare->wlanbuf_list, "0x%.8lx", 0);
	PRINT_ONE("    count: ", "%s", 0);
	PRINT_ONE(priv->pshare->pwlanbuf_poll->count, "%d", 1);

	PRINT_ONE("    icv poll:", "%s", 1);
	PRINT_ONE("      head: ", "%s", 0);
	PRINT_ONE((unsigned long)&priv->pshare->wlanicv_list, "0x%.8lx", 0);
	PRINT_ONE("    count: ", "%s", 0);
	PRINT_ONE(priv->pshare->pwlanicv_poll->count, "%d", 1);

	PRINT_ONE("    mic poll:", "%s", 1);
	PRINT_ONE("      head: ", "%s", 0);
	PRINT_ONE((unsigned long)&priv->pshare->wlanmic_list, "0x%.8lx", 0);
	PRINT_ONE("    count: ", "%s", 0);
	PRINT_ONE(priv->pshare->pwlanmic_poll->count, "%d", 1);

	return pos;
}


#ifdef ENABLE_RTL_SKB_STATS
static int rtl8192cd_proc_skb_info(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;

	PRINT_ONE("  skb counter...", "%s", 1);
	PRINT_SINGL_ARG("    skb_tx_cnt: ", rtl_atomic_read(&priv->rtl_tx_skb_cnt) , "%d");
	PRINT_SINGL_ARG("    skb_rx_cnt: ", rtl_atomic_read(&priv->rtl_rx_skb_cnt) , "%d");

	return pos;
}
#endif

static int rtl8192cd_proc_mib_11n(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;

	PRINT_ONE("  dot11nConfigEntry...", "%s", 1);

	PRINT_SINGL_ARG("    supportedmcs: ", get_supported_mcs(priv), "%08x");
	PRINT_SINGL_ARG("    basicmcs: ", priv->pmib->dot11nConfigEntry.dot11nBasicMCS, "%08x");
	PRINT_SINGL_ARG("    use40M: ", priv->pmib->dot11nConfigEntry.dot11nUse40M, "%d");
	PRINT_SINGL_ARG("    currBW: ", (!priv->pshare->is_40m_bw) ? 20 :(priv->pshare->is_40m_bw*40), "%dM");
	PRINT_SINGL_ARG("    currBW(op): ", (!priv->pshare->CurrentChannelBW) ? 20 :(priv->pshare->CurrentChannelBW*40), "%dM");

	PRINT_ONE("    2ndchoffset: ", "%s", 0);
	switch (priv->pshare->offset_2nd_chan) {
	case HT_2NDCH_OFFSET_BELOW:
		PRINT_ONE("below", "%s", 1);
		break;
	case HT_2NDCH_OFFSET_ABOVE:
		PRINT_ONE("above", "%s", 1);
		break;
	default:
		PRINT_ONE("dontcare", "%s", 1);
		break;
	}

	PRINT_SINGL_ARG("    shortGI20M: ", priv->pmib->dot11nConfigEntry.dot11nShortGIfor20M, "%d");
	PRINT_SINGL_ARG("    shortGI40M: ", priv->pmib->dot11nConfigEntry.dot11nShortGIfor40M, "%d");
	PRINT_SINGL_ARG("    shortGI80M: ", priv->pmib->dot11nConfigEntry.dot11nShortGIfor80M, "%d");
	PRINT_SINGL_ARG("    stbc: ", priv->pmib->dot11nConfigEntry.dot11nSTBC, "%d");
	PRINT_SINGL_ARG("    ldpc: ", priv->pmib->dot11nConfigEntry.dot11nLDPC, "%d");
	PRINT_SINGL_ARG("    ampdu: ", priv->pmib->dot11nConfigEntry.dot11nAMPDU, "%d");
	PRINT_SINGL_ARG("    amsdu: ", priv->pmib->dot11nConfigEntry.dot11nAMSDU, "%d");
	PRINT_SINGL_ARG("    ampduSndSz: ", priv->pmib->dot11nConfigEntry.dot11nAMPDUSendSz, "%d");
	PRINT_SINGL_ARG("    amsduMax: ", priv->pmib->dot11nConfigEntry.dot11nAMSDURecvMax, "%d");
	PRINT_SINGL_ARG("    amsduTimeout: ", priv->pmib->dot11nConfigEntry.dot11nAMSDUSendTimeout, "%d");
	PRINT_SINGL_ARG("    amsduNum: ", priv->pmib->dot11nConfigEntry.dot11nAMSDUSendNum, "%d");
	PRINT_SINGL_ARG("    curAmsduNum: ", priv->pmib->dot11nConfigEntry.dot11curAMSDUSendNum, "%d");
	PRINT_SINGL_ARG("    lgyEncRstrct: ", priv->pmib->dot11nConfigEntry.dot11nLgyEncRstrct, "%d");
#ifdef WIFI_11N_2040_COEXIST
	PRINT_SINGL_ARG("    coexist: ", priv->pmib->dot11nConfigEntry.dot11nCoexist, "%d");
	if (COEXIST_ENABLE) {
#ifdef CLIENT_MODE
		if (OPMODE & WIFI_STATION_STATE) {
			PRINT_SINGL_ARG("    coexist_connection: ", priv->coexist_connection, "%d");
			PRINT_SINGL_ARG("    obss_scan:          ", priv->pmib->dot11nConfigEntry.dot11nCoexist_obss_scan, "%d");
		}

		if ((OPMODE & WIFI_AP_STATE) ||
			((OPMODE & WIFI_STATION_STATE) && priv->coexist_connection))
#endif
			PRINT_SINGL_ARG("    bg_ap_timeout: ", priv->bg_ap_timeout, "%d");
#ifdef CLIENT_MODE
		if ((OPMODE & WIFI_STATION_STATE) && priv->coexist_connection) {
			if (priv->bg_ap_timeout)
				PRINT_ARRAY_ARG("    bg_ap_timeout_ch: ", priv->bg_ap_timeout_ch, "%d ", 14);

			PRINT_SINGL_ARG("    intolerant_timeout: ", priv->intolerant_timeout, "%d");
		} else
#endif
		if (OPMODE & WIFI_AP_STATE) {
			PRINT_BITMAP_ARG("    force_20_sta", priv->force_20_sta);
			PRINT_BITMAP_ARG("    switch_20_sta", priv->switch_20_sta);
			PRINT_SINGL_ARG("    bg_ap_rssi_chk_th: ", priv->pmib->dot11nConfigEntry.dot11nBGAPRssiChkTh, "%d");
		}
	}
#endif

	PRINT_SINGL_ARG("    txnoack: ", priv->pmib->dot11nConfigEntry.dot11nTxNoAck, "%d");

	if (priv->ht_cap_len) {
		unsigned char *pbuf = (unsigned char *)&priv->ht_cap_buf;
		PRINT_ARRAY_ARG("    ht_cap: ", pbuf, "%02x", priv->ht_cap_len);
	}
	else {
		PRINT_ONE("    ht_cap: none", "%s", 1);
	}
	if (priv->ht_ie_len) {
		unsigned char *pbuf = (unsigned char *)&priv->ht_ie_buf;
		PRINT_ARRAY_ARG("    ht_ie: ", pbuf, "%02x", priv->ht_ie_len);
	}
	else {
		PRINT_ONE("    ht_ie: none", "%s", 1);
	}

	PRINT_SINGL_ARG("    legacy_obss_to: ", priv->ht_legacy_obss_to, "%d");
	PRINT_SINGL_ARG("    nomember_legacy_sta_to: ", priv->ht_nomember_legacy_sta_to, "%d");
	PRINT_SINGL_ARG("    legacy_sta_num: ", priv->ht_legacy_sta_num, "%d");
	PRINT_SINGL_ARG("    11nProtection: ", priv->ht_protection, "%d");
//	PRINT_BITMAP_ARG("    has_2r_sta", priv->pshare->has_2r_sta);

#ifdef COCHANNEL_RTS
	PRINT_SINGL_ARG("    cochannel: ", priv->cochannel_to, "%d");
#endif
	return pos;
}




#ifdef ERR_ACCESS_CNTR
static int rtl8192cd_proc_err_access(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;
	int i;
	unsigned char tmpbuf[32];

	for (i=0; i<MAX_ERR_ACCESS_CNTR; i++) {
		if (priv->err_ac_list[i].used) {
			sprintf(tmpbuf, "%02x:%02x:%02x:%02x:%02x:%02x %u",
				priv->err_ac_list[i].mac[0], priv->err_ac_list[i].mac[1], priv->err_ac_list[i].mac[2],
				priv->err_ac_list[i].mac[3], priv->err_ac_list[i].mac[4], priv->err_ac_list[i].mac[5],
				priv->err_ac_list[i].num);
			PRINT_ONE(tmpbuf, "%s", 1);
		}
	}
	memset(priv->err_ac_list, 0, sizeof(struct err_access_list) * MAX_ERR_ACCESS_CNTR);

	return pos;
}
#endif


#ifdef AUTO_TEST_SUPPORT
static int rtl8192cd_proc_SSR_read(struct seq_file *s, void *data)
{
    struct net_device *dev = PROC_GET_DEV();
    struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
    int len = 0;

    off_t pos = 0, base;
    int size;

    int i1;
    unsigned char tmp[6];

    //wait SiteSurvey completed
    if(priv->ss_req_ongoing)	{
        PRINT_ONE("waitting", "%s", 1);
        len = pos;
    }else{
        PRINT_ONE(" SiteSurvey result : ", "%s", 1);
        PRINT_ONE("    ====================", "%s", 1);
        if(priv->site_survey->count_backup==0){
            PRINT_ONE("none", "%s", 1);
            len = pos;
        }else{
            len = pos;

            for(i1=0; i1<priv->site_survey->count_backup ;i1++){
                base = pos;
                memcpy(tmp,priv->site_survey->bss_backup[i1].bssid,MACADDRLEN);
                /*
                            panic_printk("Mac=%02X%02X%02X:%02X%02X%02X ;Channel=%02d ;SSID:%s  \n",
                            tmp[0],tmp[1],tmp[2],tmp[3],tmp[4],tmp[5]
                            ,priv->site_survey->bss_backup[i1].channel
                            ,priv->site_survey->bss_backup[i1].ssid
                            );
                            */
                PRINT_ARRAY_ARG("    HwAddr: ",	tmp, "%02x", MACADDRLEN);
                PRINT_SINGL_ARG("    Channel: ",	priv->site_survey->bss_backup[i1].channel, "%d");
                PRINT_SINGL_ARG("    SSID: ", priv->site_survey->bss_backup[i1].ssid, "%s");
                PRINT_SINGL_ARG("    Type: ", ((priv->site_survey->bss_backup[i1].bsstype == 16) ? "AP" : "Ad-Hoc"), "%s");
                //PRINT_SINGL_ARG("    Type: ", priv->site_survey->bss_backup[i1].bsstype, "%d");
                PRINT_ONE("    ====================", "%s", 1);

                size = pos - base;
                CHECK_LEN;
                pos = len;
            }
        }
    }
    return 0;
}
#endif
#ifdef CLIENT_MODE
static int rtl8192cd_proc_up_read(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int len = 0;
	seq_printf(s, "%d\n",priv->up_flag);
	return len;
}

static int rtl8192cd_proc_up_write(struct file *file, const char *buffer,
				unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	char tmp[4];

	if (buffer && !copy_from_user(tmp, buffer, 4)) {
		if(0 == (tmp[0]-'0'))
			priv->up_flag = 0;
	}
	return count;
}
#endif

#ifdef CONFIG_RTL_WLAN_STATUS
static int rtl8192cd_proc_up_event_read(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);

	int len = 0;

	seq_printf(s, "%d\n",priv->wlan_status_flag);
	return len;
}

static int rtl8192cd_proc_up_event_write(struct file *file, const char *buffer,
				unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	char tmp[4];

	if (buffer && !copy_from_user(tmp, buffer, 4)) {
		priv->wlan_status_flag=tmp[0]-'0';
	}
	return count;
}
#endif

#ifdef CONFIG_RTK_VLAN_SUPPORT
static int rtl8192cd_proc_vlan_read(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;
	PRINT_ONE("  vlan setting...", "%s", 1);
	PRINT_SINGL_ARG("    global_vlan: ", priv->pmib->vlan.global_vlan, "%d");
	PRINT_SINGL_ARG("    is_lan: ", priv->pmib->vlan.is_lan, "%d");
	PRINT_SINGL_ARG("    vlan_enable: ", priv->pmib->vlan.vlan_enable, "%d");
	PRINT_SINGL_ARG("    vlan_tag: ", priv->pmib->vlan.vlan_tag, "%d");
	PRINT_SINGL_ARG("    vlan_id: ", priv->pmib->vlan.vlan_id, "%d");
	PRINT_SINGL_ARG("    vlan_pri: ", priv->pmib->vlan.vlan_pri, "%d");
	PRINT_SINGL_ARG("    vlan_cfi: ", priv->pmib->vlan.vlan_cfi, "%d");
#if defined(CONFIG_RTK_BRIDGE_VLAN_SUPPORT)|| defined(CONFIG_RTL_HW_VLAN_SUPPORT)
	PRINT_SINGL_ARG("    vlan_forwarding_rule: ", priv->pmib->vlan.forwarding_rule, "%d");
#endif

	return pos;
}


#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
__NOMIPS16
#endif
static int rtl8192cd_proc_vlan_write(struct file *file, const char *buffer,
		unsigned long count, void *data)
{
	struct net_device *dev = PDE_DATA(file_inode(file));
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	char tmp[128];

	if (count < 2)
		return -EFAULT;
	if (buffer && !copy_from_user(tmp, buffer, 128)) {
#if defined(CONFIG_RTK_BRIDGE_VLAN_SUPPORT)|| defined(CONFIG_RTL_HW_VLAN_SUPPORT)
		int num = sscanf(tmp, "%d %d %d %d %d %d %d %d",
#else
		int num = sscanf(tmp, "%d %d %d %d %d %d %d",
#endif
			&priv->pmib->vlan.global_vlan, &priv->pmib->vlan.is_lan,
			&priv->pmib->vlan.vlan_enable, &priv->pmib->vlan.vlan_tag,
			&priv->pmib->vlan.vlan_id, &priv->pmib->vlan.vlan_pri,
			&priv->pmib->vlan.vlan_cfi
#if defined(CONFIG_RTK_BRIDGE_VLAN_SUPPORT)|| defined(CONFIG_RTL_HW_VLAN_SUPPORT)
			, &priv->pmib->vlan.forwarding_rule
#endif
			);

#if defined(CONFIG_RTK_BRIDGE_VLAN_SUPPORT)|| defined(CONFIG_RTL_HW_VLAN_SUPPORT)
		if (num != 8)
#else
		if (num != 7)
#endif
		{
			panic_printk("invalid vlan parameter!\n");
		}
	}


	return count;
}
#ifdef CONFIG_RTL_HW_VLAN_SUPPORT
int is_rtl_nat_wlan_vlan(struct net_device *dev)
{
   struct rtl8192cd_priv *priv;
        priv = ((struct rtl8192cd_priv *)netdev_priv(dev))->wlan_priv;
   if(priv->pmib->vlan.global_vlan && priv->pmib->vlan.vlan_enable )
        if(priv->pmib->vlan.vlan_id && (priv->pmib->vlan.forwarding_rule==2)) //NAT rule
           return 1;

    return 0;
}
#endif
#endif // CONFIG_RTK_VLAN_SUPPORT


#ifdef SUPPORT_MULTI_PROFILE
static int rtl8192cd_proc_mib_ap_profile(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);

	int pos = 0, i;
	unsigned char tmpbuf[100];

	PRINT_ONE("  Client Profile...", "%s", 1);
	PRINT_SINGL_ARG("    enable_profile: ", priv->pmib->ap_profile.enable_profile, "%d");
	PRINT_SINGL_ARG("    profile_num: ", priv->pmib->ap_profile.profile_num, "%d");
	PRINT_SINGL_ARG("    in_use_profile: ", ((priv->profile_idx == 0) ? (priv->pmib->ap_profile.profile_num-1) : (priv->profile_idx-1)), "%d");

	for (i=0; i<priv->pmib->ap_profile.profile_num && i<PROFILE_NUM; i++) {
		sprintf(tmpbuf, "       profile[%d]...", i);
		PRINT_ONE(tmpbuf, "%s", 1);
		PRINT_SINGL_ARG("         ssid: ", priv->pmib->ap_profile.profile[i].ssid, "%s");
		PRINT_SINGL_ARG("         encryption: ", priv->pmib->ap_profile.profile[i].encryption, "%d");
		PRINT_SINGL_ARG("         auth_type: ", priv->pmib->ap_profile.profile[i].auth_type, "%d");
		if (priv->pmib->ap_profile.profile[i].encryption == 1 || priv->pmib->ap_profile.profile[i].encryption == 2) {
			PRINT_SINGL_ARG("         wep_default_key: ", priv->pmib->ap_profile.profile[i].wep_default_key, "%d");
			 if (priv->pmib->ap_profile.profile[i].encryption == 1) {
				PRINT_ARRAY_ARG("         wep_key1: ", priv->pmib->ap_profile.profile[i].wep_key1, "%02x", 5);
				PRINT_ARRAY_ARG("         wep_key2: ", priv->pmib->ap_profile.profile[i].wep_key2, "%02x", 5);
				PRINT_ARRAY_ARG("         wep_key3: ", priv->pmib->ap_profile.profile[i].wep_key3, "%02x", 5);
				PRINT_ARRAY_ARG("         wep_key4: ", priv->pmib->ap_profile.profile[i].wep_key4, "%02x", 5);
			}
			else {
				PRINT_ARRAY_ARG("         wep_key1: ", priv->pmib->ap_profile.profile[i].wep_key1, "%02x", 13);
				PRINT_ARRAY_ARG("         wep_key2: ", priv->pmib->ap_profile.profile[i].wep_key2, "%02x", 13);
				PRINT_ARRAY_ARG("         wep_key3: ", priv->pmib->ap_profile.profile[i].wep_key3, "%02x", 13);
				PRINT_ARRAY_ARG("         wep_key4: ", priv->pmib->ap_profile.profile[i].wep_key4, "%02x", 13);
			}
		}
		else if (priv->pmib->ap_profile.profile[i].encryption == 3 || priv->pmib->ap_profile.profile[i].encryption == 4) {
			PRINT_SINGL_ARG("         wpa_cipher: ", priv->pmib->ap_profile.profile[i].wpa_cipher, "%d");
			PRINT_SINGL_ARG("         wpa_psk: ", priv->pmib->ap_profile.profile[i].wpa_psk, "%s");
		#ifdef CONFIG_IEEE80211W_CLI
			PRINT_SINGL_ARG("         bss_PMF: ", priv->pmib->ap_profile.profile[i].bss_PMF, "%d");
		#endif
		}
	}
	return pos;
}
#endif // SUPPORT_MULTI_PROFILE

static int rtl8192cd_proc_mib_misc(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);

	int pos = 0;

	PRINT_ONE("  miscEntry...", "%s", 1);

	PRINT_SINGL_ARG("    show_hidden_bss: ", priv->pmib->miscEntry.show_hidden_bss, "%d");
	PRINT_SINGL_ARG("    ack_timeout: ", (unsigned char)priv->pmib->miscEntry.ack_timeout, "%d");
	PRINT_ARRAY_ARG("    private_ie: ", priv->pmib->miscEntry.private_ie, "%02x", priv->pmib->miscEntry.private_ie_len);
	PRINT_SINGL_ARG("    rxInt: ", priv->pmib->miscEntry.rxInt_thrd, "%d");
#ifdef DRVMAC_LB
	PRINT_SINGL_ARG("    dmlb: ", priv->pmib->miscEntry.drvmac_lb, "%d");
	PRINT_ARRAY_ARG("    lb_da: ", priv->pmib->miscEntry.lb_da, "%02x", MACADDRLEN);
	PRINT_SINGL_ARG("    lb_tps: ", priv->pmib->miscEntry.lb_tps, "%d");
#endif
	PRINT_SINGL_ARG("    groupID: ", priv->pmib->miscEntry.groupID, "%d");
	PRINT_SINGL_ARG("    rc_enable: ", priv->pmib->reorderCtrlEntry.ReorderCtrlEnable, "%d");
	PRINT_SINGL_ARG("    rc_winsz: ", priv->pmib->reorderCtrlEntry.ReorderCtrlWinSz, "%d");
	PRINT_SINGL_ARG("    rc_timeout: ", priv->pmib->reorderCtrlEntry.ReorderCtrlTimeout, "%d");
	PRINT_SINGL_ARG("    rc_timeout_cli: ", priv->pmib->reorderCtrlEntry.ReorderCtrlTimeoutCli, "%d");
	PRINT_SINGL_ARG("    vap_enable: ", priv->pmib->miscEntry.vap_enable, "%d");
#ifdef RESERVE_TXDESC_FOR_EACH_IF
	PRINT_SINGL_ARG("    rsv_txdesc: ", GET_ROOT(priv)->pmib->miscEntry.rsv_txdesc, "%d");
#endif
#ifdef USE_TXQUEUE
	PRINT_SINGL_ARG("    use_txq: ", GET_ROOT(priv)->pmib->miscEntry.use_txq, "%d");
#endif
	PRINT_SINGL_ARG("    func_off: ", priv->pmib->miscEntry.func_off, "%d");
    PRINT_SINGL_ARG("    raku_only: ", priv->pmib->miscEntry.raku_only, "%d");
#ifdef TV_MODE
    PRINT_SINGL_ARG("    tv_mode_status: ", priv->tv_mode_status, "%d");
#endif

#ifdef CONFIG_RECORD_CLIENT_HOST
	PRINT_SINGL_ARG("    client_host_sniffer_enable: ", priv->pmib->miscEntry.client_host_sniffer_enable, "%d");
#endif

#ifdef TX_EARLY_MODE
	PRINT_SINGL_ARG("    em_waitq_on: ", GET_ROOT(priv)->pshare->em_waitq_on, "%d");
#endif
#ifdef SUPPORT_MONITOR
	PRINT_SINGL_ARG("	 chan_switch_time: ", priv->pmib->miscEntry.chan_switch_time, "%d");
	PRINT_SINGL_ARG("	 chan_switch_disable: ", priv->pmib->miscEntry.chan_switch_disable, "%d");
	PRINT_SINGL_ARG("	 pkt_filter_len: ", priv->pmib->miscEntry.pkt_filter_len, "%d");
#endif
#if defined(WIFI_11N_2040_COEXIST_EXT)
    PRINT_BITMAP_ARG("    80M map", priv->pshare->_80m_staMap);
	PRINT_BITMAP_ARG("    40M map", priv->pshare->_40m_staMap);
#endif
	PRINT_BITMAP_ARG("    dynamic MIMO ps", priv->pshare->mimo_ps_dynamic_sta);

	PRINT_SINGL_ARG("    RxTag mismatch:   ", priv->pshare->RxTagMismatchCount, "%d");
	PRINT_SINGL_ARG("    H2C full ctr: ", priv->pshare->h2c_box_full, "%d");
	PRINT_SINGL_ARG("    paused STA: ", priv->pshare->paused_sta_num, "%u");
	PRINT_SINGL_ARG("    Unlock pwr: ", priv->pshare->unlock_counter1, "%u");
	PRINT_SINGL_ARG("    Unlock timer: ", priv->pshare->unlock_counter2, "%u");
	PRINT_SINGL_ARG("    lock counter: ", priv->pshare->lock_counter, "%u");
#ifdef _OUTSRC_COEXIST
	if(IS_OUTSRC_CHIP(priv))
#endif
	{
		PRINT_SINGL_ARG("    False alarm: ", ODMPTR->FalseAlmCnt.Cnt_all, "%d");
		PRINT_SINGL_ARG("    CCA: ", ODMPTR->FalseAlmCnt.Cnt_CCA_all, "%d");
	}
#if !defined(USE_OUT_SRC) || defined(_OUTSRC_COEXIST)
#ifdef _OUTSRC_COEXIST
	if(!IS_OUTSRC_CHIP(priv))
#endif
	{
		PRINT_SINGL_ARG("    False alarm: ", priv->pshare->FA_total_cnt, "%d");
		PRINT_SINGL_ARG("    CCA: ", priv->pshare->CCA_total_cnt, "%d");
	}
#endif
#ifdef SSID_PRIORITY_SUPPORT
	PRINT_SINGL_ARG("	 Manual Priority: ", priv->pmib->miscEntry.manual_priority, "%d");
#endif
	return pos;
}


#ifdef WIFI_SIMPLE_CONFIG
static int rtl8192cd_proc_mib_wsc(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;

	PRINT_ONE("  wscEntry...", "%s", 1);
	PRINT_SINGL_ARG("    wsc_enable: ", priv->pmib->wscEntry.wsc_enable, "%d");
	PRINT_ARRAY_ARG("    beacon_ie: ",
			priv->pmib->wscEntry.beacon_ie, "%02x", priv->pmib->wscEntry.beacon_ielen);
	PRINT_SINGL_ARG("    beacon_ielen: ", priv->pmib->wscEntry.beacon_ielen, "%d");
	PRINT_ARRAY_ARG("    probe_rsp_ie: ",
			priv->pmib->wscEntry.probe_rsp_ie, "%02x", priv->pmib->wscEntry.probe_rsp_ielen);
	PRINT_SINGL_ARG("    probe_rsp_ielen: ", priv->pmib->wscEntry.probe_rsp_ielen, "%d");
	PRINT_ARRAY_ARG("    probe_req_ie: ",
			priv->pmib->wscEntry.probe_req_ie, "%02x", priv->pmib->wscEntry.probe_req_ielen);
	PRINT_SINGL_ARG("    probe_req_ielen: ", priv->pmib->wscEntry.probe_req_ielen, "%d");
	PRINT_ARRAY_ARG("    assoc_ie: ",
			priv->pmib->wscEntry.assoc_ie, "%02x", priv->pmib->wscEntry.assoc_ielen);
	PRINT_SINGL_ARG("    assoc_ielen: ", priv->pmib->wscEntry.assoc_ielen, "%d");

	return pos;
}
#endif

#ifdef WLANHAL_MACDM

#if 1
#define MACDM_MAX_PROC_ARG_NUM  7
int rtl8192cd_proc_macdm_write(struct file *file, const char *buffer,
				unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	char                tmp[60], command[8], action[10];
	unsigned int        num;
    unsigned int        mode_sel, state_sel, rssi_idx, threshold, state_idx, reg_idx, reg_offset, reg_value;
    PHAL_DATA_TYPE      pHalData = _GET_HAL_DATA(priv);

	if (buffer && !copy_from_user(tmp, buffer, 32)) {
		num = sscanf(tmp, "%s %s", command, action);

		if (num == 0) {
			panic_printk("argument num: 0 (invalid)\n");
			return num;
		}
        else if (num > MACDM_MAX_PROC_ARG_NUM) {
            panic_printk("argument num: %d (invalid)\n", num);
			return num;
        }
	}

	panic_printk("Command: [%s], action: [%s]\n", command, action);

	if (!memcmp(command, "config", 6)) {
        if (!memcmp(action, "mode", 4)) {
    		num = sscanf(tmp, "%s %s %u", command, action, &mode_sel);

            pHalData->MACDM_Mode_Sel = mode_sel;

            if (num != 3) {
                panic_printk("argument num:%d (invalid, should be 3)\n", num);
            }
        }
        else if (!memcmp(action, "thrs", 4)) {
    		num = sscanf(tmp, "%s %s %u %u %u", command, action, &state_sel, &rssi_idx, &threshold);

            pHalData->MACDM_stateThrs[state_sel][rssi_idx] = threshold;

            if (num != 5) {
                panic_printk("argument num:%d (invalid, should be 5)\n", num);
            }
        }
        else if (!memcmp(action, "table", 5)) {
    		num = sscanf(tmp, "%s %s %u %u %u %x %x", command, action, &state_idx, &rssi_idx, &reg_idx, &reg_offset, &reg_value);

            pHalData->MACDM_Table[state_idx][rssi_idx][reg_idx].offset   =  reg_offset;
            pHalData->MACDM_Table[state_idx][rssi_idx][reg_idx].value    =  reg_value;

            if (num != MACDM_MAX_PROC_ARG_NUM) {
                panic_printk("argument num:%d (invalid, should be %d)\n", num, MACDM_MAX_PROC_ARG_NUM);
            }
        }
        else if (!memcmp(action, "help", 4)) {
            panic_printk("Command example:\n");
            panic_printk("  config mode [mode_sel(0~3)]\n");
            panic_printk("  config thrs [state_sel(0~3)] [rssi_idx(0~2)] [threshold(dec)]\n");
            panic_printk("  config table [state_idx(0~2)] [rssi_idx(0~2)] [reg_idx(0~29)] [reg_offset(hex)] [reg_value(hex)]\n");
        }
        else {
            panic_printk("action code: %s (invalid, should be mode/thrs/table/help)\n", action);
        }
	}
    else {
        panic_printk("Command not supported!, please type 'config help'\n");
    }

	return count;
}
#endif

static int rtl8192cd_proc_macdm(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;
    PHAL_DATA_TYPE              pHalData    = _GET_HAL_DATA(priv);
    unsigned int                stateThrs_idx, state_idx, rssi_idx, reg_idx;


	PRINT_ONE("  MACDM...", "%s", 1);
	PRINT_SINGL_ARG("    MACDM_Mode: ", pHalData->MACDM_Mode_Sel, "%d");
    PRINT_SINGL_ARG("    MACDM_state: ", pHalData->MACDM_State, "%d");

    //Threshold
    for (stateThrs_idx=0; stateThrs_idx<MACDM_TP_THRS_MAX_NUM; stateThrs_idx++) {
        for (rssi_idx=0; rssi_idx<RSSI_LVL_MAX_NUM; rssi_idx++) {
            PRINT_ONE(stateThrs_idx, "MACDM_stateThrs[0x%x]", 0);
            PRINT_ONE(rssi_idx, "[0x%x]:", 0);
            PRINT_ONE(pHalData->MACDM_stateThrs[stateThrs_idx][rssi_idx], "0x%x", 1);
        }
    }

    //Table
    for (state_idx=0; state_idx<MACDM_TP_STATE_MAX_NUM; state_idx++) {
        PRINT_SINGL_ARG("\n    MACDM_Table, state: ", state_idx, "%d");
        for(rssi_idx=0; rssi_idx<RSSI_LVL_MAX_NUM; rssi_idx++) {
            PRINT_SINGL_ARG("    MACDM_Table, rssi: ", rssi_idx, "%d");

            for(reg_idx=0; reg_idx<MAX_MACDM_REG_NUM; reg_idx++) {
                if (pHalData->MACDM_Table[state_idx][rssi_idx][reg_idx].offset == 0xffff) {
                    break;
                }

                PRINT_ONE(pHalData->MACDM_Table[state_idx][rssi_idx][reg_idx].offset, "Reg: 0x%x, ", 0);
                PRINT_ONE(pHalData->MACDM_Table[state_idx][rssi_idx][reg_idx].value, "Value: 0x%x", 1);
            }
        }
    }

	return pos;
}
#endif //WLANHAL_MACDM
static int rtl8192cd_proc_mib_all(struct seq_file *s, void *data)
{
    struct net_device *dev = PROC_GET_DEV();
    struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);

    int size;


    seq_printf(s, "  Make info: v%d.%d (%s)\n", DRV_VERSION_H, DRV_VERSION_L, DRV_RELDATE);
    {
            seq_printf(s, "  RTL8192 firmware version: %d.%d,  built time: %02x-%02x %02x:%02x\n", priv->pshare->fw_version,
                priv->pshare->fw_sub_version, priv->pshare->fw_date_month, priv->pshare->fw_date_day,
                priv->pshare->fw_date_hour, priv->pshare->fw_date_minute);
    }

    rtl8192cd_proc_mib_rf(s, data);
    rtl8192cd_proc_mib_operation(s, data);
    rtl8192cd_proc_mib_staconfig(s, data);
    rtl8192cd_proc_mib_dkeytbl(s, data);
    rtl8192cd_proc_mib_auth(s, data);
    rtl8192cd_proc_mib_gkeytbl(s, data);
    rtl8192cd_proc_mib_bssdesc(s, data);
    rtl8192cd_proc_mib_erp(s, data);
    rtl8192cd_proc_mib_misc(s, data);
#ifdef BT_COEXIST
	rtl8192cd_proc_bt_coexist(s, data);
#endif
#ifdef WIFI_SIMPLE_CONFIG
    rtl8192cd_proc_mib_wsc(s, data);
#endif
    rtl8192cd_proc_probe_info(s, data);

#ifdef HS2_SUPPORT
    rtl8192cd_proc_mib_hs2(s, data);
#endif


    rtl8192cd_proc_mib_brext(s, data);

#if (BEAMFORMING_SUPPORT == 1)
	rtl8192cd_proc_mib_txbf(s, data);
#endif
    rtl8192cd_proc_mib_DFS(s, data);

    rtl8192cd_proc_mib_rf_ac(s, data);

    size = rtl8192cd_proc_mib_11n(s, data);

#ifdef CONFIG_RTK_VLAN_SUPPORT
    rtl8192cd_proc_vlan_read(s, data);
#endif

#ifdef SUPPORT_MULTI_PROFILE
    rtl8192cd_proc_mib_ap_profile(s, data);
#endif
#ifdef THERMAL_CONTROL
	rtl8192cd_proc_thermal_control(s, data);
#endif

    return 0;
}

#define CHECK_LEN_B do {} while(0)

//static int dump_one_stainfo(int num, struct stat_info *pstat, char *buf, char **start,
//			off_t offset, int length, int *eof, void *data)
static int dump_one_stainfo(struct seq_file *s, int num, struct rtl8192cd_priv *priv, struct stat_info *pstat, int *rc)
{
	int pos = 0, idx = 0;
	unsigned int m, n;
	char tmp[32];
	//unsigned short rate;
	unsigned char tmpbuf[16];
	unsigned char *rate;

#ifdef SW_TX_QUEUE
	unsigned int	swq_en = 0;
#endif

	PRINT_ONE(num,  " %d: stat_info...", 1);
	CHECK_LEN_B;
	PRINT_SINGL_ARG("    state: ", pstat->state, "%x");
	CHECK_LEN_B;
	PRINT_SINGL_ARG("    AuthAlgrthm: ", pstat->AuthAlgrthm, "%d");
	CHECK_LEN_B;
	PRINT_SINGL_ARG("    ieee8021x_ctrlport: ", pstat->ieee8021x_ctrlport, "%d");
	CHECK_LEN_B;
#ifdef CONFIG_IEEE80211W
	#ifdef CONFIG_IEEE80211W_CLI
	PRINT_SINGL_ARG("	 bss support PMF : ", priv->bss_support_pmf, "%d");
	CHECK_LEN_B;
	#endif
	PRINT_SINGL_ARG("    isPMF: ", pstat->isPMF, "%d");
	CHECK_LEN_B;
#endif
	PRINT_ARRAY_ARG("    hwaddr: ",	pstat->hwaddr, "%02x", MACADDRLEN);
	CHECK_LEN_B;
	PRINT_ARRAY_ARG("    bssrateset: ", pstat->bssrateset, "%02x", pstat->bssratelen);
	CHECK_LEN_B;
	PRINT_SINGL_ARG("    aid: ", pstat->aid, "%d");
	CHECK_LEN_B;
	PRINT_SINGL_ARG("    tx_bytes: ", pstat->tx_bytes, "%u");
	CHECK_LEN_B;
	PRINT_SINGL_ARG("    rx_bytes: ", pstat->rx_bytes, "%u");
	CHECK_LEN_B;

#ifdef CAM_SWAP
	PRINT_SINGL_ARG("	 delta_tx_bytes: ", pstat->traffic.delta_tx_bytes, "%u");
	CHECK_LEN_B;
	PRINT_SINGL_ARG("	 delta_rx_bytes: ", pstat->traffic.delta_rx_bytes, "%u");
	CHECK_LEN_B;

	PRINT_SINGL_ARG("	 level: ", pstat->traffic.level, "%u");
	CHECK_LEN_B;

	PRINT_SINGL_ARG("    keyInCam: ", (pstat->dot11KeyMapping.keyInCam? "yes" : "no"), "%s");
	CHECK_LEN_B;
#endif

#ifdef RADIUS_ACCOUNTING
	PRINT_SINGL_ARG("    tx_bytes per minute: ", pstat->tx_bytes_1m, "%u");
	CHECK_LEN_B;
	PRINT_SINGL_ARG("    rx_bytes per minute: ", pstat->rx_bytes_1m, "%u");
	CHECK_LEN_B;
#endif
	PRINT_SINGL_ARG("    tx_pkts: ", pstat->tx_pkts, "%u");
	CHECK_LEN_B;
	PRINT_SINGL_ARG("    rx_pkts: ", pstat->rx_pkts, "%u");
	CHECK_LEN_B;
	PRINT_SINGL_ARG("    tx_fail: ", pstat->tx_fail, "%u");
	CHECK_LEN_B;
#ifdef TXRETRY_CNT
	if(is_support_TxRetryCnt(priv)) {
	//PRINT_SINGL_ARG("    cur_tx_retry_pkts: ", pstat->cur_tx_retry_pkts, "%u");
	//CHECK_LEN_B;
	PRINT_SINGL_ARG("    cur_tx_retry_cnt: ", pstat->cur_tx_retry_cnt, "%u");
	CHECK_LEN_B;
	//PRINT_SINGL_ARG("    total_tx_retry_pkts: ", pstat->total_tx_retry_pkts, "%u");
	//CHECK_LEN_B;
	PRINT_SINGL_ARG("    total_tx_retry_cnt: ", pstat->total_tx_retry_cnt, "%u");
	CHECK_LEN_B;
	PRINT_SINGL_ARG("    tx_retry_ratio: ", pstat->txretry_ratio, "%u");
	CHECK_LEN_B;
	}
#endif
	PRINT_SINGL_ARG("    tx_avarage:    ", pstat->tx_avarage, "%u");
	CHECK_LEN_B;
	PRINT_SINGL_ARG("    rx_avarage:    ", pstat->rx_avarage, "%u");
	CHECK_LEN_B;
	PRINT_SINGL_ARG("    dz_queue_len: ", skb_queue_len(&pstat->dz_queue), "%u");
	CHECK_LEN_B;
#ifdef SW_TX_QUEUE
	//for debug
#if 0
	PRINT_SINGL_ARG("    tx_av: ", pstat->tx_avarage, "%u");
        CHECK_LEN_B;
        PRINT_SINGL_ARG("    tx_agn: ", pstat->aggn_cnt, "%u");
        CHECK_LEN_B;
        PRINT_SINGL_ARG("    tx_be_tout: ", pstat->be_timeout_cnt, "%u");
        CHECK_LEN_B;
        PRINT_SINGL_ARG("    tx_bk_tout: ", pstat->bk_timeout_cnt, "%u");
        CHECK_LEN_B;
        PRINT_SINGL_ARG("    tx_vi_tout: ", pstat->vi_timeout_cnt, "%u");
        CHECK_LEN_B;
        PRINT_SINGL_ARG("    tx_vo_tout: ", pstat->vo_timeout_cnt, "%u");
        CHECK_LEN_B;
#endif
#endif
	PRINT_ONE("    rssi: ", "%s", 0);
	PRINT_ONE(pstat->rssi, "%u", 0);
	PRINT_ONE(pstat->rf_info.mimorssi[0], " (%u", 0);
	PRINT_ONE(pstat->rf_info.mimorssi[1], " %u", 0);
	PRINT_ONE(")", "%s", 1);
	CHECK_LEN_B;

	{
		PRINT_SINGL_ARG("    expired_time: ", pstat->expire_to, "%d");
		CHECK_LEN_B;
	}

	PRINT_SINGL_ARG("    sleep: ", (!list_empty(&pstat->sleep_list) ? "yes" : "no"), "%s");
	CHECK_LEN_B;
	PRINT_SINGL_ARG("    sta_in_firmware_mem: ",(pstat->sta_in_firmware == -1 ? "removed" : ((pstat->sta_in_firmware) == 0 ? "no":"yes")), "%s" );
	CHECK_LEN_B;
	PRINT_SINGL_ARG("    remapped_aid: ", pstat->remapped_aid, "%d");
	CHECK_LEN_B;
#if (MU_BEAMFORMING_SUPPORT == 1)
	PRINT_SINGL_ARG("    force_rate: ", pstat->force_rate, "%d");
	CHECK_LEN_B;

	if(pstat->muPartner) {
		PRINT_SINGL_ARG("	 mu Partner aid: ", pstat->muPartner->aid, "%d");
	} else {
		PRINT_ONE("	 mu muPartner aid: NULL\n", "%s", 0);
	}
	CHECK_LEN_B;

	PRINT_SINGL_ARG("    mu_tx_rate: ", pstat->mu_tx_rate, "%d");
	CHECK_LEN_B;

	PRINT_SINGL_ARG("    mu_deq_num: ", pstat->mu_deq_num, "%d");
	CHECK_LEN_B;
	PRINT_SINGL_ARG("    isSendNDPA: ", pstat->isSendNDPA, "%d");
	CHECK_LEN_B;
	PRINT_SINGL_ARG("    isRssiApplyMU: ", pstat->isRssiApplyMU, "%d");
	CHECK_LEN_B;
	PRINT_SINGL_ARG("    inTXBFEntry: ", pstat->inTXBFEntry, "%d");
	CHECK_LEN_B;
#endif

#ifdef RTK_AC_SUPPORT  //vht rate , todo, dump vht rates in Mbps
	if(pstat->current_tx_rate >= VHT_RATE_ID){
		int rate = query_vht_rate(pstat);
		PRINT_ONE("    current_tx_rate: VHT NSS", "%s", 0);
		PRINT_ONE(((pstat->current_tx_rate - VHT_RATE_ID)/10)+1, "%d", 0);
		PRINT_ONE("-MCS", "%s", 0);
		PRINT_ONE((pstat->current_tx_rate - VHT_RATE_ID)%10, "%d", 0);
		PRINT_ONE(rate, "  %d", 1);
		CHECK_LEN_B;
	}
	else
#endif
	if (is_MCS_rate(pstat->current_tx_rate)) {
		PRINT_ONE("    current_tx_rate: MCS", "%s", 0);
		PRINT_ONE((pstat->current_tx_rate - HT_RATE_ID), "%d", 0);
		rate = (unsigned char *)MCS_DATA_RATEStr[pstat->tx_bw][(pstat->ht_current_tx_info&BIT(1))?1:0][(pstat->current_tx_rate - HT_RATE_ID)];
		PRINT_ONE(rate, " %s", 1);
		CHECK_LEN_B;
	}
	else
	{
		PRINT_SINGL_ARG("    current_tx_rate: ", pstat->current_tx_rate/2, "%d");
		CHECK_LEN_B;
	}


#ifdef RTK_AC_SUPPORT  //vht rate , todo, dump vht rates in Mbps
	if(pstat->rx_rate >= VHT_RATE_ID){
		int rate = query_vht_rx_rate(pstat);
		PRINT_ONE("    rx_rate: VHT NSS", "%s", 0);
		PRINT_ONE(((pstat->rx_rate - VHT_RATE_ID)/10)+1, "%d", 0);
		PRINT_ONE("-MCS", "%s", 0);
		PRINT_ONE((pstat->rx_rate - VHT_RATE_ID)%10, "%d", 0);
		PRINT_ONE(rate, "  %d", 1);
		CHECK_LEN_B;
	}
	else
#endif

	if (is_MCS_rate(pstat->rx_rate)) {
		PRINT_ONE("    current_rx_rate: MCS", "%s", 0);
		PRINT_ONE((pstat->rx_rate - HT_RATE_ID), "%d", 0);
		rate = (unsigned char *)MCS_DATA_RATEStr[pstat->rx_bw&0x01][pstat->rx_splcp&0x01][(pstat->rx_rate - HT_RATE_ID)];
		PRINT_ONE(rate, " %s", 1);
		CHECK_LEN_B;
	}
	else
	{
		PRINT_SINGL_ARG("    current_rx_rate: ", pstat->rx_rate/2, "%d");
		CHECK_LEN_B;
	}

//8812_client tx_bw rx_bw ??
	PRINT_SINGL_ARG("    rx_bw: ", (0x1<<(pstat->rx_bw))*20, "%dM");
	CHECK_LEN_B;

	PRINT_SINGL_ARG("    tx_bw: ", (pstat->tx_bw ? (pstat->tx_bw*40) : 20), "%dM");
	//PRINT_SINGL_ARG("    tx_bw: ", (0x1<<(pstat->tx_bw))*20, "%dM");
	CHECK_LEN_B;

	PRINT_SINGL_ARG("    tx_bw_bak: ", (pstat->tx_bw_bak ? (pstat->tx_bw_bak*40) : 20), "%dM");
	CHECK_LEN_B;

#ifdef RTK_AC_SUPPORT
	PRINT_SINGL_ARG("    nss: ", (pstat->nss), "%d");
	CHECK_LEN_B;
	PRINT_SINGL_ARG("    support_mcs: ", cpu_to_le32(pstat->vht_cap_buf.vht_support_mcs[0]), "%x");
	CHECK_LEN_B;
	if(GET_CHIP_VER(priv) == VERSION_8812E) {
	    PRINT_SINGL_ARG("    shrink_ac_bw: ", pstat->shrink_ac_bw, "%d");
		CHECK_LEN_B;
	}
#endif

	PRINT_SINGL_ARG("    hp_level: ", pstat->hp_level, "%d");
	CHECK_LEN_B;

	if(pstat->useCts2self) {
		PRINT_SINGL_ARG("    rts/cts2self: ", "cts2self", "%s");
	} else {
		PRINT_SINGL_ARG("    rts/cts2self: ", "rts", "%s");
	}

	CHECK_LEN_B;

#ifdef WIFI_WMM
	PRINT_SINGL_ARG("    QoS Enable: ", pstat->QosEnabled, "%d");
	CHECK_LEN_B;
#ifdef WMM_APSD
	PRINT_SINGL_ARG("    APSD bitmap: ", pstat->apsd_bitmap, "0x%01x");
	CHECK_LEN_B;
#endif
#endif

	PRINT_SINGL_ARG("    tx_ra_bitmap: ", pstat->tx_ra_bitmap, "0x%08x");

	if (pstat->is_realtek_sta)
		sprintf((char *)tmpbuf, "Realtek");

	else if (pstat->IOTPeer==HT_IOT_PEER_BROADCOM)
		sprintf((char *)tmpbuf, "Broadcom");
	else if (pstat->IOTPeer==HT_IOT_PEER_MARVELL)
		sprintf((char *)tmpbuf, "Marvell");
	else if (pstat->IOTPeer==HT_IOT_PEER_INTEL)
		sprintf((char *)tmpbuf, "Intel");
	else if (pstat->IOTPeer==HT_IOT_PEER_RALINK)
		sprintf((char *)tmpbuf, "Ralink");
	else if (pstat->IOTPeer==HT_IOT_PEER_HTC)
		sprintf((char *)tmpbuf, "HTC");
	else

		sprintf((char *)tmpbuf, "--");
	PRINT_SINGL_ARG("    Chip Vendor: ", tmpbuf, "%s");
	CHECK_LEN_B;
#ifdef RTK_WOW
	if (pstat->is_realtek_sta) {
		PRINT_SINGL_ARG("    is_rtk_wow_sta: ", (pstat->is_rtk_wow_sta? "yes":"no"), "%s");
		CHECK_LEN_B;
	}
#endif

	m = pstat->link_time / 86400;
	n = pstat->link_time % 86400;
	if (m)	idx += sprintf(tmp, "%d day ", m);
	m = n / 3600;
	n = n % 3600;
	if (m)	idx += sprintf(tmp+idx, "%d hr ", m);
	m = n / 60;
	n = n % 60;
	if (m)	idx += sprintf(tmp+idx, "%d min ", m);
	idx += sprintf(tmp+idx, "%d sec ", n);
	PRINT_SINGL_ARG("    link_time: ", tmp, "%s");
	CHECK_LEN_B;

	if (pstat->private_ie_len) {
		PRINT_ARRAY_ARG("    private_ie: ", pstat->private_ie, "%02x", pstat->private_ie_len);
		CHECK_LEN_B;
	}
	if (pstat->ht_cap_len) {
		unsigned char *pbuf = (unsigned char *)&pstat->ht_cap_buf;
		PRINT_ARRAY_ARG("    ht_cap: ", pbuf, "%02x", pstat->ht_cap_len);
		CHECK_LEN_B;

		PRINT_ONE("    11n MIMO ps: ", "%s", 0);
		if (!(pstat->MIMO_ps)) {
			PRINT_ONE("no limit", "%s", 1);
		}
		else {
			PRINT_ONE(((pstat->MIMO_ps&BIT(0))?"static":"dynamic"), "%s", 1);
		}
		CHECK_LEN_B;

		PRINT_SINGL_ARG("    Is_8K_AMSDU: ", pstat->is_8k_amsdu, "%d");
		CHECK_LEN_B;
		PRINT_SINGL_ARG("    amsdu_level: ", pstat->amsdu_level, "%d");
		CHECK_LEN_B;
		PRINT_SINGL_ARG("    diffAmpduSz: 0x", pstat->diffAmpduSz, "%x");
		CHECK_LEN_B;
		PRINT_SINGL_ARG("    AMSDU_AMPDU_support: ", pstat->AMSDU_AMPDU_support, "%d");
		CHECK_LEN_B;
		switch (pstat->aggre_mthd) {
		case AGGRE_MTHD_MPDU:
			sprintf(tmp, "AMPDU");
			break;
		case AGGRE_MTHD_MSDU:
			sprintf(tmp, "AMSDU");
			break;
		case AGGRE_MTHD_MPDU_AMSDU:
			sprintf(tmp, "AMPDU_AMSDU");
			break;
		default:
			sprintf(tmp, "None");
			break;
		}
		PRINT_SINGL_ARG("    aggre mthd: ", tmp, "%s");
		CHECK_LEN_B;

#ifdef SW_TX_QUEUE
		for(idx=0; idx<8; idx++){
			if(pstat->swq.swq_en[idx])
				swq_en |= BIT(idx);
		}
		PRINT_SINGL_ARG("    swq_en: ", swq_en, "0x%x");
#endif
		PRINT_SINGL_ARG("    cnt_sleep: ", pstat->cnt_sleep, "%d");

#if (BEAMFORMING_SUPPORT == 1)
		PRINT_SINGL_ARG("    ht beamformee: ", (cpu_to_le32(pstat->ht_cap_buf.txbf_cap) & 0x8)?"Y":"N", "%s");
		CHECK_LEN_B;
		PRINT_SINGL_ARG("    ht beamformer: ", (cpu_to_le32(pstat->ht_cap_buf.txbf_cap) & 0x10)?"Y":"N", "%s");
		CHECK_LEN_B;
#endif

#ifdef _DEBUG_RTL8192CD_
		PRINT_ONE("    ch_width: ", "%s", 0);
		PRINT_ONE((pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_))?"40M":"20M", "%s", 1);
		CHECK_LEN_B;
		PRINT_ONE("    ampdu_mf: ", "%s", 0);
		PRINT_ONE(pstat->ht_cap_buf.ampdu_para & 0x03, "%d", 1);
		CHECK_LEN_B;
		PRINT_ONE("    ampdu_amd: ", "%s", 0);
		PRINT_ONE((pstat->ht_cap_buf.ampdu_para & _HTCAP_AMPDU_SPC_MASK_) >> _HTCAP_AMPDU_SPC_SHIFT_, "%d", 1);
		CHECK_LEN_B;
#endif
	}
	else {
		PRINT_ONE("    ht_cap: none", "%s", 1);
		CHECK_LEN_B;
	}


//AC capability
#ifdef RTK_AC_SUPPORT
	if (pstat->vht_cap_len) {
		unsigned char *pbuf = (unsigned char *)&pstat->vht_cap_buf;
		PRINT_ARRAY_ARG("    vht_cap_ie: ", pbuf, "%02x", pstat->vht_cap_len);
		CHECK_LEN_B;

#if (BEAMFORMING_SUPPORT == 1)
		PRINT_SINGL_ARG("    vht beamformee: ", (cpu_to_le32(pstat->vht_cap_buf.vht_cap_info) & BIT(SU_BFEE_S))?"Y":"N", "%s");
		CHECK_LEN_B;
		PRINT_SINGL_ARG("    vht beamformer: ", (cpu_to_le32(pstat->vht_cap_buf.vht_cap_info) & BIT(SU_BFER_S))?"Y":"N", "%s");
		CHECK_LEN_B;
#endif

	}
	else {
		PRINT_ONE("    vht_cap_ie: none", "%s", 1);
		CHECK_LEN_B;
	}

	if (pstat->vht_oper_len) {
		unsigned char *pbuf = (unsigned char *)&pstat->vht_oper_buf;
		PRINT_ARRAY_ARG("    vht_oper_ie: ", pbuf, "%02x", pstat->vht_oper_len);
		CHECK_LEN_B;
	}
	else {
		PRINT_ONE("    vht_oper_ie: none", "%s", 1);
		CHECK_LEN_B;
	}

	PRINT_SINGL_ARG("    maxAggNum: ", pstat->maxAggNum, "%d");
	CHECK_LEN_B;

#endif

#if (BEAMFORMING_SUPPORT == 1)
	if (priv->pmib->dot11RFEntry.txbf == 1)	{
		u1Byte					Idx = 0;
		PRT_BEAMFORMING_ENTRY	pEntry;
		pEntry = Beamforming_GetEntryByMacId(priv, pstat->aid, &Idx);
		PRINT_SINGL_ARG("    Activate Tx beamforming : ", (pEntry ? "Y" : "N"),"%s");
	}
#endif

#ifdef SUPPORT_TX_MCAST2UNI
	PRINT_SINGL_ARG("    ipmc_num: ", pstat->ipmc_num, "%d");
	CHECK_LEN_B;
	for (idx=0; idx<MAX_IP_MC_ENTRY; idx++) {
		if (pstat->ipmc[idx].used) {
			PRINT_ARRAY_ARG("    mcmac: ",	pstat->ipmc[idx].mcmac, "%02x", MACADDRLEN);
			CHECK_LEN_B;
		}
	}
#endif


    if(IS_HAL_CHIP(priv)) {
        // below is add by SD1 Eric 2013/5/6
        // For 92E,8881 MACID Sleep and MACID drop status in driver and SRAM status

        u1Byte bDrop = 0; // txrpt need allocate 16byte
        PRINT_SINGL_ARG("    bSleep: ", pstat->txpause_flag,"%02x");
        PRINT_SINGL_ARG("    bDrop: ", pstat->bDrop, "%02x");

        if(RT_STATUS_FAILURE ==  GET_HAL_INTERFACE(priv)->GetTxRPTHandler(priv, pstat->aid,TXRPT_VAR_PKT_DROP, 0 , &bDrop))
        {
            PRINT_SINGL_ARG("    FW bDrop: ", 0xff,"%02x");
        }
        else
        {
            PRINT_SINGL_ARG("    FW bDrop: ", bDrop,"%02x");
        }
    }


#ifdef CLIENT_MODE
	if (pstat->ht_ie_len) {
		unsigned char *pbuf = (unsigned char *)&pstat->ht_ie_buf;
		PRINT_ARRAY_ARG("    ht_ie: ", pbuf, "%02x", pstat->ht_ie_len);
		CHECK_LEN_B;
	}
	else {
		PRINT_ONE("    ht_ie: none", "%s", 1);
		CHECK_LEN_B;
	}

#ifdef HS2_SUPPORT
/* Hotspot 2.0 Release 1 */
	PRINT_ARRAY_ARG("    sta ip:", pstat->sta_ip, "%d.", 4);
	CHECK_LEN_B;
    memset(tmpbuf,'\0',64);
    for(idx=0;idx<pstat->v6ipCount;idx++){
        sprintf(tmpbuf, "[%08X][%08X][%08X][%08X]",pstat->sta_v6ip[idx].s6_addr32[0],pstat->sta_v6ip[idx].s6_addr32[1],pstat->sta_v6ip[idx].s6_addr32[2],pstat->sta_v6ip[idx].s6_addr32[3]);
		PRINT_SINGL_ARG("    ipv6: ", tmpbuf, "%s");
    }
    CHECK_LEN_B;
    memset(tmpbuf,'\0',64);
#endif

#endif


	if ((pstat->WirelessMode & WIRELESS_MODE_AC_5G) || (pstat->WirelessMode & WIRELESS_MODE_AC_24G))
		sprintf(tmpbuf, "AC");
	else if (pstat->WirelessMode & WIRELESS_MODE_N_5G)
		sprintf(tmpbuf, "AN");
	else if (pstat->WirelessMode & WIRELESS_MODE_N_24G)
		sprintf(tmpbuf, "BGN");
	else if (pstat->WirelessMode & WIRELESS_MODE_G)
		sprintf(tmpbuf, "BG");
	else if (pstat->WirelessMode & WIRELESS_MODE_A)
		sprintf(tmpbuf, "A");
	else if (pstat->WirelessMode & WIRELESS_MODE_B)
		sprintf(tmpbuf, "B");
	else
		sprintf(tmpbuf, "Unknown WirelessMode:%x\n",pstat->WirelessMode);
	PRINT_SINGL_ARG("    wireless mode: ", tmpbuf, "%s");
	CHECK_LEN_B;

#ifdef A4_STA
    if(priv->pshare->rf_ft_var.a4_enable) {
		PRINT_SINGL_ARG("    A4 STA: ", (pstat->state & WIFI_A4_STA)?"Y":"N", "%s");
		CHECK_LEN_B;
    }
#endif

#ifdef TV_MODE
    if (OPMODE & WIFI_AP_STATE && priv->tv_mode_status & BIT1) /*if tv_mode is auto*/
    {
        PRINT_SINGL_ARG("    TV AUTO: ", pstat->tv_auto_support?"Y":"N", "%s");
        CHECK_LEN_B;
    }
#endif


#ifdef CONFIG_VERIWAVE_MU_CHECK
	PRINT_SINGL_ARG("    isVeriwaveInValidSTA: ", pstat->isVeriwaveInValidSTA,"%d");
	PRINT_ARRAY_ARG("    mu_csi_data:", pstat->mu_csi_data, "%02x ", 3);
#endif

#ifdef CONFIG_IEEE80211V
    if(WNM_ENABLE) {
    	PRINT_SINGL_ARG("    BSS Trans Support: ", pstat->bssTransSupport, "%d");
		PRINT_SINGL_ARG("    BSS Trans Rsp Status Code: ", pstat->bssTransStatusCode, "%d");
		PRINT_SINGL_ARG("    BSS Trans Rejection Count: ", pstat->bssTransRejectionCount, "%d");
		PRINT_SINGL_ARG("    BSS Trans Expired  Time: ", pstat->bssTransExpiredTime, "%d");
		PRINT_SINGL_ARG("    BSS Trans Event Triggered: ", pstat->bssTransTriggered, "%d");
		if(priv->pmib->wnmEntry.Is11kDaemonOn)
			PRINT_SINGL_ARG("    Rev Neighbor Report: ", pstat->rcvNeighborReport, "%d");
	}
#endif


#ifdef CONFIG_RECORD_CLIENT_HOST
	char client_ip[24];
	if(priv->pmib->miscEntry.client_host_sniffer_enable){
		if(pstat->client_host_ip[0]){
			PRINT_SINGL_ARG("    Client host name: ", pstat->client_host_name, "%s");
			snprintf(client_ip,24,"%d:%d:%d:%d",pstat->client_host_ip[0],pstat->client_host_ip[1],pstat->client_host_ip[2],pstat->client_host_ip[3]);
			PRINT_SINGL_ARG("    Client host ip: ", client_ip, "%s");
		}
	}
#endif

	PRINT_SINGL_ARG("    rx_snr0: ", pstat->rx_snr[0], "%d ");
	CHECK_LEN_B;
	PRINT_SINGL_ARG("    rx_snr1: ", pstat->rx_snr[1], "%d ");
	CHECK_LEN_B;
	PRINT_SINGL_ARG("    rx_snr2: ", pstat->rx_snr[2], "%d ");
	CHECK_LEN_B;
	PRINT_SINGL_ARG("    rx_snr3: ", pstat->rx_snr[3], "%d ");
	CHECK_LEN_B;

	PRINT_ONE("", "%s", 1);

	*rc = 1; //finished, assign okay to return code.
	return pos;
}

static int rtl8192cd_proc_stainfo(struct seq_file *s, void *data)
{
    struct net_device *dev = PROC_GET_DEV();
    struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
    int len = 0, rc=1;
    int size, num=1;
    struct list_head *phead, *plist;
    struct stat_info *pstat;

#if 1//!defined(SMP_SYNC) || (defined(CONFIG_USB_HCI) || defined(CONFIG_SDIO_HCI))
    unsigned long flags=0;
#endif


    SAVE_INT_AND_CLI(flags);
    SMP_LOCK_ASOC_LIST(flags);

    // !!! it REALLY waste the cpu time ==> it will do from beginning when kernel call rtl8192cd_proc_stainfo() function again and again.
    seq_printf(s, "-- STA info table -- (active: %d)\n", priv->assoc_num);

    phead = &priv->asoc_list;
    if (!(priv->drv_state & DRV_STATE_OPEN) || list_empty(phead)) {
        goto _ret;
    }

    plist = phead->next;
    while (plist != phead) {
        pstat = list_entry(plist, struct stat_info, asoc_list);
        //size = dump_one_stainfo(num++, pstat, buf+len, start, offset, length, eof, data);
        size = dump_one_stainfo(s, num++, priv, pstat, &rc);
        CHECK_LEN;


        plist = plist->next;
    }


_ret:
    SMP_UNLOCK_ASOC_LIST(flags);
    RESTORE_INT(flags);


    return len;
}


#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
__NOMIPS16
#endif
static int dump_one_sta_keyinfo(struct seq_file *s, int num, struct stat_info *pstat)
{
	int pos = 0;
	unsigned char *ptr;

	PRINT_ONE(num,  " %d: stat_keyinfo...", 1);
	PRINT_ARRAY_ARG("    hwaddr: ",	pstat->hwaddr, "%02x", MACADDRLEN);
	PRINT_SINGL_ARG("    keyInCam: ", (pstat->dot11KeyMapping.keyInCam? "yes" : "no"), "%s");
	PRINT_SINGL_ARG("    dot11Privacy: ",
			pstat->dot11KeyMapping.dot11Privacy, "%d");
	PRINT_SINGL_ARG("    dot11EncryptKey.dot11TTKeyLen: ",
			pstat->dot11KeyMapping.dot11EncryptKey.dot11TTKeyLen, "%d");
	PRINT_SINGL_ARG("    dot11EncryptKey.dot11TMicKeyLen: ",
			pstat->dot11KeyMapping.dot11EncryptKey.dot11TMicKeyLen, "%d");
	PRINT_ARRAY_ARG("    dot11EncryptKey.dot11TTKey.skey: ",
			pstat->dot11KeyMapping.dot11EncryptKey.dot11TTKey.skey, "%02x", 16);
	PRINT_ARRAY_ARG("    dot11EncryptKey.dot11TMicKey1.skey: ",
			pstat->dot11KeyMapping.dot11EncryptKey.dot11TMicKey1.skey, "%02x", 16);
	PRINT_ARRAY_ARG("    dot11EncryptKey.dot11TMicKey2.skey: ",
			pstat->dot11KeyMapping.dot11EncryptKey.dot11TMicKey2.skey, "%02x", 16);
	ptr = (unsigned char *)&pstat->dot11KeyMapping.dot11EncryptKey.dot11TXPN48.val48;
	PRINT_ARRAY_ARG("    dot11EncryptKey.dot11TXPN48.val48: ", ptr, "%02x", 8);
	ptr = (unsigned char *)&pstat->dot11KeyMapping.dot11EncryptKey.dot11RXPN48.val48;
	PRINT_ARRAY_ARG("    dot11EncryptKey.dot11RXPN48.val48: ", ptr, "%02x", 8);

	PRINT_ONE("", "%s", 1);

	return pos;
}


#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
__NOMIPS16
#endif
static int rtl8192cd_proc_sta_keyinfo(struct seq_file *s, void *data)
{
    struct net_device *dev = PROC_GET_DEV();
    struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
    int len = 0;
    int size, num=1;
    struct list_head *phead, *plist;
    struct stat_info *pstat;

#if 1//!defined(SMP_SYNC) || (defined(CONFIG_USB_HCI) || defined(CONFIG_SDIO_HCI))
    unsigned long flags=0;
#endif

    SAVE_INT_AND_CLI(flags);
    SMP_LOCK_ASOC_LIST(flags);

    seq_printf(s, "-- STA key info table --\n");

    phead = &priv->asoc_list;
    if (!(priv->drv_state & DRV_STATE_OPEN) || list_empty(phead))
        goto _ret;

    plist = phead->next;
    while (plist != phead) {
        pstat = list_entry(plist, struct stat_info, asoc_list);
        plist = plist->next;
        size = dump_one_sta_keyinfo(s, num++, pstat);
        CHECK_LEN;
    }

_ret:
    SMP_UNLOCK_ASOC_LIST(flags);
    RESTORE_INT(flags);


    return len;
}

static int dump_one_sta_dbginfo(struct seq_file *s, int num, struct rtl8192cd_priv *priv, struct stat_info *pstat)
{
    int pos = 0;
#ifdef TX_SHORTCUT
    int i;
    struct tx_insn		*txsc_txcfg;
#endif

    PRINT_ARRAY_ARG("    hwaddr: ",	pstat->hwaddr, "%02x", MACADDRLEN);
#ifdef TX_SHORTCUT
    PRINT_SINGL_ARG("    tx_sc_pkts_lv1:  ", pstat->tx_sc_pkts_lv1, "%d");
    PRINT_SINGL_ARG("    tx_sc_pkts_lv2:  ", pstat->tx_sc_pkts_lv2, "%d");
    PRINT_SINGL_ARG("    tx_sc_pkts_slow: ", pstat->tx_sc_pkts_slow, "%u");
#ifdef SUPPORT_TX_AMSDU_SHORTCUT
    PRINT_SINGL_ARG("    tx_sc_amsdu_pkts_lv1:  ", pstat->tx_sc_amsdu_pkts_lv1, "%d");
    PRINT_SINGL_ARG("    tx_sc_amsdu_pkts_lv2:  ", pstat->tx_sc_amsdu_pkts_lv2, "%d");
    PRINT_SINGL_ARG("    tx_sc_amsdu_pkts_slow: ", pstat->tx_sc_amsdu_pkts_slow, "%u");
#endif
    PRINT_SINGL_ARG("    rx_sc_pkts:      ", pstat->rx_sc_pkts, "%d");
    PRINT_SINGL_ARG("    rx_sc_pkts_slow: ", pstat->rx_sc_pkts_slow, "%u");

    for(i = 0 ; i < TX_SC_ENTRY_NUM;i++) {
        txsc_txcfg = &pstat->tx_sc_ent[i].txcfg;
        if(txsc_txcfg->fr_len > 0) {

            PRINT_ONE("    TX shortcut ", "%s", 0);
            PRINT_ONE(i, "%d:", 0);
			PRINT_ARRAY(pstat->tx_sc_ent[i].ethhdr.daddr, "%02x", MACADDRLEN, 0);
			PRINT_ARRAY(pstat->tx_sc_ent[i].ethhdr.saddr, "%02x", MACADDRLEN, 0);
            PRINT_ONE(pstat->tx_sc_ent[i].ethhdr.type, " %04X", 0);

            #ifdef WLAN_HAL_HW_TX_SHORTCUT_REUSE_TXDESC
            if(IS_SUPPORT_WLAN_HAL_HW_TX_SHORTCUT_REUSE_TXDESC(priv) && pstat->tx_sc_hw_idx == i) {
                PRINT_ONE(" HW", "%s", 0);
            }
            #endif

            PRINT_ONE("", "%s", 1);
        }
    }
#endif

#if defined(RX_SHORTCUT)
	for(i = 0 ; i < RX_SC_ENTRY_NUM;i++) {
		PRINT_ONE("    RX shortcut ", "%s", 0);
		PRINT_ARRAY(pstat->rx_sc_ent[i].rx_ethhdr.daddr, "%02x", MACADDRLEN, 0);
		PRINT_ARRAY(pstat->rx_sc_ent[i].rx_ethhdr.saddr, "%02x", MACADDRLEN, 0);
#if defined(RX_RL_SHORTCUT)
		PRINT_ARRAY(pstat->rx_sc_ent[i].rx_wlanhdr.meshhdr.DestMACAddr, "%02x", MACADDRLEN, 0);
		PRINT_ARRAY(pstat->rx_sc_ent[i].rx_wlanhdr.meshhdr.SrcMACAddr, "%02x", MACADDRLEN, 0);
#endif
		PRINT_ONE("", "%s", 1);
	}
#endif


	PRINT_SINGL_ARG("    dz_queue_len: ", skb_queue_len(&pstat->dz_queue), "%u");
	PRINT_SINGL_ARG("    hp_level: ", pstat->hp_level, "%d");

#ifdef _DEBUG_RTL8192CD_
	PRINT_ONE("    ch_width:  ", "%s", 0);
	PRINT_ONE((pstat->ht_cap_buf.ht_cap_info & cpu_to_le16(_HTCAP_SUPPORT_CH_WDTH_))?"40M":"20M", "%s", 1);
	PRINT_ONE("    ampdu_mf:  ", "%s", 0);
	PRINT_ONE(pstat->ht_cap_buf.ampdu_para & 0x03, "%d", 1);
	PRINT_ONE("    ampdu_amd: ", "%s", 0);
	PRINT_ONE((pstat->ht_cap_buf.ampdu_para & _HTCAP_AMPDU_SPC_MASK_) >> _HTCAP_AMPDU_SPC_SHIFT_, "%d", 1);
#endif


	PRINT_SINGL_ARG("    tx amsdu timerovf:  ", pstat->tx_amsdu_timer_ovf, "%u");
	PRINT_SINGL_ARG("    rx reorder4 count:  ", pstat->rx_rc4_count, "%u");

#ifdef _AMPSDU_AMSDU_DEBUG_//_DEBUG_RTL8192CD_
	PRINT_SINGL_ARG("    tx amsdu to:      ", pstat->tx_amsdu_to, "%u");
	PRINT_SINGL_ARG("	 tx amsdu bufof:   ", pstat->tx_amsdu_buf_overflow, "%u");
	PRINT_SINGL_ARG("    tx amsdu 1pkt:    ", pstat->tx_amsdu_1pkt, "%u");
	PRINT_SINGL_ARG("    tx amsdu 2pkt:    ", pstat->tx_amsdu_2pkt, "%u");
	PRINT_SINGL_ARG("    tx amsdu 3pkt:    ", pstat->tx_amsdu_3pkt, "%u");
	PRINT_SINGL_ARG("    tx amsdu 4pkt:    ", pstat->tx_amsdu_4pkt, "%u");
	PRINT_SINGL_ARG("    tx amsdu 5pkt:    ", pstat->tx_amsdu_5pkt, "%u");
	PRINT_SINGL_ARG("    tx amsdu gt 5pkt: ", pstat->tx_amsdu_gt5pkt, "%u");

	PRINT_SINGL_ARG("    amsdu err:     ", pstat->rx_amsdu_err, "%u");
	PRINT_SINGL_ARG("    amsdu 1pkt:    ", pstat->rx_amsdu_1pkt, "%u");
	PRINT_SINGL_ARG("    amsdu 2pkt:    ", pstat->rx_amsdu_2pkt, "%u");
	PRINT_SINGL_ARG("    amsdu 3pkt:    ", pstat->rx_amsdu_3pkt, "%u");
	PRINT_SINGL_ARG("    amsdu 4pkt:    ", pstat->rx_amsdu_4pkt, "%u");
	PRINT_SINGL_ARG("    amsdu 5pkt:    ", pstat->rx_amsdu_5pkt, "%u");
	PRINT_SINGL_ARG("    amsdu gt 5pkt: ", pstat->rx_amsdu_gt5pkt, "%u");
#endif

#ifdef _DEBUG_RTL8192CD_
	PRINT_SINGL_ARG("    rc drop1:     ", pstat->rx_rc_drop1, "%u");
	PRINT_SINGL_ARG("    rc passup2:   ", pstat->rx_rc_passup2, "%u");
	PRINT_SINGL_ARG("    rc drop3:     ", pstat->rx_rc_drop3, "%u");
	PRINT_SINGL_ARG("    rc reorder3:  ", pstat->rx_rc_reorder3, "%u");
	PRINT_SINGL_ARG("    rc passup3:   ", pstat->rx_rc_passup3, "%u");
	PRINT_SINGL_ARG("    rc drop4:     ", pstat->rx_rc_drop4, "%u");
	PRINT_SINGL_ARG("    rc reorder4:  ", pstat->rx_rc_reorder4, "%u");
	PRINT_SINGL_ARG("    rc passup4:   ", pstat->rx_rc_passup4, "%u");
	PRINT_SINGL_ARG("    rc passupi:   ", pstat->rx_rc_passupi, "%u");
#endif
#ifdef DETECT_STA_EXISTANCE
	PRINT_SINGL_ARG("    sta leave:   ", pstat->leave, "%u");
#endif
#ifdef SW_TX_QUEUE
	PRINT_SINGL_ARG("    bk aggnum:   ", pstat->swq.q_aggnum[BK_QUEUE], "%d");
	PRINT_SINGL_ARG("    be aggnum:   ", pstat->swq.q_aggnum[BE_QUEUE], "%d");
	PRINT_SINGL_ARG("	 be qlen:   ", skb_queue_len(&pstat->swq.swq_queue[BE_QUEUE]), "%d");
	PRINT_SINGL_ARG("    vi aggnum:   ", pstat->swq.q_aggnum[VI_QUEUE], "%d");
	PRINT_SINGL_ARG("    vo aggnum:   ", pstat->swq.q_aggnum[VO_QUEUE], "%d");
	PRINT_SINGL_ARG("    bk backtime:   ", pstat->swq.q_aggnumIncSlow[BK_QUEUE], "%d");
    PRINT_SINGL_ARG("    be backtime:   ", pstat->swq.q_aggnumIncSlow[BE_QUEUE], "%d");
    PRINT_SINGL_ARG("    vi backtime:   ", pstat->swq.q_aggnumIncSlow[VI_QUEUE], "%d");
    PRINT_SINGL_ARG("    vo backtime:   ", pstat->swq.q_aggnumIncSlow[VO_QUEUE], "%d");
#endif

#if defined(MULTI_STA_REFINE) && defined(CONFIG_WLAN_HAL)
	PRINT_SINGL_ARG("    Drop packets cur:   ", pstat->dropPktCurr, "%u");
	PRINT_SINGL_ARG("    Drop packets total:	 ", pstat->dropPktTotal, "%u");
	PRINT_SINGL_ARG("    Drop aging:   ", pstat->drop_expire, "%u");
#endif

#if (BEAMFORMING_SUPPORT == 1)
	PRINT_SINGL_ARG("    bf score:   ", pstat->bf_score, "%u");
#endif

	PRINT_ONE("", "%s", 1);

	return pos;
}

static int rtl8192cd_proc_sta_dbginfo(struct seq_file *s, void *data)
{
    struct net_device *dev = PROC_GET_DEV();
    struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
    int len = 0;
    int size, num=1;
    struct list_head *phead, *plist;
    struct stat_info *pstat;

#if 1//!defined(SMP_SYNC) || (defined(CONFIG_USB_HCI) || defined(CONFIG_SDIO_HCI))
    unsigned long flags=0;
#endif

    SAVE_INT_AND_CLI(flags);
    SMP_LOCK_ASOC_LIST(flags);

    seq_printf(s, "-- STA dbg info table --\n");

#ifdef SW_TX_QUEUE
	PRINT_SINGL_ARG("    swq_numActiveSTA:   ", priv->pshare->swq_numActiveSTA, "%d");
#endif

    phead = &priv->asoc_list;
    if (!(priv->drv_state & DRV_STATE_OPEN) || list_empty(phead))
        goto _ret;

    plist = phead->next;
    while (plist != phead) {
        pstat = list_entry(plist, struct stat_info, asoc_list);
        plist = plist->next;
        size = dump_one_sta_dbginfo(s, num++, priv, pstat);
        CHECK_LEN;
    }

_ret:
    SMP_UNLOCK_ASOC_LIST(flags);
    RESTORE_INT(flags);


    return len;
}



static int rtl8192cd_proc_stats(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0, idx = 0;
	unsigned int m, n, print=0;
	char tmp[32];
	unsigned char *rate;
	unsigned long flags=0;

	SAVE_INT_AND_CLI(flags);

	PRINT_ONE("  Statistics...", "%s", 1);

	m = priv->up_time / 86400;
	n = priv->up_time % 86400;
	if (m) {
		idx += sprintf(tmp, "%d day ", m);
		print = 1;
	}
	m = n / 3600;
	n = n % 3600;
	if (m || print) {
		idx += sprintf(tmp+idx, "%d hr ", m);
		print = 1;
	}
	m = n / 60;
	n = n % 60;
	if (m || print) {
		idx += sprintf(tmp+idx, "%d min ", m);
		print = 1;
	}
	idx += sprintf(tmp+idx, "%d sec ", n);
	PRINT_SINGL_ARG("    up_time:       ", tmp, "%s");

#ifdef CH_LOAD_CAL
	/*reference IEEE,Std 802.11-2012,page567,use 0~255 to representing 0~100%*/
	PRINT_SINGL_ARG("    ch_utilization:	", priv->ext_stats.ch_utilization, "%lu");
	PRINT_SINGL_ARG("    tx_time:       ", priv->ext_stats.tx_time, "%lu");
	PRINT_SINGL_ARG("    rx_time:       ", priv->ext_stats.rx_time, "%lu");
#endif
	PRINT_SINGL_ARG("    tx_packets:    ", priv->net_stats.tx_packets, "%lu");
#ifdef TRX_DATA_LOG
	PRINT_SINGL_ARG("    tx_data_pkts:  ", priv->ext_stats.tx_data_packets, "%lu");
#endif
	PRINT_SINGL_ARG("    tx_bytes:      ", priv->net_stats.tx_bytes, "%lu");
#if defined(CONFIG_RTL8672) || defined(CONFIG_WLAN_STATS_EXTENTION)
	PRINT_SINGL_ARG("    tx_ucast_pkts: ", priv->ext_stats.tx_ucast_pkts_cnt, "%lu");
	PRINT_SINGL_ARG("    tx_mcast_pkts: ", priv->ext_stats.tx_mcast_pkts_cnt, "%lu");
	PRINT_SINGL_ARG("    tx_bcast_pkts: ", priv->ext_stats.tx_bcast_pkts_cnt, "%lu");
#endif
	PRINT_SINGL_ARG("    tx_retrys:     ", priv->ext_stats.tx_retrys, "%lu");
	PRINT_SINGL_ARG("    tx_fails:      ", priv->net_stats.tx_errors, "%lu");
	PRINT_SINGL_ARG("    tx_drops:      ", priv->ext_stats.tx_drops, "%lu");
	PRINT_SINGL_ARG("    tx_dma_err:    ", priv->pshare->tx_dma_err, "%lu");
	PRINT_SINGL_ARG("    rx_dma_err:    ", priv->pshare->rx_dma_err, "%lu");
	PRINT_SINGL_ARG("    tx_dma_status: ", priv->pshare->tx_dma_status, "%lu");
	PRINT_SINGL_ARG("    rx_dma_status: ", priv->pshare->rx_dma_status, "%lu");
#ifdef SUPPORT_AXI_BUS_EXCEPTION
	PRINT_SINGL_ARG("    axi_exception: ", priv->pshare->axi_exception, "%lu");
#endif //SUPPORT_AXI_BUS_EXCEPTION

	PRINT_SINGL_ARG("    rx_packets:    ", priv->net_stats.rx_packets, "%lu");
#ifdef TRX_DATA_LOG
	PRINT_SINGL_ARG("    rx_data_pkts:  ", priv->ext_stats.rx_data_packets, "%lu");
#endif
	PRINT_SINGL_ARG("    rx_bytes:      ", priv->net_stats.rx_bytes, "%lu");
#if defined(CONFIG_RTL8672) || defined(CONFIG_WLAN_STATS_EXTENTION)
	PRINT_SINGL_ARG("    rx_ucast_pkts: ", priv->ext_stats.rx_ucast_pkts_cnt, "%lu");
	PRINT_SINGL_ARG("    rx_mcast_pkts: ", priv->ext_stats.rx_mcast_pkts_cnt, "%lu");
	PRINT_SINGL_ARG("    rx_bcast_pkts: ", priv->ext_stats.rx_bcast_pkts_cnt, "%lu");
	PRINT_SINGL_ARG("    unknown_pro_pkts: ", priv->ext_stats.unknown_pro_pkts_cnt, "%lu");
	PRINT_SINGL_ARG("    total_mic_fail: ", priv->ext_stats.total_mic_fail, "%lu");
	PRINT_SINGL_ARG("    total_psk_fail: ", priv->ext_stats.total_psk_fail, "%lu");
#endif
	PRINT_SINGL_ARG("    rx_retrys:     ", priv->ext_stats.rx_retrys, "%lu");
	PRINT_SINGL_ARG("    rx_crc_errors: ", priv->net_stats.rx_crc_errors, "%lu");
	PRINT_SINGL_ARG("    rx_errors:     ", priv->net_stats.rx_errors, "%lu");
	PRINT_SINGL_ARG("    rx_data_drops: ", priv->ext_stats.rx_data_drops, "%lu");
	PRINT_SINGL_ARG("    rx_mc_pn_drops: ", priv->ext_stats.rx_mc_pn_drops, "%lu");
	PRINT_SINGL_ARG("    rx_cnt_psk_to: ", priv->ext_stats.rx_cnt_psk_to, "%lu");
	PRINT_SINGL_ARG("    rx_decache:    ", priv->ext_stats.rx_decache, "%lu");
	PRINT_SINGL_ARG("    rx_fifoO:      ", priv->ext_stats.rx_fifoO, "%lu");
	PRINT_SINGL_ARG("    rx_rdu:        ", priv->ext_stats.rx_rdu, "%lu");
	PRINT_SINGL_ARG("    rx_reuse:      ", priv->ext_stats.rx_reuse, "%lu");
	PRINT_SINGL_ARG("    beacon_ok:     ", priv->ext_stats.beacon_ok, "%lu");
	PRINT_SINGL_ARG("    beacon_er:     ", priv->ext_stats.beacon_er, "%lu");
	PRINT_SINGL_ARG("    beacon_dma_err:", priv->ext_stats.beacon_dma_err, "%lu");

	PRINT_SINGL_ARG("    freeskb_err:   ", priv->ext_stats.freeskb_err, "%lu");
	PRINT_SINGL_ARG("    dz_queue_len:  ", CIRC_CNT(priv->dz_queue.head, priv->dz_queue.tail, NUM_TXPKT_QUEUE), "%d");

#ifdef CHECK_HANGUP
	if (IS_ROOT_INTERFACE(priv))
	{
#ifdef CHECK_TX_HANGUP
		PRINT_SINGL_ARG("    check_cnt_tx:  ", priv->check_cnt_tx, "%d");
		PRINT_SINGL_ARG("    check_cnt_adaptivity: ", priv->check_cnt_adaptivity, "%d");
#endif
#ifdef CHECK_FW_ERROR
		PRINT_SINGL_ARG("    check_cnt_fw:  ", priv->check_cnt_fw, "%d");
#endif
#if defined(CHECK_RX_HANGUP) || defined(CHECK_RX_DMA_ERROR)
		PRINT_SINGL_ARG("    check_cnt_rx:  ", priv->check_cnt_rx, "%d");
#endif
#ifdef CHECK_BEACON_HANGUP
		PRINT_SINGL_ARG("    check_cnt_bcn: ", priv->check_cnt_bcn, "%d");
#endif
#ifdef CHECK_AFTER_RESET
		PRINT_SINGL_ARG("    check_cnt_rst: ", priv->check_cnt_rst, "%d");
#endif
	}
#endif

PRINT_SINGL_ARG("    cnt_sta1_only:  ", priv->cnt_sta1_only, "%d");
PRINT_SINGL_ARG("    cnt_sta2_only:  ", priv->cnt_sta2_only, "%d");
PRINT_SINGL_ARG("    cnt_sta1_sta2:  ", priv->cnt_sta1_sta2, "%d");
PRINT_SINGL_ARG("    cnt_mu_swqoverflow:  ", priv->cnt_mu_swqoverflow, "%d");
PRINT_SINGL_ARG("    cnt_mu_swqtimeout:  ", priv->cnt_mu_swqtimeout, "%d");

#if (MU_BEAMFORMING_SUPPORT == 1)

PRINT_ARRAY_ARG("    mu_ok: ",	priv->pshare->rf_ft_var.mu_ok, "%d ", MAX_NUM_BEAMFORMEE_MU);
PRINT_ARRAY_ARG("    mu_fail: ",	priv->pshare->rf_ft_var.mu_fail, "%d ", MAX_NUM_BEAMFORMEE_MU);

PRINT_SINGL_ARG("    mu_BB_ok:  ", priv->pshare->rf_ft_var.mu_BB_ok, "%d");
PRINT_SINGL_ARG("    mu_BB_fail:  ", priv->pshare->rf_ft_var.mu_BB_fail, "%d");


#endif
	PRINT_SINGL_ARG("    VO drop:       ", priv->pshare->phw->VO_droppkt_count, "%d");
	PRINT_SINGL_ARG("    VI drop:       ", priv->pshare->phw->VI_droppkt_count, "%d");
	PRINT_SINGL_ARG("    BE drop:       ", priv->pshare->phw->BE_droppkt_count, "%d");
	PRINT_SINGL_ARG("    BK drop:       ", priv->pshare->phw->BK_droppkt_count, "%d");




	PRINT_SINGL_ARG("    reused_skb:    ", priv->ext_stats.reused_skb, "%lu");

	PRINT_SINGL_ARG("    skb_free_num:  ", priv->pshare->skb_queue.qlen, "%d");
	PRINT_SINGL_ARG("    tx_avarage:    ", priv->ext_stats.tx_avarage, "%lu");
	PRINT_SINGL_ARG("    rx_avarage:    ", priv->ext_stats.rx_avarage, "%lu");

#if defined(CONFIG_RTL8196B_TR) || defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD) || defined(CONFIG_RTL8196B_KLD) || defined(CONFIG_RTL8196C_KLD) || defined(CONFIG_RTL8196C_EC)
	PRINT_SINGL_ARG("    tx_peak:       ", priv->ext_stats.tx_peak, "%lu");
	PRINT_SINGL_ARG("    rx_peak:       ", priv->ext_stats.rx_peak, "%lu");
#endif

	PRINT_SINGL_ARG("    rx_desc_num:   ", RX_DESC_NUM, "%lu")
	PRINT_SINGL_ARG("    rx_buf_len:    ", RX_BUF_LEN, "%lu")

#ifdef RTK_AC_SUPPORT  //vht rate , todo, dump vht rates in Mbps
	if(priv->pshare->current_tx_rate >= VHT_RATE_ID){
		int tx_rate = VHT_MCS_DATA_RATE[priv->pshare->is_40m_bw][(priv->pshare->ht_current_tx_info&BIT(1))?1:0][(priv->pshare->current_tx_rate - VHT_RATE_ID)];
		tx_rate = tx_rate >> 1;
		PRINT_ONE("    cur_tx_rate:   VHT NSS", "%s", 0);
		PRINT_ONE(((priv->pshare->current_tx_rate - VHT_RATE_ID)/10)+1, "%d", 0);
		PRINT_ONE("-MCS", "%s", 0);
		PRINT_ONE((priv->pshare->current_tx_rate - VHT_RATE_ID)%10, "%d", 0);
		PRINT_ONE(tx_rate, "  %d", 1);
	}
	else
#endif
	if (is_MCS_rate(priv->pshare->current_tx_rate)) {
		PRINT_ONE("    cur_tx_rate:   MCS", "%s", 0);
		PRINT_ONE((priv->pshare->current_tx_rate - HT_RATE_ID), "%d", 0);
		rate = (unsigned char *)MCS_DATA_RATEStr[(priv->pshare->ht_current_tx_info&BIT(0))?1:0][(priv->pshare->ht_current_tx_info&BIT(1))?1:0][(priv->pshare->current_tx_rate - HT_RATE_ID)];
		PRINT_ONE(rate, " %s", 1);
	}
	else
	{
		PRINT_SINGL_ARG("    cur_tx_rate:   ", priv->pshare->current_tx_rate/2, "%d");
	}

#ifdef RESERVE_TXDESC_FOR_EACH_IF
	if (RSVQ_ENABLE) {
		PRINT_SINGL_ARG("    bkq_used_desc: ", (UINT)priv->use_txdesc_cnt[RSVQ(BK_QUEUE)], "%d");
		PRINT_SINGL_ARG("    beq_used_desc: ", (UINT)priv->use_txdesc_cnt[RSVQ(BE_QUEUE)], "%d");
		PRINT_SINGL_ARG("    viq_used_desc: ", (UINT)priv->use_txdesc_cnt[RSVQ(VI_QUEUE)], "%d");
		PRINT_SINGL_ARG("    voq_used_desc: ", (UINT)priv->use_txdesc_cnt[RSVQ(VO_QUEUE)], "%d");
	}
#endif

#ifdef HS2_SUPPORT
/* Hotspot 2.0 Release 1 */
	PRINT_SINGL_ARG("    proxy arp:     ", priv->proxy_arp, "%d");
	PRINT_SINGL_ARG("    dgaf_disable:	",priv->dgaf_disable, "%d");
	PRINT_SINGL_ARG("    IWLen:         ",priv->pmib->hs2Entry.interworking_ielen, "%d");
#endif

#ifdef USE_TXQUEUE
	if (BUFQ_ENABLE) {
		PRINT_SINGL_ARG("    txq_bk_num:    ", (UINT)txq_len(&priv->pshare->txq_list[RSVQ(BK_QUEUE)]), "%d");
		PRINT_SINGL_ARG("    txq_be_num:    ", (UINT)txq_len(&priv->pshare->txq_list[RSVQ(BE_QUEUE)]), "%d");
		PRINT_SINGL_ARG("    txq_vi_num:    ", (UINT)txq_len(&priv->pshare->txq_list[RSVQ(VI_QUEUE)]), "%d");
		PRINT_SINGL_ARG("    txq_vo_num:    ", (UINT)txq_len(&priv->pshare->txq_list[RSVQ(VO_QUEUE)]), "%d");
	}
#endif
#ifdef SW_TX_QUEUE
	PRINT_SINGL_ARG("    swq enable:    ", priv->pshare->swq_en, "%d");
	PRINT_SINGL_ARG("    swq hw timer:  ", priv->pshare->swq_use_hw_timer, "%d");
#endif

	PRINT_SINGL_ARG("    use hal:       ", priv->pshare->use_hal, "%d");
	PRINT_SINGL_ARG("    use outsrc:    ", priv->pshare->use_outsrc, "%d");

#if defined(SHORTCUT_STATISTIC) //defined(__ECOS) && defined(_DEBUG_RTL8192CD_)
	PRINT_SINGL_ARG("    tx_cnt_nosc:   ", priv->ext_stats.tx_cnt_nosc, "%lu");
	PRINT_SINGL_ARG("    tx_cnt_sc1:    ", priv->ext_stats.tx_cnt_sc1, "%lu");
	PRINT_SINGL_ARG("    tx_cnt_sc2:    ", priv->ext_stats.tx_cnt_sc2, "%lu");
	PRINT_SINGL_ARG("    rx_cnt_nosc:   ", priv->ext_stats.rx_cnt_nosc, "%lu");
	PRINT_SINGL_ARG("    rx_cnt_sc:     ", priv->ext_stats.rx_cnt_sc, "%lu");
	PRINT_SINGL_ARG("    br_cnt_nosc:   ", priv->ext_stats.br_cnt_nosc, "%lu");
	PRINT_SINGL_ARG("    br_cnt_sc:     ", priv->ext_stats.br_cnt_sc, "%lu");
#endif

	if(RTL_R8(0x1f)==0){
		PRINT_SINGL_ARG("    rf_lock:       ", "true", "%s");
	}else{
		PRINT_SINGL_ARG("    rf_lock:       ", "false", "%s");
	}
	PRINT_SINGL_ARG("    IQK total count: ", priv->pshare->IQK_total_cnt, "%d");
	PRINT_SINGL_ARG("    Check IQK:     ", priv->pshare->IQK_fail_cnt, "%d");

	if (IS_OUTSRC_CHIP(priv))
		if(ODMPTR->ConfigBBRF)
			PRINT_SINGL_ARG("    Phy para Version: ", priv->pshare->PhyVersion, "%d");

	PRINT_SINGL_ARG("    adaptivity_enable: ", priv->pshare->rf_ft_var.adaptivity_enable, "%d");
	if (priv->pshare->rf_ft_var.adaptivity_enable)
	{
		int adaptivity_status;
		adaptivity_status = check_adaptivity_test(priv);
		PRINT_SINGL_ARG("    adaptivity_status: ", adaptivity_status, "%d");
		PRINT_SINGL_ARG("    bcn_dont_ignore_edcca: ", priv->pshare->rf_ft_var.bcn_dont_ignore_edcca, "%d");
	}

	PRINT_SINGL_ARG("    null_interrupt_cnt1: ", priv->ext_stats.null_interrupt_cnt1, "%lu");
	PRINT_SINGL_ARG("    null_interrupt_cnt2: ", priv->ext_stats.null_interrupt_cnt2, "%lu");
	RESTORE_INT(flags);

	return pos;
}

static int rtl8192cd_proc_stats_clear(struct file *file, const char *buffer,
				unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);

	memset(&priv->net_stats, 0, sizeof(struct net_device_stats));
	memset(&priv->ext_stats, 0, sizeof(struct extra_stats));
	return count;
}


#ifdef RF_FINETUNE
static int rtl8192cd_proc_rfft(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;
#ifdef SUPPORT_TX_MCAST2UNI
	int i;
	int tmpbuf[64];
#endif

	PRINT_ONE("  RF fine tune variables...", "%s", 1);

	PRINT_SINGL_ARG("    rssi: ", priv->pshare->rf_ft_var.rssi_dump, "%d");
	PRINT_SINGL_ARG("    rxfifoO: ", priv->pshare->rf_ft_var.rxfifoO, "%x");
	PRINT_SINGL_ARG("    raGoDownUpper: ", priv->pshare->rf_ft_var.raGoDownUpper, "%d");
	PRINT_SINGL_ARG("    raGoDown20MLower: ", priv->pshare->rf_ft_var.raGoDown20MLower, "%d");
	PRINT_SINGL_ARG("    raGoDown40MLower: ", priv->pshare->rf_ft_var.raGoDown40MLower, "%d");
	PRINT_SINGL_ARG("    raGoUpUpper: ", priv->pshare->rf_ft_var.raGoUpUpper, "%d");
	PRINT_SINGL_ARG("    raGoUp20MLower: ", priv->pshare->rf_ft_var.raGoUp20MLower, "%d");
	PRINT_SINGL_ARG("    raGoUp40MLower: ", priv->pshare->rf_ft_var.raGoUp40MLower, "%d");
	PRINT_SINGL_ARG("    dig_enable: ", priv->pshare->rf_ft_var.dig_enable, "%d");
	PRINT_SINGL_ARG("    digGoLowerLevel: ", priv->pshare->rf_ft_var.digGoLowerLevel, "%d");
	PRINT_SINGL_ARG("    digGoUpperLevel: ", priv->pshare->rf_ft_var.digGoUpperLevel, "%d");
	PRINT_SINGL_ARG("    rssiTx20MUpper: ", priv->pshare->rf_ft_var.rssiTx20MUpper, "%d");
	PRINT_SINGL_ARG("    rssiTx20MLower: ", priv->pshare->rf_ft_var.rssiTx20MLower, "%d");
	PRINT_SINGL_ARG("    rssi_expire_to: ", priv->pshare->rf_ft_var.rssi_expire_to, "%d");

	PRINT_SINGL_ARG("    cck_pwr_max: ", priv->pshare->rf_ft_var.cck_pwr_max, "%d");
	PRINT_SINGL_ARG("    cck_tx_pathB: ", priv->pshare->rf_ft_var.cck_tx_pathB, "%d");

	PRINT_SINGL_ARG("    tx_pwr_ctrl: ", priv->pshare->rf_ft_var.tx_pwr_ctrl, "%d");

	// 11n ap AES debug
	PRINT_SINGL_ARG("    aes_check_th: ", priv->pshare->rf_ft_var.aes_check_th, "%d KB");

	// Tx power tracking
	PRINT_SINGL_ARG("    tpt_period: ", priv->pshare->rf_ft_var.tpt_period, "%d");
#ifdef CONFIG_RF_DPK_SETTING_SUPPORT
	PRINT_SINGL_ARG("    dpk_period: ", priv->pshare->rf_ft_var.dpk_period, "%d");
#endif
	// TXOP enlarge
	PRINT_SINGL_ARG("    txop_enlarge_upper: ", priv->pshare->rf_ft_var.txop_enlarge_upper, "%d");
	PRINT_SINGL_ARG("    txop_enlarge_lower: ", priv->pshare->rf_ft_var.txop_enlarge_lower, "%d");

	// 2.3G support
	PRINT_SINGL_ARG("    frq_2_3G: ", priv->pshare->rf_ft_var.use_frq_2_3G, "%d");

	//Support IP multicast->unicast
#ifdef SUPPORT_TX_MCAST2UNI
	PRINT_SINGL_ARG("    mc2u_disable: ", priv->pshare->rf_ft_var.mc2u_disable, "%d");
	PRINT_SINGL_ARG("    mc2u_drop_unknown: ", priv->pshare->rf_ft_var.mc2u_drop_unknown, "%d");
	PRINT_SINGL_ARG("    mc2u_flood_ctrl: ", priv->pshare->rf_ft_var.mc2u_flood_ctrl, "%d");
	if(priv->pshare->rf_ft_var.mc2u_flood_ctrl)
	{
		PRINT_SINGL_ARG("    mc2u_flood_mac_num: ", priv->pshare->rf_ft_var.mc2u_flood_mac_num, "%d");
		for (i=0; i< priv->pshare->rf_ft_var.mc2u_flood_mac_num; i++) {
			sprintf(tmpbuf, "    mc2u_flood_mac[%d]: ", i);
			PRINT_ARRAY_ARG(tmpbuf,	priv->pshare->rf_ft_var.mc2u_flood_mac[i].macAddr, "%02x", 6);
		}
	}
#endif

#ifdef	HIGH_POWER_EXT_PA
	PRINT_SINGL_ARG("    use_ext_pa: ", priv->pshare->rf_ft_var.use_ext_pa, "%d");
#endif
#ifdef HIGH_POWER_EXT_LNA
	PRINT_SINGL_ARG("    use_ext_lna: ", priv->pshare->rf_ft_var.use_ext_lna, "%d");
#endif
	PRINT_SINGL_ARG("    NDSi_support: ", priv->pshare->rf_ft_var.NDSi_support, "%d");
	PRINT_SINGL_ARG("    EDCCA threshold: ", priv->pshare->rf_ft_var.edcca_thd, "%d");
	PRINT_SINGL_ARG("    1rcca: ", priv->pshare->rf_ft_var.one_path_cca, "%d");

	return pos;
}
#endif


#ifdef GBWC
static int rtl8192cd_proc_mib_gbwc(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0, i;

	PRINT_ONE("  miscGBWC...", "%s", 1);

	PRINT_SINGL_ARG("    GBWCMode: ", priv->pmib->gbwcEntry.GBWCMode, "%d");
	PRINT_SINGL_ARG("    GBWCThrd_tx: ", priv->pmib->gbwcEntry.GBWCThrd_tx, "%d kbps");
	PRINT_SINGL_ARG("    GBWCThrd_rx: ", priv->pmib->gbwcEntry.GBWCThrd_rx, "%d kbps");
	PRINT_ONE("    Address List:", "%s", 1);
	for (i=0; i<priv->pmib->gbwcEntry.GBWCNum; i++) {
		PRINT_ARRAY_ARG("      ", priv->pmib->gbwcEntry.GBWCAddr[i], "%02x", MACADDRLEN);
	}

	return pos;
}
#endif


#ifdef CONFIG_RTL_KERNEL_MIPS16_WLAN
__NOMIPS16
#endif
static int rtl8192cd_proc_led(struct file *file, const char *buffer,
				unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	char tmpbuf[100];
	int flag;

	if (buffer && !copy_from_user(tmpbuf, buffer, count)) {
		sscanf(tmpbuf, "%d", &flag);
		if (flag == 0) // disable
			control_wireless_led(priv, 0);
		else if (flag == 1) // enable
			control_wireless_led(priv, 1);
		else if (flag == 2) // restore
			control_wireless_led(priv, 2);
		else
			printk("flag [%d] not supported!\n", flag);
    }
	return count;
}


#ifdef RTL_MANUAL_EDCA
static int rtl8192cd_proc_mib_edca(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;
	char *queue[] = {"", "BK", "BE", "VI", "VO"};

	PRINT_SINGL_ARG("  Manually config EDCA : ", priv->pmib->dot11QosEntry.ManualEDCA, "%d");
	PRINT_ONE("  EDCA for AP...", "%s", 1);
	PRINT_SINGL_ARG("  [BE]slot number (AIFS): ", priv->pmib->dot11QosEntry.AP_manualEDCA[BE].AIFSN, "%d");
	PRINT_SINGL_ARG("      Maximal contention window period: ", priv->pmib->dot11QosEntry.AP_manualEDCA[BE].ECWmax, "%d");
	PRINT_SINGL_ARG("      Minimal contention window period: ", priv->pmib->dot11QosEntry.AP_manualEDCA[BE].ECWmin, "%d");
	PRINT_SINGL_ARG("      TXOP limit: ", priv->pmib->dot11QosEntry.AP_manualEDCA[BE].TXOPlimit, "%d");
	PRINT_SINGL_ARG("  [BK]slot number (AIFS): ", priv->pmib->dot11QosEntry.AP_manualEDCA[BK].AIFSN, "%d");
	PRINT_SINGL_ARG("      Maximal contention window period: ", priv->pmib->dot11QosEntry.AP_manualEDCA[BK].ECWmax, "%d");
	PRINT_SINGL_ARG("      Minimal contention window period: ", priv->pmib->dot11QosEntry.AP_manualEDCA[BK].ECWmin, "%d");
	PRINT_SINGL_ARG("      TXOP limit: ", priv->pmib->dot11QosEntry.AP_manualEDCA[BK].TXOPlimit, "%d");
	PRINT_SINGL_ARG("  [VI]slot number (AIFS)= ", priv->pmib->dot11QosEntry.AP_manualEDCA[VI].AIFSN, "%d");
	PRINT_SINGL_ARG("      Maximal contention window period: ", priv->pmib->dot11QosEntry.AP_manualEDCA[VI].ECWmax, "%d");
	PRINT_SINGL_ARG("      Minimal contention window period: ", priv->pmib->dot11QosEntry.AP_manualEDCA[VI].ECWmin, "%d");
	PRINT_SINGL_ARG("      TXOP limit: ", priv->pmib->dot11QosEntry.AP_manualEDCA[VI].TXOPlimit, "%d");
	PRINT_SINGL_ARG("  [VO]slot number (AIFS): ", priv->pmib->dot11QosEntry.AP_manualEDCA[VO].AIFSN, "%d");
	PRINT_SINGL_ARG("      Maximal contention window period: ", priv->pmib->dot11QosEntry.AP_manualEDCA[VO].ECWmax, "%d");
	PRINT_SINGL_ARG("      Minimal contention window period: ", priv->pmib->dot11QosEntry.AP_manualEDCA[VO].ECWmin, "%d");
	PRINT_SINGL_ARG("      TXOP limit: ", priv->pmib->dot11QosEntry.AP_manualEDCA[VO].TXOPlimit, "%d");
	PRINT_ONE("  EDCA for Wireless client...", "%s", 1);
	PRINT_SINGL_ARG("  [BE]ACM: ", priv->pmib->dot11QosEntry.STA_manualEDCA[BE].ACM, "%d");
	PRINT_SINGL_ARG("      slot number (AIFS): ", priv->pmib->dot11QosEntry.STA_manualEDCA[BE].AIFSN, "%d");
	PRINT_SINGL_ARG("      Maximal contention window period: ", priv->pmib->dot11QosEntry.STA_manualEDCA[BE].ECWmax, "%d");
	PRINT_SINGL_ARG("      Minimal contention window period: ", priv->pmib->dot11QosEntry.STA_manualEDCA[BE].ECWmin, "%d");
	PRINT_SINGL_ARG("      TXOP limit: ", priv->pmib->dot11QosEntry.STA_manualEDCA[BE].TXOPlimit, "%d");
	PRINT_SINGL_ARG("  [BK]ACM:", priv->pmib->dot11QosEntry.STA_manualEDCA[BK].ACM, "%d");
	PRINT_SINGL_ARG("      slot number (AIFS): ", priv->pmib->dot11QosEntry.STA_manualEDCA[BK].AIFSN, "%d");
	PRINT_SINGL_ARG("      Maximal contention window period: ", priv->pmib->dot11QosEntry.STA_manualEDCA[BK].ECWmax, "%d");
	PRINT_SINGL_ARG("      Minimal contention window period: ", priv->pmib->dot11QosEntry.STA_manualEDCA[BK].ECWmin, "%d");
	PRINT_SINGL_ARG("      TXOP limit: ", priv->pmib->dot11QosEntry.STA_manualEDCA[BK].TXOPlimit, "%d");
	PRINT_SINGL_ARG("  [VI]ACM: ", priv->pmib->dot11QosEntry.STA_manualEDCA[VI].ACM, "%d");
	PRINT_SINGL_ARG("      slot number (AIFS): ", priv->pmib->dot11QosEntry.STA_manualEDCA[VI].AIFSN, "%d");
	PRINT_SINGL_ARG("      Maximal contention window period: ", priv->pmib->dot11QosEntry.STA_manualEDCA[VI].ECWmax, "%d");
	PRINT_SINGL_ARG("      Minimal contention window period: ", priv->pmib->dot11QosEntry.STA_manualEDCA[VI].ECWmin, "%d");
	PRINT_SINGL_ARG("      TXOP limit:", priv->pmib->dot11QosEntry.STA_manualEDCA[VI].TXOPlimit, "%d");
	PRINT_SINGL_ARG("  [VO]ACM: ", priv->pmib->dot11QosEntry.STA_manualEDCA[VO].ACM, "%d");
	PRINT_SINGL_ARG("      slot number (AIFS): ", priv->pmib->dot11QosEntry.STA_manualEDCA[VO].AIFSN, "%d");
	PRINT_SINGL_ARG("      Maximal contention window period: ", priv->pmib->dot11QosEntry.STA_manualEDCA[VO].ECWmax, "%d");
	PRINT_SINGL_ARG("      Minimal contention window period: ", priv->pmib->dot11QosEntry.STA_manualEDCA[VO].ECWmin, "%d");
	PRINT_SINGL_ARG("      TXOP limit: ", priv->pmib->dot11QosEntry.STA_manualEDCA[VO].TXOPlimit, "%d");

	PRINT_SINGL_ARG("      TID0 mapping: ", queue[priv->pmib->dot11QosEntry.TID_mapping[0]], "%s");
	PRINT_SINGL_ARG("      TID1 mapping: ", queue[priv->pmib->dot11QosEntry.TID_mapping[1]], "%s");
	PRINT_SINGL_ARG("      TID2 mapping: ", queue[priv->pmib->dot11QosEntry.TID_mapping[2]], "%s");
	PRINT_SINGL_ARG("      TID3 mapping: ", queue[priv->pmib->dot11QosEntry.TID_mapping[3]], "%s");
	PRINT_SINGL_ARG("      TID4 mapping: ", queue[priv->pmib->dot11QosEntry.TID_mapping[4]], "%s");
	PRINT_SINGL_ARG("      TID5 mapping: ", queue[priv->pmib->dot11QosEntry.TID_mapping[5]], "%s");
	PRINT_SINGL_ARG("      TID6 mapping: ", queue[priv->pmib->dot11QosEntry.TID_mapping[6]], "%s");
	PRINT_SINGL_ARG("      TID7 mapping: ", queue[priv->pmib->dot11QosEntry.TID_mapping[7]], "%s");

	return pos;
}
#endif //RTL_MANUAL_EDCA


#ifdef TLN_STATS
static int proc_wifi_conn_stats(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)dev->priv;
	int pos = 0;

	PRINT_ONE("  Wifi Connection Stats...", "%s", 1);

	PRINT_SINGL_ARG("    Time Interval: ", priv->pshare->rf_ft_var.stats_time_interval, "%d");
	PRINT_SINGL_ARG("    Connected Clients: ", priv->wifi_stats.connected_sta, "%d");
	PRINT_SINGL_ARG("    MAX Clients: ", priv->wifi_stats.max_sta, "%d");
	PRINT_SINGL_ARG("    MAX Clients Timestamp: ", priv->wifi_stats.max_sta_timestamp, "%d");
	PRINT_SINGL_ARG("    Rejected clients: ", priv->wifi_stats.rejected_sta, "%d");

	return pos;
}


static int proc_wifi_conn_stats_clear(struct file *file, const char *buffer,
				unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)dev->priv;

	memset(&priv->wifi_stats, 0, sizeof(struct tln_wifi_stats));
	return count;
}

static int proc_ext_wifi_conn_stats(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)dev->priv;
	int pos = 0;

	PRINT_ONE("  Extended WiFi Connection Stats...", "%s", 1);

	PRINT_ONE("  Reject Reason/Status: Reject Count", "%s", 1);
	PRINT_ONE(" =====================================================", "%s", 1);
	PRINT_SINGL_ARG("    Unspecified reason: ", priv->ext_wifi_stats.rson_UNSPECIFIED_1, "%d");
	PRINT_SINGL_ARG("    Previous auth no longer valid: ", priv->ext_wifi_stats.rson_AUTH_INVALID_2, "%d");
	PRINT_SINGL_ARG("    Deauth because of leaving (or has left): ", priv->ext_wifi_stats.rson_DEAUTH_STA_LEAVING_3, "%d");
	PRINT_SINGL_ARG("    Disassoc due to inactivity: ", priv->ext_wifi_stats.rson_INACTIVITY_4, "%d");
	PRINT_SINGL_ARG("    Disassoc because AP cannot handle: ", priv->ext_wifi_stats.rson_RESOURCE_INSUFFICIENT_5, "%d");
	PRINT_SINGL_ARG("    Class 2 frame from non-auth STA: ", priv->ext_wifi_stats.rson_UNAUTH_CLS2FRAME_6, "%d");
	PRINT_SINGL_ARG("    Class 3 frame from non-assoc STA: ", priv->ext_wifi_stats.rson_UNAUTH_CLS3FRAME_7, "%d");
	PRINT_SINGL_ARG("    Disassoc because leaving (or has left): ", priv->ext_wifi_stats.rson_DISASSOC_STA_LEAVING_8, "%d");
	PRINT_SINGL_ARG("    STA request (re)assoc did not auth: ", priv->ext_wifi_stats.rson_ASSOC_BEFORE_AUTH_9, "%d");
	PRINT_SINGL_ARG("    Invalid IE: ", priv->ext_wifi_stats.rson_INVALID_IE_13, "%d");
	PRINT_SINGL_ARG("    MIC failure: ", priv->ext_wifi_stats.rson_MIC_FAILURE_14, "%d");
	PRINT_SINGL_ARG("    4-Way Handshake timeout: ", priv->ext_wifi_stats.rson_4WAY_TIMEOUT_15, "%d");
	PRINT_SINGL_ARG("    Group Key Handshake timeout: ", priv->ext_wifi_stats.rson_GROUP_KEY_TIMEOUT_16, "%d");
	PRINT_SINGL_ARG("    IE in 4-Way Handshake different: ", priv->ext_wifi_stats.rson_DIFF_IE_17, "%d");
	PRINT_SINGL_ARG("    Invalid group cipher: ", priv->ext_wifi_stats.rson_MCAST_CIPHER_INVALID_18, "%d");
	PRINT_SINGL_ARG("    Invalid pairwise cipher: ", priv->ext_wifi_stats.rson_UCAST_CIPHER_INVALID_19, "%d");
	PRINT_SINGL_ARG("    Invalid AKMP: ", priv->ext_wifi_stats.rson_AKMP_INVALID_20, "%d");
	PRINT_SINGL_ARG("    Unsupported RSNIE version: ", priv->ext_wifi_stats.rson_UNSUPPORT_RSNIE_VER_21, "%d");
	PRINT_SINGL_ARG("    Invalid RSNIE capabilities: ", priv->ext_wifi_stats.rson_RSNIE_CAP_INVALID_22, "%d");
	PRINT_SINGL_ARG("    IEEE 802.1X auth failed: ", priv->ext_wifi_stats.rson_802_1X_AUTH_FAIL_23, "%d");
	PRINT_SINGL_ARG("    Reason out of scope of the device: ", priv->ext_wifi_stats.rson_OUT_OF_SCOPE, "%d");

	PRINT_SINGL_ARG("    Unspecified failure: ", priv->ext_wifi_stats.status_FAILURE_1, "%d");
	PRINT_SINGL_ARG("    Cannot support all capabilities: ", priv->ext_wifi_stats.status_CAP_FAIL_10, "%d");
	PRINT_SINGL_ARG("    Reassoc denied due to cannot confirm assoc exists: ", priv->ext_wifi_stats.status_NO_ASSOC_11, "%d");
	PRINT_SINGL_ARG("    Assoc denied due to reason beyond: ", priv->ext_wifi_stats.status_OTHER_12, "%d");
	PRINT_SINGL_ARG("    Not support specified auth alg: ", priv->ext_wifi_stats.status_NOT_SUPPORT_ALG_13, "%d");
	PRINT_SINGL_ARG("    Auth seq out of expected: ", priv->ext_wifi_stats.status_OUT_OF_AUTH_SEQ_14, "%d");
	PRINT_SINGL_ARG("    Challenge failure: ", priv->ext_wifi_stats.status_CHALLENGE_FAIL_15, "%d");
	PRINT_SINGL_ARG("    Auth timeout: ", priv->ext_wifi_stats.status_AUTH_TIMEOUT_16, "%d");
	PRINT_SINGL_ARG("    Denied because AP cannot handle: ", priv->ext_wifi_stats.status_RESOURCE_INSUFFICIENT_17, "%d");
	PRINT_SINGL_ARG("    Denied because STA not support all rates: ", priv->ext_wifi_stats.status_RATE_FAIL_18, "%d");
	PRINT_SINGL_ARG("    Status out of scope of the device: ", priv->ext_wifi_stats.status_OUT_OF_SCOPE, "%d");

	return pos;
}


static int proc_ext_wifi_conn_stats_clear(struct file *file, const char *buffer,
				unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)dev->priv;

	memset(&priv->ext_wifi_stats, 0, sizeof(struct tln_ext_wifi_stats));
	return count;
}
#endif


#if defined(RTLWIFINIC_GPIO_CONTROL)
static int rtl8192cd_proc_gpio_ctrl_read(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;
	int i;
	char tmp[16];

	for (i=0; i<12; i++) {
		if (priv->pshare->phw->GPIO_dir[i] == 0x01) {
			sprintf(tmp, "GPIO%d %d", i, RTLWIFINIC_GPIO_read_proc(priv, i));
			PRINT_ONE(tmp, "%s", 1);
		}
	}

	return pos;
}

#ifdef CONFIG_RTL_ASUSWRT
void asus_led_ctrl_write(struct net_device *dev, unsigned int pin, unsigned int val)
{
	RTLWIFINIC_GPIO_init_priv(GET_DEV_PRIV(dev));
	RTLWIFINIC_GPIO_config(pin, 0x10);
	RTLWIFINIC_GPIO_write(pin, val);
}

unsigned char asus_led_get_rssi(struct net_device *dev)
{
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	return priv->pmib->dot11Bss.rssi;
}
#endif

static int rtl8192cd_proc_gpio_ctrl_write(struct file *file, const char *buffer,
				unsigned long count, void *data)
{
	struct net_device *dev = (struct net_device *)data;
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int gpio_num_upper=11;
	char tmp[32], command[8], action[4];
	unsigned int num, gpio_num, direction, value;

	if (buffer && !copy_from_user(tmp, buffer, 32)) {
		num = sscanf(tmp, "%s %d %s", command, &gpio_num, action);

		if (num != 3) {
			panic_printk("Invalid gpio parameter! Failed!\n");
			return num;
		}
	}
	panic_printk("Command: [%s] gpio: [%d] action: [%s]\n", command, gpio_num, action);

	if (!memcmp(command, "config", 6)) {
		if (!memcmp(action, "r", 1))
			direction = 0x01;
		else if (!memcmp(action, "w", 1))
			direction = 0x10;
		else {
			panic_printk("Action not supported!\n");
			return count;
		}


		if ((gpio_num >= 0) && (gpio_num <= gpio_num_upper))
			priv->pshare->phw->GPIO_dir[gpio_num] = direction;
		else {
			panic_printk("GPIO pin not supported!\n");
			return count;
		}

		RTLWIFINIC_GPIO_config_proc(priv, gpio_num, direction);
	}
	else if (!memcmp(command, "set", 3)) {
		if (!memcmp(action, "0", 1))
			value = 0;
		else if (!memcmp(action, "1", 1))
			value = 1;
		else {
			panic_printk("Action not supported!\n");
			return count;
		}

		if (((gpio_num >= 0) && (gpio_num <= gpio_num_upper)) && (priv->pshare->phw->GPIO_dir[gpio_num] == 0x10))
			RTLWIFINIC_GPIO_write_proc(priv, gpio_num, value);
		else {
			panic_printk("GPIO pin not supported!\n");
			return count;
		}
	}
	else {
		panic_printk("Command not supported!\n");
	}

	return count;
}
#endif

static int rtl8192cd_proc_psd_scan_read(struct seq_file *s, void *data)
{

	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);

	int pos=0;
	int i, idx=0;
	char tmp[200];
	int freq=0, p=0, dBm=0;

	memset(tmp, 0x0, sizeof(tmp));

	idx += sprintf(tmp+idx, "CH %d ", priv->rtk->psd_fft_info[0]);
	idx += sprintf(tmp+idx, "BW %dM ", priv->rtk->psd_fft_info[1]);
	idx += sprintf(tmp+idx, "PTS %d ", priv->rtk->psd_fft_info[2]);
	PRINT_SINGL_ARG("PSD SCAN: ", tmp, "%s");

	idx=0;
	memset(tmp, 0x0, sizeof(tmp));
	if(priv->rtk->psd_fft_info[0]<14)
		freq = 2412+5*(priv->rtk->psd_fft_info[0]-1);
	else
		freq = 5180+5*(priv->rtk->psd_fft_info[0]-36);

	idx += sprintf(tmp+idx, "centeral %dMHz", freq);
	idx += sprintf(tmp+idx, " from %dMHz to", (freq-20));
	idx += sprintf(tmp+idx, " %dMHz (per unit is 1.25MHz)", (freq+20));
	PRINT_SINGL_ARG("Channel Frequency: ", tmp, "%s");

	idx=0;
	memset(tmp, 0x0, sizeof(tmp));

	//fix bandwidth=40 psd_pts=128
#if 1
	for(i=0;i<(128/4);i++)
	{
		p = priv->rtk->psd_fft_info[16+i*4+0]+priv->rtk->psd_fft_info[16+i*4+1]+
			priv->rtk->psd_fft_info[16+i*4+2]+priv->rtk->psd_fft_info[16+i*4+3];

		idx += sprintf(tmp+idx, "%4x ", p);

		if((i+1)%4 == 0)
		{
			PRINT_ONE(tmp, "%s", 1);
			memset(tmp, 0x0, sizeof(tmp));
			idx=0;
		}
	}
#else
	int size=priv->rtk->psd_fft_info[2]+16;
	for (i=16; i<size; i++)
	{
		idx += sprintf(tmp+idx, "%3x ", priv->rtk->psd_fft_info[i]);
		if((i+1)%16 == 0)
		{
			PRINT_ONE(tmp, "%s", 1);
			memset(tmp, 0x0, 64);
			idx=0;
		}
	}
#endif
	return pos;
}


static int rtl8192cd_proc_psd_scan_write(struct file *file, const char *buffer,
				unsigned long count, void *data)
{

	struct net_device *dev = PDE_DATA(file_inode(file));
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);

	char tmp[32];
	unsigned int num, chnl, bw=40, pts=128;//fix bandwidth=40 scan_pts=128

	if (buffer && !copy_from_user(tmp, buffer, 32)) {
		//num = sscanf(tmp, "%d", &chnl, &bw, &pts);
		num = sscanf(tmp, "%d", &chnl);
		if (num != 1) {
			panic_printk("Invalid psd scan parameter! Failed!\n");
			return count;
		}
	}
	panic_printk("Channel: [%d] Bandwidth: [%d] PSD_PTS: [%d]\n", chnl, bw, pts);

	priv->rtk->psd_chnl = chnl;
	priv->rtk->psd_bw = bw;
	priv->rtk->psd_pts= pts;

	return count;
}



static char *phydm_msg = NULL;
#define PHYDM_MSG_LEN	80*24

static int proc_get_phydm_cmd(struct seq_file *s, void *v)
{
    int len = 0;
    struct net_device *netdev = PROC_GET_DEV();
    struct rtl8192cd_priv *priv = GET_DEV_PRIV(netdev);

    if (GET_CHIP_VER(priv) < VERSION_8188E) {
        panic_printk("RTL8192C and RTL8192D don't support this cmd\n");
        return -EFAULT;
    }

    //allocate memory to phydm_msg
    if (NULL == phydm_msg) {
        phydm_msg = rtw_zmalloc(PHYDM_MSG_LEN);
        if (NULL == phydm_msg) {
            return -EFAULT;
        }
        phydm_cmd(ODMPTR, NULL, 0, 0, phydm_msg, PHYDM_MSG_LEN);
    }

    PRINT_ONE(phydm_msg, "%s", 1); //print phydm_smg to buf for proc file
    rtw_mfree(phydm_msg, PHYDM_MSG_LEN); //free memory
    phydm_msg = NULL;

    return len;
}

static int proc_set_phydm_cmd(struct file *file, const char *buffer,
				unsigned long count, void *data)
{

    struct net_device *dev = PDE_DATA(file_inode(file));
    struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
    char tmp[64] = {0};

    if (GET_CHIP_VER(priv) < VERSION_8188E)  {
        panic_printk("RTL8192C and RTL8192D don't support this cmd\n");
        return -EFAULT;
    }

    if (count < 1)
        return -EFAULT;

    if (count > sizeof(tmp))
        return -EFAULT;

    if (buffer && !copy_from_user(tmp, buffer, 64)) { //read cmd from proc file
		if (NULL == phydm_msg) {
			phydm_msg = rtw_zmalloc(PHYDM_MSG_LEN);
			if (NULL == phydm_msg)
				return -ENOMEM;
		}
		else {
			memset(phydm_msg, 0, PHYDM_MSG_LEN);
		}
		phydm_cmd(ODMPTR, tmp, count, 1, phydm_msg, PHYDM_MSG_LEN);
		if (strlen(phydm_msg) == 0) {
			rtw_mfree(phydm_msg, PHYDM_MSG_LEN);
			phydm_msg = NULL;
		}
    }
    return count;
}


static int rtl8192cd_proc_thermal(struct seq_file *s, void *data)
{
	struct net_device *dev = PROC_GET_DEV();
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	int pos = 0;

	if (netif_running(priv->dev)) {
		if (GET_CHIP_VER(priv)==VERSION_8192D)
			PHY_SetRFReg(priv, RF92CD_PATH_A, RF_T_METER_92D, bMask20Bits, 0x30000);
		else
			PHY_SetRFReg(priv, RF92CD_PATH_A, 0x24, bMask20Bits, 0x60);

		// delay for 1 second
		delay_ms(1000);

		// query rf reg 0x24[4:0], for thermal meter value
		if (GET_CHIP_VER(priv)==VERSION_8192D)
			priv->pshare->rf_ft_var.curr_ther = PHY_QueryRFReg(priv, RF92CD_PATH_A, RF_T_METER_92D, 0xf800, 1);
		else
			priv->pshare->rf_ft_var.curr_ther = PHY_QueryRFReg(priv, RF92CD_PATH_A, 0x24, bMask20Bits, 1) & 0x01f;
	}
	PRINT_SINGL_ARG("", priv->pshare->rf_ft_var.curr_ther, "%u");
	return pos;
}


	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_mib_all);
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_mib_rf);
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_mib_operation);
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_mib_staconfig);
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_mib_dkeytbl);
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_mib_auth);
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_mib_gkeytbl);
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_mib_bssdesc);
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_stainfo);
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_sta_keyinfo);
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_wmm);
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_sta_dbginfo);
	RTK_DECLARE_READ_WRITE_PROC_FOPS(rtl8192cd_proc_stats, rtl8192cd_proc_stats_clear);

	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_mib_erp);
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_probe_info);
#ifdef STA_ASSOC_STATISTIC
	 RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_reject_assoc_info);
	 RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_rm_assoc_info);
	 RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_assoc_status_info);
#endif
#ifdef PROC_STA_CONN_FAIL_INFO
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_sta_conn_fail);
#endif
#ifdef STA_RATE_STATISTIC
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_sta_rateinfo);
#endif
#ifdef CONFIG_RTL_WLAN_DIAGNOSTIC
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_diagnostic);
#endif
#ifdef HS2_SUPPORT
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_mib_hs2);
#endif

	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_mib_brext);
    RTK_DECLARE_READ_WRITE_PROC_FOPS(rtl8192cd_proc_nat25filter_read, rtl8192cd_proc_nat25filter_write);


#ifdef CONFIG_IEEE80211V
    RTK_DECLARE_READ_WRITE_PROC_FOPS(rtl8192cd_proc_transition_read, rtl8192cd_proc_transition_write);
#endif

#if (BEAMFORMING_SUPPORT == 1)
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_mib_txbf);
#endif
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_mib_DFS);

	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_mib_rf_ac);
#ifdef IDLE_NOISE_LEVEL
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_mib_noise_level);
#endif

	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_mib_misc);

#ifdef WIFI_SIMPLE_CONFIG
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_mib_wsc);
#endif

#ifdef GBWC
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_mib_gbwc);
#endif


	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_mib_11n);

#ifdef RTL_MANUAL_EDCA
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_mib_edca);
#endif
#ifdef CONFIG_RTK_VLAN_SUPPORT
	RTK_DECLARE_READ_WRITE_PROC_FOPS(rtl8192cd_proc_vlan_read, rtl8192cd_proc_vlan_write);
#endif
#ifdef SUPPORT_MULTI_PROFILE
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_mib_ap_profile);
#endif

	RTK_DECLARE_READ_WRITE_PROC_FOPS(rtl8192cd_proc_txdesc_info, rtl8192cd_proc_txdesc_idx_write);
#ifdef CLIENT_MODE
	RTK_DECLARE_READ_WRITE_PROC_FOPS(rtl8192cd_proc_up_read, rtl8192cd_proc_up_write);
#endif

	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_rxdesc_info);
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_desc_info);
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_buf_info);
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_cam_info);
#ifdef ENABLE_RTL_SKB_STATS
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_skb_info);
#endif
#ifdef RF_FINETUNE
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_rfft);
#endif
	RTK_DECLARE_WRITE_PROC_FOPS(rtl8192cd_proc_led);
#ifdef AUTO_TEST_SUPPORT
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_SSR_read);
#endif
#if defined(RTLWIFINIC_GPIO_CONTROL)
	RTK_DECLARE_READ_WRITE_PROC_FOPS(rtl8192cd_proc_gpio_ctrl_read, rtl8192cd_proc_gpio_ctrl_write);
#endif
#ifdef CONFIG_RTL_WLAN_STATUS
	RTK_DECLARE_READ_WRITE_PROC_FOPS(rtl8192cd_proc_up_event_read, rtl8192cd_proc_up_event_write);
#endif

#ifdef A4_STA
    RTK_DECLARE_READ_PROC_FOPS(a4_dump_sta_info);
#endif



	RTK_DECLARE_READ_WRITE_PROC_FOPS(rtl8192cd_proc_psd_scan_read, rtl8192cd_proc_psd_scan_write);
#ifdef BT_COEXIST
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_bt_coexist);
#endif

	RTK_DECLARE_READ_WRITE_PROC_FOPS( proc_get_phydm_cmd, proc_set_phydm_cmd);

	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_thermal);

#ifdef CONFIG_RTL_WLAN_DIAGNOSTIC
char diag_log_buff[DIAGNOSTIC_LOG_SIZE];
char tmp_log[128];
#endif

#ifdef THERMAL_CONTROL
	RTK_DECLARE_READ_PROC_FOPS(rtl8192cd_proc_thermal_control);
#endif

void MDL_DEVINIT rtl8192cd_proc_init(struct net_device *dev)
{
    struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
    struct proc_dir_entry *rtl8192cd_proc_root = NULL ;
    struct proc_dir_entry *p;

    rtl8192cd_proc_root = proc_mkdir(dev->name, NULL);
    priv->proc_root = rtl8192cd_proc_root ;
    if (rtl8192cd_proc_root == NULL) {
        printk("create proc root failed!\n");
        return;
    }

    RTK_CREATE_PROC_READ_ENTRY(p, "mib_all", rtl8192cd_proc_mib_all);

#ifdef CONFIG_RTL_WLAN_DIAGNOSTIC
	RTK_CREATE_PROC_READ_ENTRY(p, "diagnostic", rtl8192cd_proc_diagnostic);
#endif

    RTK_CREATE_PROC_READ_ENTRY(p, "mib_rf", rtl8192cd_proc_mib_rf);
#ifdef IDLE_NOISE_LEVEL
    RTK_CREATE_PROC_READ_ENTRY(p, "mib_noise_level", rtl8192cd_proc_mib_noise_level);
#endif
    RTK_CREATE_PROC_READ_ENTRY(p, "mib_operation", rtl8192cd_proc_mib_operation);
    RTK_CREATE_PROC_READ_ENTRY(p, "mib_staconfig", rtl8192cd_proc_mib_staconfig);
    RTK_CREATE_PROC_READ_ENTRY(p, "mib_dkeytbl", rtl8192cd_proc_mib_dkeytbl);
    RTK_CREATE_PROC_READ_ENTRY(p, "mib_auth", rtl8192cd_proc_mib_auth);
    RTK_CREATE_PROC_READ_ENTRY(p, "mib_gkeytbl", rtl8192cd_proc_mib_gkeytbl);
    RTK_CREATE_PROC_READ_ENTRY(p, "mib_bssdesc", rtl8192cd_proc_mib_bssdesc);
    RTK_CREATE_PROC_READ_ENTRY(p, "sta_info", rtl8192cd_proc_stainfo);

    RTK_CREATE_PROC_READ_ENTRY(p, "wmm", rtl8192cd_proc_wmm);
    RTK_CREATE_PROC_READ_ENTRY(p, "sta_keyinfo", rtl8192cd_proc_sta_keyinfo);


    RTK_CREATE_PROC_READ_ENTRY(p, "sta_dbginfo", rtl8192cd_proc_sta_dbginfo);

    RTK_CREATE_PROC_READ_WRITE_ENTRY(p, "stats", rtl8192cd_proc_stats, rtl8192cd_proc_stats_clear);
    RTK_CREATE_PROC_READ_ENTRY(p, "mib_erp", rtl8192cd_proc_mib_erp);

    RTK_CREATE_PROC_READ_ENTRY(p, "probe_info", rtl8192cd_proc_probe_info);
#ifdef STA_ASSOC_STATISTIC
	RTK_CREATE_PROC_READ_ENTRY(p, "reject_assoc_info", rtl8192cd_proc_reject_assoc_info);
	RTK_CREATE_PROC_READ_ENTRY(p, "rm_assoc_info", rtl8192cd_proc_rm_assoc_info);
	RTK_CREATE_PROC_READ_ENTRY(p, "sta_assoc_status", rtl8192cd_proc_assoc_status_info);
#endif
#ifdef PROC_STA_CONN_FAIL_INFO
	RTK_CREATE_PROC_READ_ENTRY(p, "sta_conn_fail", rtl8192cd_proc_sta_conn_fail);
#endif
#ifdef STA_RATE_STATISTIC
	RTK_CREATE_PROC_READ_ENTRY(p, "sta_rateinfo", rtl8192cd_proc_sta_rateinfo);
#endif
#ifdef HS2_SUPPORT
    RTK_CREATE_PROC_READ_ENTRY(p, "mib_hs2", rtl8192cd_proc_mib_hs2);
#endif

    RTK_CREATE_PROC_READ_ENTRY(p, "mib_brext", rtl8192cd_proc_mib_brext);
    RTK_CREATE_PROC_READ_WRITE_ENTRY(p, "nat25filter", rtl8192cd_proc_nat25filter_read, rtl8192cd_proc_nat25filter_write);


#ifdef CONFIG_IEEE80211V
     RTK_CREATE_PROC_READ_WRITE_ENTRY(p, "wnm_transition_list", rtl8192cd_proc_transition_read, rtl8192cd_proc_transition_write);
#endif

#if (BEAMFORMING_SUPPORT == 1)
	RTK_CREATE_PROC_READ_ENTRY(p, "mib_txbf", rtl8192cd_proc_mib_txbf);
#endif
    RTK_CREATE_PROC_READ_ENTRY(p, "mib_dfs", rtl8192cd_proc_mib_DFS);

    RTK_CREATE_PROC_READ_ENTRY(p, "mib_rf_ac", rtl8192cd_proc_mib_rf_ac);

    RTK_CREATE_PROC_READ_ENTRY(p, "mib_misc", rtl8192cd_proc_mib_misc);

#ifdef WIFI_SIMPLE_CONFIG
    RTK_CREATE_PROC_READ_ENTRY(p, "mib_wsc", rtl8192cd_proc_mib_wsc);
#endif

#ifdef GBWC
    RTK_CREATE_PROC_READ_ENTRY(p, "mib_gbwc", rtl8192cd_proc_mib_gbwc);
#endif


    RTK_CREATE_PROC_READ_ENTRY(p, "mib_11n", rtl8192cd_proc_mib_11n);

#ifdef RTL_MANUAL_EDCA
    RTK_CREATE_PROC_READ_ENTRY(p, "mib_EDCA", rtl8192cd_proc_mib_edca);
#endif

#ifdef CONFIG_RTK_VLAN_SUPPORT
    RTK_CREATE_PROC_READ_WRITE_ENTRY(p, "mib_vlan", rtl8192cd_proc_vlan_read, rtl8192cd_proc_vlan_write);
#endif

#ifdef TLN_STATS
    RTK_CREATE_PROC_READ_WRITE_ENTRY(p, "wifi_conn_stats", proc_wifi_conn_stats, proc_wifi_conn_stats_clear);
    RTK_CREATE_PROC_READ_WRITE_ENTRY(p, "ext_wifi_conn_stats", proc_ext_wifi_conn_stats, proc_ext_wifi_conn_stats_clear);
#endif

#ifdef SUPPORT_MULTI_PROFILE
    RTK_CREATE_PROC_READ_ENTRY(p, "mib_ap_profile", rtl8192cd_proc_mib_ap_profile);
#endif


    if (IS_ROOT_INTERFACE(priv))  // is root interface
    {
        RTK_CREATE_PROC_READ_WRITE_ENTRY(p, "txdesc", rtl8192cd_proc_txdesc_info, rtl8192cd_proc_txdesc_idx_write);

#ifdef CLIENT_MODE
        RTK_CREATE_PROC_READ_WRITE_ENTRY(p, "up_flag", rtl8192cd_proc_up_read, rtl8192cd_proc_up_write);
#endif

        RTK_CREATE_PROC_READ_ENTRY(p, "rxdesc", rtl8192cd_proc_rxdesc_info);
        RTK_CREATE_PROC_READ_ENTRY(p, "desc_info", rtl8192cd_proc_desc_info);
        RTK_CREATE_PROC_READ_ENTRY(p, "buf_info", rtl8192cd_proc_buf_info);
        RTK_CREATE_PROC_READ_ENTRY(p, "cam_info", rtl8192cd_proc_cam_info);

#ifdef ENABLE_RTL_SKB_STATS
        RTK_CREATE_PROC_READ_ENTRY(p, "skb_info", rtl8192cd_proc_skb_info);
#endif

#ifdef RF_FINETUNE
        RTK_CREATE_PROC_READ_ENTRY(p, "rf_finetune", rtl8192cd_proc_rfft);
#endif


		RTK_CREATE_PROC_WRITE_ENTRY(p, "led", rtl8192cd_proc_led);

#ifdef AUTO_TEST_SUPPORT
		RTK_CREATE_PROC_READ_ENTRY(p, "SS_Result", rtl8192cd_proc_SSR_read);
#endif

#if defined(RTLWIFINIC_GPIO_CONTROL)
        RTK_CREATE_PROC_READ_WRITE_ENTRY(p, "gpio_ctrl", rtl8192cd_proc_gpio_ctrl_read, rtl8192cd_proc_gpio_ctrl_write);

#endif

    }



#ifdef WLANHAL_MACDM
    RTK_CREATE_PROC_READ_WRITE_ENTRY(p, "macdm", rtl8192cd_proc_macdm, rtl8192cd_proc_macdm_write);
#endif //WLANHAL_MACDM

#ifdef CONFIG_RTL_WLAN_STATUS
    RTK_CREATE_PROC_READ_WRITE_ENTRY(p, "up_event", rtl8192cd_proc_up_event_read, rtl8192cd_proc_up_event_write);
#endif

#ifdef A4_STA
    RTK_CREATE_PROC_READ_ENTRY(p, "a4_sta_info", a4_dump_sta_info);
#endif

#ifdef ERR_ACCESS_CNTR
	RTK_CREATE_PROC_READ_ENTRY(p, "err_access", rtl8192cd_proc_err_access);
#endif



    RTK_CREATE_PROC_READ_WRITE_ENTRY(p, "psd_scan", rtl8192cd_proc_psd_scan_read, rtl8192cd_proc_psd_scan_write);
#ifdef BT_COEXIST
	RTK_CREATE_PROC_READ_ENTRY(p, "bt_coexist", rtl8192cd_proc_bt_coexist);
#endif

    RTK_CREATE_PROC_READ_WRITE_ENTRY(p, "cmd", proc_get_phydm_cmd, proc_set_phydm_cmd);

#ifdef THERMAL_CONTROL
	RTK_CREATE_PROC_READ_ENTRY(p, "thermal_control", rtl8192cd_proc_thermal_control);
#endif

    RTK_CREATE_PROC_READ_ENTRY(p, "thermal", rtl8192cd_proc_thermal);
}


void /*__devexit*/MDL_EXIT rtl8192cd_proc_remove (struct net_device *dev)
{
    struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
    struct proc_dir_entry *rtl8192cd_proc_root = priv->proc_root;

    if (rtl8192cd_proc_root != NULL) {
        remove_proc_entry( "mib_all", rtl8192cd_proc_root );
#ifdef CONFIG_RTL_WLAN_DIAGNOSTIC
		remove_proc_entry( "diagnostic", rtl8192cd_proc_root );
#endif
        remove_proc_entry( "mib_rf", rtl8192cd_proc_root );
        remove_proc_entry( "mib_operation", rtl8192cd_proc_root );
        remove_proc_entry( "mib_staconfig", rtl8192cd_proc_root );
        remove_proc_entry( "mib_dkeytbl", rtl8192cd_proc_root );
        remove_proc_entry( "mib_auth", rtl8192cd_proc_root );
        remove_proc_entry( "mib_gkeytbl", rtl8192cd_proc_root );
        remove_proc_entry( "mib_bssdesc", rtl8192cd_proc_root );
        remove_proc_entry( "sta_info", rtl8192cd_proc_root );
        remove_proc_entry( "sta_keyinfo", rtl8192cd_proc_root );
        remove_proc_entry( "wmm", rtl8192cd_proc_root );
        remove_proc_entry( "sta_dbginfo", rtl8192cd_proc_root );
        remove_proc_entry( "stats", rtl8192cd_proc_root );
        remove_proc_entry( "mib_erp", rtl8192cd_proc_root );
        remove_proc_entry( "probe_info", rtl8192cd_proc_root );


        remove_proc_entry( "mib_brext", rtl8192cd_proc_root );
        remove_proc_entry( "nat25filter", rtl8192cd_proc_root );


#if (BEAMFORMING_SUPPORT == 1)
        remove_proc_entry( "mib_txbf", rtl8192cd_proc_root );
#endif

        remove_proc_entry( "mib_dfs", rtl8192cd_proc_root );

#ifdef RTK_AC_SUPPORT //eric-8822
        remove_proc_entry( "mib_rf_ac", rtl8192cd_proc_root );
#endif

        remove_proc_entry( "mib_misc", rtl8192cd_proc_root );

#ifdef WIFI_SIMPLE_CONFIG
        remove_proc_entry( "mib_wsc", rtl8192cd_proc_root );
#endif


#ifdef GBWC
        remove_proc_entry( "mib_gbwc", rtl8192cd_proc_root );
#endif

        remove_proc_entry( "mib_11n", rtl8192cd_proc_root );

#ifdef RTL_MANUAL_EDCA
        remove_proc_entry( "mib_EDCA", rtl8192cd_proc_root );
#endif
#ifdef CONFIG_RTK_VLAN_SUPPORT
        remove_proc_entry( "mib_vlan", rtl8192cd_proc_root );
#endif

#ifdef TLN_STATS
        remove_proc_entry( "wifi_conn_stats", rtl8192cd_proc_root );
        remove_proc_entry( "ext_wifi_conn_stats", rtl8192cd_proc_root );
#endif

#ifdef SUPPORT_MULTI_PROFILE
        remove_proc_entry( "mib_ap_profile", rtl8192cd_proc_root );
#endif

        if (IS_ROOT_INTERFACE(priv))  // is root interface
        {
            remove_proc_entry( "txdesc", rtl8192cd_proc_root );
            remove_proc_entry( "rxdesc", rtl8192cd_proc_root );
            remove_proc_entry( "desc_info", rtl8192cd_proc_root );
            remove_proc_entry( "buf_info", rtl8192cd_proc_root );
            remove_proc_entry( "cam_info", rtl8192cd_proc_root );
#ifdef ENABLE_RTL_SKB_STATS
            remove_proc_entry( "skb_info", rtl8192cd_proc_root );
#endif
#ifdef RF_FINETUNE
            remove_proc_entry( "rf_finetune", rtl8192cd_proc_root );
#endif
#ifdef CLIENT_MODE
            remove_proc_entry( "up_flag", rtl8192cd_proc_root );
#endif
            remove_proc_entry( "led", rtl8192cd_proc_root );

#ifdef AUTO_TEST_SUPPORT
            remove_proc_entry( "SS_Result", rtl8192cd_proc_root );
#endif
#ifdef RTLWIFINIC_GPIO_CONTROL
            remove_proc_entry( "gpio_ctrl", rtl8192cd_proc_root );
#endif

        }


#ifdef CONFIG_RTL_WLAN_STATUS
        remove_proc_entry( "up_event", rtl8192cd_proc_root );
#endif

#ifdef A4_STA
        remove_proc_entry( "a4_sta_info", rtl8192cd_proc_root );
#endif

#ifdef ERR_ACCESS_CNTR
		remove_proc_entry( "err_access", rtl8192cd_proc_root );
#endif


        remove_proc_entry( "psd_scan", rtl8192cd_proc_root );


        remove_proc_entry( "cmd", rtl8192cd_proc_root );

        remove_proc_entry( "thermal", rtl8192cd_proc_root );
        remove_proc_entry( dev->name, NULL );
        rtl8192cd_proc_root = NULL;
    }
}



#endif // __INCLUDE_PROC_FS__
