# AcidLine – סינת' אסיד לאבלטון

סינת' בס מונופוני בסגנון 303: אוסילטור Saw/Square, פילטר לאדר 24dB עם רזוננס, מעטפת פילטר, Accent, Slide ו-Drive.

## הכפתורים
| כפתור | מה הוא עושה |
|---|---|
| CUTOFF | תדר הפילטר. הכפתור הכי חשוב לסוויפים |
| RESONANCE | ה"צווחה" של האסיד. ככל שעולים, הסאונד יותר חד ומצפצף |
| ENV MOD | כמה כל נוטה "פותחת" את הפילטר. זה ה"וואו" בכל נוטה |
| DECAY | כמה מהר הפילטר נסגר. קצר נותן סאונד פלאקי, ארוך נותן סאונד מתנפח |

הגל (Saw), האקסנט, הסלייד והדיסטורשן מכוונים מראש בפנים. את הווליום משנים בערוץ של אבלטון.

## תוספות
- **PATTERN**: 4 פטרנים מוכנים של 16 צעדים (Classic Acid, Rolling Psy, Squelchy, Hypnotic). הם מסונכרנים לטמפו של אבלטון ומתחילים לנגן כשלוחצים Play, גם בלי קליפ MIDI. לחיצה על קליד משנה את הטון של הפטרן ונשארת עד הלחיצה הבאה. ב-"Off" הסינת' מנגן רגיל מ-MIDI.
- **GUITAR DIST**: דיסטורשן בסגנון מגבר גיטרה. יש בו הידוק של הבס לפני הדיסטורשן, שני שלבי קליפינג אסימטרי (שנותנים חום) וסימולציית קבינט שמורידה את הצרימה. 0 = כבוי.

**בקליפ MIDI (במצב Off):**
- **Accent**: נוטה עם velocity 100 ומעלה
- **Slide**: נוטה שחופפת לנוטה הבאה (מאריכים אותה קצת לתוך הבאה)

## איך מקבלים קובץ פלאגין (הדרך הכי קלה, בלי להתקין כלום)
1. פותחים חשבון ב-GitHub ויוצרים repository חדש
2. מעלים אליו את כל התיקייה (כולל `.github`)
3. בלשונית **Actions** הבנייה רצה לבד (בערך 10 דקות)
4. בסיום מורידים את `AcidLine-VST3-Windows` או `AcidLine-VST3-macOS` מתחתית הדף

## בנייה במחשב שלך
צריך: CMake ו-Git, וגם Visual Studio 2022 עם "Desktop development with C++" (ווינדוס) או Xcode (מק).
```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
התוצאה: `build/AcidLine_artefacts/Release/VST3/AcidLine.vst3`

## התקנה באבלטון
- **ווינדוס:** מעתיקים את `AcidLine.vst3` אל `C:\Program Files\Common Files\VST3`
- **מק:** מעתיקים אל `~/Library/Audio/Plug-Ins/VST3`. אם מק חוסם אותו, מריצים בטרמינל:
  `xattr -cr ~/Library/Audio/Plug-Ins/VST3/AcidLine.vst3`
- באבלטון: Settings ← Plug-Ins ← מפעילים "Use VST3 Plug-In System Folders" ולוחצים Rescan
- גוררים את AcidLine מ-Plug-Ins לערוץ MIDI
