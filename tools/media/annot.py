import asyncio, os
from playwright.async_api import async_playwright
from data import *

def panel(x, y, w, title, sub, items, tip=None):
    s = f'<div class="panel" style="left:{x}px;top:{y}px;width:{w}px"><h1>{title}</h1><div class="sub">{sub}</div>'
    for n, d in items:
        s += f'<div class="item"><div class="n">{n}</div><div class="d">{d}</div></div>'
    if tip:
        s += f'<div style="margin-top:14px;padding:12px 16px;border-radius:10px;background:rgba(255,154,31,.14);border:1px solid rgba(255,154,31,.55);font:400 14px/1.6 Inter,\'Noto Sans Arabic\';color:#ffe2bd">{tip}</div>'
    return s + '</div>'

FOOT = '<div class="foot" style="left:40px;bottom:26px">github.com/yacineboulyali/pcmigrate-portable</div>'

def backup():
    st = base_state(); st["mode"] = "annot"; st["kicker"] = "Backup · old PC"
    st["b"]["apps"] = True; st["b"]["appsCount"] = 5
    st["log"] += ["[10:02:40] ===== BACKUP started 2026-11-18 10:02:40 =====",
                  "[10:02:40] Source Windows: D: (offline)   Package: E:\\PCMigrate_20261118_1002",
                  "[10:02:41] Reading installed programs ...", "[10:02:42] User: Ahmed",
                  "[10:02:42]   browser found: Chrome", "[10:02:43]   settings: 13 registry key(s) exported.",
                  "[10:02:49] Total to copy: 18.42 GB"]
    st["busy"] = False; st["progress"] = 420; st["status"] = "Copying 7.74 GB / 18.42 GB  :  IMG_20250814_183344.jpg"
    st["badges"] = [[1, 286, 13], [2, 8, 80], [3, 796, 67], [4, 8, 150], [5, 545, 100], [6, 803, 250], [7, 8, 355], [8, 796, 336], [9, -14, 443], [10, -14, 560]]
    st["hl"] = [[420, 236, 368, 28]]
    st["panel"] = panel(900, 92, 680, "الخطوة 1 — النسخ من الجهاز القديم",
        "شغّل البرنامج على الحاسوب القديم: من Hiren's BootCD PE (يُنصح به) أو من ويندوز نفسه.",
        [(1, "<b>التبويبات</b> — <code>Backup</code> للجهاز القديم، و<code>Restore</code> للجهاز الجديد."),
         (2, "<b>ويندوز المصدر</b> — يكتشف أقراص ويندوز تلقائيًا ويبيّن إن كان النظام <code>offline</code> (من Hiren's) أو يعمل الآن."),
         (3, "<b>Refresh</b> — يعيد البحث عن الأقراص بعد توصيل قرص أو فتح BitLocker."),
         (4, "<b>المستخدمون</b> — حدّد الحسابات التي تريد نقلها."),
         (5, "<b>ما الذي يُنقل</b> — الملفات، المتصفحات، إعدادات ويندوز و Wi-Fi، وقائمة البرامج لإعادة تثبيتها بـ winget."),
         (6, "<b>البرامج مع بياناتها</b> — فعّله ثم اضغط <code>Choose...</code> لتختار البرامج التي تُنقل بملفاتها وسجلّها وخدماتها."),
         (7, "<b>مكان الحفظ</b> — قرص خارجي أو فلاشة (يُفضَّل NTFS أو exFAT). يقترح البرنامج واحدًا تلقائيًا."),
         (8, "<b>Start Backup</b> — ينشئ مجلد <code>PCMigrate_التاريخ</code> فيه كل شيء."),
         (9, "<b>التقدّم و Cancel</b> — الحجم المنسوخ والملف الحالي، مع إمكانية الإيقاف."),
         (10, "<b>السجل</b> — كل خطوة وكل خطأ، ويُحفظ أيضًا في <code>backup.log</code>.")]) + FOOT
    return st

def chooser_img():
    st = base_state(); st["mode"] = "annot"; st["kicker"] = "Program chooser"
    st["hideMain"] = True; st["chooser"] = chooser(PICK); st["chooserPos"] = [40, 150]
    st["chooser"]["apps"][7]["sel"] = True
    st["badgeBase"] = [40, 181]
    st["badges"] = [[1, -14, 100], [2, 432, 62], [3, 600, 62], [4, 660, 237], [5, -14, 475], [6, 724, 460]]
    st["panel"] = panel(920, 92, 660, "اختيار البرامج المنقولة مع بياناتها",
        "تظهر هذه النافذة عند الضغط على <code style=\"font-family:Inter\">Choose...</code> في تبويب Backup.",
        [(1, "<b>البرامج المثبتة</b> — ضع علامة على كل برنامج تريد نقله."),
         (2, "<b>الحجم</b> — الحجم التقريبي المسجّل في ويندوز."),
         (3, "<b>مكان التثبيت</b> — المجلد الذي يُنسخ كما هو إلى الجهاز الجديد."),
         (4, "<b>ملاحظات</b> — <code>MSI</code>: يُفضَّل إعادة تثبيته. <code>data only</code>: تُنقل بياناته فقط. <code>[user]</code>: مثبّت لمستخدم واحد."),
         (5, "<b>Select all / Select none</b> — تحديد الكل أو إلغاء التحديد."),
         (6, "<b>OK</b> — يحفظ الاختيار، ويظهر العدد بجانب الخيار في النافذة الرئيسية.")],
        tip="💡 <b style=\"font-family:Inter,'Noto Kufi Arabic'\">أفضل النتائج:</b> البرامج العادية مثل <bdi>7-Zip</bdi> و <bdi>Notepad++</bdi> و <bdi>VLC</bdi> وبرامج التسيير. برامج الحماية و VPN والتعريفات وتراخيص Office و Adobe تحتاج غالبًا إعادة تثبيت.") + FOOT
    return st

