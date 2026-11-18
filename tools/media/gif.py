import asyncio, copy, math, os, subprocess, shutil, sys
from playwright.async_api import async_playwright
from data import *

FPS = 15
LANG = 'ar'
TEXTS = {'ar': {'b1': ('يكتشف البرنامج ويندوز القديم والمستخدمين تلقائيًا', "يعمل من Hiren's BootCD PE حتى لو كان ويندوز القديم لا يُقلع"), 'b2': ('اختر ما تريد نقله', 'الملفات • المتصفحات • إعدادات ويندوز و Wi-Fi • قائمة البرامج'), 'b3': ('انقل البرامج مع ملفاتها وبياناتها', 'فعّل الخيار ← تظهر قائمة البرامج المثبتة، فاختر منها'), 'b4': ('اضغط <bdi>Start Backup</bdi>', 'تُحفظ حزمة النقل كاملة على القرص الخارجي <bdi>E:\\</bdi>'), 'b5': ('حزمة النقل جاهزة', '<bdi>E:\\PCMigrate_20261118_1002</bdi> — وصّل القرص بالجهاز الجديد وافتح تبويب Restore'), 'r1': ('على الجهاز الجديد: افتح تبويب Restore', 'شغّل PCMigrate.exe من الفلاشة بحساب المستخدم الجديد'), 'r2': ('يعثر على حزمة النقل تلقائيًا', 'اختر المستخدم القديم والحساب على الجهاز الجديد — تُصحَّح المسارات إن اختلف الاسم'), 'r3': ('استعد الملفات والمتصفحات والإعدادات', '<bdi>Start Restore</bdi> — الملفات الأحدث على الجهاز الجديد لا تُستبدل'), 'r4': ('انقل البرامج أو ثبّتها بنقرة واحدة', 'Copy = من الحزمة مع بياناته  •  winget = تنزيل وتثبيت نظيف'), 'r5': ('تمّ! أعد تشغيل الجهاز', 'الملفات والمتصفحات والإعدادات والبرامج أصبحت على الجهاز الجديد')}, 'en': {'b1': ('Detects the old Windows and its users automatically', "Works from Hiren's BootCD PE — even if the old Windows no longer boots"), 'b2': ('Choose what to transfer', 'Files • Browsers • Windows settings &amp; Wi-Fi • Program list'), 'b3': ('Move programs with their files and data', 'Tick the option → choose programs from the installed list'), 'b4': ('Click Start Backup', 'The whole migration package is saved to the external disk E:\\'), 'b5': ('The migration package is ready', 'E:\\PCMigrate_20261118_1002 — plug the disk into the new PC and open the Restore tab'), 'r1': ('On the new PC: open the Restore tab', 'Run PCMigrate.exe from the USB stick, signed in as the new user'), 'r2': ('The migration package is found automatically', 'Pick the old user and the account on the new PC — paths are fixed if the name changed'), 'r3': ('Restore files, browsers and settings', 'Start Restore — newer files already on the new PC are kept'), 'r4': ('Transfer or install programs in one click', 'Copy = from the package with its data  •  winget = clean download &amp; install'), 'r5': ('Done! Restart the PC', 'Files, browsers, settings and programs are now on the new PC')}}
TOPR = {'ar': ('الجهاز القديم — Backup', 'الجهاز الجديد — Restore'), 'en': ('Old PC — Backup', 'New PC — Restore')}
TX = TEXTS['ar']
def C(x, y): return [39 + x, 83 + y]          # main window client -> page
def CH(x, y): return [29 + x, 163 + y]        # chooser client -> page
def MB(m, x, y):                              # message box client -> page
    w, h = m.get("w", 430), m.get("h", 170)
    return [39 + (820 - w) / 2 + x, 52 + (668 - h) / 2 + 31 + y]

