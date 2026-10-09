#!/usr/bin/env python3
"""Regenerate the NanoOcp1 SPDX 2.3 SBOM from the current source tree.

Run by .github/workflows/release.yml on every version-tag push so each
GitHub Release carries an SBOM that actually matches the tagged commit,
instead of a hand-maintained file going stale. Can also be run locally,
e.g.:

    python3 scripts/generate_spdx_sbom.py \\
        --version 0.6.3 --commit $(git rev-parse HEAD) \\
        --output NanoOcp1-0.6.3.spdx.json

Fails loudly (non-zero exit) on anything that would make the resulting
SBOM wrong rather than just incomplete:
  * the tag/--version doesn't match the root CMakeLists.txt project version
  * source files in the same package disagree about which license they're under
  * LICENSE doesn't match the license the source headers declare
"""
import argparse
import hashlib
import re
import subprocess
import sys
import uuid
from datetime import datetime, timezone
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

LICENSE_MARKERS = [
    (re.compile(r"GNU Lesser General Public License version (\d+\.\d+)"), "LGPL", "GNU LESSER GENERAL PUBLIC LICENSE"),
    (re.compile(r"GNU General Public License version (\d+\.\d+)"), "GPL", "GNU GENERAL PUBLIC LICENSE"),
]

COPYRIGHT_RE = re.compile(r"^/?\*?\s*Copyright \(c\) (\d{4})(?:-(\d{4}))?,\s*(.+?)\s*$", re.MULTILINE)


def sha1_hex(data: bytes) -> str:
    return hashlib.sha1(data).hexdigest()


