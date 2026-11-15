"""Execute the real 1.1.1 constructor against guarded memory (private dump required).

Optional developer check: Python unicorn is needed. No game instructions are distributed
by this test. This verifies the native object boundary, not gameplay or travel arrivals.
"""
import argparse
from pathlib import Path
import struct
import sys
import re

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from profile import BASE, make_patch


def check(image):
    from unicorn import Uc, UC_ARCH_ARM64, UC_MODE_ARM, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn.arm64_const import UC_ARM64_REG_X0, UC_ARM64_REG_X1, UC_ARM64_REG_X2
    from unicorn.arm64_const import UC_ARM64_REG_SP, UC_ARM64_REG_PC, UC_ARM64_REG_LR

    make_patch(image)
    source = (Path(__file__).resolve().parents[1] / 'src/travel_runtime.c').read_text()
    match = re.search(r'EVENT_REQUEST_BYTES = (0x[0-9a-fA-F]+)', source)
    assert match, 'native request buffer declaration missing'
    capacity = int(match[1], 16)
    flat = image[0x100:]
    machine = Uc(UC_ARCH_ARM64, UC_MODE_ARM)
    machine.mem_map(0x80D99000, 0x2000)
    machine.mem_write(0x80D99000, flat[0x80D99000 - BASE:0x80D9B000 - BASE])
    machine.mem_map(0x84C9D000, 0x2000)
    machine.mem_map(0x83C62000, 0x1000)
    machine.mem_map(0x100000, 0x1000)
    machine.mem_map(0x200000, 0x4000)
    machine.mem_map(0x300000, 0x4000)
    machine.mem_write(0x84C9DEE0, struct.pack('<Q', 0x100000))
    machine.mem_write(0x84C9E820, struct.pack('<Q', 0x100100))
    request = 0x200100
    machine.mem_write(request - 16, b'\xA5' * (capacity + 32))
    machine.reg_write(UC_ARM64_REG_X0, request)
    machine.reg_write(UC_ARM64_REG_SP, 0x303000)
    touched = []

    def code_hook(uc, address, size, unused):
        if address == 0x80D99F10:  # PACIBSP, irrelevant to the memory-layout assertion.
            uc.reg_write(UC_ARM64_REG_PC, address + 4)
        elif address == 0x83C62370:  # The constructor's single memcpy import.
            destination = uc.reg_read(UC_ARM64_REG_X0)
            source = uc.reg_read(UC_ARM64_REG_X1)
            length = uc.reg_read(UC_ARM64_REG_X2)
            uc.mem_write(destination, bytes(uc.mem_read(source, length)))
            touched.append((destination, length))
            uc.reg_write(UC_ARM64_REG_PC, uc.reg_read(UC_ARM64_REG_LR))
        elif address == 0x80D9A2EC:  # RETAB; all constructor stores have completed.
            uc.emu_stop()

    def write_hook(uc, access, address, size, value, unused):
        if request - 16 <= address < request + 0x2000:
            touched.append((address, size))

    machine.hook_add(UC_HOOK_CODE, code_hook)
    machine.hook_add(UC_HOOK_MEM_WRITE, write_hook)
    machine.emu_start(0x80D99F10, 0, count=10000)
    high = max(address + size - request for address, size in touched)
    assert high == 0x1B1C, hex(high)
    assert high > 0x1AA0, 'old undersized buffer must reproduce the boundary violation'
    assert high <= capacity, f'native request exceeds the C buffer: {high:#x} > {capacity:#x}'
    assert machine.mem_read(request - 16, 16) == b'\xA5' * 16
    assert machine.mem_read(request + capacity, 16) == b'\xA5' * 16
    print('Real native constructor writes through +0x1b1b; 0x1b20 object guards are intact.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--main', type=Path, required=True)
    args = parser.parse_args()
    check(args.main.read_bytes())
