#!/usr/bin/env python3
"""Regenerates the SyncLens icon set: SVG sources in src/ and the PNGs Qt loads.

The mark is a lens whose rim is formed by two sync arrows. Tray icons sit on a
dark disc so they read on light and dark panels, and every state differs in
both colour and centre shape so it doesn't rely on colour vision alone.

Requires rsvg-convert. Usage: python3 generate_icons.py
"""
import math
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
SRC = HERE / "src"
NAVY = "#0B1B3A"
C = 128  # all artwork uses a 256x256 viewBox


def sync_arcs(radius, stroke, head, colour):
    """Two clockwise arcs with arrowheads, rotationally symmetric about the centre."""
    parts = []
    for start in (-165, 15):
        sweep = 122
        end = start + sweep
        a0, a1 = math.radians(start), math.radians(end)
        x0, y0 = C + radius * math.cos(a0), C + radius * math.sin(a0)
        x1, y1 = C + radius * math.cos(a1), C + radius * math.sin(a1)
        parts.append(
            f'<path d="M{x0:.2f} {y0:.2f}A{radius} {radius} 0 0 1 {x1:.2f} {y1:.2f}" '
            f'fill="none" stroke="{colour}" stroke-width="{stroke}" stroke-linecap="round"/>'
        )
        # Arrowhead: base straddles the rim at the arc's end, tip points along the tangent.
        tx, ty = -math.sin(a1), math.cos(a1)
        nx, ny = math.cos(a1), math.sin(a1)
        points = [
            (x1 + nx * head, y1 + ny * head),
            (x1 - nx * head, y1 - ny * head),
            (x1 + tx * head * 1.25, y1 + ty * head * 1.25),
        ]
        d = " ".join(f"{x:.2f},{y:.2f}" for x, y in points)
        parts.append(
            f'<polygon points="{d}" fill="{colour}" stroke="{colour}" '
            f'stroke-width="6" stroke-linejoin="round"/>'
        )
    return "".join(parts)


def svg(body, defs=""):
    return (
        '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 256 256">'
        f"<defs>{defs}</defs>{body}</svg>\n"
    )


def app_icon():
    defs = (
        '<linearGradient id="bg" x1="0" y1="0" x2="1" y2="1">'
        '<stop offset="0" stop-color="#1C8BFF"/><stop offset="1" stop-color="#0A3FC4"/>'
        "</linearGradient>"
        f'<clipPath id="glass"><circle cx="{C}" cy="{C}" r="46"/></clipPath>'
    )
    body = (
        '<rect x="8" y="8" width="240" height="240" rx="54" fill="url(#bg)"/>'
        + sync_arcs(86, 18, 21, "#FFFFFF")
        + f'<circle cx="{C}" cy="{C}" r="46" fill="{NAVY}"/>'
        + '<path d="M78 150L150 78" stroke="#6FE7FF" stroke-width="20" clip-path="url(#glass)"/>'
    )
    return svg(body, defs)


def tray_icon(colour, centre):
    return svg(
        f'<circle cx="{C}" cy="{C}" r="126" fill="{NAVY}"/>'
        + sync_arcs(93, 24, 28, colour)
        + centre
    )


def dot(colour):
    return f'<circle cx="{C}" cy="{C}" r="34" fill="{colour}"/>'


def ring(colour):
    return f'<circle cx="{C}" cy="{C}" r="26" fill="none" stroke="{colour}" stroke-width="18"/>'


def pause(colour):
    return "".join(
        f'<rect x="{x}" y="98" width="20" height="60" rx="6" fill="{colour}"/>' for x in (102, 134)
    )


def cross(colour):
    return (
        f'<path d="M108 108L148 148M148 108L108 148" fill="none" stroke="{colour}" '
        'stroke-width="22" stroke-linecap="round"/>'
    )


ICONS = {
    "synclens": (app_icon(), 512),
    "synclens-healthy": (tray_icon("#34D399", dot("#34D399")), 256),
    "synclens-connecting": (tray_icon("#FBBF24", ring("#FBBF24")), 256),
    "synclens-stopped": (tray_icon("#A7B0BF", pause("#A7B0BF")), 256),
    "synclens-error": (tray_icon("#F87171", cross("#F87171")), 256),
}


def main():
    SRC.mkdir(exist_ok=True)
    for name, (content, size) in ICONS.items():
        source = SRC / f"{name}.svg"
        source.write_text(content)
        subprocess.run(
            ["rsvg-convert", "-w", str(size), "-h", str(size), "-o", str(HERE / f"{name}.png"), str(source)],
            check=True,
        )


if __name__ == "__main__":
    main()
