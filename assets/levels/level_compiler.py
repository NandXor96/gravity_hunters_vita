#!/usr/bin/env python3
"""
Level compiler: JSON -> binary .lvl according to lvl_binary.md

Usage:
    python3 tools/level_compiler.py build levels/example_level.json build/example_level.lvl

Design goals:
- Clear constants for all magic values / enum mappings (no magic numbers inline)
- Validate input and produce helpful errors
- Produce little-endian binary compatible with the layout in `lvl_binary.md`

"""
import sys
import json
import struct
from pathlib import Path

# --------------------------- Layout constants ---------------------------
MAGIC = b'GHLV'
VERSION = 1

# Field sizes / limits
MAX_PLANETS = 1024
MAX_ENEMIES = 1024

PLANET_TYPE = {
    'unknown': 0,
    'rocky': 1,
    'gas': 2,
    'ice': 3,
    'metal': 4,
}

ENEMY_TYPE = {
    'unknown': 0,
    'fighter': 1,
    'bomber': 2,
    'shooter': 3,
}

SPAWN_KIND = {
    'on_start': 0,
    'on_timer': 1,
    'on_death': 2,
}

# Struct formats (little-endian)
# Header fixed part stops before start_text which is a nul-terminated utf-8 string
HEADER_FMT = '<4sHHIHH3IffIII'
# fields: magic(4s), version(H), reserved(H), time_limit(I),
# goal_kills(H), goal_deaths(H), rating[3](3I),
# player_pos_x(f), player_pos_y(f), player_health(I), planets_count(I), enemies_count(I)

PLANET_FMT = '<Bfff'  # type(uint8), size(float), pos_x(float), pos_y(float)
ENEMY_FMT = '<HBffBIBII'  # id(uint16), type(uint8), pos_x(f), pos_y(f), difficulty(uint8), health(uint32), spawn_kind(uint8), spawn_arg(uint32), spawn_delay(uint32)

# --------------------------- Helpers ---------------------------

def clamp_u16(x):
    if x is None:
        return 0
    x = int(x)
    if x < 0:
        return 0
    if x > 0xFFFF:
        return 0xFFFF
    return x


def clamp_u32(x):
    if x is None:
        return 0
    x = int(x)
    if x < 0:
        return 0
    if x > 0xFFFFFFFF:
        return 0xFFFFFFFF
    return x


def parse_spawn_when(s):
    # returns (spawn_kind, spawn_arg, spawn_delay)
    # Accepts multiple forms:
    # - None / missing -> on_start
    # - numeric -> on_timer with seconds
    # - string: 'on_start' | 'on_timer:30' | 'on_death:1'
    # - dict: {"kind": "on_start"|"on_timer"|"on_death", "arg": <int>, "delay": <int>}
    if not s:
        return SPAWN_KIND['on_start'], 0, 0
    # dict form
    if isinstance(s, dict):
        kind_s = s.get('kind', 'on_start')
        if kind_s not in SPAWN_KIND:
            raise ValueError(f'unknown spawn.kind: {kind_s}')
        kind = SPAWN_KIND[kind_s]
        arg = s.get('arg', 0)
        delay = s.get('delay', 0)
        return kind, arg, delay
    # numeric -> on_timer
    if isinstance(s, (int, float)):
        return SPAWN_KIND['on_timer'], int(s), 0
    # string
    s = s.strip()
    if s == 'on_start':
        return SPAWN_KIND['on_start'], 0, 0
    if s.startswith('on_timer:'):
        _, val = s.split(':', 1)
        return SPAWN_KIND['on_timer'], int(val), 0
    if s.startswith('on_death:'):
        _, val = s.split(':', 1)
        return SPAWN_KIND['on_death'], val, 0
    raise ValueError(f'unrecognized spawn_when: {s}')


def parse_difficulty(value):
    if value is None:
        raise ValueError('enemy difficulty is required')
    if isinstance(value, bool):
        raise ValueError('enemy difficulty must be an integer between 0 and 255')
    if isinstance(value, (int, float)):
        if isinstance(value, float):
            if not value.is_integer():
                raise ValueError(f'enemy difficulty must be an integer, got {value!r}')
            ivalue = int(value)
        else:
            ivalue = int(value)
    elif isinstance(value, str):
        stripped = value.strip()
        if stripped == '':
            raise ValueError('enemy difficulty must be an integer between 0 and 255')
        try:
            ivalue = int(stripped, 10)
        except ValueError as exc:
            raise ValueError(f'enemy difficulty must be an integer, got {value!r}') from exc
    else:
        raise ValueError(f'enemy difficulty has unsupported type {type(value)!r}')

    if ivalue < 0 or ivalue > 255:
        raise ValueError(f'enemy difficulty must be within 0..255, got {ivalue}')
    return ivalue


# --------------------------- Main builder ---------------------------

