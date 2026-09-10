from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
IMAGES = ROOT / "resources" / "images"
FONTS = ROOT / "resources" / "fonts"
OUTPUT = ROOT / "store-assets"

SIZE = (200, 228)
BANNER_SIZE = (720, 320)

VARIANTS = [
    ("top_right", "#000000", "#AAAAAA", "#FFFFFF"),
    ("top_left", "#000055", "#55FFFF", "#FFFFFF"),
    ("bottom_right", "#550000", "#FFAA55", "#FFFFFF"),
    ("bottom_left", "#FFFFAA", "#005555", "#000000"),
]


def render_face(orientation, background, contours, text):
    mask = Image.open(IMAGES / f"topo_v5_{orientation}.png").convert("L")
    face = Image.new("RGB", SIZE, background)
    line_layer = Image.new("RGB", SIZE, contours)
    face.paste(line_layer, mask=mask)

    left = orientation.endswith("left")
    bottom = orientation.startswith("bottom")
    draw = ImageDraw.Draw(face)
    date_font = ImageFont.truetype(FONTS / "Roboto-Light.ttf", 21)
    time_font = ImageFont.truetype(FONTS / "Roboto-Light.ttf", 46)
    date_y = 10 if not bottom else 132
    time_y = 31 if not bottom else 153
    anchor = "la" if left else "ra"
    x = 9 if left else 188

    draw.text((x, date_y), "THU 10", font=date_font, fill=text, anchor=anchor)
    draw.text((x, time_y), "10:09", font=time_font, fill=text, anchor=anchor)
    return face


def render_banner(keyframes):
    banner = Image.new("RGB", BANNER_SIZE, "#080A0B")

    # Carry the watchface's contour language into the banner background.
    mask = Image.open(IMAGES / "topo_v5_top_right.png").convert("L")
    mask = mask.resize((720, 821), Image.Resampling.NEAREST).crop((0, 250, 720, 570))
    banner.paste(Image.new("RGB", BANNER_SIZE, "#202426"), mask=mask)

    draw = ImageDraw.Draw(banner)
    draw.rounded_rectangle((22, 67, 326, 241), radius=16, fill="#080A0B")
    title_font = ImageFont.truetype(FONTS / "Roboto-Light.ttf", 41)
    subtitle_font = ImageFont.truetype(FONTS / "Roboto-Light.ttf", 19)
    detail_font = ImageFont.truetype(FONTS / "Roboto-Bold.ttf", 13)

    draw.text((38, 91), "TOPOGRAPHIC", font=title_font, fill="#FFFFFF")
    draw.text((40, 146), "Time meets terrain.", font=subtitle_font, fill="#AAAAAA")
    draw.text((40, 202), "4 CORNERS  •  CUSTOM COLORS", font=detail_font, fill="#55FFFF")

    for x, frame in zip((356, 475, 594), keyframes[:3]):
        preview = frame.resize((108, 123), Image.Resampling.NEAREST)
        shadow = Image.new("RGBA", (116, 131), (0, 0, 0, 0))
        shadow_draw = ImageDraw.Draw(shadow)
        shadow_draw.rounded_rectangle((4, 4, 115, 130), radius=10, fill=(0, 0, 0, 150))
        banner.paste(shadow, (x - 4, 103), shadow)
        draw.rounded_rectangle((x - 2, 105, x + 110, 232), radius=9,
                               fill="#111111", outline="#555555", width=2)
        banner.paste(preview, (x, 107))

    return banner


def main():
    OUTPUT.mkdir(exist_ok=True)
    keyframes = [render_face(*variant) for variant in VARIANTS]
    keyframes[0].save(OUTPUT / "topographic-thumbnail-still.png", optimize=True)

    for variant, frame in zip(VARIANTS, keyframes):
        frame.save(OUTPUT / f"topographic-{variant[0].replace('_', '-')}.png", optimize=True)

    sheet = Image.new("RGB", SIZE)
    for index, frame in enumerate(keyframes):
        tile = frame.resize((100, 114), Image.Resampling.NEAREST)
        sheet.paste(tile, ((index % 2) * 100, (index // 2) * 114))
    sheet.save(OUTPUT / "topographic-feature-sheet.png", optimize=True)

    render_banner(keyframes).save(OUTPUT / "topographic-store-banner.png", optimize=True)

    keyframes[0].save(
        OUTPUT / "topographic-features.gif",
        save_all=True,
        append_images=keyframes[1:],
        duration=[1200] * len(keyframes),
        loop=0,
        optimize=True,
        disposal=2,
    )


if __name__ == "__main__":
    main()
