<div dir="rtl">

# PC Migrate Portable

أداة مجانية ومفتوحة المصدر لنقل **الملفات والمتصفحات وإعدادات ويندوز والبرامج المثبتة مع بياناتها** من حاسوب قديم إلى حاسوب جديد، على طريقة PCmover.

- ملف `exe` واحد حجمه حوالي 470 KB، **Portable** ولا يحتاج .NET ولا Python ولا أي مكتبة.
- يعمل على **Windows 10/11** وداخل **Hiren's BootCD PE** (WinPE) عند الإقلاع من USB.

## ⬇️ التحميل

| الملف | الاستعمال |
|---|---|
| [**PCMigrate-Setup-1.1.0.exe**](https://github.com/yacineboulyali/pcmigrate-portable/releases/latest/download/PCMigrate-Setup-1.1.0.exe) | ملف تثبيت عادي: اختصار في قائمة ابدأ وسطح المكتب، مع إمكانية الإزالة |
| [**PCMigrate-Portable-1.1.0.zip**](https://github.com/yacineboulyali/pcmigrate-portable/releases/latest/download/PCMigrate-Portable-1.1.0.zip) | نسخة محمولة للفلاشة و Hiren's: `PCMigrate.exe` و `Run-PCMigrate.bat` و `Copy-To-USB.bat` |
| [**PCMigrate.exe**](https://github.com/yacineboulyali/pcmigrate-portable/releases/latest/download/PCMigrate.exe) | البرنامج وحده |

كل الإصدارات في صفحة [Releases](https://github.com/yacineboulyali/pcmigrate-portable/releases).

> ⚠️ البرنامج غير موقّع رقميًا، لذلك قد يظهر تحذير **Windows SmartScreen**. اضغط **More info** ثم **Run anyway**.

## ماذا ينقل؟

| العنصر | التفاصيل |
|---|---|
| ملفات المستخدم | Desktop، Documents، Downloads، Pictures، Music، Videos، Favorites... مع مجلدات OneDrive |
| المتصفحات | Chrome، Edge، Brave، Opera، Firefox: المفضلة والسجل والإضافات |
| إعدادات ويندوز | الماوس، لوحة المفاتيح، اللغة، الثيم، الخلفية، Quick Access، Wi-Fi |
| **البرامج مع بياناتها** | ملفات البرنامج، الـ Registry، الخدمات، الاختصارات، و AppData / ProgramData |
| إعادة التثبيت | قائمة البرامج، وإعادة تثبيتها نظيفةً عبر **winget** |

## طريقة الاستعمال السريعة

1. **على الجهاز القديم:**
   - أقلع بـ Hiren's BootCD PE (أو شغّل البرنامج في ويندوز).
   - في تبويب **1. Backup** اختر المستخدمين وما تريد نقله، والبرامج عبر زر **Choose...**.
   - احفظ حزمة النقل على قرص خارجي.
2. **على الجهاز الجديد:**
   - في تبويب **2. Restore** اضغط **Start Restore**.
   - في قائمة البرامج اضغط **Install / transfer checked**.
   - أعد تشغيل الجهاز.

الدليل الكامل والحدود في [docs/README_AR.md](docs/README_AR.md).

## البناء من المصدر

```sh
sudo apt install g++-mingw-w64-x86-64 nsis zip
./build.sh      # build/PCMigrate.exe
./package.sh    # dist/: Setup + Portable zip
```

على Windows: استعمل `build.bat` مع MSYS2 / MinGW-w64.

كل دفع (push) يُبنى تلقائيًا عبر **GitHub Actions**، وكل tag يبدأ بـ `v` (مثل `v1.2.0`) يُنشئ Release جديدًا مع ملفات التحميل.

</div>

---

### English

**PC Migrate Portable** moves user files, browsers, Windows settings and installed programs (with their files, registry, services and data) from an old PC to a new one, like PCmover.

- Single static Win32/C++ executable, no runtime needed.
- Runs on Windows 10/11 and inside Hiren's BootCD PE.
- Download: [Setup](https://github.com/yacineboulyali/pcmigrate-portable/releases/latest/download/PCMigrate-Setup-1.1.0.exe) · [Portable zip](https://github.com/yacineboulyali/pcmigrate-portable/releases/latest/download/PCMigrate-Portable-1.1.0.zip)

MIT License © 2026 Yacine Boulyali