def restore():
    st = base_state(); st["mode"] = "annot"; st["kicker"] = "Restore · new PC"; st["tab"] = 1
    st["r"]["progs"] = restore_progs(); st["r"]["progs"][0]["sel"] = True
    st["log"] = RESTORE_LOG + ["[11:21:10] ===== RESTORE started 2026-11-18 11:21:10 =====",
                               "[11:21:10] Package: E:\\PCMigrate_20261118_1002   user: Ahmed  ->  C:\\Users\\Ahmed",
                               "[11:21:11]   browser: Chrome", "[11:21:12] Total to copy: 11.06 GB"]
    st["progress"] = 610; st["status"] = "Copying 6.75 GB / 11.06 GB  :  Rapport_annuel_2025.docx"
    st["badges"] = [[1, 8, 81], [2, 8, 136], [3, 392, 111], [4, 8, 172], [5, 796, 112], [6, 8, 240], [7, 620, 228], [8, 8, 397], [9, 556, 397], [10, 796, 382], [11, -14, 560]]
    st["panel"] = panel(900, 86, 680, "الخطوة 2 — الاستعادة على الجهاز الجديد",
        "سجّل الدخول بحساب المستخدم الجديد، وشغّل البرنامج من الفلاشة.",
        [(1, "<b>حزمة النقل</b> — يعثر عليها تلقائيًا، أو اخترها بـ <code>Browse...</code> ثم <code>Load</code>."),
         (2, "<b>المستخدم في الحزمة</b> — الحساب القديم الذي تستعيد بياناته."),
         (3, "<b>الحساب الهدف</b> — الحساب على الجهاز الجديد (افتراضيًا: أنت). تُصحَّح المسارات إن اختلف الاسم."),
         (4, "<b>ما الذي يُستعاد</b> — الملفات، المتصفحات، إعدادات ويندوز و Wi-Fi."),
         (5, "<b>Start Restore</b> — ينسخ كل شيء، ولا يستبدل الملفات الأحدث الموجودة."),
         (6, "<b>قائمة البرامج</b> — كل برامج الجهاز القديم."),
         (7, "<b>عمود الطريقة</b> — <code>Copy from package</code> يعني النقل من الحزمة بملفاته وبياناته، و<code>winget</code> يعني التنزيل والتثبيت من الإنترنت."),
         (8, "<b>Check recommended / Uncheck all</b> — تحديد المقترح أو إلغاء الكل."),
         (9, "<b>Prefer winget</b> — تثبيت نظيف ثم استرجاع بيانات البرنامج (الأكثر أمانًا)."),
         (10, "<b>Install / transfer checked</b> — ينفّذ ما حدّدته، ثم أعد تشغيل الجهاز."),
         (11, "<b>التقدّم والسجل</b> — يُحفظ في <code>restore.log</code> داخل الحزمة.")]) + FOOT
    return st

def card(icon, n, title, lines):
    li = "".join(f'<li>{l}</li>' for l in lines)
    return f'''<div style="width:440px;background:rgba(255,255,255,.07);border:1px solid rgba(255,255,255,.18);border-radius:18px;padding:30px 30px 26px;position:relative">
      <div style="position:absolute;top:-22px;right:28px;width:44px;height:44px;border-radius:50%;background:#ff9a1f;color:#1b1b1b;font:800 22px Inter,'Noto Kufi Arabic';display:flex;align-items:center;justify-content:center;box-shadow:0 0 0 6px rgba(255,154,31,.25)">{n}</div>
      <div style="height:150px;display:flex;align-items:center;justify-content:center">{icon}</div>
      <div style="font:700 23px/1.4 Inter,'Noto Kufi Arabic';color:#fff;margin:6px 0 10px;text-align:center">{title}</div>
      <ul style="font:400 15.5px/1.75 Inter,'Noto Sans Arabic';color:#d6e6f6;padding-right:20px;text-align:right">{li}</ul></div>'''

