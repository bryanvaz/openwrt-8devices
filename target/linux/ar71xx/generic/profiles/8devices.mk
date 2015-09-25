#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/CARAMBOLA2
        NAME:=Carambola2 board from 8Devices
        PACKAGES:=kmod-usb-core kmod-usb2
endef

define Profile/CARAMBOLA2/Description
        Package set optimized for the 8devices Carambola2 board.
endef

$(eval $(call Profile,CARAMBOLA2))

define Profile/CENTIPEDE
        NAME:=Centipede board from 8Devices
        PACKAGES:=kmod-usb-core kmod-usb2
endef

define Profile/CENTIPEDE/Description
        Package set optimized for the 8devices Centipede board.
endef

$(eval $(call Profile,CENTIPEDE))


define Profile/ALL8DEVICES
	NAME:=8devices Products
	PACKAGES:=kmod-usb-core kmod-usb2
endef

define Profile/ALL8DEVICES/Description
	Build images for all 8devices products
endef

$(eval $(call Profile,ALL8DEVICES))