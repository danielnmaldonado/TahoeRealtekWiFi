#!/usr/bin/env python3
"""Exercise pinned country generation and reject modified primary inputs."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

class RegulatoryGeneratorTests(unittest.TestCase):
    def test_generated_table_matches_pinned_primary(self):
        p = subprocess.run([sys.executable, str(ROOT/'scripts/generate-dfs-regulatory.py'), '--check'], capture_output=True, text=True)
        self.assertEqual(p.returncode, 0, p.stdout+p.stderr)
        self.assertIn('183 domains; 15 centers', p.stdout)

    def test_changed_input_never_overwrites_existing_table(self):
        with tempfile.TemporaryDirectory(prefix='rtw-regulatory-') as temporary:
            copy = Path(temporary)
            for name in ['scripts/generate-dfs-regulatory.py', 'src/DFSRegulatory-Upstream.json',
                         'vendor/wireless-regdb/db.txt', 'vendor/wireless-regdb/dbparse.py',
                         'vendor/wireless-regdb/LICENSE']:
                destination = copy/name
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(ROOT/name, destination)
            table = copy/'src/DFSRegulatoryTables.inc'
            table.write_text('preserve-this-output\n')
            database = copy/'vendor/wireless-regdb/db.txt'
            database.write_bytes(database.read_bytes()+b'\n')
            p = subprocess.run([sys.executable, str(copy/'scripts/generate-dfs-regulatory.py')], capture_output=True, text=True)
            self.assertNotEqual(p.returncode, 0)
            self.assertIn('Changed pinned wireless-regdb input', p.stderr)
            self.assertEqual(table.read_text(), 'preserve-this-output\n')

    def test_country_frequency_boundary_is_not_extrapolated(self):
        import runpy
        generated = runpy.run_path(str(ROOT/'scripts/generate-dfs-regulatory.py'))['generate']()[0]
        egypt = next(row for row in generated.splitlines() if row.startswith('    {{69,71},'))
        self.assertEqual(egypt.count('RTW_DFS_REGULATORY_UNAVAILABLE'), 11)
        self.assertTrue(egypt.startswith('    {{69,71},{2000,2000,2000,2000,'))

if __name__ == '__main__':
    unittest.main()