PC_OLD = '''<svg width="190" height="140" viewBox="0 0 190 140" style="direction:ltr"><rect x="15" y="10" width="160" height="100" rx="8" fill="#5a6473"/><rect x="25" y="20" width="140" height="80" fill="#1e293b"/><text x="95" y="56" fill="#93c5fd" font-family="Inter" font-size="13" font-weight="700" text-anchor="middle">Hiren's BootCD PE</text><text x="95" y="76" fill="#cbd5e1" font-family="Inter" font-size="11" text-anchor="middle">PCMigrate.exe</text><rect x="80" y="110" width="30" height="14" fill="#5a6473"/><rect x="55" y="124" width="80" height="8" rx="3" fill="#5a6473"/><rect x="160" y="96" width="26" height="12" rx="2" fill="#ff9a1f"/><rect x="152" y="98" width="10" height="8" fill="#e2e8f0"/></svg>'''
DISK = '''<svg width="190" height="140" viewBox="0 0 190 140" style="direction:ltr"><rect x="40" y="8" width="110" height="124" rx="14" fill="#334155" stroke="#94a3b8" stroke-width="2"/><circle cx="95" cy="108" r="6" fill="#22c55e"/><text x="95" y="38" fill="#fff" font-family="Inter" font-size="11" font-weight="700" text-anchor="middle">PCMigrate_</text><text x="95" y="52" fill="#fff" font-family="Inter" font-size="11" font-weight="700" text-anchor="middle">20261118</text><g font-family="Inter" font-size="9.5" fill="#cbd5e1"><text x="56" y="70">Users\\</text><text x="56" y="82">Apps\\</text><text x="56" y="94">programs.txt</text></g></svg>'''
PC_NEW = '''<svg width="190" height="140" viewBox="0 0 190 140" style="direction:ltr"><rect x="15" y="10" width="160" height="100" rx="8" fill="#146ed2"/><rect x="25" y="20" width="140" height="80" fill="#e0f2fe"/><rect x="35" y="30" width="34" height="26" rx="3" fill="#0ea5e9"/><rect x="78" y="30" width="34" height="26" rx="3" fill="#22c55e"/><rect x="121" y="30" width="34" height="26" rx="3" fill="#f59e0b"/><rect x="35" y="64" width="120" height="26" rx="3" fill="#bae6fd"/><path d="M80 77 l10 8 l18 -16" stroke="#047857" stroke-width="5" fill="none" stroke-linecap="round" stroke-linejoin="round"/><rect x="80" y="110" width="30" height="14" fill="#146ed2"/><rect x="55" y="124" width="80" height="8" rx="3" fill="#146ed2"/></svg>'''
ARROW = '<svg width="70" height="40" viewBox="0 0 70 40"><path d="M66 20 H12 M26 6 L10 20 L26 34" stroke="#ff9a1f" stroke-width="5" fill="none" stroke-linecap="round" stroke-linejoin="round"/></svg>'

def workflow():
    st = base_state(); st["mode"] = "annot"; st["kicker"] = "How it works"; st["hideMain"] = True
    cards = (card(PC_OLD, 1, "الجهاز القديم", ["أقلع بفلاشة Hiren's (أو شغّله من ويندوز)", "تبويب <b>Backup</b>: اختر المستخدمين والبرامج", "<b>Start Backup</b>"]) + ARROW +
             card(DISK, 2, "حزمة النقل", ["مجلد واحد على قرص خارجي أو فلاشة", "الملفات، المتصفحات، الإعدادات، Wi-Fi", "البرامج مع ملفاتها وبياناتها"]) + ARROW +
             card(PC_NEW, 3, "الجهاز الجديد", ["تبويب <b>Restore</b>: <b>Start Restore</b>", "<b>Install / transfer checked</b> للبرامج", "أعد تشغيل الجهاز ✓"]))
    st["panel"] = f'''<div class="panel" style="left:0;top:150px;width:1620px;text-align:center">
       <h1 style="font-size:40px">كيف يعمل <bdi>PC Migrate Portable</bdi>؟</h1>
       <div class="sub" style="font-size:18px">انقل ملفاتك وبرامجك وإعداداتك إلى حاسوب جديد في 3 خطوات — بدون تثبيت وبدون إنترنت</div>
       <div style="display:flex;direction:rtl;justify-content:center;align-items:center;gap:22px;margin-top:80px">{cards}</div></div>''' + FOOT
    return st

async def main():
    async with async_playwright() as p:
        b = await p.chromium.launch(executable_path="/usr/bin/google-chrome", args=["--no-sandbox"])
        pg = await b.new_page(viewport={"width": 1620, "height": 900}, device_scale_factor=2)
        await pg.goto("file:///home/claude/media/mock.html"); await pg.wait_for_timeout(400)
        for name, fn in [("00-how-it-works", workflow), ("01-backup", backup), ("02-program-chooser", chooser_img), ("03-restore", restore)]:
            await pg.evaluate("s=>render(s)", fn()); await pg.wait_for_timeout(250)
            os.makedirs("out/img/ar", exist_ok=True); await pg.locator(".stage-annot").screenshot(path=f"out/img/ar/{name}.png")
            print("ok", name)
        await b.close()
if __name__ == '__main__':
    asyncio.run(main())
