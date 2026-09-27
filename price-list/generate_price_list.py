#!/usr/bin/env python3
"""Generate the AMSKY CTCP machine price list PDF."""

from pathlib import Path

from reportlab.lib.pagesizes import A4
from reportlab.lib.colors import Color, white
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.pdfgen import canvas

ROOT = Path(__file__).resolve().parent
OUT = ROOT / "amsky-ctcp-price-list.pdf"

FONT_DIR = Path("/usr/share/fonts/truetype/macos")
pdfmetrics.registerFont(TTFont("Inter", str(FONT_DIR / "Inter-Regular.ttf")))
pdfmetrics.registerFont(TTFont("Inter-Medium", str(FONT_DIR / "Inter-Medium.ttf")))
pdfmetrics.registerFont(TTFont("Inter-Semi", str(FONT_DIR / "Inter-SemiBold.ttf")))
pdfmetrics.registerFont(TTFont("Inter-Bold", str(FONT_DIR / "Inter-Bold.ttf")))

NAVY = Color(0.078, 0.161, 0.235)
INK = Color(0.086, 0.145, 0.196)
MUTED = Color(0.365, 0.416, 0.455)
RULE = Color(0.855, 0.827, 0.780)
PAPER = Color(0.965, 0.953, 0.925)
CARD = Color(1, 0.992, 0.976)
GOLD = Color(0.651, 0.518, 0.290)
GOLD_SOFT = Color(0.945, 0.910, 0.835)
ROW = Color(0.973, 0.961, 0.937)
GROUP = Color(0.910, 0.933, 0.945)

PAGE_W, PAGE_H = A4
MARGIN = 36

# Prices are the quoted package prices in US dollars.
# Each package is CTCP engine + processor + computer.
OFFERS = [
    {
        "format": "A1",
        "channels": 32,
        "price": 7000,
    },
    {
        "format": "A1",
        "channels": 48,
        "price": 8000,
    },
    {
        "format": "A2",
        "channels": 32,
        "price": 8000,
    },
    {
        "format": "A2",
        "channels": 48,
        "price": 9000,
    },
]

PARTS = [
    ("01", "CTCP engine", "Plate imaging unit"),
    ("02", "Processor", "Plate processor"),
    ("03", "Computer", "Control computer"),
]


def tracked_width(c, text, font, size, tracking):
    if not text:
        return 0
    widths = [c.stringWidth(ch, font, size) for ch in text]
    return sum(widths) + tracking * (len(text) - 1)


def draw_tracked(c, text, x, y, font, size, tracking, color, align="left"):
    c.setFont(font, size)
    c.setFillColor(color)
    total = tracked_width(c, text, font, size, tracking)
    if align == "right":
        x -= total
    elif align == "center":
        x -= total / 2
    for ch in text:
        c.drawString(x, y, ch)
        x += c.stringWidth(ch, font, size) + tracking
    return total


def wrap_text(c, text, font, size, max_width):
    lines = []
    current = ""
    for word in text.split():
        trial = word if not current else f"{current} {word}"
        if c.stringWidth(trial, font, size) <= max_width:
            current = trial
        else:
            if current:
                lines.append(current)
            current = word
    if current:
        lines.append(current)
    return lines


def money(amount):
    return f"{amount:,.0f}"


def draw_header(c):
    header_h = 118
    c.setFillColor(NAVY)
    c.rect(0, PAGE_H - header_h, PAGE_W, header_h, fill=1, stroke=0)
    c.setFillColor(GOLD)
    c.rect(0, PAGE_H - header_h - 4, PAGE_W, 4, fill=1, stroke=0)

    draw_tracked(
        c, "AMSKY", MARGIN, PAGE_H - 58, "Inter-Bold", 26, 3.2, white
    )
    draw_tracked(
        c,
        "COMPUTER TO CONVENTIONAL PLATE",
        MARGIN,
        PAGE_H - 82,
        "Inter-Medium",
        8,
        1.15,
        GOLD,
    )

    draw_tracked(
        c,
        "PRICE LIST",
        PAGE_W - MARGIN,
        PAGE_H - 52,
        "Inter-Semi",
        11,
        1.8,
        white,
        align="right",
    )
    c.setFillColor(Color(0.78, 0.82, 0.86))
    c.setFont("Inter", 9)
    c.drawRightString(PAGE_W - MARGIN, PAGE_H - 72, "27 September 2026")
    c.setFont("Inter", 8.5)
    c.drawRightString(PAGE_W - MARGIN, PAGE_H - 88, "Prices in US dollars")


