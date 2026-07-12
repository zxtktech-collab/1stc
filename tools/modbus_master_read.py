#!/usr/bin/env python3
"""Modbus RTU master: read 4x PT100 temperatures from the acquisition board.

Examples:
  python3 tools/modbus_master_read.py --port /dev/ttyUSB0
  python3 tools/modbus_master_read.py --port COM3 --slave 1 --baud 9600
"""

from __future__ import annotations

import argparse
import struct
import sys
import time

try:
    from pymodbus.client import ModbusSerialClient
except ImportError:  # pymodbus < 3
    try:
        from pymodbus.client.sync import ModbusSerialClient  # type: ignore
    except ImportError:
        print("Install dependency: pip install 'pymodbus>=3.0.0' pyserial", file=sys.stderr)
        raise


TEMP_FAULT = 0x7FFF


def u16_to_i16(v: int) -> int:
    return struct.unpack("!h", struct.pack("!H", v & 0xFFFF))[0]


def decode_temp(reg: int) -> str:
    if reg == TEMP_FAULT:
        return "FAULT"
    return f"{u16_to_i16(reg) / 10.0:.1f} °C"


def decode_resistance(reg: int) -> str:
    if reg == 0xFFFF:
        return "FAULT"
    return f"{reg / 10.0:.1f} Ω"


def read_inputs(client: ModbusSerialClient, slave: int):
    # pymodbus v3 uses device_id= / slave= depending on version
    kwargs = dict(address=0, count=16)
    try:
        rr = client.read_input_registers(device_id=slave, **kwargs)
    except TypeError:
        try:
            rr = client.read_input_registers(slave=slave, **kwargs)
        except TypeError:
            rr = client.read_input_registers(unit=slave, **kwargs)

    if rr.isError():
        raise RuntimeError(f"Modbus error: {rr}")
    return list(rr.registers)


def main() -> int:
    p = argparse.ArgumentParser(description="Read PT100 temperatures via Modbus RTU RS485")
    p.add_argument("--port", required=True, help="Serial port, e.g. /dev/ttyUSB0 or COM3")
    p.add_argument("--baud", type=int, default=9600)
    p.add_argument("--slave", type=int, default=1, help="Modbus slave ID (default 1)")
    p.add_argument("--once", action="store_true", help="Single read then exit")
    p.add_argument("--interval", type=float, default=1.0, help="Poll interval seconds")
    args = p.parse_args()

    client = ModbusSerialClient(
        port=args.port,
        baudrate=args.baud,
        parity="N",
        stopbits=1,
        bytesize=8,
        timeout=1.0,
    )
    if not client.connect():
        print(f"Failed to open {args.port}", file=sys.stderr)
        return 1

    print(
        f"Connected {args.port} @ {args.baud} 8N1, slave={args.slave}\n"
        "Reading input registers 0..15 (FC04)\n"
    )

    try:
        while True:
            regs = read_inputs(client, args.slave)
            temps = regs[0:4]
            adcs = regs[4:8]
            rs = regs[8:12]
            status = regs[12]
            fw = regs[13]
            print("-" * 60)
            print(f"FW=0x{fw:04X}  STATUS=0x{status:04X}")
            for i in range(4):
                print(
                    f"CH{i}: {decode_temp(temps[i]):>10}   "
                    f"R={decode_resistance(rs[i]):>10}   ADC={adcs[i]}"
                )
            if args.once:
                break
            time.sleep(args.interval)
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        client.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
