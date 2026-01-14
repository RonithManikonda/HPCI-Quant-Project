# Order Book

Minimal in-memory limit order book with a small demo and basic tests.

Build:
```
cmake -S . -B build
cmake --build build
```

Run demo:
```
./build/ob_demo
```

Run tests:
```
./build/ob_tests
```

Ingest scaffold (SoupBinTCP + ITCH 5.0 skeleton):
```
./build/ob_itch_ingest --help
```

Optional .env settings (copy from .env.example):
- `OB_HOST`, `OB_PORT`, `OB_USER`, `OB_PASS`, `OB_SESSION`
- `OB_SEQ`, `OB_FRAMES`, `OB_NO_LOGIN`, `OB_VERBOSE`
- `OB_ITCH_FILE`

Examples:
```
./build/ob_itch_ingest --host HOST --port PORT --user USER --pass PASS --session SESSION --frames 5 --verbose
./build/ob_itch_ingest --file /path/to/ITCH_5.0.bin
```
