#!/bin/sh
ifname=
# 若是/etc/wpa_supplicant.conf 是只读的，可以建立一个链接指向一个可写的文件区域。
# ln -s /usr/resource/wpa_supplicant.conf /etc/wpa_supplicant.conf
# 以后这里将通过prop等管理机制把服务给建立起来，而不采用脚本形式。
echo $2
echo $1
ifname=$2

function wifi_down()
{
	ifconfig $ifname down
	killall -9 udhcpc
	killall -9 wpa_supplicant
	rm -rf /var/run/wpa_supplicant
	rm -rf /tmp/wpa_ctrl_*
}

function wifi_up()
{
	killall -9 udhcpc
	killall -9 wpa_supplicant
	rm -rf /var/run/wpa_supplicant
	rm -rf /tmp/wpa_ctrl_*
	rfkill unblock wifi
	ifconfig $ifname up
	wpa_supplicant -i $ifname -c /etc/wpa_supplicant.conf &
	#udhcpc -i $ifname &
}

function dhcp()
{
	if [ x$ifname == x ]; then
		killall -9 udhcpc
	else
		udhcpc -i $ifname &
	fi
}

while [ $# -ge 2 ] ; do
        case "$1" in
            up)
				wifi_up
				shift 2;;
            down)
				wifi_down
				shift 2;;
			dhcp)
				dhcp
				shift 2;;
            *)
				echo "unknown parameter $1." ;
				exit 1 ;
				break;;
        esac
done
