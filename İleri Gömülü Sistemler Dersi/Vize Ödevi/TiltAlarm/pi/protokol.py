
import math


def checksum(text):
    result = 0

    for byte in text.encode("ascii"):
        result ^= byte

    return result


def parse_sensor_packet(line):
    try:
        line = line.strip()

        parts = line.split(",")

        if len(parts) != 8:
            return None

        if parts[0] != "DATA":
            return None

        payload = ",".join(parts[:-1])
        received_checksum = int(parts[-1], 16)

        if checksum(payload) != received_checksum:
            return None

        values = [float(value) for value in parts[1:7]]

        return values

    except (ValueError, UnicodeError):
        return None


def calculate_angle(ax, az):
    return math.degrees(math.atan2(ax, az))
