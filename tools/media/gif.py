import asyncio, copy, math, os, subprocess, shutil, sys
from playwright.async_api import async_playwright
import data as D

FPS = 15
CAPS = {
 "ar": {"b1": ("يكتشف البرنامج ويندوز القديم والمستخدمين تلقائيًا", "يعمل من Hiren's BootCD PE حتى لو كان ويندوز القديم لا يُقلع"),
        "b2": ("اختر ما تريد نقله", "الملفات • المتصفحات • إعدادات ويندوز و Wi-Fi • قائمة البرامج"),
        "b3": ("انقل البرامج مع ملفاتها وبياناتها", "فعّل الخيار ← تظهر قائمة البرامج المثبتة، فاختر منها"),
        "b4": ("اضغط «بدء النسخ»", "تُحفظ حزمة النقل كاملة على القرص الخارجي <bdi>E:\\</bdi>"),
        "b5": ("حزمة النقل جاهزة", "<bdi>E:\\PCMigrate_20261118_1002</bdi> — وصّل القرص بالجهاز الجديد وافتح تبويب الاستعادة"),
        "r1": ("على الجهاز الجديد: افتح تبويب الاستعادة", "شغّل PCMigrate.exe من الفلاشة بحساب المستخدم الجديد"),
        "r2": ("يعثر على حزمة النقل تلقائيًا", "اختر المستخدم القديم والحساب على الجهاز الجديد — تُصحَّح المسارات إن اختلف الاسم"),
        "r3": ("استعد الملفات والمتصفحات والإعدادات", "«بدء الاستعادة» — الملفات الأحدث على الجهاز الجديد لا تُستبدل"),
        "r4": ("انقل البرامج أو ثبّتها بنقرة واحدة", "Copy = من الحزمة مع بياناته  •  winget = تنزيل وتثبيت نظيف"),
        "r5": ("تمّ! أعد تشغيل الجهاز", "الملفات والمتصفحات والإعدادات والبرامج أصبحت على الجهاز الجديد")},
 "en": {"b1": ("Detects the old Windows and its users automatically", "Works from Hiren's BootCD PE — even if the old Windows no longer boots"),
        "b2": ("Choose what to transfer", "Files • Browsers • Windows settings &amp; Wi-Fi • Program list"),
        "b3": ("Move programs with their files and data", "Tick the option → choose programs from the installed list"),
        "b4": ("Click Start Backup", "The whole migration package is saved to the external disk E:\\"),
        "b5": ("The migration package is ready", "E:\\PCMigrate_20261118_1002 — plug the disk into the new PC and open the Restore tab"),
        "r1": ("On the new PC: open the Restore tab", "Run PCMigrate.exe from the USB stick, signed in as the new user"),
        "r2": ("The migration package is found automatically", "Pick the old user and the account on the new PC — paths are fixed if the name changed"),
        "r3": ("Restore files, browsers and settings", "Start Restore — newer files already on the new PC are kept"),
        "r4": ("Transfer or install programs in one click", "Copy = from the package with its data  •  winget = clean download &amp; install"),
        "r5": ("Done! Restart the PC", "Files, browsers, settings and programs are now on the new PC")},
 "fr": {"b1": ("Détecte automatiquement l'ancien Windows et ses utilisateurs", "Fonctionne depuis Hiren's BootCD PE — même si l'ancien Windows ne démarre plus"),
        "b2": ("Choisissez ce qu'il faut transférer", "Fichiers • Navigateurs • Paramètres Windows et Wi-Fi • Liste des programmes"),
        "b3": ("Déplacez les programmes avec leurs fichiers et données", "Cochez l'option → choisissez dans la liste des programmes installés"),
        "b4": ("Cliquez sur « Lancer la sauvegarde »", "Le paquet de migration complet est enregistré sur le disque externe E:\\"),
        "b5": ("Le paquet de migration est prêt", "E:\\PCMigrate_20261118_1002 — branchez le disque sur le nouveau PC, onglet Restauration"),
        "r1": ("Sur le nouveau PC : ouvrez l'onglet Restauration", "Lancez PCMigrate.exe depuis la clé USB, connecté avec le nouvel utilisateur"),
        "r2": ("Le paquet de migration est trouvé automatiquement", "Choisissez l'ancien utilisateur et le compte du nouveau PC — chemins corrigés si le nom change"),
        "r3": ("Restaurez fichiers, navigateurs et paramètres", "« Lancer la restauration » — les fichiers plus récents du nouveau PC sont conservés"),
        "r4": ("Transférez ou installez les programmes en un clic", "Copy = depuis le paquet avec ses données  •  winget = téléchargement et installation propres"),
        "r5": ("Terminé ! Redémarrez le PC", "Fichiers, navigateurs, paramètres et programmes sont sur le nouveau PC")},
 "es": {"b1": ("Detecta automáticamente el Windows antiguo y sus usuarios", "Funciona desde Hiren's BootCD PE — aunque el Windows antiguo ya no arranque"),
        "b2": ("Elige qué transferir", "Archivos • Navegadores • Ajustes de Windows y Wi-Fi • Lista de programas"),
        "b3": ("Mueve los programas con sus archivos y datos", "Marca la opción → elige en la lista de programas instalados"),
        "b4": ("Pulsa «Iniciar copia»", "El paquete de migración completo se guarda en el disco externo E:\\"),
        "b5": ("El paquete de migración está listo", "E:\\PCMigrate_20261118_1002 — conecta el disco al PC nuevo y abre la pestaña Restaurar"),
        "r1": ("En el PC nuevo: abre la pestaña Restaurar", "Ejecuta PCMigrate.exe desde el USB con la sesión del nuevo usuario"),
        "r2": ("El paquete de migración se encuentra automáticamente", "Elige el usuario antiguo y la cuenta del PC nuevo — las rutas se corrigen si cambia el nombre"),
        "r3": ("Restaura archivos, navegadores y ajustes", "«Iniciar restauración» — se conservan los archivos más recientes del PC nuevo"),
        "r4": ("Transfiere o instala los programas con un clic", "Copy = desde el paquete con sus datos  •  winget = descarga e instalación limpias"),
        "r5": ("¡Listo! Reinicia el PC", "Archivos, navegadores, ajustes y programas ya están en el PC nuevo")},
}
TOPR = {"ar": ("الجهاز القديم", "الجهاز الجديد"), "en": ("Old PC — Backup", "New PC — Restore"),
        "fr": ("Ancien PC — Sauvegarde", "Nouveau PC — Restauration"), "es": ("PC antiguo — Copia", "PC nuevo — Restaurar")}