def draw_intro(c):
    y = PAGE_H - 158
    c.setFillColor(INK)
    c.setFont("Inter-Semi", 16)
    c.drawString(MARGIN, y, "CTCP machine packages")

    body = (
        "Four AMSKY CTCP configurations. Each price is for one complete set "
        "and already includes the CTCP engine, the processor, and the computer."
    )
    lines = wrap_text(c, body, "Inter", 10, PAGE_W - MARGIN * 2)
    text_y = y - 22
    c.setFillColor(MUTED)
    c.setFont("Inter", 10)
    for line in lines:
        c.drawString(MARGIN, text_y, line)
        text_y -= 14
    return text_y - 10


def draw_parts(c, top):
    gap = 10
    card_w = (PAGE_W - MARGIN * 2 - gap * 2) / 3
    card_h = 78
    y = top - card_h

    for index, (number, title, detail) in enumerate(PARTS):
        x = MARGIN + index * (card_w + gap)
        c.setFillColor(CARD)
        c.setStrokeColor(RULE)
        c.setLineWidth(0.8)
        c.roundRect(x, y, card_w, card_h, 6, fill=1, stroke=1)

        c.setFillColor(GOLD_SOFT)
        c.circle(x + 18, y + card_h - 22, 10, fill=1, stroke=0)
        c.setFillColor(NAVY)
        c.setFont("Inter-Semi", 7.5)
        c.drawCentredString(x + 18, y + card_h - 25, number)

        c.setFillColor(INK)
        c.setFont("Inter-Semi", 11)
        c.drawString(x + 36, y + card_h - 26, title)
        c.setFillColor(MUTED)
        c.setFont("Inter", 8.5)
        c.drawString(x + 16, y + 18, detail)

    return y - 22


def draw_table(c, top):
    rows = []
    last_format = None
    for offer in OFFERS:
        if offer["format"] != last_format:
            rows.append(("group", offer["format"]))
            last_format = offer["format"]
        rows.append(("item", offer))

    x = MARGIN
    width = PAGE_W - MARGIN * 2
    header_h = 28
    group_h = 26
    row_h = 46
    body_h = sum(group_h if kind == "group" else row_h for kind, _ in rows)
    table_h = header_h + body_h
    bottom = top - table_h
    radius = 6

    c.saveState()
    clip = c.beginPath()
    clip.roundRect(x, bottom, width, table_h, radius)
    c.clipPath(clip, stroke=0, fill=0)

    c.setFillColor(CARD)
    c.rect(x, bottom, width, table_h, fill=1, stroke=0)

    c.setFillColor(NAVY)
    c.rect(x, top - header_h, width, header_h, fill=1, stroke=0)

    headers = [
        (x + 18, "Format", "left"),
        (x + 118, "Laser channels", "left"),
        (x + 268, "Included in the price", "left"),
        (x + width - 18, "Price (USD)", "right"),
    ]
    c.setFillColor(white)
    c.setFont("Inter-Medium", 8)
    for hx, label, align in headers:
        if align == "right":
            c.drawRightString(hx, top - 18, label.upper())
        else:
            c.drawString(hx, top - 18, label.upper())

    cursor = top - header_h
    item_index = 0
    for kind, payload in rows:
        if kind == "group":
            cursor -= group_h
            c.setFillColor(GROUP)
            c.rect(x, cursor, width, group_h, fill=1, stroke=0)
            c.setFillColor(GOLD)
            c.rect(x, cursor, 4, group_h, fill=1, stroke=0)
            c.setFillColor(NAVY)
            c.setFont("Inter-Semi", 10)
            c.drawString(x + 18, cursor + 8, f"{payload} size")
            continue

        cursor -= row_h
        if item_index % 2 == 1:
            c.setFillColor(ROW)
            c.rect(x, cursor, width, row_h, fill=1, stroke=0)
        item_index += 1

        offer = payload
        baseline = cursor + 17
        c.setFillColor(INK)
        c.setFont("Inter-Semi", 12)
        c.drawString(x + 18, baseline, offer["format"])

        channel = str(offer["channels"])
        c.setFont("Inter-Semi", 13)
        c.drawString(x + 118, baseline, channel)
        channel_w = c.stringWidth(channel, "Inter-Semi", 13)
        c.setFillColor(MUTED)
        c.setFont("Inter", 9)
        c.drawString(x + 118 + channel_w + 5, baseline + 1, "channel")

        c.setFillColor(INK)
        c.setFont("Inter", 9)
        c.drawString(x + 268, baseline + 1, "Engine  +  processor  +  computer")

        c.setFillColor(NAVY)
        c.setFont("Inter-Bold", 14)
        c.drawRightString(x + width - 18, baseline, money(offer["price"]))

    c.restoreState()

    c.setStrokeColor(RULE)
    c.setLineWidth(0.9)
    c.roundRect(x, bottom, width, table_h, radius, fill=0, stroke=1)
    return bottom