def build_level(json_path: Path, out_path: Path):
    with json_path.open('r', encoding='utf-8') as f:
        data = json.load(f)

    # Header fields with defaults
    version = int(data.get('version', VERSION))
    time_limit = clamp_u32(data.get('time_limit', 0))

    goal = data.get('goal', {})
    goal_kills = clamp_u16(goal.get('kills', 0))
    goal_deaths = clamp_u16(goal.get('deaths', 0))

    rating = data.get('rating', [0, 0, 0])
    if not (isinstance(rating, list) and len(rating) == 3):
        raise ValueError('rating must be an array of 3 integers')
    rating = [clamp_u32(x) for x in rating]

    player = data.get('player', {})
    player_pos = player.get('pos', [0.0, 0.0])
    player_pos_x = float(player_pos[0])
    player_pos_y = float(player_pos[1])
    player_health = clamp_u32(player.get('health', 0))

    planets = data.get('planets', []) or []
    enemies = data.get('enemies', []) or []

    if len(planets) > MAX_PLANETS:
        raise ValueError(f'too many planets: {len(planets)} (max {MAX_PLANETS})')
    if len(enemies) > MAX_ENEMIES:
        raise ValueError(f'too many enemies: {len(enemies)} (max {MAX_ENEMIES})')

    # Parse planet entries
    planet_entries = []
    for p in planets:
        # planet type can be numeric (0..15) or a known string
        ptype_raw = p.get('type', 'unknown')
        if isinstance(ptype_raw, int):
            # clamp to 0..15
            ptype = max(0, min(15, int(ptype_raw)))
        else:
            ptype = PLANET_TYPE.get(ptype_raw, PLANET_TYPE['unknown'])
        size = float(p.get('size', 0.0))
        pos = p.get('pos', [0.0, 0.0])
        px = float(pos[0])
        py = float(pos[1])
        planet_entries.append((ptype, size, px, py))

    # Parse enemies (first pass)
    enemy_objects = []
    for e in enemies:
        # id: optional numeric id, else assigned later
        eid = e.get('id')
        if eid is not None:
            eid = int(eid)
            if eid < 0 or eid > 0xFFFF:
                raise ValueError('enemy id out of range 0..65535')
        # type
        # enemy type can be numeric (0..8) or a known string
        etype_raw = e.get('type', 'unknown')
        if isinstance(etype_raw, int):
            etype = max(0, min(8, int(etype_raw)))
        else:
            etype = ENEMY_TYPE.get(etype_raw, ENEMY_TYPE['unknown'])
        pos = e.get('pos', [0.0, 0.0])
        ex = float(pos[0])
        ey = float(pos[1])
        difficulty = parse_difficulty(e.get('difficulty'))
        health = clamp_u32(e.get('health', 0))
        spawn_when_raw = e.get('spawn_when', 'on_start')
        spawn_kind, spawn_arg, spawn_delay = parse_spawn_when(spawn_when_raw)
        spawn_delay = clamp_u32(spawn_delay)
        enemy_objects.append({
            'raw_id': eid,
            'type': etype,
            'pos': (ex, ey),
            'difficulty': difficulty,
            'health': health,
            'spawn_kind': spawn_kind,
            'spawn_arg': spawn_arg,
            'spawn_delay': spawn_delay,
        })

    # Build mapping of enemy ids to indices (index used by spawn_arg for on_death)
    # Assign numeric ids if none provided: use 1-based index
    id_to_index = {}
    for idx, e in enumerate(enemy_objects):
        if e['raw_id'] is None:
            eid = idx + 1
        else:
            eid = int(e['raw_id'])
        e['id'] = eid
        id_to_index[eid] = idx

    # Resolve spawn_arg for on_death (they may contain target id strings or ints)
    for e in enemy_objects:
        if e['spawn_kind'] == SPAWN_KIND['on_death']:
            # spawn_arg may be a string id or numeric id
            target = e['spawn_arg']
            try:
                target_id = int(target)
            except Exception:
                raise ValueError(f'bad on_death target id: {target}')
            if target_id not in id_to_index:
                raise ValueError(f'on_death target id {target_id} not found among enemies')
            # Store the actual enemy ID (as provided in JSON) in spawn_arg
            # The runtime loader may resolve this ID to an index if needed.
            e['spawn_arg_index'] = target_id
        else:
            e['spawn_arg_index'] = int(e['spawn_arg']) if isinstance(e['spawn_arg'], int) else 0

    # ----- Write binary -----
    header_packed = struct.pack(
        HEADER_FMT,
        MAGIC,
        int(version),
        0,
        int(time_limit),
        int(goal_kills),
        int(goal_deaths),
        int(rating[0]),
        int(rating[1]),
        int(rating[2]),
        float(player_pos_x),
        float(player_pos_y),
        int(player_health),
        int(len(planet_entries)),
        int(len(enemy_objects)),
    )

    # start_text (nul-terminated utf-8)
    start_text = data.get('start_text', '')
    start_bytes = start_text.encode('utf-8') + b'\x00'

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open('wb') as f:
        f.write(header_packed)
        f.write(start_bytes)
        # planets
        for p in planet_entries:
            p_packed = struct.pack(PLANET_FMT, int(p[0]), float(p[1]), float(p[2]), float(p[3]))
            f.write(p_packed)
        # enemies
        for idx, e in enumerate(enemy_objects):
            # id is uint16, spawn_arg for on_death is index (uint32)
            eid = int(e['id'])
            etype = int(e['type'])
            ex, ey = e['pos']
            diff = int(e['difficulty'])
            health = int(e['health'])
            skind = int(e['spawn_kind'])
            sarg = int(e['spawn_arg_index'])
            sdelay = int(e['spawn_delay'])
            e_packed = struct.pack(ENEMY_FMT, eid, etype, float(ex), float(ey), diff, health, skind, sarg, sdelay)
            f.write(e_packed)

    print(f'Wrote {out_path} (planets={len(planet_entries)} enemies={len(enemy_objects)})')


# --------------------------- CLI ---------------------------
if __name__ == '__main__':
    if len(sys.argv) < 4 or sys.argv[1] != 'build':
        print('Usage: level_compiler.py build <input.json> <output.lvl>')
        sys.exit(2)
    inp = Path(sys.argv[2])
    out = Path(sys.argv[3])
    build_level(inp, out)
