"""One identity-guarded standard GET_STATUS; no claim, reset or SET request."""
import ctypes
import json
import os
from pathlib import Path
import stat
import sys
import time

port = Path('/sys/bus/usb/devices/1-7')
assert os.geteuid() == 0 and os.uname().machine == 'x86_64'
assert len(sys.argv) == 4 and all(v.isdecimal() for v in sys.argv[1:])
expected = tuple(map(int, sys.argv[1:]))

def identity():
    for name, value in {'idVendor': '1d6b', 'idProduct': '0104', 'bcdDevice': '0214',
                        'product': 'J716c ACM finite RAM probe',
                        'serial': 'J716C-ACM-FINITE-TEST', 'speed': '480',
                        'bNumInterfaces': '2'}.items():
        if (port / name).read_text().strip() != value:
            raise RuntimeError('wrong exact-port schema')
    observed = (int((port / 'busnum').read_text()),
                int((port / 'devnum').read_text()), port.stat().st_ino)
    if observed != expected:
        raise RuntimeError('USB generation changed')

class Control(ctypes.Structure):
    _fields_ = [('request_type', ctypes.c_uint8), ('request', ctypes.c_uint8),
                ('value', ctypes.c_uint16), ('index', ctypes.c_uint16),
                ('length', ctypes.c_uint16), ('timeout', ctypes.c_uint32),
                ('data', ctypes.c_void_p)]

assert ctypes.sizeof(Control) == 24 and Control.data.offset == 16
identity()
bus, device, _ = expected
assert 1 <= bus <= 999 and 1 <= device <= 127
fd = os.open(f'/dev/bus/usb/{bus:03}/{device:03}', os.O_RDWR | os.O_CLOEXEC | os.O_NOFOLLOW)
try:
    node = os.fstat(fd)
    assert stat.S_ISCHR(node.st_mode) and os.major(node.st_rdev) == 189
    assert os.minor(node.st_rdev) == (bus - 1) * 128 + device - 1
    identity()
    data = (ctypes.c_uint8 * 2)()
    control = Control(0x80, 0, 0, 0, 2, 500, ctypes.addressof(data))
    libc = ctypes.CDLL(None, use_errno=True)
    libc.ioctl.argtypes = [ctypes.c_int, ctypes.c_ulong, ctypes.c_void_p]
    libc.ioctl.restype = ctypes.c_int
    start = time.time_ns() // 1000
    result = libc.ioctl(fd, 0xC0185500, ctypes.byref(control))
    if result < 0:
        raise OSError(ctypes.get_errno(), 'GET_STATUS ioctl failed')
    if result != 2:
        raise RuntimeError('short GET_STATUS result')
    identity()
    print(json.dumps(dict(request=dict(bmRequestType='80', bRequest='00', wValue='0000',
                                      wIndex='0000', wLength='0002'),
                          returned_bytes=list(data), result=result,
                          begin_realtime_us=start, end_realtime_us=time.time_ns() // 1000)))
finally:
    os.close(fd)
