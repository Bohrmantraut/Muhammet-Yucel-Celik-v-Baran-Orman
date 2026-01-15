import argparse
import time
import random
import re

import numpy as np
import serial

SYNC = 0xBB
IMG_BYTES = 28 * 28
PRED_RE = re.compile(r"PRED\s*=\s*(\d+)")
DT_RE = re.compile(r"DT\s*=\s*(\d+)")

def flush_input(ser: serial.Serial, seconds: float = 0.3) -> None:
    t0 = time.time()
    while time.time() - t0 < seconds:
        ser.read(4096)

def send_image(ser: serial.Serial, img28_u8: np.ndarray) -> None:
    if img28_u8.shape != (28, 28):
        raise ValueError(f"Expected (28,28), got {img28_u8.shape}")
    if img28_u8.dtype != np.uint8:
        img28_u8 = img28_u8.astype(np.uint8)

    ser.write(bytes([SYNC]))
    ser.write(img28_u8.flatten().tobytes())
    ser.flush()

def read_pred_line(ser: serial.Serial, timeout: float = 12.0):
    """
    Waits until a line containing PRED=<digit> arrives.
    Returns (pred_int, dt_ms_or_none, line_str) or (None, None, None) on timeout.
    """
    t0 = time.time()
    while time.time() - t0 < timeout:
        line = ser.readline()
        if not line:
            continue
        s = line.decode(errors="replace").strip()

        m = PRED_RE.search(s)
        if m:
            pred = int(m.group(1))
            dt_m = DT_RE.search(s)
            dt = int(dt_m.group(1)) if dt_m else None
            return pred, dt, s

    return None, None, None

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True, help="COM port, e.g. COM6")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--n", type=int, default=100)
    ap.add_argument("--seed", type=int, default=123)
    ap.add_argument("--pred_timeout", type=float, default=12.0)
    ap.add_argument("--delay_ms", type=int, default=120)
    ap.add_argument("--print_ok", action="store_true", help="Print OK lines too (default prints all anyway)")
    args = ap.parse_args()

    # MNIST
    import tensorflow as tf
    (_, _), (x_test, y_test) = tf.keras.datasets.mnist.load_data()

    rng = random.Random(args.seed)

    with serial.Serial(args.port, args.baud, timeout=0.1) as ser:
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        time.sleep(0.2)
        flush_input(ser, 0.3)

        got = 0
        correct = 0
        timeouts = 0
        wrong_tests = []  # list of dicts

        for i in range(1, args.n + 1):
            idx = rng.randrange(0, len(x_test))
            img = x_test[idx].astype(np.uint8)
            true = int(y_test[idx])

            send_image(ser, img)

            stm32_pred, dt, line = read_pred_line(ser, timeout=args.pred_timeout)
            if stm32_pred is None:
                timeouts += 1
                print(f"[{i:03d}/{args.n}] true={true} | stm32=(timeout)")
                flush_input(ser, 0.05)
                continue

            got += 1
            ok = (stm32_pred == true)
            correct += int(ok)

            dt_str = f"{dt}ms" if dt is not None else "?"
            status = "OK" if ok else "NO"

            # İstediğin format: sağ tarafta stm32 yazsın
            print(f"[{i:03d}/{args.n}] true={true} | stm32={stm32_pred} {status} | DT={dt_str} | {line}")

            if not ok:
                wrong_tests.append({
                    "test_no": i,
                    "true": true,
                    "stm32": stm32_pred,
                    "mnist_idx": idx,
                    "dt_ms": dt
                })

            if args.delay_ms > 0:
                time.sleep(args.delay_ms / 1000.0)

        print("\n==== SUMMARY ====")
        print(f"Total tests     : {args.n}")
        print(f"Got predictions : {got}")
        print(f"Timeouts        : {timeouts}")
        if got > 0:
            acc = 100.0 * correct / got
            print(f"Accuracy        : {acc:.2f}%  ({correct}/{got})")
        else:
            print("Accuracy        : N/A (no predictions received)")

        if wrong_tests:
            nums = [w["test_no"] for w in wrong_tests]
            print("\nWrong test numbers:")
            print(nums)

            print("\nDetails (first 20):")
            for w in wrong_tests[:20]:
                print(f'  test#{w["test_no"]}: true={w["true"]} stm32={w["stm32"]} dt={w["dt_ms"]}ms (mnist_idx={w["mnist_idx"]})')

if __name__ == "__main__":
    main()
