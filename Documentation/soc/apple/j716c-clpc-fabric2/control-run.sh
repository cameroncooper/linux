#!/bin/sh
set -eu
umask 077
[ "$#" = 1 ]
n=/tmp/j716c-postcfg/target
lease=/tmp/j716c-postcfg-lease/lease-owner
lw=/run/j716c-postcfg-lease
cap=/tmp/j716c-clpc-private
root=/tmp/j716c-clpc/root
printf '%s\n' '314379b92714aca08fa31fb426781d63c510d035e5551de077deecc0901f077b  /tmp/j716c-postcfg-lease/lease-owner' '769bf935d667149fc90b8cc910eaa6ebe00c17ade6ac52c73651e13f391ceaa3  /tmp/j716c-postcfg/target' | sha256sum -c - >/dev/null
[ "$(cat /proc/1/comm)" = init ]
[ "$(cat /sys/devices/system/cpu/online)" = 0-17 ]
[ "$(tr -d '\000' </proc/device-tree/soc@2300000000/gpu@2302600000/status)" = okay ]
[ "$(tr -d '\000' </proc/device-tree/display-subsystem/status)" = disabled ]
[ "$(stat -f -c %T /)" = tmpfs ]
mkdir -m700 "$lw" "$cap"
cat /proc/sys/kernel/random/boot_id >"$lw/boot"
: >"$lw/owner.log";chmod 600 "$lw/owner.log" "$lw/boot"
"$lease" </dev/null >"$lw/owner.log" 2>&1 &
echo "$!" >"$lw/owner.pid"
i=0
while ! grep -qx LEASE_READY=1 "$lw/owner.log";do i=$((i+1));[ "$i" -lt 100 ];sleep 0.05;done
"$n" --lease-proof >"$cap/lease-before.txt"
finish() {
 rc=$?;trap - EXIT HUP INT TERM;set +e
 [ -z "${sampler:-}" ] || wait "$sampler" || rc=1
 dmesg >"$cap/dmesg-after.txt"
 cat /proc/interrupts >"$cap/interrupts-after.txt"
 [ -z "${status:-}" ] || cat "$status" >"$cap/clpc-after.txt"
 "$n" --lease-state >"$cap/lease-after.txt" || rc=1
 cp "$lw/owner.log" "$cap/lease-owner.log"
 printf 'GPU_OBSERVER_EXIT=%s\nTHERMAL_TELEMETRY=unavailable-no-new-SMC-probe\n' "$rc" >"$cap/scope.txt"
 timeout 5 busybox tar -czf /tmp/j716c-clpc-private.tar.gz -C "$cap" . || rc=1
 echo BEGIN_J716C_PRIVATE_DIAG_GZIP_BASE64
 timeout 5 busybox base64 /tmp/j716c-clpc-private.tar.gz || rc=1
 echo END_J716C_PRIVATE_DIAG_GZIP_BASE64
 echo GPU_OBSERVER_EXIT=$rc
 exit "$rc"
}
trap finish EXIT
trap 'exit 130' HUP INT TERM
echo GPU_PHASE=cpu-frequency-module
printf '%s\n' '80202e78842066fcc675543f344f681a0857d21fc0508718cc7c701432603767  /tmp/j716c-clpc/apple-soc-cpufreq.ko' | sha256sum -c - >/dev/null
[ ! -d /sys/module/apple_soc_cpufreq ]
timeout 5 insmod /tmp/j716c-clpc/apple-soc-cpufreq.ko
for id in 0 6 12;do
 p=/sys/devices/system/cpu/cpufreq/policy$id
 case "$id" in 0|6) frequency_cap=1992000;;12) frequency_cap=1980000;;esac
 printf 'policy%s initial_min=' "$id" >>"$cap/cpu-caps-initial.txt"
 cat "$p/scaling_min_freq" >>"$cap/cpu-caps-initial.txt"
 printf 'policy%s initial_max=' "$id" >>"$cap/cpu-caps-initial.txt"
 cat "$p/scaling_max_freq" >>"$cap/cpu-caps-initial.txt"
 [ "$(cat "$p/scaling_min_freq")" -le "$frequency_cap" ]
 case " $(cat "$p/scaling_available_frequencies") " in *" $frequency_cap "*) :;;*) exit 1;;esac
 printf '%s' "$frequency_cap" >"$p/scaling_max_freq"
 [ "$(cat "$p/scaling_max_freq")" = "$frequency_cap" ]
done
echo GPU_PHASE=cpu-caps-confirmed
for p in /sys/devices/system/cpu/cpufreq/policy*;do
 [ -d "$p" ];v=$(cat "$p/scaling_max_freq");[ "$v" -gt 0 ] && [ "$v" -le 2000000 ]
 printf '%s max=%s\n' "${p##*/}" "$v" >>"$cap/cpu-caps.txt"
done
[ "$(wc -l <"$cap/cpu-caps.txt")" = 3 ]
echo GPU_PHASE=runtime-integrity
cd /tmp
sha256sum -c /tmp/j716c-clpc/runtime.sha256 >"$cap/runtime-integrity.txt"
for name in dev proc sys;do mount --bind "/$name" "$root/$name";done
if ! grep -q ' /sys/kernel/debug debugfs ' /proc/mounts;then mount -t debugfs none /sys/kernel/debug;fi
echo GPU_PHASE=CLPC-provider
device=
for p in /sys/bus/platform/drivers/apple-clpc/*;do
 [ -L "$p/driver" ] || continue
 [ -z "$device" ];device=$p
done
[ -n "$device" ];status=/sys/kernel/debug/$(basename "$device")/status
[ -f "$status" ]
cat "$status" >"$cap/clpc-before.txt"
dmesg >"$cap/dmesg-before.txt"
cat /proc/interrupts >"$cap/interrupts-before.txt"
"$n" --lease-proof >"$cap/lease-GPU-before.txt"
echo GPU_PHASE=GPU-module
timeout 10 modprobe aurora
[ -c /dev/dri/renderD128 ]
readlink -f /sys/class/drm/renderD128/device/driver >"$cap/render-driver.txt"
[ "$(basename "$(cat "$cap/render-driver.txt")")" = aurora ]
cat "$status" >"$cap/clpc-attached.txt"
(
 i=0
 while [ "$i" -lt 250 ];do
  printf 'SAMPLE=%s UPTIME=' "$i";cat /proc/uptime
  cat "$status"
  i=$((i+1));sleep 0.01
 done
) >"$cap/clpc-samples.txt" 2>"$cap/clpc-sampler.err" &
sampler=$!
echo CLPC_GPU_EXCHANGE_BEGIN
timeout -k 2 25 env -i PATH=/usr/bin:/bin LC_ALL=C XDG_CACHE_HOME=/tmp/cache XDG_RUNTIME_DIR=/tmp/run /bin/busybox chroot "$root" /usr/bin/j716c-vulkan-checkerboard >"$cap/checkerboard.txt" 2>"$cap/checkerboard.err"
grep -qx 'PASS frames=32 pixels=24829472 mismatches=0' "$cap/checkerboard.txt"
echo CLPC_GPU_EXCHANGE_PASS
wait "$sampler";sampler=
[ ! -s "$cap/clpc-sampler.err" ]
"$n" --lease-proof >"$cap/lease-GPU-after.txt"
