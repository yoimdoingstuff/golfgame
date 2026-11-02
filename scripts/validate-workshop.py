#!/usr/bin/env python3
"""Validate the generated Golfgame Workshop level pack before launching the game."""
import json
import pathlib
import sys

ROOT = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "build/data")
LEVELS = ROOT / "levels"
EXPECTED = 18
errors = []
print(f"Checking Workshop levels under {LEVELS}")

for number in range(1, EXPECTED + 1):
    path = LEVELS / f"workshop-{number:02d}.level"
    if not path.is_file():
        errors.append(f"missing {path.name}")
        continue
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        errors.append(f"{path.name}: invalid JSON: {exc}")
        continue

    materials = {item.get("name") for item in data.get("materials", [])}
    lightmaps = {item.get("name") for item in data.get("lightmap_images", [])}
    entities = data.get("entities", [])
    types = [item.get("type") for item in entities]
    if "ball-start" not in types:
        errors.append(f"{path.name}: no ball-start entity")
    if "hole" not in types:
        errors.append(f"{path.name}: no hole entity")
    if "camera_zone" not in types:
        errors.append(f"{path.name}: no camera_zone entity")
    if not lightmaps:
        errors.append(f"{path.name}: no lightmap images")

    face_count = 0
    for entity in entities:
        geo = entity.get("geo")
        if not geo:
            continue
        points = geo.get("p", [])
        if len(points) % 3:
            errors.append(f"{path.name}/{entity.get('name')}: point array is not xyz-packed")
        section = entity.get("lightmap_section", {})
        if section.get("lightmap_name") not in lightmaps:
            errors.append(f"{path.name}/{entity.get('name')}: unknown lightmap")
        faces = geo.get("faces", [])
        uv_count = len(section.get("uvs", []))
        if uv_count != len(faces) * 6:
            errors.append(f"{path.name}/{entity.get('name')}: lightmap UV count {uv_count}, expected {len(faces)*6}")
        for face in faces:
            face_count += 1
            indexes = face.get("idxs", [])
            if len(indexes) != 3 or any(not isinstance(i, int) or i < 0 or i >= len(points)//3 for i in indexes):
                errors.append(f"{path.name}/{entity.get('name')}: invalid triangle indexes")
                break
            if face.get("material_name") not in materials:
                errors.append(f"{path.name}/{entity.get('name')}: unknown material {face.get('material_name')}")
            if len(face.get("uvs", [])) != 6:
                errors.append(f"{path.name}/{entity.get('name')}: invalid face UV count")

    print(f"  Hole {number:02d}: {path.stat().st_size:,} bytes, {len(entities)} entities, {face_count} triangles")

actual = sorted(LEVELS.glob("*.level")) if LEVELS.exists() else []
if len(actual) != EXPECTED:
    errors.append(f"expected exactly {EXPECTED} level files, found {len(actual)}")
stock = [p.name for p in actual if p.name.startswith("level-")]
if stock:
    errors.append("upstream sample level files were staged: " + ", ".join(stock))

if errors:
    print("\nWORKSHOP DATA VALIDATION FAILED:")
    for error in errors:
        print("  - " + error)
    sys.exit(1)

print(f"Workshop data validation passed: {EXPECTED} original holes.")