def draw_notes(c, top):
    notes = [
        "Currency is the United States dollar (USD).",
        "Each line is a separate package. The figure is the price of that one set.",
        "Every set includes three parts: the CTCP engine, the processor, and the computer.",
        "A1 and A2 are the plate-format classes. 32 and 48 are the laser channel counts on the engine.",
    ]
    box_h = 112
    y = top - box_h
    c.setFillColor(CARD)
    c.setStrokeColor(RULE)
    c.setLineWidth(0.8)
    c.roundRect(MARGIN, y, PAGE_W - MARGIN * 2, box_h, 6, fill=1, stroke=1)

    c.setFillColor(GOLD)
    c.rect(MARGIN, y + 6, 4, box_h - 12, fill=1, stroke=0)

    c.setFillColor(NAVY)
    c.setFont("Inter-Semi", 9)
    c.drawString(MARGIN + 18, y + box_h - 22, "Notes")

    text_y = y + box_h - 42
    c.setFont("Inter", 8.5)
    for note in notes:
        c.setFillColor(GOLD)
        c.circle(MARGIN + 22, text_y + 3, 2, fill=1, stroke=0)
        c.setFillColor(INK)
        c.drawString(MARGIN + 32, text_y, note)
        text_y -= 17

    c.setFillColor(MUTED)
    c.setFont("Inter", 8)
    c.drawString(MARGIN, 26, "AMSKY CTCP  ·  Machine price list")
    c.drawRightString(PAGE_W - MARGIN, 26, "Page 1 of 1")


def build():
    c = canvas.Canvas(str(OUT), pagesize=A4)
    c.setTitle("AMSKY CTCP Machine Price List")
    c.setAuthor("AMSKY")
    c.setSubject("A1 and A2 CTCP packages with engine, processor, and computer")
    c.setCreator("AMSKY CTCP price list")

    c.setFillColor(PAPER)
    c.rect(0, 0, PAGE_W, PAGE_H, fill=1, stroke=0)

    draw_header(c)
    cursor = draw_intro(c)
    cursor = draw_parts(c, cursor)
    draw_tracked(c, "CONFIGURATIONS", MARGIN, cursor - 2, "Inter-Semi", 8, 1.3, NAVY)
    cursor = draw_table(c, cursor - 18)
    draw_notes(c, cursor - 22)

    c.showPage()
    c.save()
    return OUT


if __name__ == "__main__":
    path = build()
    print(path)
