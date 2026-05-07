include $(TOPDIR)/rules.mk

PKG_NAME:=td-system-tools
PKG_VERSION:=2.5.4
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=https://www.nntb.no/~dreibh/system-tools/download/
PKG_HASH:=REPLACE_WITH_ACTUAL_SHA256_HASH

PKG_MAINTAINER:=Thomas Dreibholz <thomas.dreibholz@gmail.com>
PKG_LICENSE:=GPL-3.0-or-later

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/td-system-tools
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Utilities for system management and maintenance
  URL:=https://www.nntb.no/~dreibh/system-tools/
  # Note: '+' means the dependency is strictly required.
  DEPENDS:=+bash +gettext-full +libopenssl +procps-ng +uuid
endef

define Package/td-system-tools/description
  System-Tools is a collection of utilities for system management and
  maintenance.
  
  The package includes tools for system information, SSH key handling,
  X.509 certificate handling, and text stream manipulation.
endef

# Disable tools that do not make sense on a router environment
CMAKE_OPTIONS += \
	-DWITH_SYSTEM_MAINTENANCE=OFF \
	-DWITH_CONFIGURE_GRUB=OFF \
	-DWITH_RESET_MACHINE_ID=OFF

define Package/td-system-tools/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin/
	
	$(INSTALL_DIR) $(1)/etc/system-info.d
	$(CP) $(PKG_INSTALL_DIR)/etc/system-info.d/* $(1)/etc/system-info.d/
	
	$(INSTALL_DIR) $(1)/usr/share/System-Info
	$(CP) $(PKG_INSTALL_DIR)/usr/share/System-Info/* $(1)/usr/share/System-Info/
endef

$(eval $(call BuildPackage,td-system-tools))
