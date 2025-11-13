### AnyKernel3 Ramdisk Mod Script
### Modified by @RissuDesu
## osm0sis @ xda-developers

### AnyKernel setup
# global properties
properties() { '
kernel.string=4.14.356-Mephis-Silent_eBPF
do.devicecheck=0
do.modules=0
do.systemless=0
do.cleanup=1
do.cleanuponabort=0
device.name1=
device.name2=
device.name3=
device.name4=
device.name5=
supported.versions=
supported.patchlevels=
supported.vendorpatchlevels=
'; } # end properties


### AnyKernel install
## boot files attributes
boot_attributes() {
set_perm_recursive 0 0 755 644 $RAMDISK/*;
set_perm_recursive 0 0 750 750 $RAMDISK/init* $RAMDISK/sbin;
} # end attributes

# boot shell variables
BLOCK=/dev/block/platform/bootdevice/by-name/boot;
IS_SLOT_DEVICE=0;
RAMDISK_COMPRESSION=auto;
PATCH_VBMETA_FLAG=auto;

# import functions/variables and setup patching - see for reference (DO NOT REMOVE)
. tools/ak3-core.sh;

# boot install
dump_boot; # use split_boot to skip ramdisk unpack, e.g. for devices with init_boot ramdisk

ui_print " ";
ui_print "###############################################################";
ui_print "  Fork of upstream kernel by RISSU (rsuntk) — many thanks to him.";
ui_print "  Maintainer: OrkGabb";
ui_print " ";
ui_print "  Performance-focused kernel with overclocking, undervolting";
ui_print "  and selected backports/tweaks for improved responsiveness.";
ui_print " ";
ui_print "  Overclocking / Undervolting:";
ui_print "   - GPU: 950 MHz -> 1.100 GHz (undervolt applied)";
ui_print "   - CPU: Undervolt applied across the big-core voltage table";
ui_print "   - CCI: OPP0 1277000 KHz -> 1550000 KHz (1.277 -> 1.550 GHz)";
ui_print " ";
ui_print " ";
ui_print "  Backports & Tweaks:";
ui_print "   - Full eBPF support, network tuning (BBR), HZ=300 and Disable HMP/EAS enabled";
ui_print "   - Additional kernel backports and features:";
ui_print "    RESYM / FPSGO / EARA / PERFMGR / IO_BOOST / PNPMGR / QoS_v1(V2 wen..) / PSI / USERFAULTFD ";
ui_print " ";
ui_print "  Notes: Only key OPPs shown; other OPPs adjusted conservatively";
ui_print "         to preserve stability across tested Galaxy M32 units.";
ui_print " ";
ui_print "  DISCLAIMER: Use at your own risk. I do not accept responsibility for";
ui_print "              any damage that may result from applying this kernel.";
ui_print "###############################################################";
ui_print " ";

write_boot; # use flash_boot to skip ramdisk repack, e.g. for devices with init_boot ramdisk
## end boot install
