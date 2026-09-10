from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
IMAGES = ROOT / "resources" / "images"
FONTS = ROOT / "resources" / "fonts"
OUTPUT = ROOT / "store-assets"

SCALE = 3
SIZE = (200, 228)

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
    return face.resize((SIZE[0] * SCALE, SIZE[1] * SCALE), Image.Resampling.NEAREST)


def main():
    OUTPUT.mkdir(exist_ok=True)
    keyframes = [render_face(*variant) for variant in VARIANTS]
    keyframes[0].save(OUTPUT / "topographic-thumbnail-still.png", optimize=True)

    for variant, frame in zip(VARIANTS, keyframes):
        frame.save(OUTPUT / f"topographic-{variant[0].replace('_', '-')}.png", optimize=True)

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
