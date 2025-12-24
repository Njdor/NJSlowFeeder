import sys, time
from collections import deque

import serial
import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator

# ====== USER SETTINGS ======
PORT = "/dev/cu.usbmodem2301"  
BAUD = 115200
MAX_POINTS = 600      
REFRESH_SEC = 0.01    
# ===========================

def parse_csv(line):
    """Parse a CSV line into floats. Returns None if it can't parse."""
    try:
        if not line.startswith("rate,"):
            return None
        parts = line.strip().split(",")
        if len(parts) < 4:
            return None
        rr = float(parts[1])
        ef = float(parts[2])
        es = float(parts[3])
        return rr, ef, es 
    except Exception:
        return None

def main():
    print(f"Opening serial: {PORT} @ {BAUD}")
    ser = serial.Serial(PORT, BAUD, timeout=1)

    # Rolling buffers
    xs = deque(maxlen=MAX_POINTS)
    rawRate = deque(maxlen=MAX_POINTS)
    emaFast = deque(maxlen=MAX_POINTS)
    emaSlow = deque(maxlen=MAX_POINTS)
    pval = deque(maxlen=MAX_POINTS)

    i = 0
    plt.ion()
    fig, ax1 = plt.subplots(figsize=(9, 5))

    # Left axis: emaFast, emaSlow
    ln_rr, = ax1.plot([], [], label="rawRate")
    ln_fast, = ax1.plot([], [], label="emaFast")
    ln_slow, = ax1.plot([], [], label="emaSlow")
    ax1.set_xlabel("T")
    ax1.set_ylabel("Rate")
    ax1.xaxis.set_major_locator(MultipleLocator(1))
    ax1.grid(True)

    # Right axis: pval (secondary y)

    # Build a combined legend
    lines = [ln_rr, ln_fast, ln_slow]
    labels = [l.get_label() for l in lines]
    ax1.legend(lines, labels, loc="upper right")

    last_draw = time.time()

    try:
        while True:
            line = ser.readline().decode(errors="ignore").strip()
            if not line:
                # no data yet
                continue

            vals = parse_csv(line)
            if vals is None:
                # line didn’t parse, skip
                # print("skipped:", line)
                continue

            rr, ef, es = vals
            xs.append(i); i += 1
            rawRate.append(rr)
            emaFast.append(ef)
            emaSlow.append(es)

            # Update line data
            ln_rr.set_data(xs, rawRate)
            ln_fast.set_data(xs, emaFast)
            ln_slow.set_data(xs, emaSlow)

            # Rescale axes
            ax1.relim(); ax1.autoscale_view()

            # Throttle redraws for smoothness/CPU
            now = time.time()
            if now - last_draw >= REFRESH_SEC:
                plt.pause(0.001)  # yield to UI loop
                last_draw = now

    except KeyboardInterrupt:
        print("\nStopping.")
    finally:
        ser.close()

if __name__ == "__main__":
    if len(sys.argv) > 1:
        # allow passing the port as arg: python live_plot.py COM5
        PORT = sys.argv[1]
    main()
