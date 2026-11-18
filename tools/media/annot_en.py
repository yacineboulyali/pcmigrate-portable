import asyncio, os
from playwright.async_api import async_playwright
import annot as A

def panel_en(x, y, w, title, sub, items, tip=None):
    s = f'<div class="panel" style="left:{x}px;top:{y}px;width:{w}px;direction:ltr;text-align:left"><h1 style="font-family:Inter">{title}</h1><div class="sub">{sub}</div>'
    for n, d in items:
        s += f'<div class="item"><div class="n">{n}</div><div class="d">{d}</div></div>'
    if tip:
        s += f'<div style="margin-top:14px;padding:12px 16px;border-radius:10px;background:rgba(255,154,31,.14);border:1px solid rgba(255,154,31,.55);font:400 14px/1.6 Inter;color:#ffe2bd">{tip}</div>'
    return s + '</div>'

def backup():
    st = A.backup()
    st["panel"] = panel_en(900, 92, 680, "Step 1 — Back up the old PC",
        "Run the program on the old computer: from Hiren's BootCD PE (recommended) or from Windows itself.",
        [(1, "<b>Tabs</b> — <code>Backup</code> for the old PC, <code>Restore</code> for the new PC."),
         (2, "<b>Source Windows</b> — Windows drives are detected automatically, shown as <code>offline</code> (Hiren's) or running."),
         (3, "<b>Refresh</b> — scans the drives again after plugging a disk or unlocking BitLocker."),
         (4, "<b>Users</b> — tick the accounts you want to move."),
         (5, "<b>What to transfer</b> — files, browsers, Windows settings &amp; Wi-Fi, and the program list for winget."),
         (6, "<b>Programs with their data</b> — tick it, then <code>Choose...</code> the programs to move with files, registry and services."),
         (7, "<b>Destination</b> — an external disk or USB stick (NTFS or exFAT preferred). One is suggested automatically."),
         (8, "<b>Start Backup</b> — creates a <code>PCMigrate_date</code> folder with everything inside."),
         (9, "<b>Progress &amp; Cancel</b> — copied size and current file, with the option to stop."),
         (10, "<b>Log</b> — every step and error, also saved to <code>backup.log</code>.")]) + A.FOOT
    return st

def chooser_img():
    st = A.chooser_img()
    st["panel"] = panel_en(920, 92, 660, "Choosing programs to move with their data",
        "This window opens when you click <code style=\"font-family:Inter\">Choose...</code> in the Backup tab.",
        [(1, "<b>Installed programs</b> — tick every program you want to move."),
         (2, "<b>Size</b> — the approximate size registered in Windows."),
         (3, "<b>Install folder</b> — copied as-is to the new PC."),
         (4, "<b>Notes</b> — <code>MSI</code>: reinstall recommended. <code>data only</code>: only its data moves. <code>[user]</code>: installed for one user."),
         (5, "<b>Select all / Select none</b> — tick or untick everything."),
         (6, "<b>OK</b> — saves the choice; the count appears next to the option in the main window.")],
        tip="💡 <b>Best results:</b> regular desktop programs such as 7-Zip, Notepad++, VLC and business software. Antivirus, VPN, drivers and Office / Adobe licences usually need a reinstall.") + A.FOOT
    return st

def restore():
    st = A.restore()
    st["panel"] = panel_en(900, 86, 680, "Step 2 — Restore on the new PC",
        "Sign in with the new user account and run the program from the USB stick.",
        [(1, "<b>Migration package</b> — found automatically, or pick it with <code>Browse...</code> then <code>Load</code>."),
         (2, "<b>User in the package</b> — the old account whose data you restore."),
         (3, "<b>Target account</b> — the account on the new PC (default: you). Paths are fixed if the name differs."),
         (4, "<b>What to restore</b> — files, browsers, Windows settings &amp; Wi-Fi."),
         (5, "<b>Start Restore</b> — copies everything; newer files already on the PC are kept."),
         (6, "<b>Program list</b> — every program from the old PC."),
         (7, "<b>Method</b> — <code>Copy from package</code> moves it with files and data; <code>winget</code> downloads and installs it."),
         (8, "<b>Check recommended / Uncheck all</b> — select the suggested items or clear all."),
         (9, "<b>Prefer winget</b> — clean install, then the program's data is restored (safest)."),
         (10, "<b>Install / transfer checked</b> — runs your selection; restart the PC afterwards."),
         (11, "<b>Progress &amp; log</b> — saved to <code>restore.log</code> inside the package.")]) + A.FOOT
    return st

def workflow():
    st = A.workflow()
    card = lambda icon, n, t, lines: A.card(icon, n, t, lines).replace("right:28px", "left:28px").replace("padding-right:20px;text-align:right", "padding-left:20px;text-align:left")
    cards = (card(A.PC_OLD, 1, "Old PC", ["Boot the Hiren's USB (or run it in Windows)", "<b>Backup</b> tab: pick users and programs", "<b>Start Backup</b>"]) + A.ARROW.replace("<svg ", '<svg style="transform:scaleX(-1)" ') +
             card(A.DISK, 2, "Migration package", ["One folder on an external disk or USB", "Files, browsers, settings, Wi-Fi", "Programs with their files and data"]) + A.ARROW.replace("<svg ", '<svg style="transform:scaleX(-1)" ') +
             card(A.PC_NEW, 3, "New PC", ["<b>Restore</b> tab: <b>Start Restore</b>", "<b>Install / transfer checked</b> for programs", "Restart the PC ✓"]))
    st["panel"] = f'''<div class="panel" style="left:0;top:150px;width:1620px;text-align:center;direction:ltr">
       <h1 style="font-size:40px;font-family:Inter">How does PC Migrate Portable work?</h1>
       <div class="sub" style="font-size:18px">Move your files, programs and settings to a new PC in 3 steps — no install, no Internet</div>
       <div style="display:flex;direction:ltr;justify-content:center;align-items:center;gap:22px;margin-top:80px">{cards}</div></div>''' + A.FOOT
    return st

async def main():
    os.makedirs("out/img/en", exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(executable_path="/usr/bin/google-chrome", args=["--no-sandbox"])
        pg = await b.new_page(viewport={"width": 1620, "height": 900}, device_scale_factor=2)
        await pg.goto("file:///home/claude/media/mock.html"); await pg.wait_for_timeout(400)
        for name, fn in [("00-how-it-works", workflow), ("01-backup", backup), ("02-program-chooser", chooser_img), ("03-restore", restore)]:
            await pg.evaluate("s=>render(s)", fn()); await pg.wait_for_timeout(250)
            await pg.locator(".stage-annot").screenshot(path=f"out/img/en/{name}.png"); print("ok", name)
        await b.close()
asyncio.run(main())
