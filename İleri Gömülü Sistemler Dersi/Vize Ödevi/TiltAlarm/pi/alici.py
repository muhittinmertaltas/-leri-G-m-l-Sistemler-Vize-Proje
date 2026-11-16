
import serial
import csv
import time

from collections import deque
from datetime import datetime
from pathlib import Path

from protokol import parse_sensor_packet, calculate_angle


PORT = "/dev/serial0"
BAUD = 115200

SAMPLE_RATE = 50
WINDOW_SIZE = 10

ANGLE_ON = 30.0
ANGLE_OFF = 25.0

LOG_FILE = (
    Path.home()
    / "TiltAlarm"
    / "olcum"
    / "alarm_log.csv"
)

LOG_FILE.parent.mkdir(parents=True, exist_ok=True)

samples = deque(maxlen=WINDOW_SIZE)

alarm_active = False

valid_count = 0
bad_count = 0


def log_event(angle, event):
    file_exists = LOG_FILE.exists()

    with open(LOG_FILE, "a", newline="") as file:
        writer = csv.writer(file)

        if not file_exists:
            writer.writerow([
                "timestamp",
                "angle",
                "event"
            ])

        writer.writerow([
            datetime.now().isoformat(),
            f"{angle:.2f}",
            event
        ])


def main():

    global alarm_active
    global valid_count
    global bad_count

    print("TILT ALARM RASPBERRY PI BASLADI")
    print("UART:", PORT)
    print("BAUD:", BAUD)

    try:
        with serial.Serial(
            PORT,
            BAUD,
            timeout=1
        ) as ser:

            while True:

                raw_line = ser.readline()

                if not raw_line:
                    continue

                try:
                    line = raw_line.decode(
                        "ascii"
                    ).strip()

                except UnicodeDecodeError:
                    bad_count += 1
                    print("Gecersiz karakter")
                    continue

                values = parse_sensor_packet(line)

                if values is None:
                    bad_count += 1
                    print("Gecersiz paket veya checksum hatasi")
                    continue

                ax, ay, az, gx, gy, gz = values

                angle = calculate_angle(ax, az)

                samples.append(angle)

                valid_count += 1

                if len(samples) < WINDOW_SIZE:
                    continue

                filtered_angle = sum(samples) / WINDOW_SIZE

                if not alarm_active and filtered_angle >= ANGLE_ON:

                    alarm_active = True

                    log_event(
                        filtered_angle,
                        "ALARM_ON"
                    )

                    print(
                        f"ALARM ON | Aci: {filtered_angle:.2f}"
                    )

                elif alarm_active and filtered_angle <= ANGLE_OFF:

                    alarm_active = False

                    log_event(
                        filtered_angle,
                        "ALARM_OFF"
                    )

                    print(
                        f"ALARM OFF | Aci: {filtered_angle:.2f}"
                    )

                else:

                    print(
                        f"Aci: {filtered_angle:.2f} derece"
                    )

    except serial.SerialException as error:

        print(f"UART hatasi: {error}")

    except KeyboardInterrupt:

        print("\nProgram durduruldu.")

    finally:

        print("Gecerli paket:", valid_count)
        print("Hatali paket:", bad_count)


if __name__ == "__main__":
    main()
