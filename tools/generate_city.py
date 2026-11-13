#!/usr/bin/env python3
"""Build the fixed city mesh from an editable layout, without an OpenGL context."""
import argparse
import json
import math
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent
LAYOUT = ROOT / 'assets/city/layout.json'
OUTPUT = ROOT / 'assets/city/city.mesh'
PALETTES = {
    'tower': {'towerBlue', 'towerGreen', 'towerPurple'},
    'residential': {'residentialClay', 'residentialSand', 'residentialRose'},
    'commercial': {'commercialYellow', 'commercialGreen', 'commercialPeach'},
}


def number(value, name, positive=False):
    if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
        raise ValueError(f'{name} must be a finite number')
    if positive and value <= 0:
        raise ValueError(f'{name} must be positive')
    return float(value)


def overlap(a, b):
    return (abs(a['x'] - b['x']) < (a['width'] + b['width']) / 2 and
            abs(a['z'] - b['z']) < (a['depth'] + b['depth']) / 2)


def validate(data):
    size = number(data['chunk_size'], 'chunk_size', True)
    spacing = number(data['road_spacing'], 'road_spacing', True)
    road = number(data['road_width'], 'road_width', True)
    height = number(data['central_tower_height'], 'central_tower_height', True)
    # Park and generated-block coordinates currently assume this grid.
    if size != 360 or spacing != 120 or road != 8:
        raise ValueError('the current chunk template requires chunk_size 360, road_spacing 120 and road_width 8')
    if height < 4.5:
        raise ValueError('central_tower_height must be at least 4.5')
    buildings = data['buildings']
    if not isinstance(buildings, list) or not buildings:
        raise ValueError('buildings must be a non-empty array')
    parsed = []
    for index, item in enumerate(buildings):
        b = {key: number(item[key], f'building {index + 1} {key}', key in ('width', 'depth', 'height'))
             for key in ('x', 'z', 'width', 'depth', 'height')}
        style = item['style']
        palette = item['palette']
        if style not in PALETTES or palette not in PALETTES[style]:
            raise ValueError(f'building {index + 1}: invalid style/palette combination')
        b.update(style=style, palette=palette)
        if abs(b['x']) + b['width']/2 > size/2 or abs(b['z']) + b['depth']/2 > size/2:
            raise ValueError(f'building {index + 1}: outside starting chunk')
        for c in (-size/2, -spacing/2, spacing/2, size/2):
            if abs(b['x']-c) < (b['width']+road+6)/2 or abs(b['z']-c) < (b['depth']+road+6)/2:
                raise ValueError(f'building {index + 1}: overlaps road or sidewalk')
        if overlap(b, {'x':0,'z':0,'width':30,'depth':30}):
            raise ValueError(f'building {index + 1}: overlaps the central tower')
        if overlap(b, {'x':spacing,'z':spacing,'width':spacing-14,'depth':spacing-14}):
            raise ValueError(f'building {index + 1}: overlaps the reserved park')
        if any(overlap(b, other) for other in parsed):
            raise ValueError(f'building {index + 1}: overlaps another building')
        # The generated no-park template fills the reserved park block with two buildings.
        for other in ({'x':96,'z':100,'width':36,'depth':40}, {'x':144,'z':140,'width':32,'depth':30}):
            if overlap(b, other):
                raise ValueError(f'building {index + 1}: overlaps a generated-chunk building')
        parsed.append(b)
    return size, spacing, road, height, parsed


def f(value):
    return f'{value:.9g}f' if '.' in f'{value:.9g}' or 'e' in f'{value:.9g}' else f'{value:.9g}.0f'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--layout', type=Path, default=LAYOUT)
    parser.add_argument('--output', type=Path, default=OUTPUT)
    args = parser.parse_args()
    try:
        data = json.loads(args.layout.read_text())
        size, spacing, road, height, buildings = validate(data)
        with tempfile.TemporaryDirectory(prefix='city-generator-') as tmp:
            tmp = Path(tmp)
            header = ['constexpr Building startingBuildings[] = {']
            for b in buildings:
                fields = ', '.join(f(b[k]) for k in ('x','z','width','depth','height'))
                header.append(f'    {{{fields}, {b["palette"]}, BuildingStyle::{b["style"].capitalize()}}},')
            header.append('};')
            (tmp / 'city_layout_generated.h').write_text('\n'.join(header)+'\n')
            compiler = os.environ.get('CXX', 'g++')
            command = [compiler, '-std=c++17', '-O2', '-I'+str(ROOT/'src'), '-I'+str(tmp),
                       '-I'+str(ROOT/'tools/city'),
                       f'-DCITY_CHUNK_SIZE={f(size)}', f'-DCITY_ROAD_SPACING={f(spacing)}',
                       f'-DCITY_ROAD_WIDTH={f(road)}', f'-DCITY_TOWER_HEIGHT={f(height)}',
                       str(ROOT/'tools/city/export.cpp'), str(ROOT/'tools/city/CityGenerator.cpp'),
                       str(ROOT/'src/math/Matrix4.cpp'), str(ROOT/'src/renderer/PrimitiveMeshes.cpp'),
                       '-o', str(tmp/'city-export')]
            subprocess.run(command, cwd=ROOT, check=True)
            result = tmp/'city.mesh'
            subprocess.run([str(tmp/'city-export'), str(result)], cwd=ROOT, check=True)
            with result.open('rb') as stream:
                magic, version, count = struct.unpack('<8sII', stream.read(16))
            if (magic, version) != (b'CITYMESH', 1) or not count:
                raise ValueError('generator produced an invalid asset')
            args.output.parent.mkdir(parents=True, exist_ok=True)
            # Replace only after the entire asset was generated successfully.
            staged = args.output.with_suffix(args.output.suffix + '.tmp')
            try:
                staged.write_bytes(result.read_bytes())
                os.replace(staged, args.output)
            finally:
                if staged.exists():
                    staged.unlink()
        print(f'Wrote {args.output} ({len(buildings)} starting buildings, {count} batches)')
    except (OSError, KeyError, TypeError, ValueError, subprocess.CalledProcessError) as error:
        print(f'City generation failed: {error}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
