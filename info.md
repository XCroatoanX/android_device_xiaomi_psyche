Pulling dmesg

adb shell
Use this command to enter shell
su
Use this command to get su permission
dmesg > /sdcard/logs.txt
logcat -b all -d > /sdcard/logs_all.txt
Use this command to get dmesg inside internal storage

---Flashing rom----
1.Reboot to fastboot 
2.fastboot --set-active=b
3.fastboot boot twrp_image_name.img just drag Twrp.img into cmd prompt press enter..
4.set Twrp to reflash after flashing rom
5.switch to slot a as active so rom will be flashed into slot b primary
6.flash rom then magisk if you use it
7.wipe dalvik and cache
8.format data with yes repeat step 8 twice if won't format..
9.go to reboot choose slot b as active and reboot to system
10.Enjoy in rom
