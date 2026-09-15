from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
IMAGES = ROOT / "resources" / "images"
FONTS = ROOT / "resources" / "fonts"
OUTPUT = ROOT / "store-assets"
WORK_SIZE = 288


def render_master():
    master = Image.new("RGB", (WORK_SIZE, WORK_SIZE), "#050708")
    mask = Image.open(IMAGES / "topo_v5_top_right.png").convert("L")
    mask = mask.crop((0, 0, 200, 200)).resize((WORK_SIZE, WORK_SIZE), Image.Resampling.NEAREST)
    master.paste(Image.new("RGB", master.size, "#AAB4B7"), mask=mask)

    draw = ImageDraw.Draw(master)
    time_font = ImageFont.truetype(FONTS / "Roboto-Light.ttf", 68)
    draw.text((266, 35), "10:09", font=time_font, fill="#FFFFFF", anchor="ra")

    clip = Image.new("L", master.size, 0)
    ImageDraw.Draw(clip).rounded_rectangle((0, 0, 287, 287), radius=48, fill=255)
    finished = Image.new("RGB", master.size, "#050708")
    finished.paste(master, mask=clip)
    return finished


def main():
    OUTPUT.mkdir(exist_ok=True)
    master = render_master()
    for size in (80, 144):
        icon = master.resize((size, size), Image.Resampling.LANCZOS)
        icon.save(OUTPUT / f"topographic-app-icon-{size}.png", optimize=True)


if __name__ == "__main__":
    main()