LANG = "en"; TX = CAPS["en"]; RTL = False

def C(x, y): return [39 + (820 - x if RTL else x), 83 + y]
def CH(x, y): return [29 + (840 - x if RTL else x), 163 + y]
def MB(m, x, y):
    w, h = m.get("w", 430), m.get("h", 170)
    return [39 + (820 - w) / 2 + (w - x if RTL else x), 52 + (668 - h) / 2 + 31 + y]

class Movie:
    def __init__(self, st): self.st = st; self.frames = []; self.cur = [760 if not RTL else 140, 760]; self.marks = [0]
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
    def cap(self, n, key): t, s = TX[key]; self.st["caption"] = {"n": n, "t": t, "s": s}

def new_state(which):
    st = D.base_state(); st["topRight"] = TOPR[LANG][which]; st["capDir"] = "rtl" if RTL else "ltr"
    return st

def backup_movie():
    st = new_state(0); m = Movie(st); tr = D.tr; L = D.L
    m.cap(1, "b1"); m.hold(1.2); m.move(C(300, 80), 0.8); m.hold(0.6); m.move(C(200, 152), 0.6); m.hold(1.0)
    m.cap(2, "b2")
    for y in (138, 166, 194, 222): m.move(C(431, y), 0.35); m.hold(0.25)
    m.hold(0.6)
    m.mark(); m.cap(3, "b3"); m.move(C(431, 250), 0.5)
    def open_chooser(s): s["b"]["apps"] = True; s["chooser"] = D.chooser()
    m.click(after=open_chooser); m.hold(0.8)
    names = [a[0] for a in D.CHOOSER_APPS]
    for nm in D.PICK:
        i = names.index(nm)
        m.move(CH(23, 50 + 24 + 19 * i + 9), 0.45)
        def tick(s, i=i): s["chooser"]["apps"][i]["checked"] = True
        m.click(after=tick); m.hold(0.15)
    m.hold(0.5); m.move(CH(676, 475), 0.6); st["hot"] = "ok"; m.hold(0.2)
    def close_chooser(s): s["chooser"] = None; s["b"]["appsCount"] = 5; s["hot"] = None
    m.click("ok", after=close_chooser); m.hold(1.0)
    m.hold(0.4); m.mark(); m.cap(4, "b4")
    m.move(C(704, 355), 0.7); st["hot"] = "bstart"; m.hold(0.3)
    def start(s):
        s["hot"] = None; s["busy"] = True
        s["log"] += [L("10:02:40", "S_L_B_STARTED", "2026-11-18 10:02:40"),
                     L("10:02:40", "S_L_SOURCE", "D:", tr("S_OFFLINE"), "E:\\PCMigrate_20261118_1002")]
    m.click("bstart", after=start); m.move(C(560, 600), 0.5)
    pre = [L("10:02:41", "S_ST_READPROGS"), L("10:02:42", "S_L_USER", "Ahmed"), L("10:02:42", "S_L_BROWSER_FOUND", "Chrome"),
           L("10:02:43", "S_L_SETTINGS_EXP", 13), L("10:02:44", "S_L_APPDATA", "Discord", 2, 1), L("10:02:45", "S_L_USER", "Salma"),
           L("10:02:46", "S_L_PROGRAM", "7-Zip 24.08 (x64)", "  [C:\\Program Files\\7-Zip]"),
           L("10:02:47", "S_L_PROGRAM", "GestCom Pro 5.2", "  [C:\\GestCom]"), L("10:02:47", "S_L_SERVICE", "GestComSync"),
           L("10:02:48", "S_L_WIFI_COPY", 4), L("10:02:48", "S_L_PROGLIST", 64), L("10:02:49", "S_L_SCANNING", 31),
           L("10:02:52", "S_L_TOTAL", "18.42 GB")]
    for l in pre: st["log"].append(l); st["status"] = tr("S_ST_SCANNING"); m.snap(2)
    files = ["Rapport_2025.docx", "F-2025-118.pdf", "IMG_20250814_183344.jpg", "Mariage_2024.mp4", "Bookmarks", "base.mdb", "libvlccore.dll", "setup_files.zip"]
    srcs = ["-> D:\\Users\\Ahmed\\Desktop", "-> D:\\Users\\Ahmed\\Documents", "-> D:\\Users\\Ahmed\\Pictures", "-> D:\\Users\\Ahmed\\Videos",
            "-> D:\\Users\\Ahmed\\AppData\\Local\\Google\\Chrome\\User Data", "-> C:\\GestCom", "-> C:\\Program Files\\VideoLAN\\VLC", "-> D:\\Users\\Salma\\Downloads"]
    N = 75
    for k in range(N + 1):
        p = int(1000 * (k / N) ** 0.9); st["progress"] = p
        fi = min(len(files) - 1, k * len(files) // (N + 1))
        st["status"] = tr("S_ST_COPYING", "%.2f GB" % (18.42 * p / 1000), "18.42 GB", files[fi])
        if k % (N // len(srcs) + 1) == 0 and srcs: st["log"].append("[10:%02d:%02d] " % (3 + k // 6, (k * 7) % 60) + srcs.pop(0))
        m.snap()
    st["busy"] = False; st["status"] = tr("S_ST_FINISHED")
    st["log"] += [L("10:19:05", "S_L_COPIED", 48211, 0, 0, 12), L("10:19:05", "S_L_B_DONE", "2026-11-18 10:19:05"),
                  L("10:19:05", "S_L_PKG_READY", "E:\\PCMigrate_20261118_1002"), L("10:19:05", "S_L_NEXT")]
    m.cap("✓", "b5"); m.hold(3.5)
    return m.frames, m.marks

def restore_movie():
    st = new_state(1); tr = D.tr; L = D.L
    st["log"] = D.restore_log()[:2]
    m = Movie(st)
    m.cap(1, "r1"); m.hold(1.0); m.move(C(200, 13), 0.8)
    def tab(s): s["tab"] = 1; s["r"]["progs"] = D.restore_progs(); s["log"] = D.restore_log()
    m.click(after=tab); m.hold(0.6)
    m.cap(2, "r2"); m.move(C(260, 81), 0.6); m.hold(0.7); m.move(C(120, 136), 0.5); m.hold(0.5); m.move(C(420, 136), 0.5); m.hold(0.9)
    m.cap(3, "r3"); m.move(C(704, 131), 0.6); st["hot"] = "rstart"; m.hold(0.3)
    mb = {"text": tr("S_Q_RESTORE", "Ahmed", "C:\\Users\\Ahmed"), "w": 470, "h": 210}
    def ask(s): s["hot"] = None; s["msgbox"] = mb
    m.click("rstart", after=ask); m.hold(0.5)
    m.move(MB(mb, 470 - 142, 210 - 42 + 21), 0.6); st["hot"] = "yes"; m.hold(0.2)
    def go(s):
        s["msgbox"] = None; s["hot"] = None; s["busy"] = True
        s["log"] += [L("11:21:10", "S_L_R_STARTED", "2026-11-18 11:21:10"), L("11:21:10", "S_L_R_PKG", "E:\\PCMigrate_20261118_1002", "Ahmed", "C:\\Users\\Ahmed"),
                     L("11:21:11", "S_L_BROWSER", "Chrome"), L("11:21:11", "S_L_BROWSER", "Firefox"), L("11:21:12", "S_L_TOTAL", "11.06 GB")]
    m.click("yes", after=go); m.move(C(560, 600), 0.5)
    N = 45
    for k in range(N + 1):
        p = int(1000 * k / N); st["progress"] = p
        st["status"] = tr("S_ST_COPYING", "%.2f GB" % (11.06 * p / 1000), "11.06 GB", ["Rapport_2025.docx", "F-2025-118.pdf", "IMG_20250814_183344.jpg", "Bookmarks"][min(3, k * 4 // (N + 1))])
        m.snap()
    st["log"] += [L("11:33:40", "S_L_COPIED", 30544, 0, 0, 0), L("11:33:40", "S_L_APPLY"), L("11:33:41", "S_L_REGIMP", 13, 13),
                  L("11:33:41", "S_L_WALL"), L("11:33:42", "S_L_WIFI_IMP", 4, 4), L("11:33:42", "S_L_R_DONE", "2026-11-18 11:33:42")]
    st["busy"] = False; st["status"] = tr("S_ST_FINISHED"); m.hold(1.2)
    m.mark(); m.cap(4, "r4")
    m.move(C(200, 330), 0.6); m.hold(0.5); m.move(C(600, 300), 0.6); m.hold(0.6)
    m.move(C(704, 397), 0.6); st["hot"] = "install"; m.hold(0.3)
    mb2 = {"text": tr("S_Q_INSTALL", 8, 5, "", 3, "C:\\Users\\Ahmed"), "w": 470, "h": 220}
    def ask2(s): s["hot"] = None; s["msgbox"] = mb2
    m.click("install", after=ask2); m.hold(0.6)
    m.move(MB(mb2, 470 - 142, 220 - 42 + 21), 0.6); st["hot"] = "yes"; m.hold(0.2)
    def go2(s): s["msgbox"] = None; s["hot"] = None; s["busy"] = True; s["progress"] = 0; s["log"].append(L("11:34:02", "S_L_P_STARTED", "2026-11-18 11:34:02"))
    m.click("yes", after=go2); m.move(C(560, 600), 0.5)
    full, data_only = tr("S_L_FULL"), tr("S_L_DATAONLY2")
    steps = [("7-Zip 24.08 (x64)", [("S_L_TRANSFER", "7-Zip 24.08 (x64)", full), None, ("S_L_REGSUM", 3, 1, "")], "  -> C:\\Program Files\\7-Zip"),
             ("Discord", [("S_L_TRANSFER", "Discord", full), None, ("S_L_REGSUM", 0, 2, "")], "  -> C:\\Users\\Ahmed\\AppData\\Local\\Discord"),
             ("GestCom Pro 5.2", [("S_L_TRANSFER", "GestCom Pro 5.2", full), None, ("S_L_REGSUM", 5, 1, "")], "  -> C:\\GestCom"),
             ("Notepad++ (64-bit x64)", [("S_L_TRANSFER", "Notepad++ (64-bit x64)", full), ("S_L_REGSUM", 2, 1, "")], None),
             ("VLC media player", [("S_L_TRANSFER", "VLC media player", full), ("S_L_REGSUM", 3, 0, "")], None),
             ("Google Chrome", [("S_L_WG_INSTALL", "Google Chrome", "Google.Chrome"), ("S_L_OK",)], None),
             ("Mozilla Firefox (x64 fr)", [("S_L_WG_INSTALL", "Mozilla Firefox (x64 fr)", "Mozilla.Firefox"), ("S_L_OK",)], None),
             ("Zoom Workplace", [("S_L_WG_INSTALL", "Zoom Workplace", "Zoom.Zoom"), ("S_L_OK",)], None)]
    t = 34 * 60 + 5
    for i, (nm, lines, arrow) in enumerate(steps):
        st["status"] = tr("S_ST_PROGRAM", i + 1, 8, nm)
        for j, l in enumerate(lines):
            t += 9; ts = "11:%02d:%02d" % (t // 60, t % 60)
            st["log"].append(f"[{ts}] " + arrow if l is None else L(ts, *l))
            st["progress"] = int(1000 * (i + (j + 1) / len(lines)) / 8); m.snap(3)
        for r in st["r"]["progs"]:
            if r["name"] == nm: r["sel"] = True
        m.snap(2)
    st["log"] += [L("11:37:51", "S_L_P_DONE", 8, 8), L("11:37:51", "S_L_RESTART")]
    st["busy"] = False; st["progress"] = 1000; st["status"] = tr("S_ST_FINISHED")
    m.cap("✓", "r5"); m.hold(3.5)
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

if __name__ == "__main__":
    for LANG in (sys.argv[1:] or ["en", "fr", "ar", "es"]):
        D.LANG = LANG; TX = CAPS[LANG]; RTL = LANG == "ar"
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
