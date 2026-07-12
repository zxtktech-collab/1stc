#!/usr/bin/env python3
"""Offline check of PT100 resistance → temperature math (matches firmware)."""

import math

CVD_A = 3.9083e-3
CVD_B = -5.775e-7
R0 = 100.0
IEXC = 0.001
GAIN = 21.0
VREF = 3.3
ADC_MAX = 4095.0


def temp_from_r(r: float) -> float:
    r_norm = r / R0
    disc = (CVD_A * CVD_A) - (4.0 * CVD_B * (1.0 - r_norm))
    return (-CVD_A + math.sqrt(disc)) / (2.0 * CVD_B)


def adc_from_r(r: float) -> int:
    v_rt = r * IEXC
    vadc = v_rt * GAIN
    return int(round(vadc * ADC_MAX / VREF))


def main() -> None:
    # PT100 approx: 0°C=100Ω, 25°C≈109.73Ω, 100°C≈138.51Ω
    cases = [(100.0, 0.0), (109.73, 25.0), (138.51, 100.0)]
    for r, expect in cases:
        t = temp_from_r(r)
        adc = adc_from_r(r)
        err = abs(t - expect)
        print(f"R={r:7.2f} Ω  ADC≈{adc:4d}  T={t:7.2f} °C  (expect {expect:.1f}, err={err:.3f})")
        assert err < 0.2, err
        assert 0 < adc < 4095
    print("OK")


if __name__ == "__main__":
    main()
