#!/usr/bin/env python3
"""Generate the CTP machine quotation PDF."""

from pathlib import Path

from reportlab.lib.pagesizes import A4
from reportlab.lib.colors import Color, white
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.pdfgen import canvas

ROOT = Path(__file__).resolve().parent
OUT = ROOT / "ctp-machine-quotation.pdf"

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

PAGE_W, PAGE_H = A4
MARGIN = 36

# Each price is for one machine, EXW, with freight to the Guangzhou warehouse included.
MACHINES = [
    {
        "brand": "CRON",
        "model": "UVP-4664GX+",
        "detail": "64-channel UV CTP",
        "price": 14000,
    },
    {
        "brand": "AMSKY",
        "model": "T864",
        "detail": "64-channel thermal CTP",
        "price": 16000,
    },
    {
        "brand": "AMSKY",
        "model": "U8128",
        "detail": "128-channel UV CTP",
        "price": 20000,
    },
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

    draw_tracked(c, "QUOTATION", MARGIN, PAGE_H - 58, "Inter-Bold", 26, 2.4, white)
    draw_tracked(
        c,
        "CTP MACHINES",
        MARGIN,
        PAGE_H - 82,
        "Inter-Medium",
        8,
        1.4,
        GOLD,
    )

    c.setFillColor(Color(0.78, 0.82, 0.86))
    c.setFont("Inter", 9)
    c.drawRightString(PAGE_W - MARGIN, PAGE_H - 50, "15 October 2026")
    c.setFont("Inter-Medium", 9)
    c.setFillColor(white)
    c.drawRightString(PAGE_W - MARGIN, PAGE_H - 68, "Q-CTP-20261015")
    c.setFillColor(Color(0.78, 0.82, 0.86))
    c.setFont("Inter", 8.5)
    c.drawRightString(PAGE_W - MARGIN, PAGE_H - 86, "Prices in US dollars")


def draw_intro(c):
    y = PAGE_H - 158
    c.setFillColor(INK)
    c.setFont("Inter-Semi", 16)
    c.drawString(MARGIN, y, "Three CTP machines")

    body = (
        "Each price is for one machine. The trade term is EXW, and freight "
        "to the Guangzhou warehouse is included."
    )
    lines = wrap_text(c, body, "Inter", 10, PAGE_W - MARGIN * 2)
    text_y = y - 22
    c.setFillColor(MUTED)
    c.setFont("Inter", 10)
    for line in lines:
        c.drawString(MARGIN, text_y, line)
        text_y -= 14
    return text_y - 12


def draw_terms(c, top):
    gap = 10
    card_w = (PAGE_W - MARGIN * 2 - gap) / 2
    card_h = 70
    y = top - card_h
    terms = [
        ("Trade term", ["EXW"]),
        ("Freight", ["Included", "Guangzhou warehouse"]),
    ]
    for index, (label, value_lines) in enumerate(terms):
        x = MARGIN + index * (card_w + gap)
        c.setFillColor(CARD)
        c.setStrokeColor(RULE)
        c.setLineWidth(0.8)
        c.roundRect(x, y, card_w, card_h, 6, fill=1, stroke=1)
        c.setFillColor(GOLD)
        c.rect(x, y + 10, 4, card_h - 20, fill=1, stroke=0)
        c.setFillColor(MUTED)
        c.setFont("Inter-Medium", 8)
        c.drawString(x + 18, y + card_h - 22, label.upper())
        c.setFillColor(NAVY)
        c.setFont("Inter-Semi", 12)
        value_y = y + card_h - 42
        for line in value_lines:
            c.drawString(x + 18, value_y, line)
            value_y -= 15
    return y - 24


def draw_table(c, top):
    x = MARGIN
    width = PAGE_W - MARGIN * 2
    header_h = 28
    row_h = 78
    table_h = header_h + row_h * len(MACHINES)
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

    c.setFillColor(white)
    c.setFont("Inter-Medium", 8)
    c.drawString(x + 18, top - 18, "MACHINE")
    c.drawRightString(x + width - 18, top - 18, "PRICE (USD)")

    cursor = top - header_h
    for index, machine in enumerate(MACHINES):
        cursor -= row_h
        if index % 2 == 1:
            c.setFillColor(ROW)
            c.rect(x, cursor, width, row_h, fill=1, stroke=0)

        c.setFillColor(GOLD_SOFT)
        c.circle(x + 28, cursor + 40, 12, fill=1, stroke=0)
        c.setFillColor(NAVY)
        c.setFont("Inter-Semi", 8)
        c.drawCentredString(x + 28, cursor + 37, f"{index + 1:02d}")

        c.setFillColor(GOLD)
        c.setFont("Inter-Medium", 8)
        c.drawString(x + 52, cursor + 50, machine["brand"])

        c.setFillColor(INK)
        c.setFont("Inter-Semi", 14)
        c.drawString(x + 52, cursor + 30, machine["model"])

        c.setFillColor(MUTED)
        c.setFont("Inter", 9)
        c.drawString(x + 52, cursor + 14, machine["detail"])

        c.setFillColor(NAVY)
        c.setFont("Inter-Bold", 16)
        c.drawRightString(x + width - 18, cursor + 32, money(machine["price"]))

    c.restoreState()
    c.setStrokeColor(RULE)
    c.setLineWidth(0.9)
    c.roundRect(x, bottom, width, table_h, radius, fill=0, stroke=1)
    return bottom


def draw_notes(c, top):
    notes = [
        "Currency is the United States dollar (USD).",
        "Trade term is EXW.",
        "Freight to the Guangzhou warehouse is included in each price.",
        "Each line is the price of one machine.",
    ]
    box_h = 112
    y = top - box_h
    c.setFillColor(CARD)
    c.setStrokeColor(RULE)
    c.setLineWidth(0.8)
    c.roundRect(MARGIN, y, PAGE_W - MARGIN * 2, box_h, 6, fill=1, stroke=1)
    c.setFillColor(GOLD)
    c.rect(MARGIN, y + 10, 4, box_h - 20, fill=1, stroke=0)

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
    c.drawString(MARGIN, 26, "Quotation Q-CTP-20261015  ·  CTP machines")
    c.drawRightString(PAGE_W - MARGIN, 26, "Page 1 of 1")


def build():
    c = canvas.Canvas(str(OUT), pagesize=A4)
    c.setTitle("CTP Machine Quotation Q-CTP-20261015")
    c.setAuthor("CTP quotation")
    c.setSubject("CRON UVP-4664GX+, AMSKY T864, and AMSKY U8128. EXW, freight to Guangzhou warehouse included.")
    c.setCreator("CTP machine quotation")

    c.setFillColor(PAPER)
    c.rect(0, 0, PAGE_W, PAGE_H, fill=1, stroke=0)

    draw_header(c)
    cursor = draw_intro(c)
    cursor = draw_terms(c, cursor)
    draw_tracked(c, "MACHINES", MARGIN, cursor - 2, "Inter-Semi", 8, 1.3, NAVY)
    cursor = draw_table(c, cursor - 18)
    draw_notes(c, cursor - 22)

    c.showPage()
    c.save()
    return OUT


if __name__ == "__main__":
    print(build())
