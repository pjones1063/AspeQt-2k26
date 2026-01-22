import os
import re

# --- Configuration ---
# File extensions to scan
EXTENSIONS = {'.cpp', '.h', '.hpp', '.ui'}

# --- The "Red Flag" Patterns ---
# Format: "Pattern Name": {"regex": r"regex_here", "advice": "How to fix it"}
PATTERNS = {
    "QtZlib Include": {
        "regex": r"#include\s+<QtZlib",
        "advice": "QtZlib internal headers are hidden in Qt 6. Use '#include <zlib.h>' and link against system zlib."
    },
    "Bearer Management (Mobility)": {
        "regex": r"(QNetworkConfigurationManager|QNetworkSession|QNetworkConfiguration)",
        "advice": "The Bearer Management API was removed. Remove this logic; assume network is online or handle request errors."
    },
    "QRegExp Usage": {
        "regex": r"QRegExp",
        "advice": "QRegExp is removed. Replace with 'QRegularExpression'. Note: Wildcard syntax changed."
    },
    "QTextCodec Usage": {
        "regex": r"QTextCodec",
        "advice": "QTextCodec is largely removed. Ensure your files are UTF-8 and use QString directly."
    },
    "QLinkedList Usage": {
        "regex": r"QLinkedList",
        "advice": "QLinkedList is removed. Replace with 'std::list'."
    },
    "QWheelEvent Delta": {
        "regex": r"\->delta\(\)",
        "advice": "QWheelEvent::delta() is deprecated. Use 'angleDelta().y()'."
    },
    "QDesktopWidget": {
        "regex": r"QDesktopWidget",
        "advice": "QDesktopWidget is removed. Use QGuiApplication::primaryScreen() or QScreen."
    },
    "QAction Header": {
        "regex": r"#include\s+<QtWidgets/QAction>",
        "advice": "QAction moved to QtGui in Qt 6. Change include to <QAction> or <QtGui/QAction>."
    },
    "Missing QAction Include": {
        "regex": r"\bQAction\b",
        "advice": "Check if <QAction> is included. It is no longer transitively included by QtWidgets."
    },
    "Printing Setup": {
        "regex": r"QPrinter::ScreenResolution",
        "advice": "ScreenResolution is removed. Use QPrinter::HighResolution or configure DPI manually."
    }
}

def scan_file(filepath):
    issues_found = []
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
            for i, line in enumerate(lines):
                for name, rule in PATTERNS.items():
                    if re.search(rule["regex"], line):
                        # Avoid duplicates per line if multiple matches
                        issues_found.append({
                            "line": i + 1,
                            "rule": name,
                            "advice": rule["advice"],
                            "content": line.strip()
                        })
    except Exception as e:
        print(f"Could not read {filepath}: {e}")
    return issues_found

def main():
    print("--- Starting Qt 6 Migration Scan ---")
    current_dir = os.getcwd()
    total_issues = 0
    
    for root, _, files in os.walk(current_dir):
        for file in files:
            if any(file.endswith(ext) for ext in EXTENSIONS):
                filepath = os.path.join(root, file)
                issues = scan_file(filepath)
                
                if issues:
                    print(f"\n📂 File: {file}")
                    for issue in issues:
                        print(f"   [Line {issue['line']}] {issue['rule']}")
                        print(f"     -> Fix: {issue['advice']}")
                        # print(f"     -> Code: {issue['content']}") # Uncomment for verbosity
                        total_issues += 1

    print("\n" + "="*40)
    print(f"Scan Complete. Found {total_issues} potential issues.")
    print("="*40)

if __name__ == "__main__":
    main()

