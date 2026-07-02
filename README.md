```markdown
# DuplicateFinder 

A lightweight, high-performance, and completely **non-destructive** JPEG image duplicate detector written in C/C++.

Developed by **Terry Deem** in collaboration with **Google Gemini**, this tool is designed "old-school" style to recursively scan directories, analyze image content (independent of filename or resolution), and export an easy-to-read database (`imageDB.csv`) for seamless duplicate management in Microsoft Excel.

---

## Motivation & Philosophy
Managing multi-generational family photo collections often results in thousands of duplicate files scattered across various backup drives. Over time, filenames change, or some photos are compressed/resized, making basic file-name or file-size matching completely useless.

While modern AI or machine learning models can solve this, **DuplicateFinder** takes an elegant, lightweight approach:
* **Resolution-Agnostic:** Images are resized dynamically in RAM so variations in image dimensions don't break matching patterns.
* **Content Hashing:** Uses a custom pixel-based MD5 fingerprint instead of raw file hashes, evaluating the true visual content while ignoring container metadata metadata variations.
* **100% Safe & Read-Only:** The program is strictly **read-only**. It will never delete, modify, or move your original pictures. It simply provides a report, letting you maintain total control over your memories.
* **Zero Bloat:** Designed with public-domain static libraries to build into a standalone executable. No dynamic link libraries (DLLs) are required when sharing it with non-technical family members.

---

## How It Works

1. **Recursive Directory Traversal:** Leveraging the native Windows API (`FindFirstFileA` / `FindNextFile`), the program walks down your folders starting from the directory where the application is run (`.\\`).
2. **In-Memory Image Normalization:** It utilizes the lightweight, header-only `stb_image.h` library to decode JPEG graphics. To counter resolution mismatches, it scales every image to a standard `512x512` pixel grid in system memory using a nearest-neighbor algorithm.
3. **Pixel Hashing:** Instead of hashing the raw file container (which includes metadata variations like dates and tags), it computes an MD5 signature directly on the normalized raw pixel buffer in system memory. This eliminates temporary file write operations and speeds up processing.
4. **Cross-Matching & Logging:** The software analyzes all entries to match identical hashes and maps duplicates into a tabular matrix.

---

## Output Format (`imageDB.csv`)
The program outputs a single comma-separated values spreadsheet named `imageDB.csv` in its execution folder. It contains three columns:

| Column Name | Description |
| :--- | :--- |
| `FilePath` | The absolute or relative system path to the scanned JPEG. |
| `Hash` | The unique 32-character hexadecimal MD5 signature of the normalized pixel data. |
| `DuplicateOf` | Comma-separated **1-based row numbers** pointing to subsequent matching duplicates in the exact same sheet (perfect for tracking lines natively inside Microsoft Excel). |

---

## Environment & Dependencies

* **Operating System:** Microsoft Windows (relies on `<windows.h>` and native file navigation).
* **External Core Libraries:** `stb_image.h` (Single-file header-only public domain library from [nothings/stb](https://github.com/nothings/stb)). No complex linker configurations or package managers required!
* **Compiler Requirements:** Any modern standard C++ Compiler supporting Windows API environments (MSVC `cl.exe`, MinGW `g++`, Clang).

---

## 🚀 Building and Running

### 1. Prerequisites
Make sure you place the `stb_image.h` file in the same directory as your source code before compiling. You can download it directly from the official source:
```bash
curl -O [https://raw.githubusercontent.com/nothings/stb/master/stb_image.h](https://raw.githubusercontent.com/nothings/stb/master/stb_image.h)

### 2. Compilation (Using MSVC via Developer Command Prompt)
DOS
cl.exe /O2 /MT DuplicateFinder.cpp
(The /O2 flag optimizes the code for execution speed, and /MT links the C/C++ run-time library statically so the program can be run portably on any Windows machine without needing external DLLs).

### 3. Execution
Place the compiled executable (DuplicateFinder.exe) into the root folder containing your image collection.

Run the program by double-clicking it or through your command terminal:

DOS
DuplicateFinder.exe
Open the newly generated imageDB.csv file in Microsoft Excel or any spreadsheet tool to sort, evaluate, and clean up your duplicate photos safely.

### Authorship, Disclaimer & License
I used Google Gemini to write most of this code - credit or blame AI... Since AI wrote most of this - I'm not making any claim to the code - feel free to use it for whatever you want. Any license concerns are the AI's problem and not mine. This code was developed for personal use only.
— Terry Deem

License: Public Domain / Unlicense. Feel free to fork, refactor, optimize, or integrate this code into any project.
Warranty: Provided "as is" without warranty of any kind. Take proper precautions (such as backing up your data) when executing custom utilities over your storage drives. The authors assume no liability for bugs, data loss, or system instability.
