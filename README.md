# VetalMed ERP AI Assistant System

## Overview

This repository contains a hybrid ERP AI assistant system where the entire backend and AI layer are implemented in C++.

Architecture:
- React frontend for user interaction
- C++ REST backend using Drogon
- C++ AI core for intent parsing, retrieval, structured processing, and response composition
- SQLite storage for file metadata and contextual content
- Optional FAISS vector storage support via compile-time feature

## Build Instructions

### Prerequisites
- C++17 compiler
- CMake 3.16+
- Drogon
- nlohmann_json
- SQLite3
- Node.js / npm for frontend

### Build backend

```bash
cd "c:/Users/rithi/OneDrive/Documents/vetalmed project"
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Run backend

```bash
./core/erp_ai_backend
```

### Frontend

```bash
cd frontend
npm install
npm run dev
```

## API Endpoints

- `GET /health`
- `POST /upload`
- `POST /chat`
- `GET /datasets`
