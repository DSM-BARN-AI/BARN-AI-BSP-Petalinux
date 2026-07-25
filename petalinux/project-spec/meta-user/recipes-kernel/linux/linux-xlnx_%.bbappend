FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " file://bsp.cfg"
KERNEL_FEATURES:append = " bsp.cfg"
SRC_URI += "file://user_2026-07-24-12-55-00.cfg \
            file://user_2026-07-25-04-57-00.cfg \
            "

