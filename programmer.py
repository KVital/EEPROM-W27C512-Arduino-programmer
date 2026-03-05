import serial
import sys
import time
import os

def send_command(port, command):
    """Send a single-letter command to Arduino and print response."""
    ser = serial.Serial(port, 115200, timeout=1)
    time.sleep(2)  # wait for Arduino reset
    ser.write(command.encode())
    time.sleep(0.1)

    # Read all Arduino output until nothing left
    while ser.in_waiting:
        line = ser.readline().decode(errors='ignore').strip()
        if line:
            print(line)
    ser.close()


def write_rom(port, rom_file):

    if not os.path.exists(rom_file):
        print("ROM file not found")
        return

    with open(rom_file, "rb") as f:
        data = f.read()
        if len(data) != 65536:
            print("ROM file must be exactly 64KB")
            return

    ser = serial.Serial(port, 115200, timeout=5)
    time.sleep(2)             # wait for Arduino reset
    ser.reset_input_buffer()   # flush startup messages
    ser.write(b'W')            # send Write command
    time.sleep(0.1)
    ser.reset_input_buffer()   # flush any extra text

    BLOCK_SIZE = 256
    print("Writing ROM with block checksum...")

    for block_start in range(0, 65536, BLOCK_SIZE):
        block = data[block_start:block_start+BLOCK_SIZE]
        checksum = sum(block) & 0xFF

        while True:
            # send block
            ser.write(block)
            # send checksum
            ser.write(bytes([checksum]))

            # wait for Arduino response
            resp = ser.read(1)
            if not resp:
                print(f"Timeout on block starting at {block_start:04X}")
                return
            if resp[0] == 0xFF:
                break   # block OK
            print(f"Checksum mismatch on block {block_start:04X}, retrying...")

        # optional: wait for per-byte ACKs (could be skipped for speed)
        for i in range(BLOCK_SIZE):
            ack = ser.read(1)
            if not ack or ack[0] != 0xFF:
                print(f"ACK error at byte {block_start + i:04X}")
                return

        # print progress every 1KB
        if (block_start + BLOCK_SIZE) % 1024 == 0:
            print(f"Progress: {block_start + BLOCK_SIZE}/65536 bytes")

    ser.close()
    print("ROM programming complete with checksum verification.")


def read_rom(port, out_file):
    ser = serial.Serial(port, 115200, timeout=1)
    time.sleep(2)
    ser.write(b'V')  # verify/read mode

    with open(out_file, 'wb') as f:
        count = 0
        while count < 65536:
            byte = ser.read(1)
            if byte:
                f.write(byte)
                count += 1

    ser.close()
    print(f"Read complete, saved to {out_file}")

def verify_rom(port, rom_file):
    # Open serial
    ser = serial.Serial(port, 115200, timeout=1)
    time.sleep(2)  # wait for Arduino reset

    # Open ROM file
    with open(rom_file, 'rb') as f:
        rom_data = f.read()
        if len(rom_data) != 65536:
            print("ROM file must be exactly 64KB")
            return

    # Send 'V' to start verify mode
    ser.write(b'V')
    time.sleep(0.1)

    print("Verifying ROM...")

    errors = 0
    for addr in range(65536):
        byte = ser.read(1)
        if not byte:
            print(f"Timeout at address {addr:04X}")
            break
        chip_byte = byte[0]
        if chip_byte != rom_data[addr]:
            if errors < 10:
                print(f"Error at {addr:04X}: chip=0x{chip_byte:02X} expected=0x{rom_data[addr]:02X}")
            errors += 1

    ser.close()

    if errors == 0:
        print("Verify OK — ROM matches chip!")
    else:
        print(f"Verify finished with {errors} errors.")

def erase_rom(port):
    print("Sending erase command...")
    send_command(port, 'E')  # send erase command

def blank_check(port):
    print("Sending blank check command...")
    ser = serial.Serial(port, 115200, timeout=1)
    time.sleep(2)  # wait for Arduino reset
    ser.write(b'B')  # send blank check command

    # Wait and read Arduino output
    while True:
        line = ser.readline().decode(errors='ignore').strip()
        if line:
            print(line)
            if "OK" in line or "Not blank" in line:
                break  # exit when Arduino finishes
    ser.close()
# CLI
if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python programmer.py write/read/verify/erase/blank COMx ROM.bin")
        sys.exit(1)

    cmd = sys.argv[1].lower()
    port = sys.argv[2]
    file = sys.argv[3] if len(sys.argv) > 3 else None

    if cmd == "write":
        if not file:
            print("Specify ROM file to write")
        else:
            write_rom(port, file)
    elif cmd == "read":
        if not file:
            print("Specify output file for read")
        else:
            read_rom(port, file)
    elif cmd == "verify":
        if not file:
            print("Specify ROM file to verify")
        else:
            verify_rom(port, file)
    elif cmd == "erase":
        erase_rom(port)
    elif cmd == "blank":
        blank_check(port)
    else:
        print("Unknown command:", cmd)
