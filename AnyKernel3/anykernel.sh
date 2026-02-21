### AnyKernel3 Ramdisk Mod Script
### Modified by @RissuDesu
## osm0sis @ xda-developers

### AnyKernel setup
# global properties
properties() { '
kernel.string=4.14.356-OrkGabb
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
ui_print "  Maintainer: OrkGabb";
ui_print "  Fork of upstream kernel by RISSU (rsuntk) — many thanks to him.";
ui_print " ";
ui_print "  Performance-focused kernel with overclocking, undervolting";
ui_print "  and selected backports/tweaks for improved responsiveness.";
ui_print " ";
ui_print "  Overclocking / Undervolting:";
ui_print "   - GPU: 950 MHz -> 1.100 GHz (undervolt applied)";
ui_print "   - CPU: Big only: 2Ghz > 2,1ghz. Undervolt applied across voltage table(Big/Little)";
ui_print "   - CCI: 1,27Ghz > 1,55ghz";
ui_print " ";
ui_print " ";
ui_print "  Backports & Tweaks:";
ui_print "   - eBPF 5.4 support, network tuning (BBR), HZ=300 and Disable HMP. Added full EAS(legacy)";
ui_print "   - Additional kernel backports and features:";
ui_print "    RESYM / FPSGO / EARA / PERFMGR / IO_BOOST / PNPMGR / QoS_v1 / PSI / USERFAULTFD ";
ui_print "   - Ported(partial) and enhanced GPU driver r25p0 > r54p2(newest driver) + some new functions and 'Tweaks' ";
ui_print " Rework r54p2 drivers to better perf in M32";
ui_print "   - Upstream LZ4_v1.9.3";
ui_print "   - Upstream ZSTD to lasted ver(kernel 5.15)";
ui_print "   - Upstream BBR TCP Congestion  algorithms to BBR_v2(set by default)";
ui_print "   - Added Scheduler Bore + Dram Tweaks + ppm_v3 (Thanks @msan_vigus to patchs/commits)";
ui_print "   - Added more support for Oneui 8.5(QPR1)"
ui_print "   - Adjust deadline I/O read_ahead_kb 128kb > 512kb";
ui_print "   - CCI/GPU/CPU OC/UV Redesigned for greater efficiency and performance +  DVFS Boost.";
ui_print " ";
ui_print "  Notes: OPPs adjusted conservatively";
ui_print "         to preserve stability/perf across tested Galaxy M32 units.";
ui_print " ";
ui_print "  DISCLAIMER: Use at your own risk. I do not accept responsibility for";
ui_print "              any damage that may result from applying this kernel.";
ui_print "###############################################################";
ui_print " ";

write_boot; # use flash_boot to skip ramdisk repack, e.g. for devices with init_boot ramdisk
## end boot install
