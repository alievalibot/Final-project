"""Run with python test_assembler.py (requires gcc or clang on PATH)."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent


class AssemblerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = shutil.which("gcc") or shutil.which("clang")
        if not compiler:
            raise unittest.SkipTest("gcc or clang is required")
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        cls.binary = Path(cls.temp.name) / "assembler.exe"
        subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                        str(ROOT / "main.c"), str(ROOT / "scanner.c"),
                        str(ROOT / "parser.c"), "-o", str(cls.binary)], check=True)

    def assemble(self, source):
        path = Path(self.temp.name) / "input.asm"
        path.write_bytes(source.encode("ascii"))
        return subprocess.run([str(self.binary), str(path)],
                              capture_output=True, text=True)

    def test_all_operations_and_destinations(self):
        operations = {"or": 0, "add": 1, "sub": 2, "nand": 3,
                      "and": 4, "xor": 6, "eq": 7}
        source, expected = [], []
        for name, opcode in operations.items():
            for dest in range(1, 7):
                source.append(f"{name} reg{dest}, reg1, reg2")
                expected.append(f"{0x40 | (dest << 3) | opcode:08b}\n")
        result = self.assemble("\n".join(source))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, "")
        self.assertEqual(result.stdout, "".join(expected))

    def test_existing_moves_comments_and_line_endings(self):
        result = self.assemble("; comment\r\nmov reg1, 63 ; maximum\r\n"
                               "mov reg5, reg1\r\nmov reg2, reg3")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "00111111\n10001101\n10011010\n")

    def test_multiple_errors_and_recovery(self):
        result = self.assemble("mov reg1,\nadd reg3, reg1, reg2\n"
                               "unknown reg1\nmov reg2, 10\n"
                               "xor reg3, reg2, reg1\nmov reg1, 64\n"
                               "and reg3, reg1, reg2 extra\n@\n"
                               "mov reg1, 999999999999999999999999\nsub")
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(result.stdout, "")
        for line in (1, 3, 4, 5, 6, 7, 8, 9, 10):
            self.assertIn(f"Error on line {line}:", result.stderr)
        self.assertEqual(result.stderr.count("Error on line"), 9)

    def test_empty_input(self):
        result = self.assemble("")
        self.assertEqual((result.returncode, result.stdout, result.stderr), (0, "", ""))

    def test_output_capacity(self):
        result = self.assemble("mov reg1, 0\n" * 255)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "00000000\n" * 255)
        result = self.assemble("mov reg1, 0\n" * 256)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(result.stdout, "")
        self.assertIn("Error on line 256:", result.stderr)


if __name__ == "__main__":
    unittest.main()
