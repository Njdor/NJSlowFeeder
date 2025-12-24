import math
import sys
import time
from collections import deque

import matplotlib.pyplot as plt
import serial
from matplotlib.ticker import MultipleLocator

# ====== USER SETTINGS ======
PORT = "/dev/cu.usbmodem2301"
BAUD = 115200
MAX_POINTS = 600
REFRESH_SEC = 0.01
# ===========================

def parse_diff(line: str):
    if not line.startswith("gap,"):
        return None
    parts = [p for p in line.split(",") if p]
    if len(parts) < 3:
        return None
    try:
        ema_diff = float(parts[1])
        var = float(parts[2])
    except ValueError:
        return None
    return ema_diff, var

def main():
    port = PORT
    if len(sys.argv) > 1:
        port = sys.argv[1]

    print(f"Opening serial: {port} @ {BAUD}")
    ser = serial.Serial(port, BAUD, timeout=1)

    xs = deque(maxlen=MAX_POINTS)
    ema_diff_vals = deque(maxlen=MAX_POINTS)
    upper_vals = deque(maxlen=MAX_POINTS)
    lower_vals = deque(maxlen=MAX_POINTS)

    i = 0
    plt.ion()
    fig, ax_main = plt.subplots(figsize=(9, 5))

    ln_diff, = ax_main.plot([], [], label="emaGap")
    ln_upper, = ax_main.plot([], [], label="+1.3x sigma", linestyle="--")
    ln_lower, = ax_main.plot([], [], label="-1.3x sigma", linestyle="--")

    ax_main.set_ylabel("Signal")
    ax_main.set_xlabel("T")
    ax_main.grid(True)
    ax_main.xaxis.set_major_locator(MultipleLocator(1))
    ax_main.yaxis.set_major_locator(MultipleLocator(0.1))

    ax_main.axhline(0.0, color="tab:gray", linestyle=":", linewidth=0.8)

    ax_main.legend(loc="upper right")

    last_draw = time.time()

    try:
        while True:
            line = ser.readline().decode(errors="ignore").strip()
            if not line:
                continue

            parsed = parse_diff(line)
            if parsed is None:
                continue

            ema_diff, var = parsed
            sigma = math.sqrt(var) if var > 0.0 else 0.0
            upper = 1.3 * sigma
            lower = -1.3 * sigma

            xs.append(i)
            ema_diff_vals.append(ema_diff)
            upper_vals.append(upper)
            lower_vals.append(lower)
            i += 1

            ln_diff.set_data(xs, ema_diff_vals)
            ln_upper.set_data(xs, upper_vals)
            ln_lower.set_data(xs, lower_vals)

            ax_main.relim()
            ax_main.autoscale_view()

            now = time.time()
            if now - last_draw >= REFRESH_SEC:
                plt.pause(0.001)
                last_draw = now

    except KeyboardInterrupt:
        print("\nStopping.")
    finally:
        ser.close()

if __name__ == "__main__":
    main()
