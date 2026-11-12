# SPDX-License-Identifier: MIT
# Exact exercised FD-based peer; caller admits/configures/holds the exact TTY.
import os,select,time,hashlib,struct

def serial_exchange(fd,token,receipts):
    end=time.monotonic()+10
    def transfer(data=None,size=0):
        sending=data is not None;used=0;result=bytearray();poll=select.poll();poll.register(fd,select.POLLOUT if sending else select.POLLIN)
        wanted=len(data) if sending else size
        while used<wanted:
            left=end-time.monotonic()
            if left<=0:raise TimeoutError('ACM raw data absolute10s deadline')
            events=poll.poll(max(1,int(left*1000)))
            if not events or any(e&(select.POLLERR|select.POLLHUP|select.POLLNVAL) for _,e in events):raise RuntimeError('ACM data poll/timeout/HUP')
            try:
                value=os.write(fd,data[used:]) if sending else os.read(fd,wanted-used)
            except BlockingIOError:continue
            n=value if sending else len(value)
            if n<=0:raise RuntimeError('ACM data zero read/write')
            if not sending:result.extend(value)
            used+=n
        return bytes(result)
    def header(seq,n,direction):return b'J7ACM1\r\n'+token.encode()+struct.pack('<II',seq,n)+bytes([direction])+bytes(15)
    def payload(seq,n,direction):return bytes((i*29+(i>>8)+ord(token[i%32])+direction*97+seq*13)&255 for i in range(n))
    for seq,n in enumerate([4096,65536]):
        data=payload(seq,n,0);transfer(header(seq,n,0)+data)
        receipts.append({'direction':'host_to_target','sequence':seq,'bytes':n,'sha256':hashlib.sha256(data).hexdigest()})
        if transfer(size=64)!=header(seq,n,1):raise RuntimeError('ACM reply header mismatch')
        result=transfer(size=n)
        if result!=payload(seq,n,1):raise RuntimeError('ACM reply byte mismatch')
        receipts.append({'direction':'target_to_host','sequence':seq,'bytes':n,'sha256':hashlib.sha256(result).hexdigest()})
    poll=select.poll();poll.register(fd,select.POLLIN)
    if poll.poll(0):raise RuntimeError('unexpected additional serial bytes/events')
    return receipts