class Movie:
    def __init__(self, st): self.st = st; self.frames = []; self.cur = [760, 760]; self.marks = [0]
    def mark(self): self.marks.append(len(self.frames))
    def snap(self, n=1, **extra):
        for _ in range(n):
            f = copy.deepcopy(self.st); f["cursor"] = list(self.cur); f.update(extra); self.frames.append(f)
    def hold(self, sec): self.snap(max(1, int(sec * FPS)))
    def move(self, to, sec=0.6):
        n = max(2, int(sec * FPS)); a = list(self.cur)
        for i in range(1, n + 1):
            t = i / n; e = t * t * (3 - 2 * t)
            self.cur = [a[0] + (to[0] - a[0]) * e, a[1] + (to[1] - a[1]) * e - math.sin(t * math.pi) * 25]
            self.snap()
        self.cur = list(to)
    def click(self, press_id=None, after=None):
        for k in range(4):
            extra = {"ripple": [self.cur[0], self.cur[1], 0.5 + k * 0.25, 1 - k * 0.2]}
            if press_id: extra["press"] = press_id
            if k == 2 and after: after(self.st)
            self.snap(**extra)
    def cap(self, n, t, s=""): self.st["caption"] = {"n": n, "t": t, "s": s}

def backup_movie():
    st = base_state(); st["topRight"] = TOPR[LANG][0]; st["capDir"] = "rtl" if LANG == "ar" else "ltr"
    m = Movie(st)
    m.cap(1, *TX["b1"])
    m.hold(1.2); m.move(C(300, 80), 0.8); m.hold(0.6); m.move(C(200, 152), 0.6); m.hold(1.0)
    m.cap(2, *TX["b2"])
    for y in (138, 166, 194, 222): m.move(C(431, y), 0.35); m.hold(0.25)
    m.hold(0.6)
    m.mark(); m.cap(3, *TX["b3"])
    m.move(C(431, 250), 0.5)
    def open_chooser(s): s["b"]["apps"] = True; s["chooser"] = chooser()
    m.click(after=open_chooser); m.hold(0.8)
    names = [a[0] for a in CHOOSER_APPS]
    for nm in ["7-Zip 24.08 (x64)", "Discord", "GestCom Pro 5.2", "Notepad++ (64-bit x64)", "VLC media player"]:
        i = names.index(nm)
        m.move(CH(23, 50 + 24 + 19 * i + 9), 0.45)
        def tick(s, i=i): s["chooser"]["apps"][i]["checked"] = True
        m.click(after=tick); m.hold(0.15)
    m.hold(0.5)
    m.move(CH(676, 475), 0.6); st["hot"] = "ok"; m.hold(0.2)
    def close_chooser(s): s["chooser"] = None; s["b"]["appsCount"] = 5; s["hot"] = None
    m.click("ok", after=close_chooser); m.hold(1.0)
    m.hold(0.4); m.mark(); m.cap(4, *TX["b4"])
    m.move(C(704, 355), 0.7); st["hot"] = "bstart"; m.hold(0.3)
    def start(s):
        s["hot"] = None; s["busy"] = True
        s["log"] += ["[10:02:40] ===== BACKUP started 2026-11-18 10:02:40 =====",
                     "[10:02:40] Source Windows: D: (offline)   Package: E:\\PCMigrate_20261118_1002"]
    m.click("bstart", after=start)
    m.move(C(560, 600), 0.5)
    pre = ["[10:02:41] Reading installed programs ...", "[10:02:42] User: Ahmed", "[10:02:42]   browser found: Chrome",
           "[10:02:43]   settings: 13 registry key(s) exported.", "[10:02:44]   program data 'Discord': 2 folder(s), 1 registry key(s)",
           "[10:02:45] User: Salma", "[10:02:46] Program: 7-Zip 24.08 (x64)  [C:\\Program Files\\7-Zip]",
           "[10:02:47] Program: GestCom Pro 5.2  [C:\\GestCom]", "[10:02:47]   service: GestComSync",
           "[10:02:48]   Wi-Fi: 4 profile(s) copied.", "[10:02:48] Programs: 64 found, list saved to programs.txt",
           "[10:02:49] Scanning 31 location(s) ...", "[10:02:52] Total to copy: 18.42 GB"]
    for l in pre: st["log"].append(l); st["status"] = "Scanning files ..."; m.snap(2)
    files = ["Desktop\\Rapport_2025.docx", "Documents\\Factures\\F-2025-118.pdf", "Pictures\\IMG_20250814_183344.jpg",
             "Videos\\Mariage_2024.mp4", "Browsers\\Chrome\\Default\\Bookmarks", "Apps\\GestCom Pro 5.2\\Files\\data\\base.mdb",
             "Apps\\VLC media player\\Files\\libvlccore.dll", "Downloads\\setup_files.zip"]
    srcs = ["-> D:\\Users\\Ahmed\\Desktop", "-> D:\\Users\\Ahmed\\Documents", "-> D:\\Users\\Ahmed\\Pictures", "-> D:\\Users\\Ahmed\\Videos",
            "-> D:\\Users\\Ahmed\\AppData\\Local\\Google\\Chrome\\User Data", "-> C:\\GestCom", "-> C:\\Program Files\\VideoLAN\\VLC", "-> D:\\Users\\Salma\\Downloads"]
    N = 75
    for k in range(N + 1):
        p = int(1000 * (k / N) ** 0.9); st["progress"] = p
        gb = 18.42 * p / 1000
        fi = min(len(files) - 1, k * len(files) // (N + 1))
        st["status"] = f"Copying {gb:.2f} GB / 18.42 GB  :  {files[fi].split(chr(92))[-1]}"
        if k % (N // len(srcs) + 1) == 0 and srcs: st["log"].append("[10:%02d:%02d] " % (3 + k // 6, (k * 7) % 60) + srcs.pop(0))
        m.snap()
    st["busy"] = False; st["status"] = "Finished. See the log below."
    st["log"] += ["[10:19:05] Copied 48211 file(s), 0 unchanged, 0 failed, 12 cloud-only (OneDrive placeholders) skipped.",
                  "[10:19:05] ===== BACKUP FINISHED 2026-11-18 10:19:05 =====",
                  "[10:19:05] Package ready: E:\\PCMigrate_20261118_1002"]
    m.cap("✓", *TX["b5"])
    m.hold(3.5)
    return m.frames, m.marks

def restore_movie():
    st = base_state(); st["topRight"] = TOPR[LANG][1]; st["capDir"] = "rtl" if LANG == "ar" else "ltr"
    st["log"] = RESTORE_LOG[:2]; st["status"] = "Ready."
    m = Movie(st)
    m.cap(1, *TX["r1"])
    m.hold(1.0); m.move(C(200, 13), 0.8)
    def tab(s): s["tab"] = 1; s["r"]["progs"] = restore_progs(); s["log"] = list(RESTORE_LOG)
    m.click(after=tab); m.hold(0.6)
    m.cap(2, *TX["r2"])
    m.move(C(260, 81), 0.6); m.hold(0.7); m.move(C(120, 136), 0.5); m.hold(0.5); m.move(C(420, 136), 0.5); m.hold(0.9)
    m.cap(3, *TX["r3"])
    m.move(C(704, 131), 0.6); st["hot"] = "rstart"; m.hold(0.3)
    mb = {"text": "Restore user 'Ahmed' into:\nC:\\Users\\Ahmed\n\nNewer files already on this PC are kept. Existing browser\nprofiles are renamed to .bak.\nContinue?", "w": 440, "h": 200}
    def ask(s): s["hot"] = None; s["msgbox"] = mb
    m.click("rstart", after=ask); m.hold(0.5)
    m.move(MB(mb, 440 - 142, 200 - 42 + 21), 0.6); st["hot"] = "yes"; m.hold(0.2)
    def go(s):
        s["msgbox"] = None; s["hot"] = None; s["busy"] = True
        s["log"] += ["[11:21:10] ===== RESTORE started 2026-11-18 11:21:10 =====",
                     "[11:21:10] Package: E:\\PCMigrate_20261118_1002   user: Ahmed  ->  C:\\Users\\Ahmed",
                     "[11:21:11]   browser: Chrome", "[11:21:11]   browser: Firefox", "[11:21:12] Total to copy: 11.06 GB"]
    m.click("yes", after=go); m.move(C(560, 600), 0.5)
    N = 45
    for k in range(N + 1):
        p = int(1000 * k / N); st["progress"] = p
        st["status"] = f"Copying {11.06 * p / 1000:.2f} GB / 11.06 GB  :  " + ["Rapport_2025.docx", "F-2025-118.pdf", "IMG_20250814_183344.jpg", "Bookmarks"][min(3, k * 4 // (N + 1))]
        m.snap()
    st["log"] += ["[11:33:40] Copied 30544 file(s), 0 unchanged, 0 failed, 0 cloud-only (OneDrive placeholders) skipped.",
                  "[11:33:40] Applying Windows settings ...", "[11:33:41]   registry: 13 / 13 key file(s) imported.",
                  "[11:33:41]   wallpaper restored.", "[11:33:42] Wi-Fi: 4 / 4 profile(s) imported.",
                  "[11:33:42] ===== RESTORE FINISHED 2026-11-18 11:33:42 ====="]
    st["busy"] = False; st["status"] = "Finished. See the log below."; m.hold(1.2)
    m.mark(); m.cap(4, *TX["r4"])
    m.move(C(200, 330), 0.6); m.hold(0.5); m.move(C(600, 300), 0.6); m.hold(0.6)
    m.move(C(704, 397), 0.6); st["hot"] = "install"; m.hold(0.3)
    mb2 = {"text": "8 program(s) selected:\n - 5 saved in the package (copied with their data)\n - 3 to download with winget (needs Internet)\n\nTarget profile: C:\\Users\\Ahmed\n\nContinue?", "w": 440, "h": 210}
    def ask2(s): s["hot"] = None; s["msgbox"] = mb2
    m.click("install", after=ask2); m.hold(0.6)
    m.move(MB(mb2, 440 - 142, 210 - 42 + 21), 0.6); st["hot"] = "yes"; m.hold(0.2)
    def go2(s): s["msgbox"] = None; s["hot"] = None; s["busy"] = True; s["progress"] = 0; s["log"].append("[11:34:02] ===== PROGRAMS started 2026-11-18 11:34:02 =====")
    m.click("yes", after=go2); m.move(C(560, 600), 0.5)
    steps = [("7-Zip 24.08 (x64)", ["Transferring 7-Zip 24.08 (x64)  (files + registry + data)", "  -> C:\\Program Files\\7-Zip", "  registry: 3 machine + 1 user key file(s) imported"]),
             ("Discord", ["Transferring Discord  (files + registry + data)", "  -> C:\\Users\\Ahmed\\AppData\\Local\\Discord", "  registry: 0 machine + 2 user key file(s) imported"]),
             ("GestCom Pro 5.2", ["Transferring GestCom Pro 5.2  (files + registry + data)", "  -> C:\\GestCom", "  registry: 5 machine + 1 user key file(s) imported"]),
             ("Notepad++ (64-bit x64)", ["Transferring Notepad++ (64-bit x64)  (files + registry + data)", "  registry: 2 machine + 1 user key file(s) imported"]),
             ("VLC media player", ["Transferring VLC media player  (files + registry + data)", "  registry: 3 machine + 0 user key file(s) imported"]),
             ("Google Chrome", ["winget: installing Google Chrome  (Google.Chrome) ...", "  OK"]),
             ("Mozilla Firefox (x64 fr)", ["winget: installing Mozilla Firefox (x64 fr)  (Mozilla.Firefox) ...", "  OK"]),
             ("Zoom Workplace", ["winget: installing Zoom Workplace  (Zoom.Zoom) ...", "  OK"])]
    t = 34 * 60 + 5
    for i, (nm, lines) in enumerate(steps):
        st["status"] = f"Program {i + 1}/8 : {nm}"
        for j, l in enumerate(lines):
            t += 9; st["log"].append("[11:%02d:%02d] " % (t // 60, t % 60) + l)
            st["progress"] = int(1000 * (i + (j + 1) / len(lines)) / 8); m.snap(3)
        for r in st["r"]["progs"]:
            if r["name"] == nm: r["sel"] = True
        m.snap(2)
    st["log"] += ["[11:37:51] ===== PROGRAMS finished: 8 / 8 OK =====", "[11:37:51] Services were registered: RESTART the PC so they start."]
    st["busy"] = False; st["progress"] = 1000; st["status"] = "Finished. See the log below."
    m.cap("✓", *TX["r5"])
    m.hold(3.5)
    return m.frames, m.marks

async def render_frames(d, frames):
    shutil.rmtree(d, ignore_errors=True); os.makedirs(d)
    async with async_playwright() as p:
        b = await p.chromium.launch(executable_path="/usr/bin/google-chrome", args=["--no-sandbox"])
        pg = await b.new_page(viewport={"width": 900, "height": 862}, device_scale_factor=1)
        await pg.goto("file:///home/claude/media/mock.html"); await pg.wait_for_timeout(400)
        for i, f in enumerate(frames):
            await pg.evaluate("s=>render(s)", f)
            await pg.screenshot(path=f"{d}/{i:05d}.png", clip={"x": 0, "y": 0, "width": 900, "height": 862})
        await b.close()

def make_gif(d, start, end, out, tail=1.0):
    seg = f"{d}_seg"; shutil.rmtree(seg, ignore_errors=True); os.makedirs(seg)
    idx = list(range(start, end)) + [end - 1] * int(tail * FPS)
    for k, i in enumerate(idx): shutil.copy(f"{d}/{i:05d}.png", f"{seg}/{k:05d}.png")
    pal = f"{seg}/pal.png"
    subprocess.run(["ffmpeg", "-v", "error", "-y", "-framerate", str(FPS), "-i", f"{seg}/%05d.png", "-vf", "palettegen=max_colors=192:stats_mode=diff", pal], check=True)
    subprocess.run(["ffmpeg", "-v", "error", "-y", "-framerate", str(FPS), "-i", f"{seg}/%05d.png", "-i", pal,
                    "-lavfi", "paletteuse=dither=bayer:bayer_scale=4:diff_mode=rectangle", out], check=True)
    subprocess.run(["gifsicle", "-O3", "--lossy=40", "-b", out], check=True)
    print(out, len(idx), "frames", os.path.getsize(out) // 1024, "KB")

def make_mp4(d, out):
    subprocess.run(["ffmpeg", "-v", "error", "-y", "-framerate", str(FPS), "-i", f"{d}/%05d.png", "-c:v", "libx264", "-preset", "veryfast",
                    "-pix_fmt", "yuv420p", "-crf", "22", out], check=True)

for LANG in (sys.argv[1:] or ["ar", "en"]):
    TX = TEXTS[LANG]
    od = f"out/gifs/{LANG}"; os.makedirs(od, exist_ok=True)
    fb, mb = backup_movie(); d = f"/tmp/fr_{LANG}_b"
    asyncio.run(render_frames(d, fb))
    make_gif(d, mb[0], mb[1], f"{od}/01-detect-and-select.gif")
    make_gif(d, mb[1], mb[2], f"{od}/02-choose-programs.gif")
    make_gif(d, mb[2], len(fb), f"{od}/03-start-backup.gif", tail=0.5)
    make_mp4(d, f"{od}/demo-backup.mp4")
    fr, mr = restore_movie(); d = f"/tmp/fr_{LANG}_r"
    asyncio.run(render_frames(d, fr))
    make_gif(d, mr[0], mr[1], f"{od}/04-restore-files.gif")
    make_gif(d, mr[1], len(fr), f"{od}/05-transfer-programs.gif", tail=0.5)
    make_mp4(d, f"{od}/demo-restore.mp4")
