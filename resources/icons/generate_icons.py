#!/usr/bin/env python3
"""Regenerates the SyncLens icons. Each icon is a single SVG in this directory,
used both when installed and when bundled in the binary.

The mark is a lens whose rim is formed by two sync arrows. The app icon is a
flat circle, matching circular icon themes such as Tela-circle.

Tray icons (synclens-tray-<state>.svg) use KDE's current-color-scheme
stylesheet for the arrows: when installed, Plasma paints them in the panel's
text colour. When they aren't installed, TrayController renders the bundled
copy with the arrows in the palette's text colour. Only the centre is coloured,
with fixed vivid colours (healthy matches the CachyOS mint), and every state
differs in both colour and shape so it doesn't rely on colour vision alone.

Usage: python3 generate_icons.py
"""
import math
from pathlib import Path

HERE = Path(__file__).resolve().parent
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


def glare(radius, offset, colour, opacity):
    """The part of the lens (circle of `radius` at the centre) above-left of the
    diagonal line x + y = 2C - offset*sqrt(2), drawn as an explicit circular
    segment. Avoids clipPath, which some renderers (e.g. Qt SVG, used by Plasma
    for icons) ignore, which would let the glare spill over the whole icon."""
    half = math.sqrt(radius * radius - offset * offset)  # half chord length
    mx, my = C - offset / math.sqrt(2), C - offset / math.sqrt(2)  # chord midpoint
    dx, dy = half / math.sqrt(2), -half / math.sqrt(2)  # along the chord
    x0, y0 = mx - dx, my - dy  # lower-left end
    x1, y1 = mx + dx, my + dy  # upper-right end
    large = 1 if offset < 0 else 0
    return (
        f'<path d="M{x0:.2f} {y0:.2f}A{radius} {radius} 0 {large} 1 {x1:.2f} {y1:.2f}Z" '
        f'fill="{colour}" fill-opacity="{opacity}"/>'
    )


def app_icon():
    # Flat disc with a white glyph, sized like Tela-circle (r=27 of 64).
    # Glass glare: a soft tint over the top-left half, a brighter band along its
    # outer edge, and a faint rim light on the opposite edge.
    body = (
        f'<circle cx="{C}" cy="{C}" r="108" fill="#1C7CF0"/>'
        + sync_arcs(70, 14, 17, "#FFFFFF")
        + f'<circle cx="{C}" cy="{C}" r="34" fill="#FFFFFF"/>'
        + f'<circle cx="{C}" cy="{C}" r="24" fill="{NAVY}"/>'
        + glare(24, 0, "#6FE7FF", 0.45)
        + glare(24, 20 / math.sqrt(2), "#FFFFFF", 0.2)
        + '<path d="M146 134A19 19 0 0 1 134 146" fill="none" stroke="#6FE7FF" '
        'stroke-opacity="0.55" stroke-width="4" stroke-linecap="round"/>'
    )
    return svg(body)


# Default arrow colour, used when the icon isn't drawn by Plasma. TrayController
# replaces TEXT with the palette's text colour, so keep it unique.
TEXT = "#DFDFDF"
SCHEME = (
    '<style id="current-color-scheme" type="text/css">'
    f".ColorScheme-Text{{color:{TEXT}}}"
    "</style>"
)


def themed(markup, cls):
    """Makes every fill/stroke in markup follow a ColorScheme class."""
    return f'<g class="{cls}">' + markup.replace(
        "#FFFFFF", "currentColor"
    ) + "</g>"


def themed_tray(centre_markup):
    return svg(
        themed(sync_arcs(93, 24, 28, "#FFFFFF"), "ColorScheme-Text") + centre_markup,
        SCHEME,
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
    "synclens": app_icon(),
    "synclens-tray-healthy": themed_tray(dot("#02FDC2")),
    "synclens-tray-connecting": themed_tray(ring("#FBBF24")),
    "synclens-tray-stopped": themed_tray(pause("#60A5FA")),
    "synclens-tray-error": themed_tray(cross("#F87171")),
}


def main():
    for name, content in ICONS.items():
        (HERE / f"{name}.svg").write_text(content)


if __name__ == "__main__":
    main()