def sha256_hex(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def spdx_id_for(rel_path: str) -> str:
    safe = re.sub(r"[^A-Za-z0-9]+", "-", rel_path)
    return f"SPDXRef-File-{safe.strip('-')}"


def detect_license(text: str):
    """Returns (spdx_id, human_name) or None if no recognized header found."""
    for pattern, short, license_file_title in LICENSE_MARKERS:
        m = pattern.search(text)
        if m:
            version = m.group(1)
            suffix = "or-later" if "or any later version" in text else "only"
            spdx_id = f"{short}-{version}-{suffix}"
            return spdx_id, license_file_title
    return None


def collect_package(files, package_label):
    """Hash every file, detect+verify a single consistent license, aggregate copyrights."""
    file_entries = []
    sha1_values = []
    license_seen = {}
    copyrights = {}  # name -> [min_year, max_year], insertion-ordered

    for path in files:
        rel_path = "./" + str(path.relative_to(REPO_ROOT)).replace("\\", "/")
        data = path.read_bytes()
        text = data.decode("utf-8", errors="replace")

        sha1 = sha1_hex(data)
        sha256 = sha256_hex(data)
        sha1_values.append(sha1)

        lic = detect_license(text)
        if lic is None:
            sys.exit(f"error: {rel_path} ({package_label}) has no recognizable LGPL/GPL license header")
        license_seen.setdefault(lic[0], []).append(rel_path)

        for year_start, year_end, name in COPYRIGHT_RE.findall(text):
            start = int(year_start)
            end = int(year_end) if year_end else start
            if name not in copyrights:
                copyrights[name] = [start, end]
            else:
                copyrights[name][0] = min(copyrights[name][0], start)
                copyrights[name][1] = max(copyrights[name][1], end)

        spdx_id = spdx_id_for(str(path.relative_to(REPO_ROOT)))
        file_entries.append({
            "fileName": rel_path,
            "SPDXID": spdx_id,
            "checksums": [
                {"algorithm": "SHA1", "checksumValue": sha1},
                {"algorithm": "SHA256", "checksumValue": sha256},
            ],
            "licenseConcluded": lic[0],
            "copyrightText": "NOASSERTION",
        })

    if len(license_seen) > 1:
        details = "; ".join(f"{lic}: {len(paths)} file(s), e.g. {paths[0]}" for lic, paths in license_seen.items())
        sys.exit(
            f"error: {package_label} files disagree on license -- {details}. "
            "Fix the inconsistent header(s) before the SBOM can be generated."
        )
    license_id, license_file_title = next(iter(license_seen)), detect_license(
        files[0].read_text(encoding="utf-8", errors="replace")
    )[1]

    verification_code = sha1_hex("".join(sorted(sha1_values)).encode("ascii"))
    copyright_text = "; ".join(
        f"Copyright (c) {start}, {name}" if start == end else f"Copyright (c) {start}-{end}, {name}"
        for name, (start, end) in copyrights.items()
    ) or "NOASSERTION"

    return {
        "files": file_entries,
        "hasFiles": [f["SPDXID"] for f in file_entries],
        "packageVerificationCode": verification_code,
        "licenseId": license_id,
        "licenseFileTitle": license_file_title,
        "copyrightText": copyright_text,
    }


def check_license_file_matches(expected_title: str):
    license_path = REPO_ROOT / "LICENSE"
    first_line = license_path.read_text(encoding="utf-8").splitlines()[0].strip()
    if expected_title not in first_line:
        sys.exit(
            f"error: LICENSE starts with '{first_line}' but source headers declare "
            f"'{expected_title}' -- regenerate/replace LICENSE to match before releasing."
        )


def googletest_version() -> str:
    text = (REPO_ROOT / "Tests" / "CMakeLists.txt").read_text(encoding="utf-8")
    m = re.search(r"googletest\s+GIT_REPOSITORY\s+\S+\s+GIT_TAG\s+(\S+)", text)
    if not m:
        sys.exit("error: could not find googletest GIT_TAG in Tests/CMakeLists.txt")
    return m.group(1).lstrip("v")


def doxygen_awesome_css_commit() -> str:
    out = subprocess.run(
        ["git", "ls-tree", "HEAD", "--", "submodules/doxygen-awesome-css"],
        cwd=REPO_ROOT, capture_output=True, text=True, check=True,
    ).stdout.strip()
    if not out:
        sys.exit("error: submodules/doxygen-awesome-css not found in HEAD's tree")
    return out.split()[2]


def project_version_from_cmake() -> str:
    text = (REPO_ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    m = re.search(r"project\(\s*NanoOcp1\s+VERSION\s+(\d+\.\d+\.\d+)", text)
    if not m:
        sys.exit("error: could not find project(NanoOcp1 VERSION ...) in CMakeLists.txt")
    return m.group(1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", required=True, help="Release version, e.g. 0.6.3 (must match CMakeLists.txt)")
    parser.add_argument("--commit", required=True, help="Full commit SHA the release is built from")
    parser.add_argument("--output", required=True, help="Path to write the .spdx.json file to")
    args = parser.parse_args()

    cmake_version = project_version_from_cmake()
    if cmake_version != args.version:
        sys.exit(
            f"error: --version {args.version} does not match CMakeLists.txt's "
            f"project(NanoOcp1 VERSION {cmake_version} ...) -- bump CMakeLists.txt or fix the tag."
        )

    nanoocp1_files = sorted(
        p for p in (REPO_ROOT / "Source").rglob("*")
        if p.is_file() and (p.suffix in (".cpp", ".h") or p.name.endswith(".h.in"))
    )
    demo_files = sorted(
        p for p in (REPO_ROOT / "NanoOcp1Demo").glob("*")
        if p.is_file() and p.suffix in (".cpp", ".h")
    )

    nanoocp1 = collect_package(nanoocp1_files, "NanoOcp1")
    demo = collect_package(demo_files, "NanoOcp1Demo")

    check_license_file_matches(nanoocp1["licenseFileTitle"])

    gtest_version = googletest_version()
    doxygen_commit = doxygen_awesome_css_commit()

    created = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    doc_name = f"NanoOcp1-{args.version}"
    doc_namespace = f"https://github.com/ChristianAhrens/NanoOcp/spdx/{doc_name}-{uuid.uuid4()}"

    spdx = {
        "spdxVersion": "SPDX-2.3",
        "dataLicense": "CC0-1.0",
        "SPDXID": "SPDXRef-DOCUMENT",
        "name": doc_name,
        "documentNamespace": doc_namespace,
        "creationInfo": {
            "created": created,
            "creators": ["Tool: generate_spdx_sbom.py"],
            "comment": (
                "Auto-generated by scripts/generate_spdx_sbom.py in the Release GitHub Actions "
                f"workflow, from commit {args.commit}. File hashes, license headers, the "
                "googletest/doxygen-awesome-css dependency versions, and LICENSE consistency "
                "are all re-derived from the source tree at generation time, not hand-maintained."
            ),
        },
        "documentDescribes": ["SPDXRef-Package-NanoOcp1", "SPDXRef-Package-NanoOcp1Demo"],
        "packages": [
            {
                "name": "NanoOcp1",
                "SPDXID": "SPDXRef-Package-NanoOcp1",
                "versionInfo": args.version,
                "supplier": "Person: Christian Ahrens",
                "originator": "Person: Christian Ahrens",
                "downloadLocation": f"git+https://github.com/ChristianAhrens/NanoOcp.git@{args.commit}",
                "homepage": "https://github.com/ChristianAhrens/NanoOcp",
                "primaryPackagePurpose": "LIBRARY",
                "filesAnalyzed": True,
                "packageVerificationCode": {"packageVerificationCodeValue": nanoocp1["packageVerificationCode"]},
                "hasFiles": nanoocp1["hasFiles"],
                "licenseConcluded": nanoocp1["licenseId"],
                "licenseDeclared": nanoocp1["licenseId"],
                "licenseInfoFromFiles": [nanoocp1["licenseId"]],
                "copyrightText": nanoocp1["copyrightText"],
                "description": (
                    "C++17, dependency-free AES70/OCP.1 TCP client and server library (message "
                    "framing, data types, and device object definitions for d&b audiotechnik "
                    "amplifiers and the DS100 signal engine). Built as the NanoOcp1 static "
                    "library from Source/."
                ),
            },
            {
                "name": "NanoOcp1Demo",
                "SPDXID": "SPDXRef-Package-NanoOcp1Demo",
                "versionInfo": args.version,
                "supplier": "Person: Christian Ahrens",
                "originator": "Person: Christian Ahrens",
                "downloadLocation": f"git+https://github.com/ChristianAhrens/NanoOcp.git@{args.commit}",
                "homepage": "https://github.com/ChristianAhrens/NanoOcp",
                "primaryPackagePurpose": "APPLICATION",
                "filesAnalyzed": True,
                "packageVerificationCode": {"packageVerificationCodeValue": demo["packageVerificationCode"]},
                "hasFiles": demo["hasFiles"],
                "licenseConcluded": demo["licenseId"],
                "licenseDeclared": demo["licenseId"],
                "licenseInfoFromFiles": [demo["licenseId"]],
                "copyrightText": demo["copyrightText"],
                "description": (
                    "Interactive terminal demo CLI (NanoOcp1Demo/) that exercises the NanoOcp1 "
                    "library against a live AES70/OCP.1 device. Statically links NanoOcp1; no "
                    "other third-party code."
                ),
            },
            {
                "name": "googletest",
                "SPDXID": "SPDXRef-Package-googletest",
                "versionInfo": gtest_version,
                "supplier": "Organization: Google LLC",
                "originator": "Organization: Google LLC",
                "downloadLocation": f"git+https://github.com/google/googletest.git@v{gtest_version}",
                "homepage": "https://github.com/google/googletest",
                "primaryPackagePurpose": "LIBRARY",
                "filesAnalyzed": False,
                "licenseConcluded": "BSD-3-Clause",
                "licenseDeclared": "BSD-3-Clause",
                "copyrightText": "NOASSERTION",
                "description": "C++ test framework used only by the NanoOcp1Tests unit-test suite.",
                "comment": (
                    "Fetched at CMake configure time via FetchContent (Tests/CMakeLists.txt); "
                    "linked only into the NanoOcp1Tests test binary, which is neither installed "
                    "nor attached to GitHub Releases. Not present in, or distributed with, the "
                    "NanoOcp1 library or the NanoOcp1Demo application."
                ),
            },
            {
                "name": "doxygen-awesome-css",
                "SPDXID": "SPDXRef-Package-doxygen-awesome-css",
                "versionInfo": doxygen_commit[:10],
                "supplier": "Person: jothepro",
                "originator": "Person: jothepro",
                "downloadLocation": f"git+https://github.com/jothepro/doxygen-awesome-css.git@{doxygen_commit}",
                "homepage": "https://github.com/jothepro/doxygen-awesome-css",
                "primaryPackagePurpose": "OTHER",
                "filesAnalyzed": False,
                "licenseConcluded": "MIT",
                "licenseDeclared": "MIT",
                "copyrightText": "NOASSERTION",
                "description": (
                    "Doxygen HTML theme (CSS/JS) used solely to style the generated API "
                    "documentation published to GitHub Pages."
                ),
                "comment": (
                    "Vendored as the git submodule submodules/doxygen-awesome-css. "
                    "Documentation tooling only -- not compiled, linked, or distributed with "
                    "the NanoOcp1 library, NanoOcp1Demo, or NanoOcp1Tests."
                ),
            },
            {
                "name": "Winsock2 (ws2_32)",
                "SPDXID": "SPDXRef-Package-ws2-32",
                "versionInfo": "NOASSERTION",
                "supplier": "Organization: Microsoft Corporation",
                "downloadLocation": "NOASSERTION",
                "homepage": "NOASSERTION",
                "primaryPackagePurpose": "LIBRARY",
                "filesAnalyzed": False,
                "licenseConcluded": "NOASSERTION",
                "licenseDeclared": "NOASSERTION",
                "copyrightText": "NOASSERTION",
                "description": "Windows Sockets 2 system library, part of the Windows SDK.",
                "comment": (
                    "Linked via target_link_libraries(NanoOcp1 PUBLIC ws2_32) only when NanoOcp1 "
                    "is built for Windows (Source/CMakeLists.txt, if(WIN32) guard). "
                    "Platform-provided system component, not a redistributed third-party "
                    "dependency; included here for completeness/traceability only."
                ),
            },
        ],
        "files": nanoocp1["files"] + demo["files"],
        "relationships": [
            {"spdxElementId": "SPDXRef-DOCUMENT", "relationshipType": "DESCRIBES", "relatedSpdxElement": "SPDXRef-Package-NanoOcp1"},
            {"spdxElementId": "SPDXRef-DOCUMENT", "relationshipType": "DESCRIBES", "relatedSpdxElement": "SPDXRef-Package-NanoOcp1Demo"},
            {
                "spdxElementId": "SPDXRef-Package-NanoOcp1Demo",
                "relationshipType": "STATIC_LINK",
                "relatedSpdxElement": "SPDXRef-Package-NanoOcp1",
                "comment": "NanoOcp1Demo links the NanoOcp1 static library (target_link_libraries(NanoOcp1Demo PRIVATE NanoOcp1)).",
            },
            {
                "spdxElementId": "SPDXRef-Package-NanoOcp1",
                "relationshipType": "OPTIONAL_DEPENDENCY_OF",
                "relatedSpdxElement": "SPDXRef-Package-ws2-32",
                "comment": "ws2_32 is linked only in Windows builds (if(WIN32) in Source/CMakeLists.txt); on macOS/Linux NanoOcp1 has zero external dependencies.",
            },
            {
                "spdxElementId": "SPDXRef-Package-googletest",
                "relationshipType": "TEST_DEPENDENCY_OF",
                "relatedSpdxElement": "SPDXRef-Package-NanoOcp1",
                "comment": "Build/test-time only; excluded from the installed library and from NanoOcp1Demo.",
            },
            {
                "spdxElementId": "SPDXRef-Package-doxygen-awesome-css",
                "relationshipType": "DEV_DEPENDENCY_OF",
                "relatedSpdxElement": "SPDXRef-Package-NanoOcp1",
                "comment": "Documentation-build-time only (Doxygen HTML theme); excluded from the installed library and from NanoOcp1Demo.",
            },
        ],
    }

    import json
    output_path = Path(args.output)
    output_path.write_text(json.dumps(spdx, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {output_path} ({len(spdx['files'])} files analyzed, license {nanoocp1['licenseId']})")


if __name__ == "__main__":
    main()
