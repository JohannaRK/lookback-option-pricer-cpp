#!/bin/bash

# Vai nella cartella del progetto
cd "$(dirname "$0")"

# Esegui il programma C++
./lookback --from-csv inputs.csv --out output/price_delta.csv