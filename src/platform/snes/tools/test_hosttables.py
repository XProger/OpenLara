import copy
import struct
import unittest

import hosttables as host


def fixture(**overrides):
    counts = dict.fromkeys(host.COUNTS, 1)
    counts.update(overrides)
    raw = bytearray(28)
    manifest = {"counts": counts, "tables": {}}
    for i, name in enumerate(host.TABLES):
        shape = host.SHAPES[i]
        length = ((counts[shape[0]] + (name == "STATIC_FIRST")) * shape[1]
                  if shape else counts["anims"] * 14 if name == "ANIM_BOX" else
                  12 + counts["boxes"] * 2 if name == "ZONE" else 1000)
        manifest["tables"][name] = {"offset": len(raw), "length": length}
        if name == "ANIM_BOX":
            raw.extend(bytes([i + 1]) * (counts["anims"] * 12))
            raw.extend(bytes(counts["anims"] * 2))
        elif name == "ZONE":
            raw.extend(struct.pack("<6H", *([12] * 6)))
            raw.extend(bytes([i + 1]) * (length - 12))
        else:
            raw.extend(bytes([i + 1]) * length)
    return raw, manifest


class HostTablesTest(unittest.TestCase):
    def test_roundtrip(self):
        raw, manifest = fixture()
        image = host.build(raw, manifest)
        parsed = host.parse(image)
        self.assertEqual(parsed["counts"], manifest["counts"])
        for name, descriptor in parsed["tables"].items():
            a, n = descriptor["offset"], descriptor["length"]
            old = manifest["tables"][name]["offset"]
            self.assertEqual(image[a:a + n], raw[old:old + n])
            self.assertEqual(descriptor["flags"], int(name == "SECTOR"))
            self.assertEqual(descriptor["address"], 0xE00000 + a)

    def test_aggregate_exceeds_bank_without_wrapping(self):
        raw, manifest = fixture(anims=3000, sectors=12000, boxes=4000)
        image = host.build(raw, manifest)
        self.assertGreater(len(image), 0x20000)
        for table in host.parse(image)["tables"].values():
            self.assertLessEqual((table["offset"] & 65535) + table["length"], 65536)

    def test_exact_full_bank_table(self):
        raw, manifest = fixture(sectors=16384)
        image = host.build(raw, manifest)
        sector = host.parse(image)["tables"]["SECTOR"]
        self.assertEqual((sector["offset"], sector["length"]), (65536, 65536))

    def test_zero_length_tables_and_odd_variable_payload(self):
        raw, manifest = fixture(anims=0, trans=0, cmds=0, statics=0)
        manifest["tables"]["AIM"]["length"] -= 1
        host.parse(host.build(raw, manifest))

    def test_oversized_individual_table(self):
        with self.assertRaisesRegex(ValueError, "16-bit table"):
            host.build(*fixture(sectors=16385))

    def test_bad_manifest(self):
        raw, base = fixture()
        for change in (lambda m: m["counts"].update(items=True),
                       lambda m: m["tables"].pop("NAV"),
                       lambda m: m["tables"]["ROOM"].update(offset=len(raw)),
                       lambda m: m["tables"]["ROOM"].update(length=9),
                       lambda m: m["tables"]["LINK"].update(offset=28)):
            manifest = copy.deepcopy(base)
            change(manifest)
            with self.assertRaises(ValueError):
                host.build(raw, manifest)

    def test_bad_directory(self):
        good = host.build(*fixture())
        for offset, fmt, value in ((0, "4s", b"OLH1"), (4, "H", 2), (4, "H", 3), (4, "H", 4), (4, "H", 6),
                                   (6, "H", 17), (8, "I", len(good) + 1),
                                   (12, "H", 255), (14, "H", 1),
                                   (40, "I", 255), (40, "I", 0xFFFF),
                                   (44, "I", 9), (48, "H", 1),
                                   (50, "H", 1), (52, "I", 256),
                                   (60, "H", 0)):
            image = bytearray(good)
            struct.pack_into("<" + fmt, image, offset, value)
            with self.subTest(offset=offset, value=value), self.assertRaises(ValueError):
                host.parse(image)
        for size in (0, 20, 255, len(good) - 1):
            with self.assertRaises(ValueError):
                host.parse(good[:size])

    def test_empty_table_still_needs_an_addressable_pointer(self):
        image = bytearray(host.build(*fixture()))
        image.extend(bytes(host.CAPACITY - len(image)))
        struct.pack_into("<I", image, 8, len(image))
        struct.pack_into("<II", image, 40 + host.TABLES.index("NAV") * 12,
                         host.CAPACITY, 0)
        with self.assertRaisesRegex(ValueError, "table range"):
            host.parse(image)

    def test_six_zone_maps_and_identical_array_aliases(self):
        data = struct.pack("<6H", 12, 18, 24, 18, 12, 24)
        data += struct.pack("<9H", 0, 65535, 1, 2, 3, 4, 5, 6, 7)
        self.assertEqual(host.parse_zones(data, 3),
                         [[0, 65535, 1], [2, 3, 4], [5, 6, 7],
                          [2, 3, 4], [0, 65535, 1], [5, 6, 7]])
        for offset in (0, 11, 13, 14, 26, 65534):
            bad = bytearray(data)
            struct.pack_into("<H", bad, 0, offset)
            with self.subTest(offset=offset), self.assertRaises(ValueError):
                host.parse_zones(bad, 3)
        for bad in (data[:11], data[:-1], data + b"\0\0"):
            with self.assertRaises(ValueError):
                host.parse_zones(bad, 3)
        self.assertEqual(host.parse_zones(struct.pack("<6H", *([12] * 6)), 0), [[]] * 6)

    def test_current_frames_share_boxes_and_whole_tick_maps(self):
        # Three unions, two animations share a map, third uses its union.
        union = (-20, 20, -100, 0, -40, 40)
        pose = (-10, 10, -90, 5, -30, 35)
        data = struct.pack("<18h", *(union * 3))
        data += struct.pack("<3H", 42, 42, 0)
        data += struct.pack("<4H", 3, 0, 50, 50)
        data += struct.pack("<6h", *pose)
        self.assertEqual(host.parse_frames(data, 3), [[union, pose, pose]] * 2 + [[]])
        for offset, value in ((36, 2), (36, 43), (36, 62), (36, 65534),
                              (38, 44), (42, 0), (42, 256), (42, 255),
                              (44, 2), (44, 36), (44, 42), (46, 48), (46, 52)):
            bad = bytearray(data)
            struct.pack_into("<H", bad, offset, value)
            with self.subTest(offset=offset, value=value), self.assertRaises(ValueError):
                host.parse_frames(bad, 3)
        for bad in (data[:40], data[:-1], data[:-2], data + bytes(2)):
            with self.assertRaises(ValueError):
                host.parse_frames(bad, 3)
        self.assertEqual(host.parse_frames(b"", 0), [])

    def test_frame_shape_is_validated_on_pack_and_parse(self):
        raw, manifest = fixture()
        table = manifest["tables"]["ANIM_BOX"]
        struct.pack_into("<H", raw, table["offset"] + 12, 2)
        with self.assertRaisesRegex(ValueError, "ANIM_BOX"):
            host.build(raw, manifest)
        image = bytearray(host.build(*fixture()))
        offset = host.parse(image)["tables"]["ANIM_BOX"]["offset"]
        struct.pack_into("<H", image, offset + 12, 2)
        with self.assertRaisesRegex(ValueError, "ANIM_BOX"):
            host.parse(image)


if __name__ == "__main__":
    unittest.main()
