 #!/usr/bin/env bash
 # Simple macOS build helper for BIT Anomalies – 夜市
 
 set -e
 
 echo "Building night_market with Makefile..."
 make
 
 echo "Done. Run with:"
 echo "  ./night_market"
